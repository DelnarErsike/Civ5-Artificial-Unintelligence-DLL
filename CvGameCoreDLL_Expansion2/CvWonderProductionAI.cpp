/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#include "CvGameCoreDLLPCH.h"
#include "CvGameCoreDLLUtil.h"
#include "CvWonderProductionAI.h"
#include "CvGameCoreUtils.h"
#include "CvInternalGameCoreUtils.h"
#include "CvCitySpecializationAI.h"
#include "CvMinorCivAI.h"
#include "CvDiplomacyAI.h"
#include "CvInfosSerializationHelper.h"
// include this after all other headers
#include "LintFree.h"


/// Constructor
#ifdef AUI_PER_CITY_WONDER_PRODUCTION_AI
CvWonderProductionAI::CvWonderProductionAI(CvCity* pCity, CvBuildingXMLEntries* pBuildings):
m_pCity(pCity),
m_pBuildings(pBuildings)
#else
CvWonderProductionAI::CvWonderProductionAI(CvPlayer* pPlayer, CvBuildingXMLEntries* pBuildings):
	m_pPlayer(pPlayer),
	m_pBuildings(pBuildings)
#endif
{
}

/// Destructor
CvWonderProductionAI::~CvWonderProductionAI(void)
{
}

/// Initialize
#ifdef AUI_PER_CITY_WONDER_PRODUCTION_AI
void CvWonderProductionAI::Init(CvBuildingXMLEntries* pBuildings, CvCity* pCity)
#else
void CvWonderProductionAI::Init(CvBuildingXMLEntries* pBuildings, CvPlayer* pPlayer, bool bIsCity)
#endif
{
	// Init base class
	CvFlavorRecipient::Init();

	// Store off the pointer to the buildings for this game
	m_pBuildings = pBuildings;
#ifdef AUI_PER_CITY_WONDER_PRODUCTION_AI
	m_pCity = pCity;
#else
	m_pPlayer = pPlayer;
	m_bIsCity = bIsCity;
#endif

	Reset();
}

/// Clear out AI local variables
void CvWonderProductionAI::Reset()
{
	CvAssertMsg(m_pBuildings != NULL, "Wonder Production AI init failure: building entries are NULL");

	// Reset vector
	m_WonderAIWeights.clear();

	// Loop through reading each one and adding it to our vector
	if(m_pBuildings)
	{
		for(int i = 0; i < m_pBuildings->GetNumBuildings(); i++)
		{
			m_WonderAIWeights.push_back(i, 0);
		}
	}
}

/// Serialization read
void CvWonderProductionAI::Read(FDataStream& kStream)
{
	// Version number to maintain backwards compatibility
	uint uiVersion;
	kStream >> uiVersion;

	int iWeight;

#ifndef AUI_PER_CITY_WONDER_PRODUCTION_AI
	CvAssertMsg(m_piLatestFlavorValues != NULL && GC.getNumFlavorTypes() > 0, "Number of flavor values to serialize is expected to greater than 0");

	int iNumFlavors;
	kStream >> iNumFlavors;

	ArrayWrapper<int> wrapm_piLatestFlavorValues(iNumFlavors, m_piLatestFlavorValues);
	kStream >> wrapm_piLatestFlavorValues;

	CvAssertMsg(m_pBuildings != NULL, "Wonder Production AI init failure: building entries are NULL");
#endif

	// Reset vector
	m_WonderAIWeights.clear();

	// Loop through reading each one and adding it to our vector
	if(m_pBuildings)
	{
		for(int i = 0; i < m_pBuildings->GetNumBuildings(); i++)
		{
			m_WonderAIWeights.push_back(i, 0);
		}

		int iNumEntries;
		int iType;

		kStream >> iNumEntries;

		for(int iI = 0; iI < iNumEntries; iI++)
		{
			bool bValid = true;
			iType = CvInfosSerializationHelper::ReadHashed(kStream, &bValid);
			if(iType != -1 || !bValid)
			{
				kStream >> iWeight;
				if(iType != -1)
				{
					m_WonderAIWeights.IncreaseWeight(iType, iWeight);
				}
				else
				{
					CvString szError;
					szError.Format("LOAD ERROR: Building Type not found");
					GC.LogMessage(szError.GetCString());
					CvAssertMsg(false, szError);
				}
			}
		}
	}
}

/// Serialization write
void CvWonderProductionAI::Write(FDataStream& kStream) const
{
	// Current version number
	uint uiVersion = 1;
	kStream << uiVersion;

#ifndef AUI_PER_CITY_WONDER_PRODUCTION_AI
	CvAssertMsg(m_piLatestFlavorValues != NULL && GC.getNumFlavorTypes() > 0, "Number of flavor values to serialize is expected to greater than 0");
	kStream << GC.getNumFlavorTypes();
	kStream << ArrayWrapper<int>(GC.getNumFlavorTypes(), m_piLatestFlavorValues);
#endif

	if(m_pBuildings)
	{
		int iNumBuildings = m_pBuildings->GetNumBuildings();
		kStream << iNumBuildings;

		// Loop through writing each entry
		for(int iI = 0; iI < iNumBuildings; iI++)
		{
			const BuildingTypes eBuilding = static_cast<BuildingTypes>(iI);
			CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
			if(pkBuildingInfo)
			{
				CvInfosSerializationHelper::WriteHashed(kStream, pkBuildingInfo);
				kStream << m_WonderAIWeights.GetWeight(iI);
			}
			else
			{
				kStream << (int)0;
			}
		}
	}
#ifndef AUI_PER_CITY_WONDER_PRODUCTION_AI
	else
	{
		CvAssertMsg(m_pBuildings != NULL, "Wonder Production AI init failure: building entries are NULL");
	}
#endif
}

/// Respond to a new set of flavor values
void CvWonderProductionAI::FlavorUpdate()
{
#ifdef AUI_PER_CITY_WONDER_PRODUCTION_AI
	m_pCity->GetCityStrategyAI()->FlavorUpdate();
#else
	// Broadcast to our sub AI objects
	for(int iFlavor = 0; iFlavor < GC.getNumFlavorTypes(); iFlavor++)
	{
		int iFlavorValue = GetLatestFlavorValue((FlavorTypes)iFlavor);
		AddFlavorWeights((FlavorTypes)iFlavor, iFlavorValue);
	}
#endif
}

/// Establish weights for one flavor; can be called multiple times to layer strategies
void CvWonderProductionAI::AddFlavorWeights(FlavorTypes eFlavor, int iWeight)
{
#ifdef AUI_POLICY_BUILDING_CLASS_FLAVOR_MODIFIERS
	CvPlayer* pPlayer = m_pCity->GetPlayer();
	CvPlayerPolicies* pPlayerPolicies = NULL;
	if (pPlayer)
		pPlayerPolicies = pPlayer->GetPlayerPolicies();
#endif
#ifdef AUI_BELIEF_BUILDING_CLASS_FLAVOR_MODIFIERS
	const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(m_pCity->GetCityReligions()->GetReligiousMajority(), m_pCity->getOwner());
#endif
#ifdef AUI_BUILDING_PRODUCTION_AI_LUA_FLAVOR_WEIGHTS
	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
#endif
	// Loop through all buildings (even though we're only go to do anything on wonders)
	for(int iBldg = 0; iBldg < m_pBuildings->GetNumBuildings(); iBldg++)
	{
		CvBuildingEntry* entry = m_pBuildings->GetEntry(iBldg);
		if(entry)
		{
			CvBuildingEntry& kBuilding = *entry;
			if(IsWonder(kBuilding))
			{
#if defined(AUI_POLICY_BUILDING_CLASS_FLAVOR_MODIFIERS) || defined(AUI_BELIEF_BUILDING_CLASS_FLAVOR_MODIFIERS) || defined(AUI_BUILDING_PRODUCTION_AI_LUA_FLAVOR_WEIGHTS)
				int iFlavorValue = entry->GetFlavorValue(eFlavor);
#endif
#ifdef AUI_POLICY_BUILDING_CLASS_FLAVOR_MODIFIERS
				if (pPlayerPolicies)
				{
					for (int iI = 0; iI < GC.getNumPolicyInfos(); iI++)
					{
						PolicyTypes ePolicy = static_cast<PolicyTypes>(iI);
						CvPolicyEntry* pPolicy = GC.getPolicyInfo(ePolicy);
						if (pPolicy && pPlayerPolicies->HasPolicy(ePolicy))
						{
							iFlavorValue += pPolicy->GetBuildingClassFlavorChanges(entry->GetBuildingClassType(), eFlavor);
						}
					}
				}
#endif
#ifdef AUI_BELIEF_BUILDING_CLASS_FLAVOR_MODIFIERS
				if (pReligion)
				{
					pReligion->m_Beliefs.GetBuildingClassFlavorChange(static_cast<BuildingClassTypes>(entry->GetBuildingClassType()), eFlavor);
				}
#endif
#ifdef AUI_BUILDING_PRODUCTION_AI_LUA_FLAVOR_WEIGHTS
				if (!GC.getDISABLE_BUILDING_AI_FLAVOR_LUA_MODDING() && pkScriptSystem)
				{
					CvLuaArgsHandle args;
					args->Push(m_pCity->getOwner());
					args->Push(m_pCity->GetID());
					args->Push(iBldg);
					args->Push(eFlavor);

					int iResult = 0;
					if (LuaSupport::CallAccumulator(pkScriptSystem, "ExtraBuildingFlavor", args.get(), iResult))
						iFlavorValue += iResult;
				}
#endif
				// Set its weight by looking at wonder's weight for this flavor and using iWeight multiplier passed in
#if defined(AUI_POLICY_BUILDING_CLASS_FLAVOR_MODIFIERS) || defined(AUI_BELIEF_BUILDING_CLASS_FLAVOR_MODIFIERS) || defined(AUI_BUILDING_PRODUCTION_AI_LUA_FLAVOR_WEIGHTS)
				m_WonderAIWeights.IncreaseWeight(iBldg, iFlavorValue * iWeight);
#else
				m_WonderAIWeights.IncreaseWeight(iBldg, kBuilding.GetFlavorValue(eFlavor) * iWeight);
#endif
			}
		}
	}
}

/// Retrieve sum of weights on one item
int CvWonderProductionAI::GetWeight(BuildingTypes eBldg)
{
#ifdef AUI_BUILDING_PRODUCTION_AI_CONSIDER_FREE_STUFF
	int iWeight = m_WonderAIWeights.GetWeight(eBldg);
	CvBuildingEntry* entry = m_pBuildings->GetEntry(eBldg);
	if (entry)
	{
		CvPlayer* pPlayer = m_pCity->GetPlayer();
		int iLoop = 0;

		BuildingTypes eFreeBuildingThisCity = static_cast<BuildingTypes>(entry->GetFreeBuildingThisCity());
		if (eFreeBuildingThisCity != NO_BUILDING)
		{
			if (m_pCity->GetCityBuildings()->GetNumBuilding(eFreeBuildingThisCity) == 0)
				iWeight += m_pCity->GetCityStrategyAI()->GetBuildingProductionAI()->GetWeight(eFreeBuildingThisCity);
		}

		BuildingClassTypes eFreeBuildingClassAllCities = static_cast<BuildingClassTypes>(entry->GetFreeBuildingClass());
		if (eFreeBuildingClassAllCities != NO_BUILDINGCLASS)
		{
			BuildingTypes eFreeBuilding = static_cast<BuildingTypes>(m_pCity->getCivilizationInfo().getCivilizationBuildings(eFreeBuildingClassAllCities));
			for (CvCity* pLoopCity = pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = pPlayer->nextCity(&iLoop))
			{
				if (pLoopCity->GetCityBuildings()->GetNumBuilding(eFreeBuilding) == 0)
					iWeight += pLoopCity->GetCityStrategyAI()->GetBuildingProductionAI()->GetWeight(eFreeBuilding);
			}
		}

		for (int iI = 0; iI < GC.getNumUnitInfos(); iI++)
		{
			int iNumFreeUnits = entry->GetNumFreeUnits(iI);
			if (iNumFreeUnits > 0)
			{
				iWeight += iNumFreeUnits * m_pCity->GetCityStrategyAI()->GetUnitProductionAI()->GetWeight((UnitTypes)iI);
			}
		}

		if (entry->GetInstantMilitaryIncrease())
		{
			FFastVector<UnitTypes, true, c_eCiv5GameplayDLL> aExtraUnits;
			for (CvUnit* pLoopUnit = pPlayer->firstUnit(&iLoop); pLoopUnit != NULL; pLoopUnit = pPlayer->nextUnit(&iLoop))
			{
				if (pLoopUnit->getDomainType() == DOMAIN_LAND && pLoopUnit->IsCombatUnit())
				{
					UnitTypes eCurrentUnitType = pLoopUnit->getUnitType();

					// check for duplicate unit
					bool bAddUnit = true;
					for (uint ui = 0; ui < aExtraUnits.size(); ui++)
					{
						if (aExtraUnits[ui] == eCurrentUnitType)
						{
							bAddUnit = false;
							break;
						}
					}
					if (bAddUnit)
					{
						aExtraUnits.push_back(eCurrentUnitType);
					}
				}
			}
			for (uint ui = 0; ui < aExtraUnits.size(); ui++)
			{
				iWeight += m_pCity->GetCityStrategyAI()->GetUnitProductionAI()->GetWeight(aExtraUnits[ui]);
			}
		}
	}
	return iWeight;
#else
	return m_WonderAIWeights.GetWeight(eBldg);
#endif
}

/// Recommend highest-weighted wonder, also return total weight of all buildable wonders
BuildingTypes CvWonderProductionAI::ChooseWonder(bool bUseAsyncRandom, bool bAdjustForOtherPlayers, int& iWonderWeight)
{
	int iBldgLoop;
	int iWeight;
	int iTurnsRequired;
#if !defined(AUI_WONDER_PRODUCTION_FIX_CONSIDER_PRODUCTION_MODIFIERS) && !defined(AUI_PER_CITY_WONDER_PRODUCTION_AI)
	int iEstimatedProductionPerTurn;
#endif
#ifndef AUI_PER_CITY_WONDER_PRODUCTION_AI
	int iCityLoop;
#endif
	RandomNumberDelegate fcn;
	BuildingTypes eSelection;

#ifdef AUI_WONDER_PRODUCTION_CHOOSE_WONDER_FLAVOR_UPDATE
	FlavorUpdate();
#endif

	// Use the asynchronous random number generate if "no random" is set
	if(bUseAsyncRandom)
	{
		fcn = MakeDelegate(&GC.getGame(), &CvGame::getAsyncRandNum);
	}
	else
	{
		fcn = MakeDelegate(&GC.getGame(), &CvGame::getJonRandNum);
	}

	// Reset list of all the possible wonders
	m_Buildables.clear();

#ifdef AUI_PER_CITY_WONDER_PRODUCTION_AI
	CvCity* pWonderCity = m_pCity;
#else
	// Guess which city will be producing this (doesn't matter that much since weights are all relative)
	CvCity* pWonderCity = m_pPlayer->GetCitySpecializationAI()->GetWonderBuildCity();
	if(pWonderCity == NULL)
	{
		pWonderCity = m_pPlayer->firstCity(&iCityLoop);
	}
#endif

	CvAssertMsg(pWonderCity, "Trying to choose the next wonder to build and the player has no cities!");
	if(pWonderCity == NULL)
		return NO_BUILDING;

#if !defined(AUI_WONDER_PRODUCTION_FIX_CONSIDER_PRODUCTION_MODIFIERS) && !defined(AUI_PER_CITY_WONDER_PRODUCTION_AI)
	iEstimatedProductionPerTurn = pWonderCity->getCurrentProductionDifference(true, false);
	if(iEstimatedProductionPerTurn < 1)
	{
		iEstimatedProductionPerTurn = 1;
	}
#endif

	// Loop through adding the available wonders
	for(iBldgLoop = 0; iBldgLoop < GC.GetGameBuildings()->GetNumBuildings(); iBldgLoop++)
	{
		const BuildingTypes eBuilding = static_cast<BuildingTypes>(iBldgLoop);
		CvBuildingEntry* pkBuildingInfo = m_pBuildings->GetEntry(eBuilding);
		if(pkBuildingInfo)
		{
			CvBuildingEntry& kBuilding = *pkBuildingInfo;
			const CvBuildingClassInfo& kBuildingClassInfo = kBuilding.GetBuildingClassInfo();

			// Make sure this wonder can be built now
#ifdef AUI_PER_CITY_WONDER_PRODUCTION_AI
#ifdef AUI_WONDER_PRODUCTION_CHOOSE_WONDER_NO_NATIONAL_WONDERS
			if (::isWorldWonderClass(kBuildingClassInfo) && m_pCity->canConstruct(eBuilding))
#else
			if(IsWonder(kBuilding) && m_pCity->canConstruct(eBuilding))
#endif
			{
				iTurnsRequired = pWonderCity->getProductionTurnsLeft(eBuilding, 0);
#else
#ifdef AUI_WONDER_PRODUCTION_CHOOSE_WONDER_NO_NATIONAL_WONDERS
			if (::isWorldWonderClass(kBuildingClassInfo) && HaveCityToBuild((BuildingTypes)iBldgLoop))
#else
			if(IsWonder(kBuilding) && HaveCityToBuild((BuildingTypes)iBldgLoop))
#endif
			{
#ifdef AUI_WONDER_PRODUCTION_FIX_CONSIDER_PRODUCTION_MODIFIERS
#ifdef AUI_WONDER_PRODUCTION_FIX_CHOOSE_WONDER_TURNS_REQUIRED_USES_PLAYER_MOD
				iTurnsRequired = std::max(1, m_pPlayer->getProductionNeeded((BuildingTypes)iBldgLoop) / pWonderCity->getProductionDifference(0, 0, pWonderCity->getProductionModifier(eBuilding), true, false));
#else
				iTurnsRequired = std::max(1, kBuilding.GetProductionCost() / pWonderCity->getProductionDifference(0, 0, pWonderCity->getProductionModifier(eBuilding), true, false));
#endif
#else
#ifdef AUI_WONDER_PRODUCTION_FIX_CHOOSE_WONDER_TURNS_REQUIRED_USES_PLAYER_MOD
				iTurnsRequired = std::max(1, m_pPlayer->getProductionNeeded((BuildingTypes)iBldgLoop) / iEstimatedProductionPerTurn);
#else
				iTurnsRequired = std::max(1, kBuilding.GetProductionCost() / iEstimatedProductionPerTurn);
#endif
#endif
#endif

				// if we are forced to restart a wonder, give one that has been started already a huge bump
				bool bAlreadyStarted = pWonderCity->GetCityBuildings()->GetBuildingProduction(eBuilding) > 0;
				int iTempWeight = bAlreadyStarted ? m_WonderAIWeights.GetWeight(iBldgLoop) * 25 : m_WonderAIWeights.GetWeight(iBldgLoop);

				// Don't build the UN if you aren't going for the diplo victory
				if(pkBuildingInfo->IsDiplomaticVoting())
				{
					int iVotesNeededToWin = GC.getGame().GetVotesNeededForDiploVictory();
					int iSecuredVotes = 0;
#ifdef AUI_PER_CITY_WONDER_PRODUCTION_AI
					TeamTypes myTeamID = m_pCity->getTeam();
					PlayerTypes myPlayerID = m_pCity->getOwner();
#else
					TeamTypes myTeamID = m_pPlayer->getTeam();
					PlayerTypes myPlayerID = m_pPlayer->GetID();
#endif

					// Loop through Players to see if they'll vote for this player
					PlayerTypes eLoopPlayer;
					TeamTypes eLoopTeam;
					for(int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
					{
						eLoopPlayer = (PlayerTypes) iPlayerLoop;

						if(GET_PLAYER(eLoopPlayer).isAlive())
						{
							eLoopTeam = GET_PLAYER(eLoopPlayer).getTeam();

							// Liberated?
							if(GET_TEAM(eLoopTeam).GetLiberatedByTeam() == myTeamID)
							{
								iSecuredVotes++;
							}

							// Minor civ?
							else if(GET_PLAYER(eLoopPlayer).isMinorCiv())
							{
								// Best Relations?
								if(GET_PLAYER(eLoopPlayer).GetMinorCivAI()->GetAlly() == myPlayerID)
								{
									iSecuredVotes++;
								}
							}
						}
					}

					int iNumberOfPlayersWeNeedToBuyOff = MAX(0, iVotesNeededToWin - iSecuredVotes);

#ifdef AUI_PER_CITY_WONDER_PRODUCTION_AI
					if(!m_pCity->GetPlayer()->GetDiplomacyAI() || !m_pCity->GetPlayer()->GetDiplomacyAI()->IsGoingForDiploVictory() || m_pCity->GetPlayer()->GetTreasury()->GetGold() < iNumberOfPlayersWeNeedToBuyOff * 500 )
#else
					if(!m_pPlayer->GetDiplomacyAI() || !m_pPlayer->GetDiplomacyAI()->IsGoingForDiploVictory() || m_pPlayer->GetTreasury()->GetGold() < iNumberOfPlayersWeNeedToBuyOff * 500 )
#endif
					{
						iTempWeight = 0;
					}
				}

				iWeight = CityStrategyAIHelpers::ReweightByTurnsLeft(iTempWeight, iTurnsRequired);

				if(bAdjustForOtherPlayers && ::isWorldWonderClass(kBuildingClassInfo))
				{
					// Adjust weight for this wonder down based on number of other players currently working on it
					int iNumOthersConstructing = 0;
					for(int iPlayerLoop = 0; iPlayerLoop < MAX_MAJOR_CIVS; iPlayerLoop++)
					{
						PlayerTypes eLoopPlayer = (PlayerTypes) iPlayerLoop;
						if(GET_PLAYER(eLoopPlayer).getBuildingClassMaking((BuildingClassTypes)kBuilding.GetBuildingClassType()) > 0)
						{
							iNumOthersConstructing++;
						}
					}
					iWeight = iWeight / (1 + iNumOthersConstructing);
				}

				m_Buildables.push_back(iBldgLoop, iWeight);
			}
		}
	}

	// Sort items and grab the first one
	if(m_Buildables.size() > 0)
	{
		m_Buildables.SortItems();
		LogPossibleWonders();

		if(m_Buildables.GetTotalWeight() > 0)
		{
			int iNumChoices = GC.getGame().getHandicapInfo().GetCityProductionNumOptions();
			eSelection = (BuildingTypes)m_Buildables.ChooseFromTopChoices(iNumChoices, &fcn, "Choosing wonder from Top Choices");
			iWonderWeight = m_Buildables.GetTotalWeight();
			return eSelection;
		}

		// Nothing with any weight
		else
		{
			return NO_BUILDING;
		}
	}

	// Unless we didn't find any
	else
	{
		return NO_BUILDING;
	}
}


/// Recommend highest-weighted wonder and what city to build it at
#ifdef AUI_PER_CITY_WONDER_PRODUCTION_AI
#ifdef AUI_WONDER_PRODUCTION_CHOOSE_WONDER_FOR_GREAT_ENGINEER_WEIGH_COST
BuildingTypes CvWonderProductionAI::ChooseWonderForGreatEngineer(CvUnit* pUnit, bool bUseAsyncRandom, int iExtraTurns, int& iWonderWeight)
#else
BuildingTypes CvWonderProductionAI::ChooseWonderForGreatEngineer(bool bUseAsyncRandom, int iExtraTurns, int& iWonderWeight)
#endif
#else
#ifdef AUI_WONDER_PRODUCTION_CHOOSE_WONDER_FOR_GREAT_ENGINEER_WEIGH_COST
BuildingTypes CvWonderProductionAI::ChooseWonderForGreatEngineer(CvUnit* pUnit, bool bUseAsyncRandom, int& iWonderWeight, CvCity*& pCityToBuildAt)
#else
BuildingTypes CvWonderProductionAI::ChooseWonderForGreatEngineer(bool bUseAsyncRandom, int& iWonderWeight, CvCity*& pCityToBuildAt)
#endif
#endif
{
	int iBldgLoop;
	int iWeight;
#ifndef AUI_PER_CITY_WONDER_PRODUCTION_AI
	int iCityLoop;
#endif
	RandomNumberDelegate fcn;
	BuildingTypes eSelection;

#ifndef AUI_PER_CITY_WONDER_PRODUCTION_AI
	pCityToBuildAt = 0;
#endif
	iWonderWeight = 0;

#ifdef AUI_WONDER_PRODUCTION_CHOOSE_WONDER_FLAVOR_UPDATE
	FlavorUpdate();
#endif

	// Use the asynchronous random number generate if "no random" is set
	if (bUseAsyncRandom)
	{
		fcn = MakeDelegate(&GC.getGame(), &CvGame::getAsyncRandNum);
	}
	else
	{
		fcn = MakeDelegate(&GC.getGame(), &CvGame::getJonRandNum);
	}

	// Reset list of all the possible wonders
	m_Buildables.clear();

#ifdef AUI_PER_CITY_WONDER_PRODUCTION_AI
	CvCity* pWonderCity = m_pCity;
#else
	// Guess which city will be producing this
	CvCity* pWonderCity = m_pPlayer->GetCitySpecializationAI()->GetWonderBuildCity();
	if (pWonderCity == NULL)
	{
		pWonderCity = m_pPlayer->firstCity(&iCityLoop);
	}
#endif

	CvAssertMsg(pWonderCity, "Trying to choose the next wonder to build and the player has no cities!");
	if (pWonderCity == NULL)
		return NO_BUILDING;

#ifdef AUI_WONDER_PRODUCTION_CHOOSE_WONDER_FOR_GREAT_ENGINEER_WEIGH_COST
	int iMaxProductionGenerated = 0;
	if (pUnit)
		iMaxProductionGenerated = pUnit->getMaxHurryProduction(pWonderCity);
#endif

	// Loop through adding the available wonders
	for (iBldgLoop = 0; iBldgLoop < GC.GetGameBuildings()->GetNumBuildings(); iBldgLoop++)
	{
		const BuildingTypes eBuilding = static_cast<BuildingTypes>(iBldgLoop);
		CvBuildingEntry* pkBuildingInfo = m_pBuildings->GetEntry(eBuilding);
		if (pkBuildingInfo)
		{
			CvBuildingEntry& kBuilding = *pkBuildingInfo;
			// Make sure this wonder can be built now
#ifdef AUI_PER_CITY_WONDER_PRODUCTION_AI
#ifdef AUI_WONDER_PRODUCTION_CHOOSE_WONDER_NO_NATIONAL_WONDERS
			const CvBuildingClassInfo& kBuildingClassInfo = kBuilding.GetBuildingClassInfo();
			if (::isWorldWonderClass(kBuildingClassInfo) && m_pCity->canConstruct(eBuilding))
#else
			if(IsWonder(kBuilding) && m_pCity->canConstruct(eBuilding))
#endif
#else
#ifdef AUI_WONDER_PRODUCTION_CHOOSE_WONDER_NO_NATIONAL_WONDERS
			const CvBuildingClassInfo& kBuildingClassInfo = kBuilding.GetBuildingClassInfo();
			if (::isWorldWonderClass(kBuildingClassInfo) && HaveCityToBuild((BuildingTypes)iBldgLoop))
#else
			if (IsWonder(kBuilding) && HaveCityToBuild((BuildingTypes)iBldgLoop))
#endif
#endif
			{
				iWeight = m_WonderAIWeights.GetWeight((UnitTypes)iBldgLoop); // use raw weight since this wonder is essentially free
				// Don't build the UN if you aren't going for the diplo victory and have a chance of winning it
				if(pkBuildingInfo->IsDiplomaticVoting())
				{
					int iVotesNeededToWin = GC.getGame().GetVotesNeededForDiploVictory();
					int iSecuredVotes = 0;
#ifdef AUI_PER_CITY_WONDER_PRODUCTION_AI
					TeamTypes myTeamID = m_pCity->getTeam();
					PlayerTypes myPlayerID = m_pCity->getOwner();
#else
					TeamTypes myTeamID = m_pPlayer->getTeam();
					PlayerTypes myPlayerID = m_pPlayer->GetID();
#endif

					// Loop through Players to see if they'll vote for this player
					PlayerTypes eLoopPlayer;
					TeamTypes eLoopTeam;
					for(int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
					{
						eLoopPlayer = (PlayerTypes) iPlayerLoop;

						if(GET_PLAYER(eLoopPlayer).isAlive())
						{
							eLoopTeam = GET_PLAYER(eLoopPlayer).getTeam();

							// Liberated?
							if(GET_TEAM(eLoopTeam).GetLiberatedByTeam() == myTeamID)
							{
								iSecuredVotes++;
							}

							// Minor civ?
							else if(GET_PLAYER(eLoopPlayer).isMinorCiv())
							{
								// Best Relations?
								if(GET_PLAYER(eLoopPlayer).GetMinorCivAI()->GetAlly() == myPlayerID)
								{
									iSecuredVotes++;
								}
							}
						}
					}

					int iNumberOfPlayersWeNeedToBuyOff = MAX(0, iVotesNeededToWin - iSecuredVotes);

#ifdef AUI_PER_CITY_WONDER_PRODUCTION_AI
					if (!m_pCity->GetPlayer()->GetDiplomacyAI() || !m_pCity->GetPlayer()->GetDiplomacyAI()->IsGoingForDiploVictory() || m_pCity->GetPlayer()->GetTreasury()->GetGold() < iNumberOfPlayersWeNeedToBuyOff * 500)
#else
					if(!m_pPlayer->GetDiplomacyAI() || !m_pPlayer->GetDiplomacyAI()->IsGoingForDiploVictory() || m_pPlayer->GetTreasury()->GetGold() < iNumberOfPlayersWeNeedToBuyOff * 500 )
#endif
					{
						iWeight = 0;
					}
				}

#ifdef AUI_WONDER_PRODUCITON_CHOOSE_WONDER_FOR_GREAT_ENGINEER_WANT_WORLD_WONDER
				if (!(::isWorldWonderClass(kBuilding.GetBuildingClassInfo())))
					iWeight /= AUI_WONDER_PRODUCITON_CHOOSE_WONDER_FOR_GREAT_ENGINEER_WANT_WORLD_WONDER;
#endif

#ifdef AUI_WONDER_PRODUCTION_CHOOSE_WONDER_FOR_GREAT_ENGINEER_WEIGH_COST
#ifdef AUI_PER_CITY_WONDER_PRODUCTION_AI
				int iProduction = 0;
				int iFirstBuildingOrder = pWonderCity->getFirstBuildingOrder(eBuilding);
				int iProductionNeeded = pWonderCity->getProductionNeeded(eBuilding) * 100;
				int iProductionModifier = pWonderCity->getProductionModifier(eBuilding);

				if((iFirstBuildingOrder == -1) || (iFirstBuildingOrder == 0))
				{
					iProduction += pWonderCity->GetCityBuildings()->GetBuildingProductionTimes100(eBuilding);
				}
				int iTurnsRequired = pWonderCity->getProductionTurnsLeft(iProductionNeeded, iProduction + iMaxProductionGenerated, pWonderCity->getProductionDifferenceTimes100(iProductionNeeded, iProduction, iProductionModifier, false, true), pWonderCity->getProductionDifferenceTimes100(iProductionNeeded, iProduction, iProductionModifier, false, false));
				int iTurnsSaved = pWonderCity->getProductionTurnsLeft(iProductionNeeded, iProduction + iMaxProductionGenerated, pWonderCity->getProductionDifferenceTimes100(iProductionNeeded, iProduction, iProductionModifier, false, true), pWonderCity->getProductionDifferenceTimes100(iProductionNeeded, iProduction, iProductionModifier, false, false)) - 1 - iExtraTurns;
#else
				int iTurnsRequired = MAX(1, m_pPlayer->getProductionNeeded(eBuilding) / pWonderCity->getProductionDifference(0, 0, pWonderCity->getProductionModifier(eBuilding), true, false));
				int iTurnsSaved = MAX(iTurnsRequired - 1, (m_pPlayer->getProductionNeeded(eBuilding) - iMaxProductionGenerated) / pWonderCity->getProductionDifference(0, 0, pWonderCity->getProductionModifier(eBuilding), true, false) + iExtraTurns);
#endif
				iWeight = CityStrategyAIHelpers::ReweightByTurnsLeft(iWeight, iTurnsRequired - iTurnsSaved);
#endif

				// ??? do we want to weight it more for more expensive wonders?
				m_Buildables.push_back(iBldgLoop, iWeight);
			}
		}
	}

	// Sort items and grab the first one
	if(m_Buildables.size() > 0)
	{
		m_Buildables.SortItems();
		LogPossibleWonders();

		if(m_Buildables.GetTotalWeight() > 0)
		{
			int iNumChoices = 1;
			eSelection = (BuildingTypes)m_Buildables.ChooseFromTopChoices(iNumChoices, &fcn, "Choosing wonder from Top Choices");
			iWonderWeight = m_Buildables.GetTotalWeight();

#ifndef AUI_PER_CITY_WONDER_PRODUCTION_AI
			// first check if the wonder city can build it
			if (pWonderCity->canConstruct(eSelection))
			{
				pCityToBuildAt = pWonderCity;
			}
			// if it can't then check for other cities
			else
			{
				CvCity* pLoopCity;
				int iLoop;
				for(pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
				{
					if(pLoopCity->canConstruct(eSelection))
					{
						pCityToBuildAt = pLoopCity;
						break; // todo: find the best city 
					}
				}
			}
#endif

			return eSelection;
		}

		// Nothing with any weight
		else
		{
			return NO_BUILDING;
		}
	}

	// Unless we didn't find any
	else
	{
		return NO_BUILDING;
	}
}


/// Log all potential builds
void CvWonderProductionAI::LogPossibleWonders()
{
	if(GC.getLogging() && GC.getAILogging())
	{
		// Find the name of this civ
#ifdef AUI_PER_CITY_WONDER_PRODUCTION_AI
		CvString playerName = m_pCity->GetPlayer()->getCivilizationShortDescription();

		// Open the log file
		FILogFile* pLog = LOGFILEMGR.GetLog(m_pCity->GetPlayer()->GetCitySpecializationAI()->GetLogFileName(playerName), FILogFile::kDontTimeStamp);
#else
		CvString playerName = m_pPlayer->getCivilizationShortDescription();

		// Open the log file
		FILogFile* pLog = LOGFILEMGR.GetLog(m_pPlayer->GetCitySpecializationAI()->GetLogFileName(playerName), FILogFile::kDontTimeStamp);
#endif

		// Get the leading info for this line
		CvString strBaseString;
		strBaseString.Format("%03d, ", GC.getGame().getElapsedGameTurns());
		strBaseString += playerName + ", ";



		// Dump out the weight of each buildable item
		for(int iI = 0; iI < m_Buildables.size(); iI++)
		{
			CvString strOutBuf = strBaseString;

			CvBuildingEntry* pEntry = GC.GetGameBuildings()->GetEntry(m_Buildables.GetElement(iI));
			if(pEntry != NULL)
			{
				CvString strDesc = pEntry->GetDescription();
				CvString strTemp;
				strTemp.Format("Wonder, %s, %d", strDesc.GetCString(), m_Buildables.GetWeight(iI));
				strOutBuf += strTemp;

			}

			pLog->Msg(strOutBuf);
		}
	}
}

/// Stub - Probably don't need to log flavors to city specialization log -- is in enough places already
void CvWonderProductionAI::LogFlavors(FlavorTypes)
{
}

/// Check to make sure this is one of the buildings we consider to be a wonder
bool CvWonderProductionAI::IsWonder(const CvBuildingEntry& kBuilding) const
{
	const CvBuildingClassInfo& kBuildingClass = kBuilding.GetBuildingClassInfo();

	if(::isWorldWonderClass(kBuildingClass) ||
	        ::isTeamWonderClass(kBuildingClass) ||
	        ::isNationalWonderClass(kBuildingClass))
	{
		return true;
	}
	return false;
}

#ifndef AUI_PER_CITY_WONDER_PRODUCTION_AI
// PRIVATE METHODS

/// Check to make sure some city can build this wonder
bool CvWonderProductionAI::HaveCityToBuild(BuildingTypes eBuilding) const
{
	CvCity* pLoopCity;
	int iLoop;
	for(pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
	{
		if(pLoopCity->canConstruct(eBuilding))
		{
			return true;
		}
	}
	return false;
}
#endif