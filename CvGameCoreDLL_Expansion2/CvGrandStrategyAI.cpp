/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#include "CvGameCoreDLLPCH.h"
#include "CvGrandStrategyAI.h"
#include "CvEconomicAI.h"
#include "CvCitySpecializationAI.h"
#include "CvDiplomacyAI.h"
#include "CvMinorCivAI.h"
#include "ICvDLLUserInterface.h"

#ifdef AUI_GS_PRIORITY_OVERHAUL
#include "CvGameCoreUtils.h"
#endif

#ifdef AUI_GS_SPACESHIP_TECH_RATIO
#include "CvTechAI.h"
#endif

// must be included after all other headers
#include "LintFree.h"

//------------------------------------------------------------------------------
CvAIGrandStrategyXMLEntry::CvAIGrandStrategyXMLEntry(void):
	m_piFlavorValue(NULL),
	m_piSpecializationBoost(NULL),
	m_piFlavorModValue(NULL)
{
}
//------------------------------------------------------------------------------
CvAIGrandStrategyXMLEntry::~CvAIGrandStrategyXMLEntry(void)
{
	SAFE_DELETE_ARRAY(m_piFlavorValue);
	SAFE_DELETE_ARRAY(m_piSpecializationBoost);
	SAFE_DELETE_ARRAY(m_piFlavorModValue);
}
//------------------------------------------------------------------------------
bool CvAIGrandStrategyXMLEntry::CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility)
{
	if(!CvBaseInfo::CacheResults(kResults, kUtility))
		return false;

	//Arrays
	const char* szType = GetType();
	kUtility.SetFlavors(m_piFlavorValue, "AIGrandStrategy_Flavors", "AIGrandStrategyType", szType);
	kUtility.SetYields(m_piSpecializationBoost, "AIGrandStrategy_Yields", "AIGrandStrategyType", szType);
	kUtility.SetFlavors(m_piFlavorModValue, "AIGrandStrategy_FlavorMods", "AIGrandStrategyType", szType);

	return true;
}

/// What Flavors will be added by adopting this Grand Strategy?
int CvAIGrandStrategyXMLEntry::GetFlavorValue(int i) const
{
	FAssertMsg(i < GC.getNumFlavorTypes(), "Index out of bounds");
	FAssertMsg(i > -1, "Index out of bounds");
	return m_piFlavorValue ? m_piFlavorValue[i] : -1;
}

/// What Flavors will be added by adopting this Grand Strategy?
int CvAIGrandStrategyXMLEntry::GetSpecializationBoost(YieldTypes eYield) const
{
	FAssertMsg(eYield < NUM_YIELD_TYPES, "Index out of bounds");
	FAssertMsg(eYield > -1, "Index out of bounds");
	return m_piSpecializationBoost ? m_piSpecializationBoost[(int)eYield] : 0;
}

/// What Flavors will be added by adopting this Grand Strategy?
int CvAIGrandStrategyXMLEntry::GetFlavorModValue(int i) const
{
	FAssertMsg(i < GC.getNumFlavorTypes(), "Index out of bounds");
	FAssertMsg(i > -1, "Index out of bounds");
	return m_piFlavorModValue ? m_piFlavorModValue[i] : 0;
}



//=====================================
// CvAIGrandStrategyXMLEntries
//=====================================
/// Constructor
CvAIGrandStrategyXMLEntries::CvAIGrandStrategyXMLEntries(void)
{

}

/// Destructor
CvAIGrandStrategyXMLEntries::~CvAIGrandStrategyXMLEntries(void)
{
	DeleteArray();
}

/// Returns vector of AIStrategy entries
std::vector<CvAIGrandStrategyXMLEntry*>& CvAIGrandStrategyXMLEntries::GetAIGrandStrategyEntries()
{
	return m_paAIGrandStrategyEntries;
}

/// Number of defined AIStrategies
int CvAIGrandStrategyXMLEntries::GetNumAIGrandStrategies()
{
	return m_paAIGrandStrategyEntries.size();
}

/// Clear AIStrategy entries
void CvAIGrandStrategyXMLEntries::DeleteArray()
{
	for(std::vector<CvAIGrandStrategyXMLEntry*>::iterator it = m_paAIGrandStrategyEntries.begin(); it != m_paAIGrandStrategyEntries.end(); ++it)
	{
		SAFE_DELETE(*it);
	}

	m_paAIGrandStrategyEntries.clear();
}

/// Get a specific entry
CvAIGrandStrategyXMLEntry* CvAIGrandStrategyXMLEntries::GetEntry(int index)
{
	return m_paAIGrandStrategyEntries[index];
}



//=====================================
// CvGrandStrategyAI
//=====================================
/// Constructor
CvGrandStrategyAI::CvGrandStrategyAI():
	m_paiGrandStrategyPriority(NULL),
	m_eGuessOtherPlayerActiveGrandStrategy(NULL),
	m_eGuessOtherPlayerActiveGrandStrategyConfidence(NULL)
{
}

/// Destructor
CvGrandStrategyAI::~CvGrandStrategyAI(void)
{
}

/// Initialize
void CvGrandStrategyAI::Init(CvAIGrandStrategyXMLEntries* pAIGrandStrategies, CvPlayer* pPlayer)
{
	// Store off the pointer to the AIStrategies active for this game
	m_pAIGrandStrategies = pAIGrandStrategies;

	m_pPlayer = pPlayer;

	// Initialize AIGrandStrategy status array
	FAssertMsg(m_paiGrandStrategyPriority==NULL, "about to leak memory, CvGrandStrategyAI::m_paiGrandStrategyPriority");
	m_paiGrandStrategyPriority = FNEW(int[m_pAIGrandStrategies->GetNumAIGrandStrategies()], c_eCiv5GameplayDLL, 0);

	FAssertMsg(m_eGuessOtherPlayerActiveGrandStrategy==NULL, "about to leak memory, CvGrandStrategyAI::m_eGuessOtherPlayerActiveGrandStrategy");
	m_eGuessOtherPlayerActiveGrandStrategy = FNEW(int[MAX_MAJOR_CIVS], c_eCiv5GameplayDLL, 0);

	FAssertMsg(m_eGuessOtherPlayerActiveGrandStrategyConfidence==NULL, "about to leak memory, CvGrandStrategyAI::m_eGuessOtherPlayerActiveGrandStrategyConfidence");
	m_eGuessOtherPlayerActiveGrandStrategyConfidence = FNEW(int[MAX_MAJOR_CIVS], c_eCiv5GameplayDLL, 0);

	Reset();
}

/// Deallocate memory created in initialize
void CvGrandStrategyAI::Uninit()
{
	SAFE_DELETE_ARRAY(m_paiGrandStrategyPriority);
	SAFE_DELETE_ARRAY(m_eGuessOtherPlayerActiveGrandStrategy);
	SAFE_DELETE_ARRAY(m_eGuessOtherPlayerActiveGrandStrategyConfidence);
}

/// Reset AIStrategy status array to all false
void CvGrandStrategyAI::Reset()
{
	int iI;

	m_iNumTurnsSinceActiveSet = 0;

	m_eActiveGrandStrategy = NO_AIGRANDSTRATEGY;

	for(iI = 0; iI < m_pAIGrandStrategies->GetNumAIGrandStrategies(); iI++)
	{
		m_paiGrandStrategyPriority[iI] = -1;
	}

	for(iI = 0; iI < MAX_MAJOR_CIVS; iI++)
	{
		m_eGuessOtherPlayerActiveGrandStrategy[iI] = NO_AIGRANDSTRATEGY;
		m_eGuessOtherPlayerActiveGrandStrategyConfidence[iI] = NO_GUESS_CONFIDENCE_TYPE;
	}
}

/// Serialization read
void CvGrandStrategyAI::Read(FDataStream& kStream)
{
	// Version number to maintain backwards compatibility
	uint uiVersion;
	kStream >> uiVersion;

	kStream >> m_iNumTurnsSinceActiveSet;
	kStream >> (int&)m_eActiveGrandStrategy;

	FAssertMsg(m_pAIGrandStrategies != NULL && m_pAIGrandStrategies->GetNumAIGrandStrategies() > 0, "Number of AIGrandStrategies to serialize is expected to greater than 0");
#ifdef _MSC_VER
// JAR - if m_pAIGrandStrategies can be NULL at this point,
// the load will fail if the data isn't read. Better to crash
// here where the problem is than defer it.
#pragma warning ( push )
#pragma warning ( disable : 6011 )
#endif//_MSC_VER
	ArrayWrapper<int> wrapm_paiGrandStrategyPriority(m_pAIGrandStrategies->GetNumAIGrandStrategies(), m_paiGrandStrategyPriority);
#ifdef _MSC_VER
#pragma warning ( pop )
#endif//_MSC_VER

	kStream >> wrapm_paiGrandStrategyPriority;

	ArrayWrapper<int> wrapm_eGuessOtherPlayerActiveGrandStrategy(MAX_MAJOR_CIVS, m_eGuessOtherPlayerActiveGrandStrategy);
	kStream >> wrapm_eGuessOtherPlayerActiveGrandStrategy;

	ArrayWrapper<int> wrapm_eGuessOtherPlayerActiveGrandStrategyConfidence(MAX_MAJOR_CIVS, m_eGuessOtherPlayerActiveGrandStrategyConfidence);
	kStream >> wrapm_eGuessOtherPlayerActiveGrandStrategyConfidence;

}

/// Serialization write
void CvGrandStrategyAI::Write(FDataStream& kStream)
{
	// Current version number
	uint uiVersion = 1;
	kStream << uiVersion;

	kStream << m_iNumTurnsSinceActiveSet;
	kStream << m_eActiveGrandStrategy;

	FAssertMsg(GC.getNumAIGrandStrategyInfos() > 0, "Number of AIStrategies to serialize is expected to greater than 0");
	kStream << ArrayWrapper<int>(m_pAIGrandStrategies->GetNumAIGrandStrategies(), m_paiGrandStrategyPriority);

	kStream << ArrayWrapper<int>(MAX_MAJOR_CIVS, m_eGuessOtherPlayerActiveGrandStrategy);
	kStream << ArrayWrapper<int>(MAX_MAJOR_CIVS, m_eGuessOtherPlayerActiveGrandStrategyConfidence);
}

/// Returns the Player object the Strategies are associated with
CvPlayer* CvGrandStrategyAI::GetPlayer()
{
	return m_pPlayer;
}

/// Returns AIGrandStrategies object stored in this class
CvAIGrandStrategyXMLEntries* CvGrandStrategyAI::GetAIGrandStrategies()
{
	return m_pAIGrandStrategies;
}

/// Runs every turn to determine what the player's Active Grand Strategy is and to change Priority Levels as necessary
void CvGrandStrategyAI::DoTurn()
{
	DoGuessOtherPlayersActiveGrandStrategy();

	int iGrandStrategiesLoop;
	AIGrandStrategyTypes eGrandStrategy;
	CvAIGrandStrategyXMLEntry* pGrandStrategy;
	CvString strGrandStrategyName;

	// Loop through all GrandStrategies to set their Priorities
	for(iGrandStrategiesLoop = 0; iGrandStrategiesLoop < GetAIGrandStrategies()->GetNumAIGrandStrategies(); iGrandStrategiesLoop++)
	{
		eGrandStrategy = (AIGrandStrategyTypes) iGrandStrategiesLoop;
		pGrandStrategy = GetAIGrandStrategies()->GetEntry(iGrandStrategiesLoop);
		strGrandStrategyName = (CvString) pGrandStrategy->GetType();

#ifdef AUI_GS_PRIORITY_OVERHAUL
		// Base Priority gives 1000 and drops off to 0 over the course of the game; it depends only on civ flavor
		// Dropoff curve depends on flavor, with minimum flavor giving no curve (linear), and higher flavor prolonging the drop
		double dPriority = GetBaseGrandStrategyPriority(eGrandStrategy);

		// ETA Priority starts out at 0 and slowly rises to up to 1000 over the course of the game; it depends only on in-game factors
		// The rise curve is the inverse of base priority's dropoff, giving higher flavor civs a longer chance to get the GS they desire
		if(strGrandStrategyName == "AIGRANDSTRATEGY_CONQUEST")
		{
			dPriority += GetConquestETAPriority();
		}
		else if(strGrandStrategyName == "AIGRANDSTRATEGY_CULTURE")
		{
			dPriority += GetCultureETAPriority();
		}
		else if(strGrandStrategyName == "AIGRANDSTRATEGY_UNITED_NATIONS")
		{
			dPriority += GetUnitedNationsETAPriority();
		}
		else if(strGrandStrategyName == "AIGRANDSTRATEGY_SPACESHIP")
		{
			dPriority += GetSpaceshipETAPriority();
		}
#else
		// Base Priority looks at Personality Flavors (0 - 10) and multiplies * the Flavors attached to a Grand Strategy (0-10),
		// so expect a number between 0 and 100 back from this
		int iPriority = GetBaseGrandStrategyPriority(eGrandStrategy);

		if(strGrandStrategyName == "AIGRANDSTRATEGY_CONQUEST")
		{
			iPriority += GetConquestPriority();
		}
		else if(strGrandStrategyName == "AIGRANDSTRATEGY_CULTURE")
		{
			iPriority += GetCulturePriority();
		}
		else if(strGrandStrategyName == "AIGRANDSTRATEGY_UNITED_NATIONS")
		{
			iPriority += GetUnitedNationsPriority();
		}
		else if(strGrandStrategyName == "AIGRANDSTRATEGY_SPACESHIP")
		{
			iPriority += GetSpaceshipPriority();
		}

		// Random element
#ifdef AUI_BINOM_RNG
		iPriority += GC.getGame().getJonRandNumBinom(/*50*/ GC.getAI_GS_RAND_ROLL(), "Grand Strategy AI: GS rand roll.");
#else
		iPriority += GC.getGame().getJonRandNum(/*50*/ GC.getAI_GS_RAND_ROLL(), "Grand Strategy AI: GS rand roll.");
#endif

		// Give a boost to the current strategy so that small fluctuation doesn't cause a big change
		if(GetActiveGrandStrategy() == eGrandStrategy && GetActiveGrandStrategy() != NO_AIGRANDSTRATEGY)
		{
			iPriority += /*50*/ GC.getAI_GRAND_STRATEGY_CURRENT_STRATEGY_WEIGHT();
		}
#endif

#ifdef AUI_GS_PRIORITY_OVERHAUL
		SetGrandStrategyPriority(eGrandStrategy, int(fPriority + 0.5f));
#else
		SetGrandStrategyPriority(eGrandStrategy, iPriority);
#endif //AUI_GS_PRIORITY_OVERHAUL
	}

	// Now look at what we think the other players in the game are up to - we might have an opportunity to capitalize somewhere
	int iNumPlayersAliveAndMet = 0;

	int iMajorLoop;

	for(iMajorLoop = 0; iMajorLoop < MAX_MAJOR_CIVS; iMajorLoop++)
	{
		if(GET_PLAYER((PlayerTypes) iMajorLoop).isAlive())
		{
			if(GET_TEAM(GetPlayer()->getTeam()).isHasMet(GET_PLAYER((PlayerTypes) iMajorLoop).getTeam()))
			{
				iNumPlayersAliveAndMet++;
			}
		}
	}

	FStaticVector< int, 5, true, c_eCiv5GameplayDLL > viNumGrandStrategiesAdopted;
	int iNumPlayers;

	// Init vector
	for(iGrandStrategiesLoop = 0; iGrandStrategiesLoop < GetAIGrandStrategies()->GetNumAIGrandStrategies(); iGrandStrategiesLoop++)
	{
		iNumPlayers = 0;

		// Tally up how many players we think are pusuing each Grand Strategy
		for(iMajorLoop = 0; iMajorLoop < MAX_MAJOR_CIVS; iMajorLoop++)
		{
			if(GetGuessOtherPlayerActiveGrandStrategy((PlayerTypes) iMajorLoop) == (AIGrandStrategyTypes) iGrandStrategiesLoop)
			{
				iNumPlayers++;
			}
		}

		viNumGrandStrategiesAdopted.push_back(iNumPlayers);
	}

	FStaticVector< int, 5, true, c_eCiv5GameplayDLL > viGrandStrategyChangeForLogging;

	int iChange;

	// Now modify our preferences based on how many people are going for stuff
	for(iGrandStrategiesLoop = 0; iGrandStrategiesLoop < GetAIGrandStrategies()->GetNumAIGrandStrategies(); iGrandStrategiesLoop++)
	{
		eGrandStrategy = (AIGrandStrategyTypes) iGrandStrategiesLoop;
		// If EVERYONE else we know is also going for this Grand Strategy, reduce our Priority by 50%
		iChange = GetGrandStrategyPriority(eGrandStrategy) * /*50*/ GC.getAI_GRAND_STRATEGY_OTHER_PLAYERS_GS_MULTIPLIER();
		iChange = iChange * viNumGrandStrategiesAdopted[eGrandStrategy] / iNumPlayersAliveAndMet;
		iChange /= 100;

#ifndef AUI_GS_PRIORITY_OVERHAUL
		ChangeGrandStrategyPriority(eGrandStrategy, -iChange);
#endif

		viGrandStrategyChangeForLogging.push_back(-iChange);
	}

	ChangeNumTurnsSinceActiveSet(1);

	// Now see which Grand Strategy should be active, based on who has the highest Priority right now
	// Grand Strategy must be run for at least 10 turns
	if(GetActiveGrandStrategy() == NO_AIGRANDSTRATEGY || GetNumTurnsSinceActiveSet() >= /*10*/ GC.getAI_GRAND_STRATEGY_NUM_TURNS_STRATEGY_MUST_BE_ACTIVE())
	{
		int iBestPriority = -1;
		int iPriority;

		AIGrandStrategyTypes eBestGrandStrategy = NO_AIGRANDSTRATEGY;

		for(iGrandStrategiesLoop = 0; iGrandStrategiesLoop < GetAIGrandStrategies()->GetNumAIGrandStrategies(); iGrandStrategiesLoop++)
		{
			eGrandStrategy = (AIGrandStrategyTypes) iGrandStrategiesLoop;

			iPriority = GetGrandStrategyPriority(eGrandStrategy);

			if(iPriority > iBestPriority)
			{
				iBestPriority = iPriority;
				eBestGrandStrategy = eGrandStrategy;
			}
		}

		if(eBestGrandStrategy != GetActiveGrandStrategy())
		{
			SetActiveGrandStrategy(eBestGrandStrategy);
			m_pPlayer->GetCitySpecializationAI()->SetSpecializationsDirty(SPECIALIZATION_UPDATE_NEW_GRAND_STRATEGY);
		}
	}

	LogGrandStrategies(viGrandStrategyChangeForLogging);
}

#ifdef AUI_PUBLIC_HAS_MET_MAJOR
/// True if one other major has been met, False otherwise
bool CvGrandStrategyAI::HasMetMajor()
{
	bool bHasMetMajor = false;
	CvTeam& pTeam = GET_TEAM(GetPlayer()->getTeam());

	for (int iTeamLoop = 0; iTeamLoop < MAX_CIV_TEAMS; iTeamLoop++)
	{
		if (pTeam.GetID() != iTeamLoop && !GET_TEAM((TeamTypes)iTeamLoop).isMinorCiv())
		{
			if (pTeam.isHasMet((TeamTypes)iTeamLoop))
			{
				bHasMetMajor = true;
				break;
			}
		}
	}

	return bHasMetMajor;
}
#endif

/// Returns Priority for Conquest Grand Strategy
int CvGrandStrategyAI::GetConquestPriority()
{
#ifdef AUI_GS_USE_DOUBLES
	double dPriority = 0;

	// If Conquest Victory isn't even available then don't bother with anything
	VictoryTypes eVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_DOMINATION", true);
	if(eVictory == NO_VICTORY || !GC.getGame().isVictoryValid(eVictory))
	{
		if(!GC.getGame().areNoVictoriesValid())
		{
			return -100;
		}
	}

	int iGeneralWarlikeness = GetPlayer()->GetDiplomacyAI()->GetPersonalityMajorCivApproachBias(MAJOR_CIV_APPROACH_WAR);
	int iGeneralHostility = GetPlayer()->GetDiplomacyAI()->GetPersonalityMajorCivApproachBias(MAJOR_CIV_APPROACH_HOSTILE);
	int iGeneralDeceptiveness = GetPlayer()->GetDiplomacyAI()->GetPersonalityMajorCivApproachBias(MAJOR_CIV_APPROACH_DECEPTIVE);
	int iGeneralFriendliness = GetPlayer()->GetDiplomacyAI()->GetPersonalityMajorCivApproachBias(MAJOR_CIV_APPROACH_FRIENDLY);

	int iGeneralApproachModifier = max(max(iGeneralDeceptiveness, iGeneralHostility),iGeneralWarlikeness) - iGeneralFriendliness;
	// Boldness gives the base weight for Conquest (no flavors added earlier)
#ifdef AUI_GS_CONQUEST_TWEAKED_ERAS
	dPriority += (GetPlayer()->GetDiplomacyAI()->GetBoldness() + iGeneralApproachModifier) * (12 - sqrt(m_pPlayer->GetCurrentEra() + 1.0)); // make a little less likely as time goes on
#else
	dPriority += ((GetPlayer()->GetDiplomacyAI()->GetBoldness() + iGeneralApproachModifier) * (12 - m_pPlayer->GetCurrentEra())); // make a little less likely as time goes on
#endif

	CvTeam& pTeam = GET_TEAM(GetPlayer()->getTeam());

	// How many turns must have passed before we test for having met nobody?
	if(GC.getGame().getElapsedGameTurns() >= /*20*/ GC.getAI_GS_CONQUEST_NOBODY_MET_FIRST_TURN())
	{
		// If we haven't met any Major Civs yet, then we probably shouldn't be planning on conquering the world
#ifdef AUI_PUBLIC_HAS_MET_MAJOR
		if(!HasMetMajor())
#else
		bool bHasMetMajor = false;

		for (int iTeamLoop = 0; iTeamLoop < MAX_CIV_TEAMS; iTeamLoop++)
		{
			if (pTeam.GetID() != iTeamLoop && !GET_TEAM((TeamTypes)iTeamLoop).isMinorCiv())
			{
				if (pTeam.isHasMet((TeamTypes)iTeamLoop))
				{
					bHasMetMajor = true;
					break;
				}
			}
		}
		if (!bHasMetMajor)
#endif
		{
			dPriority += /*-50*/ GC.getAI_GRAND_STRATEGY_CONQUEST_NOBODY_MET_WEIGHT();
		}
	}

	// How many turns must have passed before we test for us having a weak military?
	if(GC.getGame().getElapsedGameTurns() >= /*60*/ GC.getAI_GS_CONQUEST_MILITARY_STRENGTH_FIRST_TURN())
	{
		// Compare our military strength to the rest of the world
		int iWorldMilitaryStrength = GC.getGame().GetWorldMilitaryStrengthAverage(GetPlayer()->GetID(), true, true);

		if(iWorldMilitaryStrength > 0)
		{
#ifdef AUI_GS_CONQUEST_TWEAKED_MILITARY_RATIO
			const int iRatioAbs = (GetPlayer()->GetMilitaryMight() - iWorldMilitaryStrength) < 0 ? -1 : 1;
			double dMilitaryRatio = iRatioAbs * pow((GetPlayer()->GetMilitaryMight() - iWorldMilitaryStrength) / (double)iWorldMilitaryStrength, 2.0)
				* /*100*/ GC.getAI_GRAND_STRATEGY_CONQUEST_POWER_RATIO_MULTIPLIER();

			// Make the likelihood of BECOMING a warmonger lower than dropping the bad behavior
			if(dMilitaryRatio > 0)
				dMilitaryRatio /= M_SQRT3;
#else
			double dMilitaryRatio = (GetPlayer()->GetMilitaryMight() - iWorldMilitaryStrength) * /*100*/ GC.getAI_GRAND_STRATEGY_CONQUEST_POWER_RATIO_MULTIPLIER() / (double)iWorldMilitaryStrength;

			// Make the likelihood of BECOMING a warmonger lower than dropping the bad behavior
			if (dMilitaryRatio > 0)
				dMilitaryRatio /= 2;
#endif

			dPriority += dMilitaryRatio;	// This will add between -100 and 100 depending on this player's MilitaryStrength relative the world average. The number will typically be near 0 though, as it's fairly hard to get away from the world average
		}
	}

	// If we're at war, then boost the weight a bit
	if(pTeam.getAtWarCount(/*bIgnoreMinors*/ false) > 0)
	{
		dPriority += /*10*/ GC.getAI_GRAND_STRATEGY_CONQUEST_AT_WAR_WEIGHT();
	}

	// If our neighbors are cramping our style, consider less... scrupulous means of obtaining more land
	if(GetPlayer()->IsCramped())
	{
		PlayerTypes ePlayer;
		int iNumPlayersMet = 1;	// Include 1 for me!
		int iTotalLandMe = 0;
		int iTotalLandPlayersMet = 0;

		// Count the number of Majors we know
		for(int iMajorLoop = 0; iMajorLoop < MAX_MAJOR_CIVS; iMajorLoop++)
		{
			ePlayer = (PlayerTypes) iMajorLoop;

			if(GET_PLAYER(ePlayer).isAlive() && iMajorLoop != GetPlayer()->GetID())
			{
				if(pTeam.isHasMet(GET_PLAYER(ePlayer).getTeam()))
				{
					iNumPlayersMet++;
				}
			}
		}

		if(iNumPlayersMet > 0)
		{
			// Check every plot for ownership
			for(int iPlotLoop = 0; iPlotLoop < GC.getMap().numPlots(); iPlotLoop++)
			{
				if(GC.getMap().plotByIndexUnchecked(iPlotLoop)->isOwned())
				{
					ePlayer = GC.getMap().plotByIndexUnchecked(iPlotLoop)->getOwner();

					if(ePlayer == GetPlayer()->GetID())
					{
						iTotalLandPlayersMet++;
						iTotalLandMe++;
					}
					else if (ePlayer != NO_PLAYER && !GET_PLAYER(ePlayer).isMinorCiv() && pTeam.isHasMet(GET_PLAYER(ePlayer).getTeam()))
					{
						iTotalLandPlayersMet++;
					}
				}
			}

			if(iTotalLandMe > 0)
			{
#ifdef AUI_GS_CONQUEST_FIX_CRAMPED
				if (iTotalLandPlayersMet / (double)iNumPlayersMet / (double)iTotalLandMe > 1)
#else
				if (iTotalLandPlayersMet / (double)iNumPlayersMet / (double)iTotalLandMe > 0)
#endif
				{
					dPriority += /*20*/ GC.getAI_GRAND_STRATEGY_CONQUEST_CRAMPED_WEIGHT();
				}
			}
		}
	}

#ifndef AUI_GS_CONQUEST_IGNORE_ENEMY_NUKES
	// if we do not have nukes and we know someone else who does...
	if(GetPlayer()->getNumNukeUnits() == 0)
	{
		for(int iMajorLoop = 0; iMajorLoop < MAX_MAJOR_CIVS; iMajorLoop++)
		{
			PlayerTypes ePlayer = (PlayerTypes) iMajorLoop;

			if(GET_PLAYER(ePlayer).isAlive() && iMajorLoop != GetPlayer()->GetID())
			{
				if(pTeam.isHasMet(GET_PLAYER(ePlayer).getTeam()))
				{
					if (GET_PLAYER(ePlayer).getNumNukeUnits() > 0)
					{
						dPriority -= 50;
						break;
					}
				}
			}
		}
	}
#endif

	return (int)(dPriority + 0.5);
#else
	int iPriority = 0;

	// If Conquest Victory isn't even available then don't bother with anything
	VictoryTypes eVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_DOMINATION", true);
	if(eVictory == NO_VICTORY || !GC.getGame().isVictoryValid(eVictory))
	{
		if(!GC.getGame().areNoVictoriesValid())
		{
			return -100;
		}
	}

	int iGeneralWarlikeness = GetPlayer()->GetDiplomacyAI()->GetPersonalityMajorCivApproachBias(MAJOR_CIV_APPROACH_WAR);
	int iGeneralHostility = GetPlayer()->GetDiplomacyAI()->GetPersonalityMajorCivApproachBias(MAJOR_CIV_APPROACH_HOSTILE);
	int iGeneralDeceptiveness = GetPlayer()->GetDiplomacyAI()->GetPersonalityMajorCivApproachBias(MAJOR_CIV_APPROACH_DECEPTIVE);
	int iGeneralFriendliness = GetPlayer()->GetDiplomacyAI()->GetPersonalityMajorCivApproachBias(MAJOR_CIV_APPROACH_FRIENDLY);

	int iGeneralApproachModifier = max(max(iGeneralDeceptiveness, iGeneralHostility),iGeneralWarlikeness) - iGeneralFriendliness;
	// Boldness gives the base weight for Conquest (no flavors added earlier)
	iPriority += ((GetPlayer()->GetDiplomacyAI()->GetBoldness() + iGeneralApproachModifier) * (12 - m_pPlayer->GetCurrentEra())); // make a little less likely as time goes on

	CvTeam& pTeam = GET_TEAM(GetPlayer()->getTeam());

	// How many turns must have passed before we test for having met nobody?
	if(GC.getGame().getElapsedGameTurns() >= /*20*/ GC.getAI_GS_CONQUEST_NOBODY_MET_FIRST_TURN())
	{
		// If we haven't met any Major Civs yet, then we probably shouldn't be planning on conquering the world
		bool bHasMetMajor = false;

		for(int iTeamLoop = 0; iTeamLoop < MAX_CIV_TEAMS; iTeamLoop++)
		{
			if(pTeam.GetID() != iTeamLoop && !GET_TEAM((TeamTypes) iTeamLoop).isMinorCiv())
			{
				if(pTeam.isHasMet((TeamTypes) iTeamLoop))
				{
					bHasMetMajor = true;
					break;
				}
			}
		}
		if(!bHasMetMajor)
		{
			iPriority += /*-50*/ GC.getAI_GRAND_STRATEGY_CONQUEST_NOBODY_MET_WEIGHT();
		}
	}

	// How many turns must have passed before we test for us having a weak military?
	if(GC.getGame().getElapsedGameTurns() >= /*60*/ GC.getAI_GS_CONQUEST_MILITARY_STRENGTH_FIRST_TURN())
	{
		// Compare our military strength to the rest of the world
		int iWorldMilitaryStrength = GC.getGame().GetWorldMilitaryStrengthAverage(GetPlayer()->GetID(), true, true);

		if(iWorldMilitaryStrength > 0)
		{
			int iMilitaryRatio = (GetPlayer()->GetMilitaryMight() - iWorldMilitaryStrength) * /*100*/ GC.getAI_GRAND_STRATEGY_CONQUEST_POWER_RATIO_MULTIPLIER() / iWorldMilitaryStrength;

			// Make the likelihood of BECOMING a warmonger lower than dropping the bad behavior
			if(iMilitaryRatio > 0)
				iMilitaryRatio /= 2;

			iPriority += iMilitaryRatio;	// This will add between -100 and 100 depending on this player's MilitaryStrength relative the world average. The number will typically be near 0 though, as it's fairly hard to get away from the world average
		}
	}

	// If we're at war, then boost the weight a bit
	if(pTeam.getAtWarCount(/*bIgnoreMinors*/ false) > 0)
	{
		iPriority += /*10*/ GC.getAI_GRAND_STRATEGY_CONQUEST_AT_WAR_WEIGHT();
	}

	// If our neighbors are cramping our style, consider less... scrupulous means of obtaining more land
	if(GetPlayer()->IsCramped())
	{
		PlayerTypes ePlayer;
		int iNumPlayersMet = 1;	// Include 1 for me!
		int iTotalLandMe = 0;
		int iTotalLandPlayersMet = 0;

		// Count the number of Majors we know
		for(int iMajorLoop = 0; iMajorLoop < MAX_MAJOR_CIVS; iMajorLoop++)
		{
			ePlayer = (PlayerTypes) iMajorLoop;

			if(GET_PLAYER(ePlayer).isAlive() && iMajorLoop != GetPlayer()->GetID())
			{
				if(pTeam.isHasMet(GET_PLAYER(ePlayer).getTeam()))
				{
					iNumPlayersMet++;
				}
			}
		}

		if(iNumPlayersMet > 0)
		{
			// Check every plot for ownership
			for(int iPlotLoop = 0; iPlotLoop < GC.getMap().numPlots(); iPlotLoop++)
			{
				if(GC.getMap().plotByIndexUnchecked(iPlotLoop)->isOwned())
				{
					ePlayer = GC.getMap().plotByIndexUnchecked(iPlotLoop)->getOwner();

					if(ePlayer == GetPlayer()->GetID())
					{
						iTotalLandPlayersMet++;
						iTotalLandMe++;
					}
					else if(!GET_PLAYER(ePlayer).isMinorCiv() && pTeam.isHasMet(GET_PLAYER(ePlayer).getTeam()))
					{
						iTotalLandPlayersMet++;
					}
				}
			}

			iTotalLandPlayersMet /= iNumPlayersMet;

			if(iTotalLandMe > 0)
			{
				if(iTotalLandPlayersMet / iTotalLandMe > 0)
				{
					iPriority += /*20*/ GC.getAI_GRAND_STRATEGY_CONQUEST_CRAMPED_WEIGHT();
				}
			}
		}
	}

	// if we do not have nukes and we know someone else who does...
	if(GetPlayer()->getNumNukeUnits() == 0)
	{
		for(int iMajorLoop = 0; iMajorLoop < MAX_MAJOR_CIVS; iMajorLoop++)
		{
			PlayerTypes ePlayer = (PlayerTypes) iMajorLoop;

			if(GET_PLAYER(ePlayer).isAlive() && iMajorLoop != GetPlayer()->GetID())
			{
				if(pTeam.isHasMet(GET_PLAYER(ePlayer).getTeam()))
				{
					if (GET_PLAYER(ePlayer).getNumNukeUnits() > 0)
					{
						iPriority -= 50; 
						break;
					}
				}
			}
		}
	}

	return iPriority;
#endif
}

/// Returns Priority for Culture Grand Strategy
int CvGrandStrategyAI::GetCulturePriority()
{
#ifdef AUI_GS_USE_DOUBLES
	double dPriority = 0;

	// If Culture Victory isn't even available then don't bother with anything
	VictoryTypes eVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_CULTURAL", true);
	if(eVictory == NO_VICTORY || !GC.getGame().isVictoryValid(eVictory))
	{
		return -100;
	}

	// Before tourism kicks in, add weight based on flavor
	int iFlavorCulture =  m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_CULTURE"));
#ifdef AUI_GS_CULTURE_TWEAKED_ERAS
	dPriority += (11 - sqrt(m_pPlayer->GetCurrentEra() + 1.0)) * 2.0 * iFlavorCulture;
#else
	dPriority += (10 - m_pPlayer->GetCurrentEra()) * iFlavorCulture * 200.0 / 100.0;
#endif

#ifdef AUI_GS_CULTURE_TWEAKED_CULTURE_TOURISM_AHEAD
	// Loop through Players to see how we are doing on Tourism and Culture
	PlayerTypes eLoopPlayer;
	int iOurCulture = m_pPlayer->GetTotalJONSCulturePerTurn();
	int iOurTotalCulture = m_pPlayer->GetCulture()->GetLastTurnLifetimeCulture();
	int iOurTourism = m_pPlayer->GetCulture()->GetTourism();
	int iHighestCulture = iOurCulture;
	int iHighestTotalCulture = iOurTotalCulture;
	int iSecondHighestTotalCulture = iOurTotalCulture;
	PlayerTypes eHighestTotalCulturePlayer = m_pPlayer->GetID();
	PlayerTypes eSecondHighestTotalCulturePlayer = m_pPlayer->GetID();
	int iHighestTourism = iOurTourism;
	PlayerTypes eHighestTourismPlayer = m_pPlayer->GetID();

	for(int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
	{
		eLoopPlayer = (PlayerTypes) iPlayerLoop;
		CvPlayer &kPlayer = GET_PLAYER(eLoopPlayer);

		if (kPlayer.isAlive() && !kPlayer.isMinorCiv() && !kPlayer.isBarbarian() && iPlayerLoop != m_pPlayer->GetID())
		{
			if (kPlayer.GetTotalJONSCulturePerTurn() > iHighestCulture)
			{
				iHighestCulture = kPlayer.GetTotalJONSCulturePerTurn();
			}

			if (kPlayer.GetCulture()->GetLastTurnLifetimeCulture() > iHighestTotalCulture && kPlayer.GetCulture()->GetLastTurnLifetimeCulture() > iSecondHighestTotalCulture)
			{
				iSecondHighestTotalCulture = iHighestTotalCulture;
				iHighestTotalCulture = kPlayer.GetCulture()->GetLastTurnLifetimeCulture();
				eSecondHighestTotalCulturePlayer = eHighestTotalCulturePlayer;
				eHighestTotalCulturePlayer = kPlayer.GetID();
			}
			else if (kPlayer.GetCulture()->GetLastTurnLifetimeCulture() > iSecondHighestTotalCulture)
			{
				iSecondHighestTotalCulture = kPlayer.GetCulture()->GetLastTurnLifetimeCulture();
				eSecondHighestTotalCulturePlayer = kPlayer.GetID();
			}
			
			if (kPlayer.GetCulture()->GetTourism() > iHighestTourism)
			{
				iHighestTourism = kPlayer.GetCulture()->GetTourism();
				eHighestTourismPlayer = kPlayer.GetID();
			}
		}
	}
	
	// now based on actual culture values instead of how many civs we are ahead of culture-wise
	dPriority += GC.getAI_GS_CULTURE_AHEAD_WEIGHT() * ((double)iOurCulture / (double)iHighestCulture - 0.5) * 2.0;

	// since this is used for culture victory ETA, the highest total culture player must be different form highest tourism player
	const bool bHighestTourismNeedsSecond = eHighestTotalCulturePlayer == eHighestTourismPlayer;
	double dFastestVictoryETA;
	double dOurVictoryETA;
	if (!bHighestTourismNeedsSecond)
	{
		dFastestVictoryETA = (double)iHighestTotalCulture / (double)MAX(iHighestTourism - GET_PLAYER(eHighestTotalCulturePlayer).GetTotalJONSCulturePerTurn(), 1);
	}
	else
	{
		dFastestVictoryETA = (double)iSecondHighestTotalCulture / (double)MAX(iHighestTourism - GET_PLAYER(eSecondHighestTotalCulturePlayer).GetTotalJONSCulturePerTurn(), 1);
	}
	if (eHighestTotalCulturePlayer != m_pPlayer->GetID())
	{
		dOurVictoryETA = (double)iHighestTotalCulture / (double)MAX(iOurTourism - GET_PLAYER(eHighestTotalCulturePlayer).GetTotalJONSCulturePerTurn(), 1);
	}
	else
	{
		dOurVictoryETA = (double)iSecondHighestTotalCulture / (double)MAX(iOurTourism - GET_PLAYER(eSecondHighestTotalCulturePlayer).GetTotalJONSCulturePerTurn(), 1);
	}

	// Tourism ahead is now based on victory ETA's; sine term is to make sure this doesn't kick in prematurely (it's about = 0.896 halfway through the game)
	dPriority += GC.getAI_GS_CULTURE_TOURISM_AHEAD_WEIGHT() * (0.5 - dFastestVictoryETA / dOurVictoryETA) * 2.0
		* sin(sqrt((double)GC.getGame().getElapsedGameTurns() / (double)GC.getGame().getEstimateEndTurn()) * M_PI / 2.0);

	// Influential code now relies on ETA instead of number of influential civs; this way 5 stupidly uncultured civs don't force the AI down cultural victory
	if (m_pPlayer->GetCulture()->GetNumCivsInfluentialOn() > 0 && dOurVictoryETA / 2.0 < (GC.getGame().getEstimateEndTurn() - GC.getGame().getElapsedGameTurns()))
	{
		dPriority += GC.getAI_GS_CULTURE_INFLUENTIAL_CIV_MOD() * MAX(7.0, 2.0 * (GC.getGame().getEstimateEndTurn() - GC.getGame().getElapsedGameTurns()) / dOurVictoryETA);
	}

#else
	// Loop through Players to see how we are doing on Tourism and Culture
	PlayerTypes eLoopPlayer;
	int iOurCulture = m_pPlayer->GetTotalJONSCulturePerTurn();
	int iOurTourism = m_pPlayer->GetCulture()->GetTourism();
	int iNumCivsBehindCulture = 0;
	int iNumCivsAheadCulture = 0;
	int iNumCivsBehindTourism = 0;
	int iNumCivsAheadTourism = 0;
	int iNumCivsAlive = 0;
	int iHighestCulture = iOurCulture;
	int iLowestCulture = iOurCulture;
	int iHighestTourism = iOurTourism;
	int iLowestTourism = iOurTourism;

	for(int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
	{
		eLoopPlayer = (PlayerTypes) iPlayerLoop;
		CvPlayer &kPlayer = GET_PLAYER(eLoopPlayer);

		if (kPlayer.isAlive() && !kPlayer.isMinorCiv() && !kPlayer.isBarbarian() && iPlayerLoop != m_pPlayer->GetID())
		{
			if (iOurCulture > kPlayer.GetTotalJONSCulturePerTurn())
			{
				iNumCivsAheadCulture++;
			}
			else
			{
				iNumCivsBehindCulture++;
			}
			if (iOurTourism > kPlayer.GetCulture()->GetTourism())
			{
				iNumCivsAheadTourism++;
			}
			else
			{
				iNumCivsBehindTourism++;
			}
			iNumCivsAlive++;
		}
	}

	if (iNumCivsAlive > 0 && iNumCivsAheadCulture > iNumCivsBehindCulture)
	{
		dPriority += (GC.getAI_GS_CULTURE_AHEAD_WEIGHT() * (iNumCivsAheadCulture - iNumCivsBehindCulture) / (double)iNumCivsAlive);
	}
	if (iNumCivsAlive > 0 && iNumCivsAheadTourism > iNumCivsBehindTourism)
	{
		dPriority += (GC.getAI_GS_CULTURE_TOURISM_AHEAD_WEIGHT() * (iNumCivsAheadTourism - iNumCivsBehindTourism) / (double)iNumCivsAlive);
	}

	// for every civ we are Influential over increase this
	int iNumInfluential = m_pPlayer->GetCulture()->GetNumCivsInfluentialOn();
	dPriority += iNumInfluential * GC.getAI_GS_CULTURE_INFLUENTIAL_CIV_MOD();

#endif

	return (int)(dPriority + 0.5);

#else
	int iPriority = 0;

	// If Culture Victory isn't even available then don't bother with anything
	VictoryTypes eVictory = (VictoryTypes)GC.getInfoTypeForString("VICTORY_CULTURAL", true);
	if (eVictory == NO_VICTORY || !GC.getGame().isVictoryValid(eVictory))
	{
		return -100;
	}

	// Before tourism kicks in, add weight based on flavor
	int iFlavorCulture = m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_CULTURE"));
	iPriority += (10 - m_pPlayer->GetCurrentEra()) * iFlavorCulture * 200 / 100;

	// Loop through Players to see how we are doing on Tourism and Culture
	PlayerTypes eLoopPlayer;
	int iOurCulture = m_pPlayer->GetTotalJONSCulturePerTurn();
	int iOurTourism = m_pPlayer->GetCulture()->GetTourism();
	int iNumCivsBehindCulture = 0;
	int iNumCivsAheadCulture = 0;
	int iNumCivsBehindTourism = 0;
	int iNumCivsAheadTourism = 0;
	int iNumCivsAlive = 0;

	for (int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
	{
		eLoopPlayer = (PlayerTypes)iPlayerLoop;
		CvPlayer &kPlayer = GET_PLAYER(eLoopPlayer);

		if (kPlayer.isAlive() && !kPlayer.isMinorCiv() && !kPlayer.isBarbarian() && iPlayerLoop != m_pPlayer->GetID())
		{
			if (iOurCulture > kPlayer.GetTotalJONSCulturePerTurn())
			{
				iNumCivsAheadCulture++;
			}
			else
			{
				iNumCivsBehindCulture++;
			}
			if (iOurTourism > kPlayer.GetCulture()->GetTourism())
			{
				iNumCivsAheadTourism++;
			}
			else
			{
				iNumCivsBehindTourism++;
			}
			iNumCivsAlive++;
		}
	}

	if (iNumCivsAlive > 0 && iNumCivsAheadCulture > iNumCivsBehindCulture)
	{
		iPriority += (GC.getAI_GS_CULTURE_AHEAD_WEIGHT() * (iNumCivsAheadCulture - iNumCivsBehindCulture) / iNumCivsAlive);
	}
	if (iNumCivsAlive > 0 && iNumCivsAheadTourism > iNumCivsBehindTourism)
	{
		iPriority += (GC.getAI_GS_CULTURE_TOURISM_AHEAD_WEIGHT() * (iNumCivsAheadTourism - iNumCivsBehindTourism) / iNumCivsAlive);
	}

	// for every civ we are Influential over increase this
	int iNumInfluential = m_pPlayer->GetCulture()->GetNumCivsInfluentialOn();
	iPriority += iNumInfluential * GC.getAI_GS_CULTURE_INFLUENTIAL_CIV_MOD();

	return iPriority;
#endif
}

/// Returns Priority for United Nations Grand Strategy
int CvGrandStrategyAI::GetUnitedNationsPriority()
{
#ifdef AUI_GS_USE_DOUBLES
	double dPriority = 0;
	PlayerTypes ePlayer = m_pPlayer->GetID();

	// If UN Victory isn't even available then don't bother with anything
	VictoryTypes eVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_DIPLOMATIC", true);
	if(eVictory == NO_VICTORY || !GC.getGame().isVictoryValid(eVictory))
	{
		return -100;
	}

	int iNumMinorsAttacked = GET_TEAM(GetPlayer()->getTeam()).GetNumMinorCivsAttacked();
	dPriority += (iNumMinorsAttacked* /*-30*/ GC.getAI_GRAND_STRATEGY_UN_EACH_MINOR_ATTACKED_WEIGHT());

	int iVotesNeededToWin = GC.getGame().GetVotesNeededForDiploVictory();

	int iVotesControlled = 0;
	int iVotesControlledDelta = 0;
	int iUnalliedCityStates = 0;
	if (GC.getGame().GetGameLeagues()->GetNumActiveLeagues() == 0)
	{
		// Before leagues kick in, add weight based on flavor
		int iFlavorDiplo =  m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_DIPLOMACY"));
#ifdef AUI_GS_DIPLOMATIC_TWEAKED_ERAS
		dPriority += (11 - sqrt(m_pPlayer->GetCurrentEra() + 1.0)) * iFlavorDiplo * 2.0;
#else
		fPriority += (10 - m_pPlayer->GetCurrentEra()) * iFlavorDiplo * 150.0f / 100.0f;
#endif
	}
	else
	{
		CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
		CvAssert(pLeague != NULL);
		if (pLeague != NULL)
		{
#ifdef AUI_GS_DIPLOMATIC_TWEAKED_ERAS
			// Add a bit from flavor as well unless UN is enabled
			if (!pLeague->IsUnitedNations())
			{
				int iFlavorDiplo = m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_DIPLOMACY"));
				dPriority += (10 - sqrt(m_pPlayer->GetCurrentEra() + 1.0)) * iFlavorDiplo * (double)MAX(4 - GC.getGame().GetGameLeagues()->GetNumLeaguesEverFounded(), 1);
			}
#endif
			
			// Votes we control
			iVotesControlled += pLeague->CalculateStartingVotesForMember(ePlayer);

			// Votes other players control
			int iHighestOtherPlayerVotes = 0;
			for (int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
			{
				PlayerTypes eLoopPlayer = (PlayerTypes) iPlayerLoop;

				if(eLoopPlayer != ePlayer && GET_PLAYER(eLoopPlayer).isAlive())
				{
					if (GET_PLAYER(eLoopPlayer).isMinorCiv())
					{
						if (GET_PLAYER(eLoopPlayer).GetMinorCivAI()->GetAlly() == NO_PLAYER)
						{
							iUnalliedCityStates++;
						}
					}
					else
					{
						int iOtherPlayerVotes = pLeague->CalculateStartingVotesForMember(eLoopPlayer);
						if (iOtherPlayerVotes > iHighestOtherPlayerVotes)
						{
							iHighestOtherPlayerVotes = iOtherPlayerVotes;
						}
					}
				}
			}

			// How we compare
			iVotesControlledDelta = iVotesControlled - iHighestOtherPlayerVotes;
		}
	}

	// Are we close to winning?
	if (iVotesControlled >= iVotesNeededToWin)
	{
		return 1000;
	}
	else if (4 * iVotesControlled >= iVotesNeededToWin * 3)
	{
		dPriority += 40;
	}

	// We have the most votes
	if (iVotesControlledDelta > 0)
	{
		dPriority += MAX(40, iVotesControlledDelta * 5);
	}
	// We are equal or behind in votes
	else
	{
		// Could we make up the difference with currently unallied city-states?
		int iPotentialCityStateVotes = iUnalliedCityStates * 2;
		int iPotentialVotesDelta = iPotentialCityStateVotes + iVotesControlledDelta;
		if (iPotentialVotesDelta > 0)
		{
			dPriority += MAX(20, iPotentialVotesDelta * 5);
		}
		else if (iPotentialVotesDelta < 0)
		{
			dPriority += MIN(-40, iPotentialVotesDelta * -5);
		}
	}

	// factor in some traits that could be useful (or harmful)
	dPriority += m_pPlayer->GetPlayerTraits()->GetCityStateFriendshipModifier();
	dPriority += m_pPlayer->GetPlayerTraits()->GetCityStateBonusModifier();
	dPriority -= m_pPlayer->GetPlayerTraits()->GetCityStateCombatModifier();

	return (int)(dPriority + 0.5);
#else
	int iPriority = 0;
	PlayerTypes ePlayer = m_pPlayer->GetID();

	// If UN Victory isn't even available then don't bother with anything
	VictoryTypes eVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_DIPLOMATIC", true);
	if(eVictory == NO_VICTORY || !GC.getGame().isVictoryValid(eVictory))
	{
		return -100;
	}

	int iNumMinorsAttacked = GET_TEAM(GetPlayer()->getTeam()).GetNumMinorCivsAttacked();
	iPriority += (iNumMinorsAttacked* /*-30*/ GC.getAI_GRAND_STRATEGY_UN_EACH_MINOR_ATTACKED_WEIGHT());

	int iVotesNeededToWin = GC.getGame().GetVotesNeededForDiploVictory();

	int iVotesControlled = 0;
	int iVotesControlledDelta = 0;
	int iUnalliedCityStates = 0;
	if (GC.getGame().GetGameLeagues()->GetNumActiveLeagues() == 0)
	{
		// Before leagues kick in, add weight based on flavor
		int iFlavorDiplo =  m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_DIPLOMACY"));
		iPriority += (10 - m_pPlayer->GetCurrentEra()) * iFlavorDiplo * 150 / 100;
	}
	else
	{
		CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
		CvAssert(pLeague != NULL);
		if (pLeague != NULL)
		{
			// Votes we control
			iVotesControlled += pLeague->CalculateStartingVotesForMember(ePlayer);

			// Votes other players control
			int iHighestOtherPlayerVotes = 0;
			for (int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
			{
				PlayerTypes eLoopPlayer = (PlayerTypes) iPlayerLoop;

				if(eLoopPlayer != ePlayer && GET_PLAYER(eLoopPlayer).isAlive())
				{
					if (GET_PLAYER(eLoopPlayer).isMinorCiv())
					{
						if (GET_PLAYER(eLoopPlayer).GetMinorCivAI()->GetAlly() == NO_PLAYER)
						{
							iUnalliedCityStates++;
						}
					}
					else
					{
						int iOtherPlayerVotes = pLeague->CalculateStartingVotesForMember(eLoopPlayer);
						if (iOtherPlayerVotes > iHighestOtherPlayerVotes)
						{
							iHighestOtherPlayerVotes = iOtherPlayerVotes;
						}
					}
				}
			}

			// How we compare
			iVotesControlledDelta = iVotesControlled - iHighestOtherPlayerVotes;
		}
	}

	// Are we close to winning?
	if (iVotesControlled >= iVotesNeededToWin)
	{
		return 1000;
	}
	else if (iVotesControlled >= ((iVotesNeededToWin * 3) / 4))
	{
		iPriority += 40;
	}

	// We have the most votes
	if (iVotesControlledDelta > 0)
	{
		iPriority += MAX(40, iVotesControlledDelta * 5);
	}
	// We are equal or behind in votes
	else
	{
		// Could we make up the difference with currently unallied city-states?
		int iPotentialCityStateVotes = iUnalliedCityStates * 2;
		int iPotentialVotesDelta = iPotentialCityStateVotes + iVotesControlledDelta;
		if (iPotentialVotesDelta > 0)
		{
			iPriority += MAX(20, iPotentialVotesDelta * 5);
		}
		else if (iPotentialVotesDelta < 0)
		{
			iPriority += MIN(-40, iPotentialVotesDelta * -5);
		}
	}

	// factor in some traits that could be useful (or harmful)
	iPriority += m_pPlayer->GetPlayerTraits()->GetCityStateFriendshipModifier();
	iPriority += m_pPlayer->GetPlayerTraits()->GetCityStateBonusModifier();
	iPriority -= m_pPlayer->GetPlayerTraits()->GetCityStateCombatModifier();

	return iPriority;
#endif
}

/// Returns Priority for Spaceship Grand Strategy
int CvGrandStrategyAI::GetSpaceshipPriority()
{
#ifdef AUI_GS_USE_DOUBLES
	double dPriority = 0;

	// If SS Victory isn't even available then don't bother with anything
	VictoryTypes eVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_SPACE_RACE", true);
	if(eVictory == NO_VICTORY || !GC.getGame().isVictoryValid(eVictory))
	{
		return -100;
	}

	int iFlavorScience =  m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_SCIENCE"));
#ifdef AUI_GS_SPACESHIP_TWEAKED_FLAVORS
	// might not be scientific, but goals still suit victory type
	iFlavorScience += (int)(sqrt((double)(m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_PRODUCTION")) *
		MAX(m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_GROWTH")), 
		m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_EXPANSION"))))) + 0.5);
#endif

	// the later the game the greater the chance
#ifdef AUI_GS_SPACESHIP_TWEAKED_ERAS
	dPriority += sqrt(m_pPlayer->GetCurrentEra() + 1.0) * iFlavorScience * 4.0;
#else
	dPriority += m_pPlayer->GetCurrentEra() * iFlavorScience * 150.0 / 100.0;
#endif

#ifdef AUI_GS_SPACESHIP_TECH_RATIO
	// Give between 0 and 100 priority based on tech ratio; scaling is exponential, so higher tech civs get more priority
	dPriority += pow(101.0, 1.0 - m_pPlayer->GetPlayerTechs()->GetTechAI()->GetTechRatio()) - 1;
#endif

	// if I already built the Apollo Program I am very likely to follow through
	ProjectTypes eApolloProgram = (ProjectTypes) GC.getInfoTypeForString("PROJECT_APOLLO_PROGRAM", true);
	if(eApolloProgram != NO_PROJECT)
	{
		if(GET_TEAM(m_pPlayer->getTeam()).getProjectCount(eApolloProgram) > 0)
		{
			dPriority += /*150*/ GC.getAI_GS_SS_HAS_APOLLO_PROGRAM();
		}
	}
#ifdef AUI_GS_SPACESHIP_TWEAKED_ERAS
	if (eApolloProgram == NO_PROJECT || GET_TEAM(m_pPlayer->getTeam()).getProjectCount(eApolloProgram) == 0)
	{
		// We're on Future Tech, so might as well try to power through to spaceship
		if (m_pPlayer->GetPlayerTechs()->IsCurrentResearchRepeat())
		{
			dPriority += GC.getAI_GS_SS_HAS_APOLLO_PROGRAM() * iFlavorScience / (double)GC.getFLAVOR_MAX_VALUE();
		}
	}
#endif

	return (int)(dPriority + 0.5);
#else
	int iPriority = 0;

	// If SS Victory isn't even available then don't bother with anything
	VictoryTypes eVictory = (VictoryTypes)GC.getInfoTypeForString("VICTORY_SPACE_RACE", true);
	if (eVictory == NO_VICTORY || !GC.getGame().isVictoryValid(eVictory))
	{
		return -100;
	}

	int iFlavorScience = m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_SCIENCE"));

	// the later the game the greater the chance
	iPriority += m_pPlayer->GetCurrentEra() * iFlavorScience * 150 / 100;

	// if I already built the Apollo Program I am very likely to follow through
	ProjectTypes eApolloProgram = (ProjectTypes)GC.getInfoTypeForString("PROJECT_APOLLO_PROGRAM", true);
	if (eApolloProgram != NO_PROJECT)
	{
		if (GET_TEAM(m_pPlayer->getTeam()).getProjectCount(eApolloProgram) > 0)
		{
			iPriority += /*150*/ GC.getAI_GS_SS_HAS_APOLLO_PROGRAM();
		}
	}

	return iPriority;
#endif
}

#ifdef AUI_GS_PRIORITY_OVERHAUL
/// Get the curve to be applied to a GS priority; starts at 1, ends at 0, higher flavor prolongs the curve's drop
double CvGrandStrategyAI::GetFlavorCurve(AIGrandStrategyTypes eGrandStrategy)
{
	// fCurve(x, y) = 1 - X^Y; X = game progress (between 0 and 1), Y = exponent to determine curve (linear if = 1, drops earlier if < 1, drops later if > 1)
	CvAIGrandStrategyXMLEntry* pGrandStrategy = GetAIGrandStrategies()->GetEntry(eGrandStrategy);

	double dFlavor = (double)GC.getPERSONALITY_FLAVOR_MIN_VALUE(); // used to calculate Y-value later
	int iFlavorSources = 0; // in case we have multiple flavors influencing one GS, use arithmetic average weighted by the amount of influence
	for (int iFlavorLoop = 0; iFlavorLoop < GC.getNumFlavorTypes(); iFlavorLoop++)
	{
		if (pGrandStrategy->GetFlavorValue(iFlavorLoop) != 0)
		{
			if (iFlavorSources == 0)
			{
				dFlavor = (double)(pGrandStrategy->GetFlavorValue(iFlavorLoop) * GetPlayer()->GetFlavorManager()->GetPersonalityIndividualFlavor((FlavorTypes)iFlavorLoop));
			}
			else
			{
				dFlavor += (double)(pGrandStrategy->GetFlavorValue(iFlavorLoop) * GetPlayer()->GetFlavorManager()->GetPersonalityIndividualFlavor((FlavorTypes)iFlavorLoop));
			}
			iFlavorSources += pGrandStrategy->GetFlavorValue(iFlavorLoop);
		}
	}
	dFlavor /= (double)MAX(1, iFlavorSources);

	// Higher flavor value curves the drop more and more, effectively giving the AI more time to adjust circumstances before game-specific portion of GS takes over
	const double dCurveExponent = pow(MAX(dFlavor - GC.getPERSONALITY_FLAVOR_MIN_VALUE() + 1, 1.0), 1.0/3.0);

	const double dGameProgress = (double)GC.getGame().getElapsedGameTurns() / (double)GC.getGame().getEstimateEndTurn(); // used as the X-value

	return (1.0 - pow(dGameProgress, dCurveExponent));
}

/// Get the game-independent portion of GS Priority; starts high, drops to 0 on the very last turn, drop curve affected by personality
double CvGrandStrategyAI::GetBaseGrandStrategyPriority(AIGrandStrategyTypes eGrandStrategy)
{
	const double fPriority = (double)GC.getFLAVOR_MAX_VALUE();

	return (fPriority * GetFlavorCurve(eGrandStrategy));
}

/// Get the game-dependent portion of Conquest GS Priority;
double CvGrandStrategyAI::GetConquestETAPriority()
{
	VictoryTypes eVictory = (VictoryTypes)GC.getInfoTypeForString("VICTORY_DOMINATION", true);
	AIGrandStrategyTypes eGrandStrategy = (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST");

	// If Diplo Victory isn't even available then don't bother with anything
	if (eVictory == NO_VICTORY || !GC.getGame().isVictoryValid(eVictory))
	{
		return -GetBaseGrandStrategyPriority(eGrandStrategy);
	}

	// Number of turns it would (roughly) take to win this victory
	int iTurnsToVictory = 0;
	const int iTurnsRemaining = GC.getGame().getEstimateEndTurn() - GC.getGame().getElapsedGameTurns();

	double fDistanceMultiplier = sqrt(double(GC.getNumEraInfos() - m_pPlayer->GetCurrentEra() - 1) / (double)GC.getNumEraInfos());

	// Populated list of all original capitals we do not own
	std::vector<CvCity*> apOriginalCapitals;
	PlayerTypes eLoopPlayer;
	for (int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
	{
		eLoopPlayer = (PlayerTypes)iPlayerLoop;
		CvPlayer &kPlayer = GET_PLAYER(eLoopPlayer);

		if (!kPlayer.isMinorCiv() && !kPlayer.isBarbarian() && iPlayerLoop != m_pPlayer->GetID())
		{
			CvCity* pLoopCity;
			int iLoop;
			for (pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
			{
				if (pLoopCity->IsOriginalMajorCapital())
				{
					apOriginalCapitals.push_back(pLoopCity);
				}
			}
		}
	}

	CvCity* pOurCapital = m_pPlayer->getCapitalCity();

	// Calculate the necessary turns required for to capture each capital
	for (std::vector<CvCity*>::iterator it = apOriginalCapitals.begin(); it != apOriginalCapitals.end(); ++it)
	{		
		int iDistance = MAX_INT;
		if (!(*it)->plot()->isVisible(m_pPlayer->getTeam()))
		{
			// Capital isn't visible, so assume its distance is the max possible
			iDistance = GC.getMap().maxPlotDistance();
		}
		else
		{
			int iLoop;
			CvCity* pLoopCity;
			int iLoopDistance = MAX_INT;
			bool bInSameArea = false;
			for (pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
			{
				iLoopDistance = plotDistance(pLoopCity->getX(), pLoopCity->getY(), (*it)->getX(), (*it)->getY());
				if (iLoopDistance < iDistance)
				{
					iDistance = iLoopDistance;
				}
				if (pLoopCity->getArea() == (*it)->getArea())
				{
					bInSameArea = true;
				}
			}
			if (bInSameArea)
			{
				iDistance = int(iDistance / 2.0 + 0.5);
			}
		}

		// Multiplier from military power ratio between original capital's current owner and us (larger = they are stronger)
		double fMilitaryRatio = (double)(GET_PLAYER((*it)->getOwner()).GetMilitaryMight() + (*it)->getStrengthValue()) / (double)MAX(m_pPlayer->GetMilitaryMight(), 1);

		// In lieu of proper time values, Boldness is used instead
		double fBaseTime = (double)GC.getPERSONALITY_FLAVOR_MAX_VALUE() / sqrt((double)MAX(m_pPlayer->GetDiplomacyAI()->GetBoldness() + GC.getPERSONALITY_FLAVOR_MIN_VALUE(), 1));

		// If capturing the capital would make us unhappy, reduce weight
		double dUnhappinessFactor = 1.0;
		if (m_pPlayer->GetHappiness() - m_pPlayer->GetUnhappiness(NULL, (*it)) <= GC.getVERY_UNHAPPY_THRESHOLD())
		{
			dUnhappinessFactor = 2.0;
		}

		iTurnsToVictory += int((dBaseTime * dMilitaryRatio + iDistance * dDistanceMultiplier) * dUnhappinessFactor);
	}

	const double dLogisticExpander = GC.getFLAVOR_MAX_VALUE() * 10.0 / (double)GC.getGame().getEstimateEndTurn();
	// Logistic function, halfway point when iTurnsRemaining = iTurnsToVictory
	double dPriority = (double)GC.getFLAVOR_MAX_VALUE() / (1.0 + exp((1.0 - (double)iTurnsRemaining / (double)iTurnsToVictory) * dLogisticExpander));

	return (dPriority * (1.0 - GetFlavorCurve(eGrandStrategy)));
}

/// Get the game-dependent portion of Culture GS Priority;
double CvGrandStrategyAI::GetCultureETAPriority()
{
	VictoryTypes eVictory = (VictoryTypes)GC.getInfoTypeForString("VICTORY_CULTURAL", true);
	AIGrandStrategyTypes eGrandStrategy = (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE");

	// If Diplo Victory isn't even available then don't bother with anything
	if (eVictory == NO_VICTORY || !GC.getGame().isVictoryValid(eVictory))
	{
		return -GetBaseGrandStrategyPriority(eGrandStrategy);
	}

	// Number of turns it would (roughly) take to win this victory
	int iTurnsToVictory = 0;
	const int iTurnsRemaining = GC.getGame().getEstimateEndTurn() - GC.getGame().getElapsedGameTurns();

	PlayerTypes ePlayer = m_pPlayer->GetID();

	// If we don't have culture, don't add any game-specific priority
	int iOurTourism = m_pPlayer->GetCulture()->GetTourism();
	if (iOurTourism <= 0)
	{
		return 0.0;
	}
	
	PlayerTypes eLoopPlayer;
		
	// Weight is the amount of turns it would take to get influential with the player
	CvWeightedVector<PlayerTypes, MAX_CIV_PLAYERS, true> aePlayerInfluenceTurns;

	int iInfluentialCount = 0;

	for (int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
	{
		eLoopPlayer = (PlayerTypes)iPlayerLoop;
		CvPlayer &kPlayer = GET_PLAYER(eLoopPlayer);

		int iTurnsToInfluential;

		if (kPlayer.isAlive() && !kPlayer.isMinorCiv() && !kPlayer.isBarbarian() && iPlayerLoop != m_pPlayer->GetID())
		{
			// Copy of GetTurnsToInfluential form CvPlayerCulture, but without the arbitrary 999 cap
			iTurnsToInfluential = MAX_INT;
			if (m_pPlayer->GetCulture()->GetInfluenceLevel(ePlayer) >= INFLUENCE_LEVEL_INFLUENTIAL)
			{
				iTurnsToInfluential = 0;
				iInfluentialCount++;
			}
			else if (m_pPlayer->GetCulture()->GetInfluenceTrend(ePlayer) == INFLUENCE_TREND_RISING)
			{
				int iInfluence = m_pPlayer->GetCulture()->GetInfluenceOn(ePlayer);
				int iInflPerTurn = m_pPlayer->GetCulture()->GetInfluencePerTurn(ePlayer);
				double dCulture = (double)kPlayer.GetJONSCultureEverGenerated();
				double dCultPerTurn = (double)kPlayer.GetTotalJONSCulturePerTurn();
				double dNumerator = (GC.getCULTURE_LEVEL_INFLUENTIAL() * dCulture / 100.0) - iInfluence;
				double dDivisor = iInflPerTurn - (GC.getCULTURE_LEVEL_INFLUENTIAL() * dCultPerTurn / 100.0);

				if (dDivisor > 0)
				{
					iTurnsToInfluential = (int)(dNumerator / dDivisor + 0.5);
				}
			}
			aePlayerInfluenceTurns.push_back(ePlayer, iTurnsToInfluential);
		}
	}
	aePlayerInfluenceTurns.SortItems();

	
	// We will use the first non-MAX_INT value as our ETA unless it's 0, in which case we use MAX_INT
	iTurnsToVictory = MAX_INT;
	// For every player with MAX_INT influential ETA, multiply turns needed by 2; this gets cancelled for each person we have influence over (to help with early tourism that isn't big yet)
	int iTurnsMultiplier = 1;
	for (int iI = 0; iI < aePlayerInfluenceTurns.size(); iI++)
	{
		if (aePlayerInfluenceTurns.GetWeight(iI) == 0)
		{
			break;
		}
		else if (aePlayerInfluenceTurns.GetWeight(iI) != MAX_INT)
		{
			iTurnsToVictory = aePlayerInfluenceTurns.GetWeight(iI);
			break;
		}
		else
		{
			if (iInfluentialCount > 0)
			{
				iInfluentialCount--;
			}
			else
			{
				iTurnsMultiplier *= 2;
			}
		}
	}
	iTurnsToVictory *= iTurnsMultiplier;

	iTurnsToVictory = aePlayerInfluenceTurns.GetWeight(0);

	const double dLogisticExpander = GC.getFLAVOR_MAX_VALUE() * 10.0 / (double)GC.getGame().getEstimateEndTurn();
	// Logistic function, halfway point when iTurnsRemaining = iTurnsToVictory
	double dPriority = (double)GC.getFLAVOR_MAX_VALUE() / (1.0 + exp((1.0 - (double)iTurnsRemaining / (double)iTurnsToVictory) * dLogisticExpander));

	return (dPriority * (1.0 - GetFlavorCurve(eGrandStrategy)));
}

/// Get the game-dependent portion of UN GS Priority;
double CvGrandStrategyAI::GetUnitedNationsETAPriority()
{
	VictoryTypes eVictory = (VictoryTypes)GC.getInfoTypeForString("VICTORY_DIPLOMATIC", true);
	AIGrandStrategyTypes eGrandStrategy = (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS");

	// If Diplo Victory isn't even available then don't bother with anything
	if (eVictory == NO_VICTORY || !GC.getGame().isVictoryValid(eVictory))
	{
		return -GetBaseGrandStrategyPriority(eGrandStrategy);
	}

	// If there are no leagues active, only the base priority is in effect
	if (GC.getGame().GetGameLeagues()->GetNumActiveLeagues() == 0)
	{
		return 0.0;
	}

	// Number of turns it would (roughly) take to win this victory
	int iTurnsToVictory = MAX_INT;
	const int iTurnsRemaining = GC.getGame().getEstimateEndTurn() - GC.getGame().getElapsedGameTurns();

	PlayerTypes ePlayer = m_pPlayer->GetID();

	const int iVotesNeededToWin = GC.getGame().GetVotesNeededForDiploVictory();
	int iVotesControlled = 0;
	int iPotentialVotes = 0;
	std::vector<PlayerTypes> aeAlliedCityStates;
	std::vector<PlayerTypes> aeUnalliedCityStates;
	// Items are major civs, weights are their vote count
	CvWeightedVector<PlayerTypes, MAX_CIV_PLAYERS, true> aeVotingList;

	CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
	CvAssert(pLeague != NULL);
	if (pLeague != NULL)
	{
		// If it is impossible to win within the timeframe, don't bother
		if (pLeague->IsUnitedNations() && pLeague->GetTurnsUntilVictorySession() > iTurnsRemaining)
		{
			return -GetBaseGrandStrategyPriority(eGrandStrategy);
		}

		iVotesControlled = pLeague->CalculateStartingVotesForMember(ePlayer);
		
		for (int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
		{
			PlayerTypes eLoopPlayer = (PlayerTypes)iPlayerLoop;

			if (GET_PLAYER(eLoopPlayer).isAlive())
			{
				if (GET_PLAYER(eLoopPlayer).isMinorCiv())
				{
					if (GET_PLAYER(eLoopPlayer).GetMinorCivAI()->GetAlly() == ePlayer)
					{
						aeAlliedCityStates.push_back(eLoopPlayer);
					}
					else
					{
						aeUnalliedCityStates.push_back(eLoopPlayer);
					}
				}
				else
				{
					aeVotingList.push_back(eLoopPlayer, pLeague->CalculateStartingVotesForMember(eLoopPlayer));
				}
			}
		}

		aeVotingList.SortItems();
		// High priority if we're already going to win
		if (iVotesControlled >= iVotesNeededToWin && pLeague->IsUnitedNations())
		{
			iTurnsToVictory = pLeague->GetTurnsUntilVictorySession();
		}
		// Killjoy to attempt to stop a diplomatic victory in progress
		else if (aeVotingList.GetWeight(0) >= iVotesNeededToWin && pLeague->IsUnitedNations())
		{
			iTurnsToVictory = MAX(pLeague->GetTurnsUntilVictorySession() - 1, 0);
		}
		else
		{
			int iTurnsOfWaiting = 0;
			if (pLeague->IsUnitedNations())
			{
				// Potential votes from failed diplomatic victory vote if we're the top 2
				CvResolutionEntry* pDiploVictoryResolution = GC.getResolutionInfo((ResolutionTypes)0);
				for (int iI = 0; iI < GC.getNumResolutionInfos(); iI++)
				{
					const ResolutionTypes eResolutionInfo = static_cast<ResolutionTypes>(iI);
					CvResolutionEntry* pkResolutionInfo = GC.getResolutionInfo(eResolutionInfo);
					if (pkResolutionInfo && pkResolutionInfo->IsDiplomaticVictory())
					{
						pDiploVictoryResolution = pkResolutionInfo;
						break;
					}
				}
				int iVictorySessionsAfterThis = MAX((iTurnsRemaining - pLeague->GetTurnsUntilVictorySession()) / MAX(2 * pLeague->GetSessionTurnInterval(), 1), 0);
				if (iVotesControlled >= aeVotingList.GetWeight(1))
				{
					iPotentialVotes += iVictorySessionsAfterThis * pDiploVictoryResolution->GetLeadersVoteBonusOnFail();
					iTurnsOfWaiting = (iVictorySessionsAfterThis - 1) * 2 * pLeague->GetSessionTurnInterval();
				}

				// Potential votes from spies
				if (m_pPlayer->GetExtraVotesPerDiplomat() > 0)
				{
					int iNumDiplomats = 0;
					for (int i = 0; i < MAX_MAJOR_CIVS; i++)
					{
						PlayerTypes eMajor = (PlayerTypes)i;
						if (GET_PLAYER(eMajor).isAlive())
						{
							if (GET_PLAYER(ePlayer).GetEspionage()->IsMyDiplomatVisitingThem(eMajor))
							{
								iNumDiplomats++;
							}
						}
					}
					iPotentialVotes += m_pPlayer->GetExtraVotesPerDiplomat() * (m_pPlayer->GetEspionage()->GetNumSpies() - iNumDiplomats);
				}
			}

			// If our victory is secure without having to mess with city-states, keep flavor at maximum possible
			if (iVotesControlled + iPotentialVotes >= iVotesNeededToWin)
			{
				iTurnsToVictory = pLeague->GetTurnsUntilVictorySession() + int(iTurnsOfWaiting * iVotesNeededToWin / (iVotesControlled + iPotentialVotes));
			}
			else
			{
				LeagueSpecialSessionTypes eGoverningSpecialSession = NO_LEAGUE_SPECIAL_SESSION;
				if (pLeague->GetCurrentSpecialSession() != NO_LEAGUE_SPECIAL_SESSION)
				{
					eGoverningSpecialSession = pLeague->GetCurrentSpecialSession();
				}
				else if (pLeague->GetLastSpecialSession() != NO_LEAGUE_SPECIAL_SESSION)
				{
					eGoverningSpecialSession = pLeague->GetLastSpecialSession();
				}
				CvLeagueSpecialSessionEntry* pSessionInfo = GC.getLeagueSpecialSessionInfo(eGoverningSpecialSession);

				// Bit of an approximation, but it overestimates, so it's not as much of an issue
				int iTurnsNeeded = 0;
				const int iPlayerGPT = m_pPlayer->calculateGoldRateTimes100();
				
				double dTotalGPTInfluence = 0;
				// Weight is the amount of influence we need as a negative number (so sort() works properly)
				CvWeightedVector<PlayerTypes, REALLY_MAX_PLAYERS, true> aePossibleCityStates;
				for (std::vector<PlayerTypes>::iterator it = aeUnalliedCityStates.begin(); it != aeUnalliedCityStates.end(); ++it)
				{
					CvMinorCivAI* pLoopMinor = GET_PLAYER((*it)).GetMinorCivAI();
					// Value here is a constant 1000 because more precision isn't worth it; also doesn't account for decay per turn if already positive
					dTotalGPTInfluence += (double)(pLoopMinor->GetFriendshipFromGoldGift(ePlayer, 1000) * iPlayerGPT) / 100000.0; // /1000 from friendship, /100 from Times100
				}
				if (fTotalGPTInfluence != 0)
				{
					const double dAverageGPTInfluence = dTotalGPTInfluence / (double)MAX((int)aeUnalliedCityStates.size(), 1);
					const double dGPTInfluencePerMinor = dAverageGPTInfluence / (double)MAX((int)aeUnalliedCityStates.size(), 1);

					int iInfluenceNeeded;
					for (std::vector<PlayerTypes>::iterator it = aeUnalliedCityStates.begin(); it != aeUnalliedCityStates.end(); ++it)
					{
						CvMinorCivAI* pLoopMinor = GET_PLAYER((*it)).GetMinorCivAI();
						if (pLoopMinor->GetAlly() == NO_PLAYER)
						{
							iInfluenceNeeded = pLoopMinor->GetAlliesThreshold();
						}
						else
						{
							iInfluenceNeeded = pLoopMinor->GetEffectiveFriendshipWithMajor(pLoopMinor->GetAlly());
						}
						iInfluenceNeeded -= pLoopMinor->GetEffectiveFriendshipWithMajor(ePlayer);
						const double dBaseInfluencePerTurn = pLoopMinor->GetFriendshipChangePerTurnTimes100(ePlayer) / 100.0;

						aePossibleCityStates.push_back((*it), -int(iInfluenceNeeded / (dBaseInfluencePerTurn + dGPTInfluencePerMinor)));
					}
					aePossibleCityStates.SortItems();

					// Amount of city states we still need 
					int iCityStatesNeeded = (iVotesNeededToWin - iVotesControlled - iPotentialVotes) / MAX(pSessionInfo->GetCityStateDelegates(), 1);
					iCityStatesNeeded = MAX(MIN(iCityStatesNeeded, (int)aeUnalliedCityStates.size()), 1);
					const double dAdjustedGPTInfluencePerMinor = dAverageGPTInfluence / (double)MAX(iCityStatesNeeded, 1);

					for (int iI = 0; iI < iCityStatesNeeded; iI++)
					{
						CvMinorCivAI* pLoopMinor = GET_PLAYER(aePossibleCityStates.GetElement(iI)).GetMinorCivAI();
						if (pLoopMinor->GetAlly() == NO_PLAYER)
						{
							iInfluenceNeeded = pLoopMinor->GetAlliesThreshold();
						}
						else
						{
							iInfluenceNeeded = pLoopMinor->GetEffectiveFriendshipWithMajor(pLoopMinor->GetAlly());
						}
						iInfluenceNeeded -= pLoopMinor->GetEffectiveFriendshipWithMajor(ePlayer);
						const double fBaseInfluencePerTurn = pLoopMinor->GetFriendshipChangePerTurnTimes100(ePlayer) / 100.0f;

						iTurnsNeeded += int(iInfluenceNeeded / (dBaseInfluencePerTurn + dAdjustedGPTInfluencePerMinor));
					}

					iTurnsToVictory = iTurnsNeeded;
				}
			}
		}
	}

	/*
	// These values are selected so that the overall fPriority is roughly 20% of its max when iTurnsToVictory = iTurnsRemaining * 2
	double fPriority = (double)GC.getFLAVOR_MAX_VALUE() * pow(MIN((double)MAX(iTurnsRemaining, 1) / (double)MAX(iTurnsToVictory, 1), 1.0), sqrt(5.0));
	*/

	const double fLogisticExpander = GC.getFLAVOR_MAX_VALUE() * 10.0 / (double)GC.getGame().getEstimateEndTurn();
	// Logistic function, halfway point when iTurnsRemaining = iTurnsToVictory
	double dPriority = (double)GC.getFLAVOR_MAX_VALUE() / (1.0 + exp((1.0 - (double)iTurnsRemaining / (double)iTurnsToVictory) * dLogisticExpander));

	return (dPriority * (1.0 - GetFlavorCurve(eGrandStrategy)));
}

/// Get the game-dependent portion of Spaceship GS Priority;
double CvGrandStrategyAI::GetSpaceshipETAPriority()
{
	VictoryTypes eVictory = (VictoryTypes)GC.getInfoTypeForString("VICTORY_SPACE_RACE", true);
	AIGrandStrategyTypes eGrandStrategy = (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP");

	// If SS Victory isn't even available then don't bother with anything
	if (eVictory == NO_VICTORY || !GC.getGame().isVictoryValid(eVictory))
	{
		return -GetBaseGrandStrategyPriority(eGrandStrategy);
	}

	// Number of turns it would (roughly) take to win this victory
	int iTurnsToVictory = 0;
	const int iTurnsRemaining = GC.getGame().getEstimateEndTurn() - GC.getGame().getElapsedGameTurns();

	// TODO: Add mod compatibility for modded projects that unlock spaceship victory, more than one project unlocking spaceship victory, etc.

	// Weight is the number of a part needed
	CvWeightedVector<UnitTypes, 5, true> apSpaceshipParts;
	for (int iI = 0; iI < GC.getNumUnitInfos(); iI++)
	{
		const UnitTypes eUnitInfo = static_cast<UnitTypes>(iI);
		CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnitInfo);
		if (pkUnitInfo && pkUnitInfo->GetDefaultUnitAIType() == UNITAI_SPACESHIP_PART)
		{
			ProjectTypes eSSProject = (ProjectTypes)pkUnitInfo->GetSpaceshipProject();
			CvProjectEntry* pkSSProject = GC.getProjectInfo(eSSProject);
			apSpaceshipParts.push_back((UnitTypes)iI, pkSSProject->GetMaxTeamInstances());
		}
	}
	// TODO: Add support for multiple Apollo Program-like projects (if a mod implements one)
	ProjectTypes eApolloProgram = (ProjectTypes)GC.getUnitInfo(apSpaceshipParts.GetElement(1))->GetProjectPrereq();
	CvProjectEntry* pkApolloProgram = GC.getProjectInfo(eApolloProgram);

	int iBeakersNeeded = 0;
	TechTypes eApolloTechPrereq = (TechTypes)pkApolloProgram->GetTechPrereq();

	// If we don't have Apollo tech, we need to research it before we can do beakers and hammers for this victory in parallel
	bool bHasApolloTech = GET_TEAM(m_pPlayer->getTeam()).GetTeamTechs()->HasTech(eApolloTechPrereq);

	// Bit of cheating to reduce computational time and to make the AI not completely freak out by high cost of endgame research needed for SS parts
	if (!bHasApolloTech)
	{
		iBeakersNeeded = GC.getGame().GetResearchLeftToTech(m_pPlayer->getTeam(), eApolloTechPrereq);
	}
	else
	{
		int iMaxBeakersNeeded = 0;
		for (int iI = 0; iI < apSpaceshipParts.size(); iI++)
		{
			TechTypes eSSTechPrereq = (TechTypes)GC.getUnitInfo(apSpaceshipParts.GetElement(iI))->GetPrereqAndTech();
			int iBeakersNeededForPrereq = GC.getGame().GetResearchLeftToTech(m_pPlayer->getTeam(), eSSTechPrereq);
			if (iBeakersNeededForPrereq > iMaxBeakersNeeded)
			{
				iMaxBeakersNeeded = iBeakersNeededForPrereq;
			}
		}
		iBeakersNeeded = iMaxBeakersNeeded;
	}
	int iTechTurnsNeeded = iBeakersNeeded * 100 / MAX(m_pPlayer->GetScienceTimes100(), 1);

	int iHammerTurnsNeeded = 0;

	if (GET_TEAM(m_pPlayer->getTeam()).getProjectCount(eApolloProgram) == 0)
	{
		int iLoop;
		CvCity* pLoopCity;
		CvCity* pApolloCity = NULL;
		for (pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
		{
			if (pLoopCity->getProductionProject() == eApolloProgram)
			{
				pApolloCity = pLoopCity;
				break;
			}
		}
		if (pApolloCity)
		{
			iHammerTurnsNeeded = pApolloCity->getGeneralProductionTurnsLeft();
		}
		else
		{
			int iMaxApolloProduction = 0;
			for (pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
			{
				int iThisApolloProduction = pLoopCity->getProjectProductionTimes100(eApolloProgram);
				if (iThisApolloProduction > iMaxApolloProduction)
				{
					iMaxApolloProduction = iThisApolloProduction;
				}
			}
			iHammerTurnsNeeded += m_pPlayer->getProductionNeeded(eApolloProgram) * 100 / MAX(iMaxApolloProduction, 1);
		}
	}
	else
	{
		// Next bit is a fairly big shortcut, but accounting for city-production combinations would be incredibly computationally expensive
		for (int iI = 0; iI < apSpaceshipParts.size(); iI++)
		{
			UnitTypes eLoopUnit = apSpaceshipParts.GetElement(iI);
			CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eLoopUnit);
			ProjectTypes eSSProject = (ProjectTypes)pkUnitInfo->GetSpaceshipProject();

			// How many of this part do we have yet to build?
			int iPartsNeeded = apSpaceshipParts.GetWeight(iI) - GET_TEAM(m_pPlayer->getTeam()).getProjectCount(eSSProject) 
				- GET_TEAM(m_pPlayer->getTeam()).getUnitClassCount((UnitClassTypes)pkUnitInfo->GetUnitClassType());

			// Skip if we already have all the parts we need for this SS part
			if (iPartsNeeded <= 0)
			{
				continue;
			}

			// Loop through cities to check if any one of them is building the project
			int iLoop;
			CvCity* pLoopCity;
			std::vector<CvCity*> apProducingCity;
			for (pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
			{
				if (pLoopCity->getProductionUnit() == eLoopUnit)
				{
					apProducingCity.push_back(pLoopCity);
					if ((int)apProducingCity.size() >= iPartsNeeded)
					{
						break;
					}
				}
			}
			iPartsNeeded -= apProducingCity.size();
			// TODO: account for production time of SS parts
			if (iPartsNeeded > 0)
			{
				int iLoop;
				CvCity* pLoopCity;
				int iMaxSSProduction = 0;
				for (pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
				{
					int iThisSSProduction = pLoopCity->getUnitProductionTimes100(eLoopUnit);
					if (iThisSSProduction > iMaxSSProduction)
					{
						iMaxSSProduction = iThisSSProduction;
					}
				}
				iHammerTurnsNeeded += iPartsNeeded * m_pPlayer->getProductionNeeded(eLoopUnit) * 100 
					/ MAX(iMaxSSProduction * MIN((int)apSpaceshipParts.GetTotalWeight(), m_pPlayer->getNumCities()), 1);
			}
		}
	}

	if (!bHasApolloTech)
	{
		iTurnsToVictory = iTechTurnsNeeded + iHammerTurnsNeeded;
	}
	else
	{
		iTurnsToVictory = MAX(iTechTurnsNeeded, iHammerTurnsNeeded);
	}

	/*
	const double dLogisticExpander = 10.0 / (double)GC.getGame().getEstimateEndTurn();
	// Logistic function, halfway point when iTurnsRemaining = iTurnsToVictory
	double fPriority = (double)GC.getFLAVOR_MAX_VALUE() / (1.0 + exp((double)(iTurnsToVictory - iTurnsRemaining) * dLogisticExpander));
	*/
	
	// These values are selected so that the overall fPriority is roughly 20% of its max when iTurnsToVictory = iTurnsRemaining * 2
	double dPriority = (double)GC.getFLAVOR_MAX_VALUE() * pow(MIN((double)MAX(iTurnsRemaining, 1) / (double)MAX(iTurnsToVictory, 1), 1.0), sqrt(5.0));

	return (dPriority * (1.0 - GetFlavorCurve(eGrandStrategy)));
}

#else
/// Get the base Priority for a Grand Strategy; these are elements common to ALL Grand Strategies
int CvGrandStrategyAI::GetBaseGrandStrategyPriority(AIGrandStrategyTypes eGrandStrategy)
{
	CvAIGrandStrategyXMLEntry* pGrandStrategy = GetAIGrandStrategies()->GetEntry(eGrandStrategy);

	int iPriority = 0;

	// Personality effect on Priority
	for(int iFlavorLoop = 0; iFlavorLoop < GC.getNumFlavorTypes(); iFlavorLoop++)
	{
		if(pGrandStrategy->GetFlavorValue(iFlavorLoop) != 0)
		{
			iPriority += (pGrandStrategy->GetFlavorValue(iFlavorLoop) * GetPlayer()->GetFlavorManager()->GetPersonalityIndividualFlavor((FlavorTypes) iFlavorLoop));
		}
	}

	return iPriority;

}
#endif AUI_GS_PRIORITY_OVERHAUL

#ifdef AUI_GS_SCIENCE_FLAVOR_BOOST
/// Do we need a science boost? Ie. do we still have yet to acquire a crucial tech?
int CvGrandStrategyAI::ScienceFlavorBoost() const
{
	int iMultiplier = AUI_GS_SCIENCE_FLAVOR_BOOST;
	bool bReturnHalf = false;
	// Makes sure AIs don't overvalue libraries early on when they could build shrines, monuments, etc.
	if ((int)m_pPlayer->GetCurrentEra() == GC.getInfoTypeForString("ERA_ANCIENT"))
	{
		return 1;
	}
	// Still need a bit of adjustment in Classical since Universities aren't enabled yet
	if ((int)m_pPlayer->GetCurrentEra() == GC.getInfoTypeForString("ERA_CLASSICAL"))
	{
		iMultiplier /= 2;
	}
	// Conquest gives science boost if in lower quarter of players tech-wise, half boost if only in lower half but not in lower quarter
#ifdef AUI_GS_PRIORITY_RATIO
	if (IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST")))
#else
	if (GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"))
#endif
	{
		if (m_pPlayer->GetPlayerTechs()->GetTechAI()->GetTechRatio() <= 0.25f)
		{
			return iMultiplier;
		}
		if (m_pPlayer->GetPlayerTechs()->GetTechAI()->GetTechRatio() <= 0.5f)
		{
			bReturnHalf = true;
		}
	}
	// Culture gives science boost if archaeology is still locked, half boost if we don't have Internet and wouldn't win a culture victory in time without it
#ifdef AUI_GS_PRIORITY_RATIO
	if (IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE")))
#else
	if (GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
#endif
	{
		UnitTypes eArchaeologist = (UnitTypes)GC.getInfoTypeForString("UNIT_ARCHAEOLOGIST", true);
		if (eArchaeologist != NO_UNIT)
		{
			if (!m_pPlayer->canTrain(eArchaeologist))
			{
				return iMultiplier;
			}
		}
		// Are we missing a tech that would give us extra tourism
		if (!m_pPlayer->GetPlayerTechs()->GetTechAI()->HaveAllInternetTechs())
		{
			bReturnHalf = true;
		}
	}
	// Diplomacy gives science boost if diplomatic victory still locked, half boost if we don't have Globalization and are one of the two candidates in the lead
#ifdef AUI_GS_PRIORITY_RATIO
	if (IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS")))
#else
	if (GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS"))
#endif
	{
		if (!GC.getGame().GetGameLeagues()->GetActiveLeague() || !GC.getGame().GetGameLeagues()->GetActiveLeague()->IsUnitedNations())
		{
			return iMultiplier;
		}
		else
		{
			CvLeague* pLeague = GC.getGame().GetGameLeagues()->GetActiveLeague();
			CvAssert(pLeague != NULL);
			// Are leagues active and are we missing a tech that would grant us extra votes?
			if (pLeague != NULL && !m_pPlayer->GetPlayerTechs()->GetTechAI()->HaveAllUNTechs())
			{
				// Calculate the two highest vote counts
				int iHighestPlayerVotes = 0;
				int iSecondHighestPlayerVotes = 0;
				for (int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
				{
					PlayerTypes eLoopPlayer = (PlayerTypes)iPlayerLoop;
					if (GET_PLAYER(eLoopPlayer).isAlive() && !GET_PLAYER(eLoopPlayer).isMinorCiv())
					{
						int iOtherPlayerVotes = pLeague->CalculateStartingVotesForMember(eLoopPlayer);
						if (iOtherPlayerVotes > iHighestPlayerVotes && iOtherPlayerVotes > iSecondHighestPlayerVotes)
						{
							iSecondHighestPlayerVotes = iHighestPlayerVotes;
							iHighestPlayerVotes = iOtherPlayerVotes;
						}
						else if (iOtherPlayerVotes > iSecondHighestPlayerVotes)
						{
							iSecondHighestPlayerVotes = iOtherPlayerVotes;
						}
					}
				}
				// Are we one of the two people with the highest vote count?
				if (pLeague->CalculateStartingVotesForMember(m_pPlayer->GetID()) >= iSecondHighestPlayerVotes)
				{
					bReturnHalf = true;
				}
			}
		}
	}
	// Spaceship gives science boost if we don't have Rocketry, half boost otherwise
#ifdef AUI_GS_PRIORITY_RATIO
	if (IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP")))
#else
	if (GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP"))
#endif
	{
		ProjectTypes eApolloProgram = (ProjectTypes)GC.getInfoTypeForString("PROJECT_APOLLO_PROGRAM", true);
		if (eApolloProgram != NO_PROJECT)
		{
			if (!m_pPlayer->canCreate(eApolloProgram) && GET_TEAM(m_pPlayer->getTeam()).getProjectCount(eApolloProgram) == 0)
			{
				return iMultiplier;
			}
		}
		bReturnHalf = true;
	}
	if (bReturnHalf)
	{
		return (iMultiplier / 2 + iMultiplier % 2);
	}
	// Always give at least a 2x multiplier if in last place for science (last two places if 12 or more players)
	if (m_pPlayer->GetPlayerTechs()->GetTechAI()->GetTechRatio() <= 1.0f/12.0f)
	{
		return 2;
	}
	return 1;
}
#endif

/// Get AI Flavor based on Personality and Grand Strategy Ratios
int CvGrandStrategyAI::GetPersonalityAndGrandStrategy(FlavorTypes eFlavorType)
{
#ifdef AUI_GS_GET_PERSONALITY_AND_GRAND_STRATEGY_USE_COMPARE_TO_LOWEST_RATIO
	double dLowestRatio = 1.0;
	for (int iI = 0; iI < GC.getNumAIGrandStrategyInfos(); iI++)
	{
#ifdef AUI_FAST_COMP
		dLowestRatio = FASTMIN(dLowestRatio, GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)iI));
#else
		dLowestRatio = MIN(dLowestRatio, GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)iI));
#endif
	}
	if (dLowestRatio == 1.0)
		dLowestRatio = 0.0;
#endif
#ifdef AUI_GS_USE_DOUBLES
	// Personality flavors set as initial values
	double dModdedFlavor = (double)m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor(eFlavorType);

# ifdef AUI_GS_PRIORITY_RATIO
#  ifdef AUI_GS_SINUSOID_PERSONALITY_INFLUENCE
	// Percent techs researched acts as Science Flavor booster and as measure of how much grand strategy flavor influencers should activate
	double dPercentTechsResearched = (double)GET_TEAM(m_pPlayer->getTeam()).GetTeamTechs()->GetNumTechsKnown();
	if (m_pPlayer->GetPlayerTechs()->GetTechs()->GetNumTechs() > 0)
	{
		dPercentTechsResearched /= (double)m_pPlayer->GetPlayerTechs()->GetTechs()->GetNumTechs(); // remove divide by 0
	}
	dPercentTechsResearched = MAX(0.0, MIN(1.0, dPercentTechsResearched));
#  else
	const double dPercentTechsResearched = 1.0;
#  endif

	// Loop through all Grand Strategies, adding their flavor values to the modded value
	for (int iI = 0; iI < GetAIGrandStrategies()->GetNumAIGrandStrategies(); iI++)
	{
		// First line is just the Grand Strategy's base flavor, second line reduces its influence on modded flavor based on Priority Ratio (Active = 1.0)
		dModdedFlavor += GetAIGrandStrategies()->GetEntry(iI)->GetFlavorModValue(eFlavorType) * sin(dPercentTechsResearched * M_PI / 2.0) *
#ifdef AUI_GS_GET_PERSONALITY_AND_GRAND_STRATEGY_USE_COMPARE_TO_LOWEST_RATIO
			(GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)iI) - dLowestRatio) / (1.0 - dLowestRatio);
#else
			GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)iGrandStrategiesLoop);
#endif
	}
# else
	if(m_eActiveGrandStrategy != NO_AIGRANDSTRATEGY)
	{
		CvAIGrandStrategyXMLEntry* pGrandStrategy = GetAIGrandStrategies()->GetEntry(m_eActiveGrandStrategy);
		dModdedFlavor = pGrandStrategy->GetFlavorModValue(eFlavorType) + m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor(eFlavorType);
	}
# endif
	// Modded flavor can't be negative
	dModdedFlavor = MAX((double)GC.getFLAVOR_MIN_VALUE(), dModdedFlavor);
	return int(dModdedFlavor + 0.5);
#else
	if(m_eActiveGrandStrategy != NO_AIGRANDSTRATEGY)
	{
		CvAIGrandStrategyXMLEntry* pGrandStrategy = GetAIGrandStrategies()->GetEntry(m_eActiveGrandStrategy);
		int iModdedFlavor = pGrandStrategy->GetFlavorModValue(eFlavorType) + m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor(eFlavorType);
		iModdedFlavor = max(0, iModdedFlavor);
		return iModdedFlavor;
	}
	return m_pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor(eFlavorType);
#endif
}

/// Returns the Active Grand Strategy for this Player: how am I trying to win right now?
AIGrandStrategyTypes CvGrandStrategyAI::GetActiveGrandStrategy() const
{
	return m_eActiveGrandStrategy;
}

/// Sets the Active Grand Strategy for this Player: how am I trying to win right now?
void CvGrandStrategyAI::SetActiveGrandStrategy(AIGrandStrategyTypes eGrandStrategy)
{
	if(eGrandStrategy != NO_AIGRANDSTRATEGY)
	{
		m_eActiveGrandStrategy = eGrandStrategy;

		SetNumTurnsSinceActiveSet(0);
	}
}

/// The number of turns since the Active Strategy was last set
int CvGrandStrategyAI::GetNumTurnsSinceActiveSet() const
{
	return m_iNumTurnsSinceActiveSet;
}

/// Set the number of turns since the Active Strategy was last set
void CvGrandStrategyAI::SetNumTurnsSinceActiveSet(int iValue)
{
	m_iNumTurnsSinceActiveSet = iValue;
	FAssert(m_iNumTurnsSinceActiveSet >= 0);
}

/// Change the number of turns since the Active Strategy was last set
void CvGrandStrategyAI::ChangeNumTurnsSinceActiveSet(int iChange)
{
	if(iChange != 0)
	{
		m_iNumTurnsSinceActiveSet += iChange;
	}

	FAssert(m_iNumTurnsSinceActiveSet >= 0);
}

/// Returns the Priority Level the player has for a particular Grand Strategy
int CvGrandStrategyAI::GetGrandStrategyPriority(AIGrandStrategyTypes eGrandStrategy) const
{
	FAssert(eGrandStrategy != NO_AIGRANDSTRATEGY);
	return m_paiGrandStrategyPriority[eGrandStrategy];
}

/// Sets the Priority Level the player has for a particular Grand Strategy
void CvGrandStrategyAI::SetGrandStrategyPriority(AIGrandStrategyTypes eGrandStrategy, int iValue)
{
	FAssert(eGrandStrategy != NO_AIGRANDSTRATEGY);
	m_paiGrandStrategyPriority[eGrandStrategy] = iValue;
}

/// Changes the Priority Level the player has for a particular Grand Strategy
void CvGrandStrategyAI::ChangeGrandStrategyPriority(AIGrandStrategyTypes eGrandStrategy, int iChange)
{
	FAssert(eGrandStrategy != NO_AIGRANDSTRATEGY);

	if(iChange != 0)
	{
		m_paiGrandStrategyPriority[eGrandStrategy] += iChange;
	}
}

#ifdef AUI_GS_PRIORITY_RATIO
#ifdef AUI_GS_PRIORITY_OVERHAUL
/// Returns how "focused" the AI is on a grand strategy, between 0 and 1: at 1, the GS is the only one the AI is considering, and at 0, the GS is in complete equlibrium 
double CvGrandStrategyAI::GetGrandStrategyPriorityRatio(AIGrandStrategyTypes eGrandStrategy) const
{
	if (m_pPlayer->isMinorCiv() || eGrandStrategy == NO_AIGRANDSTRATEGY)
	{
		return 0.0;
	}
	else
	{
		std::vector<double> aiOtherPriorities;
		int iGrandStrategiesLoop;
		int iGrandStrategyCount = 0;
		for (iGrandStrategiesLoop = 0; iGrandStrategiesLoop < GC.getNumAIGrandStrategyInfos(); iGrandStrategiesLoop++)
		{
			const AIGrandStrategyTypes eLoopGrandStrategy = (AIGrandStrategyTypes)iGrandStrategiesLoop;
			if (eGrandStrategy != eLoopGrandStrategy)
			{
				aiOtherPriorities.push_back(GetGrandStrategyPriorityRatioSingle(eLoopGrandStrategy));
				iGrandStrategyCount++;
			}
		}
		double dGeometricMean = 1.0;
		for (std::vector<double>::iterator it = aiOtherPriorities.begin(); it != aiOtherPriorities.end(); ++it)
		{
			if ((*it) != 0)
			{
				fGeometricMean *= (*it);
			}
			else
			{
				iGrandStrategyCount--;
			}
		}
		fGeometricMean = pow(dGeometricMean, 1.0 / (double)MAX(iGrandStrategyCount, 1));
		return (1.0 - fGeometricMean);
	}

}
/// Returns Fraction of the Priority of a Grand Strategy over Active Grand Strategy's Priority (between 0 and 1) 
double CvGrandStrategyAI::GetGrandStrategyPriorityRatioSingle(AIGrandStrategyTypes eGrandStrategy) const
{
	if (m_pPlayer->isMinorCiv() || eGrandStrategy == NO_AIGRANDSTRATEGY)
	{
		return 0.0;
	}
	else
	{
		if (eGrandStrategy == GetActiveGrandStrategy())
		{
			return 1.0;
		}
		else
		{
			return (double)MAX(GetGrandStrategyPriority(eGrandStrategy), 0) / (double)GetGrandStrategyPriority(GetActiveGrandStrategy());
		}
	}

}
/// Returns whether or not a Grand Strategy is considered significant
bool CvGrandStrategyAI::IsGrandStrategySignificant(AIGrandStrategyTypes eGrandStrategy) const
{
	return GetGrandStrategyPriorityRatio(eGrandStrategy) >= 0.1 + 0.4 * (double)GC.getGame().getEstimateEndTurn() / (double)GC.getGame().getElapsedGameTurns();
}
#else
/// Returns Fraction of the Priority of a Grand Strategy over Active Grand Strategy's Priority (between 0 and 1) 
double CvGrandStrategyAI::GetGrandStrategyPriorityRatio(AIGrandStrategyTypes eGrandStrategy) const
{
	if (m_pPlayer->isMinorCiv() || eGrandStrategy == NO_AIGRANDSTRATEGY)
	{
		return 0.0;
	}
	else
	{
		if (eGrandStrategy == GetActiveGrandStrategy())
		{
			return 1.0;
		}
		else
		{
			return (double)MAX(GetGrandStrategyPriority(eGrandStrategy), 0) / (double)MAX(GetGrandStrategyPriority(GetActiveGrandStrategy()), 0);
		}
	}
}
/// Returns whether or not a Grand Strategy is considered significant
bool CvGrandStrategyAI::IsGrandStrategySignificant(AIGrandStrategyTypes eGrandStrategy) const
{
	double dGSRatioTotal = 0;
	for (int iI = 0; iI < GC.getNumAIGrandStrategyInfos(); iI++)
	{
		dGSRatioTotal += GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)iI);
	}
	return GetGrandStrategyPriorityRatio(eGrandStrategy) * GC.getNumAIGrandStrategyInfos() >= dGSRatioTotal;
}
#endif
#endif


// **********
// Stuff relating to guessing what other Players are up to
// **********



/// Runs every turn to try and figure out what other known Players' Grand Strategies are
void CvGrandStrategyAI::DoGuessOtherPlayersActiveGrandStrategy()
{
	CvWeightedVector<int, 5, true> vGrandStrategyPriorities;
	FStaticVector< int, 5, true, c_eCiv5GameplayDLL >  vGrandStrategyPrioritiesForLogging;

	GuessConfidenceTypes eGuessConfidence = NO_GUESS_CONFIDENCE_TYPE;

	int iGrandStrategiesLoop = 0;
	AIGrandStrategyTypes eGrandStrategy = NO_AIGRANDSTRATEGY;
	CvAIGrandStrategyXMLEntry* pGrandStrategy = 0;
	CvString strGrandStrategyName;

	CvTeam& pTeam = GET_TEAM(GetPlayer()->getTeam());

	int iMajorLoop = 0;
	PlayerTypes eMajor = NO_PLAYER;

	int iPriority = 0;

	// Establish world Military strength average
	int iWorldMilitaryAverage = GC.getGame().GetWorldMilitaryStrengthAverage(GetPlayer()->GetID(), true, true);

	// Establish world culture and tourism averages
	int iNumPlayersAlive = 0;
	int iWorldCultureAverage = 0;
	int iWorldTourismAverage = 0;
	for(iMajorLoop = 0; iMajorLoop < MAX_MAJOR_CIVS; iMajorLoop++)
	{
		eMajor = (PlayerTypes) iMajorLoop;

		if(GET_PLAYER(eMajor).isAlive())
		{
			iWorldCultureAverage += GET_PLAYER(eMajor).GetJONSCultureEverGenerated();
			iWorldTourismAverage += GET_PLAYER(eMajor).GetCulture()->GetTourism();
			iNumPlayersAlive++;
		}
	}
	iWorldCultureAverage /= iNumPlayersAlive;
	iWorldTourismAverage /= iNumPlayersAlive;

	// Establish world Tech progress average
	iNumPlayersAlive = 0;
	int iWorldNumTechsAverage = 0;
	TeamTypes eTeam;
	for(int iTeamLoop = 0; iTeamLoop < MAX_MAJOR_CIVS; iTeamLoop++)	// Looping over all MAJOR teams
	{
		eTeam = (TeamTypes) iTeamLoop;

		if(GET_TEAM(eTeam).isAlive())
		{
			iWorldNumTechsAverage += GET_TEAM(eTeam).GetTeamTechs()->GetNumTechsKnown();
			iNumPlayersAlive++;
		}
	}
	iWorldNumTechsAverage /= iNumPlayersAlive;

	// Look at every Major we've met
	for(iMajorLoop = 0; iMajorLoop < MAX_MAJOR_CIVS; iMajorLoop++)
	{
		eMajor = (PlayerTypes) iMajorLoop;

		if(GET_PLAYER(eMajor).isAlive() && iMajorLoop != GetPlayer()->GetID())
		{
			if(pTeam.isHasMet(GET_PLAYER(eMajor).getTeam()))
			{
				for(iGrandStrategiesLoop = 0; iGrandStrategiesLoop < GetAIGrandStrategies()->GetNumAIGrandStrategies(); iGrandStrategiesLoop++)
				{
					eGrandStrategy = (AIGrandStrategyTypes) iGrandStrategiesLoop;
					pGrandStrategy = GetAIGrandStrategies()->GetEntry(iGrandStrategiesLoop);
					strGrandStrategyName = (CvString) pGrandStrategy->GetType();

					if(strGrandStrategyName == "AIGRANDSTRATEGY_CONQUEST")
					{
						iPriority = GetGuessOtherPlayerConquestPriority(eMajor, iWorldMilitaryAverage);
					}
					else if(strGrandStrategyName == "AIGRANDSTRATEGY_CULTURE")
					{
						iPriority = GetGuessOtherPlayerCulturePriority(eMajor, iWorldCultureAverage, iWorldTourismAverage);
					}
					else if(strGrandStrategyName == "AIGRANDSTRATEGY_UNITED_NATIONS")
					{
						iPriority = GetGuessOtherPlayerUnitedNationsPriority(eMajor);
					}
					else if(strGrandStrategyName == "AIGRANDSTRATEGY_SPACESHIP")
					{
						iPriority = GetGuessOtherPlayerSpaceshipPriority(eMajor, iWorldNumTechsAverage);
					}

					vGrandStrategyPriorities.push_back(iGrandStrategiesLoop, iPriority);
					vGrandStrategyPrioritiesForLogging.push_back(iPriority);
				}

				if(vGrandStrategyPriorities.size() > 0)
				{
					// Add "No Grand Strategy" in case we just don't have enough info to go on
					iPriority = /*40*/ GC.getAI_GRAND_STRATEGY_GUESS_NO_CLUE_WEIGHT();

					vGrandStrategyPriorities.push_back(NO_AIGRANDSTRATEGY, iPriority);
					vGrandStrategyPrioritiesForLogging.push_back(iPriority);

					vGrandStrategyPriorities.SortItems();

					eGrandStrategy = (AIGrandStrategyTypes) vGrandStrategyPriorities.GetElement(0);
					iPriority = vGrandStrategyPriorities.GetWeight(0);
					eGuessConfidence = NO_GUESS_CONFIDENCE_TYPE;

					// How confident are we in our Guess?
					if(eGrandStrategy != NO_AIGRANDSTRATEGY)
					{
						if(iPriority >= /*120*/ GC.getAI_GRAND_STRATEGY_GUESS_POSITIVE_THRESHOLD())
						{
							eGuessConfidence = GUESS_CONFIDENCE_POSITIVE;
						}
						else if(iPriority >= /*70*/ GC.getAI_GRAND_STRATEGY_GUESS_LIKELY_THRESHOLD())
						{
							eGuessConfidence = GUESS_CONFIDENCE_LIKELY;
						}
						else
						{
							eGuessConfidence = GUESS_CONFIDENCE_UNSURE;
						}
					}

					SetGuessOtherPlayerActiveGrandStrategy(eMajor, eGrandStrategy, eGuessConfidence);

					LogGuessOtherPlayerGrandStrategy(vGrandStrategyPrioritiesForLogging, eMajor);
				}

				vGrandStrategyPriorities.clear();
				vGrandStrategyPrioritiesForLogging.clear();
			}
		}
	}
}

/// What does this AI BELIEVE another player's Active Grand Strategy to be?
AIGrandStrategyTypes CvGrandStrategyAI::GetGuessOtherPlayerActiveGrandStrategy(PlayerTypes ePlayer) const
{
	FAssert(ePlayer < MAX_MAJOR_CIVS);
	return (AIGrandStrategyTypes) m_eGuessOtherPlayerActiveGrandStrategy[ePlayer];
}

/// How confident is the AI in its guess of what another player's Active Grand Strategy is?
GuessConfidenceTypes CvGrandStrategyAI::GetGuessOtherPlayerActiveGrandStrategyConfidence(PlayerTypes ePlayer) const
{
	FAssert(ePlayer < MAX_MAJOR_CIVS);
	return (GuessConfidenceTypes) m_eGuessOtherPlayerActiveGrandStrategyConfidence[ePlayer];
}

/// Sets what this AI BELIEVES another player's Active Grand Strategy to be
void CvGrandStrategyAI::SetGuessOtherPlayerActiveGrandStrategy(PlayerTypes ePlayer, AIGrandStrategyTypes eGrandStrategy, GuessConfidenceTypes eGuessConfidence)
{
	FAssert(ePlayer < MAX_MAJOR_CIVS);
	m_eGuessOtherPlayerActiveGrandStrategy[ePlayer] = eGrandStrategy;
	m_eGuessOtherPlayerActiveGrandStrategyConfidence[ePlayer] = eGuessConfidence;
}

/// Guess as to how much another Player is prioritizing Conquest as his means of winning the game
int CvGrandStrategyAI::GetGuessOtherPlayerConquestPriority(PlayerTypes ePlayer, int iWorldMilitaryAverage)
{
	int iConquestPriority = 0;

	// Compare their Military to the world average; Possible range is 100 to -100 (but will typically be around -20 to 20)
	if(iWorldMilitaryAverage > 0)
	{
		iConquestPriority += (GET_PLAYER(ePlayer).GetMilitaryMight() - iWorldMilitaryAverage) * /*100*/ GC.getAI_GRAND_STRATEGY_CONQUEST_POWER_RATIO_MULTIPLIER() / iWorldMilitaryAverage;
	}

	// Minors attacked
	iConquestPriority += (GetPlayer()->GetDiplomacyAI()->GetOtherPlayerNumMinorsAttacked(ePlayer) * /*5*/ GC.getAI_GRAND_STRATEGY_CONQUEST_WEIGHT_PER_MINOR_ATTACKED());

	// Minors Conquered
	iConquestPriority += (GetPlayer()->GetDiplomacyAI()->GetOtherPlayerNumMinorsConquered(ePlayer) * /*10*/ GC.getAI_GRAND_STRATEGY_CONQUEST_WEIGHT_PER_MINOR_CONQUERED());

	// Majors attacked
	iConquestPriority += (GetPlayer()->GetDiplomacyAI()->GetOtherPlayerNumMajorsAttacked(ePlayer) * /*10*/ GC.getAI_GRAND_STRATEGY_CONQUEST_WEIGHT_PER_MAJOR_ATTACKED());

	// Majors Conquered
	iConquestPriority += (GetPlayer()->GetDiplomacyAI()->GetOtherPlayerNumMajorsConquered(ePlayer) * /*15*/ GC.getAI_GRAND_STRATEGY_CONQUEST_WEIGHT_PER_MAJOR_CONQUERED());

	return iConquestPriority;
}

/// Guess as to how much another Player is prioritizing Culture as his means of winning the game
int CvGrandStrategyAI::GetGuessOtherPlayerCulturePriority(PlayerTypes ePlayer, int iWorldCultureAverage, int iWorldTourismAverage)
{
	VictoryTypes eVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_CULTURAL", true);

	// If Culture Victory isn't even available then don't bother with anything
	if(eVictory == NO_VICTORY)
	{
		return -100;
	}

	int iCulturePriority = 0;
	int iRatio;

	// Compare their Culture to the world average; Possible range is 75 to -75
	if(iWorldCultureAverage > 0)
	{
		iRatio = (GET_PLAYER(ePlayer).GetJONSCultureEverGenerated() - iWorldCultureAverage) * /*75*/ GC.getAI_GS_CULTURE_RATIO_MULTIPLIER() / iWorldCultureAverage;
		if (iRatio > GC.getAI_GS_CULTURE_RATIO_MULTIPLIER())
		{
			iCulturePriority += GC.getAI_GS_CULTURE_RATIO_MULTIPLIER();
		}
		else if (iRatio < -GC.getAI_GS_CULTURE_RATIO_MULTIPLIER())
		{
			iCulturePriority += -GC.getAI_GS_CULTURE_RATIO_MULTIPLIER();
		}
		iCulturePriority += iRatio;
	}

	// Compare their Tourism to the world average; Possible range is 75 to -75
	if(iWorldTourismAverage > 0)
	{
		iRatio = (GET_PLAYER(ePlayer).GetCulture()->GetTourism() - iWorldTourismAverage) * /*75*/ GC.getAI_GS_TOURISM_RATIO_MULTIPLIER() / iWorldTourismAverage;
		if (iRatio > GC.getAI_GS_TOURISM_RATIO_MULTIPLIER())
		{
			iCulturePriority += GC.getAI_GS_TOURISM_RATIO_MULTIPLIER();
		}
		else if (iRatio < -GC.getAI_GS_TOURISM_RATIO_MULTIPLIER())
		{
			iCulturePriority += -GC.getAI_GS_TOURISM_RATIO_MULTIPLIER();
		}
		iCulturePriority += iRatio;
	}

	return iCulturePriority;
}

/// Guess as to how much another Player is prioritizing the UN as his means of winning the game
int CvGrandStrategyAI::GetGuessOtherPlayerUnitedNationsPriority(PlayerTypes ePlayer)
{
	VictoryTypes eVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_DIPLOMATIC", true);

	// If UN Victory isn't even available then don't bother with anything
	if(eVictory == NO_VICTORY)
	{
		return -100;
	}

	int iTheirCityStateAllies = 0;
	int iTheirCityStateFriends = 0;
	int iCityStatesAlive = 0;
	for(int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
	{
		PlayerTypes eLoopPlayer = (PlayerTypes) iPlayerLoop;

		if (eLoopPlayer != ePlayer && GET_PLAYER(eLoopPlayer).isAlive() && GET_PLAYER(eLoopPlayer).isMinorCiv())
		{
			iCityStatesAlive++;
			if (GET_PLAYER(eLoopPlayer).GetMinorCivAI()->IsAllies(ePlayer))
			{
				iTheirCityStateAllies++;
			}
			else if (GET_PLAYER(eLoopPlayer).GetMinorCivAI()->IsFriends(ePlayer))
			{
				iTheirCityStateFriends++;
			}
		}
	}
	iCityStatesAlive = MAX(iCityStatesAlive, 1);

	int iPriority = iTheirCityStateAllies + (iTheirCityStateFriends / 3);
	iPriority = iPriority * GC.getAI_GS_UN_SECURED_VOTE_MOD();
	iPriority = iPriority / iCityStatesAlive;

	return iPriority;
}

/// Guess as to how much another Player is prioritizing the SS as his means of winning the game
int CvGrandStrategyAI::GetGuessOtherPlayerSpaceshipPriority(PlayerTypes ePlayer, int iWorldNumTechsAverage)
{
	VictoryTypes eVictory = (VictoryTypes) GC.getInfoTypeForString("VICTORY_SPACE_RACE", true);

	// If SS Victory isn't even available then don't bother with anything
	if(eVictory == NO_VICTORY)
	{
		return -100;
	}

	TeamTypes eTeam = GET_PLAYER(ePlayer).getTeam();

	// If the player has the Apollo Program we're pretty sure he's going for the SS
	ProjectTypes eApolloProgram = (ProjectTypes) GC.getInfoTypeForString("PROJECT_APOLLO_PROGRAM", true);
	if(eApolloProgram != NO_PROJECT)
	{
		if(GET_TEAM(eTeam).getProjectCount(eApolloProgram) > 0)
		{
			return /*150*/ GC.getAI_GS_SS_HAS_APOLLO_PROGRAM();
		}
	}

	int iNumTechs = GET_TEAM(eTeam).GetTeamTechs()->GetNumTechsKnown();

	// Don't divide by zero, okay?
	if(iWorldNumTechsAverage == 0)
		iWorldNumTechsAverage = 1;

	int iSSPriority = (iNumTechs - iWorldNumTechsAverage) * /*300*/ GC.getAI_GS_SS_TECH_PROGRESS_MOD() / iWorldNumTechsAverage;

	return iSSPriority;
}


// PRIVATE METHODS

/// Log GrandStrategy state: what are the Priorities and who is Active?
#ifdef AUI_GS_BETTER_LOGGING
void CvGrandStrategyAI::LogGrandStrategies(const FStaticVector< int, 5, true, c_eCiv5GameplayDLL >& /*vModifiedGrandStrategyPriorities*/)
#else
void CvGrandStrategyAI::LogGrandStrategies(const FStaticVector< int, 5, true, c_eCiv5GameplayDLL >& vModifiedGrandStrategyPriorities)
#endif
{
	if(GC.getLogging() && GC.getAILogging())
	{
		CvString strOutBuf;
		CvString strBaseString;
		CvString strTemp;
		CvString playerName;
		CvString strDesc;
		CvString strLogName;

		// Find the name of this civ and city
		playerName = GetPlayer()->getCivilizationShortDescription();

		// Open the log file
		if(GC.getPlayerAndCityAILogSplit())
		{
			strLogName = "GrandStrategyAI_Log_" + playerName + ".csv";
		}
		else
		{
			strLogName = "GrandStrategyAI_Log.csv";
		}

		FILogFile* pLog;
		pLog = LOGFILEMGR.GetLog(strLogName, FILogFile::kDontTimeStamp);

		AIGrandStrategyTypes eGrandStrategy;

#ifdef AUI_GS_BETTER_LOGGING

		// Format top of log to help with Excel graph creation
		if (GC.getGame().getGameTurn() == 0 || (GC.getGame().getGameTurn() == 1 && m_pPlayer->GetID() == 0))
		{
			strBaseString = "Player, Turn, ";
			// Loop through Grand Strategies
			for (int iGrandStrategyLoop = 0; iGrandStrategyLoop < GC.getNumAIGrandStrategyInfos(); iGrandStrategyLoop++)
			{
				eGrandStrategy = (AIGrandStrategyTypes)iGrandStrategyLoop;
				CvAIGrandStrategyXMLEntry* pEntry = GC.getAIGrandStrategyInfo(eGrandStrategy);
				const char* szAIGrandStrategyType = (pEntry != NULL) ? pEntry->GetType() : "Unknown Type";

				strTemp.Format("%s, ", szAIGrandStrategyType);
				strBaseString += strTemp;
			}
			strBaseString += "Active GS";
			pLog->Msg(strBaseString);
		}
		
		strBaseString = playerName + ", ";
		strTemp.Format("%03d, ", GC.getGame().getElapsedGameTurns());
		strBaseString += strTemp;
		for (int iGrandStrategyLoop = 0; iGrandStrategyLoop < GC.getNumAIGrandStrategyInfos(); iGrandStrategyLoop++)
		{
			eGrandStrategy = (AIGrandStrategyTypes)iGrandStrategyLoop;

			strTemp.Format("%d, ", GetGrandStrategyPriority(eGrandStrategy));
			strBaseString += strTemp;
		}
		CvAIGrandStrategyXMLEntry* pActiveGS = GC.getAIGrandStrategyInfo(GetActiveGrandStrategy());
		const char* szAIGrandStrategyType = (pActiveGS != NULL)? pActiveGS->GetType() : "Unknown Type";

		strTemp.Format("%s, ", szAIGrandStrategyType);
		strOutBuf = strBaseString + strTemp;
		pLog->Msg(strOutBuf);
		
#else

		// Loop through Grand Strategies
		for(int iGrandStrategyLoop = 0; iGrandStrategyLoop < GC.getNumAIGrandStrategyInfos(); iGrandStrategyLoop++)
		{
			// Get the leading info for this line
			strBaseString.Format("%03d, ", GC.getGame().getElapsedGameTurns());
			strBaseString += playerName + ", ";

			eGrandStrategy = (AIGrandStrategyTypes) iGrandStrategyLoop;

			// GrandStrategy Info
			CvAIGrandStrategyXMLEntry* pEntry = GC.getAIGrandStrategyInfo(eGrandStrategy);
			const char* szAIGrandStrategyType = (pEntry != NULL)? pEntry->GetType() : "Unknown Type";

			if(GetActiveGrandStrategy() == eGrandStrategy)
			{
				strTemp.Format("*** %s, %d, %d", szAIGrandStrategyType, GetGrandStrategyPriority(eGrandStrategy), vModifiedGrandStrategyPriorities[eGrandStrategy]);
			}
			else
			{
				strTemp.Format("%s, %d, %d", szAIGrandStrategyType, GetGrandStrategyPriority(eGrandStrategy), vModifiedGrandStrategyPriorities[eGrandStrategy]);
			}
			strOutBuf = strBaseString + strTemp;
			pLog->Msg(strOutBuf);
		}

#endif \\ AUI_GS_BETTER_LOGGING
	}
}

/// Log our guess as to other Players' Active Grand Strategy
void CvGrandStrategyAI::LogGuessOtherPlayerGrandStrategy(const FStaticVector< int, 5, true, c_eCiv5GameplayDLL >& vGrandStrategyPriorities, PlayerTypes ePlayer)
{
	if(GC.getLogging() && GC.getAILogging())
	{
		CvString strOutBuf;
		CvString strBaseString;
		CvString strTemp;
		CvString playerName;
		CvString otherPlayerName;
		CvString strDesc;
		CvString strLogName;

		// Find the name of this civ and city
		playerName = GetPlayer()->getCivilizationShortDescription();

		// Open the log file
		if(GC.getPlayerAndCityAILogSplit())
		{
			strLogName = "GrandStrategyAI_Guess_Log_" + playerName + ".csv";
		}
		else
		{
			strLogName = "GrandStrategyAI_Guess_Log.csv";
		}

		FILogFile* pLog;
		pLog = LOGFILEMGR.GetLog(strLogName, FILogFile::kDontTimeStamp);

		AIGrandStrategyTypes eGrandStrategy;
		int iPriority;

		// Loop through Grand Strategies
		for(int iGrandStrategyLoop = 0; iGrandStrategyLoop < GC.getNumAIGrandStrategyInfos(); iGrandStrategyLoop++)
		{
			// Get the leading info for this line
			strBaseString.Format("%03d, ", GC.getGame().getElapsedGameTurns());
			strBaseString += playerName + ", ";

			eGrandStrategy = (AIGrandStrategyTypes) iGrandStrategyLoop;
			iPriority = vGrandStrategyPriorities[iGrandStrategyLoop];

			CvAIGrandStrategyXMLEntry* pEntry = GC.getAIGrandStrategyInfo(eGrandStrategy);
			const char* szGrandStrategyType = (pEntry != NULL)? pEntry->GetType() : "Unknown Strategy";

			// GrandStrategy Info
			if(GetActiveGrandStrategy() == eGrandStrategy)
			{
				strTemp.Format("*** %s, %d", szGrandStrategyType, iPriority);
			}
			else
			{
				strTemp.Format("%s, %d", szGrandStrategyType, iPriority);
			}
			otherPlayerName = GET_PLAYER(ePlayer).getCivilizationShortDescription();
			strOutBuf = strBaseString + otherPlayerName + ", " + strTemp;

			if(GetGuessOtherPlayerActiveGrandStrategy(ePlayer) == eGrandStrategy)
			{
				// Confidence in our guess
				switch(GetGuessOtherPlayerActiveGrandStrategyConfidence(ePlayer))
				{
				case GUESS_CONFIDENCE_POSITIVE:
					strTemp.Format("Positive");
					break;
				case GUESS_CONFIDENCE_LIKELY:
					strTemp.Format("Likely");
					break;
				case GUESS_CONFIDENCE_UNSURE:
					strTemp.Format("Unsure");
					break;
				default:
					strTemp.Format("XXX");
					break;
				}

				strOutBuf += ", " + strTemp;
			}

			pLog->Msg(strOutBuf);
		}

		// One more entry for NO GRAND STRATEGY
		// Get the leading info for this line
		strBaseString.Format("%03d, ", GC.getGame().getElapsedGameTurns());
		strBaseString += playerName + ", ";

		iPriority = vGrandStrategyPriorities[GC.getNumAIGrandStrategyInfos()];

		// GrandStrategy Info
		strTemp.Format("NO_GRAND_STRATEGY, %d", iPriority);
		otherPlayerName = GET_PLAYER(ePlayer).getCivilizationShortDescription();
		strOutBuf = strBaseString + otherPlayerName + ", " + strTemp;
		pLog->Msg(strOutBuf);
	}
}