/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */

//
//  FILE:    CvAStar.cpp
//
//  AUTHOR:  Casey O'Toole  --  8/27/2002
//  MOD:     Brian Wade     --  5/20/2008
//  MOD:     Ed Beach       --  4/16/2009 moved into CvGameCoreDLL
//

#include "CvGameCoreDLLPCH.h"
#include "CvGameCoreUtils.h"
#include "CvAStar.h"
#include "ICvDLLUserInterface.h"
#include "CvMinorCivAI.h"
#include "CvDllInterfaces.h"
#include "cvStopWatch.h"
#include "CvUnitMovement.h"

#define PATH_MOVEMENT_WEIGHT									(1000)
#ifdef AUI_ASTAR_AVOID_RIVER_CROSSING_WHEN_ATTACKING
#define PATH_RIVER_WEIGHT										(1)
#else
#define PATH_RIVER_WEIGHT										(100)
#endif
#define PATH_CITY_WEIGHT										(0) // slewis - reduced this to zero because we shouldn't avoid cities any more due to new garrison rules
#define PATH_DEFENSE_WEIGHT										(10)
#ifdef AUI_ASTAR_CONSIDER_DAMAGE_WHEN_ATTACKING
#define PATH_DAMAGE_WEIGHT										(5)
#endif
#define PATH_TERRITORY_WEIGHT									(3)
#define PATH_STEP_WEIGHT										(2)
#define PATH_STRAIGHT_WEIGHT									(1)
#define PATH_PARTIAL_MOVE_WEIGHT								(500)
#define PATH_THROUGH_WATER										(1000)
#define PATH_STACKING_WEIGHT									(1000000)
#define PATH_CITY_AVOID_WEIGHT									(0) // slewis - reduced this to zero because we shouldn't avoid cities any more due to new garrison rules
#define	PATH_EXPLORE_NON_HILL_WEIGHT							(300)
#define PATH_EXPLORE_NON_REVEAL_WEIGHT							(10)
#define PATH_INCORRECT_EMBARKING_WEIGHT							(1000000)
#define PATH_BUILD_ROUTE_EXISTING_ROUTE_WEIGHT					(10)
//#define PATH_BUILD_ROUTE_RESOURCE_WEIGHT						(2)
#define PATH_BUILD_ROUTE_REMOVABLE_FEATURE_DISCOUNT				(0.8f)
#define PATH_BUILD_ROUTE_ALREADY_FLAGGED_DISCOUNT				(0.5f)
#define PATH_END_TURN_MOUNTAIN_WEIGHT							(1000000)
#define PATH_END_TURN_MISSIONARY_OTHER_TERRITORY				(150000)

#include <xmmintrin.h>

// until Tim is finished with AStar optimization
#define LINT_EXTRA_SUPPRESSIONS \
	4100 /* unreferenced formal parameter */ \
	4130 /* logical operation on address of a string constant */ \
	4189 /* local variable is initialized but not referenced */ \
	4239 /* nonstandard extension used */ \
	4238 /* nonstandard extension used : class rvalue used as lvalue */ \
	4505 /* unreferenced formal parameter */ \
	4512 /* assignment operator could not be generated */ \
	4702 /* unreachable code */ \
	4706 /* assignment within conditional expression */ \
	6001 /* Using uninitialized memory */ \
	6011 /* dereferencing NULL pointer.  */ \
	6246 /* Local declaration of 'variable' hides declaration of the same name in outer scope. For additional information, see previous declaration at line 'XXX'*/ \
	6262 /* Function uses 'xxxxx' bytes of stack: exceeds /analyze:stacksize'xxxxx'. Consider moving some data to heap */ \
	6302 /* Format string mismatch */ \
	6385 /* invalid data: accessing <buffer name>, the readable size is <size1> bytes, but <size2> bytes may be read: Lines: x, y */ \
	6386 /* Buffer overrun */
#include "LintFree.h"

#define PREFETCH_FASTAR_NODE(x) _mm_prefetch((const char*)x,  _MM_HINT_T0 ); _mm_prefetch(((const char*)x)+64,  _MM_HINT_T0 );
#define PREFETCH_FASTAR_CVPLOT(x) _mm_prefetch((const char*)x,  _MM_HINT_T0 ); _mm_prefetch(((const char*)x)+64,  _MM_HINT_T0 );
static void PrefetchRegionCvAStar(const char* pHead, const uint uiSize)
{
	const uint uiLoop = (uiSize+(64-1))/64;
	for(uint iCurr = 0; iCurr<uiLoop; ++iCurr)
	{
		_mm_prefetch(pHead+(iCurr*64),  _MM_HINT_T0);
	}
}

#if !defined(FINAL_RELESE)
//#define PATH_FINDER_LOGGING
#endif

//	--------------------------------------------------------------------------------
/// Constructor
CvAStar::CvAStar()
{
	udIsPathDest = NULL;
	udDestValid = NULL;
	udHeuristic = NULL;
	udCost = NULL;
	udValid = NULL;
	udNotifyChild = NULL;
	udNotifyList = NULL;
	udNumExtraChildrenFunc = NULL;
	udGetExtraChildFunc = NULL;
	udInitializeFunc = NULL;
	udUninitializeFunc = NULL;

	m_pData = NULL;
#ifdef AUI_ASTAR_USE_DELEGATES
	m_iData = 0;
#endif

	m_pOpen = NULL;
	m_pOpenTail = NULL;
	m_pClosed = NULL;
	m_pBest = NULL;
	m_pStackHead = NULL;

	m_ppaaNodes = NULL;

	m_bIsMPCacheSafe = false;
	m_bDataChangeInvalidatesCache = false;
#ifdef AUI_ASTAR_SCRATCH_BUFFER_INSTANTIATED
	m_ScratchBuffer = NULL;
#endif
}

//	--------------------------------------------------------------------------------
/// Destructor
CvAStar::~CvAStar()
{
	DeInit();
}

//	--------------------------------------------------------------------------------
/// Frees allocated memory
void CvAStar::DeInit()
{
	if(m_ppaaNodes != NULL)
	{
		for(int iI = 0; iI < m_iColumns; iI++)
		{
			FFREEALIGNED(m_ppaaNodes[iI]);
		}

		FFREEALIGNED(m_ppaaNodes);
		m_ppaaNodes=0;
	}
#ifdef AUI_ASTAR_SCRATCH_BUFFER_INSTANTIATED
	SAFE_DELETE_ARRAY(m_ScratchBuffer);
#endif
}

//	--------------------------------------------------------------------------------
/// Initializes the AStar algorithm
#ifdef AUI_ASTAR_USE_DELEGATES
void CvAStar::Initialize(int iColumns, int iRows, bool bWrapX, bool bWrapY, CvAPointFunc IsPathDestFunc, CvAPointFunc DestValidFunc, CvAHeuristic HeuristicFunc, CvAStarFunc CostFunc, CvAStarFunc ValidFunc, CvAStarFunc NotifyChildFunc, CvAStarFunc NotifyListFunc, CvANumExtraChildren NumExtraChildrenFunc, CvAGetExtraChild GetExtraChildFunc, CvABeginOrEnd InitializeFunc, CvABeginOrEnd UninitializeFunc)
#else
void CvAStar::Initialize(int iColumns, int iRows, bool bWrapX, bool bWrapY, CvAPointFunc IsPathDestFunc, CvAPointFunc DestValidFunc, CvAHeuristic HeuristicFunc, CvAStarFunc CostFunc, CvAStarFunc ValidFunc, CvAStarFunc NotifyChildFunc, CvAStarFunc NotifyListFunc, CvANumExtraChildren NumExtraChildrenFunc, CvAGetExtraChild GetExtraChildFunc, CvABegin InitializeFunc, CvAEnd UninitializeFunc, const void* pData)
#endif
{
	int iI, iJ;

	DeInit();	// free old memory just in case

	udIsPathDest = IsPathDestFunc;
	udDestValid = DestValidFunc;
	udHeuristic = HeuristicFunc;
	udCost = CostFunc;
	udValid = ValidFunc;
	udNotifyChild = NotifyChildFunc;
	udNotifyList = NotifyListFunc;
	udNumExtraChildrenFunc = NumExtraChildrenFunc;
	udGetExtraChildFunc = GetExtraChildFunc;
	udInitializeFunc = InitializeFunc;
	udUninitializeFunc = UninitializeFunc;

#ifndef AUI_ASTAR_USE_DELEGATES
	m_pData = pData;
#endif

	m_iColumns = iColumns;
	m_iRows = iRows;

	m_iXstart = -1;
	m_iYstart = -1;
	m_iXdest = -1;
	m_iYdest = -1;
	m_iInfo = 0;

	m_bWrapX = bWrapX;
	m_bWrapY = bWrapY;
	m_bForceReset = false;

	m_pOpen = NULL;
	m_pOpenTail = NULL;
	m_pClosed = NULL;
	m_pBest = NULL;
	m_pStackHead = NULL;

	m_ppaaNodes = reinterpret_cast<CvAStarNode**>(FMALLOCALIGNED(sizeof(CvAStarNode*)*m_iColumns, 64, c_eCiv5GameplayDLL, 0));
	for(iI = 0; iI < m_iColumns; iI++)
	{
		m_ppaaNodes[iI] = reinterpret_cast<CvAStarNode*>(FMALLOCALIGNED(sizeof(CvAStarNode)*m_iRows, 64, c_eCiv5GameplayDLL, 0));
		for(iJ = 0; iJ < m_iRows; iJ++)
		{
			new(&m_ppaaNodes[iI][iJ]) CvAStarNode();
			m_ppaaNodes[iI][iJ].m_iX = iI;
			m_ppaaNodes[iI][iJ].m_iY = iJ;
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
			m_ppaaNodes[iI][iJ].m_pPlot = GC.getMap().plot(iI, iJ);
#endif
		}
	}

#ifdef AUI_ASTAR_PRECALCULATE_NEIGHBORS_ON_INITIALIZE
	for (iI = 0; iI < m_iColumns; iI++)
		for (iJ = 0; iJ < m_iRows; iJ++)
			PrecalcNeighbors(&(m_ppaaNodes[iI][iJ]));
#endif

#ifdef AUI_ASTAR_SCRATCH_BUFFER_INSTANTIATED
	if (udInitializeFunc)
		m_ScratchBuffer = FNEW(char[SCRATCH_BUFFER_SIZE], c_eCiv5GameplayDLL, 0);
#endif

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	m_bIsMultiplayer = GC.getGame().isNetworkMultiPlayer();
#endif
}

//	--------------------------------------------------------------------------------
/// Generates a path from iXstart,iYstart to iXdest,iYdest
bool CvAStar::GeneratePath(int iXstart, int iYstart, int iXdest, int iYdest, int iInfo, bool bReuse)
{
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	bool discardCacheForMPGame = m_bIsMultiplayer && !m_bIsMPCacheSafe;
#else
	CvAStarNode* temp;
	int retval;

	const CvGame& game = GC.getGame();
	bool isMultiplayer = game.isNetworkMultiPlayer();
	bool discardCacheForMPGame = isMultiplayer && !m_bIsMPCacheSafe;
#endif

	if(m_bForceReset || (m_iXstart != iXstart) || (m_iYstart != iYstart) || (m_iInfo != iInfo) || discardCacheForMPGame)
		bReuse = false;

	m_iXdest = iXdest;
	m_iYdest = iYdest;
	m_iXstart = iXstart;
	m_iYstart = iYstart;
	m_iInfo = iInfo;

#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	if (udInitializeFunc)
		udInitializeFunc(m_pData, this);
#endif

	if(!isValid(iXstart, iYstart))
	{
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
		if (udUninitializeFunc)
			udUninitializeFunc(m_pData, this);
#endif
		return false;
	}

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	if (udInitializeFunc)
#ifdef AUI_ASTAR_USE_DELEGATES
		udInitializeFunc();
#else
		udInitializeFunc(m_pData, this);
#endif

	CvAStarNode* temp;
#else
	PREFETCH_FASTAR_NODE(&(m_ppaaNodes[iXdest][iYdest]));
#endif

	if(!bReuse)
	{
		// XXX should we just be doing a memset here?
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
		while (m_pOpen)
		{
			temp = m_pOpen->m_pNext;
			m_pOpen->clear();
			m_pOpen = temp;
		}
		while(m_pClosed)
		{
			temp = m_pClosed->m_pNext;
			m_pClosed->clear();
			m_pClosed = temp;
		}
#else
		if(m_pOpen)
		{
			while(m_pOpen)
			{
				temp = m_pOpen->m_pNext;
				m_pOpen->clear();
				m_pOpen = temp;
			}
		}

		if(m_pClosed)
		{
			while(m_pClosed)
			{
				temp = m_pClosed->m_pNext;
				m_pClosed->clear();
				m_pClosed = temp;
			}
		}
#endif

#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
		PREFETCH_FASTAR_NODE(&(m_ppaaNodes[iXstart][iYstart]));
#endif

		m_pBest = NULL;
		m_pStackHead = NULL;

		m_bForceReset = false;

		temp = &(m_ppaaNodes[iXstart][iYstart]);

#ifdef AUI_ASTAR_FIX_POSSIBLE_NULL_POINTERS
		temp->clear();
#else
		temp->m_iKnownCost = 0;
#endif
#ifdef AUI_ASTAR_USE_DELEGATES
		temp->m_iHeuristicCost = UDHEUR(udHeuristic, m_iXstart, m_iYstart, m_iXdest, m_iYdest);
		temp->m_iTotalCost = temp->m_iHeuristicCost;
#else
		if(udHeuristic == NULL)
		{
			temp->m_iHeuristicCost = 0;
		}
		else
		{
			temp->m_iHeuristicCost = udHeuristic(m_iXstart, m_iYstart, m_iXdest, m_iYdest);
		}
		temp->m_iTotalCost = temp->m_iKnownCost + temp->m_iHeuristicCost;
#endif

		m_pOpen = temp;
		m_pOpenTail = temp;

#ifdef AUI_ASTAR_USE_DELEGATES
		UDFUNC(udNotifyList, NULL, m_pOpen, ASNL_STARTOPEN);
		UDFUNC(udValid, NULL, temp, 0);
		UDFUNC(udNotifyChild, NULL, temp, ASNC_INITIALADD);
#else
		udFunc(udNotifyList, NULL, m_pOpen, ASNL_STARTOPEN, m_pData);
		udFunc(udValid, NULL, temp, 0, m_pData);
		udFunc(udNotifyChild, NULL, temp, ASNC_INITIALADD, m_pData);
#endif
	}

	if(udDestValid != NULL)
	{
#ifdef AUI_ASTAR_USE_DELEGATES
		if (!udDestValid(iXdest, iYdest))
#else
		if(!udDestValid(iXdest, iYdest, m_pData, this))
#endif
		{
			if (udUninitializeFunc)
#ifdef AUI_ASTAR_USE_DELEGATES
				udUninitializeFunc();
#else
				udUninitializeFunc(m_pData, this);
#endif
			return false;
		}
	}

	if(isValid(m_iXdest, m_iYdest))
	{
		temp = &(m_ppaaNodes[m_iXdest][m_iYdest]);

		if(temp->m_eCvAStarListType == CVASTARLIST_CLOSED)
		{
			m_pBest = temp;
			if (udUninitializeFunc)
#ifdef AUI_ASTAR_USE_DELEGATES
				udUninitializeFunc();
#else
				udUninitializeFunc(m_pData, this);
#endif
			return true;
		}
	}

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	int retval = 0;
#else
	retval = 0;
#endif

	while(retval == 0)
	{
		retval = Step();
	}

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	if (udUninitializeFunc)
#ifdef AUI_ASTAR_USE_DELEGATES
		udUninitializeFunc();
#else
		udUninitializeFunc(m_pData, this);
#endif
#endif

	if(retval == -1)
	{
		assert(m_pBest == NULL);
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
		if (udUninitializeFunc)
#ifdef AUI_ASTAR_USE_DELEGATES
			udUninitializeFunc();
#else
			udUninitializeFunc(m_pData, this);
#endif
#endif
		return false;
	}

#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	if (udUninitializeFunc)
#ifdef AUI_ASTAR_USE_DELEGATES
		udUninitializeFunc();
#else
		udUninitializeFunc(m_pData, this);
#endif
#endif
	return true;
}

//	--------------------------------------------------------------------------------
/// Takes one step in the algorithm
int CvAStar::Step()
{
	if((m_pBest = GetBest()) == NULL)
	{
		return -1;
	}

	CreateChildren(m_pBest); // needs to be done, even on the last node, to allow for re-use...

	if (m_pBest == NULL)	// There seems to be a case were this will come back NULL.
		return -1;

	if(IsPathDest(m_pBest->m_iX, m_pBest->m_iY))
	{
		return 1;
	}

	return 0;
}

//	--------------------------------------------------------------------------------
/// Returns best node
CvAStarNode* CvAStar::GetBest()
{
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	CvAStarNode* temp;
#endif

	if(!m_pOpen)
	{
		return NULL;
	}

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvAStarNode* temp = m_pOpen;
#else
	temp = m_pOpen;
#endif

	m_pOpen = temp->m_pNext;
	if(m_pOpen != NULL)
	{
		m_pOpen->m_pPrev = NULL;
	}
	else
	{
		m_pOpenTail = NULL;
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	UDFUNC(udNotifyList, NULL, temp, ASNL_DELETEOPEN);
#else
	udFunc(udNotifyList, NULL, temp, ASNL_DELETEOPEN, m_pData);
#endif

	temp->m_eCvAStarListType = CVASTARLIST_CLOSED;

	temp->m_pNext = m_pClosed;
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	temp->m_pPrev = NULL;
#else
	if(m_pClosed != NULL)
	{
		m_pClosed->m_pPrev = temp;
	}
#endif
	m_pClosed = temp;

#ifdef AUI_ASTAR_USE_DELEGATES
	UDFUNC(udNotifyList, NULL, m_pClosed, ASNL_ADDCLOSED);
#else
	udFunc(udNotifyList, NULL, m_pClosed, ASNL_ADDCLOSED, m_pData);
#endif

	return temp;
}

#ifdef AUI_ASTAR_PRECALCULATE_NEIGHBORS_ON_INITIALIZE
// --------------------
/// precompute neighbors for a node
void CvAStar::PrecalcNeighbors(CvAStarNode* node) const
{
	int x, y;

	static const int s_CvAStarChildHexX[NUM_DIRECTION_TYPES] = { 0, 1, 1, 0, -1, -1 };
	static const int s_CvAStarChildHexY[NUM_DIRECTION_TYPES] = { 1, 0, -1, -1, 0, 1 };

	for (int i = 0; i < NUM_DIRECTION_TYPES; i++)
	{
		x = node->m_iX - ((node->m_iY >= 0) ? (node->m_iY >> 1) : ((node->m_iY - 1) / 2));
		x += s_CvAStarChildHexX[i];
		y = yRange(node->m_iY + s_CvAStarChildHexY[i]);
		x += ((y >= 0) ? (y >> 1) : ((y - 1) / 2));
		x = xRange(x);

		if (isValid(x, y))
			node->m_apNeighbors[i] = &(m_ppaaNodes[x][y]);
		else
			node->m_apNeighbors[i] = NULL;
	}
}

//	--------------------------------------------------------------------------------
/// Creates children for the node
void CvAStar::CreateChildren(CvAStarNode* node)
{
	CvAStarNode* check;

	for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		check = node->m_apNeighbors[iI];

#ifdef AUI_ASTAR_USE_DELEGATES
		if (check && UDFUNC(udValid, node, check, 0))
#else
		if (check && udFunc(udValid, node, check, 0, m_pData))
#endif
		{
			LinkChild(node, check);
		}
	}

	if (udNumExtraChildrenFunc && udGetExtraChildFunc)
	{
#ifdef AUI_ASTAR_USE_DELEGATES
		const uint uiNumExtraChildren = udNumExtraChildrenFunc(node);
		for (uint uiI = 0; uiI < uiNumExtraChildren; uiI++)
		{
			check = udGetExtraChildFunc(node, uiI);
			if (UDFUNC(udValid, node, check, 0))
			{
				LinkChild(node, check);
			}
#else
		int x, y;
		int iExtraChildren = udNumExtraChildrenFunc(node, this);
		for (int iI = 0; iI < iExtraChildren; iI++)
		{
			udGetExtraChildFunc(node, iI, x, y, this);
			PREFETCH_FASTAR_NODE(&(m_ppaaNodes[x][y]));

			if (isValid(x, y))
			{
				check = &(m_ppaaNodes[x][y]);

				if (udFunc(udValid, node, check, 0, m_pData))
				{
#ifdef AUI_USE_OPENMP
#pragma omp critical(LinkChild)
#endif
					LinkChild(node, check);
				}
			}
#endif
		}
	}
}
#else
//	--------------------------------------------------------------------------------
/// Creates children for the node
void CvAStar::CreateChildren(CvAStarNode* node)
{
	CvAStarNode* check;
	int range = 6;
	int x, y;
	int i;

	static int s_CvAStarChildHexX[6] = { 0, 1,  1,  0, -1, -1, };
	static int s_CvAStarChildHexY[6] = { 1, 0, -1, -1,  0,  1, };

	for(i = 0; i < range; i++)
	{
		x = node->m_iX - ((node->m_iY >= 0) ? (node->m_iY>>1) : ((node->m_iY - 1)/2));
		x += s_CvAStarChildHexX[i];
		y = yRange(node->m_iY + s_CvAStarChildHexY[i]);
		x += ((y >= 0) ? (y>>1) : ((y - 1)/2));
		x = xRange(x);

		PREFETCH_FASTAR_NODE(&(m_ppaaNodes[x][y]));
		if(isValid(x, y))
		{
			check = &(m_ppaaNodes[x][y]);

			if(udFunc(udValid, node, check, 0, m_pData))
			{
				LinkChild(node, check);
			}
		}
	}

	if(udNumExtraChildrenFunc && udGetExtraChildFunc)
	{
		int iExtraChildren = udNumExtraChildrenFunc(node, this);
		for(int i = 0; i < iExtraChildren; i++)
		{
			udGetExtraChildFunc(node, i, x, y, this);
			PREFETCH_FASTAR_NODE(&(m_ppaaNodes[x][y]));

			if(isValid(x, y))
			{
				check = &(m_ppaaNodes[x][y]);

				if(udFunc(udValid, node, check, 0, m_pData))
				{
					LinkChild(node, check);
				}
			}
		}
	}
}
#endif

//	--------------------------------------------------------------------------------
/// Link in a child
void CvAStar::LinkChild(CvAStarNode* node, CvAStarNode* check)
{
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
#ifdef AUI_ASTAR_USE_DELEGATES
	int iKnownCost = node->m_iKnownCost + UDFUNC(udCost, node, check, 0);
#else
	int iKnownCost = node->m_iKnownCost + udFunc(udCost, node, check, 0, m_pData);
#endif
#else
	int iKnownCost;

#ifdef AUI_ASTAR_USE_DELEGATES
	iKnownCost = node->m_iKnownCost + UDFUNC(udCost, node, check, 0);
#else
	iKnownCost = node->m_iKnownCost + udFunc(udCost, node, check, 0, m_pData);
#endif
#endif

	if(check->m_eCvAStarListType == CVASTARLIST_OPEN)
	{
		node->m_apChildren.push_back(check);
		node->m_iNumChildren++;

		if(iKnownCost < check->m_iKnownCost)
		{
			FAssert(node->m_pParent != check);

			check->m_pParent = node;
			check->m_iKnownCost = iKnownCost;
			check->m_iTotalCost = iKnownCost + check->m_iHeuristicCost;

#ifdef AUI_ASTAR_USE_DELEGATES
			UpdateOpenNode(check);
			UDFUNC(udNotifyChild, node, check, ASNC_OPENADD_UP);
#else
			UpdateOpenNode(check);
			udFunc(udNotifyChild, node, check, ASNC_OPENADD_UP, m_pData);
#endif
		}
	}
	else if(check->m_eCvAStarListType == CVASTARLIST_CLOSED)
	{
		node->m_apChildren.push_back(check);
		node->m_iNumChildren++;

		if(iKnownCost < check->m_iKnownCost)
		{
			FAssert(node->m_pParent != check);
			check->m_pParent = node;
			check->m_iKnownCost = iKnownCost;
			check->m_iTotalCost = iKnownCost + check->m_iHeuristicCost;
#ifdef AUI_ASTAR_USE_DELEGATES
			UDFUNC(udNotifyChild, node, check, ASNC_CLOSEDADD_UP);
#else
			udFunc(udNotifyChild, node, check, ASNC_CLOSEDADD_UP, m_pData);
#endif

			UpdateParents(check);
		}
	}
	else
	{
		FAssert(check->m_eCvAStarListType == NO_CVASTARLIST);
		FAssert(node->m_pParent != check);
		check->m_pParent = node;
		check->m_iKnownCost = iKnownCost;
#ifdef AUI_ASTAR_USE_DELEGATES
		check->m_iHeuristicCost = UDHEUR(udHeuristic, check->m_iX, check->m_iY, m_iXdest, m_iYdest);
		check->m_iTotalCost = check->m_iKnownCost + check->m_iHeuristicCost;
		UDFUNC(udNotifyChild, node, check, ASNC_NEWADD);
#else
		if(udHeuristic == NULL)
		{
			check->m_iHeuristicCost = 0;
		}
		else
		{
			check->m_iHeuristicCost = udHeuristic(check->m_iX, check->m_iY, m_iXdest, m_iYdest);
		}
		check->m_iTotalCost = check->m_iKnownCost + check->m_iHeuristicCost;

		udFunc(udNotifyChild, node, check, ASNC_NEWADD, m_pData);
#endif

		AddToOpen(check);

		node->m_apChildren.push_back(check);
		node->m_iNumChildren++;
	}
}

//	--------------------------------------------------------------------------------
/// Add node to open list
void CvAStar::AddToOpen(CvAStarNode* addnode)
{
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	CvAStarNode* node;
#endif

	addnode->m_eCvAStarListType = CVASTARLIST_OPEN;

	if(!m_pOpen)
	{
		m_pOpen = addnode;
		m_pOpenTail = addnode;
		m_pOpen->m_pNext = NULL;
		m_pOpen->m_pPrev = NULL;

#ifdef AUI_ASTAR_USE_DELEGATES
		UDFUNC(udNotifyList, NULL, addnode, ASNL_STARTOPEN);
#else
		udFunc(udNotifyList, NULL, addnode, ASNL_STARTOPEN, m_pData);
#endif

		return;
	}
#ifdef AUI_ASTAR_FIX_POSSIBLE_NULL_POINTERS
	else if (!m_pOpenTail)
	{
		CvAStarNode* temp = m_pOpen;
		while (temp->m_pNext)
		{
			temp = temp->m_pNext;
		}
		m_pOpenTail = temp;
	}

	if(addnode->m_iTotalCost < m_pOpen->m_iTotalCost)
#else
	if(addnode->m_iTotalCost <= m_pOpen->m_iTotalCost)
#endif
	{
		addnode->m_pNext = m_pOpen;
		m_pOpen->m_pPrev = addnode;
		m_pOpen = addnode;

#ifdef AUI_ASTAR_USE_DELEGATES
		UDFUNC(udNotifyList, m_pOpen->m_pNext, m_pOpen, ASNL_STARTOPEN);
#else
		udFunc(udNotifyList, m_pOpen->m_pNext, m_pOpen, ASNL_STARTOPEN, m_pData);
#endif
	}
	else if(addnode->m_iTotalCost >= m_pOpenTail->m_iTotalCost)
	{
		addnode->m_pPrev = m_pOpenTail;
		m_pOpenTail->m_pNext = addnode;
		m_pOpenTail = addnode;

#ifdef AUI_ASTAR_USE_DELEGATES
		UDFUNC(udNotifyList, addnode->m_pPrev, addnode, ASNL_ADDOPEN);
#else
		udFunc(udNotifyList, addnode->m_pPrev, addnode, ASNL_ADDOPEN, m_pData);
#endif
	}
	else if(abs(addnode->m_iTotalCost-m_pOpenTail->m_iTotalCost) < abs(addnode->m_iTotalCost-m_pOpen->m_iTotalCost))  //(addnode->m_iTotalCost > m_iOpenListAverage) // let's start at the end and work forwards
	{
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
		CvAStarNode* next = NULL;
		CvAStarNode* node = m_pOpenTail;
#else
		CvAStarNode* next;
		node = m_pOpenTail;
		next = NULL;
#endif

		while(node)
		{
			if(addnode->m_iTotalCost < node->m_iTotalCost)
			{
				next = node;
				node = node->m_pPrev;
			}
			else
			{
				if(next)
				{
					next->m_pPrev = addnode;
					addnode->m_pNext = next;
					addnode->m_pPrev = node;
					node->m_pNext = addnode;
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
					if(node->m_pNext == NULL)
					{
						m_pOpenTail = node;
					}
#endif

#ifdef AUI_ASTAR_USE_DELEGATES
					UDFUNC(udNotifyList, addnode->m_pPrev, addnode, ASNL_ADDOPEN);
#else
					udFunc(udNotifyList, addnode->m_pPrev, addnode, ASNL_ADDOPEN, m_pData);
#endif
				}
				else // we should just add it to the end of the list
				{
					addnode->m_pPrev = m_pOpenTail;
					m_pOpenTail->m_pNext = addnode;
					m_pOpenTail = addnode;

#ifdef AUI_ASTAR_USE_DELEGATES
					UDFUNC(udNotifyList, addnode->m_pPrev, addnode, ASNL_ADDOPEN);
#else
					udFunc(udNotifyList, addnode->m_pPrev, addnode, ASNL_ADDOPEN, m_pData);
#endif
				}

				return;
			}
		}

		// we made it to the start of this list - insert it at the beginning - we shouldn't ever get here, but...
		next->m_pPrev = addnode;
		addnode->m_pNext = next;
		m_pOpen = addnode;

#ifdef AUI_ASTAR_USE_DELEGATES
		UDFUNC(udNotifyList, m_pOpen->m_pNext, m_pOpen, ASNL_STARTOPEN);
#else
		udFunc(udNotifyList, m_pOpen->m_pNext, m_pOpen, ASNL_STARTOPEN, m_pData);
#endif
	}
	else // let's start at the beginning as it should be closer
	{
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
		CvAStarNode* node = m_pOpen;
		CvAStarNode* prev = NULL;
#else
		CvAStarNode* prev;
		node = m_pOpen;
		prev = NULL;
#endif

		while(node)
		{
#ifdef AUI_ASTAR_FIX_POSSIBLE_NULL_POINTERS
			if (addnode->m_iTotalCost >= node->m_iTotalCost)
#else
			if(addnode->m_iTotalCost > node->m_iTotalCost)
#endif
			{
				prev = node;
				node = node->m_pNext;
			}
			else
			{
				if(prev)
				{
					prev->m_pNext = addnode;
					addnode->m_pPrev = prev;
					addnode->m_pNext = node;
					node->m_pPrev = addnode;
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
					if(node->m_pNext == NULL)
					{
						m_pOpenTail = node;
					}
#endif

#ifdef AUI_ASTAR_USE_DELEGATES
					UDFUNC(udNotifyList, prev, addnode, ASNL_ADDOPEN);
#else
					udFunc(udNotifyList, prev, addnode, ASNL_ADDOPEN, m_pData);
#endif
				}
				else
				{
					addnode->m_pNext = m_pOpen;
					m_pOpen->m_pPrev = addnode;
					m_pOpen = addnode;

#ifdef AUI_ASTAR_USE_DELEGATES
					UDFUNC(udNotifyList, m_pOpen->m_pNext, m_pOpen, ASNL_STARTOPEN);
#else
					udFunc(udNotifyList, m_pOpen->m_pNext, m_pOpen, ASNL_STARTOPEN, m_pData);
#endif
				}

				return;
			}
		}

		// we made it to the end of this list - insert it at the end - we shouldn't ever get here, but...
		prev->m_pNext = addnode;
		addnode->m_pPrev = prev;
		m_pOpenTail = addnode;

#ifdef AUI_ASTAR_USE_DELEGATES
		UDFUNC(udNotifyList, prev, addnode, ASNL_ADDOPEN);
#else
		udFunc(udNotifyList, prev, addnode, ASNL_ADDOPEN, m_pData);
#endif
	}
}

//	--------------------------------------------------------------------------------
/// Connect in a node
void CvAStar::UpdateOpenNode(CvAStarNode* node)
{
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvAStarNode* temp = node->m_pPrev;
#else
	CvAStarNode* temp;
#endif

	FAssert(node->m_eCvAStarListType == CVASTARLIST_OPEN);

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	if (temp && (node->m_iTotalCost < temp->m_iTotalCost))
	{
		// have node free float for now
		temp->m_pNext = node->m_pNext;
		if (node->m_pNext)
		{
			node->m_pNext->m_pPrev = temp;
		}
		else
		{
			m_pOpenTail = temp;
		}
#else
	if((node->m_pPrev != NULL) && (node->m_iTotalCost < node->m_pPrev->m_iTotalCost))
	{
		// have node free float for now
		node->m_pPrev->m_pNext = node->m_pNext;
		if(node->m_pNext)
		{
			node->m_pNext->m_pPrev = node->m_pPrev;
		}
		else
		{
			m_pOpenTail = node->m_pPrev;
		}
#endif
		// scoot down the list till we find where node goes (without connecting up as we go)
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
		temp = node->m_pPrev;
#endif
		while((temp != NULL) && (node->m_iTotalCost < temp->m_iTotalCost))
		{
			temp = temp->m_pPrev;
		}
		// connect node up
		if(temp != NULL)
		{
			node->m_pNext = temp->m_pNext;
			node->m_pPrev = temp;
			if(temp->m_pNext)
			{
				temp->m_pNext->m_pPrev = node;
			}
			temp->m_pNext = node;
		}
		else
		{
			node->m_pNext = m_pOpen;
			node->m_pPrev = NULL;
			if(node->m_pNext)
			{
				node->m_pNext->m_pPrev = node;
			}
			m_pOpen = node;
		}
	}
}

//	--------------------------------------------------------------------------------
/// Refresh parent node (after linking in a child)
void CvAStar::UpdateParents(CvAStarNode* node)
{
	CvAStarNode* kid;
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	CvAStarNode* parent;
#endif
	int iKnownCost;
	int iNumChildren;
	int i;

	FAssert(m_pStackHead == NULL);

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvAStarNode* parent = node;
#else
	parent = node;
#endif

	while(parent != NULL)
	{
		iNumChildren = parent->m_iNumChildren;

		for(i = 0; i < iNumChildren; i++)
		{
			kid = parent->m_apChildren[i];

#ifdef AUI_ASTAR_USE_DELEGATES
			iKnownCost = parent->m_iKnownCost + UDFUNC(udCost, parent, kid, 0);
#else
			iKnownCost = (parent->m_iKnownCost + udFunc(udCost, parent, kid, 0, m_pData));
#endif

			if (iKnownCost < kid->m_iKnownCost)
			{
				kid->m_iKnownCost = iKnownCost;
				kid->m_iTotalCost = kid->m_iKnownCost + kid->m_iHeuristicCost;
				FAssert(parent->m_pParent != kid);
				kid->m_pParent = parent;
				if (kid->m_eCvAStarListType == CVASTARLIST_OPEN)
				{
					UpdateOpenNode(kid);
				}
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
				else
				{
					Push(kid); // Kid cannot be a parent if it's in the open list, since adding children goes through GetBest(), which adds the node to the closed list
				}
#endif
#ifdef AUI_ASTAR_USE_DELEGATES
				UDFUNC(udNotifyChild, parent, kid, ASNC_PARENTADD_UP);
#else
				udFunc(udNotifyChild, parent, kid, ASNC_PARENTADD_UP, m_pData);
#endif

#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
				Push(kid);
#endif
			}
		}

		parent = Pop();
	}
}

//	--------------------------------------------------------------------------------
/// Push a node on the stack
void CvAStar::Push(CvAStarNode* node)
{
	if(node->m_bOnStack)
	{
		return;
	}

#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	if(m_pStackHead == NULL)
	{
		m_pStackHead = &(m_ppaaNodes[node->m_iX][node->m_iY]);
	}
	else
#endif
	{
		FAssert(node->m_pStack == NULL);
		node->m_pStack = m_pStackHead;
		m_pStackHead = node;
	}

	node->m_bOnStack = true;
}

//	--------------------------------------------------------------------------------
/// Pop a node from the stack
CvAStarNode* CvAStar::Pop()
{
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	CvAStarNode* node;
#endif

	if(m_pStackHead == NULL)
	{
		return NULL;
	}

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvAStarNode* node = m_pStackHead;
#else
	node = m_pStackHead;
#endif
	m_pStackHead = m_pStackHead->m_pStack;
	node->m_pStack = NULL;

	node->m_bOnStack = false;

	return node;
}

//C-STYLE NON-MEMBER FUNCTIONS

// A structure holding some unit values that are invariant during a path plan operation
struct UnitPathCacheData
{
	int m_aBaseMoves[NUM_DOMAIN_TYPES];
	int m_iMaxMoves;
	PlayerTypes m_ePlayerID;
	TeamTypes m_eTeamID;
	DomainTypes m_eDomainType;

	bool m_bIsHuman;
	bool m_bIsAutomated;
	bool m_bIsImmobile;
	bool m_bIsNoRevealMap;
	bool m_bCanEverEmbark;
	bool m_bIsEmbarked;
	bool m_bCanAttack;
#ifdef AUI_DANGER_PLOTS_REMADE
	bool m_bDoDanger;
	inline bool DoDanger() const { return m_bDoDanger; }
#endif

	inline int baseMoves(DomainTypes eType) const { return m_aBaseMoves[eType]; }
	inline int maxMoves() const { return m_iMaxMoves; }
	inline PlayerTypes getOwner() const { return m_ePlayerID; }
	inline TeamTypes getTeam() const { return m_eTeamID; }
	inline DomainTypes getDomainType() const { return m_eDomainType; }
	inline bool isHuman() const { return m_bIsHuman; }
	inline bool IsAutomated() const { return m_bIsAutomated; }
	inline bool IsImmobile() const { return m_bIsImmobile; }
	inline bool isNoRevealMap() const { return m_bIsNoRevealMap; }
	inline bool CanEverEmbark() const { return m_bCanEverEmbark; }
	inline bool isEmbarked() const { return m_bIsEmbarked; }
	inline bool IsCanAttack() const { return m_bCanAttack; }
};

//	--------------------------------------------------------------------------------
#ifdef AUI_ASTAR_USE_DELEGATES
void CvAStar::UnitPathInitialize()
{
	const CvUnit* pUnit = m_pData;

	UnitPathCacheData* pCacheData = reinterpret_cast<UnitPathCacheData*>(GetScratchBuffer());
#else
void UnitPathInitialize(const void* pointer, CvAStar* finder)
{
	CvUnit* pUnit = ((CvUnit*)pointer);

	UnitPathCacheData* pCacheData = reinterpret_cast<UnitPathCacheData*>(finder->GetScratchBuffer());
#endif

	for (int i = 0; i < NUM_DOMAIN_TYPES; ++i)
	{
		pCacheData->m_aBaseMoves[i] = pUnit->baseMoves((DomainTypes)i);
	}

	pCacheData->m_iMaxMoves = pUnit->maxMoves();

	pCacheData->m_ePlayerID = pUnit->getOwner();
	pCacheData->m_eTeamID = pUnit->getTeam();
	pCacheData->m_eDomainType = pUnit->getDomainType();
	pCacheData->m_bIsHuman = pUnit->isHuman();
	pCacheData->m_bIsAutomated = pUnit->IsAutomated();
	pCacheData->m_bIsImmobile = pUnit->IsImmobile();
	pCacheData->m_bIsNoRevealMap = pUnit->isNoRevealMap();
	pCacheData->m_bCanEverEmbark = pUnit->CanEverEmbark();
	pCacheData->m_bIsEmbarked = pUnit->isEmbarked();
	pCacheData->m_bCanAttack = pUnit->IsCanAttack();
#ifdef AUI_DANGER_PLOTS_REMADE
#ifdef AUI_ASTAR_USE_DELEGATES
	pCacheData->m_bDoDanger = (pCacheData->m_bIsAutomated || !pCacheData->isHuman()) && (!(GetInfo() & MOVE_UNITS_IGNORE_DANGER)) && (!pUnit->IsCombatUnit() || pUnit->getArmyID() == FFreeList::INVALID_INDEX);
#else
	pCacheData->m_bDoDanger = (pCacheData->m_bIsAutomated || !pCacheData->isHuman()) && (!(finder->GetInfo() & MOVE_UNITS_IGNORE_DANGER)) && (!pUnit->IsCombatUnit() || pUnit->getArmyID() == FFreeList::INVALID_INDEX);
#endif
#endif
}

//	--------------------------------------------------------------------------------
#ifdef AUI_ASTAR_USE_DELEGATES
void CvAStar::UnitPathUninitialize()
#else
void UnitPathUninitialize(const void* pointer, CvAStar* finder)
#endif
{

}

//	--------------------------------------------------------------------------------
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::PathDest(int iToX, int iToY) const
{
	if(iToX == GetDestX() && iToY == GetDestY())
#else
int PathDest(int iToX, int iToY, const void* pointer, CvAStar* finder)
{
	if(iToX == finder->GetDestX() && iToY == finder->GetDestY())
#endif
	{
		return true;
	}
	else
	{
		return false;
	}
}


//	--------------------------------------------------------------------------------
/// Standard path finder - is this end point for the path valid?
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::PathDestValid(int iToX, int iToY)
#else
int PathDestValid(int iToX, int iToY, const void* pointer, CvAStar* finder)
#endif
{
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pToPlot = GC.getMap().plotCheckInvalid(iToX, iToY);
#else
	CvUnit* pUnit;
	CvPlot* pToPlot;
	bool bAIControl;

	pToPlot = GC.getMap().plotCheckInvalid(iToX, iToY);
#endif
	FAssert(pToPlot != NULL);

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
#ifdef AUI_ASTAR_USE_DELEGATES
	const CvUnit* pUnit = m_pData;
#else
	CvUnit* pUnit = (CvUnit*)m_pData;
#endif
	const UnitPathCacheData* pCacheData = reinterpret_cast<const UnitPathCacheData*>(GetScratchBuffer());
#else
	pUnit = ((CvUnit*)pointer);
	const UnitPathCacheData* pCacheData = reinterpret_cast<const UnitPathCacheData*>(finder->GetScratchBuffer());
#endif

	if (pToPlot == NULL || pUnit == NULL)
		return FALSE;

	if (pUnit->plot() == pToPlot)
	{
		return TRUE;
	}

#ifndef AUI_ASTAR_FIX_PATH_VALID_PATH_PEAKS_FOR_NONHUMAN
	if(pToPlot->isMountain() && (!pCacheData->isHuman() || pCacheData->IsAutomated()))
	{
		return FALSE;
	}
#endif

	if (pCacheData->IsImmobile())
	{
		return FALSE;
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	if ((GetInfo() & CvUnit::MOVEFLAG_STAY_ON_LAND) && (pToPlot->isWater() && !pToPlot->IsAllowsWalkWater()))
#else
	if ((finder->GetInfo() & CvUnit::MOVEFLAG_STAY_ON_LAND) && (pToPlot->isWater() && !pToPlot->IsAllowsWalkWater()))
#endif
	{
		return FALSE;
	}

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	bool bAIControl = pCacheData->IsAutomated();
#else
	bAIControl = pCacheData->IsAutomated();
#endif

	if (bAIControl)
	{
#ifndef AUI_ASTAR_FIX_CONSIDER_DANGER_ONLY_PATH
		if(!(finder->GetInfo() & MOVE_UNITS_IGNORE_DANGER))
		{
			if(!pUnit->IsCombatUnit() || pUnit->getArmyID() == FFreeList::INVALID_INDEX)
			{
#ifdef AUI_ASTAR_FIX_CONSIDER_DANGER_USES_COMBAT_STRENGTH
				if (GET_PLAYER(pUnit->getOwner()).GetPlotDanger(*pToPlot) > pUnit->GetBaseCombatStrengthConsideringDamage() * AUI_ASTAR_FIX_CONSIDER_DANGER_USES_COMBAT_STRENGTH)
#else
				if(GET_PLAYER(pUnit->getOwner()).GetPlotDanger(*pToPlot) > 0)
#endif
				{
					return FALSE;
				}
			}
		}
#endif

		if (pCacheData->getDomainType() == DOMAIN_LAND)
		{
			int iGroupAreaID = pUnit->area()->GetID();
			if (pToPlot->getArea() != iGroupAreaID)
			{
				if (!(pToPlot->isAdjacentToArea(iGroupAreaID)) && !pUnit->CanEverEmbark())
				{
					return FALSE;
				}
			}
		}
	}

	TeamTypes eTeam = pCacheData->getTeam();
	bool bToPlotRevealed = pToPlot->isRevealed(eTeam);
	if (!bToPlotRevealed)
	{
		if (pCacheData->isNoRevealMap())
		{
			return FALSE;
		}
	}

	if (bToPlotRevealed)
	{
		CvCity* pCity = pToPlot->getPlotCity();
		if (pCity)
		{
#ifdef AUI_ASTAR_USE_DELEGATES
			if (pCacheData->getOwner() != pCity->getOwner() && !GET_TEAM(eTeam).isAtWar(pCity->getTeam()) && !(GetInfo() & MOVE_IGNORE_STACKING))
#else
			if(pCacheData->getOwner() != pCity->getOwner() && !GET_TEAM(eTeam).isAtWar(pCity->getTeam()) && !(finder->GetInfo() & MOVE_IGNORE_STACKING))
#endif
			{
				return FALSE;
			}
		}
	}

	if (bAIControl || bToPlotRevealed)
	{
		// assume that we can change our embarking state
		byte bMoveFlags = CvUnit::MOVEFLAG_DESTINATION | CvUnit::MOVEFLAG_PRETEND_CORRECT_EMBARK_STATE;

#ifdef AUI_ASTAR_USE_DELEGATES
		if ((pUnit->IsDeclareWar() || (GetInfo() & MOVE_DECLARE_WAR)))
#else
		if((pUnit->IsDeclareWar() || (finder->GetInfo() & MOVE_DECLARE_WAR)))
#endif
		{
			bMoveFlags |= CvUnit::MOVEFLAG_ATTACK;
		}

#ifdef AUI_ASTAR_USE_DELEGATES
		if (GetInfo() & MOVE_IGNORE_STACKING)
#else
		if(finder->GetInfo() & MOVE_IGNORE_STACKING)
#endif
		{
			bMoveFlags |= CvUnit::MOVEFLAG_IGNORE_STACKING;
		}

		if(!(pUnit->canMoveOrAttackInto(*pToPlot, bMoveFlags)))
		{
			return FALSE;
		}
	}
	return TRUE;
}


//	--------------------------------------------------------------------------------
/// Standard path finder - determine heuristic cost
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::PathHeuristic(int iFromX, int iFromY, int iToX, int iToY) const
#else
int PathHeuristic(int iFromX, int iFromY, int iToX, int iToY)
#endif
{
	return (plotDistance(iFromX, iFromY, iToX, iToY) * PATH_MOVEMENT_WEIGHT);
}

//	--------------------------------------------------------------------------------
/// Standard path finder - compute cost of a path
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::PathCost(CvAStarNode* parent, CvAStarNode* node, int data)
#else
int PathCost(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
	CvMap& kMap = GC.getMap();
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	const CvPlot* pFromPlot = parent->m_pPlot;
	const CvPlot* pToPlot = node->m_pPlot;

	const int iFromPlotX = parent->m_iX;
	const int iFromPlotY = parent->m_iY;
	const int iToPlotX = node->m_iX;
	const int iToPlotY = node->m_iY;
#else
	int iFromPlotX = parent->m_iX;
	int iFromPlotY = parent->m_iY;
	CvPlot* pFromPlot = kMap.plotUnchecked(iFromPlotX, iFromPlotY);

	int iToPlotX = node->m_iX;
	int iToPlotY = node->m_iY;
	CvPlot* pToPlot = kMap.plotUnchecked(iToPlotX, iToPlotY);
#endif

#ifdef AUI_ASTAR_USE_DELEGATES
	const CvUnit* pUnit = m_pData;
	const UnitPathCacheData* pCacheData = reinterpret_cast<const UnitPathCacheData*>(GetScratchBuffer());
#else
	CvUnit* pUnit = ((CvUnit*)pointer);
	const UnitPathCacheData* pCacheData = reinterpret_cast<const UnitPathCacheData*>(finder->GetScratchBuffer());
#endif

	DomainTypes eUnitDomain = pCacheData->getDomainType();

	CvAssertMsg(eUnitDomain != DOMAIN_AIR, "pUnit->getDomainType() is not expected to be equal with DOMAIN_AIR");

	bool bToPlotIsWater = pToPlot->isWater() && !pToPlot->IsAllowsWalkWater();
	int iMax;
	if(parent->m_iData1 > 0)
	{
		iMax = parent->m_iData1;
	}
	else
	{
		if (CvUnitMovement::ConsumesAllMoves(pUnit, pFromPlot, pToPlot) || CvUnitMovement::IsSlowedByZOC(pUnit, pFromPlot, pToPlot))
		{
			// The movement would consume all moves, get the moves we will forfeit based on the source plot, rather than
			// the destination plot.  This fixes issues where a land unit that has more movement points on water than on land
			// would have a very high cost to move onto water if their first move of the turn was at the edge of the water.
			iMax = pCacheData->baseMoves((pFromPlot->isWater() && !pFromPlot->IsAllowsWalkWater())?DOMAIN_SEA:DOMAIN_LAND) * GC.getMOVE_DENOMINATOR();
		}
		else
			iMax = pCacheData->baseMoves(bToPlotIsWater?DOMAIN_SEA:DOMAIN_LAND) * GC.getMOVE_DENOMINATOR();
	}

	// Get the cost of moving to the new plot, passing in our max moves or the moves we have left, in case the movementCost 
	// method wants to burn all our remaining moves.  This is needed because our remaining moves for this segment of the path
	// may be larger or smaller than the baseMoves if some moves have already been used or if the starting domain (LAND/SEA)
	// of the path segment is different from the destination plot.
	int iCost = CvUnitMovement::MovementCost(pUnit, pFromPlot, pToPlot, pCacheData->baseMoves((pToPlot->isWater() || pCacheData->isEmbarked())?DOMAIN_SEA:pCacheData->getDomainType()), pCacheData->maxMoves(), iMax);

	TeamTypes eUnitTeam = pCacheData->getTeam();
#ifdef AUI_ASTAR_USE_DELEGATES
	bool bMaximizeExplore = GetInfo() & MOVE_MAXIMIZE_EXPLORE;
#else
	bool bMaximizeExplore = finder->GetInfo() & MOVE_MAXIMIZE_EXPLORE;
#endif

	int iMovesLeft = iMax - iCost;
	// Is the cost greater than our max?
	if (iMovesLeft < 0)
	{
		// Yes, we will still let the move happen, but that is the end of the turn.
		iCost = iMax;
		iMovesLeft = 0;
	}

	if(iMovesLeft == 0)
	{
		iCost = (PATH_MOVEMENT_WEIGHT * iCost);

		if(eUnitDomain == DOMAIN_LAND && !pFromPlot->isWater() && bToPlotIsWater && !pUnit->canEmbarkOnto(*pFromPlot, *pToPlot, true))
		{
			iCost += PATH_INCORRECT_EMBARKING_WEIGHT;
		}

		if(bMaximizeExplore)
		{
			if(!pToPlot->isHills())
			{
				iCost += PATH_EXPLORE_NON_HILL_WEIGHT;
			}
		}

		// Damage caused by features (mods)
		if(0 != GC.getPATH_DAMAGE_WEIGHT())
		{
			if(pToPlot->getFeatureType() != NO_FEATURE)
			{
#ifdef AUI_FAST_COMP
				iCost += (GC.getPATH_DAMAGE_WEIGHT() * FASTMAX(0, GC.getFeatureInfo(pToPlot->getFeatureType())->getTurnDamage())) / GC.getMAX_HIT_POINTS();
#else
				iCost += (GC.getPATH_DAMAGE_WEIGHT() * std::max(0, GC.getFeatureInfo(pToPlot->getFeatureType())->getTurnDamage())) / GC.getMAX_HIT_POINTS();
#endif
			}

			if(pToPlot->getExtraMovePathCost() > 0)
			{
				iCost += (PATH_MOVEMENT_WEIGHT * pToPlot->getExtraMovePathCost());
			}
		}

		// Penalty for stacking
#ifdef AUI_ASTAR_USE_DELEGATES
		if (GC.getPLOT_UNIT_LIMIT() > 0 && !(GetInfo() & MOVE_IGNORE_STACKING))
#else
		if(GC.getPLOT_UNIT_LIMIT() > 0 && !(finder->GetInfo() & MOVE_IGNORE_STACKING))
#endif
		{
			// Check to see if any units are present at this full-turn move plot... if the player can see what's there
			if(pToPlot->getNumFriendlyUnitsOfType(pUnit) >= GC.getPLOT_UNIT_LIMIT())
			{
				iCost += PATH_STACKING_WEIGHT;
			}
		}

		// Penalty for ending a turn on a mountain
		if(pToPlot->isMountain())
		{
			// We want to discourage AIs and automated units from exhausting their movement on a mountain, but if the unit is manually controlled by the human, let them do what they want.
			if (!pCacheData->isHuman() || pCacheData->IsAutomated())
			{
				iCost += PATH_END_TURN_MOUNTAIN_WEIGHT;
			}
#ifdef AUI_ASTAR_HUMAN_UNITS_GET_DIMINISHED_AVOID_WEIGHT
			else
			{
				iCost += AUI_ASTAR_HUMAN_UNITS_GET_DIMINISHED_AVOID_WEIGHT;
			}
#endif
		}

		if (pUnit->isHasPromotion((PromotionTypes)GC.getPROMOTION_UNWELCOME_EVANGELIST()))
		{
			// Avoid being in a territory that we are not welcome in, unless the human is manually controlling the unit.
#ifndef AUI_ASTAR_HUMAN_UNITS_GET_DIMINISHED_AVOID_WEIGHT
			if (!pCacheData->isHuman() || pCacheData->IsAutomated())
#endif
			{
				// Also, ignore the penalty if the destination of the path is in the same team's territory, no sense in avoiding a place we want to get to.				
				PlayerTypes ePlotOwner = pToPlot->getOwner();
#ifdef AUI_ASTAR_USE_DELEGATES
				CvPlot* pDestPlot = (GetDestX() >= 0 && GetDestY() >= 0) ? kMap.plotCheckInvalid(GetDestX(), GetDestY()) : NULL;
#else
				CvPlot* pDestPlot = (finder->GetDestX() >= 0 && finder->GetDestY() >= 0)?kMap.plotCheckInvalid(finder->GetDestX(), finder->GetDestY()):NULL;
#endif
				if (!pDestPlot || pDestPlot->getOwner() != ePlotOwner)
				{
					TeamTypes ePlotTeam = pToPlot->getTeam();
					if (ePlotOwner != NO_PLAYER && !GET_PLAYER(ePlotOwner).isMinorCiv() && ePlotTeam != pCacheData->getTeam() && !GET_TEAM(ePlotTeam).IsAllowsOpenBordersToTeam(pCacheData->getTeam()))
					{
#ifdef AUI_ASTAR_HUMAN_UNITS_GET_DIMINISHED_AVOID_WEIGHT
						if (!pCacheData->isHuman() || pCacheData->IsAutomated())
						{
							iCost += PATH_END_TURN_MISSIONARY_OTHER_TERRITORY;
						}
						else
						{
							iCost += AUI_ASTAR_HUMAN_UNITS_GET_DIMINISHED_AVOID_WEIGHT;
						}
#else
						iCost += PATH_END_TURN_MISSIONARY_OTHER_TERRITORY;
#endif
					}
				}
			}
		}
		else
		{
			if(pToPlot->getTeam() != eUnitTeam)
			{
				iCost += PATH_TERRITORY_WEIGHT;
			}
		}

#if PATH_CITY_AVOID_WEIGHT != 0
		if(pToPlot->getPlotCity() && !(pToPlot->getX() == finder->GetDestX() && pToPlot->getY() == finder->GetDestY()))
		{
			iCost += PATH_CITY_AVOID_WEIGHT; // slewis - this should be zeroed out currently
		}
#endif
	}
	else
	{
		iCost = (PATH_MOVEMENT_WEIGHT * iCost);
	}

	if(bMaximizeExplore)
	{
		int iUnseenPlots = pToPlot->getNumAdjacentNonrevealed(eUnitTeam);
		if(!pToPlot->isRevealed(eUnitTeam))
		{
			iUnseenPlots += 1;
		}

		iCost += (7 - iUnseenPlots) * PATH_EXPLORE_NON_REVEAL_WEIGHT;
	}

	// If we are a land unit and we are moving through the water, make the cost a little higher so that
	// we favor staying on land or getting back to land as quickly as possible because it is dangerous to
	// be on the water.  Don't add this penalty if the unit is human controlled however, we will assume they want
	// the best path, rather than the safest.
#ifdef AUI_ASTAR_HUMAN_UNITS_GET_DIMINISHED_AVOID_WEIGHT
	if (eUnitDomain == DOMAIN_LAND && bToPlotIsWater && !pToPlot->IsAllowsWalkWater())
	{
		if (!pCacheData->isHuman() || pCacheData->IsAutomated())
		{
			iCost += PATH_THROUGH_WATER;
		}
		else
		{
			iCost += AUI_ASTAR_HUMAN_UNITS_GET_DIMINISHED_AVOID_WEIGHT;
		}
	}
#else
	if(eUnitDomain == DOMAIN_LAND && bToPlotIsWater && (!pCacheData->isHuman() || pCacheData->IsAutomated()))
	{
		iCost += PATH_THROUGH_WATER;
	}
#endif

	if(pUnit->IsCombatUnit())
	{
#if defined(AUI_ASTAR_AVOID_RIVER_CROSSING_WHEN_ATTACKING) || defined(AUI_ASTAR_CONSIDER_DAMAGE_WHEN_ATTACKING)
		bool bToPlotHasEnemy = pToPlot->isVisibleEnemyDefender(pUnit) || pToPlot->isEnemyCity(*pUnit);
		if (iMovesLeft == 0 && !bToPlotHasEnemy)
#else
		if(iMovesLeft == 0)
#endif
		{
#ifdef AUI_ASTAR_FIX_DEFENSE_PENALTIES_CONSIDERED_FOR_UNITS_WITHOUT_DEFENSE_BONUS
			int iDefenseBonus = pToPlot->defenseModifier(eUnitTeam, false);
			if (iDefenseBonus > 0)
			{
				if (pUnit->noDefensiveBonus())
					iDefenseBonus = 0;
				else if (iDefenseBonus > 200)
					iDefenseBonus = 200;
			}
			iCost += PATH_DEFENSE_WEIGHT * (200 - iDefenseBonus);
#else
#ifdef AUI_FAST_COMP
			iCost += (PATH_DEFENSE_WEIGHT * FASTMAX(0, (200 - ((pUnit->noDefensiveBonus()) ? 0 : pToPlot->defenseModifier(eUnitTeam, false)))));
#else
			iCost += (PATH_DEFENSE_WEIGHT * std::max(0, (200 - ((pUnit->noDefensiveBonus()) ? 0 : pToPlot->defenseModifier(eUnitTeam, false)))));
#endif
#endif
		}

#if !defined(AUI_ASTAR_AVOID_RIVER_CROSSING_WHEN_ATTACKING) && !defined(AUI_ASTAR_CONSIDER_DAMAGE_WHEN_ATTACKING)
		if(pCacheData->IsAutomated())
#endif
		{
			if(pCacheData->IsCanAttack())
			{
#ifdef AUI_ASTAR_USE_DELEGATES
				if (IsPathDest(iToPlotX, iToPlotY))
#else
				if(finder->IsPathDest(iToPlotX, iToPlotY))
#endif
				{
#if defined(AUI_ASTAR_AVOID_RIVER_CROSSING_WHEN_ATTACKING) || defined(AUI_ASTAR_CONSIDER_DAMAGE_WHEN_ATTACKING)
					if (bToPlotHasEnemy)
#else
					if(pToPlot->isVisibleEnemyDefender(pUnit))
#endif
					{
#ifdef AUI_ASTAR_CONSIDER_DAMAGE_WHEN_ATTACKING
						int iDealtDamage = 0;
						int iSelfDamage = 0;
						CvCity* pCity = pToPlot->getPlotCity();
						if (pCity)
						{
							int iAttackerStrength = pUnit->GetMaxAttackStrength(pFromPlot, pToPlot, NULL);
							int iDefenderStrength = pCity->getStrengthValue();

							iDealtDamage = pUnit->getCombatDamage(iAttackerStrength, iDefenderStrength, pUnit->getDamage(), /*bIncludeRand*/ false, /*bAttackerIsCity*/ false, /*bDefenderIsCity*/ true);
							iSelfDamage = pUnit->getCombatDamage(iDefenderStrength, iAttackerStrength, pCity->getDamage(), /*bIncludeRand*/ false, /*bAttackerIsCity*/ true, /*bDefenderIsCity*/ false);

							// Will both the attacker die, and the city fall? If so, the unit wins
							if (iDealtDamage + pCity->getDamage() >= pCity->GetMaxHitPoints())
							{
								if (pUnit->isNoCapture())
									iDealtDamage = pCity->GetMaxHitPoints() - pCity->getDamage() - 1;
								if (iSelfDamage >= pUnit->GetCurrHitPoints())
									iSelfDamage = pUnit->GetCurrHitPoints() - 1;
							}
						}
						else
						{
							CvUnit* pDefender = pToPlot->getVisibleEnemyDefender(pUnit);
							if (pDefender && pDefender->IsCanDefend())
							{
								// handle the Zulu special thrown spear first attack
								if (pUnit->isRangedSupportFire() && pUnit->canEverRangeStrikeAt(pToPlot, pFromPlot))
									iDealtDamage = pUnit->GetRangeCombatDamage(pDefender, /*pCity*/ NULL, /*bIncludeRand*/ false, 0, NULL, pFromPlot);

								if (iDealtDamage < pDefender->GetCurrHitPoints())
								{
									int iAttackerStrength = pUnit->GetMaxAttackStrength(pFromPlot, pToPlot, pDefender);
									int iDefenderStrength = pDefender->GetMaxDefenseStrength(pToPlot, pUnit);

									if (pUnit->IsCanHeavyCharge() && !pDefender->CanFallBackFromMelee(*pUnit))
										iAttackerStrength = (iAttackerStrength * 150) / 100;
									
									iSelfDamage = pDefender->getCombatDamage(iDefenderStrength, iAttackerStrength, pDefender->getDamage() + iDealtDamage, /*bIncludeRand*/ false, /*bAttackerIsCity*/ false, /*bDefenderIsCity*/ false);
									iDealtDamage = pUnit->getCombatDamage(iAttackerStrength, iDefenderStrength, pUnit->getDamage(), /*bIncludeRand*/ false, /*bAttackerIsCity*/ false, /*bDefenderIsCity*/ false);

									// Will both units be killed by this? :o If so, take drastic corrective measures
									if (iDealtDamage >= pDefender->GetCurrHitPoints() && iSelfDamage >= pUnit->GetCurrHitPoints())
									{
										// He who hath the least amount of damage survives with 1 HP left
										if (iDealtDamage + pDefender->getDamage() > iSelfDamage + pUnit->getDamage())
											iSelfDamage = pUnit->GetCurrHitPoints() - 1;
										else
											iDealtDamage = pDefender->GetCurrHitPoints() - 1;
									}
								}
							}
						}
						if (iSelfDamage > pUnit->GetCurrHitPoints())
							iSelfDamage = pUnit->GetMaxHitPoints();
						if (iDealtDamage > GC.getMAX_HIT_POINTS())
							iDealtDamage = GC.getMAX_HIT_POINTS();
						iCost += iSelfDamage * PATH_DAMAGE_WEIGHT * pUnit->GetMaxHitPoints() / 100 + (GC.getMAX_HIT_POINTS() - iDealtDamage) * PATH_DAMAGE_WEIGHT / 10;
#else
#ifdef AUI_ASTAR_FIX_DEFENSE_PENALTIES_CONSIDERED_FOR_UNITS_WITHOUT_DEFENSE_BONUS
						int iDefenseBonus = pFromPlot->defenseModifier(eUnitTeam, false);
						if (iDefenseBonus > 0)
						{
							if (pUnit->noDefensiveBonus())
								iDefenseBonus = 0;
							else if (iDefenseBonus > 200)
								iDefenseBonus = 200;
						}
						iCost += PATH_DEFENSE_WEIGHT * (200 - iDefenseBonus);
#else
#ifdef AUI_FAST_COMP
						iCost += (PATH_DEFENSE_WEIGHT * FASTMAX(0, (200 - ((pUnit->noDefensiveBonus()) ? 0 : pFromPlot->defenseModifier(eUnitTeam, false)))));
#else
						iCost += (PATH_DEFENSE_WEIGHT * std::max(0, (200 - ((pUnit->noDefensiveBonus()) ? 0 : pFromPlot->defenseModifier(eUnitTeam, false)))));
#endif
#endif

						// I guess we may as well be the garrison
#if PATH_CITY_WEIGHT != 0
						if(!(pFromPlot->isCity()))
						{
							iCost += PATH_CITY_WEIGHT;
						}
#endif

						if(pFromPlot->isRiverCrossing(directionXY(iFromPlotX, iFromPlotY, iToPlotX, iToPlotY)))
						{
							if(!(pUnit->isRiverCrossingNoPenalty()))
							{
								iCost += (PATH_RIVER_WEIGHT * -(GC.getRIVER_ATTACK_MODIFIER()));
								iCost += (PATH_MOVEMENT_WEIGHT * iMovesLeft);
							}
						}
#endif
					}
				}
			}
		}
	}

	FAssert(iCost != MAX_INT);

	iCost += PATH_STEP_WEIGHT;

	FAssert(iCost > 0);

	return iCost;
}

//	---------------------------------------------------------------------------
/// Standard path finder - check validity of a coordinate
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::PathValid(CvAStarNode* parent, CvAStarNode* node, int data)
#else
int PathValid(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
#ifndef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	CvPlot* pToPlot = node->m_pPlot;
	if (!pToPlot)
		return FALSE;
#else
	CvMap& theMap = GC.getMap();

	CvPlot* pToPlot = theMap.plotUnchecked(node->m_iX, node->m_iY);
#endif
	PREFETCH_FASTAR_CVPLOT(reinterpret_cast<char*>(pToPlot));
#endif

#ifdef AUI_ASTAR_USE_DELEGATES
	const CvUnit* pUnit = m_pData;
	const UnitPathCacheData* pCacheData = reinterpret_cast<const UnitPathCacheData*>(GetScratchBuffer());
#else
	CvUnit* pUnit = ((CvUnit*)pointer);
	const UnitPathCacheData* pCacheData = reinterpret_cast<const UnitPathCacheData*>(finder->GetScratchBuffer());
#endif
	TeamTypes eUnitTeam = pCacheData->getTeam();
	PlayerTypes unit_owner = pCacheData->getOwner();

	CvAssertMsg(eUnitTeam != NO_TEAM, "The unit's team should be a vaild value");
	if (eUnitTeam == NO_TEAM)
	{
		eUnitTeam = GC.getGame().getActiveTeam();
	}

	CvTeam& kUnitTeam = GET_TEAM(eUnitTeam);

#ifdef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	CvPlot* pToPlot = node->m_pPlot;
	if (!pToPlot)
		return FALSE;
#else
	CvMap& theMap = GC.getMap();

	CvPlot* pToPlot = theMap.plotUnchecked(node->m_iX, node->m_iY);
#endif
	PREFETCH_FASTAR_CVPLOT(reinterpret_cast<char*>(pToPlot));
#endif

#ifdef AUI_ASTAR_FIX_PARENT_NODE_ALWAYS_VALID_OPTIMIZATION
	// If this is the first node in the path, it is always valid (starting location)
	if (parent == NULL)
	{
#ifdef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
		// Cache values for this node that we will use in the loop
		CvPathNodeCacheData& kToNodeCacheData = node->m_kCostCacheData;
		if (!kToNodeCacheData.bIsCalculated)
		{
			kToNodeCacheData.bIsCalculated = true;
			kToNodeCacheData.bPlotVisibleToTeam = true;
			kToNodeCacheData.iNumFriendlyUnitsOfType = pToPlot->getNumFriendlyUnitsOfType(pUnit);
			kToNodeCacheData.bIsMountain = pToPlot->isMountain();
			kToNodeCacheData.bIsWater = (pToPlot->isWater() && !pToPlot->IsAllowsWalkWater());
			kToNodeCacheData.bCanEnterTerrain = true;
			kToNodeCacheData.bIsRevealedToTeam = true;
			kToNodeCacheData.bContainsOtherFriendlyTeamCity = false;
			CvCity* pCity = pToPlot->getPlotCity();
			if (pCity)
			{
				if (unit_owner != pCity->getOwner() && !kUnitTeam.isAtWar(pCity->getTeam()))
					kToNodeCacheData.bContainsOtherFriendlyTeamCity = true;
			}
			kToNodeCacheData.bContainsEnemyCity = pToPlot->isEnemyCity(*pUnit);
			kToNodeCacheData.bContainsVisibleEnemy = pToPlot->isVisibleEnemyUnit(pUnit);
			kToNodeCacheData.bContainsVisibleEnemyDefender = pToPlot->getBestDefender(NO_PLAYER, unit_owner, pUnit).pointer() != NULL;
#ifdef AUI_DANGER_PLOTS_REMADE
			if (pCacheData->DoDanger())
				kToNodeCacheData.iPlotDanger = GET_PLAYER(unit_owner).GetPlotDanger(*pToPlot, pUnit);
#endif
		}
#endif
		return TRUE;
	}
#endif

	// Cache values for this node that we will use in the loop
	CvPathNodeCacheData& kToNodeCacheData = node->m_kCostCacheData;
#ifdef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
	if (!kToNodeCacheData.bIsCalculated)
	{
		kToNodeCacheData.bPlotVisibleToTeam = pToPlot->isVisible(eUnitTeam);
		kToNodeCacheData.iNumFriendlyUnitsOfType = pToPlot->getNumFriendlyUnitsOfType(pUnit);
		kToNodeCacheData.bIsMountain = pToPlot->isMountain();
		kToNodeCacheData.bIsWater = (pToPlot->isWater() && !pToPlot->IsAllowsWalkWater());
		kToNodeCacheData.bIsRevealedToTeam = pToPlot->isRevealed(eUnitTeam);
		kToNodeCacheData.bContainsOtherFriendlyTeamCity = false;
		CvCity* pCity = pToPlot->getPlotCity();
		if (pCity)
		{
			if (unit_owner != pCity->getOwner() && !kUnitTeam.isAtWar(pCity->getTeam()))
				kToNodeCacheData.bContainsOtherFriendlyTeamCity = true;
		}
		kToNodeCacheData.bContainsEnemyCity = pToPlot->isEnemyCity(*pUnit);
		kToNodeCacheData.bContainsVisibleEnemy = pToPlot->isVisibleEnemyUnit(pUnit);
		kToNodeCacheData.bContainsVisibleEnemyDefender = pToPlot->getBestDefender(NO_PLAYER, unit_owner, pUnit).pointer() != NULL;
	}
#else
	kToNodeCacheData.bPlotVisibleToTeam = pToPlot->isVisible(eUnitTeam);
	kToNodeCacheData.iNumFriendlyUnitsOfType = pToPlot->getNumFriendlyUnitsOfType(pUnit);
	kToNodeCacheData.bIsMountain = pToPlot->isMountain();
	kToNodeCacheData.bIsWater = (pToPlot->isWater() && !pToPlot->IsAllowsWalkWater());
#ifndef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
	kToNodeCacheData.bCanEnterTerrain = pUnit->canEnterTerrain(*pToPlot, CvUnit::MOVEFLAG_PRETEND_CORRECT_EMBARK_STATE);
#endif
	kToNodeCacheData.bIsRevealedToTeam = pToPlot->isRevealed(eUnitTeam);
	kToNodeCacheData.bContainsOtherFriendlyTeamCity = false;
	CvCity* pCity = pToPlot->getPlotCity();
	if(pCity)
	{
		if(unit_owner != pCity->getOwner() && !kUnitTeam.isAtWar(pCity->getTeam()))
			kToNodeCacheData.bContainsOtherFriendlyTeamCity = true;
	}
	kToNodeCacheData.bContainsEnemyCity = pToPlot->isEnemyCity(*pUnit);
	kToNodeCacheData.bContainsVisibleEnemy = pToPlot->isVisibleEnemyUnit(pUnit);
	kToNodeCacheData.bContainsVisibleEnemyDefender = pToPlot->getBestDefender(NO_PLAYER, unit_owner, pUnit).pointer() != NULL;
#endif

#ifndef AUI_ASTAR_FIX_PARENT_NODE_ALWAYS_VALID_OPTIMIZATION
	// If this is the first node in the path, it is always valid (starting location)
	if (parent == NULL)
	{
		return TRUE;
	}
#endif

#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	CvPlot* pFromPlot = parent->m_pPlot;
	if (!pFromPlot)
		return FALSE;
#else
	CvPlot* pFromPlot = theMap.plotUnchecked(parent->m_iX, parent->m_iY);
#endif
	PREFETCH_FASTAR_CVPLOT(reinterpret_cast<char*>(pFromPlot));

	// pulling invariants out of the loop
	bool bAIControl = pCacheData->IsAutomated();
	int iUnitX = pUnit->getX();
	int iUnitY = pUnit->getY();
	DomainTypes unit_domain_type = pCacheData->getDomainType();
	bool bUnitIsCombat           = pUnit->IsCombatUnit();
	bool bIsHuman				 = pCacheData->isHuman();
#ifndef AUI_ASTAR_USE_DELEGATES
	int iFinderInfo              = finder->GetInfo();
#endif
	CvPlot* pUnitPlot            = pUnit->plot();
#ifdef AUI_ASTAR_USE_DELEGATES
	int iFinderIgnoreStacking = GetInfo() & MOVE_IGNORE_STACKING;
#else
	int iFinderIgnoreStacking    = iFinderInfo & MOVE_IGNORE_STACKING;
#endif
	int iUnitPlotLimit           = GC.getPLOT_UNIT_LIMIT();
	bool bFromPlotOwned          = pFromPlot->isOwned();
	TeamTypes eFromPlotTeam      = pFromPlot->getTeam();

#ifdef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
	if (!kToNodeCacheData.bIsCalculated)
	{
#ifdef AUI_DANGER_PLOTS_REMADE
		if (pCacheData->DoDanger())
			kToNodeCacheData.iPlotDanger = GET_PLAYER(unit_owner).GetPlotDanger(*pToPlot, pUnit);
#endif
		if (bAIControl || kToNodeCacheData.bIsRevealedToTeam || !bIsHuman)
			kToNodeCacheData.bCanEnterTerrain = pUnit->canEnterTerrain(*pToPlot, CvUnit::MOVEFLAG_PRETEND_CORRECT_EMBARK_STATE);
		else
			kToNodeCacheData.bCanEnterTerrain = true;
		kToNodeCacheData.bIsCalculated = true;
	}
#else
#ifdef AUI_DANGER_PLOTS_REMADE
	if (bAIControl || !bIsHuman)
		kToNodeCacheData.iPlotDanger = GET_PLAYER(unit_owner).GetPlotDanger(*pToPlot, pUnit);
#endif
#endif

	// We have determined that this node is not the origin above (parent == NULL)
	CvAStarNode* pNode = node;
	bool bPreviousNodeHostile = false;
	bool bPreviousVisibleToTeam = kToNodeCacheData.bPlotVisibleToTeam;
#ifndef AUI_ASTAR_USE_DELEGATES
	int iDestX = finder->GetDestX();
	int iDestY = finder->GetDestY();
#endif
	int iNodeX = node->m_iX;
	int iNodeY = node->m_iY;
	int iOldNumTurns = -1;
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	int iNumTurns;
#endif

	// First run special case for checking "node" since it doesn't have a parent set yet
	bool bFirstRun = true;

	// Have to calculate this specially because the node passed into this function doesn't yet have data stored it in (hasn't reached pathAdd yet)
	int iStartMoves = parent->m_iData1;
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	int iNumTurns = parent->m_iData2;
#else
	iNumTurns = parent->m_iData2;
#endif
#if defined(AUI_ASTAR_TURN_LIMITER) && !defined(AUI_ASTAR_USE_DELEGATES)
	int iMaxTurns = finder->GetMaxTurns();
#endif

	if(iStartMoves == 0)
	{
		iNumTurns++;
	}

	iOldNumTurns = -1;

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pPlot = NULL;
#endif

	// Get a reference to the parent node cache data
	CvPathNodeCacheData& kFromNodeCacheData = parent->m_kCostCacheData;

	// Loop through the current path until we find the path origin.
	// This validates the path with the inclusion of the new path node.  We must do this because of the rules of where a unit can finish a turn.
	// Please note that this can be an expensive loop as the path gets longer and longer, do as little work as possible in validating each node.  
	// If there is an invariant value that needs to be fetched from the plot or unit for the node, please do the calculation and put it in the node's data cache.
	while(pNode != NULL)
	{
#ifdef AUI_ASTAR_TURN_LIMITER
#ifdef AUI_ASTAR_USE_DELEGATES
		if (iNumTurns > GetMaxTurns())
#else
		if (iNumTurns > iMaxTurns)
#endif
		{
			return FALSE;  // Path is too long, terminate now
		}
#endif

		PREFETCH_FASTAR_NODE(pNode->m_pParent);

		CvPathNodeCacheData& kNodeCacheData = pNode->m_kCostCacheData;
		// This is a safeguard against the algorithm believing a plot to be impassable before actually knowing it (mid-search)
#ifdef AUI_ASTAR_USE_DELEGATES
		if(iOldNumTurns != -1 || (GetDestX() == iNodeX && GetDestY() == iNodeY))
#else
		if(iOldNumTurns != -1 || (iDestX == iNodeX && iDestY == iNodeY))
#endif
		{
#ifdef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
			if (!kNodeCacheData.bCanEnterTerrain)
			{
				return FALSE;
			}
#endif
			// This plot is of greater distance than previously, so we know the unit is ending its turn here (pNode), or it's trying to attack through a unit (and might end up on this tile if an attack fails to kill the enemy)
			if(iNumTurns != iOldNumTurns || bPreviousNodeHostile || !bPreviousVisibleToTeam)
			{
				// Don't count origin, or else a unit will block its own movement!
				if(iNodeX != iUnitX || iNodeY != iUnitY)
				{
					if(kNodeCacheData.bPlotVisibleToTeam)
					{
						// Check to see if any units are present at this full-turn move plot... if the player can see what's there
						if(kNodeCacheData.iNumFriendlyUnitsOfType >= iUnitPlotLimit && !(iFinderIgnoreStacking))
						{
							return FALSE;
						}

#ifndef AUI_ASTAR_FIX_PATH_VALID_PATH_PEAKS_FOR_NONHUMAN
						if (kNodeCacheData.bIsMountain && !(iFinderIgnoreStacking) && (!bIsHuman || bAIControl))
						{
							return FALSE;
						}
#endif

#ifndef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
						if(kNodeCacheData.bIsMountain && !kNodeCacheData.bCanEnterTerrain)
						{
							return FALSE;
						}
#endif

#ifdef AUI_ASTAR_USE_DELEGATES
						if ((GetInfo() & CvUnit::MOVEFLAG_STAY_ON_LAND) && kNodeCacheData.bIsWater)
#else
						if ((iFinderInfo & CvUnit::MOVEFLAG_STAY_ON_LAND) && kNodeCacheData.bIsWater)
#endif
						{
							return FALSE;
						}
					}

					if(kNodeCacheData.bIsRevealedToTeam)
					{
						if (kNodeCacheData.bContainsOtherFriendlyTeamCity && !(iFinderIgnoreStacking))
							return FALSE;
					}
				}
			}
		}

		bPreviousNodeHostile = false;
		if(kNodeCacheData.bContainsEnemyCity)
		{
			bPreviousNodeHostile = true;
		}
		// Prevents units from passing through one another on its way to attack another unit
		else if(kNodeCacheData.bContainsVisibleEnemy)
		{
			// except when attacking an unguarded civilian unit
			if(kNodeCacheData.bContainsVisibleEnemyDefender)
			{
				bPreviousNodeHostile = true;
			}
		}

		bPreviousVisibleToTeam = kNodeCacheData.bPlotVisibleToTeam;
		// JON - Special case for the original node passed into this function because it's not yet linked to any parent
		if(pNode == node && bFirstRun)
		{
			pNode = parent;
			bFirstRun = false;
		}
		else
		{
			pNode = pNode->m_pParent;
		}

		if(pNode != NULL)
		{
			iNodeX = pNode->m_iX;
			iNodeY = pNode->m_iY;
			iOldNumTurns = iNumTurns;
			iNumTurns = pNode->m_iData2;
		}
	}

	// slewis - moved this up so units can't move directly into the water. Not 100% sure this is the right solution.
	if(unit_domain_type == DOMAIN_LAND)
	{
		if(!kFromNodeCacheData.bIsWater && kToNodeCacheData.bIsWater && kToNodeCacheData.bIsRevealedToTeam && !pUnit->canEmbarkOnto(*pFromPlot, *pToPlot, true))
		{
			if(!pUnit->IsHoveringUnit() && !pUnit->canMoveAllTerrain() && !pToPlot->IsAllowsWalkWater())
			{
				return FALSE;
			}
		}
	}

#ifndef AUI_ASTAR_FIX_RADAR
	if(!bUnitIsCombat && unit_domain_type != DOMAIN_AIR)
	{
		const PlayerTypes eUnitPlayer = unit_owner;
		const int iUnitCount = pToPlot->getNumUnits();
		for(int iUnit = 0; iUnit < iUnitCount; ++iUnit)
		{
			const CvUnit* pToPlotUnit = pToPlot->getUnitByIndex(iUnit);
			if(pToPlotUnit != NULL && pToPlotUnit->getOwner() != eUnitPlayer)
			{
				return FALSE; // Plot occupied by another player
			}
		}
	}
#endif

	// slewis - Added to catch when the unit is adjacent to an enemy unit while it is stacked with a friendly unit.
	//          The logic above (with bPreviousNodeHostile) catches this problem with a path that's longer than one step
	//          but does not catch when the path is only one step.
#ifdef AUI_ASTAR_FIX_RADAR
	if (unit_domain_type != DOMAIN_AIR && pUnitPlot->isAdjacent(pToPlot) && kToNodeCacheData.bContainsVisibleEnemy && !(iFinderIgnoreStacking))
#else
	if(bUnitIsCombat && unit_domain_type != DOMAIN_AIR && pUnitPlot->isAdjacent(pToPlot) && kToNodeCacheData.bContainsVisibleEnemy && !(iFinderIgnoreStacking))
#endif
	{
		if(kToNodeCacheData.bContainsVisibleEnemyDefender)
		{
			if(pUnitPlot->getNumFriendlyUnitsOfType(pUnit) > iUnitPlotLimit)
			{
				return FALSE;
			}
		}
	}

	if(pUnitPlot == pFromPlot)
	{
		return TRUE;
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	if (GetInfo() & MOVE_TERRITORY_NO_UNEXPLORED)
#else
	if(iFinderInfo & MOVE_TERRITORY_NO_UNEXPLORED)
#endif
	{
		if(!(kFromNodeCacheData.bIsRevealedToTeam))
		{
			return FALSE;
		}

		if(bFromPlotOwned)
		{
			if(eFromPlotTeam != eUnitTeam)
			{
				return FALSE;
			}
		}
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	if (GetInfo() & MOVE_TERRITORY_NO_ENEMY)
#else
	if(iFinderInfo & MOVE_TERRITORY_NO_ENEMY)
#endif
	{
		if(bFromPlotOwned)
		{
			if(atWar(eFromPlotTeam, eUnitTeam))
			{
				return FALSE;
			}
		}
	}

#ifdef AUI_DANGER_PLOTS_REMADE
	if (pCacheData->DoDanger())
#else
	if(bAIControl)
#endif
	{
		if((parent->m_iData2 > 1) || (parent->m_iData1 == 0))
		{
#ifdef AUI_DANGER_PLOTS_REMADE
			{
#else
#ifdef AUI_ASTAR_USE_DELEGATES
			if (!(GetInfo() & MOVE_UNITS_IGNORE_DANGER))
#else
			if(!(iFinderInfo & MOVE_UNITS_IGNORE_DANGER))
#endif
			{
				if(!bUnitIsCombat || pUnit->getArmyID() == FFreeList::INVALID_INDEX)
#endif
				{
#ifdef AUI_DANGER_PLOTS_REMADE
					if (kToNodeCacheData.iPlotDanger == MAX_INT || (kToNodeCacheData.iPlotDanger >= pUnit->GetCurrHitPoints() && kFromNodeCacheData.iPlotDanger < pUnit->GetCurrHitPoints()))
#else
#ifdef AUI_ASTAR_FIX_CONSIDER_DANGER_USES_TO_PLOT_NOT_FROM_PLOT
#ifdef AUI_ASTAR_FIX_CONSIDER_DANGER_USES_COMBAT_STRENGTH
#ifdef AUI_ASTAR_FIX_CONSIDER_DANGER_ONLY_POSITIVE_DANGER_DELTA
					if (GET_PLAYER(pUnit->getOwner()).GetPlotDanger(*pToPlot) > pUnit->GetBaseCombatStrengthConsideringDamage() * AUI_ASTAR_FIX_CONSIDER_DANGER_USES_COMBAT_STRENGTH &&
						GET_PLAYER(pUnit->getOwner()).GetPlotDanger(*pFromPlot) <= pUnit->GetBaseCombatStrengthConsideringDamage() * AUI_ASTAR_FIX_CONSIDER_DANGER_USES_COMBAT_STRENGTH)
#else
					if (GET_PLAYER(unit_owner).GetPlotDanger(*pToPlot) > pUnit->GetBaseCombatStrengthConsideringDamage() * AUI_ASTAR_FIX_CONSIDER_DANGER_USES_COMBAT_STRENGTH)
#endif
#else
#ifdef AUI_ASTAR_FIX_CONSIDER_DANGER_ONLY_POSITIVE_DANGER_DELTA
					if (GET_PLAYER(pUnit->getOwner()).GetPlotDanger(*pToPlot) > 0 && GET_PLAYER(pUnit->getOwner()).GetPlotDanger(*pFromPlot) <= 0)
#else
					if(GET_PLAYER(unit_owner).GetPlotDanger(*pToPlot) > 0)
#endif
#endif
#else
#ifdef AUI_ASTAR_FIX_CONSIDER_DANGER_USES_COMBAT_STRENGTH
					if (GET_PLAYER(unit_owner).GetPlotDanger(*pFromPlot) > pUnit->GetBaseCombatStrengthConsideringDamage() * AUI_ASTAR_FIX_CONSIDER_DANGER_USES_COMBAT_STRENGTH)
#else
					if(GET_PLAYER(unit_owner).GetPlotDanger(*pFromPlot) > 0)
#endif
#endif
#endif
					{
						return FALSE;
					}
				}
			}
		}
	}

	// slewis - added AI check and embark check to prevent units from moving into unexplored areas
#ifdef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
	if(bAIControl || kFromNodeCacheData.bIsRevealedToTeam || !bIsHuman)
#else
	if(bAIControl || kFromNodeCacheData.bIsRevealedToTeam || pCacheData->isEmbarked() || !bIsHuman)
#endif
	{
#ifdef AUI_ASTAR_USE_DELEGATES
		if (GetInfo() & MOVE_UNITS_THROUGH_ENEMY)
#else
		if(iFinderInfo & MOVE_UNITS_THROUGH_ENEMY)
#endif
		{
#ifdef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
			if (!(pUnit->canMoveOrAttackInto(*pFromPlot, CvUnit::MOVEFLAG_PRETEND_CORRECT_EMBARK_STATE, kFromNodeCacheData.bCanEnterTerrain, true)))
#else
			if(!(pUnit->canMoveOrAttackInto(*pFromPlot, CvUnit::MOVEFLAG_PRETEND_CORRECT_EMBARK_STATE)))
#endif
			{
				return FALSE;
			}
		}
		else
		{
#ifdef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
			if(!(pUnit->canMoveThrough(*pFromPlot, CvUnit::MOVEFLAG_PRETEND_CORRECT_EMBARK_STATE, kFromNodeCacheData.bCanEnterTerrain, true)))
#else
			if(!(pUnit->canMoveThrough(*pFromPlot, CvUnit::MOVEFLAG_PRETEND_CORRECT_EMBARK_STATE)))
#endif
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}



//	--------------------------------------------------------------------------------
/// Standard path finder - add a new path
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::PathAdd(CvAStarNode* parent, CvAStarNode* node, int data)
#else
int PathAdd(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
	int iMoves = MAX_INT;
#ifdef AUI_ASTAR_USE_DELEGATES
	const CvUnit* pUnit = m_pData;
	const UnitPathCacheData* pCacheData = reinterpret_cast<const UnitPathCacheData*>(GetScratchBuffer());
#else
	CvUnit* pUnit = ((CvUnit*)pointer);
	const UnitPathCacheData* pCacheData = reinterpret_cast<const UnitPathCacheData*>(finder->GetScratchBuffer());
#endif

	int iTurns;

	if(data == ASNC_INITIALADD)
	{
		iTurns = 1;
#ifdef AUI_FAST_COMP
		iMoves = FASTMIN(iMoves, pUnit->movesLeft());
#else
		iMoves = std::min(iMoves, pUnit->movesLeft());
#endif
	}
	else
	{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
		const CvPlot* pFromPlot = parent->m_pPlot;
		const CvPlot* pToPlot = node->m_pPlot;
#else
		CvMap& kMap = GC.getMap();
		CvPlot* pFromPlot = kMap.plotUnchecked(parent->m_iX, parent->m_iY);
		CvPlot* pToPlot = kMap.plotUnchecked(node->m_iX, node->m_iY);
#endif

		int iStartMoves = parent->m_iData1;
		iTurns = parent->m_iData2;

		if(iStartMoves == 0)
		{
			iTurns++;
			iStartMoves = pCacheData->baseMoves((pToPlot->isWater() && !pToPlot->IsAllowsWalkWater()) ? DOMAIN_SEA : DOMAIN_LAND) * GC.getMOVE_DENOMINATOR();
		}

		// We can't use maxMoves, because that checks where the unit is currently, and we're plotting a path so we have to see
		// what the max moves would be like if the unit was already at the desired location.
		if (CvUnitMovement::ConsumesAllMoves(pUnit, pFromPlot, pToPlot) || CvUnitMovement::IsSlowedByZOC(pUnit, pFromPlot, pToPlot))
		{
			iMoves = 0;
		}
		else
		{
#ifdef AUI_FAST_COMP
			iMoves = FASTMIN(iMoves, FASTMAX(0, iStartMoves - CvUnitMovement::MovementCost(pUnit, pFromPlot, pToPlot, pCacheData->baseMoves((pToPlot->isWater() || pCacheData->isEmbarked()) ? DOMAIN_SEA : pCacheData->getDomainType()), pCacheData->maxMoves(), iStartMoves)));
#else
			iMoves = std::min(iMoves, std::max(0, iStartMoves - CvUnitMovement::MovementCost(pUnit, pFromPlot, pToPlot, pCacheData->baseMoves((pToPlot->isWater() || pCacheData->isEmbarked())?DOMAIN_SEA:pCacheData->getDomainType()), pCacheData->maxMoves(), iStartMoves)));
#endif
		}
	}

	FAssertMsg(iMoves >= 0, "iMoves is expected to be non-negative (invalid Index)");

	node->m_iData1 = iMoves;
	node->m_iData2 = iTurns;

	return 1;
}

//	--------------------------------------------------------------------------------
/// Two layer path finder - if add a new open node with movement left, add a second one assuming stop for turn here
#ifdef AUI_ASTAR_USE_DELEGATES
int CvTwoLayerPathFinder::PathNodeAdd(CvAStarNode* parent, CvAStarNode* node, int data)
#else
int PathNodeAdd(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	CvAStarNode* pNode;
#endif

	if(data == ASNL_ADDOPEN || data == ASNL_STARTOPEN)
	{
		// Are there movement points left and we're worried about stacking or mountains?
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
#ifdef AUI_ASTAR_USE_DELEGATES
		if (node->m_iData1 > 0 && !IsPathDest(node->m_iX, node->m_iY) && (!(GetInfo() & MOVE_IGNORE_STACKING) || node->m_pPlot->isMountain()))
#else
		if (node->m_iData1 > 0 && !finder->IsPathDest(node->m_iX, node->m_iY) && (!(finder->GetInfo() & MOVE_IGNORE_STACKING) || node->m_pPlot->isMountain()))
#endif
#else
#ifdef AUI_ASTAR_USE_DELEGATES
		if(node->m_iData1 > 0 && !IsPathDest(node->m_iX, node->m_iY) && (!(GetInfo() & MOVE_IGNORE_STACKING) || GC.getMap().plotUnchecked(node->m_iX, node->m_iY)->isMountain()))
#else
		if(node->m_iData1 > 0 && !finder->IsPathDest(node->m_iX, node->m_iY) && (!(finder->GetInfo() & MOVE_IGNORE_STACKING) || GC.getMap().plotUnchecked(node->m_iX, node->m_iY)->isMountain()))
#endif
#endif
		{
			// Retrieve another node
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
#ifdef AUI_ASTAR_USE_DELEGATES
			CvAStarNode* pNode = GetPartialMoveNode(node->m_iX, node->m_iY);
#else
			CvTwoLayerPathFinder* twoLayerFinder = static_cast<CvTwoLayerPathFinder*>(finder);
			CvAStarNode* pNode = twoLayerFinder->GetPartialMoveNode(node->m_iX, node->m_iY);
#endif
#else
#ifdef AUI_ASTAR_USE_DELEGATES
			pNode = GetPartialMoveNode(node->m_iX, node->m_iY);
#else
			CvTwoLayerPathFinder* twoLayerFinder = static_cast<CvTwoLayerPathFinder*>(finder);
			pNode = twoLayerFinder->GetPartialMoveNode(node->m_iX, node->m_iY);
#endif
#endif
			pNode->m_iData1 = 0;   // Zero out movement
			pNode->m_iData2 = node->m_iData2;
			pNode->m_iHeuristicCost = node->m_iHeuristicCost;
			pNode->m_iKnownCost = node->m_iKnownCost + (PATH_MOVEMENT_WEIGHT * node->m_iData1);
			pNode->m_iTotalCost = node->m_iTotalCost;
			pNode->m_iX = node->m_iX;
			pNode->m_iY = node->m_iY;
			pNode->m_pParent = node->m_pParent;
			pNode->m_eCvAStarListType = CVASTARLIST_OPEN;
			pNode->m_kCostCacheData = node->m_kCostCacheData;
#ifdef AUI_ASTAR_USE_DELEGATES
			AddToOpen(pNode);
#else
			finder->AddToOpen(pNode);
#endif
		}
	}

	return 1;
}

//	--------------------------------------------------------------------------------
/// Ignore units path finder - is this end point for the path valid?
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::IgnoreUnitsDestValid(int iToX, int iToY)
#else
int IgnoreUnitsDestValid(int iToX, int iToY, const void* pointer, CvAStar* finder)
#endif
{
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pToPlot = GC.getMap().plotUnchecked(iToX, iToY);
#ifdef AUI_ASTAR_USE_DELEGATES
	const CvUnit* pUnit = m_pData;
#else
	CvUnit* pUnit = ((CvUnit*)pointer);
#endif
#else
	CvUnit* pUnit;
	CvPlot* pToPlot;
	bool bAIControl;
	CvMap& kMap = GC.getMap();

	pToPlot = kMap.plotUnchecked(iToX, iToY);

#ifdef AUI_ASTAR_USE_DELEGATES
	pUnit = ((CvUnit*)m_pData);
#else
	pUnit = ((CvUnit*)pointer);
#endif
#endif
#ifdef AUI_ASTAR_USE_DELEGATES
	const UnitPathCacheData* pCacheData = reinterpret_cast<const UnitPathCacheData*>(GetScratchBuffer());
#else
	const UnitPathCacheData* pCacheData = reinterpret_cast<const UnitPathCacheData*>(finder->GetScratchBuffer());
#endif

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pUnitPlot = pUnit->plot();
#else
	CvPlot* pUnitPlot = kMap.plotUnchecked(pUnit->getX(), pUnit->getY());
#endif
	if(pUnitPlot == pToPlot)
	{
		return TRUE;
	}

	if(pCacheData->IsImmobile())
	{
		return FALSE;
	}

#ifndef AUI_ASTAR_FIX_PATH_VALID_PATH_PEAKS_FOR_NONHUMAN
	if(pToPlot->isMountain() && (!pCacheData->isHuman() || pCacheData->IsAutomated()))
	{
		return FALSE;
	}
#endif

#ifdef AUI_ASTAR_USE_DELEGATES
	if ((GetInfo() & CvUnit::MOVEFLAG_STAY_ON_LAND) && (pToPlot->isWater() && !pToPlot->IsAllowsWalkWater()))
#else
	if ((finder->GetInfo() & CvUnit::MOVEFLAG_STAY_ON_LAND) && (pToPlot->isWater() && !pToPlot->IsAllowsWalkWater()))
#endif
	{
		return FALSE;
	}

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	bool bAIControl = pCacheData->IsAutomated();
#else
	bAIControl = pCacheData->IsAutomated();
#endif

	if(bAIControl)
	{
		if(pCacheData->getDomainType() == DOMAIN_LAND)
		{
			int iGroupAreaID = pUnit->area()->GetID();
			if(pToPlot->getArea() != iGroupAreaID)
			{
				if(!(pToPlot->isAdjacentToArea(iGroupAreaID)))
				{
					return FALSE;
				}
			}
		}
	}

	TeamTypes eUnitTeam = pUnit->getTeam();

	if(!pToPlot->isRevealed(eUnitTeam))
	{
		if(pCacheData->isNoRevealMap())
		{
			return FALSE;
		}
	}

	if(bAIControl || pToPlot->isRevealed(eUnitTeam))
	{
		if(!pUnit->canEnterTerrain(*pToPlot, CvUnit::MOVEFLAG_PRETEND_CORRECT_EMBARK_STATE) || !pUnit->canEnterTerritory(eUnitTeam))
		{
			return FALSE;
		}
	}
	return TRUE;
}


//	--------------------------------------------------------------------------------
/// Ignore units path finder - compute cost of a path
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::IgnoreUnitsCost(CvAStarNode* parent, CvAStarNode* node, int data)
#else
int IgnoreUnitsCost(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	CvUnit* pUnit;
	int iCost;
#endif
	int iMax;

#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	const CvPlot* pFromPlot = parent->m_pPlot;
	const CvPlot* pToPlot = node->m_pPlot;
#else
	CvMap& kMap = GC.getMap();
	int iFromPlotX = parent->m_iX;
	int iFromPlotY = parent->m_iY;
	CvPlot* pFromPlot = kMap.plotUnchecked(iFromPlotX, iFromPlotY);

	int iToPlotX = node->m_iX;
	int iToPlotY = node->m_iY;
	CvPlot* pToPlot = kMap.plotUnchecked(iToPlotX, iToPlotY);
#endif

#ifdef AUI_ASTAR_USE_DELEGATES
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvUnit* pUnit = ((CvUnit*)m_pData);
#else
	pUnit = ((CvUnit*)m_pData);
#endif
	const UnitPathCacheData* pCacheData = reinterpret_cast<const UnitPathCacheData*>(GetScratchBuffer());
#else
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvUnit* pUnit = ((CvUnit*)pointer);
#else
	pUnit = ((CvUnit*)pointer);
#endif
	const UnitPathCacheData* pCacheData = reinterpret_cast<const UnitPathCacheData*>(finder->GetScratchBuffer());
#endif

	CvAssertMsg(pUnit->getDomainType() != DOMAIN_AIR, "pUnit->getDomainType() is not expected to be equal with DOMAIN_AIR");

	if(parent->m_iData1 > 0)
	{
		iMax = parent->m_iData1;
	}
	else
	{
		iMax = pCacheData->baseMoves((pToPlot->isWater() && !pToPlot->IsAllowsWalkWater()) ? DOMAIN_SEA : DOMAIN_LAND) * GC.getMOVE_DENOMINATOR();
	}

	// Get the cost of moving to the new plot, passing in our max moves or the moves we have left, in case the movementCost 
	// method wants to burn all our remaining moves.  This is needed because our remaining moves for this segment of the path
	// may be larger or smaller than the baseMoves if some moves have already been used or if the starting domain (LAND/SEA)
	// of the path segment is different from the destination plot.
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	int iCost = CvUnitMovement::MovementCostNoZOC(pUnit, pFromPlot, pToPlot, pCacheData->baseMoves((pToPlot->isWater() || pCacheData->isEmbarked()) ? DOMAIN_SEA : pCacheData->getDomainType()), pCacheData->maxMoves(), iMax);
#else
	iCost = CvUnitMovement::MovementCostNoZOC(pUnit, pFromPlot, pToPlot, pCacheData->baseMoves((pToPlot->isWater() || pCacheData->isEmbarked())?DOMAIN_SEA:pCacheData->getDomainType()), pCacheData->maxMoves(), iMax);
#endif

	TeamTypes eUnitTeam = pUnit->getTeam();

	int iMovesLeft = iMax - iCost;
	// Is the cost greater than our max?
	if (iMovesLeft < 0)
	{
		// Yes, we will still let the move happen, but that is the end of the turn.
		iCost = iMax;
		iMovesLeft = 0;
	}

	if(iMovesLeft == 0)
	{
		iCost = (PATH_MOVEMENT_WEIGHT * iCost);

		if(!pFromPlot->isWater() && pToPlot->isWater() && !pUnit->canEmbarkOnto(*pFromPlot, *pToPlot, true))
		{
			iCost += PATH_INCORRECT_EMBARKING_WEIGHT;
		}

		if(pToPlot->getTeam() != eUnitTeam)
		{
			iCost += PATH_TERRITORY_WEIGHT;
		}

#ifdef AUI_ASTAR_USE_DELEGATES
		if (GetInfo() & MOVE_MAXIMIZE_EXPLORE)
#else
		if(finder->GetInfo() & MOVE_MAXIMIZE_EXPLORE)
#endif
		{
			if(!pToPlot->isHills())
			{
				iCost += PATH_EXPLORE_NON_HILL_WEIGHT;
			}
		}

		// Damage caused by features (mods)
		if(0 != GC.getPATH_DAMAGE_WEIGHT())
		{
			if(pToPlot->getFeatureType() != NO_FEATURE)
			{
#ifdef AUI_FAST_COMP
				iCost += (GC.getPATH_DAMAGE_WEIGHT() * FASTMAX(0, GC.getFeatureInfo(pToPlot->getFeatureType())->getTurnDamage())) / GC.getMAX_HIT_POINTS();
#else
				iCost += (GC.getPATH_DAMAGE_WEIGHT() * std::max(0, GC.getFeatureInfo(pToPlot->getFeatureType())->getTurnDamage())) / GC.getMAX_HIT_POINTS();
#endif
			}

			if(pToPlot->getExtraMovePathCost() > 0)
			{
				iCost += (PATH_MOVEMENT_WEIGHT * pToPlot->getExtraMovePathCost());
			}
		}

#if PATH_CITY_AVOID_WEIGHT != 0
		if(pToPlot->getPlotCity() && !(pToPlot->getX() == finder->GetDestX() && pToPlot->getY() == finder->GetDestY()))
		{
			iCost += PATH_CITY_AVOID_WEIGHT; // slewis - this should be zeroed out currently
		}
#endif
	}
	else
	{
		iCost = (PATH_MOVEMENT_WEIGHT * iCost);
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	if (GetInfo() & MOVE_MAXIMIZE_EXPLORE)
#else
	if(finder->GetInfo() & MOVE_MAXIMIZE_EXPLORE)
#endif
	{
		int iUnseenPlots = pToPlot->getNumAdjacentNonrevealed(eUnitTeam);
		if(!pToPlot->isRevealed(eUnitTeam))
		{
			iUnseenPlots += 1;
		}

		iCost += (7 - iUnseenPlots) * PATH_EXPLORE_NON_REVEAL_WEIGHT;
	}

	// If we are a land unit and we are moving through the water, make the cost a little higher so that
	// we favor staying on land or getting back to land as quickly as possible because it is dangerous to
	// be on the water.  Don't add this penalty if the unit is human controlled however, we will assume they want
	// the best path, rather than the safest.
#ifdef AUI_ASTAR_HUMAN_UNITS_GET_DIMINISHED_AVOID_WEIGHT
	if (pCacheData->getDomainType() == DOMAIN_LAND && (pToPlot->isWater() && !pToPlot->IsAllowsWalkWater()))
	{
		if (!pCacheData->isHuman() || pCacheData->IsAutomated())
		{
			iCost += PATH_THROUGH_WATER;
		}
		else
		{
			iCost += AUI_ASTAR_HUMAN_UNITS_GET_DIMINISHED_AVOID_WEIGHT;
		}
	}
#else
	if(pCacheData->getDomainType() == DOMAIN_LAND && (pToPlot->isWater() && !pToPlot->IsAllowsWalkWater()) && (!pCacheData->isHuman() || pUnit->IsAutomated()))
	{
		iCost += PATH_THROUGH_WATER;
	}
#endif

	if(pUnit->IsCombatUnit())
	{
#if defined(AUI_ASTAR_AVOID_RIVER_CROSSING_WHEN_ATTACKING) || defined(AUI_ASTAR_CONSIDER_DAMAGE_WHEN_ATTACKING)
		bool bToPlotHasEnemy = pToPlot->isVisibleEnemyDefender(pUnit) || pToPlot->isEnemyCity(*pUnit);
		if (iMovesLeft == 0 && !bToPlotHasEnemy)
#else
		if(iMovesLeft == 0)
#endif
		{
#ifdef AUI_ASTAR_FIX_DEFENSE_PENALTIES_CONSIDERED_FOR_UNITS_WITHOUT_DEFENSE_BONUS
			int iDefenseBonus = pToPlot->defenseModifier(eUnitTeam, false);
			if (iDefenseBonus > 0)
			{
				if (pUnit->noDefensiveBonus())
					iDefenseBonus = 0;
				else if (iDefenseBonus > 200)
					iDefenseBonus = 200;
			}
			iCost += PATH_DEFENSE_WEIGHT * (200 - iDefenseBonus);
#else
#ifdef AUI_FAST_COMP
			iCost += (PATH_DEFENSE_WEIGHT * FASTMAX(0, (200 - ((pUnit->noDefensiveBonus()) ? 0 : pToPlot->defenseModifier(eUnitTeam, false)))));
#else
			iCost += (PATH_DEFENSE_WEIGHT * std::max(0, (200 - ((pUnit->noDefensiveBonus()) ? 0 : pToPlot->defenseModifier(eUnitTeam, false)))));
#endif
#endif
		}

#if !defined(AUI_ASTAR_AVOID_RIVER_CROSSING_WHEN_ATTACKING) && !defined(AUI_ASTAR_CONSIDER_DAMAGE_WHEN_ATTACKING)
		if(pCacheData->IsAutomated())
#endif
		{
			if(pCacheData->IsCanAttack())
			{
#ifdef AUI_ASTAR_USE_DELEGATES
				if (IsPathDest(pToPlot->getX(), pToPlot->getY()))
#else
				if(finder->IsPathDest(pToPlot->getX(), pToPlot->getY()))
#endif
				{
#if defined(AUI_ASTAR_AVOID_RIVER_CROSSING_WHEN_ATTACKING) || defined(AUI_ASTAR_CONSIDER_DAMAGE_WHEN_ATTACKING)
					if (bToPlotHasEnemy)
#else
					if(pToPlot->isVisibleEnemyDefender(pUnit))
#endif
					{
#ifdef AUI_ASTAR_CONSIDER_DAMAGE_WHEN_ATTACKING
						int iDealtDamage = 0;
						int iSelfDamage = 0;
						CvCity* pCity = pToPlot->getPlotCity();
						if (pCity)
						{
							int iAttackerStrength = pUnit->GetMaxAttackStrength(pFromPlot, pToPlot, NULL);
							int iDefenderStrength = pCity->getStrengthValue();

							iDealtDamage = pUnit->getCombatDamage(iAttackerStrength, iDefenderStrength, pUnit->getDamage(), /*bIncludeRand*/ false, /*bAttackerIsCity*/ false, /*bDefenderIsCity*/ true);
							iSelfDamage = pUnit->getCombatDamage(iDefenderStrength, iAttackerStrength, pCity->getDamage(), /*bIncludeRand*/ false, /*bAttackerIsCity*/ true, /*bDefenderIsCity*/ false);

							// Will both the attacker die, and the city fall? If so, the unit wins
							if (iDealtDamage + pCity->getDamage() >= pCity->GetMaxHitPoints())
							{
								if (pUnit->isNoCapture())
									iDealtDamage = pCity->GetMaxHitPoints() - pCity->getDamage() - 1;
								if (iSelfDamage >= pUnit->GetCurrHitPoints())
									iSelfDamage = pUnit->GetCurrHitPoints() - 1;
							}
						}
						else
						{
							CvUnit* pDefender = pToPlot->getVisibleEnemyDefender(pUnit);
							if (pDefender && pDefender->IsCanDefend())
							{
								// handle the Zulu special thrown spear first attack
								if (pUnit->isRangedSupportFire() && pUnit->canEverRangeStrikeAt(pToPlot, pFromPlot))
									iDealtDamage = pUnit->GetRangeCombatDamage(pDefender, /*pCity*/ NULL, /*bIncludeRand*/ false, 0, NULL, pFromPlot);

								if (iDealtDamage < pDefender->GetCurrHitPoints())
								{
									int iAttackerStrength = pUnit->GetMaxAttackStrength(pFromPlot, pToPlot, pDefender);
									int iDefenderStrength = pDefender->GetMaxDefenseStrength(pToPlot, pUnit);

									if (pUnit->IsCanHeavyCharge() && !pDefender->CanFallBackFromMelee(*pUnit))
										iAttackerStrength = (iAttackerStrength * 150) / 100;

									iSelfDamage = pDefender->getCombatDamage(iDefenderStrength, iAttackerStrength, pDefender->getDamage() + iDealtDamage, /*bIncludeRand*/ false, /*bAttackerIsCity*/ false, /*bDefenderIsCity*/ false);
									iDealtDamage = pUnit->getCombatDamage(iAttackerStrength, iDefenderStrength, pUnit->getDamage(), /*bIncludeRand*/ false, /*bAttackerIsCity*/ false, /*bDefenderIsCity*/ false);

									// Will both units be killed by this? :o If so, take drastic corrective measures
									if (iDealtDamage >= pDefender->GetCurrHitPoints() && iSelfDamage >= pUnit->GetCurrHitPoints())
									{
										// He who hath the least amount of damage survives with 1 HP left
										if (iDealtDamage + pDefender->getDamage() > iSelfDamage + pUnit->getDamage())
											iSelfDamage = pUnit->GetCurrHitPoints() - 1;
										else
											iDealtDamage = pDefender->GetCurrHitPoints() - 1;
									}
								}
							}
						}
						if (iSelfDamage > pUnit->GetCurrHitPoints())
							iSelfDamage = pUnit->GetMaxHitPoints();
						if (iDealtDamage > GC.getMAX_HIT_POINTS())
							iDealtDamage = GC.getMAX_HIT_POINTS();
						iCost += iSelfDamage * PATH_DAMAGE_WEIGHT * pUnit->GetMaxHitPoints() / 100 + (GC.getMAX_HIT_POINTS() - iDealtDamage) * PATH_DAMAGE_WEIGHT / 10;
#else
#ifdef AUI_ASTAR_FIX_DEFENSE_PENALTIES_CONSIDERED_FOR_UNITS_WITHOUT_DEFENSE_BONUS
						int iDefenseBonus = pFromPlot->defenseModifier(eUnitTeam, false);
						if (iDefenseBonus > 0)
						{
							if (pUnit->noDefensiveBonus())
								iDefenseBonus = 0;
							else if (iDefenseBonus > 200)
								iDefenseBonus = 200;
						}
						iCost += PATH_DEFENSE_WEIGHT * (200 - iDefenseBonus);
#else
#ifdef AUI_FAST_COMP
						iCost += (PATH_DEFENSE_WEIGHT * FASTMAX(0, (200 - ((pUnit->noDefensiveBonus()) ? 0 : pFromPlot->defenseModifier(eUnitTeam, false)))));
#else
						iCost += (PATH_DEFENSE_WEIGHT * std::max(0, (200 - ((pUnit->noDefensiveBonus()) ? 0 : pFromPlot->defenseModifier(eUnitTeam, false)))));
#endif
#endif

						// I guess we may as well be the garrison
#if PATH_CITY_WEIGHT != 0
						if(!(pFromPlot->isCity()))
						{
							iCost += PATH_CITY_WEIGHT;
						}
#endif

						if(!(pUnit->isRiverCrossingNoPenalty()))
						{
							if(pFromPlot->isRiverCrossing(directionXY(pFromPlot, pToPlot)))
							{
								iCost += (PATH_RIVER_WEIGHT * -(GC.getRIVER_ATTACK_MODIFIER()));
								iCost += (PATH_MOVEMENT_WEIGHT * iMovesLeft);
							}
						}
#endif
					}
				}
			}
		}
	}

	FAssert(iCost != MAX_INT);

	iCost += PATH_STEP_WEIGHT;

	FAssert(iCost > 0);

	return iCost;
}


//	--------------------------------------------------------------------------------
/// Ignore units path finder - check validity of a coordinate
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::IgnoreUnitsValid(CvAStarNode* parent, CvAStarNode* node, int data)
#else
int IgnoreUnitsValid(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	CvUnit* pUnit;
	CvPlot* pFromPlot;
	CvPlot* pToPlot;
	bool bAIControl;
#endif

	if(parent == NULL)
	{
		return TRUE;
	}

#ifndef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	CvMap& theMap = GC.getMap();
#endif

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	CvPlot* pFromPlot = parent->m_pPlot;
	CvPlot* pToPlot = node->m_pPlot;
	if (!pToPlot || !pFromPlot)
		return FALSE;
#else
	CvPlot* pFromPlot = theMap.plotUnchecked(parent->m_iX, parent->m_iY);
	CvPlot* pToPlot = theMap.plotUnchecked(node->m_iX, node->m_iY);
#endif

#ifdef AUI_ASTAR_USE_DELEGATES
	CvUnit* pUnit = ((CvUnit*)m_pData);
#else
	CvUnit* pUnit = ((CvUnit*)pointer);
#endif
#else
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	pFromPlot = parent->m_pPlot;
	pToPlot = node->m_pPlot;
#else
	pFromPlot = theMap.plotUnchecked(parent->m_iX, parent->m_iY);
	pToPlot = theMap.plotUnchecked(node->m_iX, node->m_iY);
#endif

#ifdef AUI_ASTAR_USE_DELEGATES
	pUnit = ((CvUnit*)m_pData);
#else
	pUnit = ((CvUnit*)pointer);
#endif
#endif
#ifdef AUI_ASTAR_USE_DELEGATES
	const UnitPathCacheData* pCacheData = reinterpret_cast<const UnitPathCacheData*>(GetScratchBuffer());
#else
	const UnitPathCacheData* pCacheData = reinterpret_cast<const UnitPathCacheData*>(finder->GetScratchBuffer());
#endif

	TeamTypes eUnitTeam = pCacheData->getTeam();

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pUnitPlot = pUnit->plot();
#else
	CvPlot* pUnitPlot = theMap.plotUnchecked(pUnit->getX(), pUnit->getY());
#endif

	// slewis - moved this up so units can't move directly into the water. Not 100% sure this is the right solution.
	if(pCacheData->getDomainType() == DOMAIN_LAND)
	{
		if(!pFromPlot->isWater() && pToPlot->isWater() && pToPlot->isRevealed(eUnitTeam) && !pUnit->canEmbarkOnto(*pFromPlot, *pToPlot, true))
		{
			return FALSE;
		}
	}

	if(pUnitPlot == pFromPlot)
	{
		return TRUE;
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	if (GetInfo() & MOVE_TERRITORY_NO_UNEXPLORED)
#else
	if(finder->GetInfo() & MOVE_TERRITORY_NO_UNEXPLORED)
#endif
	{
		if(!(pFromPlot->isRevealed(eUnitTeam)))
		{
			return FALSE;
		}

		if(pFromPlot->isOwned())
		{
			if(pFromPlot->getTeam() != eUnitTeam)
			{
				return FALSE;
			}
		}
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	if (GetInfo() & MOVE_TERRITORY_NO_ENEMY)
#else
	if(finder->GetInfo() & MOVE_TERRITORY_NO_ENEMY)
#endif
	{
		if(pFromPlot->isOwned())
		{
			if(atWar(pFromPlot->getTeam(), eUnitTeam))
			{
				return FALSE;
			}
		}
	}

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	bool bAIControl = pUnit->IsAutomated();
#else
	bAIControl = pUnit->IsAutomated();
#endif

	// slewis - added AI check and embark check to prevent units from moving into unexplored areas
	if(bAIControl || (pFromPlot->isRevealed(eUnitTeam) || pCacheData->isEmbarked()) || !pCacheData->isHuman())
	{
		if(!pUnit->canEnterTerrain(*pToPlot, CvUnit::MOVEFLAG_PRETEND_CORRECT_EMBARK_STATE) || !pUnit->canEnterTerritory(eUnitTeam))
		{
			return FALSE;
		}
	}

	return TRUE;
}

//	--------------------------------------------------------------------------------
/// Ignore units path finder - add a new path
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::IgnoreUnitsPathAdd(CvAStarNode* parent, CvAStarNode* node, int data)
#else
int IgnoreUnitsPathAdd(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
	int iTurns;

#ifdef AUI_ASTAR_USE_DELEGATES
	CvUnit* pUnit = ((CvUnit*)m_pData);
	const UnitPathCacheData* pCacheData = reinterpret_cast<const UnitPathCacheData*>(GetScratchBuffer());
#else
	CvUnit* pUnit = ((CvUnit*)pointer);
	const UnitPathCacheData* pCacheData = reinterpret_cast<const UnitPathCacheData*>(finder->GetScratchBuffer());
#endif
	int iMoves = MAX_INT;

	if(data == ASNC_INITIALADD)
	{
		iTurns = 1;
#ifdef AUI_FAST_COMP
		iMoves = FASTMIN(iMoves, pUnit->movesLeft());
#else
		iMoves = std::min(iMoves, pUnit->movesLeft());
#endif
	}
	else
	{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
		CvPlot* pFromPlot = parent->m_pPlot;
		CvPlot* pToPlot = node->m_pPlot;
#else
		CvPlot* pFromPlot = GC.getMap().plotUnchecked(parent->m_iX, parent->m_iY);
		CvPlot* pToPlot = GC.getMap().plotUnchecked(node->m_iX, node->m_iY);
#endif

		int iStartMoves = parent->m_iData1;
		iTurns = parent->m_iData2;

		if(iStartMoves == 0)
		{
			iTurns++;
			iStartMoves = pCacheData->baseMoves((pToPlot->isWater() && !pToPlot->IsAllowsWalkWater()) ? DOMAIN_SEA : DOMAIN_LAND) * GC.getMOVE_DENOMINATOR();
		}

		// We can't use maxMoves, because that checks where the unit is currently, and we're plotting a path so we have to see
		// what the max moves would be like if the unit was already at the desired location.
#ifdef AUI_FAST_COMP
		iMoves = FASTMIN(iMoves, FASTMAX(0, iStartMoves - CvUnitMovement::MovementCostNoZOC(pUnit, pFromPlot, pToPlot, pCacheData->baseMoves((pToPlot->isWater() || pCacheData->isEmbarked())?DOMAIN_SEA:pCacheData->getDomainType()), pCacheData->maxMoves())));
#else
		iMoves = std::min(iMoves, std::max(0, iStartMoves - CvUnitMovement::MovementCostNoZOC(pUnit, pFromPlot, pToPlot, pCacheData->baseMoves((pToPlot->isWater() || pCacheData->isEmbarked())?DOMAIN_SEA:pCacheData->getDomainType()), pCacheData->maxMoves())));
#endif
	}

	FAssertMsg(iMoves >= 0, "iMoves is expected to be non-negative (invalid Index)");

	node->m_iData1 = iMoves;
	node->m_iData2 = iTurns;

	return 1;
}


//	--------------------------------------------------------------------------------
/// Step path finder - is this end point for the path valid?
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::StepDestValid(int iToX, int iToY) const
#else
int StepDestValid(int iToX, int iToY, const void* pointer, CvAStar* finder)
#endif
{
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pFromPlot;
	CvPlot* pToPlot;
#endif

	CvMap& kMap = GC.getMap();
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
#ifdef AUI_ASTAR_USE_DELEGATES
	CvPlot* pFromPlot = kMap.plotUnchecked(GetStartX(), GetStartY());
#else
	CvPlot* pFromPlot = kMap.plotUnchecked(finder->GetStartX(), finder->GetStartY());
#endif
	CvPlot* pToPlot = kMap.plotUnchecked(iToX, iToY);
#else
#ifdef AUI_ASTAR_USE_DELEGATES
	pFromPlot = kMap.plotUnchecked(GetStartX(), GetStartY());
#else
	pFromPlot = kMap.plotUnchecked(finder->GetStartX(), finder->GetStartY());
#endif
	pToPlot = kMap.plotUnchecked(iToX, iToY);
#endif

	if(pFromPlot->getArea() != pToPlot->getArea())
	{
		return FALSE;
	}

	return TRUE;
}


//	--------------------------------------------------------------------------------
/// Step path finder - determine heuristic cost
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::StepHeuristic(int iFromX, int iFromY, int iToX, int iToY) const
#else
int StepHeuristic(int iFromX, int iFromY, int iToX, int iToY)
#endif
{
	return plotDistance(iFromX, iFromY, iToX, iToY);
}


//	--------------------------------------------------------------------------------
/// Step path finder - compute cost of a path
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::StepCost(CvAStarNode* parent, CvAStarNode* node, int data) const
#else
int StepCost(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
	return 1;
}


//	--------------------------------------------------------------------------------
/// Step path finder - check validity of a coordinate
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::StepValid(CvAStarNode* parent, CvAStarNode* node, int data)
#else
int StepValid(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
	if(parent == NULL)
	{
		return TRUE;
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	PlayerTypes ePlayer = (PlayerTypes)(GetInfo() & 0xFF);

	PlayerTypes eEnemy = (PlayerTypes)m_iData;
#else
	int iFlags = finder->GetInfo();
	PlayerTypes ePlayer = (PlayerTypes)(iFlags & 0xFF);

	PlayerTypes eEnemy = *(PlayerTypes*)pointer;
#endif

	CvPlayer& thisPlayer = GET_PLAYER(ePlayer);

#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	CvPlot* pNewPlot = node->m_pPlot;
	if (!pNewPlot)
		return FALSE;
	if (parent->m_pPlot && parent->m_pPlot->getArea() != pNewPlot->getArea())
#else
	CvMap& kMap = GC.getMap();
	CvPlot* pNewPlot = kMap.plotUnchecked(node->m_iX, node->m_iY);

	if(kMap.plotUnchecked(parent->m_iX, parent->m_iY)->getArea() != pNewPlot->getArea())
#endif
	{
		return FALSE;
	}

#ifdef AUI_ASTAR_FIX_STEP_VALID_CONSIDERS_MOUNTAINS
	if (pNewPlot->isImpassable())
#else
	if(pNewPlot->isImpassable() || pNewPlot->isMountain())
#endif
	{
		return FALSE;
	}

	// Ocean hex and team can't navigate on oceans?
	if (!GET_TEAM(thisPlayer.getTeam()).getEmbarkedAllWaterPassage())
	{
		if (pNewPlot->getTerrainType() == TERRAIN_OCEAN)
		{
			return FALSE;
		}
	}

	PlayerTypes ePlotOwnerPlayer = pNewPlot->getOwner();
	if (ePlotOwnerPlayer != NO_PLAYER && ePlotOwnerPlayer != eEnemy && !pNewPlot->IsFriendlyTerritory(ePlayer))
	{
		CvPlayer& plotOwnerPlayer = GET_PLAYER(ePlotOwnerPlayer);
		bool bPlotOwnerIsMinor = plotOwnerPlayer.isMinorCiv();

		if(!bPlotOwnerIsMinor)
		{
			TeamTypes eMyTeam = thisPlayer.getTeam();
			TeamTypes ePlotOwnerTeam = plotOwnerPlayer.getTeam();

			if(!atWar(eMyTeam, ePlotOwnerTeam))
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}


//	--------------------------------------------------------------------------------
/// Step path finder - check validity of a coordinate (special case that allows any area)
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::StepValidAnyArea(CvAStarNode* parent, CvAStarNode* node, int data)
#else
int StepValidAnyArea(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
	if(parent == NULL)
	{
		return TRUE;
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	PlayerTypes ePlayer = (PlayerTypes)(GetInfo() & 0xFF);

	PlayerTypes eEnemy = (PlayerTypes)m_iData;
#else
	int iFlags = finder->GetInfo();
	PlayerTypes ePlayer = (PlayerTypes)(iFlags & 0xFF);

	PlayerTypes eEnemy = *(PlayerTypes*)pointer;
#endif

	CvPlayer& thisPlayer = GET_PLAYER(ePlayer);

#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	CvPlot* pNewPlot = node->m_pPlot;
	if (!pNewPlot)
		return FALSE;
#else
	CvMap& kMap = GC.getMap();
	CvPlot* pNewPlot = kMap.plotUnchecked(node->m_iX, node->m_iY);
#endif

	//if(kMap.plotUnchecked(parent->m_iX, parent->m_iY)->getArea() != pNewPlot->getArea())
	//{
	//	return FALSE;
	//}

	if(pNewPlot->isImpassable())
	{
		return FALSE;
	}

	// Ocean hex and team can't navigate on oceans?
	if (!GET_TEAM(thisPlayer.getTeam()).getEmbarkedAllWaterPassage())
	{
		if (pNewPlot->getTerrainType() == TERRAIN_OCEAN)
		{
			return FALSE;
		}
	}

	PlayerTypes ePlotOwnerPlayer = pNewPlot->getOwner();
	if (ePlotOwnerPlayer != NO_PLAYER && ePlotOwnerPlayer != eEnemy && !pNewPlot->IsFriendlyTerritory(ePlayer))
	{
		CvPlayer& plotOwnerPlayer = GET_PLAYER(ePlotOwnerPlayer);
		bool bPlotOwnerIsMinor = plotOwnerPlayer.isMinorCiv();

		if(!bPlotOwnerIsMinor)
		{
			TeamTypes eMyTeam = thisPlayer.getTeam();
			TeamTypes ePlotOwnerTeam = plotOwnerPlayer.getTeam();

			if(!atWar(eMyTeam, ePlotOwnerTeam))
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}


//	--------------------------------------------------------------------------------
/// Step path finder - add a new path
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::StepAdd(CvAStarNode* parent, CvAStarNode* node, int data) const
#else
int StepAdd(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
	if(data == ASNC_INITIALADD)
	{
		node->m_iData1 = 0;
	}
	else
	{
		node->m_iData1 = (parent->m_iData1 + 1);
	}

	FAssertMsg(node->m_iData1 >= 0, "node->m_iData1 is expected to be non-negative (invalid Index)");

	return 1;
}


//	--------------------------------------------------------------------------------
/// Influence path finder - is this end point for the path valid?
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::InfluenceDestValid(int iToX, int iToY)
#else
int InfluenceDestValid(int iToX, int iToY, const void* pointer, CvAStar* finder)
#endif
{
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pFromPlot;
	CvPlot* pToPlot;
#endif

	CvMap& kMap = GC.getMap();
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
#ifdef AUI_ASTAR_USE_DELEGATES
	CvPlot* pFromPlot = kMap.plotUnchecked(GetStartX(), GetStartY());
#else
	CvPlot* pFromPlot = kMap.plotUnchecked(finder->GetStartX(), finder->GetStartY());
#endif
	CvPlot* pToPlot = kMap.plotUnchecked(iToX, iToY);
#else
	pFromPlot = kMap.plotUnchecked(finder->GetStartX(), finder->GetStartY());
	pToPlot = kMap.plotUnchecked(iToX, iToY);
#endif

#ifdef AUI_ASTAR_USE_DELEGATES
	if (m_iData)
	{
		int iRange = abs(m_iData);
#else
	if(pointer)
	{
		int iRange = abs(*(int*)pointer);
#endif
		if(plotDistance(pFromPlot->getX(),pFromPlot->getY(),pToPlot->getX(),pToPlot->getY()) > iRange)
		{
			return FALSE;
		}
	}

	return TRUE;
}


//	--------------------------------------------------------------------------------
/// Influence path finder - determine heuristic cost
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::InfluenceHeuristic(int iFromX, int iFromY, int iToX, int iToY) const
#else
int InfluenceHeuristic(int iFromX, int iFromY, int iToX, int iToY)
#endif
{
	return plotDistance(iFromX, iFromY, iToX, iToY);
}

//	--------------------------------------------------------------------------------
/// Influence path finder - compute cost of a path
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::InfluenceCost(CvAStarNode* parent, CvAStarNode* node, int data)
#else
int InfluenceCost(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
	int iCost = 0;
	bool bDifferentOwner = false;
	if(parent->m_pParent || GC.getUSE_FIRST_RING_INFLUENCE_TERRAIN_COST())
	{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
		CvPlot* pFromPlot = parent->m_pPlot;
		CvPlot* pToPlot = node->m_pPlot;
#ifdef AUI_ASTAR_USE_DELEGATES
		CvPlot* pSourcePlot = GC.getMap().plotUnchecked(GetStartX(), GetStartY());
#else
		CvPlot* pSourcePlot = GC.getMap().plotUnchecked(finder->GetStartX(), finder->GetStartY());
#endif
#else
		CvMap& kMap = GC.getMap();
		CvPlot* pFromPlot = kMap.plotUnchecked(parent->m_iX, parent->m_iY);
		CvPlot* pToPlot = kMap.plotUnchecked(node->m_iX, node->m_iY);
#ifdef AUI_ASTAR_USE_DELEGATES
		CvPlot* pSourcePlot = kMap.plotUnchecked(GetStartX(), GetStartY());
#else
		CvPlot* pSourcePlot = kMap.plotUnchecked(finder->GetStartX(), finder->GetStartY());
#endif
#endif

		int iRange = 0;
#ifdef AUI_ASTAR_USE_DELEGATES
		if(m_iData)
		{
			iRange = m_iData;
#else
		if(pointer)
		{
			iRange = *(int*)pointer;
#endif
		}
		if(iRange >= 0)
		{
			if(pToPlot->getOwner() != NO_PLAYER && pSourcePlot->getOwner() != NO_PLAYER && pToPlot->getOwner() != pSourcePlot->getOwner())
				bDifferentOwner = true;
		}

		if(pFromPlot->isRiverCrossing(directionXY(pFromPlot, pToPlot)))
			iCost += GC.getINFLUENCE_RIVER_COST();

		// Mountain Cost
		if(pToPlot->isMountain())
			iCost += GC.getINFLUENCE_MOUNTAIN_COST();
		// Not a mountain - use the terrain cost
		else
		{
			// Hill cost
			if(pToPlot->isHills())
				iCost += GC.getINFLUENCE_HILL_COST();
			iCost += GC.getTerrainInfo(pToPlot->getTerrainType())->getInfluenceCost();
			iCost += ((pToPlot->getFeatureType() == NO_FEATURE) ? 0 : GC.getFeatureInfo(pToPlot->getFeatureType())->getInfluenceCost());
		}
	}
	else
	{
		iCost = 1;
	}
#ifdef AUI_FAST_COMP
	iCost = FASTMAX(1,iCost);
	iCost = FASTMIN(3,iCost);
#else
	iCost = std::max(1,iCost);
	iCost = std::min(3,iCost);
#endif
	if (bDifferentOwner)
	{
		iCost += 15;
	}
	return iCost;
}


//	--------------------------------------------------------------------------------
/// Influence path finder - check validity of a coordinate
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::InfluenceValid(CvAStarNode* parent, CvAStarNode* node, int data) const
#else
int InfluenceValid(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pNewPlot;
#endif

	if(parent == NULL)
	{
		return TRUE;
	}

#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	if (!node->m_pPlot)
	{
		return FALSE;
	}
#else
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	if (!GC.getMap().isPlot(node->m_iX, node->m_iY))
	{
		return FALSE;
	}
#else
	pNewPlot = GC.getMap().plotCheckInvalid(node->m_iX, node->m_iY);

	if(pNewPlot == NULL)
	{
		return FALSE;
	}
#endif
#endif

	// todo: a check to see if we are within the theoretical influence range would be great

	return TRUE;
}


//	--------------------------------------------------------------------------------
/// Influence path finder - add a new path
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::InfluenceAdd(CvAStarNode* parent, CvAStarNode* node, int data) const
#else
int InfluenceAdd(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
	if(data == ASNC_INITIALADD)
	{
		node->m_iData1 = 0;
	}
	else
	{
		node->m_iData1 = (parent->m_iData1 + 1/*influenceCost(parent,node,data,pointer,finder)*/);
	}

	FAssertMsg(node->m_iData1 >= 0, "node->m_iData1 is expected to be non-negative (invalid Index)");

	return 1;
}


//	--------------------------------------------------------------------------------
// Route - Return the x, y plot of the node that we want to access
#ifdef AUI_ASTAR_USE_DELEGATES
CvAStarNode* CvAStar::RouteGetExtraChild(CvAStarNode* node, int iIndex) const
{
	int iX = -1;
	int iY = -1;

	PlayerTypes ePlayer = ((PlayerTypes)(GetInfo() & 0xFF));
#else
int RouteGetExtraChild(CvAStarNode* node, int iIndex, int& iX, int& iY, CvAStar* finder)
{
	iX = -1;
	iY = -1;

	PlayerTypes ePlayer = ((PlayerTypes)(finder->GetInfo() & 0xFF));
#endif
	CvPlayerAI& kPlayer = GET_PLAYER(ePlayer);
	TeamTypes eTeam = kPlayer.getTeam();
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	CvPlot* pPlot = node->m_pPlot;
#else
	CvPlot* pPlot = GC.getMap().plotCheckInvalid(node->m_iX, node->m_iY);
#endif

	if(!pPlot)
	{
		return 0;
	}

	CvCity* pCity = pPlot->getPlotCity();

	// if there isn't a city there or the city isn't on our team
	if(!pCity || pCity->getTeam() != eTeam)
	{
		return 0;
	}

	int iValidCount = 0;
	CvCityConnections* pCityConnections = kPlayer.GetCityConnections();

	uint uiFirstCityIndex = pCityConnections->GetIndexFromCity(pCity);
	for(uint uiSecondCityIndex = 0; uiSecondCityIndex < pCityConnections->m_aiCityPlotIDs.size(); uiSecondCityIndex++)
	{
		if(uiFirstCityIndex == uiSecondCityIndex)
		{
			continue;
		}

		CvCityConnections::RouteInfo* pRouteInfo = pCityConnections->GetRouteInfo(uiFirstCityIndex, uiSecondCityIndex);
		if(!pRouteInfo)
		{
			continue;
		}

		// get the two cities
		CvCity* pFirstCity  = pCityConnections->GetCityFromIndex(uiFirstCityIndex);
		CvCity* pSecondCity = pCityConnections->GetCityFromIndex(uiSecondCityIndex);

		if(!pFirstCity || !pSecondCity)
		{
			continue;
		}

		if(pRouteInfo->m_cRouteState & CvCityConnections::HAS_WATER_ROUTE)
		{
			if(iValidCount == iIndex)
			{
				iX = pSecondCity->getX();
				iY = pSecondCity->getY();
#ifdef AUI_ASTAR_USE_DELEGATES
				break;
#else
				return 1;
#endif
			}
			iValidCount++;
		}
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	if (isValid(iX, iY))
	{
		return &(m_ppaaNodes[iX][iY]);
	}
#endif

	return 0;
}

//	---------------------------------------------------------------------------
/// Route path finder - check validity of a coordinate
/// This function does not require the global Tactical Analysis Map.
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::RouteValid(CvAStarNode* parent, CvAStarNode* node, int data) const
#else
int RouteValid(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pNewPlot;
#endif

	if(parent == NULL)
	{
		return TRUE;
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	PlayerTypes ePlayer = (PlayerTypes)(GetInfo() & 0xFF);
#else
	int iFlags = finder->GetInfo();
	PlayerTypes ePlayer = (PlayerTypes)(iFlags & 0xFF);
#endif
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pNewPlot = node->m_pPlot;
#else
	pNewPlot = node->m_pPlot;
#endif
	if (!pNewPlot)
		return FALSE;
#else
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pNewPlot = GC.getMap().plotUnchecked(node->m_iX, node->m_iY);
#else
	pNewPlot = GC.getMap().plotUnchecked(node->m_iX, node->m_iY);
#endif
#endif


	CvPlayer& kPlayer = GET_PLAYER(ePlayer);
#ifdef AUI_ASTAR_USE_DELEGATES
	if ((GetInfo() & MOVE_ROUTE_ALLOW_UNEXPLORED) == 0 && !(pNewPlot->isRevealed(kPlayer.getTeam())))
#else
	if((iFlags & MOVE_ROUTE_ALLOW_UNEXPLORED) == 0 && !(pNewPlot->isRevealed(kPlayer.getTeam())))
#endif
	{
		return FALSE;
	}

	if(kPlayer.GetPlayerTraits()->IsMoveFriendlyWoodsAsRoad())
	{
		if(pNewPlot->getOwner() == ePlayer)
		{
			if(pNewPlot->getFeatureType() == FEATURE_FOREST || pNewPlot->getFeatureType() == FEATURE_JUNGLE)
			{
				return TRUE;
			}
		}
	}

	RouteTypes eRouteType = pNewPlot->getRouteType();
	if(eRouteType == NO_ROUTE)
	{
		return FALSE;
	}

	if(pNewPlot->IsRoutePillaged())
	{
		return FALSE;
	}

	if(!pNewPlot->IsFriendlyTerritory(ePlayer))
	{
		PlayerTypes ePlotOwnerPlayer = pNewPlot->getOwner();
		if(ePlotOwnerPlayer != NO_PLAYER)
		{
			PlayerTypes eMajorPlayer = NO_PLAYER;
			PlayerTypes eMinorPlayer = NO_PLAYER;
			CvPlayer& kPlotOwner = GET_PLAYER(ePlotOwnerPlayer);
			if(kPlayer.isMinorCiv() && !kPlotOwner.isMinorCiv())
			{
				eMajorPlayer = ePlotOwnerPlayer;
				eMinorPlayer = ePlayer;
			}
			else if(kPlotOwner.isMinorCiv() && !kPlayer.isMinorCiv())
			{
				eMajorPlayer = ePlayer;
				eMinorPlayer = ePlotOwnerPlayer;
			}
			else
			{
				return FALSE;
			}

			if(!GET_PLAYER(eMinorPlayer).GetMinorCivAI()->IsActiveQuestForPlayer(eMajorPlayer, MINOR_CIV_QUEST_ROUTE))
			{
				return FALSE;
			}
		}
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	if (GetInfo() & MOVE_ANY_ROUTE)
#else
	if(finder->GetInfo() & MOVE_ANY_ROUTE)
#endif
	{
		// if the player can't build
		if(kPlayer.getBestRoute() == NO_ROUTE)
		{
			return FALSE;
		}

		if(eRouteType != NO_ROUTE)
		{
			return TRUE;
		}
	}
	else
	{
#ifdef AUI_ASTAR_USE_DELEGATES
		int iRoute = GetInfo() & 0xFF00;
#else
		int iRoute = iFlags & 0xFF00;
#endif
		iRoute = iRoute >> 8;
		iRoute = iRoute - 1;
		RouteTypes eRequiredRoute = (RouteTypes)(iRoute);
		if(eRouteType == eRequiredRoute)
		{
			return TRUE;
		}
	}

	return FALSE;
}

//	---------------------------------------------------------------------------
// Route - find the number of additional children. In this case, the node is at a city, push all other cities that the city has a water connection to
// This function does not require the global Tactical Analysis Map.
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::RouteGetNumExtraChildren(CvAStarNode* node) const
{
	PlayerTypes ePlayer = ((PlayerTypes)(GetInfo() & 0xFF));
#else
int RouteGetNumExtraChildren(CvAStarNode* node,  CvAStar* finder)
{
	PlayerTypes ePlayer = ((PlayerTypes)(finder->GetInfo() & 0xFF));
#endif
	CvPlayerAI& kPlayer = GET_PLAYER(ePlayer);
	TeamTypes eTeam = kPlayer.getTeam();
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	CvPlot* pPlot = node->m_pPlot;
#else
	CvPlot* pPlot = GC.getMap().plotCheckInvalid(node->m_iX, node->m_iY);
#endif

	if(!pPlot)
	{
		return 0;
	}

	// slewis - don't allow the minor civ to use harbors
	if(kPlayer.isMinorCiv())
	{
		return 0;
	}

	CvCityConnections* pCityConnections = kPlayer.GetCityConnections();
	if(pCityConnections->IsEmpty())
	{
		return 0;
	}

	int iResultNum = 0;

	CvCity* pCity = pPlot->getPlotCity();

	// if there isn't a city there or the city isn't on our team
	if(!pCity || pCity->getTeam() != eTeam)
	{
		return 0;
	}

	uint uiFirstCityIndex = pCityConnections->GetIndexFromCity(pCity);
	if(uiFirstCityIndex >= pCityConnections->m_aiCityPlotIDs.size())
	{
		CvAssertMsg(false, "City index out of bounds");
		return 0;
	}

	for(uint uiSecondCityIndex = 0; uiSecondCityIndex < pCityConnections->m_aiCityPlotIDs.size(); uiSecondCityIndex++)
	{
		if(uiFirstCityIndex == uiSecondCityIndex)
		{
			continue;
		}

		CvCityConnections::RouteInfo* pRouteInfo = pCityConnections->GetRouteInfo(uiFirstCityIndex, uiSecondCityIndex);
		if(!pRouteInfo)
		{
			continue;
		}

		// get the two cities
		CvCity* pFirstCity  = pCityConnections->GetCityFromIndex(uiFirstCityIndex);
		CvCity* pSecondCity = pCityConnections->GetCityFromIndex(uiSecondCityIndex);

		if(!pFirstCity || !pSecondCity)
		{
			continue;
		}

		if(pRouteInfo->m_cRouteState & CvCityConnections::HAS_WATER_ROUTE)
		{
			iResultNum++;
		}
	}

	return iResultNum;
}

//	--------------------------------------------------------------------------------
/// Water route valid finder - check the validity of a coordinate
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::WaterRouteValid(CvAStarNode* parent, CvAStarNode* node, int data) const
#else
int WaterRouteValid(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pNewPlot;
#endif

	if(parent == NULL)
	{
		return TRUE;
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	PlayerTypes ePlayer = (PlayerTypes)(GetInfo());
#else
	PlayerTypes ePlayer = (PlayerTypes)(finder->GetInfo());
#endif
	TeamTypes eTeam = GET_PLAYER(ePlayer).getTeam();

#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pNewPlot = node->m_pPlot;
#else
	pNewPlot = node->m_pPlot;
#endif
	if (!pNewPlot)
		return FALSE;
#else
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pNewPlot = GC.getMap().plotUnchecked(node->m_iX, node->m_iY);
#else
	pNewPlot = GC.getMap().plotUnchecked(node->m_iX, node->m_iY);
#endif
#endif

	if(!(pNewPlot->isRevealed(eTeam)))
	{
		return FALSE;
	}

	CvCity* pCity = pNewPlot->getPlotCity();
	if(pCity && pCity->getTeam() == eTeam)
	{
		return TRUE;
	}

	if(pNewPlot->isWater())
	{
		return TRUE;
	}

	return FALSE;
}

//	--------------------------------------------------------------------------------
/// Build route cost
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::BuildRouteCost(CvAStarNode* parent, CvAStarNode* node, int data) const
#else
int BuildRouteCost(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	CvPlot* pPlot = node->m_pPlot;
#else
	CvPlot* pPlot = GC.getMap().plotUnchecked(node->m_iX, node->m_iY);
#endif
#ifdef AUI_ASTAR_USE_DELEGATES
	PlayerTypes ePlayer = (PlayerTypes)(GetInfo() & 0xFF);
#else
	int iFlags = finder->GetInfo();
	PlayerTypes ePlayer = (PlayerTypes)(iFlags & 0xFF);
#endif
	TeamTypes eTeam = GET_PLAYER(ePlayer).getTeam();

#ifdef AUI_ASTAR_USE_DELEGATES
	int iRoute = GetInfo() & 0xFF00;
#else
	int iRoute = iFlags & 0xFF00;
#endif
	iRoute = iRoute >> 8;
	iRoute = iRoute - 1;
	RouteTypes eRoute = (RouteTypes)(iRoute);

	if(pPlot->getRouteType() != NO_ROUTE)
	{
		int iReturnValue = PATH_BUILD_ROUTE_EXISTING_ROUTE_WEIGHT;
		if(pPlot->getRouteType() == eRoute)
		{
			iReturnValue = 1;
		}
		return iReturnValue;
	}

	int iMaxValue = 1500;

	// if the plot is on a removable feature, it tends to be a good idea to build a road here
	int iMovementCost = ((pPlot->getFeatureType() == NO_FEATURE) ? GC.getTerrainInfo(pPlot->getTerrainType())->getMovementCost() : GC.getFeatureInfo(pPlot->getFeatureType())->getMovementCost());

	// calculate the max value based on how much of a movement increase we get
	if(iMovementCost + 1 != 0)
	{
		iMaxValue = iMaxValue / 2 + iMaxValue / (iMovementCost + 1);
	}

	// if the tile already been tagged for building a road, then provide a discount
	if(pPlot->GetBuilderAIScratchPadTurn() == GC.getGame().getGameTurn() && pPlot->GetBuilderAIScratchPadPlayer() == ePlayer)
	{
		iMaxValue = (int)(iMaxValue * PATH_BUILD_ROUTE_ALREADY_FLAGGED_DISCOUNT);
	}

	return iMaxValue;
}

//	--------------------------------------------------------------------------------
/// Build Route path finder - check validity of a coordinate
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::BuildRouteValid(CvAStarNode* parent, CvAStarNode* node, int data) const
#else
int BuildRouteValid(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pNewPlot;
#endif

	if(parent == NULL)
	{
		return TRUE;
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	PlayerTypes ePlayer = (PlayerTypes)(GetInfo() & 0xFF);
#else
	int iFlags = finder->GetInfo();
	PlayerTypes ePlayer = (PlayerTypes)(iFlags & 0xFF);
#endif

	CvPlayer& thisPlayer = GET_PLAYER(ePlayer);
	bool bThisPlayerIsMinor = thisPlayer.isMinorCiv();

#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pNewPlot = node->m_pPlot;
#else
	pNewPlot = node->m_pPlot;
#endif
	if (!pNewPlot)
		return FALSE;
#else
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pNewPlot = GC.getMap().plotUnchecked(node->m_iX, node->m_iY);
#else
	pNewPlot = GC.getMap().plotUnchecked(node->m_iX, node->m_iY);
#endif
#endif
	if(!bThisPlayerIsMinor && !(pNewPlot->isRevealed(thisPlayer.getTeam())))
	{
		return FALSE;
	}

	if(pNewPlot->isWater())
	{
		return FALSE;
	}

#ifdef AUI_ASTAR_FIX_STEP_VALID_CONSIDERS_MOUNTAINS
	if (pNewPlot->isImpassable())
#else
	if(pNewPlot->isImpassable() || pNewPlot->isMountain())
#endif
	{
		return FALSE;
	}

	PlayerTypes ePlotOwnerPlayer = pNewPlot->getOwner();
	if(ePlotOwnerPlayer != NO_PLAYER && !pNewPlot->IsFriendlyTerritory(ePlayer))
	{
		PlayerTypes eMajorPlayer = NO_PLAYER;
		PlayerTypes eMinorPlayer = NO_PLAYER;
		bool bPlotOwnerIsMinor = GET_PLAYER(ePlotOwnerPlayer).isMinorCiv();
		if(bThisPlayerIsMinor && !bPlotOwnerIsMinor)
		{
			eMajorPlayer = ePlotOwnerPlayer;
			eMinorPlayer = ePlayer;
		}
		else if(bPlotOwnerIsMinor && !bThisPlayerIsMinor)
		{
			eMajorPlayer = ePlayer;
			eMinorPlayer = ePlotOwnerPlayer;
		}
		else
		{
			return FALSE;
		}

		if(!GET_PLAYER(eMinorPlayer).GetMinorCivAI()->IsActiveQuestForPlayer(eMajorPlayer, MINOR_CIV_QUEST_ROUTE))
		{
			return FALSE;
		}
	}

	return TRUE;
}


//	--------------------------------------------------------------------------------
/// Area path finder - check validity of a coordinate
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::AreaValid(CvAStarNode* parent, CvAStarNode* node, int data) const
#else
int AreaValid(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
	if(parent == NULL)
	{
		return TRUE;
	}

#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	CvPlot* pOldPlot = parent->m_pPlot;
	CvPlot* pNewPlot = node->m_pPlot;
	if (!pOldPlot || !pNewPlot)
	{
		return FALSE;
	}
	if (pOldPlot->isImpassable() != pNewPlot->isImpassable())
	{
		return FALSE;
	}

	return (pOldPlot->isWater() == pNewPlot->isWater() ? TRUE : FALSE);
#else
	CvMap& kMap = GC.getMap();
	if(kMap.plotUnchecked(parent->m_iX, parent->m_iY)->isImpassable() != kMap.plotUnchecked(node->m_iX, node->m_iY)->isImpassable())
	{
		return FALSE;
	}

	return ((kMap.plotUnchecked(parent->m_iX, parent->m_iY)->isWater() == kMap.plotUnchecked(node->m_iX, node->m_iY)->isWater()) ? TRUE : FALSE);
#endif
}


//	--------------------------------------------------------------------------------
/// Area path finder - callback routine when node added to open/closed list
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::JoinArea(CvAStarNode* parent, CvAStarNode* node, int data) const
#else
int JoinArea(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
	if(data == ASNL_ADDCLOSED)
	{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
#ifdef AUI_ASTAR_USE_DELEGATES
		node->m_pPlot->setArea(GetInfo());
#else
		node->m_pPlot->setArea(finder->GetInfo());
#endif
#else
#ifdef AUI_ASTAR_USE_DELEGATES
		GC.getMap().plotUnchecked(node->m_iX, node->m_iY)->setArea(GetInfo());
#else
		GC.getMap().plotUnchecked(node->m_iX, node->m_iY)->setArea(finder->GetInfo());
#endif
#endif
	}

	return 1;
}


//	--------------------------------------------------------------------------------
/// Area path finder - check validity of a coordinate
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::LandmassValid(CvAStarNode* parent, CvAStarNode* node, int data) const
#else
int LandmassValid(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
	if(parent == NULL)
	{
		return TRUE;
	}

#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	return (parent->m_pPlot && node->m_pPlot && parent->m_pPlot->isWater() == node->m_pPlot->isWater() ? TRUE : FALSE);
#else
	CvMap& kMap = GC.getMap();
	return ((kMap.plotUnchecked(parent->m_iX, parent->m_iY)->isWater() == kMap.plotUnchecked(node->m_iX, node->m_iY)->isWater()) ? TRUE : FALSE);
#endif
}


//	--------------------------------------------------------------------------------
/// Area path finder - callback routine when node added to open/closed list
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::JoinLandmass(CvAStarNode* parent, CvAStarNode* node, int data) const
#else
int JoinLandmass(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
	if(data == ASNL_ADDCLOSED)
	{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
#ifdef AUI_ASTAR_USE_DELEGATES
		node->m_pPlot->setLandmass(GetInfo());
#else
		node->m_pPlot->setLandmass(finder->GetInfo());
#endif
#else
#ifdef AUI_ASTAR_USE_DELEGATES
		GC.getMap().plotUnchecked(node->m_iX, node->m_iY)->setLandmass(GetInfo());
#else
		GC.getMap().plotUnchecked(node->m_iX, node->m_iY)->setLandmass(finder->GetInfo());
#endif
#endif
	}

	return 1;
}


// DERIVED CLASSES (which have more convenient ways to access our various pathfinders)

//	--------------------------------------------------------------------------------
/// Constructor
CvTwoLayerPathFinder::CvTwoLayerPathFinder()
{
	CvAStar::CvAStar();
	m_ppaaPartialMoveNodes = NULL;
}

//	--------------------------------------------------------------------------------
/// Destructor
CvTwoLayerPathFinder::~CvTwoLayerPathFinder()
{
	CvAStar::DeInit();

	DeInit();
}

//	--------------------------------------------------------------------------------
/// Allocate memory, zero variables
#ifdef AUI_ASTAR_USE_DELEGATES
void CvTwoLayerPathFinder::Initialize(int iColumns, int iRows, bool bWrapX, bool bWrapY, CvAPointFunc IsPathDestFunc, CvAPointFunc DestValidFunc, CvAHeuristic HeuristicFunc, CvAStarFunc CostFunc, CvAStarFunc ValidFunc, CvAStarFunc NotifyChildFunc, CvAStarFunc NotifyListFunc, CvABeginOrEnd InitializeFunc, CvABeginOrEnd UninitializeFunc)
#else
void CvTwoLayerPathFinder::Initialize(int iColumns, int iRows, bool bWrapX, bool bWrapY, CvAPointFunc IsPathDestFunc, CvAPointFunc DestValidFunc, CvAHeuristic HeuristicFunc, CvAStarFunc CostFunc, CvAStarFunc ValidFunc, CvAStarFunc NotifyChildFunc, CvAStarFunc NotifyListFunc, CvABegin InitializeFunc, CvAEnd UninitializeFunc, const void* pData)
#endif
{
	int iI, iJ;

	DeInit();

#ifdef AUI_ASTAR_USE_DELEGATES
	CvAStar::Initialize(iColumns, iRows, bWrapX, bWrapY, IsPathDestFunc, DestValidFunc, HeuristicFunc, CostFunc, ValidFunc, NotifyChildFunc, NotifyListFunc, NULL, NULL, InitializeFunc, UninitializeFunc);
#else
	CvAStar::Initialize(iColumns, iRows, bWrapX, bWrapY, IsPathDestFunc, DestValidFunc, HeuristicFunc, CostFunc, ValidFunc, NotifyChildFunc, NotifyListFunc, NULL, NULL, InitializeFunc, UninitializeFunc, pData);
#endif

	m_ppaaPartialMoveNodes = FNEW(CvAStarNode*[m_iColumns], c_eCiv5GameplayDLL, 0);
	for(iI = 0; iI < m_iColumns; iI++)
	{
		m_ppaaPartialMoveNodes[iI] = FNEW(CvAStarNode[m_iRows], c_eCiv5GameplayDLL, 0);
		for(iJ = 0; iJ < m_iRows; iJ++)
		{
			m_ppaaPartialMoveNodes[iI][iJ].m_iX = iI;
			m_ppaaPartialMoveNodes[iI][iJ].m_iY = iJ;
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
			m_ppaaPartialMoveNodes[iI][iJ].m_pPlot = GC.getMap().plot(iI, iJ);
#endif
		}
	}
};

//	--------------------------------------------------------------------------------
/// Frees allocated memory
void CvTwoLayerPathFinder::DeInit()
{
	CvAStar::DeInit();

	if(m_ppaaPartialMoveNodes != NULL)
	{
		for(int iI = 0; iI < m_iColumns; iI++)
		{
			SAFE_DELETE_ARRAY(m_ppaaPartialMoveNodes[iI]);
		}

		SAFE_DELETE_ARRAY(m_ppaaPartialMoveNodes);
	}
}

//	--------------------------------------------------------------------------------
/// Return a node from the second layer of A-star nodes (for the partial moves)
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
CvAStarNode* CvTwoLayerPathFinder::GetPartialMoveNode(int iCol, int iRow) const
#else
CvAStarNode* CvTwoLayerPathFinder::GetPartialMoveNode(int iCol, int iRow)
#endif
{
	return &(m_ppaaPartialMoveNodes[iCol][iRow]);
}

//	--------------------------------------------------------------------------------
/// Return the furthest plot we can get to this turn that is on the path
CvPlot* CvTwoLayerPathFinder::GetPathEndTurnPlot() const
{
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvAStarNode* pNode = m_pBest;
#else
	CvAStarNode* pNode;

	pNode = m_pBest;
#endif

	if(NULL != pNode)
	{
		if((pNode->m_pParent == NULL) || (pNode->m_iData2 == 1))
		{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
			return pNode->m_pPlot;
#else
			return GC.getMap().plotUnchecked(pNode->m_iX, pNode->m_iY);
#endif
		}

		while(pNode->m_pParent != NULL)
		{
			if(pNode->m_pParent->m_iData2 == 1)
			{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
				return pNode->m_pParent->m_pPlot;
#else
				return GC.getMap().plotUnchecked(pNode->m_pParent->m_iX, pNode->m_pParent->m_iY);
#endif
			}

			pNode = pNode->m_pParent;
		}
	}

	FAssert(false);

	return NULL;
}

//	--------------------------------------------------------------------------------
// Path logging
static void LogPathGeneration(const CvUnit *pkUnit, CvString& strMsg)
{
	if(GC.getLogging() && GC.getAILogging() && pkUnit)
	{
		CvString strOutBuf;
		CvString strBaseString;

		CvPlayer& kPlayer = GET_PLAYER(pkUnit->getOwner());
		const char* pszPlayerName = kPlayer.getCivilizationShortDescription();
		FILogFile* pLog = LOGFILEMGR.GetLog((gDLL->IsGameCoreThread())?"AStar_GC.log":"AStar_APP.log", FILogFile::kDontTimeStamp, "Game Turn, Player, Unit, From X, From Y, To X, To Y, Info, Checksum");

		// Get the leading info for this line
		strBaseString.Format("%03d, %s, UnitID: %d, ", GC.getGame().getElapsedGameTurns(), (pszPlayerName)?pszPlayerName:"?", pkUnit->GetID());
		strOutBuf = strBaseString + strMsg;
		pLog->Msg(strOutBuf);
	}
}

//	--------------------------------------------------------------------------------
/// Generate a path, using the supplied unit as the data
#ifdef AUI_ASTAR_TURN_LIMITER
bool CvTwoLayerPathFinder::GenerateUnitPath(const CvUnit* pkUnit, int iXstart, int iYstart, int iXdest, int iYdest, int iInfo, bool bReuse, int iTargetTurns)
#else
bool CvTwoLayerPathFinder::GenerateUnitPath(const CvUnit* pkUnit, int iXstart, int iYstart, int iXdest, int iYdest, int iInfo /*= 0*/, bool bReuse /*= false*/)
#endif
{
	if (pkUnit)
	{
		CvAssert(gDLL->IsGameCoreThread() || !gDLL->IsGameCoreExecuting());
#ifdef AUI_ASTAR_TURN_LIMITER
		SetData(pkUnit, iTargetTurns);
#else
		SetData(pkUnit);
#endif
		bool bResult = GeneratePath(iXstart, iYstart, iXdest, iYdest, iInfo, bReuse);
		if(GC.getLogging() && GC.getAILogging())
		{
			CvString strLogString;
			uint uiChecksum = CRC_INIT;
			// Loop through the nodes and make a checksum
			CvAStarNode* pNode = GetLastNode();

			// Starting at the end, loop until we find a plot from this owner
			while(pNode != NULL)
			{
				// Just do the X/Y for now
				uiChecksum = g_CRC32.Calc( &pNode->m_iX, sizeof( pNode->m_iX ), uiChecksum );
				uiChecksum = g_CRC32.Calc( &pNode->m_iY, sizeof( pNode->m_iY ), uiChecksum );
		
				pNode = pNode->m_pParent;
			}

			strLogString.Format("%d, %d, %d, %d, %d, %8x", iXstart, iYstart, iXdest, iYdest, iInfo, uiChecksum);
			LogPathGeneration(pkUnit, strLogString);
		}
		return bResult;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////
// CvStepPathFinder
//////////////////////////////////////////////////////////////////////////

//	--------------------------------------------------------------------------------
/// Get distance between two plots on same land mass (return -1 if plots are in different areas)
int CvStepPathFinder::GetStepDistanceBetweenPoints(PlayerTypes ePlayer, PlayerTypes eEnemy, CvPlot* pStartPlot, CvPlot* pEndPlot)
{
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	CvAStarNode* pPathfinderNode;
#endif

	if(pStartPlot == NULL || pEndPlot == NULL || pStartPlot->getArea() != pEndPlot->getArea())
	{
		return -1;
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	SetData(eEnemy);
#else
	SetData(&eEnemy);
#endif
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	if (GeneratePath(pStartPlot->getX(), pStartPlot->getY(), pEndPlot->getX(), pEndPlot->getY(), ePlayer, false))
	{
		CvAStarNode* pPathfinderNode = GetLastNode();
#else
	bool bPathfinderSuccess = GeneratePath(pStartPlot->getX(), pStartPlot->getY(), pEndPlot->getX(), pEndPlot->getY(), ePlayer, false);
	if(bPathfinderSuccess)
	{
		pPathfinderNode = GetLastNode();
#endif

		if(pPathfinderNode != NULL)
		{
			return pPathfinderNode->m_iData1;
		}
	}

	return -1;
}

//	--------------------------------------------------------------------------------
/// Check for existence of step path between two points
bool CvStepPathFinder::DoesPathExist(PlayerTypes ePlayer, PlayerTypes eEnemy, CvPlot* pStartPlot, CvPlot* pEndPlot)
{
	if(pStartPlot == NULL || pEndPlot == NULL || pStartPlot->getArea() != pEndPlot->getArea())
	{
		return false;
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	SetData(eEnemy);
#else
	SetData(&eEnemy);
#endif
	return GeneratePath(pStartPlot->getX(), pStartPlot->getY(), pEndPlot->getX(), pEndPlot->getY(), ePlayer, false);
}

//	--------------------------------------------------------------------------------
/// Returns the last plot along the step path owned by a specific player
CvPlot* CvStepPathFinder::GetLastOwnedPlot(PlayerTypes ePlayer, PlayerTypes eEnemy, CvPlot* pStartPlot, CvPlot* pEndPlot) const
{
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	if(GC.getStepFinder().GetStepDistanceBetweenPoints(ePlayer, eEnemy, pStartPlot, pEndPlot) != -1)
#else
	CvAStarNode* pNode;
	int iNumSteps;

	// Generate step path
	iNumSteps = GC.getStepFinder().GetStepDistanceBetweenPoints(ePlayer, eEnemy, pStartPlot, pEndPlot);
	if(iNumSteps != -1)
#endif
	{
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
		CvAStarNode* pNode = GetLastNode();
#else
		pNode = GC.getStepFinder().GetLastNode();
#endif

		// Starting at the end, loop until we find a plot from this owner
#ifndef AUI_ASTAR_CACHE_PLOTS_AT_NODES
		CvMap& kMap = GC.getMap();
#endif
		while(pNode != NULL)
		{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
			CvPlot* currentPlot = pNode->m_pPlot;
#else
			CvPlot* currentPlot;
			currentPlot = pNode->m_pPlot;
#endif
#else
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
			CvPlot* currentPlot = kMap.plotUnchecked(pNode->m_iX, pNode->m_iY);
#else
			CvPlot* currentPlot;
			currentPlot = kMap.plotUnchecked(pNode->m_iX, pNode->m_iY);
#endif
#endif

			// Check and see if this plot has the right owner
			if(currentPlot->getOwner() == ePlayer)
			{
				return currentPlot;
			}

			// Move to the previous plot on the path
			pNode = pNode->m_pParent;
		}
	}

	return NULL;
}

//	--------------------------------------------------------------------------------
/// Get the plot X from the end of the step path
CvPlot* CvStepPathFinder::GetXPlotsFromEnd(PlayerTypes ePlayer, PlayerTypes eEnemy, CvPlot* pStartPlot, CvPlot* pEndPlot, int iPlotsFromEnd, bool bLeaveEnemyTerritory) const
{
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	CvAStarNode* pNode;
#endif
	CvPlot* currentPlot = NULL;
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	// Generate step path
	int iPathLen = GC.getStepFinder().GetStepDistanceBetweenPoints(ePlayer, eEnemy, pStartPlot, pEndPlot);
#ifdef AUI_FAST_COMP
	int iNumSteps = FASTMIN(iPlotsFromEnd, iPathLen);
#else
	int iNumSteps = ::min(iPlotsFromEnd, iPathLen);
#endif
#else
	int iNumSteps;
	int iPathLen;

	// Generate step path
	iPathLen = GC.getStepFinder().GetStepDistanceBetweenPoints(ePlayer, eEnemy, pStartPlot, pEndPlot);
#ifdef AUI_FAST_COMP
	iNumSteps = FASTMIN(iPlotsFromEnd, iPathLen);
#else
	iNumSteps = ::min(iPlotsFromEnd, iPathLen);
#endif
#endif

	if(iNumSteps != -1)
	{
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
		CvAStarNode* pNode = GetLastNode();
#else
		pNode = GC.getStepFinder().GetLastNode();
#endif

		if(pNode != NULL)
		{
			// Starting at the end, loop the correct number of times back
			for(int i = 0; i < iNumSteps; i++)
			{
				if(pNode->m_pParent != NULL)
				{
					// Move to the previous plot on the path
					pNode = pNode->m_pParent;
				}
			}

#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
			currentPlot = pNode->m_pPlot;
#else
			CvMap& kMap = GC.getMap();
			currentPlot = kMap.plotUnchecked(pNode->m_iX, pNode->m_iY);
#endif

			// Was an enemy specified and we don't want this plot to be in enemy territory?
			if (eEnemy != NO_PLAYER && bLeaveEnemyTerritory)
			{
				// Loop until we leave enemy territory
				for (int i = 0; i < (iPathLen - iNumSteps) && currentPlot->getOwner() == eEnemy; i++)
				{
					if (pNode->m_pParent != NULL)
					{
						// Move to the previous plot on the path
						pNode = pNode->m_pParent;
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
						currentPlot = pNode->m_pPlot;
#else
						currentPlot = kMap.plotUnchecked(pNode->m_iX, pNode->m_iY);
#endif
					}
					else
					{
						break;
					}
				}
			}
		}
	}

	return currentPlot;
}

//	--------------------------------------------------------------------------------
/// Check for existence of step path between two points
#ifdef AUI_ASTAR_TURN_LIMITER
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
bool CvIgnoreUnitsPathFinder::DoesPathExist(const CvUnit* pUnit, const CvPlot* pStartPlot, const CvPlot* pEndPlot, const int iMaxTurns)
#else
bool CvIgnoreUnitsPathFinder::DoesPathExist(CvUnit& unit, CvPlot* pStartPlot, CvPlot* pEndPlot, const int iMaxTurns)
#endif
#else
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
bool CvIgnoreUnitsPathFinder::DoesPathExist(const CvUnit* pUnit, const CvPlot* pStartPlot, const CvPlot* pEndPlot)
#else
bool CvIgnoreUnitsPathFinder::DoesPathExist(CvUnit& unit, CvPlot* pStartPlot, CvPlot* pEndPlot)
#endif
#endif
{
	m_pCurNode = NULL;

	if(pStartPlot == NULL || pEndPlot == NULL)
	{
		return false;
	}

#ifdef AUI_ASTAR_TURN_LIMITER
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	SetData(pUnit, iMaxTurns);
#else
	SetData(&unit, iMaxTurns);
#endif
#else
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	SetData(pUnit);
#else
	SetData(&unit);
#endif
#endif
	return GeneratePath(pStartPlot->getX(), pStartPlot->getY(), pEndPlot->getX(), pEndPlot->getY(), 0, true /*bReuse*/);
}

//	--------------------------------------------------------------------------------
/// Get length of last path computed by path finder in turns [should be used after a call to DoesPathExist()]
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
int CvIgnoreUnitsPathFinder::GetPathLength() const
{
	CvAStarNode* pNode = GetLastNode();
	if (pNode != NULL)
	{
		return pNode->m_iData2;
	}

	return MAX_INT;
#else
int CvIgnoreUnitsPathFinder::GetPathLength()
{
	int iPathDistance = MAX_INT;

	CvAStarNode* pNode = GetLastNode();
	if(pNode != NULL)
	{
		iPathDistance = pNode->m_iData2;
	}

	return iPathDistance;
#endif
}

//	--------------------------------------------------------------------------------
/// Returns the last plot along the step path owned by a specific player
CvPlot* CvIgnoreUnitsPathFinder::GetLastOwnedPlot(CvPlot* pStartPlot, CvPlot* pEndPlot, PlayerTypes iOwner) const
{
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	// Generate path
	if (GC.getIgnoreUnitsPathFinder().GeneratePath(pStartPlot->getX(), pStartPlot->getX(), pEndPlot->getX(), pEndPlot->getX(), 0, false))
	{
		CvAStarNode* pNode = GC.getIgnoreUnitsPathFinder().GetLastNode();
#else
	CvAStarNode* pNode;

	// Generate path
	if(GC.getIgnoreUnitsPathFinder().GeneratePath(pStartPlot->getX(), pStartPlot->getX(), pEndPlot->getX(), pEndPlot->getX(), 0, false))
	{
		pNode = GC.getIgnoreUnitsPathFinder().GetLastNode();
#endif

		// Starting at the end, loop until we find a plot from this owner
#ifndef AUI_ASTAR_CACHE_PLOTS_AT_NODES
		CvMap& kMap = GC.getMap();
#endif
		while(pNode != NULL)
		{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
			CvPlot* currentPlot = pNode->m_pPlot;
#else
			CvPlot* currentPlot;
			currentPlot = pNode->m_pPlot;
#endif
#else
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
			CvPlot* currentPlot = kMap.plotUnchecked(pNode->m_iX, pNode->m_iY);
#else
			CvPlot* currentPlot;
			currentPlot = kMap.plotUnchecked(pNode->m_iX, pNode->m_iY);
#endif
#endif

			// Check and see if this plot has the right owner
			if(currentPlot->getOwner() == iOwner)
			{
				return currentPlot;
			}

			// Move to the previous plot on the path
			pNode = pNode->m_pParent;
		}
	}

	return NULL;
}

//	--------------------------------------------------------------------------------
/// Retrieve first node of path
CvPlot* CvIgnoreUnitsPathFinder::GetPathFirstPlot() const
{
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvAStarNode* pNode = GetLastNode();
#else
	CvAStarNode* pNode;

	pNode = GC.getIgnoreUnitsPathFinder().GetLastNode();
#endif

#ifndef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	CvMap& kMap = GC.getMap();
#endif
	if(pNode->m_pParent == NULL)
	{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
		return pNode->m_pPlot;
#else
		return kMap.plotUnchecked(pNode->m_iX, pNode->m_iY);
#endif
	}

	while(pNode != NULL)
	{
		if(pNode->m_pParent->m_pParent == NULL)
		{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
			return pNode->m_pPlot;
#else
			return kMap.plotUnchecked(pNode->m_iX, pNode->m_iY);
#endif
		}

		pNode = pNode->m_pParent;
	}

	FAssert(false);

	return NULL;
}

//	--------------------------------------------------------------------------------
/// Return the furthest plot we can get to this turn that is on the path
CvPlot* CvIgnoreUnitsPathFinder::GetPathEndTurnPlot() const
{
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvAStarNode* pNode = GetLastNode();
#else
	CvAStarNode* pNode;

	pNode = GC.getIgnoreUnitsPathFinder().GetLastNode();
#endif

	if(NULL != pNode)
	{
#ifndef AUI_ASTAR_CACHE_PLOTS_AT_NODES
		CvMap& kMap = GC.getMap();
#endif
		if((pNode->m_pParent == NULL) || (pNode->m_iData2 == 1))
		{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
			return pNode->m_pPlot;
#else
			return kMap.plotUnchecked(pNode->m_iX, pNode->m_iY);
#endif
		}

		while(pNode->m_pParent != NULL)
		{
			if(pNode->m_pParent->m_iData2 == 1)
			{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
				return pNode->m_pParent->m_pPlot;
#else
				return kMap.plotUnchecked(pNode->m_pParent->m_iX, pNode->m_pParent->m_iY);
#endif
			}

			pNode = pNode->m_pParent;
		}
	}

	FAssert(false);

	return NULL;
}

//	--------------------------------------------------------------------------------
/// Get final plot on path [should be used after a call to DoesPathExist()]
CvPlot* CvIgnoreUnitsPathFinder::GetLastPlot()
{
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	CvAStarNode* pNode = GetLastNode();
	if (pNode != NULL)
	{
		// Save off node for future calls to GetPreviousPlot()
		m_pCurNode = pNode;

#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
		return pNode->m_pPlot;
#else
		return GC.getMap().plot(pNode->m_iX, pNode->m_iY);
#endif
	}

	return NULL;
#else
	CvPlot* pPlot = NULL;

	CvAStarNode* pNode = GC.getIgnoreUnitsPathFinder().GetLastNode();
	if(pNode != NULL)
	{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
		return pNode->m_pPlot;
#else
		pPlot = GC.getMap().plot(pNode->m_iX, pNode->m_iY);
#endif

		// Save off node for future calls to GetPreviousPlot()
		m_pCurNode = pNode;
	}

	return pPlot;
#endif
}

//	--------------------------------------------------------------------------------
/// Get final plot on path [should be used after a call to DoesPathExist()]
CvPlot* CvIgnoreUnitsPathFinder::GetPreviousPlot()
{
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pPlot = NULL;
#endif

	if(m_pCurNode != NULL)
	{
		m_pCurNode = m_pCurNode->m_pParent;

		if(m_pCurNode != NULL)
		{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
			return m_pCurNode->m_pPlot;
#else
			pPlot = m_pCurNode->m_pPlot;
#endif
#else
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
			return GC.getMap().plot(m_pCurNode->m_iX, m_pCurNode->m_iY);
#else
			pPlot = GC.getMap().plot(m_pCurNode->m_iX, m_pCurNode->m_iY);
#endif
#endif
		}
	}

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	return NULL;
#else
	return pPlot;
#endif
}

//	--------------------------------------------------------------------------------
/// UI path finder - check validity of a coordinate
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::UIPathValid(CvAStarNode* parent, CvAStarNode* node, int data)
#else
int UIPathValid(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
	if (parent == NULL)
	{
		return TRUE;
	}

	if(node->m_iData2 > 3)
	{
		return FALSE;
	}

#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	CvPlot* pToPlot = node->m_pPlot;
#else
	CvPlot* pToPlot = GC.getMap().plotUnchecked(node->m_iX, node->m_iY);
#endif

#ifdef AUI_ASTAR_USE_DELEGATES
	CvUnit* pUnit = ((CvUnit*)m_pData);
#else
	CvUnit* pUnit = ((CvUnit*)pointer);
#endif

	if(!pToPlot->isRevealed(pUnit->getTeam()))
	{
		if(pUnit->getNoRevealMapCount() > 0)
		{
			return FALSE;
		}
	}

#ifndef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
	if(pToPlot->isVisible(pUnit->getTeam()) && pToPlot->isVisibleEnemyUnit(pUnit))
	{
		if (!pUnit->canMoveInto(*pToPlot, CvUnit::MOVEFLAG_ATTACK))
			return FALSE;
	}
#endif

	if(pUnit->getDomainType() == DOMAIN_LAND)
	{
		int iGroupAreaID = pUnit->getArea();
		if(pToPlot->getArea() != iGroupAreaID)
		{
			if(!(pToPlot->isAdjacentToArea(iGroupAreaID)))
			{
				// antonjs: Added for Smoky Skies scenario. Allows move range to show correctly for airships,
				// which move over land and sea plots equally (canMoveAllTerrain)
				if (!pUnit->canMoveAllTerrain())
				{
					return FALSE;
				}
			}
		}
	}

	if(!pUnit->canEnterTerrain(*pToPlot, CvUnit::MOVEFLAG_ATTACK))
	{
		return FALSE;
	}

#ifdef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
	if (pToPlot->isVisible(pUnit->getTeam()) && pToPlot->isVisibleEnemyUnit(pUnit))
	{
		if (!pUnit->canMoveInto(*pToPlot, CvUnit::MOVEFLAG_ATTACK, true, true))
			return FALSE;
	}
#endif

#ifdef AUI_ASTAR_USE_DELEGATES
	if (!PathValid(parent, node, data))
#else
	if(!PathValid(parent,node,data,pointer,finder))
#endif
	{
		return FALSE;
	}

	return TRUE;
}

//	--------------------------------------------------------------------------------
/// UI path finder - add a new path and send out a message
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::UIPathAdd(CvAStarNode* parent, CvAStarNode* node, int data)
{
	PathAdd(parent, node, data);
#else
int UIPathAdd(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
{
	PathAdd(parent, node, data, pointer, finder);
#endif
	if(node)
	{
		if(node->m_iData2 < 2 /*&& node->m_eCvAStarListType == NO_CVASTARLIST*/)
		{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
			CvPlot* pPlot = node->m_pPlot;
#else
			CvPlot* pPlot = GC.getMap().plot(node->m_iX, node->m_iY);
#endif
			if(pPlot)
			{
				auto_ptr<ICvPlot1> pDllPlot = GC.WrapPlotPointer(pPlot);
				GC.GetEngineUserInterface()->AddHexToUIRange(pDllPlot.get());
			}
		}
	}

	return 1;
}

//	--------------------------------------------------------------------------------
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::AttackPathAdd(CvAStarNode* parent, CvAStarNode* node, int data)
{
	PathAdd(parent, node, data);
#else
int AttackPathAdd(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
{
	PathAdd(parent, node, data, pointer, finder);
#endif
	if(node && node->m_iData2 < 2)
	{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
		CvPlot* pPlot = node->m_pPlot;
#else
		CvPlot* pPlot = GC.getMap().plot(node->m_iX, node->m_iY);
#endif

		auto_ptr<ICvUnit1> pDllUnit(GC.GetEngineUserInterface()->GetHeadSelectedUnit());
		CvUnit* pUnit = GC.UnwrapUnitPointer(pDllUnit.get());
		CvAssertMsg(pUnit, "pUnit should be a value");

		if(pUnit && pPlot)
		{
			if(pPlot->isVisible(pUnit->getTeam()) && (pPlot->isVisibleEnemyUnit(pUnit) || pPlot->isEnemyCity(*pUnit)))
			{
				auto_ptr<ICvPlot1> pDllPlot = GC.WrapPlotPointer(pPlot);
				GC.GetEngineUserInterface()->AddHexToUIRange(pDllPlot.get());
			}
		}
	}

	return 1;
}

//	--------------------------------------------------------------------------------
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::AttackPathDestEval(int iToX, int iToY, bool bOnlyFortified, bool bOnlyCity)
{
	CvUnit* pUnit = ((CvUnit*)m_pData);
#else
int AttackPathDestEval(int iToX, int iToY, const void* pointer, CvAStar* finder, bool bOnlyFortified, bool bOnlyCity)
{
	CvUnit* pUnit = ((CvUnit*)pointer);
#endif
	CvAssertMsg(pUnit, "pUnit should be a value");
	CvPlot* pPlot = GC.getMap().plot(iToX, iToY);
#ifdef AUI_ASTAR_USE_DELEGATES
	CvAStarNode* pNode = GetLastNode();
#else
	CvAStarNode* pNode = finder->GetLastNode();
#endif

	if(pPlot->isVisible(pUnit->getTeam()) && (pPlot->isVisibleEnemyUnit(pUnit) || pPlot->isEnemyCity(*pUnit)) && pNode && pNode->m_iData2 < 2)
	{
		if (pUnit->canMoveInto(*pPlot, CvUnit::MOVEFLAG_ATTACK))
		{
			if(bOnlyFortified)
			{
				CvUnit* pEnemyUnit = pPlot->getVisibleEnemyDefender(pUnit->getOwner());
				if(pEnemyUnit && pEnemyUnit->IsFortifiedThisTurn())
				{
					return TRUE;
				}
			}
			else if(bOnlyCity)
			{
				if(pPlot->isEnemyCity(*pUnit))
				{
					return TRUE;
				}
			}
			else
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

//	--------------------------------------------------------------------------------
/// Destination is valid if there is an enemy unit there
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::AttackPathDest(int iToX, int iToY)
{
	return AttackPathDestEval(iToX, iToY, false, false);
#else
int AttackPathDest(int iToX, int iToY, const void* pointer, CvAStar* finder)
{
	return AttackPathDestEval(iToX, iToY, pointer, finder, false, false);
#endif
}

//	--------------------------------------------------------------------------------
/// Destination is valid if there is a fortified unit there
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::AttackFortifiedPathDest(int iToX, int iToY)
{
	return AttackPathDestEval(iToX, iToY, true, false);
#else
int AttackFortifiedPathDest(int iToX, int iToY, const void* pointer, CvAStar* finder)
{
	return AttackPathDestEval(iToX, iToY, pointer, finder, true, false);
#endif
}

//	--------------------------------------------------------------------------------
/// Destination is valid if there is a city there
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::AttackCityPathDest(int iToX, int iToY)
{
	return AttackPathDestEval(iToX, iToY, false, true);
#else
int AttackCityPathDest(int iToX, int iToY, const void* pointer, CvAStar* finder)
{
	return AttackPathDestEval(iToX, iToY, pointer, finder, false, true);
#endif
}

//	---------------------------------------------------------------------------
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::TacticalAnalysisMapPathValid(CvAStarNode* parent, CvAStarNode* node, int data)
#else
int TacticalAnalysisMapPathValid(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	CvPlot* pToPlot = node->m_pPlot;
	if (!pToPlot)
		return FALSE;
#else
	CvMap& theMap = GC.getMap();

	CvPlot* pToPlot = theMap.plotUnchecked(node->m_iX, node->m_iY);
#endif
	PREFETCH_FASTAR_CVPLOT(reinterpret_cast<char*>(pToPlot));

#ifdef AUI_ASTAR_USE_DELEGATES
	CvUnit* pUnit = ((CvUnit *)m_pData);
	const UnitPathCacheData* pCacheData = reinterpret_cast<const UnitPathCacheData*>(GetScratchBuffer());
#else
	CvUnit* pUnit = ((CvUnit *)pointer);
	const UnitPathCacheData* pCacheData = reinterpret_cast<const UnitPathCacheData*>(finder->GetScratchBuffer());
#endif

	TeamTypes eUnitTeam = pCacheData->getTeam();
	PlayerTypes unit_owner = pCacheData->getOwner();
	CvAssertMsg(eUnitTeam != NO_TEAM, "The unit's team should be a vaild value");
	if (eUnitTeam == NO_TEAM)
	{
		eUnitTeam = GC.getGame().getActiveTeam();
	}

	CvTeam& kUnitTeam = GET_TEAM(eUnitTeam);

	CvTacticalAnalysisMap* pTAMap = GC.getGame().GetTacticalAnalysisMap();
	FAssert(pTAMap != NULL);
	CvTacticalAnalysisCell* pToPlotCell = pTAMap->GetCell(pToPlot->GetPlotIndex());
	FAssert(pToPlotCell != NULL);

#ifdef AUI_ASTAR_FIX_PARENT_NODE_ALWAYS_VALID_OPTIMIZATION
	// If this is the first node in the path, it is always valid (starting location)
	if (parent == NULL)
	{
#ifdef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
		// Cache values for this node that we will use in the loop
		CvPathNodeCacheData& kToNodeCacheData = node->m_kCostCacheData;
		if (!kToNodeCacheData.bIsCalculated)
		{
			kToNodeCacheData.bIsCalculated = true;
			kToNodeCacheData.bPlotVisibleToTeam = true;
			kToNodeCacheData.iNumFriendlyUnitsOfType = pToPlot->getNumFriendlyUnitsOfType(pUnit);
			kToNodeCacheData.bIsMountain = pToPlot->isMountain();
			kToNodeCacheData.bIsWater = pToPlotCell->IsWater();
			kToNodeCacheData.bCanEnterTerrain = true;
			kToNodeCacheData.bIsRevealedToTeam = true;
			kToNodeCacheData.bContainsOtherFriendlyTeamCity = false;
			CvCity* pCity = pToPlot->getPlotCity();
			if (pCity)
			{
				if (unit_owner != pCity->getOwner() && !kUnitTeam.isAtWar(pCity->getTeam()))
					kToNodeCacheData.bContainsOtherFriendlyTeamCity = true;
			}
			kToNodeCacheData.bContainsEnemyCity = pToPlot->isEnemyCity(*pUnit);
			kToNodeCacheData.bContainsVisibleEnemy = pToPlotCell->GetEnemyMilitaryUnit() != NULL;
			kToNodeCacheData.bContainsVisibleEnemyDefender = pToPlot->getBestDefender(NO_PLAYER, unit_owner, pUnit).pointer() != NULL;
#ifdef AUI_DANGER_PLOTS_REMADE
			if (pCacheData->DoDanger())
				kToNodeCacheData.iPlotDanger = GET_PLAYER(unit_owner).GetPlotDanger(*pToPlot, pUnit);
#endif
		}
#endif
		return TRUE;
	}
#endif

	// Cache the data for the node
	CvPathNodeCacheData& kToNodeCacheData = node->m_kCostCacheData;
#ifdef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
	if (!kToNodeCacheData.bIsCalculated)
	{
		kToNodeCacheData.bPlotVisibleToTeam = pToPlotCell->IsVisible();
		kToNodeCacheData.iNumFriendlyUnitsOfType = pToPlot->getNumFriendlyUnitsOfType(pUnit);
		kToNodeCacheData.bIsMountain = pToPlot->isMountain();
		kToNodeCacheData.bIsWater = pToPlotCell->IsWater();
		kToNodeCacheData.bIsRevealedToTeam = pToPlotCell->IsRevealed();
		kToNodeCacheData.bContainsOtherFriendlyTeamCity = false;
		if (pToPlotCell->IsCity())
		{
			CvCity* pCity = pToPlot->getPlotCity();
			if (pCity)
			{
				if (unit_owner != pCity->getOwner() && !kUnitTeam.isAtWar(pCity->getTeam()))
					kToNodeCacheData.bContainsOtherFriendlyTeamCity = true;
			}
		}
		kToNodeCacheData.bContainsEnemyCity = pToPlotCell->IsEnemyCity();
		kToNodeCacheData.bContainsVisibleEnemy = pToPlotCell->GetEnemyMilitaryUnit() != NULL;
		kToNodeCacheData.bContainsVisibleEnemyDefender = pToPlot->getBestDefender(NO_PLAYER, unit_owner, pUnit).pointer() != NULL;
	}
#else
	kToNodeCacheData.bPlotVisibleToTeam = pToPlotCell->IsVisible();
	kToNodeCacheData.iNumFriendlyUnitsOfType = pToPlot->getNumFriendlyUnitsOfType(pUnit);
	kToNodeCacheData.bIsMountain = pToPlot->isMountain();
	kToNodeCacheData.bIsWater = pToPlotCell->IsWater();
	kToNodeCacheData.bCanEnterTerrain = pUnit->canEnterTerrain(*pToPlot, CvUnit::MOVEFLAG_PRETEND_CORRECT_EMBARK_STATE);
	kToNodeCacheData.bIsRevealedToTeam = pToPlotCell->IsRevealed();
	kToNodeCacheData.bContainsOtherFriendlyTeamCity = false;
	if(pToPlotCell->IsCity())
	{
		CvCity* pCity = pToPlot->getPlotCity();
		if(pCity)
		{
			if(unit_owner != pCity->getOwner() && !kUnitTeam.isAtWar(pCity->getTeam()))
				kToNodeCacheData.bContainsOtherFriendlyTeamCity = true;
		}
	}
	kToNodeCacheData.bContainsEnemyCity = pToPlotCell->IsEnemyCity();
	kToNodeCacheData.bContainsVisibleEnemy = pToPlotCell->GetEnemyMilitaryUnit() != NULL;
	kToNodeCacheData.bContainsVisibleEnemyDefender = pToPlot->getBestDefender(NO_PLAYER, unit_owner, pUnit).pointer() != NULL;
#endif

#ifndef AUI_ASTAR_FIX_PARENT_NODE_ALWAYS_VALID_OPTIMIZATION
	// If this is the first node in the path, it is always valid (starting location)
	if (parent == NULL)
	{
		return TRUE;
	}
#endif

#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	CvPlot* pFromPlot = parent->m_pPlot;
	if (!pFromPlot)
		return FALSE;
#else
	CvPlot* pFromPlot = theMap.plotUnchecked(parent->m_iX, parent->m_iY);
#endif
	PREFETCH_FASTAR_CVPLOT(reinterpret_cast<char*>(pFromPlot));

	CvTacticalAnalysisCell* pFromPlotCell = pTAMap->GetCell(pFromPlot->GetPlotIndex());
	FAssert(pFromPlotCell != NULL);

	bool bAIControl = pUnit->IsAutomated();

	// pulling invariants out of the loop
	int iUnitX = pUnit->getX();
	int iUnitY = pUnit->getY();
	DomainTypes unit_domain_type = pCacheData->getDomainType();
	bool bUnitIsCombat = pUnit->IsCombatUnit();
	bool bIsHuman = pCacheData->isHuman();
#ifndef AUI_ASTAR_USE_DELEGATES
	int iFinderInfo = finder->GetInfo();
#endif
	CvPlot* pUnitPlot = pUnit->plot();
#ifdef AUI_ASTAR_USE_DELEGATES
	const int iFinderIgnoreStacking = GetInfo() & MOVE_IGNORE_STACKING;
#else
	int iFinderIgnoreStacking = iFinderInfo & MOVE_IGNORE_STACKING;
#endif
	int iUnitPlotLimit = GC.getPLOT_UNIT_LIMIT();
	bool bFromPlotOwned = !pFromPlotCell->IsUnclaimedTerritory();
	TeamTypes eFromPlotTeam = pFromPlot->getTeam();

#ifdef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
	if (!kToNodeCacheData.bIsCalculated)
	{
		if (bAIControl || !bIsHuman || kToNodeCacheData.bIsRevealedToTeam)
			kToNodeCacheData.bCanEnterTerrain = pUnit->canEnterTerrain(*pToPlot, CvUnit::MOVEFLAG_PRETEND_CORRECT_EMBARK_STATE);
		else
			kToNodeCacheData.bCanEnterTerrain = true;
#ifdef AUI_DANGER_PLOTS_REMADE
		if (pCacheData->DoDanger())
			kToNodeCacheData.iPlotDanger = GET_PLAYER(unit_owner).GetPlotDanger(*pToPlot, pUnit);
#endif
		kToNodeCacheData.bIsCalculated = true;
	}
#else
#ifdef AUI_DANGER_PLOTS_REMADE
	if (bAIControl || !bIsHuman)
		kToNodeCacheData.iPlotDanger = GET_PLAYER(unit_owner).GetPlotDanger(*pToPlot, pUnit);
#endif
#endif

	// We have determined that this node is not the origin above (parent == NULL)
	CvAStarNode* pNode = node;
	bool bPreviousNodeHostile = false;
#ifndef AUI_ASTAR_USE_DELEGATES
	int iDestX = finder->GetDestX();
	int iDestY = finder->GetDestY();
#endif
	int iNodeX = node->m_iX;
	int iNodeY = node->m_iY;
	int iOldNumTurns = -1;
#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	int iNumTurns;
#endif
	TeamTypes eTeam = eUnitTeam; // this may get modified later is eTEam == NO_TEAM

	// First run special case for checking "node" since it doesn't have a parent set yet
	bool bFirstRun = true;

	// Have to calculate this specially because the node passed into this function doesn't yet have data stored it in (hasn't reached pathAdd yet)
	int iStartMoves = parent->m_iData1;
#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	int iNumTurns = parent->m_iData2;
#else
	iNumTurns = parent->m_iData2;
#endif
#if defined(AUI_ASTAR_TURN_LIMITER) && !defined(AUI_ASTAR_USE_DELEGATES)
	int iMaxTurns = finder->GetMaxTurns();
#endif

	if(iStartMoves == 0)
	{
		iNumTurns++;
	}

	iOldNumTurns = -1;

#ifndef AUI_ASTAR_MINOR_OPTIMIZATION
	CvPlot* pPlot = NULL;
#endif

	// Get a reference to the parent node cache data
	CvPathNodeCacheData& kFromNodeCacheData = parent->m_kCostCacheData;

	// Loop through the current path until we find the path origin.
	// This validates the path with the inclusion of the new path node.  We must do this because of the rules of where a unit can finish a turn.
	// Please note that this can be an expensive loop as the path gets longer and longer, do as little work as possible in validating each node.  
	// If there is an invariant value that needs to be fetched from the plot or unit for the node, please do the calculation and put it in the node's data cache.
	while(pNode != NULL)
	{
#ifdef AUI_ASTAR_TURN_LIMITER
#ifdef AUI_ASTAR_USE_DELEGATES
		if (iNumTurns > GetMaxTurns())
#else
		if (iNumTurns > iMaxTurns)
#endif
		{
			return FALSE; // Path is too long, terminate now
		}
#endif

		PREFETCH_FASTAR_NODE(pNode->m_pParent);

		CvPathNodeCacheData& kNodeCacheData = pNode->m_kCostCacheData;
		// This is a safeguard against the algorithm believing a plot to be impassable before actually knowing it (mid-search)
#ifdef AUI_ASTAR_USE_DELEGATES
		if (iOldNumTurns != -1 || (GetDestX() == iNodeX && GetDestY() == iNodeY))
#else
		if(iOldNumTurns != -1 || (iDestX == iNodeX && iDestY == iNodeY))
#endif
		{
#ifdef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
			if (!kNodeCacheData.bCanEnterTerrain)	// since this gets cached for each node anyway during buildup, it should be used whereever possible
			{
				return FALSE;
			}
#endif
			// This plot is of greater distance than previously, so we know the unit is ending its turn here (pNode), or it's trying to attack through a unit (and might end up on this tile if an attack fails to kill the enemy)
			if(iNumTurns != iOldNumTurns || bPreviousNodeHostile)
			{
				// Don't count origin, or else a unit will block its own movement!
				if(iNodeX != iUnitX || iNodeY != iUnitY)
				{
					// PREFETCH_FASTAR_CVPLOT(reinterpret_cast<char*>(pPlot));
					if(kNodeCacheData.bPlotVisibleToTeam)
					{
						// Check to see if any units are present at this full-turn move plot... if the player can see what's there
						if(kNodeCacheData.iNumFriendlyUnitsOfType >= iUnitPlotLimit && !(iFinderIgnoreStacking))
						{
							return FALSE;
						}

#ifndef AUI_ASTAR_FIX_PATH_VALID_PATH_PEAKS_FOR_NONHUMAN
						if (kNodeCacheData.bIsMountain && !(iFinderIgnoreStacking) && (!bIsHuman || bAIControl))
						{
							return FALSE;
						}
#endif

#ifndef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
						if(kNodeCacheData.bIsMountain && !kNodeCacheData.bCanEnterTerrain)	// only doing canEnterTerrain on mountain plots because it is expensive, though it probably should always be called and some other checks in this loop could be removed.
						{
							return FALSE;
						}
#endif

#ifdef AUI_ASTAR_USE_DELEGATES
						if ((GetInfo() & CvUnit::MOVEFLAG_STAY_ON_LAND) && kNodeCacheData.bIsWater)
#else
						if ((finder->GetInfo() & CvUnit::MOVEFLAG_STAY_ON_LAND) && kNodeCacheData.bIsWater)
#endif
						{
							return FALSE;
						}
					}

					if(kNodeCacheData.bIsRevealedToTeam)
					{
						if (kNodeCacheData.bContainsOtherFriendlyTeamCity && !(iFinderIgnoreStacking))
							return FALSE;
					}
				}
			}
		}

		bPreviousNodeHostile = false;
		if(kNodeCacheData.bContainsEnemyCity)
		{
			bPreviousNodeHostile = true;
		}
		// Prevents units from passing through one another on its way to attack another unit
		else if(kNodeCacheData.bContainsVisibleEnemy)
		{
			// except when attacking an unguarded civilian unit
			if(kNodeCacheData.bContainsVisibleEnemyDefender)
			{
				bPreviousNodeHostile = true;
			}
		}

		// JON - Special case for the original node passed into this function because it's not yet linked to any parent
		if(pNode == node && bFirstRun)
		{
			pNode = parent;
			bFirstRun = false;
		}
		else
		{
			pNode = pNode->m_pParent;
		}

		if(pNode != NULL)
		{

			iNodeX = pNode->m_iX;
			iNodeY = pNode->m_iY;
			iOldNumTurns = iNumTurns;
			iNumTurns = pNode->m_iData2;
		}
	}

	// slewis - moved this up so units can't move directly into the water. Not 100% sure this is the right solution.
	if(unit_domain_type == DOMAIN_LAND)
	{
		if(!kFromNodeCacheData.bIsWater && kToNodeCacheData.bIsWater && kToNodeCacheData.bIsRevealedToTeam && !pUnit->canEmbarkOnto(*pFromPlot, *pToPlot, true))
		{
			if(!pUnit->IsHoveringUnit() && !pUnit->canMoveAllTerrain() && !pToPlot->IsAllowsWalkWater())
			{
				return FALSE;
			}
		}
	}

#ifndef AUI_ASTAR_FIX_RADAR
	if(!bUnitIsCombat && unit_domain_type != DOMAIN_AIR)
	{
		const PlayerTypes eUnitPlayer = unit_owner;
		const int iUnitCount = pToPlot->getNumUnits();
		for(int iUnit = 0; iUnit < iUnitCount; ++iUnit)
		{
			const CvUnit* pToPlotUnit = pToPlot->getUnitByIndex(iUnit);
			if(pToPlotUnit != NULL && pToPlotUnit->getOwner() != eUnitPlayer)
			{
				return FALSE; // Plot occupied by another player
			}
		}
	}
#endif

	// slewis - Added to catch when the unit is adjacent to an enemy unit while it is stacked with a friendly unit.
	//          The logic above (with bPreviousNodeHostile) catches this problem with a path that's longer than one step
	//          but does not catch when the path is only one step.
#ifdef AUI_ASTAR_FIX_RADAR
	if (unit_domain_type != DOMAIN_AIR && pUnitPlot->isAdjacent(pToPlot) && kToNodeCacheData.bContainsVisibleEnemy && !(iFinderIgnoreStacking))
#else
	if(bUnitIsCombat && unit_domain_type != DOMAIN_AIR && pUnitPlot->isAdjacent(pToPlot) && kToNodeCacheData.bContainsVisibleEnemy && !(iFinderIgnoreStacking))
#endif
	{
		if(kToNodeCacheData.bContainsVisibleEnemyDefender)
		{
			if(pUnitPlot->getNumFriendlyUnitsOfType(pUnit) > iUnitPlotLimit)
			{
				return FALSE;
			}
		}
	}

	if(pUnitPlot == pFromPlot)
	{
		return TRUE;
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	if (GetInfo() & MOVE_TERRITORY_NO_UNEXPLORED)
#else
	if(iFinderInfo & MOVE_TERRITORY_NO_UNEXPLORED)
#endif
	{
		if(!kFromNodeCacheData.bIsRevealedToTeam)
		{
			return FALSE;
		}

		if(bFromPlotOwned)
		{
			if(eFromPlotTeam != eUnitTeam)
			{
				return FALSE;
			}
		}
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	if (GetInfo() & MOVE_TERRITORY_NO_ENEMY)
#else
	if(iFinderInfo & MOVE_TERRITORY_NO_ENEMY)
#endif
	{
		if(bFromPlotOwned)
		{
			if(atWar(eFromPlotTeam, eUnitTeam))
			{
				return FALSE;
			}
		}
	}

#ifdef AUI_DANGER_PLOTS_REMADE
	if (pCacheData->DoDanger())
#else
	if(bAIControl)
#endif
	{
		if((parent->m_iData2 > 1) || (parent->m_iData1 == 0))
		{
#ifdef AUI_DANGER_PLOTS_REMADE
			{
#else
#ifdef AUI_ASTAR_USE_DELEGATES
			if(!(GetInfo() & MOVE_UNITS_IGNORE_DANGER))
#else
			if(!(iFinderInfo & MOVE_UNITS_IGNORE_DANGER))
#endif
			{
				if(!bUnitIsCombat || pUnit->getArmyID() == FFreeList::INVALID_INDEX)
#endif
				{
#ifdef AUI_DANGER_PLOTS_REMADE
					if (kToNodeCacheData.iPlotDanger == MAX_INT || (kToNodeCacheData.iPlotDanger >= pUnit->GetCurrHitPoints() && kFromNodeCacheData.iPlotDanger < pUnit->GetCurrHitPoints()))
#else
#ifdef AUI_ASTAR_FIX_CONSIDER_DANGER_USES_TO_PLOT_NOT_FROM_PLOT
#ifdef AUI_ASTAR_FIX_CONSIDER_DANGER_USES_COMBAT_STRENGTH
#ifdef AUI_ASTAR_FIX_CONSIDER_DANGER_ONLY_POSITIVE_DANGER_DELTA
					if (GET_PLAYER(unit_owner).GetPlotDanger(*pToPlot) > pUnit->GetBaseCombatStrengthConsideringDamage() * AUI_ASTAR_FIX_CONSIDER_DANGER_USES_COMBAT_STRENGTH &&
						GET_PLAYER(unit_owner).GetPlotDanger(*pFromPlot) <= pUnit->GetBaseCombatStrengthConsideringDamage() * AUI_ASTAR_FIX_CONSIDER_DANGER_USES_COMBAT_STRENGTH)
#else
					if (GET_PLAYER(unit_owner).GetPlotDanger(*pToPlot) > pUnit->GetBaseCombatStrengthConsideringDamage() * AUI_ASTAR_FIX_CONSIDER_DANGER_USES_COMBAT_STRENGTH)
#endif
#else
#ifdef AUI_ASTAR_FIX_CONSIDER_DANGER_ONLY_POSITIVE_DANGER_DELTA
					if (GET_PLAYER(unit_owner).GetPlotDanger(*pToPlot) > 0 && GET_PLAYER(pUnit->getOwner()).GetPlotDanger(*pFromPlot) <= 0)
#else
					if(GET_PLAYER(unit_owner).GetPlotDanger(*pToPlot) > 0)
#endif
#endif
#else
#ifdef AUI_ASTAR_FIX_CONSIDER_DANGER_USES_COMBAT_STRENGTH
					if (GET_PLAYER(unit_owner).GetPlotDanger(*pFromPlot) > pUnit->GetBaseCombatStrengthConsideringDamage() * AUI_ASTAR_FIX_CONSIDER_DANGER_USES_COMBAT_STRENGTH)
#else
					if(GET_PLAYER(unit_owner).GetPlotDanger(*pFromPlot) > 0)
#endif
#endif
#endif
					{
						return FALSE;
					}
				}
			}
		}
	}

	// slewis - added AI check and embark check to prevent units from moving into unexplored areas
#ifdef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
	if(bAIControl || !bIsHuman || kFromNodeCacheData.bIsRevealedToTeam)
#else
	if(bAIControl || !bIsHuman || kFromNodeCacheData.bIsRevealedToTeam || pCacheData->isEmbarked())
#endif
	{
#ifdef AUI_ASTAR_USE_DELEGATES
		if (GetInfo() & MOVE_UNITS_THROUGH_ENEMY)
#else
		if(iFinderInfo & MOVE_UNITS_THROUGH_ENEMY)
#endif
		{
#ifdef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
			if(!(pUnit->canMoveOrAttackInto(*pFromPlot, CvUnit::MOVEFLAG_PRETEND_CORRECT_EMBARK_STATE, kFromNodeCacheData.bCanEnterTerrain, true)))
#else
			if(!(pUnit->canMoveOrAttackInto(*pFromPlot, CvUnit::MOVEFLAG_PRETEND_CORRECT_EMBARK_STATE)))
#endif
			{
				return FALSE;
			}
		}
		else
		{
#ifdef AUI_ASTAR_FIX_CAN_ENTER_TERRAIN_NO_DUPLICATE_CALLS
			if (!(pUnit->canMoveThrough(*pFromPlot, CvUnit::MOVEFLAG_PRETEND_CORRECT_EMBARK_STATE, kFromNodeCacheData.bCanEnterTerrain, true)))
#else
			if(!(pUnit->canMoveThrough(*pFromPlot, CvUnit::MOVEFLAG_PRETEND_CORRECT_EMBARK_STATE)))
#endif
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

//	---------------------------------------------------------------------------
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::FindValidDestinationDest(int iToX, int iToY)
{
	CvUnit* pUnit = ((CvUnit*)m_pData);
#else
int FindValidDestinationDest(int iToX, int iToY, const void* pointer, CvAStar* finder)
{
	CvUnit* pUnit = ((CvUnit*)pointer);
#endif
	CvPlot* pToPlot = GC.getMap().plotUnchecked(iToX, iToY);

	if(pToPlot->getNumFriendlyUnitsOfType(pUnit) >= GC.getPLOT_UNIT_LIMIT())
	{
		return false;
	}

	if(pToPlot->getNumVisibleEnemyDefenders(pUnit) > 0)
	{
		return false;
	}

	// can't capture the unit with a non-combat unit
	if(!pUnit->IsCombatUnit() && pToPlot->isVisibleEnemyUnit(pUnit))
	{
		return false;
	}

	return true;
}

//	--------------------------------------------------------------------------------
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::FindValidDestinationPathValid(CvAStarNode* parent, CvAStarNode* node, int data)
{
	CvUnit* pUnit = ((CvUnit*)m_pData);
#else
int FindValidDestinationPathValid(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
{
	CvUnit* pUnit = ((CvUnit*)pointer);
#endif
#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	const CvPlot* pToPlot = node->m_pPlot;
	if (!pToPlot)
		return FALSE;
#else
	CvPlot* pToPlot = GC.getMap().plotUnchecked(node->m_iX, node->m_iY);
#endif

#ifdef AUI_ASTAR_FIX_FASTER_CHECKS
	if (node->m_iData2 > 3)
	{
		return FALSE;
	}
#endif

	if(!pUnit->canEnterTerrain(*pToPlot, CvUnit::MOVEFLAG_PRETEND_CORRECT_EMBARK_STATE))
	{
		return FALSE;
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	if (!PathValid(parent, node, data))
#else
	if(!PathValid(parent,node,data,pointer,finder))
#endif
	{
		return FALSE;
	}

#ifndef AUI_ASTAR_FIX_FASTER_CHECKS
	if(node->m_iData2 > 3)
	{
		return FALSE;
	}
#endif

	return TRUE;
}

#ifdef AUI_ASTAR_TWEAKED_OPTIMIZED_BUT_CAN_STILL_USE_ROADS
// If there is a valid road within the unit's base movement range, multiply range by movement modifier of best road type
// Check is fairly fast and is good enough for most cases.
void IncreaseMoveRangeForRoads(const CvUnit* pUnit, int& iRange)
{
	// Filtering out units that don't need road optimization
	if (pUnit->getDomainType() != DOMAIN_LAND || pUnit->flatMovementCost())
	{
		return;
	}

#ifdef AUI_ASTAR_TURN_LIMITER
	// With the turn limiter, we no longer need to worry as much about A* loops getting out of hand when calculating paths to a tile 20 turns away
	iRange *= GET_TEAM(pUnit->getTeam()).GetBestRoadMovementMultiplier(pUnit);
#else
	// Don't want to call this on each loop, so we'll call it once out of loop and be done with it
	const bool bIsIroquois = GET_PLAYER(pUnit->getOwner()).GetPlayerTraits()->IsMoveFriendlyWoodsAsRoad();
	CvPlot* pLoopPlot;
	FeatureTypes eFeature;
	for (int iDY = -iRange; iDY <= iRange; iDY++)
	{
#ifdef AUI_FAST_COMP
		int iMaxDX = iRange - FASTMAX(0, iDY);
		for (int iDX = -iRange - FASTMIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#else
		int iMaxDX = iRange - MAX(0, iDY);
		for (int iDX = -iRange - MIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#endif
		{
			pLoopPlot = plotXY(pUnit->getX(), pUnit->getY(), iDX, iDY);
			if (pLoopPlot)
			{
				eFeature = pLoopPlot->getFeatureType();
				if (bIsIroquois && pUnit->getOwner() == pLoopPlot->getOwner() && (eFeature == FEATURE_FOREST || eFeature == FEATURE_JUNGLE))
				{
					iRange *= GET_TEAM(pUnit->getTeam()).GetBestRoadMovementMultiplier(pUnit);
					return;
				}
				else if (pLoopPlot->isValidRoute(pUnit.pointer()))
				{
					CvPlot* pEvalPlot;
					// Check for neighboring roads that would make this road usable
					for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
					{
						pEvalPlot = plotDirection(pLoopPlot->getX(), pLoopPlot->getY(), (DirectionTypes)iI);
						if (pEvalPlot && pEvalPlot->isValidRoute(pUnit.pointer()))
						{
							iRange *= GET_TEAM(pUnit->getTeam()).GetBestRoadMovementMultiplier(pUnit);
							return;
						}
					}
				}
			}
		}
	}
#endif
}

// AdjustDistanceFilterForRoads() call for when the distance isn't being stored
int GetIncreasedMoveRangeForRoads(const CvUnit* pUnit, int iRange)
{
	IncreaseMoveRangeForRoads(pUnit, iRange);
	return iRange;
}
#endif

//	--------------------------------------------------------------------------------
/// Can a unit reach this destination in "X" turns of movement (pass in 0 if need to make it in 1 turn with movement left)?
// ***
// *** WARNING - The optimization below (so that TurnsToReachTarget() doesn't get called too often breaks down when we get to RR.  We need to address this!
// ***
#ifdef AUI_ASTAR_PARADROP
bool CanReachInXTurns(UnitHandle pUnit, const CvPlot* pTarget, int iTurns, bool bIgnoreUnits, bool bIgnoreParadrop, int* piTurns /* = NULL */)
#else
bool CanReachInXTurns(UnitHandle pUnit, CvPlot* pTarget, int iTurns, bool bIgnoreUnits, int* piTurns /* = NULL */)
#endif
{
	int iDistance;

	if(!pTarget)
	{
		return false;
	}

#ifdef AUI_ASTAR_PARADROP
	int iTurnsAwayAfterParadrop = MAX_INT;
	if (pUnit->getDropRange() > 0 && (!bIgnoreParadrop || pUnit->isBlitz() || pUnit->getNumAttacks() > 1))
	{
		if (pUnit->canParadropAt(pUnit->plot(), pTarget->getX(), pTarget->getY(), bIgnoreUnits))
		{
			if (piTurns)
				*piTurns = 0;
			return (0 <= iTurns);
		}
		else
		{
			int iTempTurns = MAX_INT;
			CvPlot* pLoopPlot;			
			int iRange = FASTMIN((pUnit->getMoves() + GC.getMOVE_DENOMINATOR() - 1 - GC.getMOVE_DENOMINATOR() / 2) / GC.getMOVE_DENOMINATOR(), pUnit->baseMoves());
			if (iTurns > 1)
				iRange += pUnit->baseMoves() * (iTurns - 1);
#ifdef AUI_ASTAR_TWEAKED_OPTIMIZED_BUT_CAN_STILL_USE_ROADS
			IncreaseMoveRangeForRoads(pUnit.pointer(), iRange);
#endif
			for (int iDY = -iRange; iDY <= iRange; iDY++)
			{
#ifdef AUI_FAST_COMP
				int iMaxDX = iRange - FASTMAX(0, iDY);
				for (int iDX = -iRange - FASTMIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#else
				iMaxDX = iRange - MAX(0, iDY);
				for (int iDX = -iRange - MIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#endif
				{

					pLoopPlot = plotXY(pTarget->getX(), pTarget->getY(), iDX, iDY);
					if (pLoopPlot && pUnit->canParadropAt(pUnit->plot(), pLoopPlot->getX(), pLoopPlot->getY(), bIgnoreUnits))
					{
						iTempTurns = TurnsToReachTargetFromPlot(pUnit, pTarget, pLoopPlot, true /*bReusePaths*/, bIgnoreUnits, false, iTurnsAwayAfterParadrop);
						if (iTempTurns < iTurnsAwayAfterParadrop)
						{
							iTurnsAwayAfterParadrop = iTempTurns;
							if (iTurnsAwayAfterParadrop == 0)
							{
								if (piTurns)
									*piTurns = 0;
								return (0 <= iTurns);
							}
						}
					}
				}
			}
		}
	}
#endif

	// Compare distance to movement rate
	iDistance = plotDistance(pUnit->getX(), pUnit->getY(), pTarget->getX(), pTarget->getY());
	// KWG: If the unit is a land unit that can embark, baseMoves() is only going to give correct value if the starting and ending locations
	//		are in the same domain (LAND vs. SEA) and no transition occurs.
#ifdef AUI_ASTAR_TWEAKED_OPTIMIZED_BUT_CAN_STILL_USE_ROADS
	int iBaseMoves = GetIncreasedMoveRangeForRoads(pUnit.pointer(), pUnit->baseMoves());
	if (iTurns == 0 && iDistance >= iBaseMoves)
	{
		return false;
	}

	else if(iTurns > 0 && iDistance > (iBaseMoves * iTurns))
#else
	if(iTurns == 0 && iDistance >= pUnit->baseMoves())
	{
		return false;
	}

	else if(iTurns > 0 && iDistance > (pUnit->baseMoves() * iTurns))
#endif
	{
#ifdef AUI_ASTAR_PARADROP
		int iTurnsCalculated = iTurnsAwayAfterParadrop;
		if (piTurns)
			*piTurns = iTurnsCalculated;
		return (iTurnsCalculated <= iTurns);
#else
		return false;
#endif
	}

	// Distance not too far, now use pathfinder
	else
	{
#ifdef AUI_ASTAR_TURN_LIMITER
		int iTurnsCalculated = TurnsToReachTarget(pUnit, pTarget, true /*bReusePaths*/, bIgnoreUnits, false, iTurns);
#else
		int iTurnsCalculated = TurnsToReachTarget(pUnit, pTarget, false /*bReusePaths*/, bIgnoreUnits);
#endif
#ifdef AUI_ASTAR_PARADROP
		if (iTurnsCalculated > iTurnsAwayAfterParadrop)
			iTurnsCalculated = iTurnsAwayAfterParadrop;
#endif
		if (piTurns)
			*piTurns = iTurnsCalculated;
		return (iTurnsCalculated <= iTurns);
	}
}

#ifdef AUI_ASTAR_TURN_LIMITER
// Delnar: if you're checking if a unit can reach a tile within X turns, set the iTargetTurns parameter to X to speed up the pathfinder
int TurnsToReachTarget(UnitHandle pUnit, const CvPlot* pTarget, bool bReusePaths, bool bIgnoreUnits, bool bIgnoreStacking, int iTargetTurns)
{
	return TurnsToReachTargetFromPlot(pUnit, pTarget, NULL, bReusePaths, bIgnoreUnits, bIgnoreStacking, iTargetTurns);
}
#endif

//	--------------------------------------------------------------------------------
/// How many turns will it take a unit to get to a target plot (returns MAX_INT if can't reach at all; returns 0 if makes it in 1 turn and has movement left)
// Should call it with bIgnoreStacking true if want foolproof way to see if can make it in 0 turns (since that way doesn't open
// open the 2nd layer of the pathfinder)
#ifdef AUI_ASTAR_TURN_LIMITER
// Delnar: if you're checking if a unit can reach a tile within X turns, set the iTargetTurns parameter to X to speed up the pathfinder
int TurnsToReachTargetFromPlot(UnitHandle pUnit, const CvPlot* pTarget, const CvPlot* pFromPlot, bool bReusePaths, bool bIgnoreUnits, bool bIgnoreStacking, int iTargetTurns)
#else
int TurnsToReachTarget(UnitHandle pUnit, CvPlot* pTarget, bool bReusePaths, bool bIgnoreUnits, bool bIgnoreStacking)
#endif
{
	int rtnValue = MAX_INT;
	CvAStarNode* pNode = NULL;

	if(pTarget == pUnit->plot())
	{
		return 0;
	}

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
	if (pUnit && pTarget)
	{
#else
	if(pUnit)
	{
#endif
#ifdef PATH_FINDER_LOGGING
		CvString strBaseString;
		cvStopWatch kTimer(strBaseString, "Pathfinder.csv");
#endif

#ifdef AUI_ASTAR_MINOR_OPTIMIZATION
		int iFlags = MOVE_UNITS_IGNORE_DANGER;
		if(bIgnoreStacking)
		{
			iFlags |= MOVE_IGNORE_STACKING;
		}

		CvAStar* pPathfinder = &GC.GetTacticalAnalysisMapFinder();
		if (bIgnoreUnits)
			pPathfinder = &GC.getIgnoreUnitsPathFinder();

#ifdef AUI_ASTAR_TURN_LIMITER
		if (!pFromPlot)
			pFromPlot = pUnit->plot();
		pPathfinder->SetData(pUnit.pointer(), iTargetTurns);
#else
		pPathfinder->SetData(pUnit.pointer());
#endif
		if (pPathfinder->GeneratePath(pFromPlot->getX(), pFromPlot->getY(), pTarget->getX(), pTarget->getY(), iFlags, bReusePaths))
		{
			pNode = pPathfinder->GetLastNode();
		}
#else
		if(bIgnoreUnits)
		{
#ifdef AUI_ASTAR_TURN_LIMITER
			if (!pFromPlot)
				pFromPlot = pUnit->plot();
			GC.getIgnoreUnitsPathFinder().SetData(pUnit.pointer(), iTargetTurns);
			if (GC.getIgnoreUnitsPathFinder().GeneratePath(pFromPlot->getX(), pFromPlot->getY(), pTarget->getX(), pTarget->getY(), 0, bReusePaths))
#else
			GC.getIgnoreUnitsPathFinder().SetData(pUnit.pointer());
			if (GC.getIgnoreUnitsPathFinder().GeneratePath(pUnit->getX(), pUnit->getY(), pTarget->getX(), pTarget->getY(), 0, bReusePaths))
#endif
			{
				pNode = GC.getIgnoreUnitsPathFinder().GetLastNode();
			}
		}
		else
		{
			int iFlags = MOVE_UNITS_IGNORE_DANGER;
			if(bIgnoreStacking)
			{
				iFlags |= MOVE_IGNORE_STACKING;
			}

			CvAssertMsg(pTarget != NULL, "Passed in a NULL destination to GeneratePath");
			if(pTarget == NULL)
			{
				return false;
			}

			bool bSuccess;

#ifdef AUI_ASTAR_TURN_LIMITER
			if (!pFromPlot)
				pFromPlot = pUnit->plot();
			GC.GetTacticalAnalysisMapFinder().SetData(pUnit.pointer(), iTargetTurns);
			bSuccess = GC.GetTacticalAnalysisMapFinder().GeneratePath(pFromPlot->getX(), pFromPlot->getY(), pTarget->getX(), pTarget->getY(), iFlags, bReusePaths);
#else
			GC.GetTacticalAnalysisMapFinder().SetData(pUnit.pointer());
			bSuccess = GC.GetTacticalAnalysisMapFinder().GeneratePath(pUnit->getX(), pUnit->getY(), pTarget->getX(), pTarget->getY(), iFlags, bReusePaths);
#endif
			if(bSuccess)
			{
				pNode = GC.GetTacticalAnalysisMapFinder().GetLastNode();
			}
		}
#endif

		if(pNode)
		{
			rtnValue = pNode->m_iData2;
			if(rtnValue == 1)
			{
				if(pNode->m_iData1 > 0)
				{
					rtnValue = 0;
				}
			}
		}

#ifdef PATH_FINDER_LOGGING
		// NOTE: because I'm creating the string after the cvStopWatch, the time it takes to create the string will be in the timer.
		strBaseString.Format("TurnsToReachTarget, Turn %03d, Player: %d, Unit: %d, From X: %d, Y: %d, To X: %d, Y: %d, reuse=%d, ignoreUnits=%d, ignoreStacking=%d, turns=%d", GC.getGame().getElapsedGameTurns(), (int)pUnit->getOwner(), pUnit->GetID(), pUnit->getX(), pUnit->getY(), pTarget->getX(), pTarget->getY(), bReusePaths?1:0, bIgnoreUnits?1:0, bIgnoreStacking?1:0, rtnValue);
		kTimer.SetText(strBaseString);
#endif
	}

	return rtnValue;
}

/// slewis's fault

// A structure holding some unit values that are invariant during a path plan operation
struct TradePathCacheData
{
	CvTeam* m_pTeam;
	bool m_bCanEmbarkAllWaterPassage;
	bool m_bIsRiverTradeRoad;
	bool m_bIsMoveFriendlyWoodsAsRoad;

	inline CvTeam& getTeam() const { return *m_pTeam; }
	inline bool CanEmbarkAllWaterPassage() const { return m_bCanEmbarkAllWaterPassage; }
	inline bool IsRiverTradeRoad() const { return m_bIsRiverTradeRoad; }
	inline bool IsMoveFriendlyWoodsAsRoad() const { return m_bIsMoveFriendlyWoodsAsRoad; }
};

//	--------------------------------------------------------------------------------
#ifdef AUI_ASTAR_USE_DELEGATES
void CvAStar::TradePathInitialize()
{
	const PlayerTypes ePlayer = (PlayerTypes)GetInfo();

	TradePathCacheData* pCacheData = reinterpret_cast<TradePathCacheData*>(GetScratchBuffer());
#else
void TradePathInitialize(const void* pointer, CvAStar* finder)
{
	PlayerTypes ePlayer = (PlayerTypes)finder->GetInfo();

	TradePathCacheData* pCacheData = reinterpret_cast<TradePathCacheData*>(finder->GetScratchBuffer());
#endif

	CvPlayer& kPlayer = GET_PLAYER(ePlayer);
	TeamTypes eTeam = kPlayer.getTeam();
	pCacheData->m_pTeam = &GET_TEAM(eTeam);
	pCacheData->m_bCanEmbarkAllWaterPassage = pCacheData->m_pTeam->canEmbarkAllWaterPassage();

	CvPlayerTraits* pPlayerTraits = kPlayer.GetPlayerTraits();
	if (pPlayerTraits)
	{
		pCacheData->m_bIsRiverTradeRoad = pPlayerTraits->IsRiverTradeRoad();
		pCacheData->m_bIsMoveFriendlyWoodsAsRoad = pPlayerTraits->IsMoveFriendlyWoodsAsRoad();
	}
	else
	{
		pCacheData->m_bIsRiverTradeRoad = false;
		pCacheData->m_bIsMoveFriendlyWoodsAsRoad = false;
	}

}

//	--------------------------------------------------------------------------------
#ifdef AUI_ASTAR_USE_DELEGATES
void CvAStar::TradePathUninitialize()
#else
void TradePathUninitialize(const void* pointer, CvAStar* finder)
#endif
{

}

//	--------------------------------------------------------------------------------
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::TradeRouteHeuristic(int iFromX, int iFromY, int iToX, int iToY) const
#else
int TradeRouteHeuristic(int iFromX, int iFromY, int iToX, int iToY)
#endif
{
	return plotDistance(iFromX, iFromY, iToX, iToY) * 100;
}

//	--------------------------------------------------------------------------------
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::TradeRouteLandPathCost(CvAStarNode* parent, CvAStarNode* node, int data) const
{
	PlayerTypes ePlayer = (PlayerTypes)GetInfo();
#else
int TradeRouteLandPathCost(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
{
	PlayerTypes ePlayer = (PlayerTypes)finder->GetInfo();
#endif

#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	const CvPlot* pFromPlot = parent->m_pPlot;
	const CvPlot* pToPlot = node->m_pPlot;
#else
	CvMap& kMap = GC.getMap();
	int iFromPlotX = parent->m_iX;
	int iFromPlotY = parent->m_iY;
	CvPlot* pFromPlot = kMap.plotUnchecked(iFromPlotX, iFromPlotY);

	int iToPlotX = node->m_iX;
	int iToPlotY = node->m_iY;
	CvPlot* pToPlot = kMap.plotUnchecked(iToPlotX, iToPlotY);
#endif

	int iBaseCost = 100;
	int iCost = iBaseCost;

#ifdef AUI_ASTAR_USE_DELEGATES
	const TradePathCacheData* pCacheData = reinterpret_cast<const TradePathCacheData*>(GetScratchBuffer());
#else
	const TradePathCacheData* pCacheData = reinterpret_cast<const TradePathCacheData*>(finder->GetScratchBuffer());
#endif
	FeatureTypes eFeature = pToPlot->getFeatureType();

	// super duper low costs for moving along routes
	if (pFromPlot->getRouteType() != NO_ROUTE && pToPlot->getRouteType() != NO_ROUTE)
	{
		iCost = iCost / 2;
	}
	//// super low costs for moving along rivers
	else if (pCacheData->IsRiverTradeRoad() && pFromPlot->isRiver() && pToPlot->isRiver())
	{
		iCost = iCost / 2;
	}
	// Iroquios ability
	else if ((eFeature == FEATURE_FOREST || eFeature == FEATURE_JUNGLE) && pCacheData->IsMoveFriendlyWoodsAsRoad())
	{
		iCost = iCost / 2;
	}
	else
	{
		bool bFeaturePenalty = false;
		if (eFeature == FEATURE_FOREST || eFeature == FEATURE_JUNGLE || eFeature == FEATURE_ICE)
		{
			bFeaturePenalty = true;
		}

		if (pToPlot->isHills() || bFeaturePenalty)
		{
			iCost += 1;
		}

		// extra cost for not going to an oasis! (this encourages routes to go through oasis)
		if (eFeature != FEATURE_OASIS)
		{
			iCost += 1;
		}
	}

	if (pToPlot->isWater() && !pToPlot->IsAllowsWalkWater())
	{
		iCost += 1000;
	}
	
	// Penalty for ending a turn on a mountain
	if(pToPlot->isImpassable() || pToPlot->isMountain())
	{
		iCost += 1000;
	}

	FAssert(iCost != MAX_INT);
	FAssert(iCost > 0);

	return iCost;
}

//	--------------------------------------------------------------------------------
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::TradeRouteLandValid(CvAStarNode* parent, CvAStarNode* node, int data) const
#else
int TradeRouteLandValid(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
	if(parent == NULL)
	{
		return TRUE;
	}

#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	const CvPlot* pOldPlot = parent->m_pPlot;
	const CvPlot* pNewPlot = node->m_pPlot;
	if (!pOldPlot || !pNewPlot)
		return FALSE;

	if(pOldPlot->getArea() != pNewPlot->getArea())
#else
	CvMap& kMap = GC.getMap();
	CvPlot* pNewPlot = kMap.plotUnchecked(node->m_iX, node->m_iY);

	if(kMap.plotUnchecked(parent->m_iX, parent->m_iY)->getArea() != pNewPlot->getArea())
#endif
	{
		return FALSE;
	}

	if (pNewPlot->isWater())
	{
		return FALSE;
	}

#ifdef AUI_ASTAR_FIX_STEP_VALID_CONSIDERS_MOUNTAINS
	if (pNewPlot->isImpassable())
#else
	if(pNewPlot->isMountain() || pNewPlot->isImpassable())
#endif
	{
		return FALSE;
	}

	return TRUE;
}

//	--------------------------------------------------------------------------------
/// slewis's fault
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::TradeRouteWaterPathCost(CvAStarNode* parent, CvAStarNode* node, int data) const
#else
int TradeRouteWaterPathCost(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
#ifndef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	CvMap& kMap = GC.getMap();
#endif
#ifdef AUI_ASTAR_USE_DELEGATES
	const TradePathCacheData* pCacheData = reinterpret_cast<const TradePathCacheData*>(GetScratchBuffer());
#else
	const TradePathCacheData* pCacheData = reinterpret_cast<const TradePathCacheData*>(finder->GetScratchBuffer());
#endif

#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	const CvPlot* pFromPlot = parent->m_pPlot;
	const CvPlot* pToPlot = node->m_pPlot;
#else
	int iFromPlotX = parent->m_iX;
	int iFromPlotY = parent->m_iY;
	CvPlot* pFromPlot = kMap.plotUnchecked(iFromPlotX, iFromPlotY);

	int iToPlotX = node->m_iX;
	int iToPlotY = node->m_iY;
	CvPlot* pToPlot = kMap.plotUnchecked(iToPlotX, iToPlotY);
#endif

	int iBaseCost = 100;
	int iCost = iBaseCost;

	if (!pToPlot->isCity())
	{
		bool bIsAdjacentToLand = pFromPlot->isAdjacentToLand_Cached() && pToPlot->isAdjacentToLand_Cached();
		if (!bIsAdjacentToLand)
		{
			iCost += 1;
		}

		// if is enemy tile, avoid
		TeamTypes eToPlotTeam = pToPlot->getTeam();
		if (eToPlotTeam != NO_TEAM && pCacheData->getTeam().isAtWar(eToPlotTeam))
		{
			iCost += 1000; // slewis - is this too prohibitive? Too cheap?
		}

		if (!pToPlot->isWater())
		{
			iCost += 1000;
		}
		else
		{
			if (pToPlot->getTerrainType() != (TerrainTypes) GC.getSHALLOW_WATER_TERRAIN())	// Quicker isShallowWater test, since we already know the plot is water
			{
				if (!pCacheData->CanEmbarkAllWaterPassage())
				{
					iCost += 1000;
				}
			}
		}

		if(pToPlot->isImpassable())
		{
			iCost += 1000;
		}
	}

	FAssert(iCost != MAX_INT);
	FAssert(iCost > 0);

	return iCost;
}

//	--------------------------------------------------------------------------------
#ifdef AUI_ASTAR_USE_DELEGATES
int CvAStar::TradeRouteWaterValid(CvAStarNode* parent, CvAStarNode* node, int data) const
#else
int TradeRouteWaterValid(CvAStarNode* parent, CvAStarNode* node, int data, const void* pointer, CvAStar* finder)
#endif
{
	if(parent == NULL)
	{
		return TRUE;
	}

#ifdef AUI_ASTAR_USE_DELEGATES
	const TradePathCacheData* pCacheData = reinterpret_cast<const TradePathCacheData*>(GetScratchBuffer());
#else
	const TradePathCacheData* pCacheData = reinterpret_cast<const TradePathCacheData*>(finder->GetScratchBuffer());
#endif

#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
	const CvPlot* pNewPlot = node->m_pPlot;
	if (!pNewPlot)
		return FALSE;
#else
	CvMap& kMap = GC.getMap();
	CvPlot* pNewPlot = kMap.plotUnchecked(node->m_iX, node->m_iY);
#endif

	if (!pNewPlot->isCity())
	{
		if (!pNewPlot->isWater())
		{
			return FALSE;
		}

		if (pNewPlot->getTerrainType() != (TerrainTypes) GC.getSHALLOW_WATER_TERRAIN())	// Quicker shallow water test since we know that the plot is water already
		{
			if (!pCacheData->CanEmbarkAllWaterPassage())
			{
				return FALSE;
			}
		}

#ifdef AUI_ASTAR_CACHE_PLOTS_AT_NODES
		const CvPlot* pParentPlot = parent->m_pPlot;
		if (!pParentPlot)
			return FALSE;
#else
		CvPlot* pParentPlot = kMap.plotUnchecked(parent->m_iX, parent->m_iY);
#endif
		if (!pParentPlot->isCity())
		{
			if(pParentPlot->getArea() != pNewPlot->getArea())
			{
				return FALSE;
			}
		}

		if(pNewPlot->isImpassable())
		{
			return FALSE;
		}
	}

	return TRUE;
}

//	--------------------------------------------------------------------------------
// Copy the supplied node and its parent nodes into an array of simpler path nodes for caching purposes.
// It is ok to pass in NULL, the resulting array will contain zero elements
//static
void CvAStar::CopyPath(const CvAStarNode* pkEndNode, CvPathNodeArray& kPathArray)
{
	if(pkEndNode != NULL)
	{
		const CvAStarNode* pkNode = pkEndNode;

		// Count the number of nodes
		uint uiNodeCount = 1;

		while(pkNode->m_pParent != NULL)
		{
			++uiNodeCount;
			pkNode = pkNode->m_pParent;
		}

		kPathArray.setsize(uiNodeCount);

		pkNode = pkEndNode;
		kPathArray[0] = *pkNode;

		uint uiIndex = 1;
		while(pkNode->m_pParent != NULL)
		{
			pkNode = pkNode->m_pParent;
			kPathArray[uiIndex++] = *pkNode;
		}
	}
	else
		kPathArray.setsize(0);	// Setting the size to 0 rather than clearing so that the array data is not deleted.  Helps with memory thrashing.
}

//	---------------------------------------------------------------------------
const CvPathNode* CvPathNodeArray::GetTurnDest(int iTurn)
{
	for (uint i = size(); i--; )
	{
		const CvPathNode& kNode = at(i);
		if (i == 0)
		{
			// Last node, only return it if it is the desired turn
			if (kNode.m_iData2 == iTurn)
				return &kNode;
			return NULL;
		}
		else
		{
			// Is this node the correct turn and the next node is a turn after it?
			if (kNode.m_iData2 == iTurn && at(i-1).m_iData2 > iTurn)
				return &kNode;
		}
	}

	return NULL;
}
