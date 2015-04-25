/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */

#include "CvGameCoreDLLPCH.h"
#include "CvPlayerAI.h"
#include "CvRandom.h"
#include "CvGlobals.h"
#include "CvGameCoreUtils.h"
#include "CvMap.h"
#include "CvArea.h"
#include "CvPlot.h"
#include "CvTeam.h"
#include "CvGameCoreUtils.h"
#include "ICvDLLUserInterface.h"
#include "CvInfos.h"
#include "CvAStar.h"
#include "CvDiplomacyAI.h"
#include "CvGrandStrategyAI.h"
#include "CvTechAI.h"
#include "CvDangerPlots.h"
#include "CvImprovementClasses.h"
#include "CvTacticalAnalysisMap.h"
#include "CvMilitaryAI.h"
#include "CvWonderProductionAI.h"
#include "CvCitySpecializationAI.h"
#include "cvStopWatch.h"
#include "CvEconomicAI.h"

// Include this after all other headers.
#include "LintFree.h"

#ifdef AUI_PLAYERAI_FREE_GP_CULTURE
#include "CvTypes.h"
#endif

#define DANGER_RANGE				(6)

// statics

CvPlayerAI* CvPlayerAI::m_aPlayers = NULL;

void CvPlayerAI::initStatics()
{
	m_aPlayers = FNEW(CvPlayerAI[MAX_PLAYERS], c_eCiv5GameplayDLL, 0);
	for(int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		m_aPlayers[iI].m_eID = ((PlayerTypes)iI);
	}
}

void CvPlayerAI::freeStatics()
{
	SAFE_DELETE_ARRAY(m_aPlayers);
}

// Public Functions...
CvPlayerAI::CvPlayerAI()
{
	AI_reset();
}


CvPlayerAI::~CvPlayerAI()
{
	AI_uninit();
}


void CvPlayerAI::AI_init()
{
	AI_reset();
}


void CvPlayerAI::AI_uninit()
{
}


void CvPlayerAI::AI_reset()
{
	AI_uninit();
}

void CvPlayerAI::AI_doTurnPre()
{
	AI_PERF_FORMAT("AI-perf.csv", ("CvPlayerAI::AI_doTurnPre, Turn %03d, %s", GC.getGame().getElapsedGameTurns(), getCivilizationShortDescription()) );
	CvAssertMsg(getPersonalityType() != NO_LEADER, "getPersonalityType() is not expected to be equal with NO_LEADER");
	CvAssertMsg(getLeaderType() != NO_LEADER, "getLeaderType() is not expected to be equal with NO_LEADER");
	CvAssertMsg(getCivilizationType() != NO_CIVILIZATION, "getCivilizationType() is not expected to be equal with NO_CIVILIZATION");

	if(isHuman())
	{
		return;
	}

	AI_updateFoundValues();

	AI_doResearch();
	AI_considerAnnex();
}


void CvPlayerAI::AI_doTurnPost()
{
	AI_PERF_FORMAT("AI-perf.csv", ("CvPlayerAI::AI_doTurnPost, Turn %03d, %s", GC.getGame().getElapsedGameTurns(), getCivilizationShortDescription()) );
	if(isHuman())
	{
		return;
	}

	if(isBarbarian())
	{
		return;
	}

	if(isMinorCiv())
	{
		return;
	}

	for(int i = 0; i < GC.getNumVictoryInfos(); ++i)
	{
		AI_launch((VictoryTypes)i);
	}

	ProcessGreatPeople();
	GetEspionageAI()->DoTurn();
	GetTradeAI()->DoTurn();
}


void CvPlayerAI::AI_doTurnUnitsPre()
{
#ifndef AUI_QUEUED_ATTACKS_REMOVED
	GetTacticalAI()->InitializeQueuedAttacks();
#endif

	if(isHuman())
	{
		return;
	}

	if(isBarbarian())
	{
		return;
	}
}


void CvPlayerAI::AI_doTurnUnitsPost()
{
	CvUnit* pLoopUnit;
	int iLoop;

	if(!isHuman())
	{
		for(pLoopUnit = firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = nextUnit(&iLoop))
		{
			pLoopUnit->AI_promote();
		}
	}
}

void CvPlayerAI::AI_updateFoundValues(bool bStartingLoc)
{
	int iGoodEnoughToBeWorthOurTime = GC.getAI_STRATEGY_MINIMUM_SETTLE_FERTILITY();
	int iLoop;
	const int iNumPlots = GC.getMap().numPlots();
	for(CvArea* pLoopArea = GC.getMap().firstArea(&iLoop); pLoopArea != NULL; pLoopArea = GC.getMap().nextArea(&iLoop))
	{
		pLoopArea->setTotalFoundValue(0);
	}

	const PlayerTypes eID = GetID();
	if(bStartingLoc)
	{
		for(int iI = 0; iI < iNumPlots; iI++)
		{
			GC.getMap().plotByIndexUnchecked(iI)->setFoundValue(eID, -1);
		}
	}
	else
	{
		const TeamTypes eTeam = getTeam();
		GC.getGame().GetSettlerSiteEvaluator()->ComputeFlavorMultipliers(this);
		for (int iI = 0; iI < iNumPlots; iI++)
		{
			CvPlot* pLoopPlot = GC.getMap().plotByIndexUnchecked(iI);

			if (pLoopPlot->isRevealed(eTeam))
			{
				const int iValue = GC.getGame().GetSettlerSiteEvaluator()->PlotFoundValue(pLoopPlot, this, NO_YIELD, false);
				pLoopPlot->setFoundValue(eID, iValue);
				if (iValue >= iGoodEnoughToBeWorthOurTime)
				{
					CvArea* pLoopArea = GC.getMap().getArea(pLoopPlot->getArea());
					if(pLoopArea && !pLoopArea->isWater())
					{
#ifdef AUI_PLAYERAI_FIX_UPDATE_FOUND_VALUES_NOT_ADDITIVE
#ifdef AUI_FAST_COMP
						pLoopArea->setTotalFoundValue(FASTMAX(pLoopArea->getTotalFoundValue(), iValue));
#else
						pLoopArea->setTotalFoundValue(MAX(pLoopArea->getTotalFoundValue(), iValue));
#endif
#else
						pLoopArea->setTotalFoundValue(pLoopArea->getTotalFoundValue() + iValue);
#endif
					}
				}
			}
			else
			{
				pLoopPlot->setFoundValue(eID, -1);
			}
		}
	}
}

//	---------------------------------------------------------------------------
void CvPlayerAI::AI_unitUpdate()
{
	GC.getPathFinder().ForceReset();
	GC.getIgnoreUnitsPathFinder().ForceReset();
	GC.getRouteFinder().ForceReset();
	GC.GetWaterRouteFinder().ForceReset();

	// Set individual pathers as MP cache safe.  A global for all pathers might be simpler,
	// but this will allow selective control in case one type of pather is causing out-of-syncs.
	bool bCommonPathFinderMPCaching = GC.getPathFinder().SetMPCacheSafe(true);
	bool bIgnoreUnitsPathFinderMPCaching = GC.getIgnoreUnitsPathFinder().SetMPCacheSafe(true);
	bool bTacticalPathFinderMPCaching = GC.GetTacticalAnalysisMapFinder().SetMPCacheSafe(true);
	bool bInfluencePathFinderMPCaching = GC.getInfluenceFinder().SetMPCacheSafe(true);
	bool bRoutePathFinderMPCaching = GC.getRouteFinder().SetMPCacheSafe(true);
	bool bWaterRoutePathFinderMPCaching = GC.GetWaterRouteFinder().SetMPCacheSafe(true);

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(GetID());

		bool bResult;
		LuaSupport::CallHook(pkScriptSystem, "PlayerPreAIUnitUpdate", args.get(), bResult);
	}

	//GC.getGame().GetTacticalAnalysisMap()->RefreshDataForNextPlayer(this);

	// this was a !hasBusyUnit around the entire rest of the function, so I tried to make it a bit flatter.
	if(hasBusyUnitOrCity())
	{
		return;
	}

	if(isHuman())
	{
		CvUnit::dispatchingNetMessage(true);
		// The homeland AI goes first.
		GetHomelandAI()->FindAutomatedUnits();
		GetHomelandAI()->Update();
		CvUnit::dispatchingNetMessage(false);
	}
	else
	{
		// Update tactical AI
		GetTacticalAI()->CommandeerUnits();

		// Now let the tactical AI run.  Putting it after the operations update allows units who have
		// just been handed off to the tactical AI to get a move in the same turn they switch between
		// AI subsystems
		GetTacticalAI()->Update();

		// Skip homeland AI processing if a barbarian
		if(m_eID != BARBARIAN_PLAYER)
		{
			// Now its the homeland AI's turn.
			GetHomelandAI()->RecruitUnits();
			GetHomelandAI()->Update();
		}
	}

	GC.getPathFinder().SetMPCacheSafe(bCommonPathFinderMPCaching);
	GC.getIgnoreUnitsPathFinder().SetMPCacheSafe(bIgnoreUnitsPathFinderMPCaching);
	GC.GetTacticalAnalysisMapFinder().SetMPCacheSafe(bTacticalPathFinderMPCaching);
	GC.getInfluenceFinder().SetMPCacheSafe(bInfluencePathFinderMPCaching);
	GC.getRouteFinder().SetMPCacheSafe(bRoutePathFinderMPCaching);
	GC.GetWaterRouteFinder().SetMPCacheSafe(bWaterRoutePathFinderMPCaching);
}


void CvPlayerAI::AI_conquerCity(CvCity* pCity, PlayerTypes eOldOwner)
{
	PlayerTypes eOriginalOwner = pCity->getOriginalOwner();
	TeamTypes eOldOwnerTeam = GET_PLAYER(eOldOwner).getTeam();

#ifdef AUI_PLAYERAI_CONQUER_CITY_TWEAKED_RAZE
	bool bGetsFreeCourthouse = false;
	bool bGetsFasterCourthouse = false;
	BuildingTypes eCourthouseBuildingType = NO_BUILDING;
	BuildingClassTypes eCourthouseType = NO_BUILDINGCLASS;
	// find courthouse
	for (int eBuildingType = 0; eBuildingType < GC.getNumBuildingInfos(); eBuildingType++)
	{
		const BuildingTypes eBuilding = static_cast<BuildingTypes>(eBuildingType);
		CvBuildingEntry* buildingInfo = GC.getBuildingInfo(eBuilding);

		if (buildingInfo)
		{
			if (buildingInfo->IsNoOccupiedUnhappiness())
			{
				eCourthouseBuildingType = eBuilding;
				eCourthouseType = (BuildingClassTypes)buildingInfo->GetBuildingClassType();
				break;
			}
		}
	}
	if (eCourthouseBuildingType != NO_BUILDING)
	{
		std::vector<BuildingTypes> freeBuildings = GetPlayerPolicies()->GetFreeBuildingsOnConquest();
		for (std::vector<BuildingTypes>::iterator it = freeBuildings.begin(); it != freeBuildings.end(); ++it)
		{
			if ((*it) == eCourthouseBuildingType)
			{
				bGetsFreeCourthouse = true;
				break;
			}
		}
	}
	if (eCourthouseType != NO_BUILDINGCLASS)
	{
		if (GetPlayerPolicies()->GetBuildingClassProductionModifier(eCourthouseType) > 0)
		{
			bGetsFasterCourthouse = true;
		}
	}
#endif

	// Liberate a city?
	if(eOriginalOwner != eOldOwner && eOriginalOwner != GetID() && CanLiberatePlayerCity(eOriginalOwner))
	{
		// minor civ
		if(GET_PLAYER(eOriginalOwner).isMinorCiv())
		{
			if(GetDiplomacyAI()->DoPossibleMinorLiberation(eOriginalOwner, pCity->GetID()))
				return;
		}
		else // major civ
		{
			bool bLiberate = false;
			if (GET_PLAYER(eOriginalOwner).isAlive())
			{
				// If the original owner and this player have a defensive pact
				// and both the original owner and the player are at war with the old owner of this city
				// give the city back to the original owner
				TeamTypes eOriginalOwnerTeam = GET_PLAYER(eOriginalOwner).getTeam();
				if (GET_TEAM(getTeam()).IsHasDefensivePact(eOriginalOwnerTeam) && GET_TEAM(getTeam()).isAtWar(eOldOwnerTeam) && GET_TEAM(eOriginalOwnerTeam).isAtWar(eOldOwnerTeam))
				{
					bLiberate = true;
				}
				// if the player is a friend and we're going for diplo victory, then liberate to score some friend points
#ifdef AUI_GS_PRIORITY_RATIO
				else if (GetDiplomacyAI()->IsDoFAccepted(eOriginalOwner) && 
					GetGrandStrategyAI()->IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS")))
#else
				else if (GetDiplomacyAI()->IsDoFAccepted(eOriginalOwner) && GetDiplomacyAI()->IsGoingForDiploVictory())
#endif
				{
					bLiberate = true;
				}
			}
			// if the player isn't human and we're going for diplo victory, resurrect players to get super diplo bonuses
#ifdef AUI_GS_PRIORITY_RATIO
			else if (!GET_PLAYER(eOriginalOwner).isHuman() && 
				GetGrandStrategyAI()->IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS")))
#else
			else if (!GET_PLAYER(eOriginalOwner).isHuman() && GetDiplomacyAI()->IsGoingForDiploVictory())
#endif
			{
				bLiberate = true;
			}

			if (bLiberate)
			{
				DoLiberatePlayer(eOriginalOwner, pCity->GetID());
				return;
			}
		}
	}

	// Do we want to burn this city down?
	if(canRaze(pCity))
	{
		// Burn the city if the empire is unhappy - keeping the city will only make things worse or if map hint dictates
		// Huns will burn down everything possible once they have a core of a few cities (was 3, but this put Attila out of the running long term as a conqueror)
#ifdef AUI_PLAYERAI_CONQUER_CITY_TWEAKED_RAZE
		if (GetHappiness() - GetUnhappiness(NULL, pCity) <= GC.getSUPER_UNHAPPY_THRESHOLD() || (GC.getMap().GetAIMapHint() & 2) ||
			(!bGetsFreeCourthouse && !bGetsFasterCourthouse && GetHappiness() - GetUnhappiness(NULL, pCity) <= GC.getVERY_UNHAPPY_THRESHOLD()) ||
			(GetPlayerTraits()->GetRazeSpeedModifier() > 0 && GetHappiness() - GetUnhappiness(pCity, NULL) <= GC.getVERY_UNHAPPY_THRESHOLD() && !bGetsFreeCourthouse && !bGetsFasterCourthouse))
#else
		if (IsEmpireUnhappy() || (GC.getMap().GetAIMapHint() & 2) || (GetPlayerTraits()->GetRazeSpeedModifier() > 0 && getNumCities() >= 3 + (GC.getGame().getGameTurn() / 100)) )
#endif
		{
			pCity->doTask(TASK_RAZE);
			return;
		}
	}

	// Puppet the city
	if(pCity->getOriginalOwner() != GetID() || GET_PLAYER(m_eID).GetPlayerTraits()->IsNoAnnexing())
	{
		pCity->DoCreatePuppet();
	}
}

bool CvPlayerAI::AI_captureUnit(UnitTypes, CvPlot* pPlot)
{
	CvCity* pNearestCity;

	CvAssert(!isHuman());

	// Barbs always capture
	if (isBarbarian())
		return true;

	// we own it
	if (pPlot->getTeam() == getTeam())
		return true;

	// no man's land - may as well
	if (pPlot->getTeam() == NO_TEAM)
		return true;

	// friendly, sure (okay, this is pretty much just means open borders)
	if (pPlot->IsFriendlyTerritory(GetID()))
		return true;

	// not friendly, but "near" us
	pNearestCity = GC.getMap().findCity(pPlot->getX(), pPlot->getY(), NO_PLAYER, getTeam());
	if (pNearestCity != NULL)
	{
		if (plotDistance(pPlot->getX(), pPlot->getY(), pNearestCity->getX(), pNearestCity->getY()) <= 7)
			return true;
	}

	// very near someone we aren't friends with (and far from our nearest city)
	pNearestCity = GC.getMap().findCity(pPlot->getX(), pPlot->getY());
	if (pNearestCity != NULL)
	{
		if (plotDistance(pPlot->getX(), pPlot->getY(), pNearestCity->getX(), pNearestCity->getY()) <= 4)
			return false;
	}

	// I'd rather we grab it and run than destroy it
	return true;
}

int CvPlayerAI::AI_foundValue(int iX, int iY, int, bool bStartingLoc)
{

	CvPlot* pPlot;
	int rtnValue = 0;

	pPlot = GC.getMap().plot(iX, iY);

	if(bStartingLoc)
	{
		rtnValue =  GC.getGame().GetStartSiteEvaluator()->PlotFoundValue(pPlot, this);
	}
	else
	{
		GC.getGame().GetSettlerSiteEvaluator()->ComputeFlavorMultipliers(this);
		rtnValue =  GC.getGame().GetSettlerSiteEvaluator()->PlotFoundValue(pPlot, this, NO_YIELD, false);
	}

	return rtnValue;
}

void CvPlayerAI::AI_chooseFreeGreatPerson()
{
	while(GetNumFreeGreatPeople() > 0)
	{
		UnitTypes eDesiredGreatPerson = NO_UNIT;

#ifdef AUI_PLAYERAI_FREE_GP_EARLY_PROPHET
		if (!GetReligions()->HasCreatedReligion() && GC.getGame().GetGameReligions()->GetNumReligionsStillToFound() > 0)
		{
			eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_PROPHET");
		}
#ifdef AUI_PLAYERAI_FREE_GP_VENETIAN_MERCHANT
		// Venice often chooses Great Merchant after early prophet is ruled out
		else if (!GET_PLAYER(GetID()).GreatMerchantWantsCash())
		{
#ifdef AUI_GS_SCIENCE_FLAVOR_BOOST
			if (AUI_GS_SCIENCE_FLAVOR_BOOST > GetGrandStrategyAI()->ScienceFlavorBoost())
			{
				eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_MERCHANT");
			}
#else
			eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_MERCHANT");
#endif
		}
#endif
		// Highly wonder competitive and still early in game?
#ifdef AUI_PLAYERAI_FREE_GP_DYNAMIC_WONDER_COMPETITIVENESS
		else if (GetDiplomacyAI()->GetWonderCompetitiveness() >= int(double(GC.getGame().getGameTurn() * AUI_PLAYERAI_FREE_GP_DYNAMIC_WONDER_COMPETITIVENESS) / (double)GC.getGame().getEstimateEndTurn() + 0.5))
#else
		else if (GetDiplomacyAI()->GetWonderCompetitiveness() >= 8 && GC.getGame().getGameTurn() <= (GC.getGame().getEstimateEndTurn() / 2))
#endif
#else
#ifdef AUI_PLAYERAI_FREE_GP_VENETIAN_MERCHANT
		// Venice often chooses Great Merchant after early prophet is ruled out
		else if (GET_PLAYER(m_eID).GetPlayerTraits()->IsNoAnnexing())
		{
#ifdef AUI_GS_SCIENCE_FLAVOR_BOOST
			if (AUI_GS_SCIENCE_FLAVOR_BOOST > GetGrandStrategyAI()->ScienceFlavorBoost())
			{
				eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_MERCHANT");
			}
#else
			eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_MERCHANT");
#endif
		}
		// Highly wonder competitive and still early in game?
#ifdef AUI_PLAYERAI_FREE_GP_DYNAMIC_WONDER_COMPETITIVENESS
		else if (GetDiplomacyAI()->GetWonderCompetitiveness() >= int(double(GC.getGame().getGameTurn() * AUI_PLAYERAI_FREE_GP_DYNAMIC_WONDER_COMPETITIVENESS) / (double)GC.getGame().getEstimateEndTurn() + 0.5))
#else
		else if (GetDiplomacyAI()->GetWonderCompetitiveness() >= 8 && GC.getGame().getGameTurn() <= (GC.getGame().getEstimateEndTurn() / 2))
#endif
#else
		// Highly wonder competitive and still early in game?
#ifdef AUI_PLAYERAI_FREE_GP_DYNAMIC_WONDER_COMPETITIVENESS
		else if (GetDiplomacyAI()->GetWonderCompetitiveness() >= int(double(GC.getGame().getGameTurn() * AUI_PLAYERAI_FREE_GP_DYNAMIC_WONDER_COMPETITIVENESS) / (double)GC.getGame().getEstimateEndTurn() + 0.5))
#else
		if(GetDiplomacyAI()->GetWonderCompetitiveness() >= 8 && GC.getGame().getGameTurn() <= (GC.getGame().getEstimateEndTurn() / 2))
#endif
#endif
#endif
		{
			eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_ENGINEER");
		}
		else
		{
			// Pick the person based on our victory method
			AIGrandStrategyTypes eVictoryStrategy = GetGrandStrategyAI()->GetActiveGrandStrategy();
#ifdef AUI_GS_SCIENCE_FLAVOR_BOOST
			if (eVictoryStrategy == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP") || 
				AUI_GS_SCIENCE_FLAVOR_BOOST == GetGrandStrategyAI()->ScienceFlavorBoost())
			{
				eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_SCIENTIST");
			}
			else if (eVictoryStrategy == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
			{
#ifdef AUI_PLAYERAI_FREE_GP_CULTURE
				if (GetCulture()->HasAvailableGreatWorkSlot(CvTypes::getGREAT_WORK_SLOT_ART_ARTIFACT()))
				{
					eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_ARTIST");
				}
				else if (GetCulture()->HasAvailableGreatWorkSlot(CvTypes::getGREAT_WORK_SLOT_MUSIC()))
				{
					eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_MUSICIAN");
				}
				else if (GetCulture()->HasAvailableGreatWorkSlot(CvTypes::getGREAT_WORK_SLOT_LITERATURE()))
				{
					eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_WRITER");
				}
				else
				{
					int iArtistCount = GetNumUnitsWithUnitAI(UNITAI_ARTIST);
					int iMusicianCount = GetNumUnitsWithUnitAI(UNITAI_MUSICIAN);
					int iWriterCount = GetNumUnitsWithUnitAI(UNITAI_WRITER);
#ifdef AUI_FAST_COMP
					if (iArtistCount < FASTMAX(iWriterCount, iMusicianCount))
#else
					if (iArtistCount < MAX(iWriterCount, iMusicianCount))
#endif
					{
						eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_ARTIST");
					}
#ifdef AUI_FAST_COMP
					else if (iWriterCount < FASTMAX(iArtistCount, iMusicianCount))
#else
					else if (iWriterCount < MAX(iArtistCount, iMusicianCount))
#endif
					{
						eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_WRITER");
					}
					else
					{
						eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_MUSICIAN");
					}
				}
#else
				eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_ARTIST");
#endif
			}
			else if (eVictoryStrategy == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS"))
			{
				if (GetGrandStrategyAI()->ScienceFlavorBoost() > 1)
				{
					eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_SCIENTIST");
				}
				else
				{
					eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_MERCHANT");
				}
			}
			else if(eVictoryStrategy == (AIGrandStrategyTypes) GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"))
			{
				if (GetGrandStrategyAI()->ScienceFlavorBoost() > 1 && GetNumUnitsWithUnitAI(UNITAI_GENERAL) > 1)
				{
					eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_SCIENTIST");
				}
				else
				{
					eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_GREAT_GENERAL");
				}
			}
#else
			if(eVictoryStrategy == (AIGrandStrategyTypes) GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"))
			{
				eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_GREAT_GENERAL");
			}
			else if(eVictoryStrategy == (AIGrandStrategyTypes) GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
			{
#ifdef AUI_PLAYERAI_FREE_GP_CULTURE
				if (GetCulture()->HasAvailableGreatWorkSlot(CvTypes::getGREAT_WORK_SLOT_ART_ARTIFACT()))
				{
					eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_ARTIST");
				}
				else if (GetCulture()->HasAvailableGreatWorkSlot(CvTypes::getGREAT_WORK_SLOT_MUSIC()))
				{
					eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_MUSICIAN");
				}
				else if (GetCulture()->HasAvailableGreatWorkSlot(CvTypes::getGREAT_WORK_SLOT_LITERATURE()))
				{
					eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_WRITER");
				}
				else
				{
					int iArtistCount = GetNumUnitsWithUnitAI(UNITAI_ARTIST);
					int iMusicianCount = GetNumUnitsWithUnitAI(UNITAI_MUSICIAN);
					int iWriterCount = GetNumUnitsWithUnitAI(UNITAI_WRITER);
#ifdef AUI_FAST_COMP
					if (iArtistCount < FASTMAX(iWriterCount, iMusicianCount))
#else
					if (iArtistCount < MAX(iWriterCount, iMusicianCount))
#endif
					{
						eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_ARTIST");
					}
#ifdef AUI_FAST_COMP
					else if (iWriterCount < FASTMAX(iArtistCount, iMusicianCount))
#else
					else if (iWriterCount < MAX(iArtistCount, iMusicianCount))
#endif
					{
						eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_WRITER");
					}
					else
					{
						eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_MUSICIAN");
					}
				}
#else
				eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_ARTIST");
#endif
			}
			else if(eVictoryStrategy == (AIGrandStrategyTypes) GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS"))
			{
				eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_MERCHANT");
			}
			else if(eVictoryStrategy == (AIGrandStrategyTypes) GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP"))
			{
				eDesiredGreatPerson = (UnitTypes)GC.getInfoTypeForString("UNIT_SCIENTIST");
			}
#endif
		}

		if(eDesiredGreatPerson != NO_UNIT)
		{
			CvCity* pCapital = getCapitalCity();
			if(pCapital)
			{
				pCapital->GetCityCitizens()->DoSpawnGreatPerson(eDesiredGreatPerson, true, false);
			}
			ChangeNumFreeGreatPeople(-1);
		}
		else
		{
			break;
		}
	}
}

void CvPlayerAI::AI_chooseFreeTech()
{
	TechTypes eBestTech = NO_TECH;

	clearResearchQueue();

	// TODO: script override

	if(eBestTech == NO_TECH)
	{
		eBestTech = GetPlayerTechs()->GetTechAI()->ChooseNextTech(this, /*bFreeTech*/ true);
	}

	if(eBestTech != NO_TECH)
	{
		GET_TEAM(getTeam()).setHasTech(eBestTech, true, GetID(), true, true);
	}
}


void CvPlayerAI::AI_chooseResearch()
{
#ifdef AUI_PERF_LOGGING_FORMATTING_TWEAKS
	AI_PERF_FORMAT("AI-perf.csv", ("AI_chooseResearch, Turn %03d, %s", GC.getGame().getGameTurn(), getCivilizationShortDescription()));
#else
	AI_PERF("AI-perf.csv", "AI_chooseResearch");
#endif

	TechTypes eBestTech = NO_TECH;
	int iI;

	clearResearchQueue();

	if(GetPlayerTechs()->GetCurrentResearch() == NO_TECH)
	{
		for(iI = 0; iI < MAX_PLAYERS; iI++)
		{
			if(GET_PLAYER((PlayerTypes)iI).isAlive())
			{
				if((iI != GetID()) && (GET_PLAYER((PlayerTypes)iI).getTeam() == getTeam()))
				{
					if(GET_PLAYER((PlayerTypes)iI).GetPlayerTechs()->GetCurrentResearch() != NO_TECH)
					{
						if(GetPlayerTechs()->CanResearch(GET_PLAYER((PlayerTypes)iI).GetPlayerTechs()->GetCurrentResearch()))
						{
							pushResearch(GET_PLAYER((PlayerTypes)iI).GetPlayerTechs()->GetCurrentResearch());
						}
					}
				}
			}
		}
	}

	if(GetPlayerTechs()->GetCurrentResearch() == NO_TECH)
	{
		//todo: script override

		if(eBestTech == NO_TECH)
		{
			eBestTech = GetPlayerTechs()->GetTechAI()->ChooseNextTech(this);
		}

		if(eBestTech != NO_TECH)
		{
			pushResearch(eBestTech);
		}
	}
}

// sort player numbers
struct CityAndProduction
{
	CvCity* pCity;
	int iProduction;
};

struct CityAndProductionEval
{
	bool operator()(CityAndProduction const& a, CityAndProduction const& b) const
	{
		return (a.iProduction > b.iProduction);
	}
};

void CvPlayerAI::AI_considerAnnex()
{
#ifdef AUI_PERF_LOGGING_FORMATTING_TWEAKS
	AI_PERF_FORMAT("AI-perf.csv", ("AI_considerAnnex, Turn %03d, %s", GC.getGame().getGameTurn(), getCivilizationShortDescription()));
#else
	AI_PERF("AI-perf.csv", "AI_ considerAnnex");
#endif

#ifdef AUI_PLAYERAI_DO_ANNEX_QUICK_FILTER
	// for Venice and City States
	if (GetPlayerTraits()->IsNoAnnexing() || isMinorCiv())
	{
		return;
	}
#endif

#ifdef AUI_PLAYERAI_DO_ANNEX_CONSIDERS_FREE_COURTHOUSE
	bool bGetsFreeCourthouse = false;
	bool bGetsFasterCourthouse = false;
	std::vector<BuildingTypes> aeCourthouseBuildingTypes;
	std::vector<BuildingClassTypes> aeCourthouseTypes;
	// find courthouse
	for (int eBuildingType = 0; eBuildingType < GC.getNumBuildingInfos(); eBuildingType++)
	{
		const BuildingTypes eBuilding = static_cast<BuildingTypes>(eBuildingType);
		CvBuildingEntry* buildingInfo = GC.getBuildingInfo(eBuilding);

		if (buildingInfo)
		{
			if (buildingInfo->IsNoOccupiedUnhappiness())
			{
				aeCourthouseBuildingTypes.push_back(eBuilding);
				aeCourthouseTypes.push_back((BuildingClassTypes)buildingInfo->GetBuildingClassType());
			}
		}
	}
	if (aeCourthouseBuildingTypes.size() > 0)
	{
		std::vector<BuildingTypes> freeBuildings = GetPlayerPolicies()->GetFreeBuildingsOnConquest();
		for (std::vector<BuildingTypes>::iterator it = freeBuildings.begin(); it != freeBuildings.end(); ++it)
		{
			for (std::vector<BuildingTypes>::iterator jt = aeCourthouseBuildingTypes.begin(); jt != aeCourthouseBuildingTypes.end(); ++jt)
			{
				if ((*it) == (*jt))
				{
					bGetsFreeCourthouse = true;
					goto skipRestCourthouseBuildingTypes;
				}		
			}
		}
	}
	skipRestCourthouseBuildingTypes:
	if (aeCourthouseTypes.size() > 0)
	{
		for (std::vector<BuildingClassTypes>::iterator it = aeCourthouseTypes.begin(); it != aeCourthouseTypes.end(); ++it)
		{
			if (GetPlayerPolicies()->GetBuildingClassProductionModifier((*it)) > 0)
			{
				bGetsFasterCourthouse = true;
				break;
			}
		}
	}

	// if the empire is unhappy, don't consider annexing
#ifdef AUI_PLAYERAI_DO_ANNEX_MORE_AGGRESSIVE
	if (!bGetsFreeCourthouse && IsEmpireVeryUnhappy())
#else
	if (!bGetsFreeCourthouse && IsEmpireUnhappy())
#endif
#else
	// if the empire is unhappy, don't consider annexing
#ifdef AUI_PLAYERAI_DO_ANNEX_MORE_AGGRESSIVE
	if (IsEmpireVeryUnhappy())
#else
	if (IsEmpireUnhappy())
#endif
#endif
	{
		return;
	}

#ifndef AUI_PLAYERAI_DO_ANNEX_IGNORES_CULTURAL_STRATEGY
	// if we're going for a culture victory, don't consider annexing
#ifdef AUI_GS_PRIORITY_RATIO
	if (GetGrandStrategyAI()->IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE")))
#else
	if (GetDiplomacyAI()->IsGoingForCultureVictory())
#endif
	{
		return;
	}
#endif

#ifndef AUI_PLAYERAI_DO_ANNEX_QUICK_FILTER
	// for Venice
	if (GetPlayerTraits()->IsNoAnnexing())
	{
		return;
	}
#endif

#ifdef AUI_PLAYERAI_DO_ANNEX_CONSIDERS_FREE_COURTHOUSE
	if (bGetsFreeCourthouse)
	{
		// Annex all the cities!
		int iLoop;
		CvCity* pLoopCity;
		for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
		{
			if (pLoopCity && pLoopCity->IsPuppet() && !pLoopCity->IsResistance())
			{
				pLoopCity->DoAnnex();
			}
		}
		// All eligible puppet cities are now annexed
		return;
	}
#endif

	// if their capital city is puppeted, annex it
	CvCity* pCity = getCapitalCity();
	if (pCity && pCity->IsPuppet())
	{
		// we should only annex one city a turn, and sense this is one, we're done!
		pCity->DoAnnex();
#ifndef AUI_PLAYERAI_FIX_DO_ANNEX_NO_STOP_AFTER_CAPITAL_ANNEX
		return;
#endif
	}

	std::vector<CityAndProduction> aCityAndProductions;
	int iLoop = 0;
	pCity = NULL;

	// Find first coastal city in same area as settler
	for(pCity = firstCity(&iLoop); pCity != NULL; pCity = nextCity(&iLoop))
	{
		CityAndProduction kEval;
		kEval.pCity = pCity;
		kEval.iProduction = pCity->getYieldRateTimes100(YIELD_PRODUCTION, false);
		aCityAndProductions.push_back(kEval);
	}
	
	std::stable_sort(aCityAndProductions.begin(), aCityAndProductions.end(), CityAndProductionEval());
	
	CvCity* pTargetCity = NULL;
	float fCutoffValue = GC.getNORMAL_ANNEX();
#ifdef AUI_PLAYERAI_DO_ANNEX_CONSIDERS_FREE_COURTHOUSE
	if (bGetsFasterCourthouse)
#else
	BuildingClassTypes eCourthouseType = NO_BUILDINGCLASS;
	// find courthouse
	for(int eBuildingType = 0; eBuildingType < GC.getNumBuildingInfos(); eBuildingType++)
	{
		const BuildingTypes eBuilding = static_cast<BuildingTypes>(eBuildingType);
		CvBuildingEntry* buildingInfo = GC.getBuildingInfo(eBuilding);

		if(buildingInfo)
		{
			if (buildingInfo->IsNoOccupiedUnhappiness())
			{
				eCourthouseType = (BuildingClassTypes)buildingInfo->GetBuildingClassType();
				break;
			}
		}
	}

	bool bCourthouseImprovement = false;
	if (eCourthouseType != NO_BUILDINGCLASS)
	{
		if (GetPlayerPolicies()->GetBuildingClassProductionModifier(eCourthouseType) > 0)
		{
			bCourthouseImprovement = true;
		}
	}

	if (bCourthouseImprovement)
#endif
	{
		fCutoffValue = GC.getAGGRESIVE_ANNEX();
	}

	uint uiCutOff = (uint)(aCityAndProductions.size() * fCutoffValue);
	for (uint ui = 0; ui < uiCutOff; ui++)
	{
#ifdef AUI_PLAYERAI_DO_ANNEX_MORE_AGGRESSIVE
#ifdef AUI_PLAYERAI_FIX_DO_ANNEX_CHECK_FOR_RESISTANCE
		if (aCityAndProductions[ui].pCity->IsPuppet() && !aCityAndProductions[ui].pCity->IsResistance() && GetHappiness() - GetUnhappiness(aCityAndProductions[ui].pCity) >= GC.getVERY_UNHAPPY_THRESHOLD())
#else
		if (aCityAndProductions[ui].pCity->IsPuppet() && GetHappiness() - GetUnhappiness(aCityAndProductions[ui].pCity) >= GC.getVERY_UNHAPPY_THRESHOLD())
#endif
#else
#ifdef AUI_PLAYERAI_FIX_DO_ANNEX_CHECK_FOR_RESISTANCE
		if (aCityAndProductions[ui].pCity->IsPuppet() && !aCityAndProductions[ui].pCity->IsResistance())
#else
		if (aCityAndProductions[ui].pCity->IsPuppet())
#endif
#endif
		{
			pTargetCity = aCityAndProductions[ui].pCity;
			break;
		}
	}

	if (pTargetCity)
	{
#ifndef AUI_PLAYERAI_FIX_DO_ANNEX_CHECK_FOR_RESISTANCE
		if (!pTargetCity->IsResistance())
#endif
		{
			pTargetCity->DoAnnex();
		}
	}
}

int CvPlayerAI::AI_plotTargetMissionAIs(CvPlot* pPlot, MissionAITypes eMissionAI, int iRange)
{
	int iCount = 0;

	int iLoop;
	for(CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit; pLoopUnit = nextUnit(&iLoop))
	{
		CvPlot* pMissionPlot = pLoopUnit->GetMissionAIPlot();
		if(!pMissionPlot)
		{
			continue;
		}

		MissionAITypes eGroupMissionAI = pLoopUnit->GetMissionAIType();
		if(eGroupMissionAI != eMissionAI)
		{
			continue;
		}

		int iDistance = plotDistance(pPlot->getX(), pPlot->getY(), pMissionPlot->getX(), pMissionPlot->getY());
		if(iDistance == iRange)
		{
			iCount++;
		}
	}

	return iCount;
}

// Protected Functions...

void CvPlayerAI::AI_doResearch()
{
	CvAssertMsg(!isHuman(), "isHuman did not return false as expected");

	if(GetPlayerTechs()->GetCurrentResearch() == NO_TECH)
	{
		AI_chooseResearch();
		//AI_forceUpdateStrategies(); //to account for current research.
	}
}


//
// read object from a stream
// used during load
//
void CvPlayerAI::Read(FDataStream& kStream)
{
	CvPlayer::Read(kStream);	// read base class data first

	// Version number to maintain backwards compatibility
	uint uiVersion;
	kStream >> uiVersion;
}


//
// save object to a stream
// used during save
//
void CvPlayerAI::Write(FDataStream& kStream) const
{
	CvPlayer::Write(kStream);	// write base class data first

	// Current version number
	uint uiVersion = 1;
	kStream << uiVersion;
}

void CvPlayerAI::AI_launch(VictoryTypes eVictory)
{
	if(GET_TEAM(getTeam()).isHuman())
	{
		return;
	}

	if(!GET_TEAM(getTeam()).canLaunch(eVictory))
	{
		return;
	}

	launch(eVictory);
}

OperationSlot CvPlayerAI::PeekAtNextUnitToBuildForOperationSlot(int iAreaID)
{
	OperationSlot thisSlot;

	// search through our operations till we find one that needs a unit
	std::map<int, CvAIOperation*>::iterator iter;
	for(iter = m_AIOperations.begin(); iter != m_AIOperations.end(); ++iter)
	{
		CvAIOperation* pThisOperation = iter->second;
		if(pThisOperation)
		{
			thisSlot = pThisOperation->PeekAtNextUnitToBuild(iAreaID);
			if(thisSlot.IsValid())
			{
				break;
			}
		}
	}

	return thisSlot;
}


OperationSlot CvPlayerAI::CityCommitToBuildUnitForOperationSlot(int iAreaID, int iTurns, CvCity* pCity)
{
	OperationSlot thisSlot;

	// search through our operations till we find one that needs a unit
	std::map<int, CvAIOperation*>::iterator iter;
	for(iter = m_AIOperations.begin(); iter != m_AIOperations.end(); ++iter)
	{
		CvAIOperation* pThisOperation = iter->second;
		if(pThisOperation)
		{
			thisSlot = pThisOperation->CommitToBuildNextUnit(iAreaID, iTurns, pCity);
			if(thisSlot.IsValid())
			{
				break;
			}
		}
	}

	return thisSlot;
}

void CvPlayerAI::CityUncommitToBuildUnitForOperationSlot(OperationSlot thisSlot)
{
	// find this operation
	CvAIOperation* pThisOperation = getAIOperation(thisSlot.m_iOperationID);
	if(pThisOperation)
	{
		pThisOperation->UncommitToBuild(thisSlot);
	}
}

void CvPlayerAI::CityFinishedBuildingUnitForOperationSlot(OperationSlot thisSlot, CvUnit* pThisUnit)
{
	// find this operation
	CvAIOperation* pThisOperation = getAIOperation(thisSlot.m_iOperationID);
	CvArmyAI* pThisArmy = getArmyAI(thisSlot.m_iArmyID);
	if(pThisOperation && pThisArmy && pThisUnit)
	{
		pThisArmy->AddUnit(pThisUnit->GetID(), thisSlot.m_iSlotID);
		pThisOperation->FinishedBuilding(thisSlot);
	}
}

int CvPlayerAI::GetNumUnitsNeededToBeBuilt()
{
	int iRtnValue = 0;

	std::map<int, CvAIOperation*>::iterator iter;
	for(iter = m_AIOperations.begin(); iter != m_AIOperations.end(); ++iter)
	{
		CvAIOperation* pThisOperation = iter->second;
		if(pThisOperation)
		{
			iRtnValue += pThisOperation->GetNumUnitsNeededToBeBuilt();
		}
	}

	return iRtnValue;
}

void CvPlayerAI::ProcessGreatPeople(void)
{
	SpecialUnitTypes eSpecialUnitGreatPerson = (SpecialUnitTypes) GC.getInfoTypeForString("SPECIALUNIT_PEOPLE");

	CvAssert(isAlive());

	if(!isAlive())
		return;

	int iLoop;
	for(CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit; pLoopUnit = nextUnit(&iLoop))
	{
		if(pLoopUnit->getSpecialUnitType() != eSpecialUnitGreatPerson)
		{
			continue;
		}

		GreatPeopleDirectiveTypes eDirective = NO_GREAT_PEOPLE_DIRECTIVE_TYPE;
		switch(pLoopUnit->AI_getUnitAIType())
		{
		case UNITAI_WRITER:
			eDirective = GetDirectiveWriter(pLoopUnit);
			break;
		case UNITAI_ARTIST:
			eDirective = GetDirectiveArtist(pLoopUnit);
			break;
		case UNITAI_MUSICIAN:
			eDirective = GetDirectiveMusician(pLoopUnit);
			break;
		case UNITAI_ENGINEER:
			eDirective = GetDirectiveEngineer(pLoopUnit);
			break;
		case UNITAI_MERCHANT:
			eDirective = GetDirectiveMerchant(pLoopUnit);
			break;
		case UNITAI_SCIENTIST:
			eDirective = GetDirectiveScientist(pLoopUnit);
			break;
		case UNITAI_GENERAL:
			eDirective = GetDirectiveGeneral(pLoopUnit);
			break;
		case UNITAI_PROPHET:
			eDirective = GetDirectiveProphet(pLoopUnit);
			break;
		case UNITAI_ADMIRAL:
			eDirective = GetDirectiveAdmiral(pLoopUnit);
			break;
		}

		pLoopUnit->SetGreatPeopleDirective(eDirective);
	}
}

bool PreparingForWar(CvPlayerAI* pPlayer)
{
	CvAssertMsg(pPlayer, "Need a player");
	if(!pPlayer)
	{
		return false;
	}
	CvMilitaryAI* pMilitaryAI = pPlayer->GetMilitaryAI();
	CvAssertMsg(pMilitaryAI, "No military AI");
	if(!pMilitaryAI)
	{
		return false;
	}

	MilitaryAIStrategyTypes eWarMobilizationStrategy = (MilitaryAIStrategyTypes)GC.getInfoTypeForString("MILITARYAISTRATEGY_WAR_MOBILIZATION");
	if(pMilitaryAI->IsUsingStrategy(eWarMobilizationStrategy))
	{
		return true;
	}

	return false;
}

bool IsSafe(CvPlayerAI* pPlayer)
{
	CvAssertMsg(pPlayer, "Need a player");
	if(!pPlayer)
	{
		return false;
	}

	if(pPlayer->GetDiplomacyAI()->GetStateAllWars() == STATE_ALL_WARS_WINNING)
	{
		return true;
	}
	else
	{
		CvMilitaryAI* pMilitaryAI = pPlayer->GetMilitaryAI();
		CvAssertMsg(pMilitaryAI, "No military AI");
		if(!pMilitaryAI)
		{
			return false;
		}

		MilitaryAIStrategyTypes eAtWarStrategy = (MilitaryAIStrategyTypes)GC.getInfoTypeForString("MILITARYAISTRATEGY_AT_WAR");
		if(!pMilitaryAI->IsUsingStrategy(eAtWarStrategy))
		{
			return true;
		}

		return false;
	}
}

GreatPeopleDirectiveTypes CvPlayerAI::GetDirectiveWriter(CvUnit* pGreatWriter)
{
	GreatPeopleDirectiveTypes eDirective = NO_GREAT_PEOPLE_DIRECTIVE_TYPE;

	// Defend against ideology pressure if not going for culture win
#ifdef AUI_GS_PRIORITY_RATIO
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE &&
		!GetGrandStrategyAI()->IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE")) &&
		GetCulture()->GetPublicOpinionUnhappiness() > 10)
#else
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && !GetDiplomacyAI()->IsGoingForCultureVictory() && GetCulture()->GetPublicOpinionUnhappiness() > 10)
#endif
	{
		eDirective = GREAT_PEOPLE_DIRECTIVE_CULTURE_BLAST;
	}

	// If not going for culture win and a Level 2 or 3 Tenet is available, try to snag it
#ifdef AUI_GS_PRIORITY_RATIO
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && 
		!GetGrandStrategyAI()->IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))  &&
		GetPlayerPolicies()->CanGetAdvancedTenet())
#else
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && !GetDiplomacyAI()->IsGoingForCultureVictory() && GetPlayerPolicies()->CanGetAdvancedTenet())
#endif
	{
		eDirective = GREAT_PEOPLE_DIRECTIVE_CULTURE_BLAST;
	}

	// Create Great Work if there is a slot
	GreatWorkType eGreatWork = pGreatWriter->GetGreatWork();
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && GetEconomicAI()->GetBestGreatWorkCity(pGreatWriter->plot(), eGreatWork))
	{
		eDirective = GREAT_PEOPLE_DIRECTIVE_USE_POWER;
	}
	else
	{
		eDirective = GREAT_PEOPLE_DIRECTIVE_CULTURE_BLAST;
	}

	return eDirective;
}

GreatPeopleDirectiveTypes CvPlayerAI::GetDirectiveArtist(CvUnit* pGreatArtist)
{
	GreatPeopleDirectiveTypes eDirective = NO_GREAT_PEOPLE_DIRECTIVE_TYPE;

	// Defend against ideology pressure if not going for culture win
#ifdef AUI_GS_PRIORITY_RATIO
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && 
		!GetGrandStrategyAI()->IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
		&& GetCulture()->GetPublicOpinionUnhappiness() > 10)
#else
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && !GetDiplomacyAI()->IsGoingForCultureVictory() && GetCulture()->GetPublicOpinionUnhappiness() > 10)
#endif
	{
		eDirective = GREAT_PEOPLE_DIRECTIVE_GOLDEN_AGE;
	}

	// If prepping for war, Golden Age will build units quickly
#ifdef AUI_GS_PRIORITY_RATIO
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && 
		!GetGrandStrategyAI()->IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE")) && PreparingForWar(this))
#else
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && !GetDiplomacyAI()->IsGoingForCultureVictory() && PreparingForWar(this))
#endif
	{
		eDirective = GREAT_PEOPLE_DIRECTIVE_GOLDEN_AGE;
	}

	// If finishing up spaceship parts, Golden Age will help build those quickly
#ifdef AUI_GS_PRIORITY_RATIO
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && 
		GetGrandStrategyAI()->IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP"))
		&& EconomicAIHelpers::IsTestStrategy_GS_SpaceshipHomestretch(this))
#else
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && GetDiplomacyAI()->IsGoingForSpaceshipVictory() && EconomicAIHelpers::IsTestStrategy_GS_SpaceshipHomestretch(this))
#endif
	{
		eDirective = GREAT_PEOPLE_DIRECTIVE_GOLDEN_AGE;
	}

	// If Persia and I'm at war, get a Golden Age going
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && GetPlayerTraits()->GetGoldenAgeMoveChange() > 0 && GetMilitaryAI()->GetNumberCivsAtWarWith() > 1 && !isGoldenAge())
	{
		eDirective = GREAT_PEOPLE_DIRECTIVE_GOLDEN_AGE;
	}

	// If Brazil and we're closing in on Culture Victory
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && GetPlayerTraits()->GetGoldenAgeTourismModifier() > 0 && GetCulture()->GetNumCivsInfluentialOn() > 0)
	{
		eDirective = GREAT_PEOPLE_DIRECTIVE_GOLDEN_AGE;
	}

	// Create Great Work if there is a slot
	GreatWorkType eGreatWork = pGreatArtist->GetGreatWork();
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && GetEconomicAI()->GetBestGreatWorkCity(pGreatArtist->plot(), eGreatWork))
	{
		eDirective = GREAT_PEOPLE_DIRECTIVE_USE_POWER;
	}

	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && !isGoldenAge())
	{
		eDirective = GREAT_PEOPLE_DIRECTIVE_GOLDEN_AGE;
	}

	return eDirective;
}

GreatPeopleDirectiveTypes CvPlayerAI::GetDirectiveMusician(CvUnit* pGreatMusician)
{
	GreatPeopleDirectiveTypes eDirective = NO_GREAT_PEOPLE_DIRECTIVE_TYPE;

	// If headed on a concert tour, keep going
	if (pGreatMusician->getArmyID() != FFreeList::INVALID_INDEX)
	{
		eDirective = GREAT_PEOPLE_DIRECTIVE_TOURISM_BLAST;
	}

	// If closing in on a Culture win, go for the Concert Tour
#ifdef AUI_PLAYERAI_GREAT_MUSICIAN_DIRECTIVE_HIGHER_INFLUENTIALS_REQUIRED_BEFORE_CONCERT_TOUR_PRIORITY
#ifdef AUI_GS_PRIORITY_RATIO
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && 
		GetGrandStrategyAI()->IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE")) &&
		4 * GetCulture()->GetNumCivsInfluentialOn() > 3 * GC.getGame().GetGameCulture()->GetNumCivsInfluentialForWin())
#else
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && GetDiplomacyAI()->IsGoingForCultureVictory() &&
		4 * GetCulture()->GetNumCivsInfluentialOn() > 3 * GC.getGame().GetGameCulture()->GetNumCivsInfluentialForWin())
#endif
#else
#ifdef AUI_GS_PRIORITY_RATIO
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE &&
		GetGrandStrategyAI()->IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE")) &&
		GetCulture()->GetNumCivsInfluentialOn() > (GC.getGame().GetGameCulture()->GetNumCivsInfluentialForWin() / 2))
#else
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && GetDiplomacyAI()->IsGoingForCultureVictory() && GetCulture()->GetNumCivsInfluentialOn() > (GC.getGame().GetGameCulture()->GetNumCivsInfluentialForWin() / 2))
#endif
#endif
	{		
		CvPlot* pTarget = FindBestMusicianTargetPlot(pGreatMusician, true);
		if(pTarget)
		{
			eDirective = GREAT_PEOPLE_DIRECTIVE_TOURISM_BLAST;
		}
	}

	// Create Great Work if there is a slot
	GreatWorkType eGreatWork = pGreatMusician->GetGreatWork();
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && GetEconomicAI()->GetBestGreatWorkCity(pGreatMusician->plot(), eGreatWork))
	{
		eDirective = GREAT_PEOPLE_DIRECTIVE_USE_POWER;
	}
	else
	{
		CvPlot* pTarget = FindBestMusicianTargetPlot(pGreatMusician, true);
		if(pTarget)
		{
			eDirective = GREAT_PEOPLE_DIRECTIVE_TOURISM_BLAST;
		}
	}

	return eDirective;
}

GreatPeopleDirectiveTypes CvPlayerAI::GetDirectiveEngineer(CvUnit* pGreatEngineer)
{
	GreatPeopleDirectiveTypes eDirective = NO_GREAT_PEOPLE_DIRECTIVE_TYPE;

	// look for a wonder to rush
	if(eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE)
	{
		int iNextWonderWeight;
		BuildingTypes eNextWonderDesired = GetWonderProductionAI()->ChooseWonder(false /*bUseAsyncRandom*/, false /*bAdjustForOtherPlayers*/, iNextWonderWeight);
		if(eNextWonderDesired != NO_BUILDING)
		{
			eDirective = GREAT_PEOPLE_DIRECTIVE_USE_POWER;
		}
	}

#ifdef AUI_PLAYERAI_TWEAKED_GREAT_ENGINEER_DIRECTIVE
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && 4 * GC.getGame().getGameTurn() <= 3 * GC.getGame().getEstimateEndTurn())
#else
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && GC.getGame().getGameTurn() <= ((GC.getGame().getEstimateEndTurn() * 3) / 4))
#endif
	{
#ifdef AUI_GS_PRIORITY_RATIO
		if (GetGrandStrategyAI()->IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST")))
#else
		if (GetDiplomacyAI()->IsGoingForWorldConquest())
#endif
		{
			eDirective = GREAT_PEOPLE_DIRECTIVE_CONSTRUCT_IMPROVEMENT;
		}
	}

	if(eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && (GC.getGame().getGameTurn() - pGreatEngineer->getGameTurnCreated()) >= GC.getAI_HOMELAND_GREAT_PERSON_TURNS_TO_WAIT())
	{
		eDirective = GREAT_PEOPLE_DIRECTIVE_CONSTRUCT_IMPROVEMENT;
	}

	return eDirective;
}

GreatPeopleDirectiveTypes CvPlayerAI::GetDirectiveMerchant(CvUnit* pGreatMerchant)
{
	GreatPeopleDirectiveTypes eDirective = NO_GREAT_PEOPLE_DIRECTIVE_TYPE;

	bool bTheVeniceException = false;
#ifdef AUI_PLAYERAI_GREAT_MERCHANT_DIRECTIVE_TWEAKED_VENICE_CHECK
	if (!GET_PLAYER(GetID()).GreatMerchantWantsCash())
#else
	if (GetPlayerTraits()->IsNoAnnexing())
#endif
	{
		bTheVeniceException = true;
	}

	// if the merchant is in an army, he's already marching to a destination, so don't evaluate him
	if(pGreatMerchant->getArmyID() != FFreeList::INVALID_INDEX)
	{
		return NO_GREAT_PEOPLE_DIRECTIVE_TYPE;
	}

#ifdef AUI_MINOR_CIV_RATIO
	// Adjusts score change based on how many city states there are
	double dCityStateCountModifier = 1.0;
	double dCityStateDeviation = 1.0 + log(GC.getGame().getCurrentMinorCivDeviation());
	// Calculation is more complex than for Policies or Beliefs because we only ever want it to be 0 when there are no city states.
	if (dCityStateDeviation >= exp(-1.0))
	{
		dCityStateCountModifier = dCityStateDeviation;
	}
	else
	{
		dCityStateCountModifier = exp(dCityStateDeviation);
	}
#endif

#ifdef AUI_MINOR_CIV_RATIO
#ifdef AUI_GS_PRIORITY_RATIO
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && !bTheVeniceException && (dCityStateCountModifier <= 4.0 / 3.0 || 
		4 * GC.getGame().getGameTurn() * dCityStateCountModifier <= 3 * GC.getGame().getEstimateEndTurn()))
#else
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && (dCityStateCountModifier <= 4.0 / 3.0 || 
		4 * GC.getGame().getGameTurn() * dCityStateCountModifier <= 3 * GC.getGame().getEstimateEndTurn()))
#endif
#else
#ifdef AUI_GS_PRIORITY_RATIO
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && !bTheVeniceException && GC.getGame().getGameTurn() <= ((GC.getGame().getEstimateEndTurn() * 2) / 4))
#else
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && GC.getGame().getGameTurn() <= ((GC.getGame().getEstimateEndTurn() * 2) / 4))
#endif
#endif
	{
#ifdef AUI_GS_PRIORITY_RATIO
		double dTestValue = pow(GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST")) *
			GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE")) *
			GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SCIENCE")), 1.0 / 3.0);
#ifdef AUI_FAST_COMP
		if (GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS")) >= 
			FASTMAX(0.25, dTestValue) * dCityStateCountModifier)
#else
		if (GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS")) >= 
			MAX(0.25, dTestValue) * dCityStateCountModifier)
#endif
#else
		if (GetDiplomacyAI()->IsGoingForDiploVictory() && !bTheVeniceException)
#endif
		{
			eDirective = GREAT_PEOPLE_DIRECTIVE_CONSTRUCT_IMPROVEMENT;
		}
	}

	// Attempt a run to a minor civ
#ifdef AUI_MINOR_CIV_RATIO
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && dCityStateCountModifier > 0 && IsSafe(this))
#else
	if(eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && IsSafe(this))
#endif
	{
		CvPlot* pTarget = FindBestMerchantTargetPlot(pGreatMerchant, true);
		if(pTarget)
		{
			eDirective = GREAT_PEOPLE_DIRECTIVE_USE_POWER;
		}
	}

	if(eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && (GC.getGame().getGameTurn() - pGreatMerchant->getGameTurnCreated()) >= GC.getAI_HOMELAND_GREAT_PERSON_TURNS_TO_WAIT() && !bTheVeniceException)
	{
		eDirective = GREAT_PEOPLE_DIRECTIVE_CONSTRUCT_IMPROVEMENT;
	}

	return eDirective;
}

GreatPeopleDirectiveTypes CvPlayerAI::GetDirectiveScientist(CvUnit* /*pGreatScientist*/)
{
	GreatPeopleDirectiveTypes eDirective = NO_GREAT_PEOPLE_DIRECTIVE_TYPE;

	// If I'm in danger, use great person to get a tech boost
	if(eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && !IsSafe(this))
	{
		eDirective = GREAT_PEOPLE_DIRECTIVE_USE_POWER;
	}

#ifdef AUI_PLAYERAI_TWEAKED_GREAT_SCIENTIST_DIRECTIVE
	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && 4 * GC.getGame().getGameTurn() <= 3 * GC.getGame().getEstimateEndTurn())
#else
	if(eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE && GC.getGame().getGameTurn() <= ((GC.getGame().getEstimateEndTurn() * 1) / 4))
#endif
	{
#ifndef AUI_PLAYERAI_TWEAKED_GREAT_SCIENTIST_DIRECTIVE
#ifdef AUI_GS_PRIORITY_RATIO
		if (GetGrandStrategyAI()->IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP")))
#else
		if(GetDiplomacyAI()->IsGoingForSpaceshipVictory())
#endif
#endif
		{
			eDirective = GREAT_PEOPLE_DIRECTIVE_CONSTRUCT_IMPROVEMENT;
		}
	}

	if (eDirective == NO_GREAT_PEOPLE_DIRECTIVE_TYPE)
	{
		// a tech boost is never bad
		eDirective = GREAT_PEOPLE_DIRECTIVE_USE_POWER;
	}

	return eDirective;
}

GreatPeopleDirectiveTypes CvPlayerAI::GetDirectiveGeneral(CvUnit* pGreatGeneral)
{
	GreatPeopleDirectiveTypes eDirective = NO_GREAT_PEOPLE_DIRECTIVE_TYPE;

	SpecialUnitTypes eSpecialUnitGreatPerson = (SpecialUnitTypes) GC.getInfoTypeForString("SPECIALUNIT_PEOPLE");

	int iGreatGeneralCount = 0;

	int iLoop;
	for(CvUnit* pLoopUnit = firstUnit(&iLoop); pLoopUnit; pLoopUnit = nextUnit(&iLoop))
	{
		if(pLoopUnit->getSpecialUnitType() != eSpecialUnitGreatPerson)
		{
			continue;
		}

		if(pLoopUnit->AI_getUnitAIType() == UNITAI_GENERAL && pLoopUnit->GetGreatPeopleDirective() != GREAT_PEOPLE_DIRECTIVE_GOLDEN_AGE)
		{
			iGreatGeneralCount++;
		}
	}

	if(iGreatGeneralCount > 2 && pGreatGeneral->plot()->getOwner() == pGreatGeneral->getOwner())
	{
		// we're using a power at this point because constructing the improvement goes through different code
		eDirective = GREAT_PEOPLE_DIRECTIVE_USE_POWER;
	}

	return eDirective;
}

GreatPeopleDirectiveTypes CvPlayerAI::GetDirectiveProphet(CvUnit*)
{
	GreatPeopleDirectiveTypes eDirective = NO_GREAT_PEOPLE_DIRECTIVE_TYPE;

	ReligionTypes eReligion = GetReligions()->GetReligionCreatedByPlayer();
	const CvReligion* pMyReligion = GC.getGame().GetGameReligions()->GetReligion(eReligion, GetID());

	// CASE 1: I have an enhanced religion
	if (pMyReligion && pMyReligion->m_bEnhanced)
	{
		// Spread religion if there is any city that needs it
#ifdef AUI_PLAYERAI_TWEAKED_GREAT_PROPHET_DIRECTIVE
		if (GetReligionAI()->ChooseProphetConversionCity(true/*bOnlyBetterThanEnhancingReligion*/) || 
			(!GetPlayerPolicies()->IsPolicyBranchUnlocked((PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_PIETY", true /*bHideAssert*/)) && 
			GetReligionAI()->ChooseProphetConversionCity(false/*bOnlyBetterThanEnhancingReligion*/)))
#else
		if (GetReligionAI()->ChooseProphetConversionCity(false/*bOnlyBetterThanEnhancingReligion*/))
#endif
		{
			eDirective = GREAT_PEOPLE_DIRECTIVE_SPREAD_RELIGION;
		}
		else
		{
			eDirective = GREAT_PEOPLE_DIRECTIVE_CONSTRUCT_IMPROVEMENT;
		}
	}


	// CASE 2: I have a religion that hasn't yet been enhanced
	else if (pMyReligion)
	{
		// Spread religion if there is a city that needs it CRITICALLY
		if (GetReligionAI()->ChooseProphetConversionCity(true/*bOnlyBetterThanEnhancingReligion*/))
		{
			eDirective = GREAT_PEOPLE_DIRECTIVE_SPREAD_RELIGION;
		}
		else
		{
			eDirective = GREAT_PEOPLE_DIRECTIVE_USE_POWER;
		}
	}

	// CASE 3: No religion for me yet
	else
	{
		// Locked out?
		if (GC.getGame().GetGameReligions()->GetNumReligionsStillToFound() <= 0)
		{
			eDirective = GREAT_PEOPLE_DIRECTIVE_CONSTRUCT_IMPROVEMENT;
		}

		// Not locked out
		else
		{
			eDirective = GREAT_PEOPLE_DIRECTIVE_USE_POWER;
		}
	}

	return eDirective;
}

GreatPeopleDirectiveTypes CvPlayerAI::GetDirectiveAdmiral(CvUnit* /*pGreatAdmiral*/)
{
	GreatPeopleDirectiveTypes eDirective = NO_GREAT_PEOPLE_DIRECTIVE_TYPE;

	return eDirective;
}

bool CvPlayerAI::GreatMerchantWantsCash()
{
	// slewis - everybody wants cash . . .
	// slewis - . . . except Venice. Venice wants to buy city states, unless it already has enough cities, then it doesn't want city states.
	bool bIsVenice = GetPlayerTraits()->IsNoAnnexing();
#ifdef AUI_PLAYERAI_TWEAKED_VENICE_CITY_TARGET
	if (bIsVenice && GC.getGame().countCivPlayersAlive() - GC.getGame().countMajorCivsAlive() > GetNumUnitsWithUnitAI(UNITAI_MERCHANT))
#else
	if (bIsVenice)
#endif
	{
#ifdef AUI_PLAYERAI_TWEAKED_VENICE_CITY_TARGET
		if (GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && isHuman())
		{
			return true;
		}

		double dDesiredCities = double(GetEconomicAI()->GetEarlyCityNumberTarget());
		int iFlavorExpansion = 0;
		int iFlavorGrowth = 0;

		for (int iFlavorLoop = 0; iFlavorLoop < GC.getNumFlavorTypes() && (iFlavorExpansion == 0 || iFlavorGrowth == 0); iFlavorLoop++)
		{
			if (GC.getFlavorTypes((FlavorTypes)iFlavorLoop) == "FLAVOR_EXPANSION")
			{
				iFlavorExpansion = GetGrandStrategyAI()->GetPersonalityAndGrandStrategy((FlavorTypes)iFlavorLoop);
			}
			else if (GC.getFlavorTypes((FlavorTypes)iFlavorLoop) == "FLAVOR_GROWTH")
			{
				iFlavorGrowth = GetGrandStrategyAI()->GetPersonalityAndGrandStrategy((FlavorTypes)iFlavorLoop);
			}
		}
#ifdef AUI_FAST_COMP
		iFlavorExpansion = FASTMAX(GC.getFLAVOR_MIN_VALUE(), iFlavorExpansion);
		iFlavorGrowth = FASTMAX(GC.getFLAVOR_MIN_VALUE(), iFlavorGrowth);
		// Extra cities from difficulty get applied as Expansion flavor
		double dDifficulty = FASTMAX(0, GC.getGame().getHandicapInfo().GetID() - 3) + 2.0;
#else
		iFlavorExpansion = MAX(GC.getFLAVOR_MIN_VALUE(), iFlavorExpansion);
		iFlavorGrowth = MAX(GC.getFLAVOR_MIN_VALUE(), iFlavorGrowth);
		// Extra cities from difficulty get applied as Expansion flavor
		double dDifficulty = MAX(0, GC.getGame().getHandicapInfo().GetID() - 3) + 2.0;
#endif
		dDifficulty /= 2.0;
		// Base flavor scaling
#ifdef AUI_FAST_COMP
		dDesiredCities *= log(iFlavorExpansion * pow(dDifficulty, 2.0)) / log((double)FASTMAX(iFlavorGrowth, 2));
#else
		dDesiredCities *= log(iFlavorExpansion * pow(dDifficulty, 2.0)) / log((double)MAX(iFlavorGrowth, 2));
#endif
		// map scaling parameters
		const int iDefaultNumTiles = 80*52;
		dDesiredCities = dDesiredCities * GC.getMap().numPlots() / iDefaultNumTiles + 1.0; // +1.0 for capital
		// player count scaling parameters
		const int iMajorCount = GC.getGame().countMajorCivsAlive();
		const int iMinorCount = GC.getGame().countCivPlayersAlive() - iMajorCount;
#ifdef AUI_FAST_COMP
		const double dDefaultCityCount = FASTMAX(16 + 8 * dDesiredCities, (double)GC.getGame().getNumCivCities());
		const double dTargetCityCount = FASTMAX(iMinorCount + iMajorCount * dDesiredCities, (double)GC.getGame().getNumCivCities());
#else
		const double dDefaultCityCount = MAX(16 + 8 * dDesiredCities, (double)GC.getGame().getNumCivCities());
		const double dTargetCityCount = MAX(iMinorCount + iMajorCount * dDesiredCities, (double)GC.getGame().getNumCivCities());
#endif
		dDesiredCities *= dDefaultCityCount / dTargetCityCount;

#ifdef AUI_FAST_COMP
		dDesiredCities = FASTMAX(dDesiredCities, 2.0);
#else
		dDesiredCities = MAX(dDesiredCities, 2.0);
#endif

		if (getNumCities() >= int(dDesiredCities + 0.5))
#else
		if (getNumCities() >= 4)
#endif
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	return true;
}

CvPlot* CvPlayerAI::FindBestMerchantTargetPlot(CvUnit* pGreatMerchant, bool bOnlySafePaths)
{
	CvAssertMsg(pGreatMerchant, "pGreatMerchant is null");
	if(!pGreatMerchant)
	{
		return NULL;
	}

	int iBestTurnsToReach = MAX_INT;
	CvPlot* pBestTargetPlot = NULL;
	int iPathTurns;
	UnitHandle pMerchant = UnitHandle(pGreatMerchant);
	CvTeam& kTeam = GET_TEAM(getTeam());

	//bool bIsVenice = GetPlayerTraits()->IsNoAnnexing();
	//bool bWantsCash = GreatMerchantWantsCash();

	// Loop through each city state
	for(int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iI);
		if (!kPlayer.isMinorCiv())
		{
			continue;
		}

		// if I'm Venice, I don't want to send a Merchant of Venice to a buy a city that I have trade routes 
		// with because it's probably more valuable as a trade partner than as an owned entity
		//if (!bWantsCash)
		//{
		//	if (bIsVenice)
		//	{
		//		if (GetTrade()->IsConnectedToPlayer(kPlayer.GetID()))
		//		{
		//			continue;
		//		}
		//	}
		//}

		CvPlot* pCSPlot = kPlayer.getStartingPlot();
		if (!pCSPlot)
		{
			continue;
		}

		if (!pCSPlot->isRevealed(getTeam()))
		{
			continue;
		}

		// Is this a minor we are friendly with?
		bool bMinorCivApproachIsCorrect = (GetDiplomacyAI()->GetMinorCivApproach(kPlayer.GetID()) != MINOR_CIV_APPROACH_CONQUEST);
		bool bNotAtWar = !kTeam.isAtWar(kPlayer.getTeam());
		bool bNotPlanningAWar = GetDiplomacyAI()->GetWarGoal(kPlayer.GetID()) == NO_WAR_GOAL_TYPE;

#ifdef AUI_PLAYERAI_FIND_BEST_MERCHANT_TARGET_PLOT_VENICE_FILTERS
		double dTurnsMod = 1.0;
		if (!GreatMerchantWantsCash())
		{
			bMinorCivApproachIsCorrect = true;
			if (kPlayer.GetMinorCivAI()->GetAlly() != GetID())
			{
				dTurnsMod *= 1.25;
			}
			for (int kK = 0; kK < GC.getNumResourceInfos(); kK++)
			{
				if (kPlayer.getNumResourceAvailable((ResourceTypes)kK, false) > 0)
				{
					if (getNumResourceAvailable((ResourceTypes)kK, false) == 0)
					{
						dTurnsMod *= sqrt(1.05);
						if (getNumResourceAvailable((ResourceTypes)kK, true) == 0)
						{
							dTurnsMod *= sqrt(1.05);
						}
					}
				}
			}
			CvCity* pCity = pCSPlot->getPlotCity();
			if (pCity)
			{
				if (pCity->isCoastal())
				{
					dTurnsMod *= 1.25;
				}
				if (pCity->getArea() != getCapitalCity()->getArea())
				{
					if (pCity->isCoastal())
					{
						dTurnsMod *= 1.10;
					}
					else
					{
						dTurnsMod /= 1.25;
					}
				}
			}
			int iLoop = 0;
			CvCity* pLoopCity;
			int iBestDistance = MAX_INT;
			int iCurDistance;
			for (pLoopCity = firstCity(&iLoop); pLoopCity != NULL; pLoopCity = nextCity(&iLoop))
			{
				iCurDistance = plotDistance(pLoopCity->getX(), pLoopCity->getY(), pCSPlot->getX(), pCSPlot->getY());
				if (iCurDistance < iBestDistance)
				{
					iBestDistance = iCurDistance;
				}
			}
			dTurnsMod *= GC.getMap().numPlots() / (double)iBestDistance;
		}
#endif

		if(bMinorCivApproachIsCorrect && bNotAtWar && bNotPlanningAWar)
		{
#ifdef AUI_ASTAR_TURN_LIMITER
#ifdef AUI_PLAYERAI_FIND_BEST_MERCHANT_TARGET_PLOT_VENICE_FILTERS
			int iMaxTurns = int(iBestTurnsToReach * dTurnsMod + 0.5);
#else
			int iMaxTurns = iBestTurnsToReach;
#endif
#endif
			// Search all the plots adjacent to this city (since can't enter the minor city plot itself)
			for(int jJ = 0; jJ < NUM_DIRECTION_TYPES; jJ++)
			{
				CvPlot* pAdjacentPlot = plotDirection(pCSPlot->getX(), pCSPlot->getY(), ((DirectionTypes)jJ));
				if(pAdjacentPlot != NULL)
				{
					// Make sure this is still owned by the city state and is revealed to us and isn't a water tile
					//if(pAdjacentPlot->getOwner() == (PlayerTypes)iI && pAdjacentPlot->isRevealed(getTeam()) && !pAdjacentPlot->isWater())
					bool bRightOwner = (pAdjacentPlot->getOwner() == (PlayerTypes)iI);
					bool bIsRevealed = pAdjacentPlot->isRevealed(getTeam());
					if(bRightOwner && bIsRevealed)
					{
#ifdef AUI_ASTAR_TURN_LIMITER
#ifdef AUI_PLAYERAI_NO_REUSE_PATHS_FOR_TARGET_PLOTS
						iPathTurns = TurnsToReachTarget(pMerchant, pAdjacentPlot, false /*bReusePaths*/, !bOnlySafePaths/*bIgnoreUnits*/, false, iMaxTurns);
#else
						iPathTurns = TurnsToReachTarget(pMerchant, pAdjacentPlot, true /*bReusePaths*/, !bOnlySafePaths/*bIgnoreUnits*/, false, iMaxTurns);
#endif
#else
#ifdef AUI_PLAYERAI_NO_REUSE_PATHS_FOR_TARGET_PLOTS
						iPathTurns = TurnsToReachTarget(pMerchant, pAdjacentPlot, false /*bReusePaths*/, !bOnlySafePaths/*bIgnoreUnits*/);
#else
						iPathTurns = TurnsToReachTarget(pMerchant, pAdjacentPlot, true /*bReusePaths*/, !bOnlySafePaths/*bIgnoreUnits*/);
#endif
#endif
#ifdef AUI_PLAYERAI_FIND_BEST_MERCHANT_TARGET_PLOT_VENICE_FILTERS
						iPathTurns = int(iPathTurns * dTurnsMod + 0.5);
#endif
						if(iPathTurns < iBestTurnsToReach)
						{
							iBestTurnsToReach = iPathTurns;
#ifdef AUI_ASTAR_TURN_LIMITER
							iMaxTurns = iPathTurns;
#endif // #ifdef AUI_ASTAR_TURN_LIMITER
							pBestTargetPlot = pAdjacentPlot;
						}
					}
				}
			}
		}
	}

	return pBestTargetPlot;
}

CvPlot* CvPlayerAI::FindBestMusicianTargetPlot(CvUnit* pGreatMusician, bool bOnlySafePaths)
{
	CvAssertMsg(pGreatMusician, "pGreatMusician is null");
	if(!pGreatMusician)
	{
		return NULL;
	}

	int iBestTurnsToReach = MAX_INT;
	CvPlot* pBestTargetPlot = NULL;
	CvCity* pBestTargetCity = NULL;
	int iPathTurns;
	UnitHandle pMusician = UnitHandle(pGreatMusician);

	// Find target civ
	PlayerTypes eTargetPlayer = GetCulture()->GetCivLowestInfluence(true /*bCheckOpenBorders*/);
	if (eTargetPlayer == NO_PLAYER)
	{
		return NULL;
	}

	CvPlayer &kTargetPlayer = GET_PLAYER(eTargetPlayer);

	// Loop through each of that player's cities
	int iLoop;
	CvCity *pLoopCity;
	for(pLoopCity = kTargetPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kTargetPlayer.nextCity(&iLoop))
	{
		// Search all the plots adjacent to this city
		for(int jJ = 0; jJ < NUM_DIRECTION_TYPES; jJ++)
		{
			CvPlot* pAdjacentPlot = plotDirection(pLoopCity->getX(), pLoopCity->getY(), ((DirectionTypes)jJ));
			if(pAdjacentPlot != NULL)
			{
				// Make sure this is still owned by target and is revealed to us
				bool bRightOwner = (pAdjacentPlot->getOwner() == eTargetPlayer);
				bool bIsRevealed = pAdjacentPlot->isRevealed(getTeam());
				if(bRightOwner && bIsRevealed)
				{
#ifdef AUI_PLAYERAI_NO_REUSE_PATHS_FOR_TARGET_PLOTS
					iPathTurns = TurnsToReachTarget(pMusician, pAdjacentPlot, false /*bReusePaths*/, !bOnlySafePaths/*bIgnoreUnits*/);
#else
					iPathTurns = TurnsToReachTarget(pMusician, pAdjacentPlot, true /*bReusePaths*/, !bOnlySafePaths/*bIgnoreUnits*/);
#endif
					if(iPathTurns < iBestTurnsToReach)
					{
						iBestTurnsToReach = iPathTurns;
						pBestTargetCity = pLoopCity;
					}
				}
			}
		}
	}

	// Found a city now look at ALL the plots owned by that player near that city
	if (pBestTargetCity)
	{
		iBestTurnsToReach = MAX_INT;
		CvPlot *pLoopPlot;
		for(int iJ = 0; iJ < NUM_CITY_PLOTS; iJ++)
		{
			pLoopPlot = plotCity(pBestTargetCity->getX(), pBestTargetCity->getY(), iJ);
			if(pLoopPlot != NULL)
			{
				// Make sure this is still owned by target and is revealed to us
				bool bRightOwner = (pLoopPlot->getOwner() == eTargetPlayer);
				bool bIsRevealed = pLoopPlot->isRevealed(getTeam());
				if(bRightOwner && bIsRevealed)
				{
#ifdef AUI_PLAYERAI_NO_REUSE_PATHS_FOR_TARGET_PLOTS
					iPathTurns = TurnsToReachTarget(pMusician, pLoopPlot, false /*bReusePaths*/, !bOnlySafePaths/*bIgnoreUnits*/);
#else
					iPathTurns = TurnsToReachTarget(pMusician, pLoopPlot, true /*bReusePaths*/, !bOnlySafePaths/*bIgnoreUnits*/);
#endif
					if(iPathTurns < iBestTurnsToReach)
					{
						iBestTurnsToReach = iPathTurns;
						pBestTargetPlot = pLoopPlot;
					}
				}
			}	
		}
	}

	return pBestTargetPlot;
}

CvPlot* CvPlayerAI::FindBestArtistTargetPlot(CvUnit* pGreatArtist, int& iResultScore)
{
	CvAssertMsg(pGreatArtist, "pGreatArtist is null");
	if(!pGreatArtist)
	{
		return NULL;
	}

	iResultScore = 0;

	CvPlotsVector& m_aiPlots = GetPlots();

	CvPlot* pBestPlot = NULL;
	int iBestScore = 0;

	// loop through plots and wipe out ones that are invalid
	const uint nPlots = m_aiPlots.size();
	for(uint ui = 0; ui < nPlots; ui++)
	{
		if(m_aiPlots[ui] == -1)
		{
			continue;
		}

		CvPlot* pPlot = GC.getMap().plotByIndex(m_aiPlots[ui]);

		if(pPlot->isWater())
		{
			continue;
		}

		if(!pPlot->IsAdjacentOwnedByOtherTeam(getTeam()))
		{
			continue;
		}

		// don't build over luxury resources
		ResourceTypes eResource = pPlot->getResourceType();
		if(eResource != NO_RESOURCE)
		{
			CvResourceInfo* pkResource = GC.getResourceInfo(eResource);
			if(pkResource != NULL)
			{
				if (pkResource->getResourceUsage() == RESOURCEUSAGE_LUXURY)
				{
					continue;
				}
			}
		}

		// if no improvement can be built on this plot, then don't consider it
		FeatureTypes eFeature = pPlot->getFeatureType();
		if (eFeature != NO_FEATURE && GC.getFeatureInfo(eFeature)->isNoImprovement())
		{
			continue;
		}

		// Improvement already here?
		ImprovementTypes eImprovement = (ImprovementTypes)pPlot->getImprovementType();
		if (eImprovement != NO_IMPROVEMENT)
		{
			CvImprovementEntry* pkImprovementInfo = GC.getImprovementInfo(eImprovement);
			if(pkImprovementInfo)
			{
				if (pkImprovementInfo->GetCultureBombRadius() > 0)
				{
					continue;
				}
			}
		}

		int iScore = 0;

		for(int iI = 0; iI < NUM_DIRECTION_TYPES; ++iI)
		{
			CvPlot* pAdjacentPlot = plotDirection(pPlot->getX(), pPlot->getY(), ((DirectionTypes)iI));
			// if there's no plot, bail
			if(pAdjacentPlot == NULL)
			{
				continue;
			}

			// if the plot is ours or no one's, bail
			if(pAdjacentPlot->getTeam() == NO_TEAM || pAdjacentPlot->getTeam() == getTeam())
			{
				continue;
			}

			// don't evaluate city plots since we don't get ownership of them with the bomb
			if(pAdjacentPlot->getPlotCity())
			{
				continue;
			}

			const PlayerTypes eOtherPlayer = pAdjacentPlot->getOwner();
			if(GET_PLAYER(eOtherPlayer).isMinorCiv())
			{
				MinorCivApproachTypes eMinorApproach = GetDiplomacyAI()->GetMinorCivApproach(eOtherPlayer);
				// if we're friendly or protective, don't be a jerk. Bail out.
				if(eMinorApproach != MINOR_CIV_APPROACH_CONQUEST && eMinorApproach != MINOR_CIV_APPROACH_IGNORE)
				{
					iScore = 0;
					break;
				}
			}
			else
			{
				MajorCivApproachTypes eMajorApproach = GetDiplomacyAI()->GetMajorCivApproach(eOtherPlayer, true);
				DisputeLevelTypes eLandDisputeLevel = GetDiplomacyAI()->GetLandDisputeLevel(eOtherPlayer);

				bool bTicked = eMajorApproach == MAJOR_CIV_APPROACH_HOSTILE;
				bool bTickedAboutLand = eMajorApproach == MAJOR_CIV_APPROACH_NEUTRAL && (eLandDisputeLevel == DISPUTE_LEVEL_STRONG || eLandDisputeLevel == DISPUTE_LEVEL_FIERCE);

				// only bomb if we're hostile
				if(!bTicked && !bTickedAboutLand)
				{
					iScore = 0;
					break;
				}
			}

			eResource = pAdjacentPlot->getResourceType();
			if(eResource != NO_RESOURCE)
			{
				iScore += GetBuilderTaskingAI()->GetResourceWeight(eResource, NO_IMPROVEMENT, pAdjacentPlot->getNumResource()) * 10;
			}
			else
			{
				for(int iYield = 0; iYield < NUM_YIELD_TYPES; iYield++)
				{
					iScore += pAdjacentPlot->getYield((YieldTypes)iYield);
				}
			}
		}

		if(iScore > iBestScore)
		{
			iBestScore = iScore;
			pBestPlot = pPlot;
		}
	}

	iResultScore = iBestScore;
	return pBestPlot;
}

