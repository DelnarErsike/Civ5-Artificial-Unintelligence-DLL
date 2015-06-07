/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#include "CvGameCoreDLLPCH.h"
#include "CvDangerPlots.h"
#include "CvGameCoreUtils.h"
#include "CvAStar.h"
#include "CvEnumSerialization.h"
#include "CvDiplomacyAI.h"
#include "CvMilitaryAI.h"
#include "CvMinorCivAI.h"
#include "FireWorks/FRemark.h"

// must be included after all other headers
#include "LintFree.h"
#ifdef _MSC_VER
#pragma warning ( disable : 4505 ) // unreferenced local function has been removed.. needed by REMARK below
#endif//_MSC_VER

REMARK_GROUP("CvDangerPlots");

/// Constructor
CvDangerPlots::CvDangerPlots(void)
	: m_ePlayer(NO_PLAYER)
	, m_bArrayAllocated(false)
	, m_bDirty(false)
{
	m_fMajorWarMod = GC.getAI_DANGER_MAJOR_APPROACH_WAR();
	m_fMajorHostileMod = GC.getAI_DANGER_MAJOR_APPROACH_HOSTILE();
	m_fMajorDeceptiveMod = GC.getAI_DANGER_MAJOR_APPROACH_DECEPTIVE();
	m_fMajorGuardedMod = GC.getAI_DANGER_MAJOR_APPROACH_GUARDED();
	m_fMajorAfraidMod = GC.getAI_DANGER_MAJOR_APPROACH_AFRAID();
	m_fMajorFriendlyMod = GC.getAI_DANGER_MAJOR_APPROACH_FRIENDLY();
	m_fMajorNeutralMod = GC.getAI_DANGER_MAJOR_APPROACH_NEUTRAL();
	m_fMinorNeutralrMod = GC.getAI_DANGER_MINOR_APPROACH_NEUTRAL();
	m_fMinorFriendlyMod = GC.getAI_DANGER_MINOR_APPROACH_FRIENDLY();
	m_fMinorBullyMod = GC.getAI_DANGER_MINOR_APPROACH_BULLY();
	m_fMinorConquestMod = GC.getAI_DANGER_MINOR_APPROACH_CONQUEST();

#ifdef AUI_DANGER_PLOTS_REMADE
	m_DangerPlots = NULL;
#endif
}

/// Destructor
CvDangerPlots::~CvDangerPlots(void)
{
	Uninit();
}

/// Initialize
void CvDangerPlots::Init(PlayerTypes ePlayer, bool bAllocate)
{
	Uninit();
	m_ePlayer = ePlayer;

	if(bAllocate)
	{
		int iGridSize = GC.getMap().numPlots();
		CvAssertMsg(iGridSize > 0, "iGridSize is zero");
#ifdef AUI_DANGER_PLOTS_REMADE
		m_DangerPlots = FNEW(CvDangerPlotContents[iGridSize], c_eCiv5GameplayDLL, 0);
#else
		m_DangerPlots.resize(iGridSize);
#endif
		m_bArrayAllocated = true;
		for(int i = 0; i < iGridSize; i++)
		{
#ifdef AUI_DANGER_PLOTS_REMADE
			m_DangerPlots[i].init(GC.getMap().plotByIndexUnchecked(i));
#else
			m_DangerPlots[i] = 0;
#endif
		}
	}
}

/// Uninitialize
void CvDangerPlots::Uninit()
{
	m_ePlayer = NO_PLAYER;
#ifdef AUI_DANGER_PLOTS_REMADE
	SAFE_DELETE_ARRAY(m_DangerPlots);
#else
	m_DangerPlots.clear();
#endif
	m_bArrayAllocated = false;
	m_bDirty = false;
}

/// Updates the danger plots values to reflect threats across the map
void CvDangerPlots::UpdateDanger(bool bPretendWarWithAllCivs, bool bIgnoreVisibility)
{
	// danger plots have not been initialized yet, so no need to update
	if(!m_bArrayAllocated)
	{
		return;
	}

	// wipe out values
	int iGridSize = GC.getMap().numPlots();
	CvAssertMsg(iGridSize == m_DangerPlots.size(), "iGridSize does not match number of DangerPlots");
	for(int i = 0; i < iGridSize; i++)
	{
#ifdef AUI_DANGER_PLOTS_REMADE
		m_DangerPlots[i].clear();
#else
		m_DangerPlots[i] = 0;
#endif
	}


	CvPlayer& thisPlayer = GET_PLAYER(m_ePlayer);
	TeamTypes thisTeam = thisPlayer.getTeam();

	// for each opposing civ
	for(int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
	{
		PlayerTypes ePlayer = (PlayerTypes)iPlayer;
		CvPlayer& loopPlayer = GET_PLAYER(ePlayer);
		TeamTypes eTeam = loopPlayer.getTeam();

		if(!loopPlayer.isAlive())
		{
			continue;
		}

		if(eTeam == thisTeam)
		{
			continue;
		}

		if(ShouldIgnorePlayer(ePlayer) && !bPretendWarWithAllCivs)
		{
			continue;
		}

		//for each unit
		int iLoop;
		CvUnit* pLoopUnit = NULL;
		for(pLoopUnit = loopPlayer.firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = loopPlayer.nextUnit(&iLoop))
		{
			if(ShouldIgnoreUnit(pLoopUnit, bIgnoreVisibility))
			{
				continue;
			}

			int iRange = pLoopUnit->baseMoves();
#ifdef AUI_ASTAR_TWEAKED_OPTIMIZED_BUT_CAN_STILL_USE_ROADS
			IncreaseMoveRangeForRoads(pLoopUnit, iRange);
#endif
			if(pLoopUnit->canRangeStrike())
			{
				iRange += pLoopUnit->GetRange() - 1;
			}

			CvPlot* pUnitPlot = pLoopUnit->plot();
#ifndef AUI_DANGER_PLOTS_REMADE
			AssignUnitDangerValue(pLoopUnit, pUnitPlot);
#endif
			CvPlot* pLoopPlot = NULL;

#ifdef AUI_HEXSPACE_DX_LOOPS
			for (int iDY = -iRange; iDY <= iRange; iDY++)
			{
#ifdef AUI_FAST_COMP
				int iMaxDX = iRange - FASTMAX(0, iDY);
				for (int iDX = -iRange - FASTMIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#else
				iMaxDX = iRange - MAX(0, iDY);
				for (iDX = -iRange - MIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#endif
				{
					// No need for range check because loops are set up properly
					pLoopPlot = plotXY(pUnitPlot->getX(), pUnitPlot->getY(), iDX, iDY);
#else
			for(int iDX = -(iRange); iDX <= iRange; iDX++)
			{
				for(int iDY = -(iRange); iDY <= iRange; iDY++)
				{
					pLoopPlot = plotXYWithRangeCheck(pUnitPlot->getX(), pUnitPlot->getY(), iDX, iDY, iRange);
#endif
					if(!pLoopPlot || pLoopPlot == pUnitPlot)
					{
						continue;
					}

#ifndef  AUI_DANGER_PLOTS_REMADE
#ifdef AUI_UNIT_CAN_MOVE_AND_RANGED_STRIKE
					if (!pLoopUnit->canMoveOrAttackInto(*pLoopPlot) && (!pLoopUnit->isRanged() || !pLoopUnit->canMoveAndRangedStrike(pLoopPlot)))
#else
					if(!pLoopUnit->canMoveOrAttackInto(*pLoopPlot) && !pLoopUnit->canRangeStrikeAt(pLoopPlot->getX(),pLoopPlot->getY()))
#endif
					{
						continue;
					}
#endif

					AssignUnitDangerValue(pLoopUnit, pLoopPlot);
				}
			}
		}

		// for each city
		CvCity* pLoopCity;
		for(pLoopCity = loopPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = loopPlayer.nextCity(&iLoop))
		{
			if(ShouldIgnoreCity(pLoopCity, bIgnoreVisibility))
			{
				continue;
			}

			int iRange = GC.getCITY_ATTACK_RANGE();
			CvPlot* pCityPlot = pLoopCity->plot();
#ifndef AUI_DANGER_PLOTS_REMADE
			AssignCityDangerValue(pLoopCity, pCityPlot);
#endif
			CvPlot* pLoopPlot = NULL;

#ifdef AUI_HEXSPACE_DX_LOOPS
			int iMaxDX, iDX;
			for (int iDY = -iRange; iDY <= iRange; iDY++)
			{
#ifdef AUI_FAST_COMP
				iMaxDX = iRange - FASTMAX(0, iDY);
				for (iDX = -iRange - FASTMIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#else
				iMaxDX = iRange - MAX(0, iDY);
				for (iDX = -iRange - MIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#endif
				{
					// No need for range check because loops are set up properly
					pLoopPlot = plotXY(pCityPlot->getX(), pCityPlot->getY(), iDX, iDY);
#else
			for(int iDX = -(iRange); iDX <= iRange; iDX++)
			{
				for(int iDY = -(iRange); iDY <= iRange; iDY++)
				{
					pLoopPlot = plotXYWithRangeCheck(pCityPlot->getX(), pCityPlot->getY(), iDX, iDY, iRange);
#endif
					if(!pLoopPlot)
					{
						continue;
					}
#ifdef AUI_DANGER_PLOTS_REMADE
					if (pLoopPlot == pCityPlot)
					{
						continue;
					}
#endif

					AssignCityDangerValue(pLoopCity, pLoopPlot);
				}
			}
		}
	}

	// Citadels
#ifndef AUI_DANGER_PLOTS_REMADE
	int iCitadelValue = GetDangerValueOfCitadel();
#endif
	int iPlotLoop;
	CvPlot* pPlot, *pAdjacentPlot;
	for(iPlotLoop = 0; iPlotLoop < GC.getMap().numPlots(); iPlotLoop++)
	{
		pPlot = GC.getMap().plotByIndexUnchecked(iPlotLoop);

		if(pPlot->isRevealed(thisTeam))
		{
#ifdef AUI_DANGER_PLOTS_REMADE
			if (pPlot->getFeatureType() != NO_FEATURE)
			{
				m_DangerPlots[iPlotLoop].m_iFlatPlotDamage = (GC.getFeatureInfo(pPlot->getFeatureType())->getTurnDamage());
			}
#endif
			ImprovementTypes eImprovement = pPlot->getRevealedImprovementType(thisTeam);
			if(eImprovement != NO_IMPROVEMENT && GC.getImprovementInfo(eImprovement)->GetNearbyEnemyDamage() > 0)
			{
				if(!ShouldIgnoreCitadel(pPlot, bIgnoreVisibility))
				{
					for(int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
					{
						pAdjacentPlot = plotDirection(pPlot->getX(), pPlot->getY(), ((DirectionTypes)iI));

						if(pAdjacentPlot != NULL)
						{
#ifdef AUI_DANGER_PLOTS_REMADE
							m_DangerPlots[iPlotLoop].m_pCitadel = pPlot;
#else
							AddDanger(pAdjacentPlot->getX(), pAdjacentPlot->getY(), iCitadelValue, true);
#endif
						}
					}
				}
			}
		}
	}

	// testing city danger values
	CvCity* pLoopCity;
	int iLoopCity = 0;
	for(pLoopCity = thisPlayer.firstCity(&iLoopCity); pLoopCity != NULL; pLoopCity = thisPlayer.nextCity(&iLoopCity))
	{
		int iThreatValue = GetCityDanger(pLoopCity);
		pLoopCity->SetThreatValue(iThreatValue);
	}

	m_bDirty = false;
}

#ifdef AUI_DANGER_PLOTS_REMADE
/// Updates the danger plots values based on a single plot (called when the AI's information is updated mid-routines
void CvDangerPlots::UpdateDanger(CvPlot* pPlot, bool bPretendWarWithAllCivs, bool bIgnoreVisibility)
{
	if (!pPlot)
		return;

	CvPlayer& thisPlayer = GET_PLAYER(m_ePlayer);
	TeamTypes thisTeam = thisPlayer.getTeam();

	IDInfo* pUnitNode = pPlot->headUnitNode();
	while (pUnitNode != NULL)
	{
		CvUnit* pLoopUnit = GetPlayerUnit(*pUnitNode);
		pUnitNode = pPlot->nextUnitNode(pUnitNode);

		if (pLoopUnit && pLoopUnit->getOwner() != m_ePlayer && (!ShouldIgnorePlayer(pLoopUnit->getOwner()) || bPretendWarWithAllCivs) && !ShouldIgnoreUnit(pLoopUnit, bIgnoreVisibility))
		{
			int iRange = pLoopUnit->baseMoves();
#ifdef AUI_ASTAR_TWEAKED_OPTIMIZED_BUT_CAN_STILL_USE_ROADS
			IncreaseMoveRangeForRoads(pLoopUnit, iRange);
#endif
			if (pLoopUnit->canRangeStrike())
			{
				iRange += pLoopUnit->GetRange() - 1;
			}

			CvPlot* pLoopPlot = NULL;
			for (int iDY = -iRange; iDY <= iRange; iDY++)
			{
#ifdef AUI_FAST_COMP
				int iMaxDX = iRange - FASTMAX(0, iDY);
				for (int iDX = -iRange - FASTMIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#else
				iMaxDX = iRange - MAX(0, iDY);
				for (iDX = -iRange - MIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#endif
				{
					// No need for range check because loops are set up properly
					pLoopPlot = plotXY(pPlot->getX(), pPlot->getY(), iDX, iDY);
					if (pLoopPlot && pLoopPlot != pPlot)
					{
						AssignUnitDangerValue(pLoopUnit, pLoopPlot);
					}
				}
			}
		}
	}

	CvCity* pCityInPlot = pPlot->getPlotCity();
	if (pCityInPlot && pCityInPlot->getOwner() != m_ePlayer && !ShouldIgnoreCity(pCityInPlot, bIgnoreVisibility) && (!ShouldIgnorePlayer(pCityInPlot->getOwner()) || bPretendWarWithAllCivs))
	{
		int iRange = GC.getCITY_ATTACK_RANGE();
		CvPlot* pLoopPlot = NULL;

		int iMaxDX, iDX;
		for (int iDY = -iRange; iDY <= iRange; iDY++)
		{
#ifdef AUI_FAST_COMP
			iMaxDX = iRange - FASTMAX(0, iDY);
			for (iDX = -iRange - FASTMIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#else
			iMaxDX = iRange - MAX(0, iDY);
			for (iDX = -iRange - MIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#endif
			{
				// No need for range check because loops are set up properly
				pLoopPlot = plotXY(pPlot->getX(), pPlot->getY(), iDX, iDY);
				if (pLoopPlot && pLoopPlot != pPlot)
				{
					AssignCityDangerValue(pCityInPlot, pLoopPlot);
				}
			}
		}
	}

	int iPlotIndex = GC.getMap().plotNum(pPlot->getX(), pPlot->getY());
	if (pPlot->isRevealed(thisTeam))
	{
		if (pPlot->getFeatureType() != NO_FEATURE)
		{
			m_DangerPlots[iPlotIndex].m_iFlatPlotDamage = (GC.getFeatureInfo(pPlot->getFeatureType())->getTurnDamage());
		}
		ImprovementTypes eImprovement = pPlot->getRevealedImprovementType(thisTeam);
		if (eImprovement != NO_IMPROVEMENT && GC.getImprovementInfo(eImprovement)->GetNearbyEnemyDamage() > 0)
		{
			if (!ShouldIgnoreCitadel(pPlot, bIgnoreVisibility))
			{
				for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
				{
					CvPlot* pAdjacentPlot = plotDirection(pPlot->getX(), pPlot->getY(), ((DirectionTypes)iI));

					if (pAdjacentPlot != NULL)
					{
						m_DangerPlots[iPlotIndex].m_pCitadel = pPlot;
					}
				}
			}
		}
	}
}

/// Return the maximum amount of damage that could be dealt to a non-specific unit at this plot
int CvDangerPlots::GetDanger(const CvPlot& kPlot, const PlayerTypes ePlayer)
{
	const int idx = kPlot.getX() + kPlot.getY() * GC.getMap().getGridWidth();
	return m_DangerPlots[idx].GetDanger(ePlayer);
}

/// Return the maximum amount of damage a city could take at this plot
int CvDangerPlots::GetDanger(const CvPlot& kPlot, const CvCity* pCity, int iAfterNIntercepts, PlayerTypes ePretendCityOwner, const CvUnit* pPretendGarrison, int iPretendGarrisonExtraDamage)
{
	const int idx = kPlot.getX() + kPlot.getY() * GC.getMap().getGridWidth();
	if (pCity != NULL)
	{
		return m_DangerPlots[idx].GetDanger(pCity, iAfterNIntercepts, ePretendCityOwner, pPretendGarrison, iPretendGarrisonExtraDamage);
	}
	return 0;
}

/// Return the maximum amount of damage a unit could take at this plot
int CvDangerPlots::GetDanger(const CvPlot& pPlot, const CvUnit* pUnit, const CvPlot* pAttackTarget, const int iAction, int iAfterNIntercepts)
#else
/// Add an amount of danger to a given tile
void CvDangerPlots::AddDanger(int iPlotX, int iPlotY, int iValue, bool bWithinOneMove)
{
	const int idx = iPlotX + iPlotY * GC.getMap().getGridWidth();
	if (iValue > 0)
	{
		if (bWithinOneMove)
		{
			iValue |= 0x1;
		}
		else
		{
			iValue &= ~0x1;
		}
	}

	m_DangerPlots[idx] += iValue;
}

/// Return the danger value of a given plot
int CvDangerPlots::GetDanger(const CvPlot& pPlot) const
#endif
{
	const int idx = pPlot.getX() + pPlot.getY() * GC.getMap().getGridWidth();
#ifdef AUI_DANGER_PLOTS_REMADE
	if (pUnit)
	{
		return m_DangerPlots[idx].GetDanger(pUnit, pAttackTarget, iAction, iAfterNIntercepts);
	}
	return 0;
#else
	return m_DangerPlots[idx];
#endif
}

#ifdef AUI_DANGER_PLOTS_REMADE
/// Returns the amount of damage citadels could do to this plot
int CvDangerPlots::GetDangerFromCitadel(const CvPlot& kPlot, const PlayerTypes ePlayer)
{
	const int idx = kPlot.getX() + kPlot.getY() * GC.getMap().getGridWidth();
	return m_DangerPlots[idx].GetCitadelDamage(ePlayer);
}

/// Returns if the tile is in danger
bool CvDangerPlots::IsUnderImmediateThreat(const CvPlot& kPlot, const PlayerTypes ePlayer)
{
	const int idx = kPlot.getX() + kPlot.getY() * GC.getMap().getGridWidth();
	return m_DangerPlots[idx].IsUnderImmediateThreat(ePlayer);
}

/// Returns if the unit is in immediate danger
bool CvDangerPlots::IsUnderImmediateThreat(const CvPlot& kPlot, const CvUnit* pUnit)
{
	const int idx = kPlot.getX() + kPlot.getY() * GC.getMap().getGridWidth();
	if (pUnit)
	{
		return m_DangerPlots[idx].IsUnderImmediateThreat(pUnit);
	}
	return false;
}

/// Returns if the unit is in immediate danger
bool CvDangerPlots::CouldAttackHere(const CvPlot& kPlot, const CvUnit* pUnit)
{
	const int idx = kPlot.getX() + kPlot.getY() * GC.getMap().getGridWidth();
	if (!pUnit)
	{
		return false;
	}
	return m_DangerPlots[idx].CouldAttackHere(pUnit);
}

/// Returns if the unit is in immediate danger
bool CvDangerPlots::CouldAttackHere(const CvPlot& kPlot, const CvCity* pCity)
{
	const int idx = kPlot.getX() + kPlot.getY() * GC.getMap().getGridWidth();
	if (!pCity)
	{
		return false;
	}
	return m_DangerPlots[idx].CouldAttackHere(pCity);
}
#else
/// Returns if the unit is in immediate danger
bool CvDangerPlots::IsUnderImmediateThreat(const CvPlot& pPlot) const
{
	return GetDanger(pPlot) & 0x1;
}
#endif

/// Sums the danger values of the plots around the city to determine the danger value of the city
int CvDangerPlots::GetCityDanger(CvCity* pCity)
{
	CvAssertMsg(pCity, "pCity is null");
	if(!pCity) return 0;

	CvAssertMsg(pCity->getOwner() == m_ePlayer, "City does not belong to us");

	CvPlot* pPlot = pCity->plot();
	int iEvalRange = GC.getAI_DIPLO_PLOT_RANGE_FROM_CITY_HOME_FRONT();

	int iDangerValue = 0;

#ifdef AUI_HEXSPACE_DX_LOOPS
	CvPlot* pEvalPlot;
	for (int iDY = -iEvalRange; iDY <= iEvalRange; iDY++)
	{
#ifdef AUI_FAST_COMP
		int iMaxDX = iEvalRange - FASTMAX(0, iDY);
		for (int iDX = -iEvalRange - FASTMIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#else
		int iMaxDX = iEvalRange - MAX(0, iDY);
		for (int iDX = -iEvalRange - MIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#endif
		{
			// No need for range check because loops are set up properly
			pEvalPlot = plotXY(pPlot->getX(), pPlot->getY(), iDX, iDY);
#else
	for(int iX = -iEvalRange; iX <= iEvalRange; iX++)
	{
		for(int iY = -iEvalRange; iY <= iEvalRange; iY++)
		{
			CvPlot* pEvalPlot = plotXYWithRangeCheck(pPlot->getX(), pPlot->getY(), iX, iY, iEvalRange);
#endif
			if(!pEvalPlot)
			{
				continue;
			}

#ifdef AUI_DANGER_PLOTS_REMADE
#ifdef AUI_FAST_COMP
			iDangerValue += (GetDanger(*pEvalPlot, pCity) / (FASTMAX(hexDistance(iDX, iDY) * NUM_DIRECTION_TYPES, 1)));
#else
			iDangerValue += (GetDanger(*pEvalPlot, pCity) / (MAX(hexDistance(iDX, iDY) * NUM_DIRECTION_TYPES, 1)));
#endif
#else
			iDangerValue += GetDanger(*pEvalPlot);
#endif
		}
	}

	return iDangerValue;
}

#ifndef AUI_DANGER_PLOTS_REMADE
int CvDangerPlots::ModifyDangerByRelationship(PlayerTypes ePlayer, CvPlot* pPlot, int iDanger)
{
	CvAssertMsg(pPlot, "No plot passed in?");
	bool bIgnoreInFriendlyTerritory = false;
	int iResult = iDanger;

	// Full value if a player we're at war with
	if(GET_TEAM(GET_PLAYER(m_ePlayer).getTeam()).isAtWar(GET_PLAYER(ePlayer).getTeam()))
	{
		return iResult;
	}

	// if it's a human player, ignore neutral units
	if(GET_PLAYER(m_ePlayer).isHuman())
	{
		return 0;
	}

	if(GET_PLAYER(m_ePlayer).isMinorCiv())  // if the evaluator is a minor civ
	{
		if(!GET_TEAM(GET_PLAYER(m_ePlayer).getTeam()).isAtWar(GET_PLAYER(ePlayer).getTeam()))  // and they're not at war with the other player
		{
			bIgnoreInFriendlyTerritory = true; // ignore friendly territory
		}
	}
	else if(!GET_PLAYER(ePlayer).isMinorCiv())
	{
		// should we be using bHideTrueFeelings?
		switch(GET_PLAYER(m_ePlayer).GetDiplomacyAI()->GetMajorCivApproach(ePlayer, /*bHideTrueFeelings*/ false))
		{
		case MAJOR_CIV_APPROACH_WAR:
			iResult = (int)(iResult * m_fMajorWarMod);
			break;
		case MAJOR_CIV_APPROACH_HOSTILE:
			iResult = (int)(iResult * m_fMajorHostileMod);
			bIgnoreInFriendlyTerritory = true;
			break;
		case MAJOR_CIV_APPROACH_DECEPTIVE:
			iResult = (int)(iResult * m_fMajorDeceptiveMod);
			bIgnoreInFriendlyTerritory = true;
			break;
		case MAJOR_CIV_APPROACH_GUARDED:
			iResult = (int)(iResult * m_fMajorGuardedMod);
			bIgnoreInFriendlyTerritory = true;
			break;
		case MAJOR_CIV_APPROACH_AFRAID:
			iResult = (int)(iResult * m_fMajorAfraidMod);
			bIgnoreInFriendlyTerritory = true;
			break;
		case MAJOR_CIV_APPROACH_FRIENDLY:
			iResult = (int)(iResult * m_fMajorFriendlyMod);
			bIgnoreInFriendlyTerritory = true;
			break;
		case MAJOR_CIV_APPROACH_NEUTRAL:
			iResult = (int)(iResult * m_fMajorNeutralMod);
			bIgnoreInFriendlyTerritory = true;
			break;
		}
	}
	else
	{
		switch(GET_PLAYER(m_ePlayer).GetDiplomacyAI()->GetMinorCivApproach(ePlayer))
		{
		case MINOR_CIV_APPROACH_IGNORE:
			iResult = (int)(iResult * m_fMinorNeutralrMod);
			bIgnoreInFriendlyTerritory = true;
			break;
		case MINOR_CIV_APPROACH_FRIENDLY:
			iResult = (int)(iResult * m_fMinorFriendlyMod);
			bIgnoreInFriendlyTerritory = true;
			break;
		case MINOR_CIV_APPROACH_BULLY:
			iResult = (int)(iResult * m_fMinorBullyMod);
			break;
		case MINOR_CIV_APPROACH_CONQUEST:
			iResult = (int)(iResult * m_fMinorConquestMod);
			break;
		}
	}

	// if the plot is in our own territory and, with the current approach, we should ignore danger values in our own territory
	// zero out the value
	if(pPlot && pPlot->getOwner() == m_ePlayer && bIgnoreInFriendlyTerritory)
	{
		iResult = 0;
	}

	return iResult;
}
#endif

//	------------------------------------------------------------------------------------------------
//	Returns true if the relationship of the danger plots owner and the input player and plot owner
//	would result in a 0 danger.  This helps avoid costly path finder calls if the end result will be 0.
bool CvDangerPlots::IsDangerByRelationshipZero(PlayerTypes ePlayer, CvPlot* pPlot)
{
	CvAssertMsg(pPlot, "No plot passed in?");
	bool bIgnoreInFriendlyTerritory = false;

	// Full value if a player we're at war with
	if(GET_TEAM(GET_PLAYER(m_ePlayer).getTeam()).isAtWar(GET_PLAYER(ePlayer).getTeam()))
	{
		return false;
	}

	// if it's a human player, ignore neutral units
	if(GET_PLAYER(m_ePlayer).isHuman())
	{
		return true;
	}

	bool bResultMultiplierIsZero = false;
	if(GET_PLAYER(m_ePlayer).isMinorCiv())  // if the evaluator is a minor civ
	{
		if(!GET_TEAM(GET_PLAYER(m_ePlayer).getTeam()).isAtWar(GET_PLAYER(ePlayer).getTeam()))  // and they're not at war with the other player
		{
#ifdef AUI_DANGER_PLOTS_FIX_IS_DANGER_BY_RELATIONSHIP_ZERO_MINORS_IGNORE_ALL_NONWARRED
			return true;
#else
			bIgnoreInFriendlyTerritory = true; // ignore friendly territory
#endif
		}
	}
	else if(!GET_PLAYER(ePlayer).isMinorCiv())
	{
		// should we be using bHideTrueFeelings?
		switch(GET_PLAYER(m_ePlayer).GetDiplomacyAI()->GetMajorCivApproach(ePlayer, /*bHideTrueFeelings*/ false))
		{
		case MAJOR_CIV_APPROACH_WAR:
			bResultMultiplierIsZero = m_fMajorWarMod == 0.f;
			break;
		case MAJOR_CIV_APPROACH_HOSTILE:
			bResultMultiplierIsZero = m_fMajorHostileMod == 0.f;
			bIgnoreInFriendlyTerritory = true;
			break;
		case MAJOR_CIV_APPROACH_DECEPTIVE:
			bResultMultiplierIsZero = m_fMajorDeceptiveMod == 0.f;
			bIgnoreInFriendlyTerritory = true;
			break;
		case MAJOR_CIV_APPROACH_GUARDED:
			bResultMultiplierIsZero = m_fMajorGuardedMod == 0.f;
			bIgnoreInFriendlyTerritory = true;
			break;
		case MAJOR_CIV_APPROACH_AFRAID:
			bResultMultiplierIsZero = m_fMajorAfraidMod == 0.f;
			bIgnoreInFriendlyTerritory = true;
			break;
		case MAJOR_CIV_APPROACH_FRIENDLY:
			bResultMultiplierIsZero = m_fMajorFriendlyMod == 0.f;
			bIgnoreInFriendlyTerritory = true;
			break;
		case MAJOR_CIV_APPROACH_NEUTRAL:
			bResultMultiplierIsZero = m_fMajorNeutralMod == 0.f;
			bIgnoreInFriendlyTerritory = true;
			break;
		}
	}
	else
	{
		switch(GET_PLAYER(m_ePlayer).GetDiplomacyAI()->GetMinorCivApproach(ePlayer))
		{
		case MINOR_CIV_APPROACH_IGNORE:
			bResultMultiplierIsZero = m_fMinorNeutralrMod == 0.f;
			bIgnoreInFriendlyTerritory = true;
			break;
		case MINOR_CIV_APPROACH_FRIENDLY:
			bResultMultiplierIsZero = m_fMinorFriendlyMod == 0.f;
			bIgnoreInFriendlyTerritory = true;
			break;
		case MINOR_CIV_APPROACH_BULLY:
			bResultMultiplierIsZero = (m_fMinorBullyMod == 0.f);
			break;
		case MINOR_CIV_APPROACH_CONQUEST:
			bResultMultiplierIsZero = m_fMinorConquestMod == 0.f;
			break;
		}
	}

	// if the plot is in our own territory and, with the current approach, we should ignore danger values in our own territory
	// zero out the value
	if(pPlot && pPlot->getOwner() == m_ePlayer && bIgnoreInFriendlyTerritory)
	{
		return true;
	}

	return bResultMultiplierIsZero;
}


/// Should this player be ignored when creating the danger plots?
#ifdef AUI_CONSTIFY
bool CvDangerPlots::ShouldIgnorePlayer(PlayerTypes ePlayer) const
#else
bool CvDangerPlots::ShouldIgnorePlayer(PlayerTypes ePlayer)
#endif
{
	if(GET_PLAYER(m_ePlayer).isMinorCiv() != GET_PLAYER(ePlayer).isMinorCiv() && !GET_PLAYER(ePlayer).isBarbarian() && !GET_PLAYER(m_ePlayer).isBarbarian())
	{
		CvPlayer* pMinor = NULL;
		CvPlayer* pMajor;

		if(GET_PLAYER(m_ePlayer).isMinorCiv())
		{
			pMinor = &GET_PLAYER(m_ePlayer);
			pMajor = &GET_PLAYER(ePlayer);
		}
		else
		{
			pMinor = &GET_PLAYER(ePlayer);
			pMajor = &GET_PLAYER(m_ePlayer);
		}

		if(pMinor->GetMinorCivAI()->IsFriends(pMajor->GetID()))
		{
			return true;
		}

		// if we're a major, we should ignore minors that are not at war with us
		if (!GET_PLAYER(m_ePlayer).isMinorCiv())
		{
			TeamTypes eMajorTeam = pMajor->getTeam();
			TeamTypes eMinorTeam = pMinor->getTeam();
			if (!GET_TEAM(eMajorTeam).isAtWar(eMinorTeam))
			{
				return true;
			}
		}
	}

	return false;
}

/// Should this unit be ignored when creating the danger plots?
#ifdef AUI_CONSTIFY
bool CvDangerPlots::ShouldIgnoreUnit(CvUnit* pUnit, bool bIgnoreVisibility) const
#else
bool CvDangerPlots::ShouldIgnoreUnit(CvUnit* pUnit, bool bIgnoreVisibility)
#endif
{
	if(!pUnit->IsCanAttack())
	{
		return true;
	}

#ifdef AUI_DANGER_PLOTS_REMADE
	if (pUnit->getDomainType() == DOMAIN_AIR)
	{
		return false;
	}

	FFastVector<CvPlot*, true, c_eCiv5GameplayDLL> vpPlotsAttackedList = pUnit->GetPlotsAttackedList();
	for (FFastVector<CvPlot*, true, c_eCiv5GameplayDLL>::iterator it = vpPlotsAttackedList.begin(); it != vpPlotsAttackedList.end(); ++it)
	{
		if ((*it)->isVisible(GET_PLAYER(m_ePlayer).getTeam()))
			return false;
	}
#endif

#if defined(AUI_DANGER_PLOTS_SHOULD_IGNORE_UNIT_MAJORS_SEE_BARBARIANS_IN_FOG) || defined(AUI_DANGER_PLOTS_SHOULD_IGNORE_UNIT_MINORS_SEE_MAJORS)
	if (pUnit->isInvisible(GET_PLAYER(m_ePlayer).getTeam(), false))
	{
		return true;
	}
#endif

#ifdef AUI_DANGER_PLOTS_SHOULD_IGNORE_UNIT_MAJORS_SEE_BARBARIANS_IN_FOG
	if (!GET_PLAYER(m_ePlayer).isMinorCiv() && !GET_PLAYER(m_ePlayer).isBarbarian() && pUnit->isBarbarian() && pUnit->plot()->isRevealed(GET_PLAYER(m_ePlayer).getTeam()))
#ifdef AUI_DANGER_PLOTS_REMADE
		if (pUnit->plot()->isAdjacentVisible(GET_PLAYER(m_ePlayer).getTeam()))
#endif
		bIgnoreVisibility = true;
#endif
#ifdef AUI_DANGER_PLOTS_SHOULD_IGNORE_UNIT_MINORS_SEE_MAJORS
	if (GET_PLAYER(m_ePlayer).isMinorCiv() && !GET_PLAYER(pUnit->getOwner()).isMinorCiv() && !pUnit->isBarbarian() && 
		GET_PLAYER(m_ePlayer).GetClosestFriendlyCity(*pUnit->plot(), AUI_DANGER_PLOTS_SHOULD_IGNORE_UNIT_MINORS_SEE_MAJORS))
		bIgnoreVisibility = true;
#endif

#ifdef AUI_DANGER_PLOTS_FIX_SHOULD_IGNORE_UNIT_IGNORE_VISIBILITY_PLOT
	if(!pUnit->plot()->isVisible(GET_PLAYER(m_ePlayer).getTeam()) && !bIgnoreVisibility)
#else
	if(!pUnit->plot()->isVisible(GET_PLAYER(m_ePlayer).getTeam()))
#endif
	{
		return true;
	}

#if !defined(AUI_DANGER_PLOTS_SHOULD_IGNORE_UNIT_MAJORS_SEE_BARBARIANS_IN_FOG) && !defined(AUI_DANGER_PLOTS_SHOULD_IGNORE_UNIT_MINORS_SEE_MAJORS)
	if(pUnit->isInvisible(GET_PLAYER(m_ePlayer).getTeam(), false))
	{
		return true;
	}
#endif

	CvPlot* pPlot = pUnit->plot();
	CvAssertMsg(pPlot, "Plot is null?")

	if(NULL != pPlot && !pPlot->isVisibleOtherUnit(m_ePlayer) && !bIgnoreVisibility)
	{
		return true;
	}

#ifndef AUI_DANGER_PLOTS_REMADE
	// fix post-gold!
	if(pUnit->getDomainType() == DOMAIN_AIR)
	{
		return true;
	}
#endif

	return false;
}

/// Should this city be ignored when creating the danger plots?
#ifdef AUI_CONSTIFY
bool CvDangerPlots::ShouldIgnoreCity(const CvCity* pCity, bool bIgnoreVisibility) const
#else
bool CvDangerPlots::ShouldIgnoreCity(CvCity* pCity, bool bIgnoreVisibility)
#endif
{
	// ignore unseen cities
	if(!pCity->isRevealed(GET_PLAYER(m_ePlayer).getTeam(), false)  && !bIgnoreVisibility)
	{
		return true;
	}

	return false;
}

/// Should this city be ignored when creating the danger plots?
#ifdef AUI_CONSTIFY
bool CvDangerPlots::ShouldIgnoreCitadel(const CvPlot* pCitadelPlot, bool bIgnoreVisibility) const
#else
bool CvDangerPlots::ShouldIgnoreCitadel(CvPlot* pCitadelPlot, bool bIgnoreVisibility)
#endif
{
	// ignore unseen cities
	if(!pCitadelPlot->isRevealed(GET_PLAYER(m_ePlayer).getTeam())  && !bIgnoreVisibility)
	{
		return true;
	}

	PlayerTypes eOwner = pCitadelPlot->getOwner();
	if(eOwner != NO_PLAYER)
	{
		// Our own citadels aren't dangerous
		if(eOwner == m_ePlayer)
		{
			return true;
		}

		if(!atWar(GET_PLAYER(m_ePlayer).getTeam(), GET_PLAYER(eOwner).getTeam()))
		{
			return true;
		}
	}

	return false;
}

//	-----------------------------------------------------------------------------------------------
/// Contains the calculations to do the danger value for the plot according to the unit
#ifdef AUI_DANGER_PLOTS_REMADE
void CvDangerPlots::AssignUnitDangerValue(CvUnit* pUnit, CvPlot* pPlot, bool bReuse)
{
	if (IsDangerByRelationshipZero(pUnit->getOwner(), pPlot))
		return;

	const int iPlotX = pPlot->getX();
	const int iPlotY = pPlot->getY();
	const int idx = GC.getMap().plotNum(iPlotX, iPlotY);

	bool bExcludeFromUnitsList = false;
	for (CvDangerPlotContents::DangerUnitVector::iterator it = m_DangerPlots[idx].m_apUnits.begin(); it != m_DangerPlots[idx].m_apUnits.end(); ++it)
	{
		if ((*it) == pUnit)
		{
			bExcludeFromUnitsList = true;
			break;
		}
	}
	bool bExcludeFromMoveOnlyUnitsList = false;
	for (CvDangerPlotContents::DangerUnitVector::iterator it = m_DangerPlots[idx].m_apMoveOnlyUnits.begin(); it != m_DangerPlots[idx].m_apMoveOnlyUnits.end(); ++it)
	{
		if ((*it) == pUnit)
		{
			bExcludeFromMoveOnlyUnitsList = true;
			break;
		}
	}

	if (bExcludeFromUnitsList && bExcludeFromMoveOnlyUnitsList)
		return;
	
	// Check to see if we're already in this plot
	if (pUnit->plot() == pPlot)
	{
		if (!bExcludeFromUnitsList)
			m_DangerPlots[idx].m_apUnits.push_back(pUnit);
		if (!bExcludeFromMoveOnlyUnitsList)
			m_DangerPlots[idx].m_apMoveOnlyUnits.push_back(pUnit);
		return;
	}
	// Check to see if another player has already done the pathfinding for us
	FFastVector<std::pair<CvPlot*, bool>, true, c_eCiv5GameplayDLL> vpUnitDangerPlotList = pUnit->GetDangerPlotList();
	FFastVector<std::pair<CvPlot*, bool>, true, c_eCiv5GameplayDLL> vpUnitDangerPlotMoveOnlyList = pUnit->GetDangerPlotList(true);
	bool* pbInList = NULL;
	bool* pbInMoveOnlyList = NULL;
	pPlot->getNumTimesInList(vpUnitDangerPlotList, true, pbInList);
	pPlot->getNumTimesInList(vpUnitDangerPlotMoveOnlyList, true, pbInMoveOnlyList);
	if (pbInList || pbInMoveOnlyList)
	{
		if (pbInList && *pbInList && !bExcludeFromUnitsList)
		{
			m_DangerPlots[idx].m_apUnits.push_back(pUnit);
		}
		if (pbInMoveOnlyList && *pbInMoveOnlyList && !bExcludeFromMoveOnlyUnitsList)
		{
			m_DangerPlots[idx].m_apMoveOnlyUnits.push_back(pUnit);
		}
		return;
	}

	CvIgnoreUnitsPathFinder& kPathFinder = GC.getIgnoreUnitsPathFinder();
#ifdef AUI_ASTAR_TURN_LIMITER
	kPathFinder.SetData(pUnit, 1);
#else
	kPathFinder.SetData(pUnit);
#endif

	CvAStarNode* pNode = NULL;
	int iTurnsAway = MAX_INT;

	if (pUnit->IsCanAttackRanged())
	{
		if (!bExcludeFromMoveOnlyUnitsList)
		{
			if (kPathFinder.GeneratePath(pUnit->getX(), pUnit->getY(), iPlotX, iPlotY, MOVE_UNITS_IGNORE_DANGER | MOVE_IGNORE_STACKING, bReuse /*bReuse*/))
			{
				pNode = kPathFinder.GetLastNode();
				if (pNode)
				{
					iTurnsAway = pNode->m_iData2;
				}
			}
			if (iTurnsAway <= 1)
			{
				m_DangerPlots[idx].m_apMoveOnlyUnits.push_back(pUnit);
				vpUnitDangerPlotMoveOnlyList.push_back(std::make_pair(pPlot, true));
			}
			else
			{
				vpUnitDangerPlotMoveOnlyList.push_back(std::make_pair(pPlot, false));
			}
		}
		if (!bExcludeFromUnitsList)
		{
			if (pUnit->canMoveAndRangedStrike(pPlot))
			{
				m_DangerPlots[idx].m_apUnits.push_back(pUnit);
				vpUnitDangerPlotList.push_back(std::make_pair(pPlot, true));
			}
			else
			{
				vpUnitDangerPlotList.push_back(std::make_pair(pPlot, false));
			}
		}
	}
	else
	{
		if (kPathFinder.GeneratePath(pUnit->getX(), pUnit->getY(), iPlotX, iPlotY, MOVE_UNITS_IGNORE_DANGER | MOVE_IGNORE_STACKING, bReuse /*bReuse*/))
		{
			pNode = kPathFinder.GetLastNode();
			if (pNode)
			{
				iTurnsAway = pNode->m_iData2;
			}
		}
		if (iTurnsAway <= 1)
		{
			if (!bExcludeFromUnitsList)
			{
				m_DangerPlots[idx].m_apUnits.push_back(pUnit);
				vpUnitDangerPlotList.push_back(std::make_pair(pPlot, true));
			}
			if (!bExcludeFromMoveOnlyUnitsList)
			{
				m_DangerPlots[idx].m_apMoveOnlyUnits.push_back(pUnit);
				vpUnitDangerPlotMoveOnlyList.push_back(std::make_pair(pPlot, true));
			}
		}
		else
		{
			if (!bExcludeFromUnitsList)
				vpUnitDangerPlotList.push_back(std::make_pair(pPlot, false));
			if (!bExcludeFromMoveOnlyUnitsList)
				vpUnitDangerPlotMoveOnlyList.push_back(std::make_pair(pPlot, false));
		}
	}
#else
void CvDangerPlots::AssignUnitDangerValue(CvUnit* pUnit, CvPlot* pPlot)
{
	// MAJIK NUMBARS TO MOVE TO XML
	int iCombatValueCalc = 100;
	int iBaseUnitCombatValue = pUnit->GetBaseCombatStrengthConsideringDamage() * iCombatValueCalc;
	// Combat capable?  If not, the calculations will always result in 0, so just skip it.
	if(iBaseUnitCombatValue > 0)
	{
		// Will any danger be zero'ed out?
		if(!IsDangerByRelationshipZero(pUnit->getOwner(), pPlot))
		{
			//int iDistance = plotDistance(pUnitPlot->getX(), pUnitPlot->getY(), pPlot->getX(), pPlot->getY());
			//int iRange = pUnit->baseMoves();
			//FAssertMsg(iRange > 0, "0 range? Uh oh");

			CvIgnoreUnitsPathFinder& kPathFinder = GC.getIgnoreUnitsPathFinder();
			kPathFinder.SetData(pUnit);

			int iPlotX = pPlot->getX();
			int iPlotY = pPlot->getY();
			
#ifdef AUI_DANGER_PLOTS_TWEAKED_RANGED
			int iTurnsAway = MAX_INT;
			// if unit is ranged, different algorithm must be used
			if (pUnit->isRanged())
			{
#ifdef AUI_DANGER_PLOTS_ADD_DANGER_CONSIDER_TERRAIN_STRENGTH_MODIFICATION
				if (pUnit->plot() == pPlot)
					iBaseUnitCombatValue = pUnit->GetMaxDefenseStrength(pUnit->plot(), NULL) * iCombatValueCalc;
				else
					iBaseUnitCombatValue = pUnit->GetMaxRangedCombatStrength(NULL, NULL, true, true) * iCombatValueCalc;
#endif
#ifdef AUI_UNIT_CAN_MOVE_AND_RANGED_STRIKE
				if (pUnit->canMoveAndRangedStrike(pPlot))
				{
					iTurnsAway = 1;
				}
#else
				if (pUnit->canEverRangeStrikeAt(pPlot))
				{
					iTurnsAway = 1;
				}
#endif
			}
#ifdef AUI_DANGER_PLOTS_ADD_DANGER_CONSIDER_TERRAIN_STRENGTH_MODIFICATION
			else
			{
				if (pUnit->plot() == pPlot)
					iBaseUnitCombatValue = pUnit->GetMaxDefenseStrength(pUnit->plot(), NULL) * iCombatValueCalc;
				else if (pUnit->plot()->isAdjacent(pPlot))
					iBaseUnitCombatValue = pUnit->GetMaxAttackStrength(pUnit->plot(), pPlot, NULL) * iCombatValueCalc;
				else
					iBaseUnitCombatValue = pUnit->GetMaxAttackStrength(NULL, pPlot, NULL) * iCombatValueCalc;
			}
#endif

			// iTurnsAway is only greater than 1 if unit cannot ranged strike onto the tile
			if (iTurnsAway > 1)
			{
				CvAStarNode* pNode = NULL;
				// can the unit actually walk there
				if (kPathFinder.GeneratePath(pUnit->getX(), pUnit->getY(), iPlotX, iPlotY, MOVE_UNITS_IGNORE_DANGER, true /*bReuse*/))
				{
					pNode = kPathFinder.GetLastNode();
				}
				
				if (pNode)
					iTurnsAway = pNode->m_iData2;
				else
					return;
				if (pUnit->GetRange() > 1)
					iTurnsAway -= 1;
				iTurnsAway += pUnit->getMustSetUpToRangedAttackCount();
#ifdef AUI_FAST_COMP
				iTurnsAway = FASTMAX(iTurnsAway, 1);
#else
				iTurnsAway = MAX(iTurnsAway, 1);
#endif
			}
#else
			// can the unit actually walk there
			if(!kPathFinder.GeneratePath(pUnit->getX(), pUnit->getY(), iPlotX, iPlotY, 0, true /*bReuse*/))
			{
				return;
			}

			CvAStarNode* pNode = kPathFinder.GetLastNode();
			int iTurnsAway = pNode->m_iData2;
#ifdef AUI_FAST_COMP
			iTurnsAway = FASTMAX(iTurnsAway, 1);
#else
			iTurnsAway = max(iTurnsAway, 1);
#endif
#endif

			int iUnitCombatValue = iBaseUnitCombatValue / iTurnsAway;
			iUnitCombatValue = ModifyDangerByRelationship(pUnit->getOwner(), pPlot, iUnitCombatValue);
			AddDanger(iPlotX, iPlotY, iUnitCombatValue, iTurnsAway <= 1);
		}
	}
#endif
}

//	-----------------------------------------------------------------------------------------------
/// Contains the calculations to do the danger value for the plot according to the city
void CvDangerPlots::AssignCityDangerValue(CvCity* pCity, CvPlot* pPlot)
{
#ifdef AUI_DANGER_PLOTS_REMADE
	const int idx = GC.getMap().plotNum(pPlot->getX(), pPlot->getY());
	for (CvDangerPlotContents::DangerCityVector::iterator it = m_DangerPlots[idx].m_apCities.begin(); it != m_DangerPlots[idx].m_apCities.end(); ++it)
	{
		if ((*it) == pCity)
			return;
	}
	m_DangerPlots[idx].m_apCities.push_back(pCity);
#else
	int iCombatValue = pCity->getStrengthValue();
	iCombatValue = ModifyDangerByRelationship(pCity->getOwner(), pPlot, iCombatValue);
	AddDanger(pPlot->getX(), pPlot->getY(), iCombatValue, false);
#endif
}

#ifndef AUI_DANGER_PLOTS_REMADE
/// How much danger should we apply to a citadel?
int CvDangerPlots::GetDangerValueOfCitadel() const
{
	// Compute power of this player's strongest unit
	CvMilitaryAI* pMilitaryAI = GET_PLAYER(m_ePlayer).GetMilitaryAI();
	int iPower = pMilitaryAI->GetPowerOfStrongestBuildableUnit(DOMAIN_LAND);

	// Magic number to approximate danger from one turn of citadel damage
	return iPower * 50;
}
#endif

/// reads in danger plots info
void CvDangerPlots::Read(FDataStream& kStream)
{
	// Version number to maintain backwards compatibility
	uint uiVersion;
	kStream >> uiVersion;

	kStream >> m_ePlayer;
	kStream >> m_bArrayAllocated;

	int iGridSize;
	kStream >> iGridSize;

#ifdef AUI_DANGER_PLOTS_REMADE
	m_DangerPlots = FNEW(CvDangerPlotContents[iGridSize], c_eCiv5GameplayDLL, 0);
	if (uiVersion >= 10)
	{
		for (int i = 0; i < iGridSize; i++)
		{
			kStream >> m_DangerPlots[i];
		}
	}
	else
	{
		for (int i = 0; i < iGridSize; i++)
		{
			m_DangerPlots[i].init(GC.getMap().plotByIndexUnchecked(i));
		}
	}
#else
	m_DangerPlots.resize(iGridSize);
	for(int i = 0; i < iGridSize; i++)
	{
		kStream >> m_DangerPlots[i];
	}
#endif

	m_bDirty = false;
}

/// writes out danger plots info
void CvDangerPlots::Write(FDataStream& kStream) const
{
	// Current version number
#ifdef AUI_DANGER_PLOTS_REMADE
	uint uiVersion = AUI_VERSION;
#else
	uint uiVersion = 1;
#endif
	kStream << uiVersion;

	kStream << m_ePlayer;
	kStream << m_bArrayAllocated;

	int iGridSize = GC.getMap().getGridWidth() * GC.getMap().getGridHeight();
	kStream << iGridSize;
	for(int i = 0; i < iGridSize; i++)
	{
		kStream << m_DangerPlots[i];
	}
}

//	-----------------------------------------------------------------------------------------------
void CvDangerPlots::SetDirty()
{
	m_bDirty = true;
}

#ifdef AUI_DANGER_PLOTS_REMADE
// Get the maximum damage unit could receive at this plot in the next turn
int CvDangerPlotContents::GetDanger(PlayerTypes ePlayer)
{
	// Damage from terrain
	int iPlotDamage = m_iFlatPlotDamage;

	// Damage from units
	CvPlot* pAttackerPlot = NULL;
	CvUnit* pCurAttacker = NULL;
	for (DangerUnitVector::iterator it = m_apUnits.begin(); it < m_apUnits.end(); ++it)
	{
		pCurAttacker = (*it);
		if (!pCurAttacker || pCurAttacker->isDelayedDeath() || !pCurAttacker->plot())
		{
			continue;
		}

		pAttackerPlot = NULL;
#ifdef AUI_UNIT_EXTRA_IN_OTHER_PLOT_HELPERS
		if (pCurAttacker->IsCanAttackRanged())
		{
			if (pCurAttacker->getDomainType() == DOMAIN_AIR)
			{
				iPlotDamage += pCurAttacker->GetAirCombatDamage(NULL, NULL, false, 0, m_pPlot);
			}
			else
			{
				iPlotDamage += pCurAttacker->GetRangeCombatDamage(NULL, NULL, false, 0, m_pPlot);
			}
		}
		else
#endif
		{
			if (plotDistance(m_iX, m_iY, pCurAttacker->getX(), pCurAttacker->getY()) == 1)
			{
				pAttackerPlot = pCurAttacker->plot();
			}
			iPlotDamage += pCurAttacker->getCombatDamage(pCurAttacker->GetMaxAttackStrength(pAttackerPlot, m_pPlot, NULL),
				1, pCurAttacker->getDamage(), false, false, false);
#ifdef AUI_UNIT_EXTRA_IN_OTHER_PLOT_HELPERS
			if (pCurAttacker->isRangedSupportFire())
			{
				iPlotDamage += pCurAttacker->GetRangeCombatDamage(NULL, NULL, false, 0, m_pPlot, pAttackerPlot);
			}
#endif
		}
	}
	
	if (ePlayer != NO_PLAYER)
	{
#ifdef AUI_UNIT_EXTRA_IN_OTHER_PLOT_HELPERS
		// Damage from cities
		for (DangerCityVector::iterator it = m_apCities.begin(); it < m_apCities.end(); ++it)
		{
			if (*it && (*it)->getTeam() != GET_PLAYER(ePlayer).getTeam())
				iPlotDamage += (*it)->rangeCombatDamage(NULL, NULL, false, m_pPlot);
		}
#endif

		// Damage from improvements
		iPlotDamage += GetCitadelDamage(ePlayer);
	}

	return iPlotDamage;
}

// Get the maximum damage unit could receive at this plot in the next turn (update this with CvUnitCombat changes!)
int CvDangerPlotContents::GetDanger(const CvUnit* pUnit, const CvPlot* pAttackTarget, const int iAction, int iAfterNIntercepts)
{
	CvUnit* pCurAttacker = NULL;
	CvCity* pFriendlyCity = m_pPlot->getPlotCity();
	if (pFriendlyCity && !m_pPlot->isFriendlyCity(*pUnit, false))
		pFriendlyCity = NULL;
	int iPlotDamage = 0;
	if (m_iFlatPlotDamage != 0 && (pUnit->atPlot(*m_pPlot) || pUnit->canMoveInto(*m_pPlot)))
	{
		// Damage from plot (no unit in tile)
		iPlotDamage = m_iFlatPlotDamage;
	}

	// Air units only take damage from interceptions
	if (pUnit->getDomainType() == DOMAIN_AIR)
	{
		if (iAction & ACTION_AIR_INTERCEPT) // Max damage from a potential air sweep against our intercept
		{
			int iBestAirSweepDamage = 0;
			int iCurrentAirSweepDamage = 0;
			for (DangerUnitVector::iterator it = m_apUnits.begin(); it < m_apUnits.end(); ++it)
			{
				pCurAttacker = (*it);
				if (!pCurAttacker || !pCurAttacker->canAirSweep() || pCurAttacker->isDelayedDeath() || !pCurAttacker->plot())
				{
					continue;
				}
				int iAttackerStrength = pCurAttacker->GetMaxRangedCombatStrength(pUnit, /*pCity*/ NULL, true, false);
				iAttackerStrength *= (100 + pCurAttacker->GetAirSweepCombatModifier());
				iAttackerStrength /= 100;
				int iDefenderStrength = pUnit->GetMaxRangedCombatStrength(pUnit, /*pCity*/ NULL, false, false);
				iCurrentAirSweepDamage = pCurAttacker->getCombatDamage(iAttackerStrength, iDefenderStrength,
					pCurAttacker->getDamage(), /*bIncludeRand*/ false, /*bAttackerIsCity*/ false, /*bDefenderIsCity*/ false);

				// It's a slower to have this in the unit loop instead of after the best damage has been calculated, but it's also more accurate
				if (iCurrentAirSweepDamage >= pUnit->GetCurrHitPoints())
				{
					int iReceiverDamage = pUnit->getCombatDamage(iDefenderStrength, iAttackerStrength,
						pUnit->getDamage(), /*bIncludeRand*/ false, /*bAttackerIsCity*/ false, /*bDefenderIsCity*/ false);
					if (iReceiverDamage >= pCurAttacker->GetCurrHitPoints())
					{
						if (iReceiverDamage + pCurAttacker->getDamage() > iCurrentAirSweepDamage + pUnit->getDamage())
						{
							iCurrentAirSweepDamage = pUnit->GetCurrHitPoints() - 1;
						}
					}
				}

				if (iCurrentAirSweepDamage > iBestAirSweepDamage)
				{
					iBestAirSweepDamage = iCurrentAirSweepDamage;
				}
			}
			if (iBestAirSweepDamage < pUnit->GetCurrHitPoints())
				return iBestAirSweepDamage + GET_PLAYER(pUnit->getOwner()).GetPlotDanger(*pUnit->plot(), pUnit);
			else
				return iBestAirSweepDamage;
		}
		else if (iAction & (ACTION_AIR_SWEEP | ACTION_AIR_ATTACK))
		{
			iPlotDamage = 0;
			int iDangerFromUnitPlot = GET_PLAYER(pUnit->getOwner()).GetPlotDanger(*pUnit->plot(), pUnit);
#ifdef AUI_UNIT_GET_NTH_BEST_INTERCEPTOR
			CvUnit* pInterceptor = pUnit->GetNthBestInterceptor(*m_pPlot, iAfterNIntercepts);
#else
			CvUnit* pInterceptor = pUnit->GetBestInterceptor(*m_pPlot);
#endif
			if (pInterceptor)
			{
				// Air sweeps take modified damage from interceptors
				if (iAction & ACTION_AIR_SWEEP)
				{
					if (pInterceptor->getDomainType() != DOMAIN_AIR)
					{
						iPlotDamage = (pInterceptor->GetInterceptionDamage(pUnit, false) * GC.getAIR_SWEEP_INTERCEPTION_DAMAGE_MOD() / 100);
					}
					else
					{
						int iAttackerStrength = pUnit->GetMaxRangedCombatStrength(pInterceptor, /*pCity*/ NULL, true, false);
						iAttackerStrength *= (100 + pUnit->GetAirSweepCombatModifier());
						iAttackerStrength /= 100;
						int iDefenderStrength = pInterceptor->GetMaxRangedCombatStrength(pUnit, /*pCity*/ NULL, false, false);
						int iReceiveDamage = pInterceptor->getCombatDamage(iDefenderStrength, iAttackerStrength,
							pInterceptor->getDamage(), /*bIncludeRand*/ false, /*bAttackerIsCity*/ false, /*bDefenderIsCity*/ false);
						if (iReceiveDamage >= pUnit->GetCurrHitPoints())
						{
							int iDamageDealt = pUnit->getCombatDamage(iAttackerStrength, iDefenderStrength,
								pUnit->getDamage(), /*bIncludeRand*/ false, /*bAttackerIsCity*/ false, /*bDefenderIsCity*/ false);
							if (iDamageDealt >= pInterceptor->GetCurrHitPoints())
							{
								if (iDamageDealt + pInterceptor->getDamage() > iReceiveDamage + pUnit->getDamage())
								{
									iReceiveDamage = pUnit->GetCurrHitPoints() - 1;
								}
							}
						}
						iPlotDamage = iReceiveDamage;
					}
				}
				else if (iAction & ACTION_AIR_ATTACK && pUnit->evasionProbability() < 100)
				{
					// Always assume interception is successful
					iPlotDamage = pInterceptor->GetInterceptionDamage(pUnit, false) + iDangerFromUnitPlot;
				}
			}

			if (iPlotDamage < pUnit->GetCurrHitPoints())
				return iPlotDamage + iDangerFromUnitPlot;
			else
				return iPlotDamage;
		}
		else
		{
			bool bWillBeCapped = true;
			// If in a city and the city will survive all attack, return a danger value of 1
			if (pFriendlyCity)
			{
				if (GetDanger(pFriendlyCity) + pFriendlyCity->getDamage() < pFriendlyCity->GetMaxHitPoints())
				{
					// Trivial amount to indicate that the plot can still be attacked
					bWillBeCapped = false;
				}
			}
			// Look for a possible plot defender
			else
			{
				CvUnit* pBestDefender = NULL;
				IDInfo* pUnitNode = m_pPlot->headUnitNode();
				while (pUnitNode != NULL)
				{
					pBestDefender = getUnit(*pUnitNode);
					pUnitNode = m_pPlot->nextUnitNode(pUnitNode);
					if (pBestDefender && pBestDefender->getOwner() == pUnit->getOwner())
					{
						if (pBestDefender->domainCargo() == DOMAIN_AIR)
						{
							if (pBestDefender != pUnit)
							{
								if (GetDanger(pBestDefender) < pBestDefender->GetCurrHitPoints())
								{
									bWillBeCapped = false;
									break;
								}
							}
						}
					}
				}
			}
			// If Civilian would be captured on this tile (only happens if iPlotDamage isn't modified), return MAX_INT
			if (bWillBeCapped)
				return MAX_INT;
			else
			{
				// If a unit always heals and will survive, add healrate
				if ((pUnit->isAlwaysHeal() || iAction & ACTION_HEAL) && !pUnit->isBarbarian() && iPlotDamage < pUnit->GetCurrHitPoints())
				{
					iPlotDamage -= pUnit->healRate(m_pPlot);
					// Overheals are ignored
					if (pUnit->GetCurrHitPoints() > pUnit->GetMaxHitPoints() + iPlotDamage)
						iPlotDamage = pUnit->GetCurrHitPoints() - pUnit->GetMaxHitPoints();
				}
				// Damage from improvements
				iPlotDamage += GetCitadelDamage(pUnit);
				return iPlotDamage;
			}			
		}
	}

	// Civilians can be captured
	if (!pUnit->IsCombatUnit() && (!m_pPlot->isWater() || pUnit->getDomainType() != DOMAIN_LAND || m_pPlot->isValidDomainForAction(*pUnit)))
	{
		CvUnit* pBestDefender = NULL;
		for (DangerUnitVector::iterator it = m_apMoveOnlyUnits.begin(); it < m_apMoveOnlyUnits.end(); ++it)
		{
			pCurAttacker = (*it);
			if (pCurAttacker && !pCurAttacker->isDelayedDeath() && pCurAttacker->plot())
			{
				bool bWillBeCapped = true;
				// If in a city and the city will survive all attack, return a danger value of 1
				if (pFriendlyCity)
				{
					if (GetDanger(pFriendlyCity) + pFriendlyCity->getDamage() < pFriendlyCity->GetMaxHitPoints())
					{
						// Trivial amount to indicate that the plot can still be attacked
						bWillBeCapped = false;
					}
				}
				// Look for a possible plot defender
				else 
				{
					IDInfo* pUnitNode = m_pPlot->headUnitNode();
					while (pUnitNode != NULL)
					{
						pBestDefender = getUnit(*pUnitNode);
						pUnitNode = m_pPlot->nextUnitNode(pUnitNode);

						if (pBestDefender && pBestDefender->getOwner() == pUnit->getOwner())
						{
							if (pBestDefender->IsCanDefend())
							{
								if (pBestDefender != pUnit)
								{
									if (pBestDefender->isWaiting() || !(pBestDefender->canMove()))
									{
										if (GetDanger(pBestDefender) < pBestDefender->GetCurrHitPoints())
										{
											bWillBeCapped = false;
											break;
										}
									}
								}
							}
						}
						pBestDefender = NULL;
					}
				}
				// If Civilian would be captured on this tile (only happens if iPlotDamage isn't modified), return MAX_INT
				if (bWillBeCapped)
					return MAX_INT;
				else
					break;
			}
		}
		// Proceed as normal, skip other damage sources if there's a defender
		if (pFriendlyCity || pBestDefender)
		{
			// If a unit always heals and will survive, add healrate
			if ((pUnit->isAlwaysHeal() || iAction & ACTION_HEAL) && !pUnit->isBarbarian() && iPlotDamage < pUnit->GetCurrHitPoints() &&
				(!m_pPlot->isWater() || pUnit->getDomainType() != DOMAIN_LAND || m_pPlot->isValidDomainForAction(*pUnit)))
			{
				iPlotDamage -= pUnit->healRate(m_pPlot);
				// Overheals are ignored
				if (pUnit->GetCurrHitPoints() > pUnit->GetMaxHitPoints() + iPlotDamage)
					iPlotDamage = pUnit->GetCurrHitPoints() - pUnit->GetMaxHitPoints();
			}
			// Damage from improvements
			iPlotDamage += GetCitadelDamage(pUnit);
			return iPlotDamage;
		}
	}

	// Garrisoning in a city will have the city's health stats replace the unit's health stats (capturing a city with a garrisoned unit destroys the garrisoned unit)
	if (pFriendlyCity)
	{
		// If the city survives all possible attacks this turn, so will the unit
		if (GetDanger(pFriendlyCity, 0, NO_PLAYER, (pUnit->getDomainType() == DOMAIN_LAND ? pUnit : NULL)) + pFriendlyCity->getDamage() < pFriendlyCity->GetMaxHitPoints())
		{
			// If a unit always heals and will survive, add healrate
			if ((pUnit->isAlwaysHeal() || iAction & ACTION_HEAL) && !pUnit->isBarbarian() && iPlotDamage < pUnit->GetCurrHitPoints() &&
				(!m_pPlot->isWater() || pUnit->getDomainType() != DOMAIN_LAND || m_pPlot->isValidDomainForAction(*pUnit)))
			{
				iPlotDamage -= pUnit->healRate(m_pPlot);
				// Overheals are ignored
				if (pUnit->GetCurrHitPoints() > pUnit->GetMaxHitPoints() + iPlotDamage)
					iPlotDamage = pUnit->GetCurrHitPoints() - pUnit->GetMaxHitPoints();
			}
			// Damage from improvements
			iPlotDamage += GetCitadelDamage(pUnit);

			return iPlotDamage;
		}
		else
		{
			return MAX_INT;
		}
	}

	int iDamageDealt = 0;
	int iDamageReceivedFromAttack = 0;
	int iExtraFortifyTurns = (iAction & ACTION_HEAL) >> 3;
	const CvUnit* pAttackTargetUnit = NULL;
	const CvCity* pAttackTargetCity = NULL;
	if (pAttackTarget)
	{
		iExtraFortifyTurns = -pUnit->getFortifyTurns();
		pAttackTargetCity = pAttackTarget->getPlotCity();
		if (!pAttackTargetCity)
			pAttackTargetUnit = pAttackTarget->getVisibleEnemyDefender(pUnit);
	}

	if (pAttackTargetCity)
	{
		if (!pUnit->IsCanAttackRanged())
		{
			int iAttackerStrength = pUnit->GetMaxAttackStrength(m_pPlot, pAttackTargetCity->plot(), NULL);
			int iDefenderStrength = pAttackTargetCity->getStrengthValue();

			iDamageDealt = pUnit->getCombatDamage(iAttackerStrength, iDefenderStrength, pUnit->getDamage(), /*bIncludeRand*/ false, /*bAttackerIsCity*/ false, /*bDefenderIsCity*/ true);
			iDamageReceivedFromAttack = pUnit->getCombatDamage(iDefenderStrength, iAttackerStrength, pAttackTargetCity->getDamage(), /*bIncludeRand*/ false, /*bAttackerIsCity*/ true, /*bDefenderIsCity*/ false);

			// Will both the attacker die, and the city fall? If so, the unit wins
			if (iDamageDealt + pAttackTargetCity->getDamage() >= pAttackTargetCity->GetMaxHitPoints())
			{
				if (iDamageReceivedFromAttack >= pUnit->GetCurrHitPoints())
					iDamageReceivedFromAttack = pUnit->GetCurrHitPoints() - 1;
				if (!pUnit->isNoCapture())
				{
					int iNewCityDamage = pAttackTargetCity->GetMaxHitPoints() - 1;

					int iBattleDamgeThreshold = pAttackTargetCity->GetMaxHitPoints() * /*50*/ GC.getCITY_CAPTURE_DAMAGE_PERCENT();
					iBattleDamgeThreshold /= 100;

					if (iNewCityDamage > iBattleDamgeThreshold)
					{
						iNewCityDamage = iBattleDamgeThreshold;
					}
					
					if (GET_PLAYER(pUnit->getOwner()).GetPlotDanger(*pAttackTarget, pAttackTargetCity, 0, (PlayerTypes)pUnit->getOwner(), pUnit, iDamageReceivedFromAttack))
					{
						// If a unit always heals and will survive, add healrate
						if ((pUnit->isAlwaysHeal() || iAction & ACTION_HEAL) && !pUnit->isBarbarian() &&
							(!pAttackTarget->isWater() || pUnit->getDomainType() != DOMAIN_LAND || pAttackTarget->isValidDomainForAction(*pUnit)))
						{
							iDamageReceivedFromAttack -= pUnit->healRate(pAttackTarget);
							// Overheals are ignored
							if (pUnit->GetCurrHitPoints() > pUnit->GetMaxHitPoints() + iDamageReceivedFromAttack)
								iDamageReceivedFromAttack = pUnit->GetCurrHitPoints() - pUnit->GetMaxHitPoints();
						}
						// Damage from improvements
						iDamageReceivedFromAttack += GET_PLAYER(pUnit->getOwner()).GetPlotDangerFromCitadel(*pAttackTarget, (PlayerTypes)pUnit->getOwner());

						return iPlotDamage;
					}
					else
					{
						return MAX_INT;
					}
				}
			}

			iPlotDamage += iDamageReceivedFromAttack;
		}
	}
	else if (pAttackTargetUnit)
	{
		if (pUnit->IsCanAttackRanged())
		{
#ifdef AUI_UNIT_EXTRA_IN_OTHER_PLOT_HELPERS
			iDamageDealt = pUnit->GetRangeCombatDamage(pAttackTargetUnit, NULL, false, 0, NULL, m_pPlot);
#else
			iDamageDealt = pUnit->GetRangeCombatDamage(pAttackTargetUnit, NULL, false, 0);
#endif
		}
		else
		{
			if (pUnit->isRangedSupportFire())
			{
#ifdef AUI_UNIT_EXTRA_IN_OTHER_PLOT_HELPERS
				iDamageDealt = pUnit->GetRangeCombatDamage(pAttackTargetUnit, NULL, false, 0, pAttackTargetUnit->plot(), m_pPlot);
#else
				iDamageDealt = pUnit->GetRangeCombatDamage(pAttackTargetUnit, NULL, false);
#endif
			}
			if (iDamageDealt < pAttackTargetUnit->GetCurrHitPoints())
			{
				int iAttackerStrength = pUnit->GetMaxAttackStrength(m_pPlot, pAttackTargetUnit->plot(), pAttackTargetUnit);
				if (pUnit->IsCanHeavyCharge() && !pAttackTargetUnit->CanFallBackFromMelee(*pUnit))
					iAttackerStrength = (iAttackerStrength * 150) / 100;
				int iDefenderStrength = pAttackTargetUnit->GetMaxDefenseStrength(pAttackTargetUnit->plot(), pUnit);

				iDamageReceivedFromAttack = pAttackTargetUnit->getCombatDamage(iDefenderStrength, iAttackerStrength, pAttackTargetUnit->getDamage() + iDamageDealt, /*bIncludeRand*/ false, /*bAttackerIsCity*/ false, /*bDefenderIsCity*/ false);
				iDamageDealt += pUnit->getCombatDamage(iAttackerStrength, iDefenderStrength, pUnit->getDamage(), false, false, false);

				// Will both units be killed by this? :o If so, take drastic corrective measures
				if (iDamageDealt >= pAttackTargetUnit->GetCurrHitPoints() && iDamageReceivedFromAttack >= pUnit->GetCurrHitPoints())
				{
					// He who hath the least amount of damage survives with 1 HP left
					if (iDamageDealt + pAttackTargetUnit->getDamage() > iDamageReceivedFromAttack + pUnit->getDamage())
					{
						iDamageReceivedFromAttack = pUnit->GetCurrHitPoints() - 1;
					}
					else
					{
						iDamageDealt = pAttackTargetUnit->GetCurrHitPoints() - 1;
					}
				}
			}
		}
		if (iDamageDealt >= pAttackTargetUnit->GetCurrHitPoints())
		{
			if (pUnit->getHPHealedIfDefeatEnemy() > 0 && (!pUnit->IsHealIfDefeatExcludeBarbarians() || !pAttackTargetUnit->isBarbarian()))
			{
				iDamageReceivedFromAttack -= FASTMIN(pUnit->getHPHealedIfDefeatEnemy(), pUnit->getDamage() + iDamageReceivedFromAttack);
			}
		}
		if (!pUnit->IsCanAttackRanged() && !(pUnit->IsCaptureDefeatedEnemy() && pUnit->AreUnitsOfSameType(*pAttackTargetUnit)))
		{
			// We're advancing to the attacked plot, so return the danger of that plot
			if (iDamageDealt >= pAttackTargetUnit->GetCurrHitPoints() || 
				(pUnit->IsCanHeavyCharge() && pAttackTargetUnit->CanFallBackFromMelee(*pUnit) && iDamageDealt > iDamageReceivedFromAttack))
			{
				// TODO: Make sure currently retreating unit is not ignored for danger when it doesn't die before the advance, eg. for Winged Hussar attackers
				return GET_PLAYER(pUnit->getOwner()).GetPlotDanger(*pAttackTarget, pUnit) + iDamageReceivedFromAttack;
			}
		}

		iPlotDamage += iDamageReceivedFromAttack;
	}

	FFastVector<const CvPlot*, true, c_eCiv5GameplayDLL> vpPossibleAttackPlots;
	FFastVector<const CvPlot*, true, c_eCiv5GameplayDLL> vpUsedAttackPlots;
	vpPossibleAttackPlots.reserve(m_apUnits.size());
	vpUsedAttackPlots.reserve(m_apUnits.size());
	const CvPlot* pAttackerPlot = NULL;
	CvUnit* pInterceptor = NULL;
	// Damage from units
	// TODO: Implement support for withdrawing
	for (DangerUnitVector::iterator it = m_apUnits.begin(); it < m_apUnits.end(); ++it)
	{
		pCurAttacker = (*it);
		if (!pCurAttacker || pCurAttacker->isDelayedDeath() || !pCurAttacker->plot())
		{
			continue;
		}

		pAttackerPlot = NULL;
		if (pCurAttacker->plot() != m_pPlot)
		{				
			if (pCurAttacker->IsCanAttackRanged())
			{
				if (pCurAttacker->getDomainType() == DOMAIN_AIR)
				{
#ifdef AUI_UNIT_GET_NTH_BEST_INTERCEPTOR
					pInterceptor = pCurAttacker->GetNthBestInterceptor(*m_pPlot, iAfterNIntercepts, pUnit);
#else
					pInterceptor = pCurAttacker->GetBestInterceptor(*m_pPlot, pUnit);
#endif
					int iInterceptDamage = 0;
					if (pInterceptor)
					{
						// Always assume interception is successful
						iInterceptDamage = pInterceptor->GetInterceptionDamage(pCurAttacker, false);
						++iAfterNIntercepts;
					}
#ifdef AUI_UNIT_EXTRA_IN_OTHER_PLOT_HELPERS
					iPlotDamage += pCurAttacker->GetAirCombatDamage(pUnit, NULL, false, iInterceptDamage, m_pPlot, NULL, iExtraFortifyTurns);
#else
					iPlotDamage += pCurAttacker->GetAirCombatDamage(pUnit, NULL, false, iInterceptDamage);
#endif
				}
				else
				{
					vpPossibleAttackPlots.clear();
					if (pCurAttacker->GetMovablePlotListOpt(vpPossibleAttackPlots, m_pPlot, false, 0, NULL, &vpUsedAttackPlots))
						pAttackerPlot = pCurAttacker->getBestMovablePlot(vpPossibleAttackPlots, m_pPlot);
					else
						continue;
					vpUsedAttackPlots.push_back(pAttackerPlot);

					if (pCurAttacker == pAttackTargetUnit)
					{
						if (iDamageDealt < pCurAttacker->GetCurrHitPoints())
						{
#ifdef AUI_UNIT_EXTRA_IN_OTHER_PLOT_HELPERS
							iPlotDamage += pCurAttacker->GetRangeCombatDamage(pUnit, NULL, false, iDamageDealt, m_pPlot, pAttackerPlot, iExtraFortifyTurns);
#else
							iPlotDamage += pCurAttacker->GetRangeCombatDamage(pUnit, NULL, false, iDamageDealt);
#endif
						}
					}
					else
					{
#ifdef AUI_UNIT_EXTRA_IN_OTHER_PLOT_HELPERS
						iPlotDamage += pCurAttacker->GetRangeCombatDamage(pUnit, NULL, false, 0, m_pPlot, pAttackerPlot, iExtraFortifyTurns);
#else
						iPlotDamage += pCurAttacker->GetRangeCombatDamage(pUnit, NULL, false);
#endif
					}
				}
			}
			else if (!(iAction & ACTION_NO_MELEE))
			{
				vpPossibleAttackPlots.clear();
				if (pCurAttacker->GetMovablePlotListOpt(vpPossibleAttackPlots, m_pPlot, false, 0, NULL, &vpUsedAttackPlots))
					pAttackerPlot = pCurAttacker->getBestMovablePlot(vpPossibleAttackPlots, m_pPlot);
				else
					continue;
				vpUsedAttackPlots.push_back(pAttackerPlot);

				if (pCurAttacker == pAttackTargetUnit)
				{
					if (iDamageDealt < pCurAttacker->GetCurrHitPoints())
					{
#ifdef AUI_UNIT_EXTRA_IN_OTHER_PLOT_HELPERS
						iPlotDamage += pCurAttacker->getCombatDamage(pCurAttacker->GetMaxAttackStrength(pAttackerPlot, m_pPlot, pUnit, iExtraFortifyTurns),
							pUnit->GetMaxDefenseStrength(m_pPlot, pCurAttacker, false, iExtraFortifyTurns), pCurAttacker->getDamage() + iDamageDealt, false, false, false);
#else
						iPlotDamage += pCurAttacker->getCombatDamage(pCurAttacker->GetMaxAttackStrength(pAttackerPlot, m_pPlot, pUnit),
							pUnit->GetMaxDefenseStrength(m_pPlot, pCurAttacker), pCurAttacker->getDamage() + iDamageDealt, false, false, false);
#endif
						if (pCurAttacker->isRangedSupportFire())
						{
#ifdef AUI_UNIT_EXTRA_IN_OTHER_PLOT_HELPERS
							iPlotDamage += pCurAttacker->GetRangeCombatDamage(pUnit, NULL, false, iDamageDealt, m_pPlot, pAttackerPlot, iExtraFortifyTurns);
#else
							iPlotDamage += pCurAttacker->GetRangeCombatDamage(pUnit, NULL, false, iDamageDealt);
#endif
						}
					}
				}
				else
				{
#ifdef AUI_UNIT_EXTRA_IN_OTHER_PLOT_HELPERS
					int iAttackerStrength = pCurAttacker->GetMaxAttackStrength(pAttackerPlot, m_pPlot, pUnit, iExtraFortifyTurns);
					int iDefenderStrength = pUnit->GetMaxDefenseStrength(m_pPlot, pCurAttacker, false, iExtraFortifyTurns);
#else
					int iAttackerStrength = pCurAttacker->GetMaxAttackStrength(pAttackerPlot, m_pPlot, pUnit);
					int iDefenderStrength = pUnit->GetMaxDefenseStrength(m_pPlot, pCurAttacker, false);
#endif
					if (pCurAttacker->IsCanHeavyCharge() && !pUnit->CanFallBackFromMelee(*pCurAttacker))
					{
						iAttackerStrength = (iAttackerStrength * 150) / 100;
					}
					iPlotDamage += pCurAttacker->getCombatDamage(iAttackerStrength, iDefenderStrength, pCurAttacker->getDamage() + iDamageDealt, false, false, false);
					if (pCurAttacker->isRangedSupportFire())
					{
#ifdef AUI_UNIT_EXTRA_IN_OTHER_PLOT_HELPERS
						iPlotDamage += pCurAttacker->GetRangeCombatDamage(pUnit, NULL, false, 0, m_pPlot, pAttackerPlot, iExtraFortifyTurns);
#else
						iPlotDamage += pCurAttacker->GetRangeCombatDamage(pUnit, NULL, false);
#endif
					}
				}
			}
		}
	}

	// Damage from cities
	for (DangerCityVector::iterator it = m_apCities.begin(); it < m_apCities.end(); ++it)
	{
		if (!(*it) || (*it)->getTeam() == pUnit->getTeam())
		{
			continue;
		}

#ifdef AUI_UNIT_EXTRA_IN_OTHER_PLOT_HELPERS
		iPlotDamage += (*it)->rangeCombatDamage(pUnit, NULL, false, m_pPlot, iExtraFortifyTurns);
#else
		iPlotDamage += (*it)->rangeCombatDamage(pUnit, NULL, false);
#endif
	}

	// If a unit always heals and will survive, add healrate
	if ((pUnit->isAlwaysHeal() || iAction & ACTION_HEAL) && !pUnit->isBarbarian() && iPlotDamage < pUnit->GetCurrHitPoints() &&
		(!m_pPlot->isWater() || pUnit->getDomainType() != DOMAIN_LAND || m_pPlot->isValidDomainForAction(*pUnit)))
	{
		iPlotDamage -= pUnit->healRate(m_pPlot);
		// Overheals are ignored
		if (pUnit->GetCurrHitPoints() > pUnit->GetMaxHitPoints() + iPlotDamage)
			iPlotDamage = pUnit->GetCurrHitPoints() - pUnit->GetMaxHitPoints();
	}

	// Damage from improvements
	iPlotDamage += GetCitadelDamage(pUnit);

	return iPlotDamage;
}

// Can this tile be attacked by an enemy unit or city next turn?
bool CvDangerPlotContents::IsUnderImmediateThreat(PlayerTypes ePlayer)
{
	// Terrain damage
	if (m_iFlatPlotDamage > 0)
	{
		return true;
	}

	// Units in range
	for (DangerUnitVector::iterator it = m_apUnits.begin(); it < m_apUnits.end(); ++it)
	{
		if (*it && !(*it)->isDelayedDeath() && (*it)->plot())
		{
			return true;
		}
	}

	if (ePlayer != NO_PLAYER)
	{
		// Cities in range
		for (DangerCityVector::iterator it = m_apCities.begin(); it < m_apCities.end(); ++it)
		{
			if (*it && (*it)->getTeam() != GET_PLAYER(ePlayer).getTeam())
			{
				return true;
			}
		}

		// Citadel in range
		if (GetCitadelDamage(ePlayer) > 0)
		{
			return true;
		}
	}

	return false;
}

// Can this tile be attacked by an enemy unit or city next turn?
bool CvDangerPlotContents::IsUnderImmediateThreat(const CvUnit* pUnit)
{
	// Air units operate off of intercepts instead of units/cities that can attack them
	if (pUnit->getDomainType() == DOMAIN_AIR)
	{
		if (pUnit->GetBestInterceptor(*m_pPlot))
		{
			return true;
		}
	}
	else
	{
		// Cities in range
		for (DangerCityVector::iterator it = m_apCities.begin(); it < m_apCities.end(); ++it)
		{
			if (*it && (*it)->getTeam() != pUnit->getTeam())
			{
				return true;
			}
		}

		// Units in range
		if (pUnit->IsCombatUnit())
		{
			for (DangerUnitVector::iterator it = m_apUnits.begin(); it < m_apUnits.end(); ++it)
			{
				if (*it && !(*it)->isDelayedDeath() && (*it)->plot())
				{
					return true;
				}
			}
		}
		else
		{
			for (DangerUnitVector::iterator it = m_apMoveOnlyUnits.begin(); it < m_apMoveOnlyUnits.end(); ++it)
			{
				if (*it && !(*it)->isDelayedDeath() && (*it)->plot())
				{
					return true;
				}
			}
		}

		// Citadel in range
		if (GetCitadelDamage(pUnit) > 0)
		{
			return true;
		}
	}

	// Terrain damage is greater than heal rate
	int iMinimumDamage = m_iFlatPlotDamage;
	if ((pUnit->plot() == m_pPlot && !pUnit->isEmbarked()) ||
		(pUnit->isAlwaysHeal() && (!m_pPlot->isWater() || pUnit->getDomainType() != DOMAIN_LAND || m_pPlot->isValidDomainForAction(*pUnit))))
	{
		iMinimumDamage -= pUnit->healRate(m_pPlot);
	}
	if (iMinimumDamage > 0)
	{
		return true;
	}

	return false;
}

bool CvDangerPlotContents::CouldAttackHere(const CvUnit* pAttacker)
{
	for (DangerUnitVector::iterator it = m_apUnits.begin(); it < m_apUnits.end(); ++it)
	{
		if (*it == pAttacker)
		{
			if ((m_pPlot->getPlotCity() && GET_TEAM(pAttacker->getTeam()).isAtWar(m_pPlot->getPlotCity()->getTeam())) ||
				m_pPlot->getBestDefender(NO_PLAYER, pAttacker->getOwner(), pAttacker, true))
			{
				return true;
			}
			return false;
		}
	}
	return false;
}

bool CvDangerPlotContents::CouldAttackHere(const CvCity* pAttacker)
{
	for (DangerCityVector::iterator it = m_apCities.begin(); it < m_apCities.end(); ++it)
	{
		if (*it == pAttacker)
		{
			if ((m_pPlot->getPlotCity() && GET_TEAM(pAttacker->getTeam()).isAtWar(m_pPlot->getPlotCity()->getTeam())) ||
				m_pPlot->getBestDefender(NO_PLAYER, pAttacker->getOwner(), NULL, true))
			{
				return true;
			}
			return false;
		}
	}
	return false;
}

// Get the maximum damage city could receive this turn if it were in this plot
int CvDangerPlotContents::GetDanger(const CvCity* pCity, int iAfterNIntercepts, PlayerTypes ePretendCityOwner, const CvUnit* pPretendGarrison, int iPretendGarrisonExtraDamage)
{
	int iPlotDamage = 0;
	CvPlot* pCityPlot = pCity->plot();
	const int iCityX = pCityPlot->getX();
	const int iCityY = pCityPlot->getY();
	const int iMaxNoCaptureDamage = pCity->GetMaxHitPoints() - pCity->getDamage() - 1;

	CvPlot* pAttackerPlot = NULL;
	CvUnit* pInterceptor = NULL;
	CvUnit* pCurAttacker = NULL;
	// Damage from ranged units and melees that cannot capture 
	for (DangerUnitVector::iterator it = m_apUnits.begin(); it < m_apUnits.end() && iPlotDamage < iMaxNoCaptureDamage; ++it)
	{
		pCurAttacker = (*it);
		if (!pCurAttacker || pCurAttacker->isDelayedDeath() || !pCurAttacker->plot())
		{
			continue;
		}

		pAttackerPlot = NULL;
		
		if (pCurAttacker->IsCanAttackRanged())
		{
			if (pCurAttacker->getDomainType() == DOMAIN_AIR)
			{
#ifdef AUI_UNIT_GET_NTH_BEST_INTERCEPTOR
				pInterceptor = pCurAttacker->GetNthBestInterceptor(*m_pPlot, iAfterNIntercepts);
#else
				pInterceptor = pCurAttacker->GetBestInterceptor(*m_pPlot);
#endif
				int iInterceptDamage = 0;
				if (pInterceptor)
				{
					// Always assume interception is successful
					iInterceptDamage = pInterceptor->GetInterceptionDamage(pCurAttacker, false);
					++iAfterNIntercepts;
				}
#ifdef AUI_UNIT_EXTRA_IN_OTHER_PLOT_HELPERS
				iPlotDamage += pCurAttacker->GetAirCombatDamage(NULL, pCity, false, iInterceptDamage, m_pPlot, NULL, 0, ePretendCityOwner, pPretendGarrison, iPretendGarrisonExtraDamage);
#else
				iPlotDamage += pCurAttacker->GetAirCombatDamage(NULL, pCity, false, iInterceptDamage);
#endif
			}
			else
			{
#ifdef AUI_UNIT_EXTRA_IN_OTHER_PLOT_HELPERS
				iPlotDamage += pCurAttacker->GetRangeCombatDamage(NULL, pCity, false, 0, m_pPlot, NULL, 0, ePretendCityOwner, pPretendGarrison, iPretendGarrisonExtraDamage);
#else
				iPlotDamage += pCurAttacker->GetRangeCombatDamage(NULL, pCity, false);
#endif
			}
		}
		else if (pCurAttacker->isNoCapture())
		{
			if (plotDistance(iCityX, iCityY, pCurAttacker->getX(), pCurAttacker->getY()) == 1)
			{
				pAttackerPlot = pCurAttacker->plot();
			}
			iPlotDamage += pCurAttacker->getCombatDamage(pCurAttacker->GetMaxAttackStrength(pAttackerPlot, pCityPlot, NULL),
				pCity->getStrengthValue(false, ePretendCityOwner, pPretendGarrison, iPretendGarrisonExtraDamage), pCurAttacker->getDamage(), false, false, true);
		}
	}

	// Damage from cities
	for (DangerCityVector::iterator it = m_apCities.begin(); it < m_apCities.end() && iPlotDamage < iMaxNoCaptureDamage; ++it)
	{
		if (!(*it) || (*it) == pCity || (*it)->getTeam() == (ePretendCityOwner == NO_PLAYER ? pCity->getTeam() : GET_PLAYER(ePretendCityOwner).getTeam()))
		{
			continue;
		}

#ifdef AUI_UNIT_EXTRA_IN_OTHER_PLOT_HELPERS
		iPlotDamage += (*it)->rangeCombatDamage(NULL, pCity, false, pCityPlot);
#else
		iPlotDamage += (*it)->rangeCombatDamage(NULL, pCity, false);
#endif
	}

	if (iPlotDamage > iMaxNoCaptureDamage)
		iPlotDamage = iMaxNoCaptureDamage;

	// Damage from melee units
	for (DangerUnitVector::iterator it = m_apMoveOnlyUnits.begin(); it < m_apMoveOnlyUnits.end(); ++it)
	{
		pCurAttacker = (*it);
		if (!pCurAttacker || pCurAttacker->isDelayedDeath() || pCurAttacker->IsDead() || !pCurAttacker->plot())
		{
			continue;
		}

		pAttackerPlot = NULL;

		if (!pCurAttacker->IsCanAttackRanged() && !pCurAttacker->isNoCapture())
		{
			if (plotDistance(iCityX, iCityY, pCurAttacker->getX(), pCurAttacker->getY()) == 1)
			{
				pAttackerPlot = pCurAttacker->plot();
			}
			iPlotDamage += pCurAttacker->getCombatDamage(pCurAttacker->GetMaxAttackStrength(pAttackerPlot, pCityPlot, NULL),
				pCity->getStrengthValue(false, ePretendCityOwner, pPretendGarrison, iPretendGarrisonExtraDamage), pCurAttacker->getDamage(), false, false, true);

			if (pCurAttacker->isRangedSupportFire() && iPlotDamage < iMaxNoCaptureDamage)
			{
#ifdef AUI_UNIT_EXTRA_IN_OTHER_PLOT_HELPERS
				iPlotDamage += pCurAttacker->GetRangeCombatDamage(NULL, pCity, false, 0, pCityPlot, NULL, 0, ePretendCityOwner, pPretendGarrison, iPretendGarrisonExtraDamage);
#else
				iPlotDamage += pCurAttacker->GetRangeCombatDamage(NULL, pCity, false);
#endif
			}
		}
	}

	return iPlotDamage;
}

// Get the amount of damage a citadel would deal to a unit
int CvDangerPlotContents::GetCitadelDamage(PlayerTypes ePlayer) const
{
	if (m_pCitadel && ePlayer != NO_PLAYER)
	{
		ImprovementTypes eImprovement = m_pCitadel->getImprovementType();
		CvTeam& kTeam = GET_TEAM(GET_PLAYER(ePlayer).getTeam());

		// Citadel still here and can fire?
		if (eImprovement != NO_IMPROVEMENT && !m_pCitadel->IsImprovementPillaged() && m_pCitadel->getOwner() != NO_PLAYER &&
			kTeam.isAtWar(m_pCitadel->getTeam()))
		{
			return GC.getImprovementInfo(eImprovement)->GetNearbyEnemyDamage();
		}
		else
		{
			int iCitadelRange = 1;
			CvPlot* pLoopPlot;

			ImprovementTypes eImprovement;
			int iDamage;

			for (int iDY = -iCitadelRange; iDY <= iCitadelRange; iDY++)
			{
#ifdef AUI_FAST_COMP
				int iMaxDX = iCitadelRange - FASTMAX(0, iDY);
				for (int iDX = -iCitadelRange - FASTMIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#else
				iMaxDX = iCitadelRange - MAX(0, iDY);
				for (iDX = -iCitadelRange - MIN(0, iDY); iX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#endif
				{
					// No need for range check because loops are set up properly
					pLoopPlot = plotXY(m_pCitadel->getX(), m_pCitadel->getY(), iDX, iDY);
					if (pLoopPlot != NULL && pLoopPlot->getOwner() != NO_PLAYER)
					{
						eImprovement = pLoopPlot->getImprovementType();

						// Citadel here?
						if (eImprovement != NO_IMPROVEMENT && !pLoopPlot->IsImprovementPillaged())
						{
							iDamage = GC.getImprovementInfo(eImprovement)->GetNearbyEnemyDamage();
							if (iDamage != 0)
							{
								if (kTeam.isAtWar(pLoopPlot->getTeam()))
								{
									return iDamage;
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;
};
#endif
