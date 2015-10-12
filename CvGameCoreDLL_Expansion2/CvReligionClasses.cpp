/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#include "CvGameCoreDLLPCH.h"
#include "CvGameCoreDLLUtil.h"
#include "ICvDLLUserInterface.h"
#include "CvGameCoreUtils.h"
#include "CvInfosSerializationHelper.h"
#include "CvMinorCivAI.h"
#include "CvDiplomacyAI.h"
#include "CvEconomicAI.h"
#include "CvGrandStrategyAI.h"
#include "CvMilitaryAI.h"
#include "cvStopWatch.h"

#include "LintFree.h"
 
//======================================================================================================
//					CvReligionEntry
//======================================================================================================
/// Constructor
CvReligionEntry::CvReligionEntry()
{
}

/// Destructor
CvReligionEntry::~CvReligionEntry()
{
}

/// Load XML data
bool CvReligionEntry::CacheResults(Database::Results& kResults, CvDatabaseUtility& kUtility)
{
	if(!CvBaseInfo::CacheResults(kResults, kUtility))
		return false;

	//Basic Properties
	m_strIconString = kResults.GetText("IconString");
	 
	return true;
}

//------------------------------------------------------------------------------
CvString CvReligionEntry::GetIconString() const
{
	return m_strIconString;
}

//=====================================
// CvReligionXMLEntries
//=====================================
/// Constructor
CvReligionXMLEntries::CvReligionXMLEntries(void)
{

}

/// Destructor
CvReligionXMLEntries::~CvReligionXMLEntries(void)
{
	DeleteArray();
}

/// Returns vector of trait entries
std::vector<CvReligionEntry*>& CvReligionXMLEntries::GetReligionEntries()
{
	return m_paReligionEntries;
}

/// Number of defined traits
int CvReligionXMLEntries::GetNumReligions()
{
	return m_paReligionEntries.size();
}

/// Clear trait entries
void CvReligionXMLEntries::DeleteArray()
{
	for(std::vector<CvReligionEntry*>::iterator it = m_paReligionEntries.begin(); it != m_paReligionEntries.end(); ++it)
	{
		SAFE_DELETE(*it);
	}

	m_paReligionEntries.clear();
}

/// Get a specific entry
CvReligionEntry* CvReligionXMLEntries::GetEntry(int index)
{
	return m_paReligionEntries[index];
}

//=====================================
// CvReligion
//=====================================
/// Default Constructor
CvReligion::CvReligion()
	: m_eReligion(NO_RELIGION)
	, m_eFounder(NO_PLAYER)
	, m_iHolyCityX(-1)
	, m_iHolyCityY(-1)
	, m_iTurnFounded(-1)
	, m_bPantheon(false)
	, m_bEnhanced(false)
{
	ZeroMemory(m_szCustomName, sizeof(m_szCustomName));
}

/// Constructor
CvReligion::CvReligion(ReligionTypes eReligion, PlayerTypes eFounder, CvCity* pHolyCity, bool bPantheon)
	: m_eReligion(eReligion)
	, m_eFounder(eFounder)
	, m_bPantheon(bPantheon)
	, m_bEnhanced(false)
{
	if(pHolyCity)
	{
		m_iHolyCityX = pHolyCity->getX();
		m_iHolyCityY = pHolyCity->getY();
	}
	m_iTurnFounded = GC.getGame().getGameTurn();
	ZeroMemory(m_szCustomName, sizeof(m_szCustomName));
}

/// Serialization read
FDataStream& operator>>(FDataStream& loadFrom, CvReligion& writeTo)
{
	uint uiVersion;
	loadFrom >> uiVersion;

	loadFrom >> writeTo.m_eReligion;
	loadFrom >> writeTo.m_eFounder;
	loadFrom >> writeTo.m_iHolyCityX;
	loadFrom >> writeTo.m_iHolyCityY;
	loadFrom >> writeTo.m_iTurnFounded;

	if(uiVersion >= 2)
	{
		loadFrom >> writeTo.m_bPantheon;
	}
	else
	{
		writeTo.m_bPantheon = false;
	}

	if(uiVersion >= 4)
	{
		loadFrom >> writeTo.m_bEnhanced;
	}
	else
	{
		writeTo.m_bEnhanced = false;
	}

	ZeroMemory(writeTo.m_szCustomName, sizeof(writeTo.m_szCustomName));
	if(uiVersion >= 3)
	{
		loadFrom >> writeTo.m_szCustomName;
	}

	writeTo.m_Beliefs.Read(loadFrom);

	return loadFrom;
}

/// Serialization write
FDataStream& operator<<(FDataStream& saveTo, const CvReligion& readFrom)
{
	uint uiVersion = 4;
	saveTo << uiVersion;

	saveTo << readFrom.m_eReligion;
	saveTo << readFrom.m_eFounder;
	saveTo << readFrom.m_iHolyCityX;
	saveTo << readFrom.m_iHolyCityY;
	saveTo << readFrom.m_iTurnFounded;
	saveTo << readFrom.m_bPantheon;
	saveTo << readFrom.m_bEnhanced;
	saveTo << readFrom.m_szCustomName;

	readFrom.m_Beliefs.Write(saveTo);

	return saveTo;
}

CvString CvReligion::GetName() const
{
	CvReligionEntry* pEntry = GC.getReligionInfo(m_eReligion);
	CvAssertMsg(pEntry, "pEntry for religion not expected to be NULL. Please send Anton or Ed your save file and version.");
	if (pEntry)
	{
		CvString szReligionName = (m_szCustomName == NULL || strlen(m_szCustomName) == 0) ? pEntry->GetDescriptionKey() : m_szCustomName;
		return szReligionName;
	}

	const char* szReligionNameBackup = "No Religion";
	return szReligionNameBackup;
}

//=====================================
// CvReligionInCity
//=====================================
/// Default Constructor
CvReligionInCity::CvReligionInCity()
{
	m_eReligion = NO_RELIGION;
	m_bFoundedHere = false;
	m_iFollowers = 0;
	m_iPressure = 0;
	m_iNumTradeRoutesApplyingPressure = 0;
	m_iTemp = 0;
}

/// Constructor
CvReligionInCity::CvReligionInCity(ReligionTypes eReligion, bool bFoundedHere, int iFollowers, int iPressure):
	m_eReligion(eReligion),
	m_bFoundedHere(bFoundedHere),
	m_iFollowers(iFollowers),
	m_iPressure(iPressure),
	m_iNumTradeRoutesApplyingPressure(0),
	m_iTemp(0)
{
}

/// Serialization read
FDataStream& operator>>(FDataStream& loadFrom, CvReligionInCity& writeTo)
{
	uint uiVersion;
	loadFrom >> uiVersion;

	loadFrom >> writeTo.m_eReligion;
	loadFrom >> writeTo.m_bFoundedHere;
	loadFrom >> writeTo.m_iFollowers;
	loadFrom >> writeTo.m_iPressure;

	if (uiVersion == 2)
	{
		loadFrom >> writeTo.m_iNumTradeRoutesApplyingPressure;
	}
	else
	{
		writeTo.m_iNumTradeRoutesApplyingPressure = 0;
	}

	return loadFrom;
}

/// Serialization write
FDataStream& operator<<(FDataStream& saveTo, const CvReligionInCity& readFrom)
{
	uint uiVersion = 2;
	saveTo << uiVersion;

	saveTo << readFrom.m_eReligion;
	saveTo << readFrom.m_bFoundedHere;
	saveTo << readFrom.m_iFollowers;
	saveTo << readFrom.m_iPressure;
	saveTo << readFrom.m_iNumTradeRoutesApplyingPressure;

	return saveTo;
}

//=====================================
// CvGameReligions
//=====================================
/// Constructor
CvGameReligions::CvGameReligions(void):
	m_iMinimumFaithForNextPantheon(0)
{
}

/// Destructor
CvGameReligions::~CvGameReligions(void)
{

}

/// Initialize class data
void CvGameReligions::Init()
{
	m_iMinimumFaithForNextPantheon = GC.getRELIGION_MIN_FAITH_FIRST_PANTHEON();
	m_iMinimumFaithForNextPantheon *= GC.getGame().getGameSpeedInfo().getTrainPercent();
	m_iMinimumFaithForNextPantheon /= 100;
}

/// Handle turn-by-turn religious updates
void CvGameReligions::DoTurn()
{
	SpreadReligion();
}

/// Spread religious pressure into adjacent cities
void CvGameReligions::SpreadReligion()
{
	// Loop through all the players
	for(int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iI);
		if(kPlayer.isAlive())
		{
			// Loop through each of their cities
			int iLoop;
			CvCity* pLoopCity;
			for(pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
			{
				SpreadReligionToOneCity(pLoopCity);
			}
		}
	}
}

/// Spread religious pressure to one city
void CvGameReligions::SpreadReligionToOneCity(CvCity* pCity)
{
#ifdef AUI_WARNING_FIXES
	if (!pCity)
		return;
#endif

	// Used to calculate how many trade routes are applying pressure to this city. This resets the value so we get a true count every turn.
	pCity->GetCityReligions()->ResetNumTradeRoutePressure();

	// Is this a city where a religion was founded?
	if(pCity->GetCityReligions()->IsHolyCityAnyReligion())
	{
		pCity->GetCityReligions()->AddHolyCityPressure();
	}

	// Loop through all the players
	for(int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iI);
		if(kPlayer.isAlive())
		{
			int iSpyPressure = kPlayer.GetReligions()->GetSpyPressure();
			if (iSpyPressure > 0)
			{
				if (kPlayer.GetEspionage()->GetSpyIndexInCity(pCity) != -1)
				{
					ReligionTypes eReligionFounded = kPlayer.GetReligions()->GetReligionCreatedByPlayer();
					pCity->GetCityReligions()->AddSpyPressure(eReligionFounded, iSpyPressure);
				}
			}

			// Loop through each of their cities
			int iLoop;
			CvCity* pLoopCity;
			for(pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
			{
				// Ignore the same city
				if(pCity == pLoopCity)
				{
					continue;
				}

				// Does this city have a majority religion?
				ReligionTypes eMajorityReligion = pLoopCity->GetCityReligions()->GetReligiousMajority();
				if(eMajorityReligion == NO_RELIGION)
				{
					continue;
				}

				int iNumTradeRoutes = 0;
				int iPressure = GetAdjacentCityReligiousPressure (eMajorityReligion, pLoopCity, pCity, iNumTradeRoutes, false);
				if (iPressure > 0)
				{
					pCity->GetCityReligions()->AddReligiousPressure(FOLLOWER_CHANGE_ADJACENT_PRESSURE, eMajorityReligion, iPressure);
					if (iNumTradeRoutes != 0)
					{
						pCity->GetCityReligions()->IncrementNumTradeRouteConnections(eMajorityReligion, iNumTradeRoutes);
					}
				}
			}
		}
	}
}

/// Religious activities at the start of a player's turn
void CvGameReligions::DoPlayerTurn(CvPlayer& kPlayer)
{
	AI_PERF_FORMAT("AI-perf.csv", ("CvGameReligions::DoPlayerTurn, Turn %03d, %s", GC.getGame().getElapsedGameTurns(), kPlayer.getCivilizationShortDescription()) );
	bool bCouldAtStartAffordFaithPurchase = kPlayer.GetReligions()->CanAffordFaithPurchase();
	const PlayerTypes ePlayer = kPlayer.GetID();

	int iFaithPerTurn = kPlayer.GetTotalFaithPerTurn();
	if(iFaithPerTurn > 0)
	{
		kPlayer.ChangeFaith(iFaithPerTurn);
	}

	// If just now can afford missionary, add a notification
	bool bCanNowAffordFaithPurchase = kPlayer.GetReligions()->CanAffordFaithPurchase();
	if (kPlayer.GetFaithPurchaseType() == NO_AUTOMATIC_FAITH_PURCHASE && !bCouldAtStartAffordFaithPurchase && bCanNowAffordFaithPurchase)
	{
		CvNotifications* pNotifications = kPlayer.GetNotifications();
		if(pNotifications)
		{
			CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_ENOUGH_FAITH_FOR_MISSIONARY");
			CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_ENOUGH_FAITH_FOR_MISSIONARY");
			pNotifications->Add(NOTIFICATION_CAN_BUILD_MISSIONARY, strBuffer, strSummary, -1, -1, -1);
		}
	}

	// Check for pantheon or great prophet spawning (now restricted so must occur before Industrial era)
	if(kPlayer.GetFaith() > 0 && !kPlayer.isMinorCiv() && kPlayer.GetCurrentEra() < GC.getInfoTypeForString("ERA_INDUSTRIAL"))
	{
		if(CanCreatePantheon(kPlayer.GetID(), true) == FOUNDING_OK)
		{
			// Create the pantheon
			if(kPlayer.isHuman())
			{
				//If the player is human then a net message will be received which will pick the pantheon.
				CvNotifications* pNotifications = kPlayer.GetNotifications();
				if(pNotifications)
				{
					CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_ENOUGH_FAITH_FOR_PANTHEON");

					CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_ENOUGH_FAITH_FOR_PANTHEON");
					pNotifications->Add(NOTIFICATION_FOUND_PANTHEON, strBuffer, strSummary, -1, -1, -1);
				}
			}
			else
			{
				const BeliefTypes eBelief = kPlayer.GetReligionAI()->ChoosePantheonBelief();

#ifdef AUI_RELIGION_FIX_POSSIBLE_NULL_POINTER
				if (eBelief != NO_BELIEF)
				{
					FoundPantheon(ePlayer, eBelief);
				}
#else
				FoundPantheon(ePlayer, eBelief);
#endif
			}
		}

		switch (kPlayer.GetFaithPurchaseType())
		{
		case NO_AUTOMATIC_FAITH_PURCHASE:
		case FAITH_PURCHASE_SAVE_PROPHET:
				CheckSpawnGreatProphet(kPlayer);
				break;
		}
	}

	// Pick a Reformation belief?
	ReligionTypes eReligionCreated = GetReligionCreatedByPlayer(ePlayer);
	if (eReligionCreated > RELIGION_PANTHEON && !HasAddedReformationBelief(ePlayer) && kPlayer.GetPlayerPolicies()->HasPolicyGrantingReformationBelief())
	{
		if (!kPlayer.isHuman())
		{
			BeliefTypes eReformationBelief = kPlayer.GetReligionAI()->ChooseReformationBelief();
#ifdef AUI_RELIGION_FIX_POSSIBLE_NULL_POINTER
			if (eReformationBelief != NO_BELIEF)
			{
				AddReformationBelief(ePlayer, eReligionCreated, eReformationBelief);
			}
#else
			AddReformationBelief(ePlayer, eReligionCreated, eReformationBelief);
#endif
		}
		else
		{
			CvNotifications* pNotifications;
			pNotifications = kPlayer.GetNotifications();
			if(pNotifications)
			{
				CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_ADD_REFORMATION_BELIEF");
				CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_ADD_REFORMATION_BELIEF");
				pNotifications->Add(NOTIFICATION_ADD_REFORMATION_BELIEF, strBuffer, strSummary, -1, -1, -1);
			}
		}
	}

	// Automatic faith purchases?
	bool bSelectionStillValid = true;
	CvString szItemName = "";
	ReligionTypes eReligion = kPlayer.GetReligionAI()->GetReligionToSpread();

	switch (kPlayer.GetFaithPurchaseType())
	{
	case FAITH_PURCHASE_SAVE_PROPHET:
		if (eReligion <= RELIGION_PANTHEON && GetNumReligionsStillToFound() <= 0)
		{
			szItemName = GetLocalizedText("TXT_KEY_RO_AUTO_FAITH_PROPHET");
			bSelectionStillValid = false;
		}
		else if (kPlayer.GetCurrentEra() >= GC.getInfoTypeForString("ERA_INDUSTRIAL"))
		{
			szItemName = GetLocalizedText("TXT_KEY_RO_AUTO_FAITH_PROPHET");
			bSelectionStillValid = false;
		}
		break;

	case FAITH_PURCHASE_UNIT:
		{
			UnitTypes eUnit = (UnitTypes)kPlayer.GetFaithPurchaseIndex();
			CvUnitEntry *pkUnit = GC.getUnitInfo(eUnit);
			if (pkUnit)
			{
				szItemName = pkUnit->GetDescriptionKey();
			}

			if (!kPlayer.IsCanPurchaseAnyCity(false, false /* Don't worry about faith balance */, eUnit, NO_BUILDING, YIELD_FAITH))
			{
				bSelectionStillValid = false;
			}
			else
			{
				if (kPlayer.IsCanPurchaseAnyCity(true, true /* Check faith balance */, eUnit, NO_BUILDING, YIELD_FAITH))
				{
					CvCity *pCity = CvReligionAIHelpers::GetBestCityFaithUnitPurchase(kPlayer, eUnit, eReligion);
					if (pCity)
					{
						pCity->Purchase(eUnit, NO_BUILDING, NO_PROJECT, YIELD_FAITH);

						CvNotifications* pNotifications = kPlayer.GetNotifications();
						if(pNotifications)
						{
							CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_AUTOMATIC_FAITH_PURCHASE", szItemName, pCity->getNameKey());
							CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_AUTOMATIC_FAITH_PURCHASE");
							pNotifications->Add(NOTIFICATION_CAN_BUILD_MISSIONARY, strBuffer, strSummary, pCity->getX(), pCity->getY(), -1);
						}
					}
					else
					{
						bSelectionStillValid = false;
					}
				}
			}
		}
		break;
	case FAITH_PURCHASE_BUILDING:
		{
			BuildingTypes eBuilding = (BuildingTypes)kPlayer.GetFaithPurchaseIndex();
			CvBuildingEntry *pkBuilding = GC.getBuildingInfo(eBuilding);
			if (pkBuilding)
			{
				szItemName = pkBuilding->GetDescriptionKey();
			}

			if (!kPlayer.IsCanPurchaseAnyCity(false, false, NO_UNIT, eBuilding, YIELD_FAITH))
			{
				bSelectionStillValid = false;
			}
			else
			{
				if (kPlayer.IsCanPurchaseAnyCity(true, true /* Check faith balance */, NO_UNIT, eBuilding, YIELD_FAITH))
				{
					CvCity *pCity = CvReligionAIHelpers::GetBestCityFaithBuildingPurchase(kPlayer, eBuilding, eReligion);
					if (pCity)
					{
						pCity->Purchase(NO_UNIT, eBuilding, NO_PROJECT, YIELD_FAITH);

						CvNotifications* pNotifications = kPlayer.GetNotifications();
						if(pNotifications)
						{
							CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_AUTOMATIC_FAITH_PURCHASE", szItemName, pCity->getNameKey());
							CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_AUTOMATIC_FAITH_PURCHASE");
							pNotifications->Add(NOTIFICATION_CAN_BUILD_MISSIONARY, strBuffer, strSummary, -1, -1, -1);
						}
					}
					else
					{
						bSelectionStillValid = false;
					}
				}
			}
		}
		break;
	}

	if (!bSelectionStillValid)
	{
		CvNotifications* pNotifications = kPlayer.GetNotifications();
		if(pNotifications)
		{
			CvString strBuffer = GetLocalizedText("TXT_KEY_NOTIFICATION_NEED_NEW_AUTOMATIC_FAITH_SELECTION", szItemName);
			CvString strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_NEED_NEW_AUTOMATIC_FAITH_SELECTION");
			pNotifications->Add(NOTIFICATION_AUTOMATIC_FAITH_PURCHASE_STOPPED, strBuffer, strSummary, -1, -1, -1);
		}

		gDLL->SendFaithPurchase(kPlayer.GetID(), NO_AUTOMATIC_FAITH_PURCHASE, 0);
	}
}

/// Time to create a pantheon?
CvGameReligions::FOUNDING_RESULT CvGameReligions::CanCreatePantheon(PlayerTypes ePlayer, bool bCheckFaithTotal)
{
	if(ePlayer == NO_PLAYER)
		return FOUNDING_INVALID_PLAYER;

	CvPlayer& kPlayer = GET_PLAYER(ePlayer);
	const int iFaith = kPlayer.GetFaith();

	if(kPlayer.isMinorCiv())
	{
		return FOUNDING_INVALID_PLAYER;
	}

	if(HasCreatedPantheon(ePlayer) || HasCreatedReligion(ePlayer))
	{
		return FOUNDING_PLAYER_ALREADY_CREATED_PANTHEON;
	}

	if(bCheckFaithTotal && iFaith < GetMinimumFaithNextPantheon())
	{
		return FOUNDING_NOT_ENOUGH_FAITH;
	}

	// Has a religion been enhanced yet (and total number of religions/pantheons is equal to number of religions allowed)?
	ReligionList::const_iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); ++it)
#else
	for(it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); it++)
#endif
	{
		if(it->m_bEnhanced)
		{
			if (GetNumPantheonsCreated() >= GC.getMap().getWorldInfo().getMaxActiveReligions())
			{
				return FOUNDING_RELIGION_ENHANCED;
			}
		}
	}

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem) 
	{
		CvLuaArgsHandle args;
		args->Push(ePlayer);

		// Attempt to execute the game events.
		// Will return false if there are no registered listeners.
		bool bResult = false;
		if (LuaSupport::CallTestAll(pkScriptSystem, "PlayerCanFoundPantheon", args.get(), bResult))
		{
			if (bResult == false) 
			{
				return FOUNDING_INVALID_PLAYER;
			}
		}
	}

	if (GetAvailablePantheonBeliefs().size() == 0)
		return FOUNDING_NO_BELIEFS_AVAILABLE;

	return FOUNDING_OK;
}

/// Get the appropriate religion for this player to found next
ReligionTypes CvGameReligions::GetReligionToFound(PlayerTypes ePlayer)
{
	ReligionTypes eCivReligion;
	eCivReligion = GET_PLAYER(ePlayer).getCivilizationInfo().GetReligion();

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem) 
	{
		CvLuaArgsHandle args;
		args->Push(ePlayer);
		args->Push(eCivReligion);
		args->Push(HasBeenFounded(eCivReligion));

		int iValue = 0;
		if (LuaSupport::CallAccumulator(pkScriptSystem, "GetReligionToFound", args.get(), iValue)) 
		{
#ifdef AUI_WARNING_FIXES
			if (uint(iValue) < GC.getNumReligionInfos() && iValue != RELIGION_PANTHEON)
#else
			if (iValue >= 0 && iValue < GC.getNumReligionInfos() && iValue != RELIGION_PANTHEON)
#endif
			{
				eCivReligion = (ReligionTypes)iValue;
			}
		}
	}

	if(!HasBeenFounded(eCivReligion))
	{
		CvReligionEntry* pEntry = GC.getReligionInfo(eCivReligion);
		if(pEntry)
		{
			return eCivReligion;
		}
	}

	// Need to "borrow" from another civ.  Loop through all religions looking for one that is eligible
#ifdef AUI_WARNING_FIXES
	for (uint iI = 0; iI < GC.getNumReligionInfos(); iI++)
#else
	for(int iI = 0; iI < GC.getNumReligionInfos(); iI++)
#endif
	{
		ReligionTypes eReligion = (ReligionTypes)iI;
		CvReligionEntry* pEntry = GC.getReligionInfo(eReligion);
		if(!pEntry)
		{
			continue;
		}

		if(pEntry->GetID() == RELIGION_PANTHEON)
		{
			continue;
		}

		if(HasBeenFounded((ReligionTypes)pEntry->GetID()))
		{
			continue;
		}

		if(IsPreferredByCivInGame(eReligion))
		{
			continue;
		}

		return (eReligion);
	}

	// Will have to use a religion that someone else prefers
#ifdef AUI_WARNING_FIXES
	for (uint iI = 0; iI < GC.getNumReligionInfos(); iI++)
#else
	for(int iI = 0; iI < GC.getNumReligionInfos(); iI++)
#endif
	{
		ReligionTypes eReligion = (ReligionTypes)iI;
		CvReligionEntry* pEntry = GC.getReligionInfo(eReligion);
		if(!pEntry)
		{
			continue;
		}

		if(pEntry->GetID() == RELIGION_PANTHEON)
		{
			continue;
		}

		if(HasBeenFounded((ReligionTypes)pEntry->GetID()))
		{
			continue;
		}

		return (eReligion);
	}

	return NO_RELIGION;
}

/// Tell the game a new pantheon has been created
void CvGameReligions::FoundPantheon(PlayerTypes ePlayer, BeliefTypes eBelief)
{
	CvGame& kGame = GC.getGame();
	CvPlayer& kPlayer = GET_PLAYER(ePlayer);

	CvReligion newReligion(RELIGION_PANTHEON, ePlayer, NULL, true);
	newReligion.m_Beliefs.AddBelief(eBelief);

	// Found it
	m_CurrentReligions.push_back(newReligion);

	// Update game systems
	kPlayer.UpdateReligion();
	kPlayer.ChangeFaith(-GetMinimumFaithNextPantheon());

	int iIncrement = GC.getRELIGION_GAME_FAITH_DELTA_NEXT_PANTHEON();
	iIncrement *= GC.getGame().getGameSpeedInfo().getTrainPercent();
	iIncrement /= 100;
	SetMinimumFaithNextPantheon(GetMinimumFaithNextPantheon() + iIncrement);

#ifdef AUI_RELIGION_FIX_FOUND_PANTHEON_NULL_POINTER_DEREFERENCE
	CvCity* pCapitol = GET_PLAYER(ePlayer).getCapitalCity();
	if (pCapitol)
	{
#endif
	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem) 
	{
		CvLuaArgsHandle args;
		args->Push(ePlayer);
#ifdef AUI_RELIGION_FIX_FOUND_PANTHEON_NULL_POINTER_DEREFERENCE
		args->Push(pCapitol->GetID());
#else
		args->Push(GET_PLAYER(ePlayer).getCapitalCity()->GetID());
#endif
		args->Push(RELIGION_PANTHEON);
		args->Push(eBelief);

		bool bResult;
		LuaSupport::CallHook(pkScriptSystem, "PantheonFounded", args.get(), bResult);
	}
#ifdef AUI_RELIGION_FIX_FOUND_PANTHEON_NULL_POINTER_DEREFERENCE
	}
#endif

	// Spread the pantheon into each of their cities
	int iLoop;
	CvCity* pLoopCity;
	for(pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
	{
		// Add enough pressure to make this the likely majority religion
		pLoopCity->GetCityReligions()->AddReligiousPressure(FOLLOWER_CHANGE_PANTHEON_FOUNDED, newReligion.m_eReligion, GC.getRELIGION_ATHEISM_PRESSURE_PER_POP() * pLoopCity->getPopulation() * 2);
	}

	// Send out messaging
	CvReligionEntry* pEntry = GC.getReligionInfo(newReligion.m_eReligion);
	CvBeliefEntry* pBelief = GC.getBeliefInfo(eBelief);
	if(pEntry && pBelief)
	{
		//Add replay message.
		Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_PANTHEON_FOUNDED_S");
		Localization::String replayText = Localization::Lookup("TXT_KEY_NOTIFICATION_PANTHEON_FOUNDED");
		replayText << kPlayer.getCivilizationShortDescriptionKey() << pBelief->getShortDescription() << pBelief->GetDescriptionKey();

		kGame.addReplayMessage(REPLAY_MESSAGE_PANTHEON_FOUNDED, newReligion.m_eFounder, replayText.toUTF8());

		for(int iNotifyLoop = 0; iNotifyLoop < MAX_MAJOR_CIVS; ++iNotifyLoop){
			PlayerTypes eNotifyPlayer = (PlayerTypes) iNotifyLoop;
			CvPlayerAI& kCurNotifyPlayer = GET_PLAYER(eNotifyPlayer);
			CvNotifications* pNotifications = kCurNotifyPlayer.GetNotifications();
			if(pNotifications){
				// Message slightly different for founder player
				if(newReligion.m_eFounder == eNotifyPlayer)
				{
					Localization::String localizedText = GetLocalizedText("TXT_KEY_NOTIFICATION_PANTHEON_FOUNDED_ACTIVE_PLAYER");
					localizedText << pBelief->getShortDescription() << pBelief->GetDescriptionKey();

					pNotifications->Add(NOTIFICATION_PANTHEON_FOUNDED_ACTIVE_PLAYER, localizedText.toUTF8(), strSummary.toUTF8(), -1, -1, -1);
				}
				else
				{
					//If the notify player has not met this civ yet, display a more ambiguous notification.
					CvTeam& kTeam = GET_TEAM(kCurNotifyPlayer.getTeam());
					if(kTeam.isHasMet(kPlayer.getTeam()))
					{
						pNotifications->Add(NOTIFICATION_PANTHEON_FOUNDED, replayText.toUTF8(), strSummary.toUTF8(), -1, -1, -1);
					}
					else
					{
						Localization::String unknownFoundedText = Localization::Lookup("TXT_KEY_NOTIFICATION_PANTHEON_FOUNDED_UNKNOWN");
						unknownFoundedText << pBelief->getShortDescription() << pBelief->GetDescriptionKey();

						pNotifications->Add(NOTIFICATION_PANTHEON_FOUNDED, unknownFoundedText.toUTF8(), strSummary.toUTF8(), -1, -1, -1);
					}
				}
			}
		}

		// Logging
		if(GC.getLogging())
		{
			CvString strLogMsg;
			strLogMsg = kPlayer.getCivilizationShortDescription();
			strLogMsg += ", PANTHEON FOUNDED, ";
			strLogMsg += pEntry->GetDescription();
			LogReligionMessage(strLogMsg);
		}

		//Achievements!
		if(ePlayer == GC.getGame().getActivePlayer())
		{
			gDLL->UnlockAchievement(ACHIEVEMENT_XP1_10);
		}
	}

	GC.GetEngineUserInterface()->setDirty(CityInfo_DIRTY_BIT, true);
}

/// Create a new religion
void CvGameReligions::FoundReligion(PlayerTypes ePlayer, ReligionTypes eReligion, const char* szCustomName, BeliefTypes eBelief1, BeliefTypes eBelief2, BeliefTypes eBelief3, BeliefTypes eBelief4, CvCity* pkHolyCity)
{
	CvPlayer& kPlayer = GET_PLAYER(ePlayer);

	CvReligion kReligion(eReligion, ePlayer, pkHolyCity, false);

	// Copy over belief from your pantheon
	BeliefTypes eBelief = GC.getGame().GetGameReligions()->GetBeliefInPantheon(kPlayer.GetID());
	if(eBelief != NO_BELIEF)
		kReligion.m_Beliefs.AddBelief(eBelief);

	kReligion.m_Beliefs.AddBelief(eBelief1);
	kReligion.m_Beliefs.AddBelief(eBelief2);

	if(eBelief3 != NO_BELIEF)
	{
		kReligion.m_Beliefs.AddBelief(eBelief3);
	}

	if(eBelief4 != NO_BELIEF)
	{
		kReligion.m_Beliefs.AddBelief(eBelief4);
	}

	if(szCustomName != NULL && strlen(szCustomName) <= sizeof(kReligion.m_szCustomName))
	{
		strcpy_s(kReligion.m_szCustomName, szCustomName);
	}

	// Found it
	m_CurrentReligions.push_back(kReligion);

	// Inform the holy city
	pkHolyCity->GetCityReligions()->DoReligionFounded(kReligion.m_eReligion);

	// Update game systems
	kPlayer.UpdateReligion();
	kPlayer.GetReligions()->SetFoundingReligion(false);

	// In case we have another prophet sitting around, make sure he's set to this religion
	int iLoopUnit;
	CvUnit* pLoopUnit;
	for(pLoopUnit = kPlayer.firstUnit(&iLoopUnit); pLoopUnit != NULL; pLoopUnit = kPlayer.nextUnit(&iLoopUnit))
	{
		if(pLoopUnit->getUnitInfo().IsFoundReligion())
		{
			pLoopUnit->GetReligionData()->SetReligion(eReligion);
			pLoopUnit->GetReligionData()->SetSpreadsLeft(pLoopUnit->getUnitInfo().GetReligionSpreads());
			pLoopUnit->GetReligionData()->SetReligiousStrength(pLoopUnit->getUnitInfo().GetReligiousStrength());
		}
	}

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(ePlayer);
		args->Push(pkHolyCity->GetID());
		args->Push(eReligion);
		args->Push(eBelief);
		args->Push(eBelief1);
		args->Push(eBelief2);
		args->Push(eBelief3);
		args->Push(eBelief4);

		bool bResult;
		LuaSupport::CallHook(pkScriptSystem, "ReligionFounded", args.get(), bResult);
	}

	// Send out messaging
	CvReligionEntry* pEntry = GC.getReligionInfo(kReligion.m_eReligion);
	if(pEntry)
	{
		//Add replay message
		CvString szReligionName = kReligion.GetName();
		Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_RELIGION_FOUNDED_S");
		Localization::String replayText = Localization::Lookup("TXT_KEY_NOTIFICATION_RELIGION_FOUNDED");
		replayText << kPlayer.getCivilizationShortDescriptionKey() << szReligionName << pkHolyCity->getNameKey();

		GC.getGame().addReplayMessage(REPLAY_MESSAGE_RELIGION_FOUNDED, kReligion.m_eFounder, replayText.toUTF8(), kReligion.m_iHolyCityX, kReligion.m_iHolyCityY);

		//Notify the masses
		for(int iNotifyLoop = 0; iNotifyLoop < MAX_MAJOR_CIVS; ++iNotifyLoop){
			PlayerTypes eNotifyPlayer = (PlayerTypes) iNotifyLoop;
			CvPlayerAI& kNotifyPlayer = GET_PLAYER(eNotifyPlayer);
			CvNotifications* pNotifications = kNotifyPlayer.GetNotifications();
			if(pNotifications){
				// Message slightly different for founder player
				if(kReligion.m_eFounder == eNotifyPlayer)
				{
					Localization::String localizedText = Localization::Lookup("TXT_KEY_NOTIFICATION_RELIGION_FOUNDED_ACTIVE_PLAYER");
					localizedText << szReligionName << pkHolyCity->getNameKey();

					pNotifications->Add(NOTIFICATION_RELIGION_FOUNDED_ACTIVE_PLAYER, localizedText.toUTF8(), strSummary.toUTF8(), -1, -1, -1);
				}
				else
				{
					CvTeam& kNotifyTeam = GET_TEAM(kNotifyPlayer.getTeam());

					if(kNotifyTeam.isHasMet(kPlayer.getTeam()))
					{
						pNotifications->Add(NOTIFICATION_RELIGION_FOUNDED, replayText.toUTF8(), strSummary.toUTF8(), -1, -1, -1);
					}
					else
					{
						Localization::String unknownCivText = Localization::Lookup("TXT_KEY_NOTIFICATION_RELIGION_FOUNDED_UNKNOWN");
						unknownCivText << szReligionName;

						pNotifications->Add(NOTIFICATION_RELIGION_FOUNDED, unknownCivText.toUTF8(), strSummary.toUTF8(), -1, -1, -1);
					}
				}
			}
		}

		// Logging
		if(GC.getLogging())
		{
			CvString strLogMsg;
			strLogMsg = kPlayer.getCivilizationShortDescription();
			strLogMsg += ", RELIGION FOUNDED, ";
			strLogMsg += pkHolyCity->getName();
			strLogMsg += ", ";
			strLogMsg += pEntry->GetDescription();
			LogReligionMessage(strLogMsg);
		}

		//Achievements!
		if(ePlayer == GC.getGame().getActivePlayer())
		{
			gDLL->UnlockAchievement(ACHIEVEMENT_XP1_11);
		}
	}
	GC.GetEngineUserInterface()->setDirty(CityInfo_DIRTY_BIT, true);
}

/// Can the supplied religion be created?
CvGameReligions::FOUNDING_RESULT CvGameReligions::CanFoundReligion(PlayerTypes ePlayer, ReligionTypes eReligion, const char* szCustomName, BeliefTypes eBelief1, BeliefTypes eBelief2, BeliefTypes eBelief3, BeliefTypes eBelief4, CvCity* pkHolyCity)
{
	if(GetNumReligionsStillToFound() <= 0)
		return FOUNDING_NO_RELIGIONS_AVAILABLE;

	if(ePlayer == NO_PLAYER)
		return FOUNDING_INVALID_PLAYER;

	if(HasCreatedReligion(ePlayer))
		return FOUNDING_PLAYER_ALREADY_CREATED_RELIGION;

	CvPlayer& kPlayer = GET_PLAYER(ePlayer);

	CvReligion kReligion(eReligion, ePlayer, pkHolyCity, false);

	// Copy over belief from your pantheon
	BeliefTypes eBelief = GC.getGame().GetGameReligions()->GetBeliefInPantheon(kPlayer.GetID());
	if(eBelief != NO_BELIEF)
		kReligion.m_Beliefs.AddBelief(eBelief);

	kReligion.m_Beliefs.AddBelief(eBelief1);
	kReligion.m_Beliefs.AddBelief(eBelief2);

	if(eBelief3 != NO_BELIEF)
	{
		kReligion.m_Beliefs.AddBelief(eBelief3);
	}

	if(eBelief4 != NO_BELIEF)
	{
		kReligion.m_Beliefs.AddBelief(eBelief4);
	}

	if(szCustomName != NULL && strlen(szCustomName) <= sizeof(kReligion.m_szCustomName))
	{
		strcpy_s(kReligion.m_szCustomName, szCustomName);
	}

	// Now see if there are any conflicts.
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (ReligionList::const_iterator it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); ++it)
#else
	for(ReligionList::const_iterator it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); it++)
#endif
	{
		if ((*it).m_eFounder != ePlayer)	// Only check other player's religions
		{
			if(kReligion.m_eReligion == (*it).m_eReligion)
				return FOUNDING_RELIGION_IN_USE;

			for(int iSrcBelief = (*it).m_Beliefs.GetNumBeliefs(); iSrcBelief--;)
			{
				BeliefTypes eSrcBelief = (*it).m_Beliefs.GetBelief(iSrcBelief);
				if(eSrcBelief != NO_BELIEF)
				{
					for(int iDestBelief = kReligion.m_Beliefs.GetNumBeliefs(); iDestBelief--;)
					{
						BeliefTypes eDestBelief = kReligion.m_Beliefs.GetBelief(iDestBelief);
						if(eDestBelief != NO_BELIEF && eDestBelief == eSrcBelief)
							return FOUNDING_BELIEF_IN_USE;
					}
				}
			}
		}
	}

	return FOUNDING_OK;
}

/// Add new beliefs to an existing religion
void CvGameReligions::EnhanceReligion(PlayerTypes ePlayer, ReligionTypes eReligion, BeliefTypes eBelief1, BeliefTypes eBelief2)
{
	bool bFoundIt = false;
	CvPlayer& kPlayer = GET_PLAYER(ePlayer);
	ReligionList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); ++it)
#else
	for(it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); it++)
#endif
	{
		if(it->m_eReligion == eReligion)
		{
			bFoundIt = true;
			break;
		}
	}
	if(!bFoundIt)
	{
		CvAssertMsg(false, "Internal error in religion code.");
		return;
	}

	it->m_Beliefs.AddBelief(eBelief1);
	it->m_Beliefs.AddBelief(eBelief2);
	it->m_bEnhanced = true;

	// Update game systems
	UpdateAllCitiesThisReligion(eReligion);
	kPlayer.UpdateReligion();

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem) 
	{
		CvLuaArgsHandle args;
		args->Push(ePlayer);
		args->Push(eReligion);
		args->Push(eBelief1);
		args->Push(eBelief2);

		bool bResult;
		LuaSupport::CallHook(pkScriptSystem, "ReligionEnhanced", args.get(), bResult);
	}

	//Notify the masses
	for(int iNotifyLoop = 0; iNotifyLoop < MAX_MAJOR_CIVS; ++iNotifyLoop){
		PlayerTypes eNotifyPlayer = (PlayerTypes) iNotifyLoop;
		CvPlayerAI& kNotifyPlayer = GET_PLAYER(eNotifyPlayer);
		CvNotifications* pNotifications = kNotifyPlayer.GetNotifications();
		if(pNotifications){
			Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_RELIGION_ENHANCED_S");
			Localization::String notificationText = Localization::Lookup("TXT_KEY_NOTIFICATION_RELIGION_ENHANCED");
			notificationText << kPlayer.getCivilizationShortDescriptionKey() << it->GetName();

			// Message slightly different for enhancing player
			if(ePlayer == eNotifyPlayer)
			{
				Localization::String localizedText = Localization::Lookup("TXT_KEY_NOTIFICATION_RELIGION_ENHANCED_ACTIVE_PLAYER");
				localizedText << it->GetName();

				pNotifications->Add(NOTIFICATION_RELIGION_ENHANCED_ACTIVE_PLAYER, localizedText.toUTF8(), strSummary.toUTF8(), -1, -1, -1);
			}
			else
			{
				CvTeam& kNotifyTeam = GET_TEAM(kNotifyPlayer.getTeam());
				if(kNotifyTeam.isHasMet(kPlayer.getTeam()))
				{
					pNotifications->Add(NOTIFICATION_RELIGION_ENHANCED, notificationText.toUTF8(), strSummary.toUTF8(), -1, -1, -1);
				}
				else
				{
					Localization::String unknownText = Localization::Lookup("TXT_KEY_NOTIFICATION_RELIGION_ENHANCED_UNKNOWN");
					unknownText << it->GetName();

					pNotifications->Add(NOTIFICATION_RELIGION_ENHANCED, unknownText.toUTF8(), strSummary.toUTF8(), -1, -1, -1);
				}
			}
		}

		// Logging
		if(GC.getLogging())
		{
			CvString strLogMsg;
			strLogMsg = kPlayer.getCivilizationShortDescription();
			strLogMsg += ", RELIGION ENHANCED";
			LogReligionMessage(strLogMsg);
		}
	}
	GC.GetEngineUserInterface()->setDirty(CityInfo_DIRTY_BIT, true);
}

/// Can the new beliefs be added to the religion?
CvGameReligions::FOUNDING_RESULT CvGameReligions::CanEnhanceReligion(PlayerTypes ePlayer, ReligionTypes eReligion, BeliefTypes eBelief1, BeliefTypes eBelief2)
{
	bool bFoundIt = false;
	ReligionList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); ++it)
#else
	for(it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); it++)
#endif
	{
		if(it->m_eReligion == eReligion && it->m_eFounder == ePlayer)
		{
			bFoundIt = true;
			break;
		}
	}

	if(bFoundIt)
	{
		if(eBelief1 != NO_BELIEF && IsInSomeReligion(eBelief1))
			return FOUNDING_BELIEF_IN_USE;
		if(eBelief2 != NO_BELIEF && IsInSomeReligion(eBelief2))
			return FOUNDING_BELIEF_IN_USE;

		return FOUNDING_OK;
	}

	return FOUNDING_RELIGION_IN_USE;
}

/// Add an extra belief to a religion (through a policy)
void CvGameReligions::AddReformationBelief(PlayerTypes ePlayer, ReligionTypes eReligion, BeliefTypes eBelief1)
{
	bool bFoundIt = false;
	CvPlayer& kPlayer = GET_PLAYER(ePlayer);
	ReligionList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); ++it)
#else
	for(it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); it++)
#endif
	{
		if(it->m_eReligion == eReligion)
		{
			bFoundIt = true;
			break;
		}
	}
	if(!bFoundIt)
	{
		CvAssertMsg(false, "Internal error in religion code.");
		return;
	}

	it->m_Beliefs.AddBelief(eBelief1);

	// Update game systems
	UpdateAllCitiesThisReligion(eReligion);
	kPlayer.UpdateReligion();

	//Notify the masses
	for(int iNotifyLoop = 0; iNotifyLoop < MAX_MAJOR_CIVS; ++iNotifyLoop){
		PlayerTypes eNotifyPlayer = (PlayerTypes) iNotifyLoop;
		CvPlayerAI& kNotifyPlayer = GET_PLAYER(eNotifyPlayer);
		CvNotifications* pNotifications = kNotifyPlayer.GetNotifications();
		if(pNotifications){
			Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_REFORMATION_BELIEF_ADDED_S");
			Localization::String notificationText = Localization::Lookup("TXT_KEY_NOTIFICATION_REFORMATION_BELIEF_ADDED");
			notificationText << kPlayer.getCivilizationShortDescriptionKey() << it->GetName();

			// Message slightly different for reformation player
			if(ePlayer == eNotifyPlayer)
			{
				Localization::String localizedText = Localization::Lookup("TXT_KEY_NOTIFICATION_REFORMATION_BELIEF_ADDED_ACTIVE_PLAYER");
				localizedText << it->GetName();

				pNotifications->Add(NOTIFICATION_REFORMATION_BELIEF_ADDED_ACTIVE_PLAYER, localizedText.toUTF8(), strSummary.toUTF8(), -1, -1, -1);
			}
			else
			{
				CvTeam& kNotifyTeam = GET_TEAM(kNotifyPlayer.getTeam());
				if(kNotifyTeam.isHasMet(kPlayer.getTeam()))
				{
					pNotifications->Add(NOTIFICATION_REFORMATION_BELIEF_ADDED, notificationText.toUTF8(), strSummary.toUTF8(), -1, -1, -1);
				}
				else
				{
					Localization::String unknownText = Localization::Lookup("TXT_KEY_NOTIFICATION_REFORMATION_BELIEF_ADDED_UNKNOWN");
					unknownText << it->GetName();

					pNotifications->Add(NOTIFICATION_REFORMATION_BELIEF_ADDED, unknownText.toUTF8(), strSummary.toUTF8(), -1, -1, -1);
				}
			}
		}

		// Logging
		if(GC.getLogging())
		{
			CvString strLogMsg;
			strLogMsg = kPlayer.getCivilizationShortDescription();
			strLogMsg += ", REFORMATION BELIEF ADDED";
			LogReligionMessage(strLogMsg);
		}
	}
	GC.GetEngineUserInterface()->setDirty(CityInfo_DIRTY_BIT, true);
}

/// Move the Holy City for a religion (useful for scenario scripting)
void CvGameReligions::SetHolyCity(ReligionTypes eReligion, CvCity* pkHolyCity)
{
	ReligionList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); ++it)
#else
	for(it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); it++)
#endif
	{
		// If talking about a pantheon, make sure to match the player
		if(it->m_eReligion == eReligion)
		{
			it->m_iHolyCityX = pkHolyCity->getX();
			it->m_iHolyCityY = pkHolyCity->getY();
			break;
		}
	}
}

/// Switch founder for a religion (useful for scenario scripting)
void CvGameReligions::SetFounder(ReligionTypes eReligion, PlayerTypes eFounder)
{
	ReligionList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); ++it)
#else
	for(it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); it++)
#endif
	{
		// If talking about a pantheon, make sure to match the player
		if(it->m_eReligion == eReligion)
		{
			it->m_eFounder = eFounder;
			break;
		}
	}
}

/// After a religion is enhanced, the newly chosen beliefs need to be turned on in all cities
void CvGameReligions::UpdateAllCitiesThisReligion(ReligionTypes eReligion)
{
	int iLoop;

	for(int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
	{
		PlayerTypes ePlayer = (PlayerTypes)iPlayer;
		CvPlayer& kPlayer = GET_PLAYER(ePlayer);
		if(kPlayer.isAlive())
		{
			for(CvCity* pCity = kPlayer.firstCity(&iLoop); pCity != NULL; pCity = kPlayer.nextCity(&iLoop))
			{
				if(pCity->GetCityReligions()->GetReligiousMajority() == eReligion)
				{
					pCity->UpdateReligion(eReligion);
				}
			}
		}
	}
}

/// Return a pointer to a religion that has been founded
const CvReligion* CvGameReligions::GetReligion(ReligionTypes eReligion, PlayerTypes ePlayer) const
{
	ReligionList::const_iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); ++it)
#else
	for(it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); it++)
#endif
	{
		// If talking about a pantheon, make sure to match the player
		if(it->m_eReligion == eReligion && it->m_eReligion == RELIGION_PANTHEON)
		{
			if(it->m_eFounder == ePlayer)
			{
				return it;
			}
		}
		else if(it->m_eReligion == eReligion)
		{
			return it;
		}
	}

	return NULL;
}

/// Has some religion already claimed this belief?
bool CvGameReligions::IsInSomeReligion(BeliefTypes eBelief) const
{
	ReligionList::const_iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); ++it)
#else
	for(it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); it++)
#endif
	{
		if(it->m_Beliefs.HasBelief(eBelief))
		{
			return true;
		}
	}

	return false;
}

/// Get the belief in this player's pantheon
BeliefTypes CvGameReligions::GetBeliefInPantheon(PlayerTypes ePlayer) const
{
	ReligionList::const_iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); ++it)
#else
	for(it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); it++)
#endif
	{
		if(it->m_eFounder == ePlayer && it->m_bPantheon)
		{
			return (BeliefTypes)it->m_Beliefs.GetBelief(0);
		}
	}

	return NO_BELIEF;
}

/// Has this player created a pantheon?
bool CvGameReligions::HasCreatedPantheon(PlayerTypes ePlayer) const
{
	ReligionList::const_iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); ++it)
#else
	for(it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); it++)
#endif
	{
		if(it->m_eFounder == ePlayer && it->m_bPantheon)
		{
			return true;
		}
	}

	return false;
}

/// How many players have created a pantheon?
int CvGameReligions::GetNumPantheonsCreated() const
{
	int iRtnValue = 0;

	for(int iI = 0; iI < MAX_MAJOR_CIVS; iI++)
	{
		ReligionList::const_iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
		for (it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); ++it)
#else
		for(it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); it++)
#endif
		{
			if (it->m_eFounder == iI)
			{
				iRtnValue++;
				break;
			}
		}
	}

	return iRtnValue;
}

/// List of beliefs that can be adopted by pantheons
std::vector<BeliefTypes> CvGameReligions::GetAvailablePantheonBeliefs()
{
	std::vector<BeliefTypes> availableBeliefs;

	CvBeliefXMLEntries* pkBeliefs = GC.GetGameBeliefs();
	const int iNumBeleifs = pkBeliefs->GetNumBeliefs();

	availableBeliefs.reserve(iNumBeleifs);
	for(int iI = 0; iI < iNumBeleifs; iI++)
	{
		const BeliefTypes eBelief(static_cast<BeliefTypes>(iI));
		if(!IsInSomeReligion(eBelief))
		{
			CvBeliefEntry* pEntry = pkBeliefs->GetEntry(eBelief);
			if(pEntry && pEntry->IsPantheonBelief())
			{
				availableBeliefs.push_back(eBelief);
			}
		}
	}

	return availableBeliefs;
}

// Is the supplied belief available to a pantheon?
#ifdef AUI_CONSTIFY
bool CvGameReligions::IsPantheonBeliefAvailable(BeliefTypes eBelief) const
#else
bool CvGameReligions::IsPantheonBeliefAvailable(BeliefTypes eBelief)
#endif
{
	CvBeliefXMLEntries* pkBeliefs = GC.GetGameBeliefs();
	if(!IsInSomeReligion(eBelief))
	{
		CvBeliefEntry* pEntry = pkBeliefs->GetEntry(eBelief);
		if(pEntry && pEntry->IsPantheonBelief())
		{
			return true;
		}
	}

	return false;
}

/// Number of followers of this religion
int CvGameReligions::GetNumFollowers(ReligionTypes eReligion) const
{
	int iRtnValue = 0;

	// Loop through all the players
	for(int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iI);
		if(kPlayer.isAlive())
		{
			// Loop through each of their cities
			int iLoop;
			CvCity* pLoopCity;
			for(pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
			{
				iRtnValue += pLoopCity->GetCityReligions()->GetNumFollowers(eReligion);
			}
		}
	}
	return iRtnValue;
}

/// Number of cities following this religion
int CvGameReligions::GetNumCitiesFollowing(ReligionTypes eReligion) const
{
	int iRtnValue = 0;

	// Loop through all the players
	for(int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iI);
		if(kPlayer.isAlive())
		{
			// Loop through each of their cities
			int iLoop;
			CvCity* pLoopCity;
			for(pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
			{
				if(pLoopCity->GetCityReligions()->GetReligiousMajority() == eReligion)
				{
					iRtnValue++;
				}
			}
		}
	}
	return iRtnValue;
}

/// Has this player created a religion?
bool CvGameReligions::HasCreatedReligion(PlayerTypes ePlayer) const
{
#ifdef AUI_RELIGION_REFORMAT_HAS_CREATED_RELIGION
	return (GetReligionCreatedByPlayer(ePlayer) > RELIGION_PANTHEON);
#else
	if (GetReligionCreatedByPlayer(ePlayer) > RELIGION_PANTHEON)
	{
		return true;
	}

	return false;
#endif
}

/// Has this player reformed their religion?
bool CvGameReligions::HasAddedReformationBelief(PlayerTypes ePlayer) const
{
	CvBeliefXMLEntries* pkBeliefs = GC.GetGameBeliefs();

	ReligionTypes eReligion = GetReligionCreatedByPlayer(ePlayer);
    if (eReligion > RELIGION_PANTHEON)
	{
		const CvReligion* pMyReligion = GetReligion(eReligion, ePlayer);
		if (pMyReligion)
		{
			for(int iI = 0; iI < pMyReligion->m_Beliefs.GetNumBeliefs(); iI++)
			{
				const BeliefTypes eBelief = pMyReligion->m_Beliefs.GetBelief(iI);
				CvBeliefEntry* pEntry = pkBeliefs->GetEntry((int)eBelief);
				if (pEntry && pEntry->IsReformationBelief())
				{
					return true;
				}
			}
		}
	}

	return false;
}

/// Does this player qualify for founder benefits?
bool CvGameReligions::IsEligibleForFounderBenefits(ReligionTypes eReligion, PlayerTypes ePlayer) const
{
	const CvReligion* religion = GetReligion(eReligion, NO_PLAYER);
	if(religion)
	{
		CvPlot* pHolyCityPlot = NULL;
		pHolyCityPlot = GC.getMap().plot(religion->m_iHolyCityX, religion->m_iHolyCityY);

		if(religion->m_eFounder == ePlayer)
		{
			if(!religion->m_bPantheon)
			{
				if(pHolyCityPlot && pHolyCityPlot->getOwner() == ePlayer)
				{
					return true;
				}
			}
		}
	}

	return false;
}

/// Is this city state friendly with the player that founded this religion?
#ifdef AUI_CONSTIFY
bool CvGameReligions::IsCityStateFriendOfReligionFounder(ReligionTypes eReligion, PlayerTypes ePlayer) const
#else
bool CvGameReligions::IsCityStateFriendOfReligionFounder(ReligionTypes eReligion, PlayerTypes ePlayer)
#endif
{
	const CvReligion* religion = GetReligion(eReligion, NO_PLAYER);
	if(religion)
	{
		CvPlayer& kMinor = GET_PLAYER(ePlayer);
		CvPlayer& kFounder = GET_PLAYER(religion->m_eFounder);
		if(!kFounder.isMinorCiv() && kMinor.GetMinorCivAI()->IsFriends(religion->m_eFounder))
		{
			return true;
		}
	}

	return false;
}

/// Get the religion this player created
ReligionTypes CvGameReligions::GetReligionCreatedByPlayer(PlayerTypes ePlayer) const
{
	ReligionTypes eRtnValue = NO_RELIGION;

	ReligionList::const_iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); ++it)
#else
	for(it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); it++)
#endif
	{
		if(it->m_eFounder == ePlayer)
		{
			if(!it->m_bPantheon)
			{
				eRtnValue = it->m_eReligion;
			}
		}
	}

	return eRtnValue;
}

/// Get the religion for which this player is eligible for founder benefits
ReligionTypes CvGameReligions::GetFounderBenefitsReligion(PlayerTypes ePlayer) const
{
	ReligionTypes eReligion;

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(ePlayer);

		int iValue = 0;
		if (LuaSupport::CallAccumulator(pkScriptSystem, "GetFounderBenefitsReligion", args.get(), iValue))
		{
			eReligion = (ReligionTypes)iValue;
			return eReligion;
		}
	}

	eReligion = GetReligionCreatedByPlayer(ePlayer);

	if(IsEligibleForFounderBenefits(eReligion, ePlayer))
	{
		return eReligion;
	}

	return NO_RELIGION;
}

/// Number of religions founded so far (does not include pantheons)
int CvGameReligions::GetNumReligionsFounded() const
{
	int iRtnValue = 0;

	ReligionList::const_iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); ++it)
#else
	for(it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); it++)
#endif
	{
		if(!it->m_bPantheon)
		{
			iRtnValue++;
		}
	}

	return iRtnValue;
}

/// Number of religions enhanced so far
int CvGameReligions::GetNumReligionsEnhanced() const
{
	int iRtnValue = 0;

	ReligionList::const_iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); ++it)
#else
	for(it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); it++)
#endif
	{
		if (it->m_bEnhanced)
		{
			iRtnValue++;
		}
	}

	return iRtnValue;
}

/// Number of religions that still can be founded on this size map
int CvGameReligions::GetNumReligionsStillToFound() const
{
	return (GC.getMap().getWorldInfo().getMaxActiveReligions() - GetNumReligionsFounded());
}

/// List of beliefs that can be adopted by religion founders
std::vector<BeliefTypes> CvGameReligions::GetAvailableFounderBeliefs()
{
	std::vector<BeliefTypes> availableBeliefs;

	CvBeliefXMLEntries* pkBeliefs = GC.GetGameBeliefs();
	const int iNumBeleifs = pkBeliefs->GetNumBeliefs();

	availableBeliefs.reserve(iNumBeleifs);
	for(int iI = 0; iI < iNumBeleifs; iI++)
	{
		const BeliefTypes eBelief(static_cast<BeliefTypes>(iI));
		if(!IsInSomeReligion(eBelief))
		{
			CvBeliefEntry* pEntry = pkBeliefs->GetEntry(eBelief);
			if(pEntry && pEntry->IsFounderBelief())
			{
				availableBeliefs.push_back(eBelief);
			}
		}
	}

	return availableBeliefs;
}

/// List of beliefs that can be adopted by religion followers
std::vector<BeliefTypes> CvGameReligions::GetAvailableFollowerBeliefs()
{
	std::vector<BeliefTypes> availableBeliefs;

	CvBeliefXMLEntries* pkBeliefs = GC.GetGameBeliefs();
	const int iNumBeleifs = pkBeliefs->GetNumBeliefs();

	availableBeliefs.reserve(iNumBeleifs);
	for(int iI = 0; iI < iNumBeleifs; iI++)
	{
		const BeliefTypes eBelief(static_cast<BeliefTypes>(iI));
		if(!IsInSomeReligion(eBelief))
		{
			CvBeliefEntry* pEntry = pkBeliefs->GetEntry(eBelief);
			if(pEntry && pEntry->IsFollowerBelief())
			{
				availableBeliefs.push_back(eBelief);
			}
		}
	}

	return availableBeliefs;
}

/// List of beliefs that enhance religions
std::vector<BeliefTypes> CvGameReligions::GetAvailableEnhancerBeliefs()
{
	std::vector<BeliefTypes> availableBeliefs;

	CvBeliefXMLEntries* pkBeliefs = GC.GetGameBeliefs();
	const int iNumBeleifs = pkBeliefs->GetNumBeliefs();

	availableBeliefs.reserve(iNumBeleifs);
	for(int iI = 0; iI < iNumBeleifs; iI++)
	{
		const BeliefTypes eBelief(static_cast<BeliefTypes>(iI));
		if(!IsInSomeReligion(eBelief))
		{
			CvBeliefEntry* pEntry = pkBeliefs->GetEntry(eBelief);
			if(pEntry && pEntry->IsEnhancerBelief())
			{
				availableBeliefs.push_back(eBelief);
			}
		}
	}

	return availableBeliefs;
}

/// List of all beliefs still available
std::vector<BeliefTypes> CvGameReligions::GetAvailableBonusBeliefs()
{
	std::vector<BeliefTypes> availableBeliefs;

	CvBeliefXMLEntries* pkBeliefs = GC.GetGameBeliefs();
	const int iNumBeleifs = pkBeliefs->GetNumBeliefs();

	availableBeliefs.reserve(iNumBeleifs);
	for(int iI = 0; iI < iNumBeleifs; iI++)
	{
		const BeliefTypes eBelief(static_cast<BeliefTypes>(iI));
		if(!IsInSomeReligion(eBelief))
		{
			CvBeliefEntry* pEntry = pkBeliefs->GetEntry(eBelief);
			if(pEntry && (pEntry->IsEnhancerBelief() || pEntry->IsFollowerBelief() || pEntry->IsFounderBelief() || pEntry->IsPantheonBelief()))
			{
				availableBeliefs.push_back(eBelief);
			}
		}
	}

	return availableBeliefs;
}

/// List of beliefs that are added with Reformation social policy
std::vector<BeliefTypes> CvGameReligions::GetAvailableReformationBeliefs()
{
	std::vector<BeliefTypes> availableBeliefs;

	CvBeliefXMLEntries* pkBeliefs = GC.GetGameBeliefs();
	const int iNumBeleifs = pkBeliefs->GetNumBeliefs();

	availableBeliefs.reserve(iNumBeleifs);
	for(int iI = 0; iI < iNumBeleifs; iI++)
	{
		const BeliefTypes eBelief(static_cast<BeliefTypes>(iI));
		if(!IsInSomeReligion(eBelief))
		{
			CvBeliefEntry* pEntry = pkBeliefs->GetEntry(eBelief);
			if(pEntry && pEntry->IsReformationBelief())
			{
				availableBeliefs.push_back(eBelief);
			}
		}
	}

	return availableBeliefs;
}

/// How much pressure is exerted between these cities?
#ifdef AUI_CONSTIFY
int CvGameReligions::GetAdjacentCityReligiousPressure(ReligionTypes eReligion, const CvCity* pFromCity, const CvCity* pToCity, int& iNumTradeRoutesInfluencing, bool bPretendTradeConnection) const
#else
int CvGameReligions::GetAdjacentCityReligiousPressure (ReligionTypes eReligion, CvCity *pFromCity, CvCity *pToCity, int& iNumTradeRoutesInfluencing, bool bPretendTradeConnection)
#endif
{
	int iPressure = 0;
	iNumTradeRoutesInfluencing = 0;

	if (eReligion <= RELIGION_PANTHEON)
	{
		return iPressure;
	}

	const CvReligion* pReligion = GetReligion(eReligion, pFromCity->getOwner());
	if (!pReligion)
	{
		return iPressure;
	}

	// Are the cities within the minimum distance?
	int iDistance = GC.getRELIGION_ADJACENT_CITY_DISTANCE();

	// Boost to distance due to belief?
	int iDistanceMod = pReligion->m_Beliefs.GetSpreadDistanceModifier();
	if(iDistanceMod > 0)
	{
		iDistance *= (100 + iDistanceMod);
		iDistance /= 100;
	}

	bool bWithinDistance = (plotDistance(pFromCity->getX(), pFromCity->getY(), pToCity->getX(), pToCity->getY()) <= iDistance);
	bool bConnectedWithTrade = GC.getGame().GetGameTrade()->IsCityConnectedToCity(pFromCity, pToCity) || bPretendTradeConnection;

	if(bWithinDistance || bConnectedWithTrade)
	{
		bool bIncrementTRInfluencing = false;
		iPressure = GC.getGame().getGameSpeedInfo().getReligiousPressureAdjacentCity();
		if (bConnectedWithTrade && !bWithinDistance)
		{
			if (!bIncrementTRInfluencing)
			{
				iNumTradeRoutesInfluencing++;
				bIncrementTRInfluencing = true;
			}
		}

		if (bConnectedWithTrade)
		{
			if (GC.getGame().GetGameTrade()->IsCityConnectedFromCityToCity(pFromCity, pToCity))
			{
				int iTradeReligionModifer = GET_PLAYER(pFromCity->getOwner()).GetPlayerTraits()->GetTradeReligionModifier();
				if (iTradeReligionModifer != 0)
				{
					iPressure *= 100 + iTradeReligionModifer;
					iPressure /= 100;

					if (!bIncrementTRInfluencing)
					{
						iNumTradeRoutesInfluencing++;
						bIncrementTRInfluencing = true;
					}
				}
			}
		}

		// If we are spreading to a friendly city state, increase the effectiveness if we have the right belief
		if(IsCityStateFriendOfReligionFounder(eReligion, pToCity->getOwner()))
		{
			int iFriendshipMod = pReligion->m_Beliefs.GetFriendlyCityStateSpreadModifier();
			if(iFriendshipMod > 0)
			{
				iPressure *= (100 + iFriendshipMod);
				iPressure /= 100;
			}
		}

		// Have a belief that always strengthens spread?
		int iStrengthMod = pReligion->m_Beliefs.GetSpreadStrengthModifier();
		if(iStrengthMod > 0)
		{
			TechTypes eDoublingTech = pReligion->m_Beliefs.GetSpreadModifierDoublingTech();
			if(eDoublingTech != NO_TECH)
			{
				CvPlayer& kPlayer = GET_PLAYER(pReligion->m_eFounder);
				if(GET_TEAM(kPlayer.getTeam()).GetTeamTechs()->HasTech(eDoublingTech))
				{
					iStrengthMod *= 2;
				}
			}
			iPressure *= (100 + iStrengthMod);
			iPressure /= 100;
		}

		// Strengthened spread from World Congress? (World Religion)
		int iLeaguesMod = GC.getGame().GetGameLeagues()->GetReligionSpreadStrengthModifier(pFromCity->getOwner(), eReligion);
		if (iLeaguesMod > 0)
		{
			iPressure *= (100 + iLeaguesMod);
			iPressure /= 100;
		}

		// Building that boosts pressure from originating city?
		int iModifier = pFromCity->GetCityReligions()->GetReligiousPressureModifier();
		if (iModifier > 0)
		{
			iPressure *= (100 + iModifier);
			iPressure /= 100;
		}
	}

	return iPressure;
}

/// How much does this prophet cost (recursive)
int CvGameReligions::GetFaithGreatProphetNumber(int iNum) const
{
	int iRtnValue = 0;

	if(iNum >= 1)
	{
		if(iNum == 1)
		{
			iRtnValue = GC.getRELIGION_MIN_FAITH_FIRST_PROPHET();
		}
		else
		{
			iRtnValue = (GC.getRELIGION_FAITH_DELTA_NEXT_PROPHET() * (iNum - 1)) + GetFaithGreatProphetNumber(iNum - 1);
		}
	}

	return iRtnValue;
}

/// How much does this great person cost (recursive)
int CvGameReligions::GetFaithGreatPersonNumber(int iNum) const
{
	int iRtnValue = 0;

	if(iNum >= 1)
	{
		if(iNum == 1)
		{
			iRtnValue = GC.getRELIGION_MIN_FAITH_FIRST_GREAT_PERSON();
		}
		else
		{
			iRtnValue = (GC.getRELIGION_FAITH_DELTA_NEXT_GREAT_PERSON() * (iNum - 1)) + GetFaithGreatPersonNumber(iNum - 1);
		}
	}

	return iRtnValue;
}

/// Does the religion in nearby city give this battle winner a yield? If so return multipler of losing unit strength
#ifdef AUI_CONSTIFY
int CvGameReligions::GetBeliefYieldForKill(YieldTypes eYield, int iX, int iY, PlayerTypes eWinningPlayer) const
#else
int CvGameReligions::GetBeliefYieldForKill(YieldTypes eYield, int iX, int iY, PlayerTypes eWinningPlayer)
#endif
{
	int iRtnValue = 0;
	int iMultiplier;
	int iLoop;
	CvCity* pLoopCity;

	// Only Faith supported for now
	if(eYield != YIELD_FAITH)
	{
		return iRtnValue;
	}

	for(pLoopCity = GET_PLAYER(eWinningPlayer).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(eWinningPlayer).nextCity(&iLoop))
	{
		// Find religion in this city
		ReligionTypes eReligion = pLoopCity->GetCityReligions()->GetReligiousMajority();

		if(eReligion != NO_RELIGION)
		{
			// Find distance to this city
			int iDistance = plotDistance(iX, iY, pLoopCity->getX(), pLoopCity->getY());

			// Do we have a yield from this?
			iMultiplier = GetReligion(eReligion, eWinningPlayer)->m_Beliefs.GetFaithFromKills(iDistance);
			if(iMultiplier > 0)
			{
				// Just looking for one city providing this
				iRtnValue = iMultiplier;
				break;
			}
			else
			{
				BeliefTypes eSecondaryPantheon = pLoopCity->GetCityReligions()->GetSecondaryReligionPantheonBelief();
				if (eSecondaryPantheon != NO_BELIEF)
				{
					iMultiplier = GC.GetGameBeliefs()->GetEntry(eSecondaryPantheon)->GetFaithFromKills();
					if(iMultiplier > 0 && iDistance <= GC.GetGameBeliefs()->GetEntry(eSecondaryPantheon)->GetMaxDistance())
					{
						// Just looking for one city providing this
						iRtnValue = iMultiplier;
						break;
					}	
				}
			}
		}
	}

	return iRtnValue;
}

/// Build log filename
CvString CvGameReligions::GetLogFileName() const
{
	CvString strLogName;
	strLogName = "ReligionLog.csv";
	return strLogName;
}

// PRIVATE METHODS

/// Has this religion already been founded?
#ifdef AUI_CONSTIFY
bool CvGameReligions::HasBeenFounded(ReligionTypes eReligion) const
#else
bool CvGameReligions::HasBeenFounded(ReligionTypes eReligion)
#endif
{
	ReligionList::const_iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); ++it)
#else
	for(it = m_CurrentReligions.begin(); it != m_CurrentReligions.end(); it++)
#endif
	{
		if(it->m_eReligion == eReligion)
		{
			return true;
		}
	}

	return false;
}

/// Does any civ in the game like this religion?
#ifdef AUI_CONSTIFY
bool CvGameReligions::IsPreferredByCivInGame(ReligionTypes eReligion) const
#else
bool CvGameReligions::IsPreferredByCivInGame(ReligionTypes eReligion)
#endif
{
	PlayerTypes eLoopPlayer;

	for(int iI = 0; iI < MAX_MAJOR_CIVS; iI++)
	{
		eLoopPlayer = (PlayerTypes) iI;
		CvPlayer& loopPlayer = GET_PLAYER(eLoopPlayer);

		if(!loopPlayer.isAlive())
		{
			continue;
		}

		ReligionTypes eCivReligion = loopPlayer.getCivilizationInfo().GetReligion();
		if(eCivReligion == eReligion)
		{
			return true;
		}
	}

	return false;
}

/// Time to spawn a Great Prophet?
bool CvGameReligions::CheckSpawnGreatProphet(CvPlayer& kPlayer)
{
	UnitTypes eUnit = (UnitTypes)GC.getInfoTypeForString("UNIT_PROPHET", true);
	if (eUnit == NO_UNIT)
	{
		return false;
	}

	const CvReligion* pReligion = NULL;
	const int iFaith = kPlayer.GetFaith();
	int iCost = kPlayer.GetReligions()->GetCostNextProphet(true /*bIncludeBeliefDiscounts*/, true /*bAdjustForSpeedDifficulty*/);

	ReligionTypes ePlayerReligion = GetReligionCreatedByPlayer(kPlayer.GetID());
	if(ePlayerReligion > RELIGION_PANTHEON)
	{
		pReligion = GetReligion(ePlayerReligion, kPlayer.GetID());
	}

	// If player hasn't founded a religion yet, drop out of this if all religions have been founded
	else if(GetNumReligionsStillToFound() <= 0)
	{
		return false;
	}

	if(iFaith < iCost)
	{
		return false;
	}

	int iChance = GC.getRELIGION_BASE_CHANCE_PROPHET_SPAWN();
	iChance += (iFaith - iCost);

	int iRand = GC.getGame().getJonRandNum(100, "Religion: spawn Great Prophet roll.");
	if(iRand >= iChance)
	{
		return false;
	}

	CvCity* pSpawnCity = NULL;
	if(pReligion)
	{
		pSpawnCity = GC.getMap().plot(pReligion->m_iHolyCityX, pReligion->m_iHolyCityY)->getPlotCity();
	}

	if(pSpawnCity != NULL && pSpawnCity->getOwner() == kPlayer.GetID())
	{
		pSpawnCity->GetCityCitizens()->DoSpawnGreatPerson(eUnit, false /*bIncrementCount*/, true);
		kPlayer.SetFaith(0);
	}
	else
	{
		pSpawnCity = kPlayer.getCapitalCity();
		if(pSpawnCity != NULL)
		{
			pSpawnCity->GetCityCitizens()->DoSpawnGreatPerson(eUnit, false /*bIncrementCount*/, true);
			kPlayer.SetFaith(0);
		}
	}

	// Logging
	if(GC.getLogging() && pSpawnCity)
	{
		CvString strLogMsg;
		strLogMsg = kPlayer.getCivilizationShortDescription();
		strLogMsg += ", PROPHET SPAWNED, ";
		strLogMsg += pSpawnCity->getName();
		strLogMsg += ", Faith: 0";
		LogReligionMessage(strLogMsg);
	}

	return true;
}

/// Log a message with status information
void CvGameReligions::LogReligionMessage(CvString& strMsg)
{
	if(GC.getLogging() && GC.getAILogging())
	{
		CvString strOutBuf;
		CvString strBaseString;
		FILogFile* pLog;

		pLog = LOGFILEMGR.GetLog(GetLogFileName(), FILogFile::kDontTimeStamp);

		// Get the leading info for this line
		strBaseString.Format("%03d, %d, ", GC.getGame().getElapsedGameTurns(), GC.getGame().getGameTurnYear());
		strOutBuf = strBaseString + strMsg;
		pLog->Msg(strOutBuf);
	}
}

// Notify the supplied player (if they are the local player) of an error when founding/modifying a religion/pantheon
void CvGameReligions::NotifyPlayer(PlayerTypes ePlayer, CvGameReligions::FOUNDING_RESULT eResult)
{
	CvString strMessage;
	CvString strSummary;

	NotificationTypes eNotificationType = NOTIFICATION_RELIGION_ERROR;

	switch(eResult)
	{
	case FOUNDING_OK:
		break;
	case FOUNDING_BELIEF_IN_USE:
		strMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_PANTHEON_BELIEF_IN_USE");
		strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_PANTHEON_BELIEF_IN_USE");
		break;
	case FOUNDING_RELIGION_IN_USE:
		strMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_RELIGION_IN_USE");
		strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_RELIGION_IN_USE");
		break;
	case FOUNDING_NOT_ENOUGH_FAITH:
		strMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_NOT_ENOUGH_FAITH_FOR_PANTHEON");
		strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_NOT_ENOUGH_FAITH_FOR_PANTHEON");
		break;
	case FOUNDING_NO_RELIGIONS_AVAILABLE:
		strMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_NO_RELIGIONS_AVAILABLE");
		strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_NO_RELIGIONS_AVAILABLE");
		break;
	case FOUNDING_INVALID_PLAYER:
		break;
	case FOUNDING_PLAYER_ALREADY_CREATED_RELIGION:
		strMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_ALREADY_CREATED_RELIGION");
		strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_ALREADY_CREATED_RELIGION");
		break;
	case FOUNDING_PLAYER_ALREADY_CREATED_PANTHEON:
		strMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_ALREADY_CREATED_PANTHEON");
		strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_ALREADY_CREATED_PANTHEON");
		break;
	case FOUNDING_NAME_IN_USE:
		strMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_RELIGION_NAME_IN_USE");
		strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_RELIGION_NAME_IN_USE");
		break;
	case FOUNDING_RELIGION_ENHANCED:
		strMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_ENHANCED_RELIGION_IN_USE");
		strSummary = GetLocalizedText("TXT_KEY_NOTIFICATION_SUMMARY_ENHANCED_RELIGION_IN_USE");
		break;
	}

	if(strMessage.GetLength() != 0)
		CvNotifications::AddToPlayer(ePlayer, eNotificationType, strMessage.c_str(), strSummary.c_str());
}

// SERIALIZATION

/// Serialization read
FDataStream& operator>>(FDataStream& loadFrom, CvGameReligions& writeTo)
{
	uint uiVersion;
	int iMinFaith;

	loadFrom >> uiVersion;

	if(uiVersion >= 3)
	{
		loadFrom >> iMinFaith;
		writeTo.SetMinimumFaithNextPantheon(iMinFaith);
	}
	else
	{
		writeTo.SetMinimumFaithNextPantheon(0);
	}

	if(uiVersion < 4)
	{
		loadFrom >> iMinFaith;
		//	writeTo.SetMinimumFaithNextGreatProphet(iMinFaith);  -- eliminated in Version 4
	}

	if(uiVersion >= 2)
	{
		int iEntriesToRead;
		CvReligion tempItem;

		writeTo.m_CurrentReligions.clear();
		loadFrom >> iEntriesToRead;
		for(int iI = 0; iI < iEntriesToRead; iI++)
		{
			loadFrom >> tempItem;
			writeTo.m_CurrentReligions.push_back(tempItem);
		}
	}

	return loadFrom;
}

/// Serialization write
FDataStream& operator<<(FDataStream& saveTo, const CvGameReligions& readFrom)
{
	uint uiVersion = 4;
	saveTo << uiVersion;

	saveTo << readFrom.GetMinimumFaithNextPantheon();

	ReligionList::const_iterator it;
	saveTo << readFrom.m_CurrentReligions.size();
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = readFrom.m_CurrentReligions.begin(); it != readFrom.m_CurrentReligions.end(); ++it)
#else
	for(it = readFrom.m_CurrentReligions.begin(); it != readFrom.m_CurrentReligions.end(); it++)
#endif
	{
		saveTo << *it;
	}

	return saveTo;
}

//=====================================
// CvPlayerReligions
//=====================================
/// Constructor
CvPlayerReligions::CvPlayerReligions(void):
	m_pPlayer(NULL),
	m_iNumProphetsSpawned(0),
	m_bFoundingReligion(false)
{
}

/// Destructor
CvPlayerReligions::~CvPlayerReligions(void)
{
	Uninit();
}

/// Initialize class data
void CvPlayerReligions::Init(CvPlayer* pPlayer)
{
	m_pPlayer = pPlayer;

	Reset();
}

/// Cleanup
void CvPlayerReligions::Uninit()
{

}

/// Reset
void CvPlayerReligions::Reset()
{
	m_bFoundingReligion = false;
	m_iNumProphetsSpawned = 0;
}

/// Serialization read
void CvPlayerReligions::Read(FDataStream& kStream)
{
	// Version number to maintain backwards compatibility
	uint uiVersion;
	kStream >> uiVersion;
	kStream >> m_iNumProphetsSpawned;
	kStream >> m_bFoundingReligion;
}

/// Serialization write
void CvPlayerReligions::Write(FDataStream& kStream)
{
	// Current version number
	uint uiVersion = 1;
	kStream << uiVersion;
	kStream << m_iNumProphetsSpawned;
	kStream << m_bFoundingReligion;
}

/// How many prophets have we spawned
int CvPlayerReligions::GetNumProphetsSpawned() const
{
	return m_iNumProphetsSpawned;
}

/// Change count of prophets spawned
void CvPlayerReligions::ChangeNumProphetsSpawned(int iValue)
{
	m_iNumProphetsSpawned += iValue;
}

/// How much will the next prophet cost this player?
int CvPlayerReligions::GetCostNextProphet(bool bIncludeBeliefDiscounts, bool bAdjustForSpeedDifficulty) const
{
	int iCost = GC.getGame().GetGameReligions()->GetFaithGreatProphetNumber(m_iNumProphetsSpawned + 1);

	// Boost to faith due to belief?
	ReligionTypes ePlayerReligion = GetReligionCreatedByPlayer();
	if (bIncludeBeliefDiscounts && ePlayerReligion > RELIGION_PANTHEON)
	{
		const CvReligion* pReligion = NULL;
		pReligion = GC.getGame().GetGameReligions()->GetReligion(ePlayerReligion, m_pPlayer->GetID());
		if(pReligion)
		{
			int iProphetCostMod = pReligion->m_Beliefs.GetProphetCostModifier();

			if(iProphetCostMod != 0)
			{
				iCost *= (100 + iProphetCostMod);
				iCost /= 100;
			}
		}
	}

	if (bAdjustForSpeedDifficulty)
	{
		// Adjust for game speed
		iCost *= GC.getGame().getGameSpeedInfo().getTrainPercent();
		iCost /= 100;

		// Adjust for difficulty
		if(!m_pPlayer->isHuman() && !m_pPlayer->IsAITeammateOfHuman() && !m_pPlayer->isBarbarian())
		{
			iCost *= GC.getGame().getHandicapInfo().getAITrainPercent();
			iCost /= 100;
		}
	}

	return iCost;
}

/// Has this player created a pantheon?
bool CvPlayerReligions::HasCreatedPantheon() const
{
	return GC.getGame().GetGameReligions()->HasCreatedPantheon(m_pPlayer->GetID());
}

/// Has this player created a religion?
bool CvPlayerReligions::HasCreatedReligion() const
{
	return GC.getGame().GetGameReligions()->HasCreatedReligion(m_pPlayer->GetID());
}

/// Has this player reformed his religion
bool CvPlayerReligions::HasAddedReformationBelief() const
{
	return GC.getGame().GetGameReligions()->HasAddedReformationBelief(m_pPlayer->GetID());
}

/// Get the religion this player created
ReligionTypes CvPlayerReligions::GetReligionCreatedByPlayer() const
{
	return GC.getGame().GetGameReligions()->GetReligionCreatedByPlayer(m_pPlayer->GetID());
}

/// Does this player have enough faith to buy a religious unit or building?
bool CvPlayerReligions::CanAffordFaithPurchase() const
{
	int iFaith = m_pPlayer->GetFaith();
#ifdef AUI_RELIGION_FIX_CAN_AFFORD_FAITH_PURCHASE_NON_CAPITAL_RELIGION
	int iCost;
	int iLoop = 0;
	CvCity* pLoopCity;
#endif
	CvCity* pCapital = m_pPlayer->getCapitalCity();
	if(pCapital)
	{
#ifdef AUI_WARNING_FIXES
		for (uint iI = 0; iI < GC.getNumUnitInfos(); iI++)
#else
		for (int iI = 0; iI < GC.getNumUnitInfos(); iI++)
#endif
		{
			const UnitTypes eUnit = static_cast<UnitTypes>(iI);
			CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnit);
			if(pkUnitInfo)
			{
				if (m_pPlayer->IsCanPurchaseAnyCity(false, false, eUnit, NO_BUILDING, YIELD_FAITH))
				{
#ifdef AUI_RELIGION_FIX_CAN_AFFORD_FAITH_PURCHASE_NON_CAPITAL_RELIGION
					for (pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
					{
						iCost = pLoopCity->GetFaithPurchaseCost(eUnit, true);
						if (iCost != 0 && iFaith > iCost)
						{
							return true;
						}
					}
#else
					int iCost = pCapital->GetFaithPurchaseCost(eUnit, true);
					if(iCost != 0 && iFaith > iCost)
					{
						return true;
					}
#endif
				}
			}
		}
#ifdef AUI_WARNING_FIXES
		for (uint iI = 0; iI < GC.getNumBuildingInfos(); iI++)
#else
		for (int iI = 0; iI < GC.getNumBuildingInfos(); iI++)
#endif
		{
			const BuildingTypes eBuilding = static_cast<BuildingTypes>(iI);
			CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
			if(pkBuildingInfo)
			{
				if (m_pPlayer->IsCanPurchaseAnyCity(false, false, NO_UNIT, eBuilding, YIELD_FAITH))
				{
#ifdef AUI_RELIGION_FIX_CAN_AFFORD_FAITH_PURCHASE_NON_CAPITAL_RELIGION
					iCost = pCapital->GetFaithPurchaseCost(eBuilding);
#else
					int iCost = pCapital->GetFaithPurchaseCost(eBuilding);
#endif
					if(iCost != 0 && iFaith > iCost)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

/// Does this player have a city following a religion?
bool CvPlayerReligions::HasReligiousCity() const
{
	int iLoop = 0;
	CvCity* pCity = NULL;

	for(pCity = m_pPlayer->firstCity(&iLoop); pCity != NULL; pCity = m_pPlayer->nextCity(&iLoop))
	{
		if(pCity)
		{
			if(pCity->GetCityReligions()->GetReligiousMajority() > RELIGION_PANTHEON)
			{
				return true;
			}
		}
	}

	return false;
}

/// Is this player happily following this other player's religion?
bool CvPlayerReligions::HasOthersReligionInMostCities(PlayerTypes eOtherPlayer) const
{
	// Not happy about it if have their own religion
	if(HasCreatedReligion())
	{
		return false;
	}

	ReligionTypes eOtherReligion = GC.getGame().GetGameReligions()->GetReligionCreatedByPlayer(eOtherPlayer);

	if(eOtherReligion <= RELIGION_PANTHEON)
	{
		return false;
	}

	int iNumFollowingCities = 0;
	int iLoop = 0;
	CvCity* pCity = NULL;
	for(pCity = m_pPlayer->firstCity(&iLoop); pCity != NULL; pCity = m_pPlayer->nextCity(&iLoop))
	{
		if(pCity)
		{
			if(pCity->GetCityReligions()->GetReligiousMajority() == eOtherReligion)
			{
				iNumFollowingCities++;
			}
		}
	}

	// Over half?
	return (iNumFollowingCities * 2 > m_pPlayer->getNumCities());
}

/// Do a majority of this player's cities follow a specific religion?
bool CvPlayerReligions::HasReligionInMostCities(ReligionTypes eReligion) const
{
	if (eReligion <= RELIGION_PANTHEON)
	{
		return false;
	}

	int iNumFollowingCities = 0;
	int iLoop = 0;
	CvCity* pCity = NULL;
	for(pCity = m_pPlayer->firstCity(&iLoop); pCity != NULL; pCity = m_pPlayer->nextCity(&iLoop))
	{
		if(pCity)
		{
			if(pCity->GetCityReligions()->GetReligiousMajority() == eReligion)
			{
				iNumFollowingCities++;
			}
		}
	}

	// Over half?
	return (iNumFollowingCities * 2 > m_pPlayer->getNumCities());
}

/// What religion is followed in a majority of our cities?
ReligionTypes CvPlayerReligions::GetReligionInMostCities() const
{
	for (int iI = RELIGION_PANTHEON + 1; iI < GC.GetGameReligions()->GetNumReligions(); iI++)
	{
		ReligionTypes eReligion = (ReligionTypes)iI;
		if (HasReligionInMostCities(eReligion))
		{
			return eReligion;
		}
	}
	return NO_RELIGION;
}

/// Does this player get a default influence boost with city states following this religion?
int CvPlayerReligions::GetCityStateMinimumInfluence(ReligionTypes eReligion) const
{
	int iMinInfluence = 0;

	ReligionTypes eFounderBenefitReligion = GC.getGame().GetGameReligions()->GetFounderBenefitsReligion(m_pPlayer->GetID());
	if (eReligion == eFounderBenefitReligion)
	{
		CvGameReligions* pReligions = GC.getGame().GetGameReligions();
		if(eFounderBenefitReligion != NO_RELIGION)
		{
			const CvReligion* pReligion = pReligions->GetReligion(eFounderBenefitReligion, NO_PLAYER);
			if(pReligion)
			{
				iMinInfluence += pReligion->m_Beliefs.GetCityStateMinimumInfluence();
			}
		}
	}

	return iMinInfluence;
}

/// Does this player get a modifier to city state influence boosts?
int CvPlayerReligions::GetCityStateInfluenceModifier() const
{
	int iRtnValue = 0;
	ReligionTypes eReligion = GetReligionCreatedByPlayer();
	if (eReligion != NO_RELIGION)
	{
		const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eReligion, NO_PLAYER);
		if(pReligion)
		{
			iRtnValue += pReligion->m_Beliefs.GetCityStateInfluenceModifier();
		}
	}
	return iRtnValue;
}

/// Does this player get religious pressure from spies?
int CvPlayerReligions::GetSpyPressure() const
{
	int iRtnValue = 0;
	ReligionTypes eReligion = GetReligionCreatedByPlayer();
	if (eReligion != NO_RELIGION)
	{
		const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eReligion, NO_PLAYER);
		if(pReligion)
		{
			iRtnValue += pReligion->m_Beliefs.GetSpyPressure();
		}
	}
	return iRtnValue;
}

/// How many foreign cities are following a religion we founded?
int CvPlayerReligions::GetNumForeignCitiesFollowing() const
{
	CvCity *pLoopCity;
	int iCityLoop;
	int iRtnValue = 0;

	ReligionTypes eFounderBenefitReligion = GC.getGame().GetGameReligions()->GetFounderBenefitsReligion(m_pPlayer->GetID());
	if (eFounderBenefitReligion > RELIGION_PANTHEON)
	{
		for(int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
		{
			CvPlayer &kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayerLoop);
			if(kLoopPlayer.isAlive() && iPlayerLoop != m_pPlayer->GetID())
			{
				for(pLoopCity = GET_PLAYER((PlayerTypes)iPlayerLoop).firstCity(&iCityLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iPlayerLoop).nextCity(&iCityLoop))
				{
					if (pLoopCity->GetCityReligions()->GetReligiousMajority() == eFounderBenefitReligion)
					{
						iRtnValue++;
					}
				}
			}
		}
	}

	return iRtnValue;
}

/// How many foreign citizens are following a religion we founded?
int CvPlayerReligions::GetNumForeignFollowers(bool bAtPeace) const
{
	CvCity *pLoopCity;
	int iCityLoop;
	int iRtnValue = 0;

	ReligionTypes eFounderBenefitReligion = GC.getGame().GetGameReligions()->GetFounderBenefitsReligion(m_pPlayer->GetID());
	if (eFounderBenefitReligion > RELIGION_PANTHEON)
	{
		for(int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
		{
			CvPlayer &kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayerLoop);
			if(kLoopPlayer.isAlive() && iPlayerLoop != m_pPlayer->GetID())
			{
				if (!bAtPeace || !atWar(m_pPlayer->getTeam(), kLoopPlayer.getTeam()))
				{
					for(pLoopCity = GET_PLAYER((PlayerTypes)iPlayerLoop).firstCity(&iCityLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iPlayerLoop).nextCity(&iCityLoop))
					{
						iRtnValue += pLoopCity->GetCityReligions()->GetNumFollowers(eFounderBenefitReligion);
					}
				}
			}
		}
	}

	return iRtnValue;
}

//=====================================
// CvCityReligions
//=====================================
/// Constructor
CvCityReligions::CvCityReligions(void):
	m_bHasPaidAdoptionBonus(false),
	m_iReligiousPressureModifier(0)
{
	m_ReligionStatus.clear();
}

/// Destructor
CvCityReligions::~CvCityReligions(void)
{

}

/// Initialize class data
void CvCityReligions::Init(CvCity* pCity)
{
	m_pCity = pCity;
	m_bHasPaidAdoptionBonus = false;
	m_iReligiousPressureModifier = 0;
	m_ReligionStatus.clear();
}

/// Cleanup
void CvCityReligions::Uninit()
{

}

/// Copy data from old city into new (for conquest)
void CvCityReligions::Copy(CvCityReligions* pOldCity)
{
	m_ReligionStatus.clear();

	SetPaidAdoptionBonus(pOldCity->HasPaidAdoptionBonus());
	SetReligiousPressureModifier(pOldCity->GetReligiousPressureModifier());

	ReligionInCityList::iterator religionIt;
	for(religionIt = pOldCity->m_ReligionStatus.begin(); religionIt != pOldCity->m_ReligionStatus.end(); ++religionIt)
	{
		m_ReligionStatus.push_back(*religionIt);
	}
}

/// How many citizens here are following this religion?
#ifdef AUI_CONSTIFY
int CvCityReligions::GetNumFollowers(ReligionTypes eReligion) const
{
	ReligionInCityList::const_iterator religionIt;
#else
int CvCityReligions::GetNumFollowers(ReligionTypes eReligion)
{
	ReligionInCityList::iterator religionIt;
#endif

	// Find the religion in the list
	for(religionIt = m_ReligionStatus.begin(); religionIt != m_ReligionStatus.end(); ++religionIt)
	{
		if(religionIt->m_eReligion == eReligion)
		{
			return religionIt->m_iFollowers;
		}
	}

	return 0;
}

/// Number of followers of this religion
#ifdef AUI_CONSTIFY
int CvCityReligions::GetNumSimulatedFollowers(ReligionTypes eReligion) const
{
	ReligionInCityList::const_iterator religionIt;
#else
int CvCityReligions::GetNumSimulatedFollowers(ReligionTypes eReligion)
{
	ReligionInCityList::iterator religionIt;
#endif

	// Find the religion in the list
	for(religionIt = m_SimulatedStatus.begin(); religionIt != m_SimulatedStatus.end(); ++religionIt)
	{
		if(religionIt->m_eReligion == eReligion)
		{
			return religionIt->m_iFollowers;
		}
	}

	return 0;
}

/// How many religions have at least 1 follower?
#ifdef AUI_CONSTIFY
int CvCityReligions::GetNumReligionsWithFollowers() const
#else
int CvCityReligions::GetNumReligionsWithFollowers()
#endif
{
	int iRtnValue = 0;
#ifdef AUI_CONSTIFY
	ReligionInCityList::const_iterator religionIt;
#else
	ReligionInCityList::iterator religionIt;
#endif

	// Find the religion in the list
	for(religionIt = m_ReligionStatus.begin(); religionIt != m_ReligionStatus.end(); ++religionIt)
	{
		if(religionIt->m_iFollowers > 0 && religionIt->m_eReligion > RELIGION_PANTHEON)
		{
			iRtnValue++;
		}
	}

	return iRtnValue;
}

///Any religion in this city?
#ifdef AUI_CONSTIFY
bool CvCityReligions::IsReligionInCity() const
{
	ReligionInCityList::const_iterator religionIt;
#else
bool CvCityReligions::IsReligionInCity()
{
	ReligionInCityList::iterator religionIt;
#endif

	for(religionIt = m_ReligionStatus.begin(); religionIt != m_ReligionStatus.end(); ++religionIt)
	{
		if(religionIt->m_eReligion != NO_RELIGION)
		{
			return true;
		}
	}

	return false;
}

/// Is this the holy city for a specific religion?
#ifdef AUI_CONSTIFY
bool CvCityReligions::IsHolyCityForReligion(ReligionTypes eReligion) const
{
	ReligionInCityList::const_iterator religionIt;
#else
bool CvCityReligions::IsHolyCityForReligion(ReligionTypes eReligion)
{
	ReligionInCityList::iterator religionIt;
#endif

	// Find the religion in the list
	for(religionIt = m_ReligionStatus.begin(); religionIt != m_ReligionStatus.end(); ++religionIt)
	{
		if(religionIt->m_eReligion == eReligion)
		{
			return religionIt->m_bFoundedHere;
		}
	}

	return false;
}

/// Is this the holy city for any religion?
#ifdef AUI_CONSTIFY
bool CvCityReligions::IsHolyCityAnyReligion() const
{
	ReligionInCityList::const_iterator religionIt;
#else
bool CvCityReligions::IsHolyCityAnyReligion()
{
	ReligionInCityList::iterator religionIt;
#endif
	for(religionIt = m_ReligionStatus.begin(); religionIt != m_ReligionStatus.end(); ++religionIt)
	{
		if(religionIt->m_bFoundedHere)
		{
			return true;
		}
	}

	return false;
}

/// Is there a "heretical" religion here that can be stomped out?
#ifdef AUI_CONSTIFY
bool CvCityReligions::IsReligionHereOtherThan(ReligionTypes eReligion) const
{
	ReligionInCityList::const_iterator it;
#else
bool CvCityReligions::IsReligionHereOtherThan(ReligionTypes eReligion)
{
	ReligionInCityList::iterator it;
#endif
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); ++it)
#else
	for(it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); it++)
#endif
	{
		if(it->m_eReligion != NO_RELIGION && it->m_eReligion != eReligion)
		{
			return true;
		}
	}
	return false;
}

/// Is there an inquisitor from another religion here?
#ifdef AUI_CONSTIFY
bool CvCityReligions::IsDefendedAgainstSpread(ReligionTypes eReligion) const
#else
bool CvCityReligions::IsDefendedAgainstSpread(ReligionTypes eReligion)
#endif
{
	CvUnit* pLoopUnit;

	CvPlot* pCityPlot = m_pCity->plot();
	if(pCityPlot)
	{
#ifdef AUI_WARNING_FIXES
		for (uint iUnitLoop = 0; iUnitLoop < pCityPlot->getNumUnits(); iUnitLoop++)
#else
		for(int iUnitLoop = 0; iUnitLoop < pCityPlot->getNumUnits(); iUnitLoop++)
#endif
		{
			pLoopUnit = pCityPlot->getUnitByIndex(iUnitLoop);
			CvUnitEntry* pkEntry = GC.getUnitInfo(pLoopUnit->getUnitType());
			if(pkEntry && pkEntry->IsProhibitsSpread())
			{
				if(pLoopUnit->getOwner() == m_pCity->getOwner() && pLoopUnit->GetReligionData()->GetReligion() != eReligion)
				{
					return true;
				}
			}
		}
	}

	CvPlot* pAdjacentPlot;
	for(int iDirectionLoop = 0; iDirectionLoop < NUM_DIRECTION_TYPES; iDirectionLoop++)
	{
		pAdjacentPlot = plotDirection(m_pCity->getX(), m_pCity->getY(), ((DirectionTypes)iDirectionLoop));

		if(pAdjacentPlot != NULL)
		{
#ifdef AUI_WARNING_FIXES
			for (uint iUnitLoop = 0; iUnitLoop < pAdjacentPlot->getNumUnits(); iUnitLoop++)
#else
			for(int iUnitLoop = 0; iUnitLoop < pAdjacentPlot->getNumUnits(); iUnitLoop++)
#endif
			{
				pLoopUnit = pAdjacentPlot->getUnitByIndex(iUnitLoop);
#ifdef AUI_CONSTIFY
				if(pLoopUnit->getUnitInfo().IsProhibitsSpread())
#else
				CvUnitEntry* pkEntry = GC.getUnitInfo(pLoopUnit->getUnitType());
				if(pkEntry && pkEntry->IsProhibitsSpread())
#endif
				{
					if(pLoopUnit->getOwner() == m_pCity->getOwner() && pLoopUnit->GetReligionData()->GetReligion() != eReligion)
					{
						return true;
					}
				}
			}
		}
	}

	return false;
}

/// Is there a religion that at least half of the population follows?
#ifdef AUI_CONSTIFY
ReligionTypes CvCityReligions::GetReligiousMajority() const
#else
ReligionTypes CvCityReligions::GetReligiousMajority()
#endif
{
	int iTotalFollowers = 0;
	int iMostFollowerPressure = 0;
	int iMostFollowers = -1;
	ReligionTypes eMostFollowers = NO_RELIGION;
#ifdef AUI_CONSTIFY
	ReligionInCityList::const_iterator religionIt;
#else
	ReligionInCityList::iterator religionIt;
#endif

	for(religionIt = m_ReligionStatus.begin(); religionIt != m_ReligionStatus.end(); ++religionIt)
	{
		iTotalFollowers += religionIt->m_iFollowers;

		if(religionIt->m_iFollowers > iMostFollowers || religionIt->m_iFollowers == iMostFollowers && religionIt->m_iPressure > iMostFollowerPressure)
		{
			iMostFollowers = religionIt->m_iFollowers;
			iMostFollowerPressure = religionIt->m_iPressure;
			eMostFollowers = religionIt->m_eReligion;
		}
	}

	if ((iMostFollowers * 2) >= iTotalFollowers)
	{
		return eMostFollowers;
	}
	else
	{
		return NO_RELIGION;
	}
}

/// Just asked to simulate a conversion - who would be the majority religion?
#ifdef AUI_CONSTIFY
ReligionTypes CvCityReligions::GetSimulatedReligiousMajority() const
#else
ReligionTypes CvCityReligions::GetSimulatedReligiousMajority()
#endif
{
	int iTotalFollowers = 0;
	int iMostFollowerPressure = 0;
	int iMostFollowers = -1;
	ReligionTypes eMostFollowers = NO_RELIGION;
#ifdef AUI_CONSTIFY
	ReligionInCityList::const_iterator religionIt;
#else
	ReligionInCityList::iterator religionIt;
#endif

	for(religionIt = m_SimulatedStatus.begin(); religionIt != m_SimulatedStatus.end(); ++religionIt)
	{
		iTotalFollowers += religionIt->m_iFollowers;

		if(religionIt->m_iFollowers > iMostFollowers || religionIt->m_iFollowers == iMostFollowers && religionIt->m_iPressure > iMostFollowerPressure)
		{
			iMostFollowers = religionIt->m_iFollowers;
			iMostFollowerPressure = religionIt->m_iPressure;
			eMostFollowers = religionIt->m_eReligion;
		}
	}

	if ((iMostFollowers * 2) >= iTotalFollowers)
	{
		return eMostFollowers;
	}
	else
	{
		return NO_RELIGION;
	}
}

/// What is the second most popular religion in this city with a majority religion?
#ifdef AUI_CONSTIFY
ReligionTypes CvCityReligions::GetSecondaryReligion() const
#else
ReligionTypes CvCityReligions::GetSecondaryReligion()
#endif
{
	int iMostFollowers = -1;
	int iMostPressure = -1;
	ReligionTypes eMajority = GetReligiousMajority();
	ReligionTypes eMostFollowers = NO_RELIGION;
#ifdef AUI_CONSTIFY
	ReligionInCityList::const_iterator religionIt;
#else
	ReligionInCityList::iterator religionIt;
#endif

	if (eMajority != NO_RELIGION)
	{	
		for(religionIt = m_ReligionStatus.begin(); religionIt != m_ReligionStatus.end(); ++religionIt)
		{
			if (religionIt->m_eReligion != eMajority && religionIt->m_eReligion > RELIGION_PANTHEON && religionIt->m_iFollowers > 0)
			{
				if (religionIt->m_iFollowers > iMostFollowers)
				{
					iMostFollowers = religionIt->m_iFollowers;
					eMostFollowers = religionIt->m_eReligion;
					iMostPressure = religionIt->m_iPressure;
				}
				else if (religionIt->m_iFollowers == iMostFollowers && religionIt->m_iPressure > iMostPressure)
				{
					iMostFollowers = religionIt->m_iFollowers;
					eMostFollowers = religionIt->m_eReligion;
					iMostPressure = religionIt->m_iPressure;
				}
			}
		}
	}

	return eMostFollowers;
}

/// Is there a pantheon belief in the secondary religion here?
#ifdef AUI_CONSTIFY
BeliefTypes CvCityReligions::GetSecondaryReligionPantheonBelief() const
#else
BeliefTypes CvCityReligions::GetSecondaryReligionPantheonBelief()
#endif
{
	BeliefTypes eRtnValue = NO_BELIEF;

	// Check for the policy that allows a secondary religion to be active
	if (GET_PLAYER(m_pCity->getOwner()).IsSecondReligionPantheon())
	{
		ReligionTypes eSecondary = GetSecondaryReligion();
		if (eSecondary != NO_RELIGION)
		{
			const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eSecondary, m_pCity->getOwner());
			if(pReligion)
			{
				for(int iI = 0; iI < pReligion->m_Beliefs.GetNumBeliefs(); iI++)
				{
					const BeliefTypes eBelief = pReligion->m_Beliefs.GetBelief(iI);
					CvBeliefEntry* pEntry = GC.GetGameBeliefs()->GetEntry((int)eBelief);
					if(pEntry && pEntry->IsPantheonBelief())
					{
						eRtnValue = eBelief;
						break;
					}
				}
			}
		}
	}

	return eRtnValue;
}

/// How many followers are there of religions OTHER than this one?
#ifdef AUI_CONSTIFY
int CvCityReligions::GetFollowersOtherReligions(ReligionTypes eReligion) const
#else
int CvCityReligions::GetFollowersOtherReligions(ReligionTypes eReligion)
#endif
{
	int iOtherFollowers = 0;
#ifdef AUI_CONSTIFY
	ReligionInCityList::const_iterator religionIt;
#else
	ReligionInCityList::iterator religionIt;
#endif

	for(religionIt = m_ReligionStatus.begin(); religionIt != m_ReligionStatus.end(); ++religionIt)
	{
		if (religionIt->m_eReligion > RELIGION_PANTHEON && religionIt->m_eReligion != eReligion)
		{
			iOtherFollowers += religionIt->m_iFollowers;
		}
	}

	return iOtherFollowers;
}

/// Total pressure exerted by all religions
#ifdef AUI_CONSTIFY
int CvCityReligions::GetTotalPressure() const
#else
int CvCityReligions::GetTotalPressure()
#endif
{
	int iTotalPressure = 0;

#ifdef AUI_CONSTIFY
	ReligionInCityList::const_iterator it;
#else
	ReligionInCityList::iterator it;
#endif
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); ++it)
#else
	for(it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); it++)
#endif
	{
		iTotalPressure += it->m_iPressure;
	}

	return iTotalPressure;
}

/// Pressure exerted by one religion
#ifdef AUI_CONSTIFY
int CvCityReligions::GetPressure(ReligionTypes eReligion) const
{
	ReligionInCityList::const_iterator it;
#else
int CvCityReligions::GetPressure(ReligionTypes eReligion)
{
	ReligionInCityList::iterator it;
#endif
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); ++it)
#else
	for(it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); it++)
#endif
	{
		if(it->m_eReligion == eReligion)
		{
			return it->m_iPressure;
		}
	}

	return 0;
}

/// Pressure exerted by one religion per turn
#ifdef AUI_CONSTIFY
int CvCityReligions::GetPressurePerTurn(ReligionTypes eReligion, int& iNumTradeRoutesInvolved) const
#else
int CvCityReligions::GetPressurePerTurn(ReligionTypes eReligion, int& iNumTradeRoutesInvolved)
#endif
{
	int iPressure = 0;
	iNumTradeRoutesInvolved = 0;

	// Loop through all the players
	for(int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iI);
		if(kPlayer.isAlive())
		{
			// Loop through each of their cities
			int iLoop;
			CvCity* pLoopCity;
			for(pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
			{
				// Ignore the same city
				if (m_pCity == pLoopCity)
				{
					continue;
				}

				// Does this city have the matching majority religion?
				ReligionTypes eMajorityReligion = pLoopCity->GetCityReligions()->GetReligiousMajority();
				if(eMajorityReligion == eReligion)
				{
					int iNumTradeRoutes;
					iPressure += GC.getGame().GetGameReligions()->GetAdjacentCityReligiousPressure (eMajorityReligion, pLoopCity, m_pCity, iNumTradeRoutes, false);
					iNumTradeRoutesInvolved += iNumTradeRoutes;
				}
			}
		}
	}

	// Holy city for this religion?
	if (IsHolyCityForReligion(eReligion))
	{
		int iHolyCityPressure = GC.getGame().getGameSpeedInfo().getReligiousPressureAdjacentCity();
		iHolyCityPressure *=  GC.getRELIGION_PER_TURN_FOUNDING_CITY_PRESSURE();
		iPressure += iHolyCityPressure;
	}

	return iPressure;
}

/// How many trade routes are applying pressure to this city
#ifdef AUI_CONSTIFY
int CvCityReligions::GetNumTradeRouteConnections(ReligionTypes eReligion) const
{
	ReligionInCityList::const_iterator it;
#else
int CvCityReligions::GetNumTradeRouteConnections (ReligionTypes eReligion)
{
	ReligionInCityList::iterator it;
#endif
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); ++it)
#else
	for(it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); it++)
#endif
	{
		if(it->m_eReligion == eReligion)
		{
			return it->m_iNumTradeRoutesApplyingPressure;
		}
	}

	return 0;
}

/// Would this city exert religious pressure toward the target city if connected with a trade route
#ifdef AUI_CONSTIFY
bool CvCityReligions::WouldExertTradeRoutePressureToward(const CvCity* pTargetCity, ReligionTypes& eReligion, int& iAmount) const
#else
bool CvCityReligions::WouldExertTradeRoutePressureToward (CvCity* pTargetCity, ReligionTypes& eReligion, int& iAmount)
#endif
{
	eReligion = NO_RELIGION;
	iAmount = 0;

	ReligionTypes eReligiousMajority = GetReligiousMajority();

	// if there isn't a religious connection, whatvz
	if (eReligiousMajority == NO_RELIGION)
	{
		return false;
	}

	const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eReligiousMajority, m_pCity->getOwner());
	if (!pReligion)
	{
		return false;
	}

	// Are the cities within the minimum distance?
	int iDistance = GC.getRELIGION_ADJACENT_CITY_DISTANCE();

	// Boost to distance due to belief?
	int iDistanceMod = pReligion->m_Beliefs.GetSpreadDistanceModifier();
	if(iDistanceMod > 0)
	{
		iDistance *= (100 + iDistanceMod);
		iDistance /= 100;
	}

	bool bWithinDistance = (plotDistance(m_pCity->getX(), m_pCity->getY(), pTargetCity->getX(), pTargetCity->getY()) <= iDistance);

	// if not within distance, then we're using a trade route
	if (!bWithinDistance) 
	{
		eReligion = eReligiousMajority;
		int iNumTradeRoutes = 0;
		iAmount = GC.getGame().GetGameReligions()->GetAdjacentCityReligiousPressure(eReligiousMajority, m_pCity, pTargetCity, iNumTradeRoutes, true);
		return true;
	}

	return false;
}


/// Handle a change in the city population
void CvCityReligions::DoPopulationChange(int iChange)
{
	ReligionTypes eMajorityReligion = GetReligiousMajority();

	// Only add pressure if the population went up; if starving, leave pressure alone (but recompute followers)
	if(iChange > 0)
	{
		AddReligiousPressure(FOLLOWER_CHANGE_POP_CHANGE, eMajorityReligion, iChange * GC.getRELIGION_ATHEISM_PRESSURE_PER_POP());
	}
	else if (iChange < 0)
	{
		RecomputeFollowers(FOLLOWER_CHANGE_POP_CHANGE, eMajorityReligion);
	}
}

/// Note that a religion was founded here
void CvCityReligions::DoReligionFounded(ReligionTypes eReligion)
{
	int iInitialPressure;
	ReligionTypes eOldMajorityReligion = GetReligiousMajority();

	iInitialPressure = GC.getRELIGION_INITIAL_FOUNDING_CITY_PRESSURE() * m_pCity->getPopulation();
	CvReligionInCity newReligion(eReligion, true, 0, iInitialPressure);
	m_ReligionStatus.push_back(newReligion);

	RecomputeFollowers(FOLLOWER_CHANGE_RELIGION_FOUNDED, eOldMajorityReligion);
}

/// Prophet spread is very powerful: eliminates all existing religions and adds to his
void CvCityReligions::AddProphetSpread(ReligionTypes eReligion, int iPressure, PlayerTypes eResponsiblePlayer)
{
	int iAtheismPressure = 0;
	int iReligionPressure = 0;
	int iPressureRetained = 0;
	ReligionTypes eOldMajorityReligion = GetReligiousMajority();
	ReligionTypes eHolyCityReligion = NO_RELIGION;
	ReligionTypes ePressureRetainedReligion = NO_RELIGION;
	bool bProphetsReligionFoundedHere = false;

	ReligionInCityList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); ++it)
#else
	for(it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); it++)
#endif
	{
		if (it->m_eReligion == NO_RELIGION)
		{
			iAtheismPressure = it->m_iPressure;
		}
		else if (eReligion == it->m_eReligion)
		{
			iReligionPressure = it->m_iPressure;
			if (it->m_bFoundedHere)
			{
				bProphetsReligionFoundedHere = true;
			}
		}
		else if (it->m_bFoundedHere)
		{
			eHolyCityReligion = it->m_eReligion;
		}

		if (it->m_eReligion > RELIGION_PANTHEON &&  it->m_eReligion != eReligion)
		{
			const CvReligion *pReligion = GC.getGame().GetGameReligions()->GetReligion(it->m_eReligion, NO_PLAYER);
			int iPressureRetention = pReligion->m_Beliefs.GetInquisitorPressureRetention();  // Normally 0
			if (iPressureRetention > 0)
			{
				ePressureRetainedReligion = it->m_eReligion;
				iPressureRetained = it->m_iPressure * iPressureRetention / 100;
			}
		}
	}

	// Clear list
	m_ReligionStatus.clear();

	// Add atheists and this back in
	CvReligionInCity atheism(NO_RELIGION, false/*bFoundedHere*/, 0, iAtheismPressure);
	m_ReligionStatus.push_back(atheism);
	CvReligionInCity prophetReligion(eReligion, bProphetsReligionFoundedHere, 0, iReligionPressure + iPressure);
	m_ReligionStatus.push_back(prophetReligion);

	// Reestablish Holy City religion
	if (eHolyCityReligion != NO_RELIGION && !bProphetsReligionFoundedHere)
	{
		if (eHolyCityReligion == ePressureRetainedReligion)
		{
			CvReligionInCity holyCityReligion(eHolyCityReligion, true/*bFoundedHere*/, 0, iPressureRetained);
			m_ReligionStatus.push_back(holyCityReligion);			
		}
		else
		{
			CvReligionInCity holyCityReligion(eHolyCityReligion, true/*bFoundedHere*/, 0, 0);
			m_ReligionStatus.push_back(holyCityReligion);
		}
	}

	// Reestablish pressure-retained religion (if wasn't Holy City religion)
	if (ePressureRetainedReligion != NO_RELIGION && eHolyCityReligion != ePressureRetainedReligion)
	{
		CvReligionInCity pressureRetainedReligion(ePressureRetainedReligion, false/*bFoundedHere*/, 0, iPressureRetained);
		m_ReligionStatus.push_back(pressureRetainedReligion);
	}

	RecomputeFollowers(FOLLOWER_CHANGE_PROPHET, eOldMajorityReligion, eResponsiblePlayer);
}

/// Add pressure to recruit followers to a religion
void CvCityReligions::AddReligiousPressure(CvReligiousFollowChangeReason eReason, ReligionTypes eReligion, int iPressure, PlayerTypes eResponsiblePlayer)
{
	bool bFoundIt = false;

	ReligionTypes eOldMajorityReligion = GetReligiousMajority();

	ReligionInCityList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); ++it)
#else
	for(it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); it++)
#endif
	{
		if(it->m_eReligion == eReligion)
		{
			it->m_iPressure += iPressure;
			bFoundIt = true;
		}

		//  If this is pressure from a real religion, reduce presence of pantheon by the same amount
		else if(eReligion > RELIGION_PANTHEON && it->m_eReligion == RELIGION_PANTHEON)
		{
#ifdef AUI_FAST_COMP
			it->m_iPressure = MAX(0, (it->m_iPressure - iPressure));
#else
			it->m_iPressure = max(0, (it->m_iPressure - iPressure));
#endif
		}

		else if (it->m_eReligion > RELIGION_PANTHEON && eReason == FOLLOWER_CHANGE_MISSIONARY)
		{
			const CvReligion *pReligion = GC.getGame().GetGameReligions()->GetReligion(eReligion, NO_PLAYER);
			int iPressureErosion = pReligion->m_Beliefs.GetOtherReligionPressureErosion();  // Normally 0
			if (iPressureErosion > 0)
			{
				int iErosionAmount = iPressureErosion * iPressure / 100;
#ifdef AUI_FAST_COMP
				it->m_iPressure = MAX(0, (it->m_iPressure - iErosionAmount));
#else
				it->m_iPressure = max(0, (it->m_iPressure - iErosionAmount));
#endif
			}
		}
	}

	// Didn't find it, add new entry
	if(!bFoundIt)
	{
		CvReligionInCity newReligion(eReligion, false, 0, iPressure);
		m_ReligionStatus.push_back(newReligion);
	}

	RecomputeFollowers(eReason, eOldMajorityReligion, eResponsiblePlayer);
}

/// Simulate prophet spread
void CvCityReligions::SimulateProphetSpread(ReligionTypes eReligion, int iPressure)
{
	int iAtheismPressure = 0;
	int iReligionPressure = 0;
	int iPressureRetained = 0;
	ReligionTypes ePressureRetainedReligion = NO_RELIGION;

	CopyToSimulatedStatus();

	ReligionInCityList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_SimulatedStatus.begin(); it != m_SimulatedStatus.end(); ++it)
#else
	for(it = m_SimulatedStatus.begin(); it != m_SimulatedStatus.end(); it++)
#endif
	{
		if(it->m_eReligion == NO_RELIGION)
		{
			iAtheismPressure = it->m_iPressure;
		}
		else if(eReligion == it->m_eReligion)
		{
			iReligionPressure = it->m_iPressure;
		}

		if (it->m_eReligion > RELIGION_PANTHEON && eReligion != it->m_eReligion)
		{
			const CvReligion *pReligion = GC.getGame().GetGameReligions()->GetReligion(it->m_eReligion, NO_PLAYER);
			int iPressureRetention = pReligion->m_Beliefs.GetInquisitorPressureRetention();  // Normally 0
			if (iPressureRetention > 0)
			{
				ePressureRetainedReligion = it->m_eReligion;
				iPressureRetained = it->m_iPressure * iPressureRetention / 100;
			}
		}
	}

	// Clear list
	m_SimulatedStatus.clear();

	// Add atheists and this back in
	CvReligionInCity atheism(NO_RELIGION, false, 0, iAtheismPressure);
	m_SimulatedStatus.push_back(atheism);
	CvReligionInCity prophetReligion(eReligion, false, 0, iReligionPressure + iPressure);
	m_SimulatedStatus.push_back(prophetReligion);

	if (ePressureRetainedReligion != NO_RELIGION)
	{
		CvReligionInCity pressureRetainedReligion(ePressureRetainedReligion, false, 0, iPressureRetained);
		m_SimulatedStatus.push_back(pressureRetainedReligion);

	}

	SimulateFollowers();
}

/// Simulate religious pressure addition
void CvCityReligions::SimulateReligiousPressure(ReligionTypes eReligion, int iPressure)
{
	bool bFoundIt = false;

	CopyToSimulatedStatus();

	ReligionInCityList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_SimulatedStatus.begin(); it != m_SimulatedStatus.end(); ++it)
#else
	for(it = m_SimulatedStatus.begin(); it != m_SimulatedStatus.end(); it++)
#endif
	{
		if(it->m_eReligion == eReligion)
		{
			it->m_iPressure += iPressure;
			bFoundIt = true;
		}

		//  If this is pressure from a real religion, reduce presence of pantheon by the same amount
		else if(eReligion > RELIGION_PANTHEON && it->m_eReligion == RELIGION_PANTHEON)
		{
#ifdef AUI_FAST_COMP
			it->m_iPressure = MAX(0, (it->m_iPressure - iPressure));
#else
			it->m_iPressure = max(0, (it->m_iPressure - iPressure));
#endif
		}

		else if (it->m_eReligion > RELIGION_PANTHEON)
		{
			const CvReligion *pReligion = GC.getGame().GetGameReligions()->GetReligion(eReligion, NO_PLAYER);
			int iPressureErosion = pReligion->m_Beliefs.GetOtherReligionPressureErosion();  // Normally 0
			if (iPressureErosion > 0)
			{
				int iErosionAmount = iPressureErosion * iPressure / 100;
#ifdef AUI_FAST_COMP
				it->m_iPressure = MAX(0, (it->m_iPressure - iErosionAmount));
#else
				it->m_iPressure = max(0, (it->m_iPressure - iErosionAmount));
#endif
			}
		}
	}

	// Didn't find it, add new entry
	if(!bFoundIt)
	{
		CvReligionInCity newReligion(eReligion, false, 0, iPressure);
		m_SimulatedStatus.push_back(newReligion);
	}

	SimulateFollowers();
}

/// Convert some percentage of followers from one religion to another
void CvCityReligions::ConvertPercentFollowers(ReligionTypes eToReligion, ReligionTypes eFromReligion, int iPercent)
{
	int iPressureConverting = 0;

	// Find old religion
	ReligionInCityList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); ++it)
#else
	for(it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); it++)
#endif
	{
		if(it->m_eReligion == eFromReligion)
		{
			iPressureConverting = it->m_iPressure * iPercent / 100;
			it->m_iPressure -= iPressureConverting;
			if (it->m_iPressure < 0)
			{
				it->m_iPressure = 0;
			}
		}
	}
	AddReligiousPressure(FOLLOWER_CHANGE_SCRIPTED_CONVERSION, eToReligion, iPressureConverting, NO_PLAYER);
}

/// Add pressure to recruit followers to a religion
void CvCityReligions::AddHolyCityPressure()
{
	bool bRecompute = false;
	ReligionTypes eOldMajorityReligion = GetReligiousMajority();

	ReligionInCityList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); ++it)
#else
	for(it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); it++)
#endif
	{
		if(it->m_bFoundedHere)
		{
			int iPressure = GC.getGame().getGameSpeedInfo().getReligiousPressureAdjacentCity();
			iPressure *=  GC.getRELIGION_PER_TURN_FOUNDING_CITY_PRESSURE();
			it->m_iPressure += iPressure;

			// Found it, so we're done
			bRecompute = true;
		}
	}

	// Didn't find it, add new entry
	if(bRecompute)
	{
		RecomputeFollowers(FOLLOWER_CHANGE_HOLY_CITY, eOldMajorityReligion);
	}
}

/// Add pressure to recruit followers to a religion
void CvCityReligions::AddSpyPressure(ReligionTypes eReligion, int iBasePressure)
{
	bool bRecompute = false;
	ReligionTypes eOldMajorityReligion = GetReligiousMajority();

	ReligionInCityList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); ++it)
#else
	for(it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); it++)
#endif
	{
		if(it->m_eReligion == eReligion)
		{
			int iPressure = GC.getGame().getGameSpeedInfo().getReligiousPressureAdjacentCity();
			iPressure *= iBasePressure;
			it->m_iPressure += iPressure;

			// Found it, so we're done
			bRecompute = true;
		}
	}

	// Didn't find it, add new entry
	if(bRecompute)
	{
		RecomputeFollowers(FOLLOWER_CHANGE_SPY_PRESSURE, eOldMajorityReligion);
	}
}

/// Set this city to have all citizens following a religion (mainly for scripting)
void CvCityReligions::AdoptReligionFully(ReligionTypes eReligion)
{
	m_ReligionStatus.clear();

	CvReligionInCity religion;

	// Add 1 pop of Atheism (needed in case other religions wiped out by an Inquisitor/Prophet
	religion.m_bFoundedHere = false;
	religion.m_eReligion = NO_RELIGION;
	religion.m_iFollowers = 1;
	religion.m_iPressure = religion.m_iFollowers * GC.getRELIGION_ATHEISM_PRESSURE_PER_POP();
	m_ReligionStatus.push_back(religion);

	// Now add full pop of this religion
	religion.m_bFoundedHere = false;
	religion.m_eReligion = eReligion;
	religion.m_iFollowers = m_pCity->getPopulation();
	religion.m_iPressure = religion.m_iFollowers * GC.getRELIGION_ATHEISM_PRESSURE_PER_POP();
	m_ReligionStatus.push_back(religion);

	m_pCity->UpdateReligion(eReligion);
}

/// Remove presence of old owner's pantheon (used when a city is conquered)
void CvCityReligions::RemoveFormerPantheon()
{
#ifndef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	bool bFoundIt = false;
#endif
	ReligionTypes eOldMajorityReligion = GetReligiousMajority();

	ReligionInCityList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); ++it)
#else
	for(it = m_ReligionStatus.begin(); it != m_ReligionStatus.end() && !bFoundIt; it++)
#endif
	{
		if(it->m_eReligion == RELIGION_PANTHEON)
		{
			m_ReligionStatus.erase(it);

			// Found it, so we're done
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
			break;
#else
			bFoundIt = true;
#endif
		}
	}

	RecomputeFollowers(FOLLOWER_CHANGE_CONQUEST, eOldMajorityReligion);
}

/// Remove other religions in a city (used by Inquisitor)
void CvCityReligions::RemoveOtherReligions(ReligionTypes eReligion, PlayerTypes eResponsiblePlayer)
{
	ReligionTypes eOldMajorityReligion = GetReligiousMajority();

	// Copy list
	ReligionInCityList tempList;
	ReligionInCityList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); ++it)
#else
	for(it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); it++)
#endif
	{
		tempList.push_back(*it);
	}

	// Erase old list
	m_ReligionStatus.clear();

	// Recopy just what we want to keep
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = tempList.begin(); it != tempList.end(); ++it)
#else
	for(it = tempList.begin(); it != tempList.end(); it++)
#endif
	{
		int iPressureRetained = 0;

		ReligionTypes eLoopReligion = it->m_eReligion;
		if (eLoopReligion > RELIGION_PANTHEON && eLoopReligion != eReligion)
		{
			const CvReligion *pReligion = GC.getGame().GetGameReligions()->GetReligion(eLoopReligion, NO_PLAYER);
			iPressureRetained = pReligion->m_Beliefs.GetInquisitorPressureRetention();  // Normally 0
		}

		if (eLoopReligion == NO_RELIGION || eLoopReligion == eReligion || iPressureRetained > 0)
		{
			if (iPressureRetained > 0)
			{
				it->m_iPressure = it->m_iPressure * iPressureRetained / 100;
			}

			m_ReligionStatus.push_back(*it);
		}
	}

	RecomputeFollowers(FOLLOWER_CHANGE_REMOVE_HERESY, eOldMajorityReligion, eResponsiblePlayer);
}

/// Called from the trade system when a trade connection is made between two cities
void CvCityReligions::UpdateNumTradeRouteConnections(CvCity* pOtherCity)
{
	ReligionTypes eReligiousMajority = GetReligiousMajority();

	// if there isn't a religious connection, whatvz
	if (eReligiousMajority == NO_RELIGION)
	{
		return;
	}

	const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eReligiousMajority, m_pCity->getOwner());
	if (!pReligion)
	{
		return;
	}

	// Are the cities within the minimum distance?
	int iDistance = GC.getRELIGION_ADJACENT_CITY_DISTANCE();

	// Boost to distance due to belief?
	int iDistanceMod = pReligion->m_Beliefs.GetSpreadDistanceModifier();
	if(iDistanceMod > 0)
	{
		iDistance *= (100 + iDistanceMod);
		iDistance /= 100;
	}

	bool bWithinDistance = (plotDistance(m_pCity->getX(), m_pCity->getY(), pOtherCity->getX(), pOtherCity->getY()) <= iDistance);

	// if not within distance, then we're using a trade route
	if (!bWithinDistance) 
	{
		pOtherCity->GetCityReligions()->IncrementNumTradeRouteConnections(eReligiousMajority, 1);
	}
}

/// Increment the number of trade connections a city has
void CvCityReligions::IncrementNumTradeRouteConnections(ReligionTypes eReligion, int iNum)
{
	ReligionInCityList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); ++it)
#else
	for(it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); it++)
#endif
	{
		if(it->m_eReligion == eReligion)
		{
			it->m_iNumTradeRoutesApplyingPressure += iNum;
			break;
		}
	}
}

/// How many followers would we have adding this religious pressure here?
int CvCityReligions::GetNumFollowersAfterSpread(ReligionTypes eReligion, int iConversionStrength)
{
	SimulateReligiousPressure(eReligion, iConversionStrength);
	return GetNumSimulatedFollowers(eReligion);
}

/// How many followers would we have having a prophet add religious pressure here?
int CvCityReligions::GetNumFollowersAfterProphetSpread(ReligionTypes eReligion, int iConversionStrength)
{
	SimulateProphetSpread(eReligion, iConversionStrength);
	return GetNumSimulatedFollowers(eReligion);
}

/// What would the majority religion be adding this religious pressure here?
ReligionTypes CvCityReligions::GetMajorityReligionAfterSpread(ReligionTypes eReligion, int iConversionStrength)
{
	SimulateReligiousPressure(eReligion, iConversionStrength);
	return GetSimulatedReligiousMajority();
}

/// What would the majority religion be adding this religious pressure here?
ReligionTypes CvCityReligions::GetMajorityReligionAfterProphetSpread(ReligionTypes eReligion, int iConversionStrength)
{
	SimulateProphetSpread(eReligion, iConversionStrength);
	return GetSimulatedReligiousMajority();
}

/// Resets the number of trade routes pressuring a city
void CvCityReligions::ResetNumTradeRoutePressure()
{
	ReligionInCityList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); ++it)
#else
	for(it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); it++)
#endif
	{
		it->m_iNumTradeRoutesApplyingPressure = 0;
	}
}

// PRIVATE METHODS

/// Calculate the number of followers for each religion
void CvCityReligions::RecomputeFollowers(CvReligiousFollowChangeReason eReason, ReligionTypes eOldMajorityReligion, PlayerTypes eResponsibleParty)
{
	int iOldFollowers = GetNumFollowers(eOldMajorityReligion);
	int iUnassignedFollowers = m_pCity->getPopulation();
	int iPressurePerFollower;

	// Safety check to avoid divide by zero
	if (iUnassignedFollowers < 1)
	{
		CvAssertMsg (false, "Invalid city population when recomputing followers");
		return;
	}

	// Find total pressure
	int iTotalPressure = 0;
	ReligionInCityList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); ++it)
#else
	for(it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); it++)
#endif
	{
		iTotalPressure += it->m_iPressure;
	}

	// safety check - if pressure was wiped out somehow, just rebuild pressure of 1 atheist
	if (iTotalPressure <= 0)
	{
		m_ReligionStatus.clear();

		CvReligionInCity religion;
		religion.m_bFoundedHere = false;
		religion.m_eReligion = NO_RELIGION;
		religion.m_iFollowers = 1;
		religion.m_iPressure = GC.getRELIGION_ATHEISM_PRESSURE_PER_POP();
		m_ReligionStatus.push_back(religion);

		iTotalPressure = GC.getRELIGION_ATHEISM_PRESSURE_PER_POP();
	}

	iPressurePerFollower = iTotalPressure / iUnassignedFollowers;

	// Loop through each religion
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); ++it)
#else
	for(it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); it++)
#endif
	{
		it->m_iFollowers = it->m_iPressure / iPressurePerFollower;
		iUnassignedFollowers -= it->m_iFollowers;
		it->m_iTemp = it->m_iPressure - (it->m_iFollowers * iPressurePerFollower);  // Remainder
	}

	// Assign out the remainder
	for (int iI = 0; iI < iUnassignedFollowers; iI++)
	{
		ReligionInCityList::iterator itLargestRemainder = NULL;
		int iLargestRemainder = 0;

#ifdef AUI_WARNING_FIXES
		for (it = m_ReligionStatus.begin(); it != m_ReligionStatus.end() && it; ++it)
#else
		for (it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); it++)
#endif
		{
			if (it->m_iTemp > iLargestRemainder)
			{
				iLargestRemainder = it->m_iTemp;
				itLargestRemainder = it;
			}
		}

		if (itLargestRemainder && iLargestRemainder > 0)
		{
			itLargestRemainder->m_iFollowers++;
			itLargestRemainder->m_iTemp = 0;
		}
	}

	ReligionTypes eMajority = GetReligiousMajority();
	int iFollowers = GetNumFollowers(eMajority);

	if(eMajority != eOldMajorityReligion || iFollowers != iOldFollowers)
	{
		CityConvertsReligion(eMajority, eOldMajorityReligion, eResponsibleParty);
		GC.GetEngineUserInterface()->setDirty(CityInfo_DIRTY_BIT, true);
		LogFollowersChange(eReason);
	}
}

/// Calculate the number of followers for each religion from simulated data
void CvCityReligions::SimulateFollowers()
{
	int iUnassignedFollowers = m_pCity->getPopulation();
	int iPressurePerFollower;

	// Find total pressure
	int iTotalPressure = 0;
	ReligionInCityList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_SimulatedStatus.begin(); it != m_SimulatedStatus.end(); ++it)
#else
	for(it = m_SimulatedStatus.begin(); it != m_SimulatedStatus.end(); it++)
#endif
	{
		iTotalPressure += it->m_iPressure;
	}

	// safety check
	if (iTotalPressure == 0 || iUnassignedFollowers == 0)
	{
		CvAssertMsg(false, "Internal religion data error. Send save to Ed");
		return;
	}

	iPressurePerFollower = iTotalPressure / iUnassignedFollowers;

	// Loop through each religion
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_SimulatedStatus.begin(); it != m_SimulatedStatus.end(); ++it)
#else
	for(it = m_SimulatedStatus.begin(); it != m_SimulatedStatus.end(); it++)
#endif
	{
		it->m_iFollowers = it->m_iPressure / iPressurePerFollower;
		iUnassignedFollowers -= it->m_iFollowers;
		it->m_iTemp = it->m_iPressure - (it->m_iFollowers * iPressurePerFollower);  // Remainder
	}

	// Assign out the remainder
	for (int iI = 0; iI < iUnassignedFollowers; iI++)
	{
		ReligionInCityList::iterator itLargestRemainder = NULL;
		int iLargestRemainder = 0;

#ifdef AUI_WARNING_FIXES
		for (it = m_SimulatedStatus.begin(); it != m_SimulatedStatus.end() && it; ++it)
#else
		for (it = m_SimulatedStatus.begin(); it != m_SimulatedStatus.end(); it++)
#endif
		{
			if (it->m_iTemp > iLargestRemainder)
			{
				iLargestRemainder = it->m_iTemp;
				itLargestRemainder = it;
			}
		}

		if (itLargestRemainder && iLargestRemainder > 0)
		{
			itLargestRemainder->m_iFollowers++;
			itLargestRemainder->m_iTemp = 0;
		}
	}
}

/// Copy data to prepare to simulate a conversion
void CvCityReligions::CopyToSimulatedStatus()
{
	m_SimulatedStatus.clear();

	ReligionInCityList::iterator it;
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); ++it)
#else
	for(it = m_ReligionStatus.begin(); it != m_ReligionStatus.end(); it++)
#endif
	{
		m_SimulatedStatus.push_back(*it);
	}
}

/// Implement changes from a city changing religion
void CvCityReligions::CityConvertsReligion(ReligionTypes eMajority, ReligionTypes eOldMajority, PlayerTypes eResponsibleParty)
{
	CvGameReligions* pReligions = GC.getGame().GetGameReligions();

	m_pCity->UpdateReligion(eMajority);

	if(eOldMajority > RELIGION_PANTHEON)
	{
		const CvReligion* pOldReligion = pReligions->GetReligion(eOldMajority, NO_PLAYER);
		GET_PLAYER(pOldReligion->m_eFounder).UpdateReligion();
	}

	if(eMajority > RELIGION_PANTHEON)
	{
		const CvReligion* pNewReligion = pReligions->GetReligion(eMajority, NO_PLAYER);
		GET_PLAYER(pNewReligion->m_eFounder).UpdateReligion();

		// Pay adoption bonuses (if any)
		if(!m_bHasPaidAdoptionBonus)
		{
			int iGoldBonus = pNewReligion->m_Beliefs.GetGoldWhenCityAdopts();
			iGoldBonus *= GC.getGame().getGameSpeedInfo().getTrainPercent();;
			iGoldBonus /= 100;

			if(iGoldBonus > 0)
			{
				GET_PLAYER(pNewReligion->m_eFounder).GetTreasury()->ChangeGold(iGoldBonus);
				SetPaidAdoptionBonus(true);

				if(pNewReligion->m_eFounder == GC.getGame().getActivePlayer())
				{
					char text[256] = {0};
					sprintf_s(text, "[COLOR_YELLOW]+%d[ENDCOLOR][ICON_GOLD]", iGoldBonus);
					GC.GetEngineUserInterface()->AddPopupText(m_pCity->getX(), m_pCity->getY(), text, 0.5f);
				}
			}
		}

		// Notification if the player's city was converted to a religion they didn't found
		PlayerTypes eOwnerPlayer = m_pCity->getOwner();
		CvPlayerAI& kOwnerPlayer = GET_PLAYER(eOwnerPlayer);
		const ReligionTypes eOwnerPlayerReligion = kOwnerPlayer.GetReligions()->GetReligionCreatedByPlayer();

		if(eOwnerPlayer != eResponsibleParty && eMajority != eOldMajority && pNewReligion->m_eFounder != eOwnerPlayer
			&& eOwnerPlayerReligion > RELIGION_PANTHEON)
		{
			if(kOwnerPlayer.GetNotifications())
			{
				Localization::String strMessage;
				Localization::String strSummary;
				strMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_RELIGION_SPREAD_ACTIVE_PLAYER", m_pCity->getName());
				strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_RELIGION_SPREAD_ACTIVE_PLAYER_S");
				kOwnerPlayer.GetNotifications()->Add(NOTIFICATION_RELIGION_SPREAD, strMessage.toUTF8(), strSummary.toUTF8(), m_pCity->getX(), m_pCity->getY(), -1);
			}

			//Achievements!
			if(eOwnerPlayer == GC.getGame().getActivePlayer()){
				const CvReligion* pkReligion = GC.getGame().GetGameReligions()->GetReligion(eOwnerPlayerReligion, eOwnerPlayer);
				if(pkReligion != NULL)
				{
					if(m_pCity->getX() == pkReligion->m_iHolyCityX && m_pCity->getY() == pkReligion->m_iHolyCityY)
					{
						gDLL->UnlockAchievement(ACHIEVEMENT_XP1_20);
					}
				}
			}
		}

		else if(eOwnerPlayer != eResponsibleParty && eMajority != eOldMajority && eOldMajority == NO_RELIGION)
		{
			if(kOwnerPlayer.GetNotifications())
			{
				Localization::String strMessage;
				Localization::String strSummary;
				strMessage = GetLocalizedText("TXT_KEY_NOTIFICATION_RELIGION_SPREAD_INITIAL_CONVERSION", m_pCity->getName());
				strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_RELIGION_SPREAD_INITIAL_CONVERSION_S");
				kOwnerPlayer.GetNotifications()->Add(NOTIFICATION_RELIGION_SPREAD_NATURAL, strMessage.toUTF8(), strSummary.toUTF8(), m_pCity->getX(), m_pCity->getY(), -1);
			}
		}

		//More Achievements
		if(m_pCity->getOwner() != GC.getGame().getActivePlayer() && pNewReligion->m_eFounder == GC.getGame().getActivePlayer())
		{
			if(m_pCity->GetCityReligions()->IsHolyCityAnyReligion() && !m_pCity->GetCityReligions()->IsHolyCityForReligion(pNewReligion->m_eReligion))
			{
				gDLL->UnlockAchievement(ACHIEVEMENT_XP1_18);
			}
		}

#ifndef AUI_ACHIEVEMENT_FIX_RELIGION_WE_ARE_FAMILY_WORKING
		if(m_pCity->isCapital() && pNewReligion->m_eFounder == GC.getGame().getActivePlayer())
		{
			//Determine if this is a standard size or larger map.
			bool bIsStandardOrLarger = false;
			Database::Connection* pDB = GC.GetGameDatabase();
			Database::Results kStandardSize;
			if(pDB->SelectAt(kStandardSize, "Worlds", "Type", "WORLDSIZE_STANDARD"))
			{
				if(kStandardSize.Step())
				{
					int idColumn = kStandardSize.ColumnPosition("ID");
					if(idColumn >= 0)
					{
						WorldSizeTypes eWorldSize = GC.getMap().getWorldSize();
						int standardWorldSize = kStandardSize.GetInt(idColumn);
						if(eWorldSize >= standardWorldSize)
						{
							bIsStandardOrLarger = true;
						}
					}
				}
			}

			if(bIsStandardOrLarger)
			{
				//Determine if this religion has spread to all capitals
				bool bSpreadToAllCapitals = true;
				for(int i = 0; i < MAX_MAJOR_CIVS; ++i)
				{
					CvPlayerAI& kPlayer = GET_PLAYER(static_cast<PlayerTypes>(i));
					if(kPlayer.isAlive())
					{
						CvCity* pCapital = kPlayer.getCapitalCity();
						if(pCapital != NULL)
						{
							CvCityReligions* pCityReligions = pCapital->GetCityReligions();
							if(pCityReligions != NULL)
							{
								if(pCityReligions->GetReligiousMajority() != pNewReligion->m_eFounder)
								{
									bSpreadToAllCapitals = false;
									break;
								}
							}
						}
					}

					if(bSpreadToAllCapitals)
					{
						gDLL->UnlockAchievement(ACHIEVEMENT_XP1_19);
					}
				}
			}
		}
#endif

		// Diplo implications (there must have been religion switch and a responsible party)
		if(eMajority != eOldMajority && eResponsibleParty != NO_PLAYER)
		{
			// Is the city owner not the founder of this religion?
			if(pNewReligion->m_eFounder != m_pCity->getOwner())
			{
				CvPlayer& kCityOwnerPlayer = GET_PLAYER(m_pCity->getOwner());

				// Did he found another religion?
				ReligionTypes eCityOwnerReligion = kCityOwnerPlayer.GetReligions()->GetReligionCreatedByPlayer();
				if(eCityOwnerReligion >= RELIGION_PANTHEON)
				{
					int iPoints = 0;

					// His religion wasn't present here, minor hit
					if(eOldMajority != eCityOwnerReligion)
					{
						iPoints = GC.getRELIGION_DIPLO_HIT_INITIAL_CONVERT_FRIENDLY_CITY(); /*1*/
					}

					// This was his holy city; huge hit!
					else if(m_pCity->GetCityReligions()->IsHolyCityForReligion(eCityOwnerReligion))
					{
						iPoints = GC.getRELIGION_DIPLO_HIT_CONVERT_HOLY_CITY(); /*25*/
					}

					// He had established his religion here, major hit
					else
					{
						iPoints = GC.getRELIGION_DIPLO_HIT_RELIGIOUS_FLIP_FRIENDLY_CITY(); /*3*/
					}

					kCityOwnerPlayer.GetDiplomacyAI()->ChangeNegativeReligiousConversionPoints(eResponsibleParty, iPoints);
				}
			}
		}

		ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
		if(pkScriptSystem)
		{
			CvLuaArgsHandle args;
			args->Push(m_pCity->getOwner());
			args->Push(eMajority);
			args->Push(m_pCity->getX());
			args->Push(m_pCity->getY());

			// Attempt to execute the game events.
			// Will return false if there are no registered listeners.
			bool bResult = false;
			LuaSupport::CallHook(pkScriptSystem, "CityConvertsReligion", args.get(), bResult);
		}
	}
}

/// Log a message with status information
void CvCityReligions::LogFollowersChange(CvReligiousFollowChangeReason eReason)
{
	if(GC.getLogging() && GC.getAILogging())
	{
		CvString strOutBuf;
		CvString strReasonString;
		CvString temp;
		FILogFile* pLog;
		CvCityReligions* pCityRel = m_pCity->GetCityReligions();

		pLog = LOGFILEMGR.GetLog(GC.getGame().GetGameReligions()->GetLogFileName(), FILogFile::kDontTimeStamp);

		// Get the leading info for this line
		strOutBuf.Format("%03d, %d, ", GC.getGame().getElapsedGameTurns(), GC.getGame().getGameTurnYear());
		strOutBuf += m_pCity->getName();
		strOutBuf += ", ";

		// Add a reason string
		switch(eReason)
		{
		case FOLLOWER_CHANGE_ADJACENT_PRESSURE:
			strReasonString = "Adjacent city pressure";
			break;
		case FOLLOWER_CHANGE_HOLY_CITY:
			strReasonString = "Holy city pressure";
			break;
		case FOLLOWER_CHANGE_POP_CHANGE:
			strReasonString = "Population change";
			break;
		case FOLLOWER_CHANGE_RELIGION_FOUNDED:
			strReasonString = "Religion founded";
			break;
		case FOLLOWER_CHANGE_PANTHEON_FOUNDED:
			strReasonString = "Pantheon founded";
			break;
		case FOLLOWER_CHANGE_CONQUEST:
			strReasonString = "City captured";
			break;
		case FOLLOWER_CHANGE_MISSIONARY:
			strReasonString = "Missionary expended";
			break;
		case FOLLOWER_CHANGE_PROPHET:
			strReasonString = "Prophet spreading";
			break;
		case FOLLOWER_CHANGE_REMOVE_HERESY:
			strReasonString = "Remove heresy";
			break;
		case FOLLOWER_CHANGE_SCRIPTED_CONVERSION:
			strReasonString = "Scripted conversion";
			break;
		case FOLLOWER_CHANGE_SPY_PRESSURE:
			strReasonString = "Spy pressure";
			break;
		}
		strOutBuf += strReasonString + ", ";
		temp.Format("Pop: %d", m_pCity->getPopulation());
		strOutBuf += temp;
		if(pCityRel->IsReligionInCity())
		{
			ReligionTypes eMajority = pCityRel->GetReligiousMajority();
			if(eMajority != NO_RELIGION)
			{
				CvReligionEntry* pEntry = GC.getReligionInfo(eMajority);
				if(pEntry)
				{
					strOutBuf += ", Majority: ";
					strOutBuf += pEntry->GetDescription();
					temp.Format("(%d)", pCityRel->GetNumFollowers(eMajority));
					strOutBuf += temp;
				}
			}
			
			ReligionTypes eSecondary = pCityRel->GetSecondaryReligion();
			if (eSecondary != NO_RELIGION)
			{
				CvReligionEntry* pEntry = GC.getReligionInfo(eSecondary);
				if(pEntry)
				{
					strOutBuf += ", Secondary: ";
					strOutBuf += pEntry->GetDescription();
					temp.Format("(%d)", pCityRel->GetNumFollowers(eSecondary));
					strOutBuf += temp;
				}
			}

			temp.Format("Atheists: %d", pCityRel->GetNumFollowers(NO_RELIGION));
			strOutBuf += ", " + temp;
		}
		else
		{
			strOutBuf += ", No religion in city";
		}

		pLog->Msg(strOutBuf);
	}
}
/// Serialization read
FDataStream& operator>>(FDataStream& loadFrom, CvCityReligions& writeTo)
{
	uint uiVersion;

	loadFrom >> uiVersion;

	if(uiVersion >= 2)
	{
		bool bTemp;
		loadFrom >> bTemp;
		writeTo.SetPaidAdoptionBonus(bTemp);
	}
	else
	{
		writeTo.SetPaidAdoptionBonus(false);
	}

	if (uiVersion >= 3)
	{
		int iTemp;
		loadFrom >> iTemp;
		writeTo.SetReligiousPressureModifier(iTemp);
	}
	else
	{
		writeTo.SetReligiousPressureModifier(0);
	}

	int iEntriesToRead;
	CvReligionInCity tempItem;

	writeTo.m_ReligionStatus.clear();
	loadFrom >> iEntriesToRead;
	for(int iI = 0; iI < iEntriesToRead; iI++)
	{
		loadFrom >> tempItem;
		writeTo.m_ReligionStatus.push_back(tempItem);
	}

	return loadFrom;
}

/// Serialization write
FDataStream& operator<<(FDataStream& saveTo, const CvCityReligions& readFrom)
{
	uint uiVersion = 3;

	saveTo << uiVersion;

	saveTo << readFrom.HasPaidAdoptionBonus();

	saveTo << readFrom.GetReligiousPressureModifier();

	ReligionInCityList::const_iterator it;
	saveTo << readFrom.m_ReligionStatus.size();
#ifdef AUI_ITERATOR_POSTFIX_INCREMENT_OPTIMIZATIONS
	for (it = readFrom.m_ReligionStatus.begin(); it != readFrom.m_ReligionStatus.end(); ++it)
#else
	for(it = readFrom.m_ReligionStatus.begin(); it != readFrom.m_ReligionStatus.end(); it++)
#endif
	{
		saveTo << *it;
	}

	return saveTo;
}

//=====================================
// CvGameReligions
//=====================================
/// Constructor
CvUnitReligion::CvUnitReligion(void):
	m_eReligion(NO_RELIGION),
	m_iStrength(0),
	m_iSpreadsLeft(0)
{
}

/// Initialize class data
void CvUnitReligion::Init()
{
	m_eReligion = NO_RELIGION;
	m_iStrength = 0;
	m_iSpreadsLeft = 0;
}

/// Serialization read
FDataStream& operator>>(FDataStream& loadFrom, CvUnitReligion& writeTo)
{
	uint uiVersion;

	loadFrom >> uiVersion;

	int temp;
	loadFrom >> temp;
	writeTo.SetReligion((ReligionTypes)temp);
	loadFrom >> temp;
	writeTo.SetReligiousStrength(temp);

	if(uiVersion >= 2)
	{
		loadFrom >> temp;
		writeTo.SetSpreadsLeft(temp);
	}
	else
	{
		writeTo.SetSpreadsLeft(0);
	}

	return loadFrom;
}

/// Serialization write
FDataStream& operator<<(FDataStream& saveTo, const CvUnitReligion& readFrom)
{
	uint uiVersion = 2;

	saveTo << uiVersion;

	saveTo << readFrom.GetReligion();
	saveTo << readFrom.GetReligiousStrength();
	saveTo << readFrom.GetSpreadsLeft();

	return saveTo;
}

//=====================================
// CvReligionAI
//=====================================
/// Constructor
CvReligionAI::CvReligionAI(void):
	m_pPlayer(NULL)
{
}

/// Destructor
CvReligionAI::~CvReligionAI(void)
{
	Uninit();
}

/// Initialize class data
void CvReligionAI::Init(CvBeliefXMLEntries* pBeliefs, CvPlayer* pPlayer)
{
	m_pBeliefs = pBeliefs;
	m_pPlayer = pPlayer;

	Reset();
}

/// Cleanup
void CvReligionAI::Uninit()
{

}

/// Reset
void CvReligionAI::Reset()
{

}

/// Serialization read
void CvReligionAI::Read(FDataStream& kStream)
{
	// Version number to maintain backwards compatibility
	uint uiVersion;
	kStream >> uiVersion;
}

/// Serialization write
void CvReligionAI::Write(FDataStream& kStream)
{
	// Current version number
	uint uiVersion = 1;
	kStream << uiVersion;
}

/// Called every turn to see what to spend Faith on
void CvReligionAI::DoTurn()
{
	// Only AI players use this function for now
	if(m_pPlayer->isHuman())
	{
		return;
	}

	AI_PERF_FORMAT("AI-perf.csv", ("Religion AI, Turn %03d, %s", GC.getGame().getElapsedGameTurns(), m_pPlayer->getCivilizationShortDescription()) );

	DoFaithPurchases();
}

/// Select the belief most helpful to this pantheon
BeliefTypes CvReligionAI::ChoosePantheonBelief()
{
	CvGameReligions* pGameReligions = GC.getGame().GetGameReligions();
	CvWeightedVector<BeliefTypes, SAFE_ESTIMATE_NUM_BELIEFS, true> beliefChoices;

	std::vector<BeliefTypes> availableBeliefs = pGameReligions->GetAvailablePantheonBeliefs();

	for(std::vector<BeliefTypes>::iterator it = availableBeliefs.begin();
	        it!= availableBeliefs.end(); ++it)
	{
		const BeliefTypes eBelief = (*it);
		CvBeliefEntry* pEntry = m_pBeliefs->GetEntry(eBelief);
		if(pEntry)
		{
			const int iScore = ScoreBelief(pEntry);
			if(iScore > 0)
			{
				beliefChoices.push_back(eBelief, iScore);
			}
		}
	}

#ifdef AUI_RELIGION_FIX_POSSIBLE_NULL_POINTER
	BeliefTypes rtnValue = NO_BELIEF;
	if (beliefChoices.size() > 0)
	{
		// Choose from weighted vector
		beliefChoices.SortItems();
		int iNumChoices = MIN(beliefChoices.size(), 3);   // Throw out two-thirds of the choices -- this was way too loose as choices way down were being selected now only top 3
#ifdef AUI_RELIGION_RELATIVE_BELIEF_SCORE
		int iToDecrease = -MAX(beliefChoices.GetWeight(beliefChoices.size() - 1) - 1, 0);
		for (int iI = 0; iI < beliefChoices.size(); iI++)
		{
			beliefChoices.IncreaseWeight(iI, iToDecrease);
		}
#endif
		RandomNumberDelegate fcn = MakeDelegate(&GC.getGame(), &CvGame::getJonRandNum);
		rtnValue = beliefChoices.ChooseFromTopChoices(iNumChoices, &fcn, "Choosing belief from Top Choices");
		LogBeliefChoices(beliefChoices, rtnValue);
	}
	else
	{
		rtnValue = availableBeliefs.at(GC.getGame().getJonRandNum(availableBeliefs.size() , "Choosing random Pantheon belief."));
	}
#else
	// Choose from weighted vector
	beliefChoices.SortItems();
	int iNumChoices = MIN(beliefChoices.size(),3);   // Throw out two-thirds of the choices -- this was way too loose as choices way down were being selected now only top 3
	RandomNumberDelegate fcn = MakeDelegate(&GC.getGame(), &CvGame::getJonRandNum);
	BeliefTypes rtnValue = beliefChoices.ChooseFromTopChoices(iNumChoices, &fcn, "Choosing belief from Top Choices");
	LogBeliefChoices(beliefChoices, rtnValue);
#endif

	return rtnValue;
}

/// Select the belief most helpful to this pantheon
BeliefTypes CvReligionAI::ChooseFounderBelief()
{
	CvGameReligions* pGameReligions = GC.getGame().GetGameReligions();
	CvWeightedVector<BeliefTypes, SAFE_ESTIMATE_NUM_BELIEFS, true> beliefChoices;

	std::vector<BeliefTypes> availableBeliefs = pGameReligions->GetAvailableFounderBeliefs();

	for(std::vector<BeliefTypes>::iterator it = availableBeliefs.begin();
	        it!= availableBeliefs.end(); ++it)
	{
		const BeliefTypes eBelief = (*it);
		CvBeliefEntry* pEntry = m_pBeliefs->GetEntry(eBelief);
		if(pEntry)
		{
			const int iScore = ScoreBelief(pEntry);
			if(iScore > 0)
			{
				beliefChoices.push_back(eBelief, iScore);
			}
		}
	}

#ifdef AUI_RELIGION_FIX_POSSIBLE_NULL_POINTER
	BeliefTypes rtnValue = NO_BELIEF;
	if (beliefChoices.size() > 0)
	{
		// Choose from weighted vector
		beliefChoices.SortItems();
		int iNumChoices = MIN(beliefChoices.size(), 3);   // this was way too loose as choices way down were being selected now only top 3
#ifdef AUI_RELIGION_RELATIVE_BELIEF_SCORE
		int iToDecrease = -MAX(beliefChoices.GetWeight(beliefChoices.size() - 1) - 1, 0);
		for (int iI = 0; iI < beliefChoices.size(); iI++)
		{
			beliefChoices.IncreaseWeight(iI, iToDecrease);
		}
#endif
		RandomNumberDelegate fcn = MakeDelegate(&GC.getGame(), &CvGame::getJonRandNum);
		rtnValue = beliefChoices.ChooseFromTopChoices(iNumChoices, &fcn, "Choosing belief from Top Choices");
		LogBeliefChoices(beliefChoices, rtnValue);
	}
	else
	{
		rtnValue = availableBeliefs.at(GC.getGame().getJonRandNum(availableBeliefs.size() , "Choosing random Founder belief."));
	}
#else
	// Choose from weighted vector
	beliefChoices.SortItems();
	int iNumChoices = MIN(beliefChoices.size(),3);   // this was way too loose as choices way down were being selected now only top 3
	RandomNumberDelegate fcn = MakeDelegate(&GC.getGame(), &CvGame::getJonRandNum);
	BeliefTypes rtnValue = beliefChoices.ChooseFromTopChoices(iNumChoices, &fcn, "Choosing belief from Top Choices");
	LogBeliefChoices(beliefChoices, rtnValue);
#endif

	return rtnValue;
}

/// Select the belief most helpful to this pantheon
BeliefTypes CvReligionAI::ChooseFollowerBelief()
{
	CvGameReligions* pGameReligions = GC.getGame().GetGameReligions();
	CvWeightedVector<BeliefTypes, SAFE_ESTIMATE_NUM_BELIEFS, true> beliefChoices;

	std::vector<BeliefTypes> availableBeliefs = pGameReligions->GetAvailableFollowerBeliefs();

	for(std::vector<BeliefTypes>::iterator it = availableBeliefs.begin();
	        it!= availableBeliefs.end(); ++it)
	{
		const BeliefTypes eBelief = (*it);
		CvBeliefEntry* pEntry = m_pBeliefs->GetEntry(eBelief);
		if(pEntry)
		{
			const int iScore = ScoreBelief(pEntry);
			if(iScore > 0)
			{
				beliefChoices.push_back(eBelief, iScore);
			}
		}
	}

#ifdef AUI_RELIGION_FIX_POSSIBLE_NULL_POINTER
	BeliefTypes rtnValue = NO_BELIEF;
	if (beliefChoices.size() > 0)
	{
		// Choose from weighted vector
		beliefChoices.SortItems();
		int iNumChoices = MIN(beliefChoices.size(), 3);   // this was way too loose as choices way down were being selected now only top 3
#ifdef AUI_RELIGION_RELATIVE_BELIEF_SCORE
		int iToDecrease = -MAX(beliefChoices.GetWeight(beliefChoices.size() - 1) - 1, 0);
		for (int iI = 0; iI < beliefChoices.size(); iI++)
		{
			beliefChoices.IncreaseWeight(iI, iToDecrease);
		}
#endif
		RandomNumberDelegate fcn = MakeDelegate(&GC.getGame(), &CvGame::getJonRandNum);
		rtnValue = beliefChoices.ChooseFromTopChoices(iNumChoices, &fcn, "Choosing belief from Top Choices");
		LogBeliefChoices(beliefChoices, rtnValue);
	}
	else
	{
		rtnValue = availableBeliefs.at(GC.getGame().getJonRandNum(availableBeliefs.size(), "Choosing random Follower belief."));
	}
#else
	// Choose from weighted vector
	beliefChoices.SortItems();
	int iNumChoices = MIN(beliefChoices.size(),3);   // this was way too loose as choices way down were being selected now only top 3
	RandomNumberDelegate fcn = MakeDelegate(&GC.getGame(), &CvGame::getJonRandNum);
	BeliefTypes rtnValue = beliefChoices.ChooseFromTopChoices(iNumChoices, &fcn, "Choosing belief from Top Choices");
	LogBeliefChoices(beliefChoices, rtnValue);
#endif

	return rtnValue;
}

/// Select the belief most helpful to enhance this religion
BeliefTypes CvReligionAI::ChooseEnhancerBelief()
{
	CvGameReligions* pGameReligions = GC.getGame().GetGameReligions();
	CvWeightedVector<BeliefTypes, SAFE_ESTIMATE_NUM_BELIEFS, true> beliefChoices;

	std::vector<BeliefTypes> availableBeliefs = pGameReligions->GetAvailableEnhancerBeliefs();

	for(std::vector<BeliefTypes>::iterator it = availableBeliefs.begin();
	        it!= availableBeliefs.end(); ++it)
	{
		const BeliefTypes eBelief = (*it);
		CvBeliefEntry* pEntry = m_pBeliefs->GetEntry(eBelief);
		if(pEntry)
		{
			const int iScore = ScoreBelief(pEntry);
			if(iScore > 0)
			{
				beliefChoices.push_back(eBelief, iScore);
			}
		}
	}

#ifdef AUI_RELIGION_FIX_POSSIBLE_NULL_POINTER
	BeliefTypes rtnValue = NO_BELIEF;
	if (beliefChoices.size() > 0)
	{
		// Choose from weighted vector
		beliefChoices.SortItems();
		int iNumChoices = MIN(beliefChoices.size(), 3);   // this was way too loose as choices way down were being selected now only top 3
#ifdef AUI_RELIGION_RELATIVE_BELIEF_SCORE
		int iToDecrease = -MAX(beliefChoices.GetWeight(beliefChoices.size() - 1) - 1, 0);
		for (int iI = 0; iI < beliefChoices.size(); iI++)
		{
			beliefChoices.IncreaseWeight(iI, iToDecrease);
		}
#endif
		RandomNumberDelegate fcn = MakeDelegate(&GC.getGame(), &CvGame::getJonRandNum);
		rtnValue = beliefChoices.ChooseFromTopChoices(iNumChoices, &fcn, "Choosing belief from Top Choices");
		LogBeliefChoices(beliefChoices, rtnValue);
	}
	else
	{
		rtnValue = availableBeliefs.at(GC.getGame().getJonRandNum(availableBeliefs.size(), "Choosing random Enhancer belief."));
	}
#else
	// Choose from weighted vector
	beliefChoices.SortItems();
	int iNumChoices = MIN(beliefChoices.size(),3);   // this was way too loose as choices way down were being selected now only top 3
	RandomNumberDelegate fcn = MakeDelegate(&GC.getGame(), &CvGame::getJonRandNum);
	BeliefTypes rtnValue = beliefChoices.ChooseFromTopChoices(iNumChoices, &fcn, "Choosing belief from Top Choices");
	LogBeliefChoices(beliefChoices, rtnValue);
#endif

	return rtnValue;
}

/// Select the belief most helpful to enhance this religion
BeliefTypes CvReligionAI::ChooseBonusBelief(int iExcludeBelief1, int iExcludeBelief2, int iExcludeBelief3)
{
	CvGameReligions* pGameReligions = GC.getGame().GetGameReligions();
	CvWeightedVector<BeliefTypes, SAFE_ESTIMATE_NUM_BELIEFS, true> beliefChoices;

	std::vector<BeliefTypes> availableBeliefs = pGameReligions->GetAvailableBonusBeliefs();

	for(std::vector<BeliefTypes>::iterator it = availableBeliefs.begin();
	        it!= availableBeliefs.end(); ++it)
	{
		const BeliefTypes eBelief = (*it);
		CvBeliefEntry* pEntry = m_pBeliefs->GetEntry(eBelief);
		if(pEntry)
		{
			if (pEntry->GetID() != iExcludeBelief1 && pEntry->GetID() != iExcludeBelief2 && pEntry->GetID() != iExcludeBelief3)
			{
				const int iScore = ScoreBelief(pEntry);
				if(iScore > 0)
				{
					beliefChoices.push_back(eBelief, iScore);
				}
			}
		}
	}

#ifdef AUI_RELIGION_FIX_POSSIBLE_NULL_POINTER
	BeliefTypes rtnValue = NO_BELIEF;
	if (beliefChoices.size() > 0)
	{
		// Choose from weighted vector
		beliefChoices.SortItems();
		int iNumChoices = MIN(beliefChoices.size(), 3);   // this was way too loose as choices way down were being selected now only top 3
#ifdef AUI_RELIGION_RELATIVE_BELIEF_SCORE
		int iToDecrease = -MAX(beliefChoices.GetWeight(beliefChoices.size() - 1) - 1, 0);
		for (int iI = 0; iI < beliefChoices.size(); iI++)
		{
			beliefChoices.IncreaseWeight(iI, iToDecrease);
		}
#endif
		RandomNumberDelegate fcn = MakeDelegate(&GC.getGame(), &CvGame::getJonRandNum);
		rtnValue = beliefChoices.ChooseFromTopChoices(iNumChoices, &fcn, "Choosing belief from Top Choices");
		LogBeliefChoices(beliefChoices, rtnValue);
	}
	else
	{
		rtnValue = availableBeliefs.at(GC.getGame().getJonRandNum(availableBeliefs.size(), "Choosing random Bonus belief."));
	}
#else
	// Choose from weighted vector
	beliefChoices.SortItems();
	int iNumChoices = MIN(beliefChoices.size(),3);   // this was way too loose as choices way down were being selected now only top 3
	RandomNumberDelegate fcn = MakeDelegate(&GC.getGame(), &CvGame::getJonRandNum);
	BeliefTypes rtnValue = beliefChoices.ChooseFromTopChoices(iNumChoices, &fcn, "Choosing belief from Top Choices");
	LogBeliefChoices(beliefChoices, rtnValue);
#endif

	return rtnValue;
}

/// Select the belief most helpful to gain from Reformation social policy
BeliefTypes CvReligionAI::ChooseReformationBelief()
{
	CvGameReligions* pGameReligions = GC.getGame().GetGameReligions();
	CvWeightedVector<BeliefTypes, SAFE_ESTIMATE_NUM_BELIEFS, true> beliefChoices;

	std::vector<BeliefTypes> availableBeliefs = pGameReligions->GetAvailableReformationBeliefs();

	for(std::vector<BeliefTypes>::iterator it = availableBeliefs.begin();
	        it!= availableBeliefs.end(); ++it)
	{
		const BeliefTypes eBelief = (*it);
		CvBeliefEntry* pEntry = m_pBeliefs->GetEntry(eBelief);
		if(pEntry)
		{
			const int iScore = ScoreBelief(pEntry);
			if(iScore > 0)
			{
				beliefChoices.push_back(eBelief, iScore);
			}
		}
	}

#ifdef AUI_RELIGION_FIX_POSSIBLE_NULL_POINTER
	BeliefTypes rtnValue = NO_BELIEF;
	if (beliefChoices.size() > 0)
	{
		// Choose from weighted vector
		beliefChoices.SortItems();
		int iNumChoices = MIN(beliefChoices.size(), 3);   // this was way too loose as choices way down were being selected now only top 3
#ifdef AUI_RELIGION_RELATIVE_BELIEF_SCORE
		int iToDecrease = -MAX(beliefChoices.GetWeight(beliefChoices.size() - 1) - 1, 0);
		for (int iI = 0; iI < beliefChoices.size(); iI++)
		{
			beliefChoices.IncreaseWeight(iI, iToDecrease);
		}
#endif
		RandomNumberDelegate fcn = MakeDelegate(&GC.getGame(), &CvGame::getJonRandNum);
		rtnValue = beliefChoices.ChooseFromTopChoices(iNumChoices, &fcn, "Choosing belief from Top Choices");
		LogBeliefChoices(beliefChoices, rtnValue);
	}
	else
	{
		rtnValue = availableBeliefs.at(GC.getGame().getJonRandNum(availableBeliefs.size(), "Choosing random Reformation belief."));
	}
#else
	// Choose from weighted vector
	beliefChoices.SortItems();
	int iNumChoices = MIN(beliefChoices.size(),3);   // this was way too loose as choices way down were being selected now only top 3
	RandomNumberDelegate fcn = MakeDelegate(&GC.getGame(), &CvGame::getJonRandNum);
	BeliefTypes rtnValue = beliefChoices.ChooseFromTopChoices(iNumChoices, &fcn, "Choosing belief from Top Choices");
	LogBeliefChoices(beliefChoices, rtnValue);
#endif

	return rtnValue;
}

/// Find the city where a missionary should next spread his religion
#ifdef AUI_CONSTIFY
CvCity* CvReligionAI::ChooseMissionaryTargetCity(UnitHandle pUnit) const
#else
CvCity* CvReligionAI::ChooseMissionaryTargetCity(UnitHandle pUnit)
#endif
{
	ReligionTypes eMyReligion = GetReligionToSpread();
	int iBestScore = 0;
	CvCity* pBestCity = NULL;

	if(eMyReligion <= RELIGION_PANTHEON)
	{
		return NULL;
	}

#pragma warning ( push )
#pragma warning ( disable : 6011 ) // Dereferencing NULL pointer
	AI_PERF_FORMAT("AI-perf-tact.csv", ("ChooseMissionaryTargetCity: %s %d, Turn %03d, %s", pUnit->getName().c_str(), pUnit->GetID(), GC.getGame().getElapsedGameTurns(), m_pPlayer->getCivilizationShortDescription()) );
#pragma warning ( pop )

	// Loop through all the players
	for(int iI = 0; iI < MAX_PLAYERS; iI++)
	{
		CvPlayer& kPlayer = GET_PLAYER((PlayerTypes)iI);
		if(kPlayer.isAlive())
		{
			// Loop through each of their cities
			int iLoop;
			CvCity* pLoopCity;
			for(pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
			{
				if(pLoopCity && !pLoopCity->GetCityReligions()->IsDefendedAgainstSpread(eMyReligion))
				{
					int iScore = ScoreCityForMissionary(pLoopCity, pUnit);
					if(iScore > iBestScore)
					{
						iBestScore = iScore;
						pBestCity = pLoopCity;
					}
				}
			}
		}
	}

	return pBestCity;
}

/// Choose a plot next to the target city for the missionary to maneuver to
#ifdef AUI_CONSTIFY
CvPlot* CvReligionAI::ChooseMissionaryTargetPlot(UnitHandle pUnit, int* piTurns) const
#else
CvPlot* CvReligionAI::ChooseMissionaryTargetPlot(UnitHandle pUnit, int* piTurns)
#endif
{
	CvCity* pCity = ChooseMissionaryTargetCity(pUnit);
	int iBestNumTurns = MAX_INT;
	int iTurns;
	int iBestDistance = MAX_INT;
	int iDistance;
	CvPlot* pBestTarget = NULL;

	if(pCity == NULL)
	{
		return NULL;
	}

	// Our city with no civilian units?  If so go right in there
	if(pCity->getOwner() == m_pPlayer->GetID())
	{
		CvUnit* pFirstUnit = pCity->plot()->getUnitByIndex(0);
		if(!pFirstUnit || pFirstUnit->IsCombatUnit())
		{
			iTurns = TurnsToReachTarget(pUnit, pCity->plot(), true /* bReusePaths */);
			if(iTurns < MAX_INT)
			{
				*piTurns = iTurns;
				return pCity->plot();
			}
		}
	}

#pragma warning ( push )
#pragma warning ( disable : 6011 ) // Dereferencing NULL pointer
#ifdef AUI_PERF_LOGGING_FORMATTING_TWEAKS
	AI_PERF_FORMAT("AI-perf-tact.csv", ("ChooseMissionaryTargetPlot: %s %d (Targeting %s), Turn %03d, %s", pUnit->getName().c_str(), pUnit->GetID(), pCity->getName().c_str(), GC.getGame().getElapsedGameTurns(), m_pPlayer->getCivilizationShortDescription()) );
#else
	AI_PERF_FORMAT("AI-perf-tact.csv", ("ChooseMissionaryTargetPlot: %s %d; %s, Turn %03d, %s", pUnit->getName().c_str(), pUnit->GetID(), pCity->getName().c_str(), GC.getGame().getElapsedGameTurns(), m_pPlayer->getCivilizationShortDescription()) );
#endif
#pragma warning ( pop )

	// Find adjacent plot with no units (that aren't our own)
	CvPlot* pLoopPlot;
	for(int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		pLoopPlot = plotDirection(pCity->getX(), pCity->getY(), ((DirectionTypes)iI));

		if(pLoopPlot != NULL)
		{
			CvUnit* pFirstUnit = pLoopPlot->getUnitByIndex(0);
			if(pFirstUnit && pFirstUnit->getOwner() != m_pPlayer->GetID())
			{
				continue;
			}

			if(pUnit->CanSpreadReligion(pLoopPlot))
			{
#ifdef AUI_ASTAR_TURN_LIMITER
				iTurns = TurnsToReachTarget(pUnit, pLoopPlot, true /* bReusePaths */, false, false, iBestNumTurns);
#else
				iTurns = TurnsToReachTarget(pUnit, pLoopPlot, true /* bReusePaths */);
#endif
				if(iTurns < MAX_INT)
				{
					iDistance = plotDistance(pUnit->getX(), pUnit->getY(), pLoopPlot->getX(), pLoopPlot->getY());

					// Consider it to be twice as far if a water plot (those are dangerous!)
					if(pLoopPlot->isWater())
					{
						iDistance *= 2;
					}

					if(iTurns < iBestNumTurns || (iTurns == iBestNumTurns && iDistance < iBestDistance))
					{
						iBestNumTurns = iTurns;
						iBestDistance = iDistance;
						pBestTarget = pLoopPlot;
					}
				}
			}
		}
	}

	if(piTurns)
		*piTurns = iBestNumTurns;
	return pBestTarget;
}

/// Find the city where an inquisitor should next remove heresy
#ifdef AUI_CONSTIFY
CvCity* CvReligionAI::ChooseInquisitorTargetCity(UnitHandle pUnit) const
#else
CvCity* CvReligionAI::ChooseInquisitorTargetCity(UnitHandle pUnit)
#endif
{
	ReligionTypes eMyReligion = GetReligionToSpread();
	int iBestScore = 0;
	CvCity* pBestCity = NULL;

	if(eMyReligion <= RELIGION_PANTHEON)
	{
		return NULL;
	}

#pragma warning ( push )
#pragma warning ( disable : 6011 ) // Dereferencing NULL pointer
	AI_PERF_FORMAT("AI-perf-tact.csv", ("ChooseInquisitorTargetCity: %s %d, Turn %03d, %s", pUnit->getName().c_str(), pUnit->GetID(), GC.getGame().getElapsedGameTurns(), m_pPlayer->getCivilizationShortDescription()) );
#pragma warning ( pop )

	// Loop through each of my cities
	int iLoop;
	CvCity* pLoopCity;
	for(pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
	{
		if(pLoopCity)
		{
			int iScore = ScoreCityForInquisitor(pLoopCity, pUnit);
			if(iScore > iBestScore)
			{
				iBestScore = iScore;
				pBestCity = pLoopCity;
			}
		}
	}

	return pBestCity;
}

/// Choose a plot next to the target city for the inquisitor to maneuver to
#ifdef AUI_CONSTIFY
CvPlot* CvReligionAI::ChooseInquisitorTargetPlot(UnitHandle pUnit, int* piTurns) const
#else
CvPlot* CvReligionAI::ChooseInquisitorTargetPlot(UnitHandle pUnit, int* piTurns)
#endif
{
	CvCity* pCity = ChooseInquisitorTargetCity(pUnit);
	int iBestNumTurns = MAX_INT;
	int iTurns;
	int iBestDistance = MAX_INT;
	int iDistance;
	CvPlot* pBestTarget = NULL;

	if(pCity == NULL)
	{
		return NULL;
	}

	// Our city with no civilian units?  If so go right in there
	if(pCity->getOwner() == m_pPlayer->GetID())
	{
		CvUnit* pFirstUnit = pCity->plot()->getUnitByIndex(0);
		if(!pFirstUnit || pFirstUnit->IsCombatUnit())
		{
			iTurns = TurnsToReachTarget(pUnit, pCity->plot(), true /* bReusePaths */);
			if(iTurns < MAX_INT)
			{
				*piTurns = iTurns;
				return pCity->plot();
			}
		}
	}

#pragma warning ( push )
#pragma warning ( disable : 6011 ) // Dereferencing NULL pointer
	AI_PERF_FORMAT("AI-perf-tact.csv", ("ChooseInquisitorTargetPlot: %s %d; %s, Turn %03d, %s", pUnit->getName().c_str(), pUnit->GetID(), pCity->getName().c_str(), GC.getGame().getElapsedGameTurns(), m_pPlayer->getCivilizationShortDescription()) );
#pragma warning ( pop )

	// Find adjacent plot with no units (that aren't our own)
	CvPlot* pLoopPlot;
	for(int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		pLoopPlot = plotDirection(pCity->getX(), pCity->getY(), ((DirectionTypes)iI));

		if(pLoopPlot != NULL)
		{
			CvUnit* pFirstUnit = pLoopPlot->getUnitByIndex(0);
			if(pFirstUnit && pFirstUnit->getOwner() != m_pPlayer->GetID())
			{
				continue;
			}

			if(pUnit->CanRemoveHeresy(pLoopPlot))
			{
#ifdef AUI_ASTAR_TURN_LIMITER
				iTurns = TurnsToReachTarget(pUnit, pLoopPlot, true /* bReusePaths */, false, false, iBestNumTurns);
#else
				iTurns = TurnsToReachTarget(pUnit, pLoopPlot, true /* bReusePaths */);
#endif
				if(iTurns < MAX_INT)
				{
					iDistance = plotDistance(pUnit->getX(), pUnit->getY(), pLoopPlot->getX(), pLoopPlot->getY());

					// Consider it to be twice as far if a water plot (those are dangerous!)
					if(pLoopPlot->isWater())
					{
						iDistance *= 2;
					}

					if(iTurns < iBestNumTurns || (iTurns == iBestNumTurns && iDistance < iBestDistance))
					{
						iBestNumTurns = iTurns;
						iBestDistance = iDistance;
						pBestTarget = pLoopPlot;
					}
				}
			}
		}
	}

	if(piTurns)
		*piTurns = iBestNumTurns;
	return pBestTarget;
}

/// If we were going to use a prophet to convert a city, which one would it be?
CvCity *CvReligionAI::ChooseProphetConversionCity(bool bOnlyBetterThanEnhancingReligion) const
{
	CvCity *pBestCity = NULL;
	CvCity *pHolyCity = NULL;
	int iBestScore = 50;  // Not zero because we don't want prophets to ALWAYS pick something up

	// Make sure we're spreading a religion and find holy city
	ReligionTypes eReligion = GetReligionToSpread();
	if (eReligion <= RELIGION_PANTHEON)
	{
		return pBestCity;
	}

	const CvReligion* pkReligion = GC.getGame().GetGameReligions()->GetReligion(eReligion, m_pPlayer->GetID());
	if (pkReligion)
	{
		CvPlot* pkPlot = GC.getMap().plot(pkReligion->m_iHolyCityX, pkReligion->m_iHolyCityY);
		if(pkPlot)
		{
			pHolyCity = pkPlot->getPlotCity();
		}
	}
	if (!pHolyCity)
	{
		return pBestCity;
	}
#ifdef AUI_RELIGION_CHOOSE_PROPHET_CONVERSION_CITY_TWEAKED
	else if (pHolyCity->GetCityReligions()->GetReligiousMajority() != eReligion)
	{
		return pHolyCity;
	}
	CvCity* pLoopCity;
#else

	// Look at our cities first, checking them for followers of other religions
	int iLoop;
	CvCity* pLoopCity;
	for(pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
	{
		ReligionTypes eMajorityReligion = pLoopCity->GetCityReligions()->GetReligiousMajority();
		int iHeretics = pLoopCity->GetCityReligions()->GetFollowersOtherReligions(eReligion);
		int iDistanceToHolyCity = plotDistance(pLoopCity->getX(), pLoopCity->getY(), pHolyCity->getX(), pHolyCity->getY());

		// If this is the holy city and it has been converted, want to go there no matter what
		if (pLoopCity == pHolyCity && eMajorityReligion != eReligion)
		{
			return pHolyCity;
		}

		// Otherwise score this city
		int iScore = (iHeretics * 100) / (iDistanceToHolyCity + 1);
		if (eMajorityReligion != eReligion && eMajorityReligion > RELIGION_PANTHEON)
		{
			iScore *= 3;
		}

		// Don't give it any score if our religion hasn't been enhanced yet
		else if (bOnlyBetterThanEnhancingReligion)
		{
			iScore = 0;
		}

		if (iScore > iBestScore)
		{
			iBestScore = iScore;
			pBestCity = pLoopCity;
		}
	}
#endif

	// Now try other players, assuming don't need to enhance religion
	if (!bOnlyBetterThanEnhancingReligion)
	{
		for(int iPlayerLoop = 0; iPlayerLoop < MAX_CIV_PLAYERS; iPlayerLoop++)
		{
			CvPlayer &kLoopPlayer = GET_PLAYER((PlayerTypes)iPlayerLoop);
			if(kLoopPlayer.isAlive() && iPlayerLoop != m_pPlayer->GetID())
			{
				int iCityLoop;
				for(pLoopCity = GET_PLAYER((PlayerTypes)iPlayerLoop).firstCity(&iCityLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER((PlayerTypes)iPlayerLoop).nextCity(&iCityLoop))
				{
					if (!pLoopCity->GetCityReligions()->IsDefendedAgainstSpread(eReligion))
					{
						ReligionTypes eMajorityReligion = pLoopCity->GetCityReligions()->GetReligiousMajority();
#ifdef AUI_RELIGION_CHOOSE_PROPHET_CONVERSION_CITY_TWEAKED
						if (eMajorityReligion == eReligion)
							continue;

						// Trying to convert the holy city of a player that they haven't lost yet is a waste of a prophet
						ReligionTypes eCityOwnersReligion = GET_PLAYER((PlayerTypes)iPlayerLoop).GetReligions()->GetReligionCreatedByPlayer();
						if (eCityOwnersReligion > RELIGION_PANTHEON && pLoopCity->GetCityReligions()->IsHolyCityForReligion(eCityOwnersReligion))
							continue;

						if (IsPressureLeader(pLoopCity, eReligion))
							continue;
#endif
						int iHeretics = pLoopCity->GetCityReligions()->GetFollowersOtherReligions(eReligion);
						int iDistanceToHolyCity = plotDistance(pLoopCity->getX(), pLoopCity->getY(), pHolyCity->getX(), pHolyCity->getY());

						// Score this city
						int iScore = (iHeretics * 50) / (iDistanceToHolyCity + 1);

#ifndef AUI_RELIGION_CHOOSE_PROPHET_CONVERSION_CITY_TWEAKED
						//    - high score if this city has another religion as its majority
						if (eMajorityReligion != eReligion)
						{
							iScore *= 3;
						}

						//    - Holy city will anger folks, let's not do that one right away
						ReligionTypes eCityOwnersReligion = GET_PLAYER((PlayerTypes)iPlayerLoop).GetReligions()->GetReligionCreatedByPlayer();
						if (eCityOwnersReligion > RELIGION_PANTHEON && pLoopCity->GetCityReligions()->IsHolyCityForReligion(eCityOwnersReligion))
						{
							iScore /= 2;
						}
#endif

						//    - City not owned by religion founder, won't anger folks as much
						const CvReligion* pkMajorityReligion = GC.getGame().GetGameReligions()->GetReligion(eMajorityReligion, NO_PLAYER);
						if (pkMajorityReligion && pkMajorityReligion->m_eFounder != pLoopCity->getOwner())
						{
							iScore *= 2;
						}

						if (iScore > iBestScore)
						{
							iBestScore = iScore;
							pBestCity = pLoopCity;
						}
					}
				}
			}
		}
	}

	return pBestCity;
}

/// Choose a plot next to the target city for the prophet to maneuver to
#ifdef AUI_CONSTIFY
CvPlot* CvReligionAI::ChooseProphetTargetPlot(UnitHandle pUnit, int* piTurns) const
#else
CvPlot* CvReligionAI::ChooseProphetTargetPlot(UnitHandle pUnit, int* piTurns)
#endif
{
	CvCity* pCity = ChooseProphetConversionCity(false/*bOnlyBetterThanEnhancingReligion*/);
	int iBestNumTurns = MAX_INT;
	int iTurns;
	int iBestDistance = MAX_INT;
	int iDistance;
	CvPlot* pBestTarget = NULL;

	if(pCity == NULL)
	{
		return NULL;
	}

	// Our city with no civilian units?  If so go right in there
	if(pCity->getOwner() == m_pPlayer->GetID())
	{
		CvUnit* pFirstUnit = pCity->plot()->getUnitByIndex(0);
		if(!pFirstUnit || pFirstUnit->IsCombatUnit())
		{
			iTurns = TurnsToReachTarget(pUnit, pCity->plot(), true /* bReusePaths */);
			if(iTurns < MAX_INT)
			{
				*piTurns = iTurns;
				return pCity->plot();
			}
		}
	}

#pragma warning ( push )
#pragma warning ( disable : 6011 ) // Dereferencing NULL pointer
	AI_PERF_FORMAT("AI-perf-tact.csv", ("ChooseProphetTargetPlot: %s %d; %s, Turn %03d, %s", pUnit->getName().c_str(), pUnit->GetID(), pCity->getName().c_str(), GC.getGame().getElapsedGameTurns(), m_pPlayer->getCivilizationShortDescription()) );
#pragma warning ( pop )

	// Find adjacent plot with no units (that aren't our own)
	CvPlot* pLoopPlot;
	for(int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
	{
		pLoopPlot = plotDirection(pCity->getX(), pCity->getY(), ((DirectionTypes)iI));

		if(pLoopPlot != NULL)
		{
			CvUnit* pFirstUnit = pLoopPlot->getUnitByIndex(0);
			if(pFirstUnit && pFirstUnit->getOwner() != m_pPlayer->GetID())
			{
				continue;
			}

			if(pUnit->CanSpreadReligion(pLoopPlot))
			{
#ifdef AUI_ASTAR_TURN_LIMITER
				iTurns = TurnsToReachTarget(pUnit, pLoopPlot, true /* bReusePaths */, false, false, iBestNumTurns);
#else
				iTurns = TurnsToReachTarget(pUnit, pLoopPlot, true /* bReusePaths */);
#endif
				if(iTurns < MAX_INT)
				{
					iDistance = plotDistance(pUnit->getX(), pUnit->getY(), pLoopPlot->getX(), pLoopPlot->getY());

					// Consider it to be twice as far if a water plot (those are dangerous!)
					if(pLoopPlot->isWater())
					{
						iDistance *= 2;
					}

					if(iTurns < iBestNumTurns || (iTurns == iBestNumTurns && iDistance < iBestDistance))
					{
						iBestNumTurns = iTurns;
						iBestDistance = iDistance;
						pBestTarget = pLoopPlot;
					}
				}
			}
		}
	}

	if(piTurns)
		*piTurns = iBestNumTurns;
	return pBestTarget;
}

/// What religion should this AI civ be spreading?
ReligionTypes CvReligionAI::GetReligionToSpread() const
{
	ReligionTypes eRtnValue = NO_RELIGION;

	ICvEngineScriptSystem1* pkScriptSystem = gDLL->GetScriptSystem();
	if(pkScriptSystem)
	{
		CvLuaArgsHandle args;
		args->Push(m_pPlayer->GetID());

		int iValue = 0;
		if (LuaSupport::CallAccumulator(pkScriptSystem, "GetReligionToSpread", args.get(), iValue))
		{
			eRtnValue = (ReligionTypes)iValue;
			return eRtnValue;
		}
	}

	CvGameReligions *pReligions = GC.getGame().GetGameReligions();
	eRtnValue = pReligions->GetReligionCreatedByPlayer(m_pPlayer->GetID());
	if(eRtnValue > RELIGION_PANTHEON)
	{
		return eRtnValue;
	}

	return NO_RELIGION;
}

// PRIVATE METHODS

/// Spend faith if already have an enhanced religion
void CvReligionAI::DoFaithPurchases()
{
	ReligionTypes eReligion = GetReligionToSpread();

	CvGameReligions* pReligions = GC.getGame().GetGameReligions();
	const CvReligion* pMyReligion = pReligions->GetReligion(eReligion, m_pPlayer->GetID());
#ifdef AUI_RELIGION_FIX_MULTIPLE_FAITH_BUILDINGS
	std::vector<BuildingClassTypes> eFaithBuilding = FaithBuildingAvailable(eReligion);
#else
	BuildingClassTypes eFaithBuilding = FaithBuildingAvailable(eReligion);
#endif
	bool bTooManyMissionaries = m_pPlayer->GetNumUnitsWithUnitAI(UNITAI_MISSIONARY) > GC.getRELIGION_MAX_MISSIONARIES();

	CvString strLogMsg;
	if(GC.getLogging())
	{
		strLogMsg = m_pPlayer->getCivilizationShortDescription();
	}

	// Save for another prophet if haven't enhanced religion yet
	if(pMyReligion == NULL || !pMyReligion->m_bEnhanced)
	{
		// Unless all religions gone and we didn't start one
#ifdef AUI_RELIGION_FIX_DO_FAITH_PURCHASES_ENHANCE_INDUSTRIAL_RELIGION
		bool bIndustrialEnhance = (pMyReligion && !pMyReligion->m_bEnhanced && m_pPlayer->GetCurrentEra() >= GC.getInfoTypeForString("ERA_INDUSTRIAL"));
		if ((pMyReligion == NULL && pReligions->GetNumReligionsStillToFound() <= 0) || bIndustrialEnhance)
		{
			// Fill our cities with any Faith buildings possible
			if (bIndustrialEnhance || !BuyAnyAvailableFaithBuilding())
#else
		if(pMyReligion == NULL && pReligions->GetNumReligionsStillToFound() <= 0)
		{
			// Fill our cities with any Faith buildings possible
			if(!BuyAnyAvailableFaithBuilding())
#endif
			{
#ifdef AUI_RELIGION_FIX_DO_FAITH_PURCHASES_DO_HURRY_WITH_FAITH
				if (DoHurryWithFaith())
				{
					if (GC.getLogging())
					{
						strLogMsg += ", Hurried With Faith";
					}
				}
				else if (m_pPlayer->GetCurrentEra() >= GC.getInfoTypeForString("ERA_INDUSTRIAL"))
#else
				if(m_pPlayer->GetCurrentEra() >= GC.getInfoTypeForString("ERA_INDUSTRIAL"))
#endif
				{
					UnitTypes eGPType = GetDesiredFaithGreatPerson();
					if (eGPType != NO_UNIT)
					{
						BuyGreatPerson(eGPType);

						if(GC.getLogging())
						{
							strLogMsg += ", Saving for Great Person, ";
							CvUnitEntry *pkEntry = GC.getUnitInfo(eGPType);
							if (pkEntry)
							{
								strLogMsg += pkEntry->GetDescription();
							}
						}				
					}
				}
			}
		}
	}
	else
	{
		// Do we need a prophet pronto to reestablish our religion?
		UnitTypes eProphetType = (UnitTypes)GC.getInfoTypeForString("UNIT_PROPHET", true);
		if (eProphetType != NO_UNIT && ChooseProphetConversionCity(true/*bOnlyBetterThanEnhancingReligion*/) && m_pPlayer->GetReligions()->GetNumProphetsSpawned() <= 5)
		{
			BuyGreatPerson(eProphetType);

			if(GC.getLogging())
			{
				strLogMsg += ", Saving for Prophet, ";
				strLogMsg += GC.getUnitInfo(eProphetType)->GetDescription();
			}				
		}
#ifdef AUI_RELIGION_DO_FAITH_PURCHASES_PRIORITIZE_OTHER_RELIGION_HAPPINESS_BUILDINGS
		// Next, purchase happiness buildings that other religions have made available to us, since we'll still want the building after the city has converted
		else if (CanBuyOtherReligionHappinessBuilding(eReligion))
		{
			if (!BuyOtherReligionHappinessBuilding(eReligion))
			{
				if (GC.getLogging())
				{
					strLogMsg += ", Saving for Happiness Building, From Another Religion";
				}
			}
		}
#endif
#ifdef AUI_RELIGION_FIX_DO_FAITH_PURCHASES_DO_HURRY_WITH_FAITH
		// Hurry with faith at cities with a religion other than our own, so we can make use of another civ's faith purchase beliefs
		else if (DoHurryWithFaith(eReligion))
		{
			if (GC.getLogging())
			{
				strLogMsg += ", Hurried with Faith, From Another Religion";
			}
		}
#endif

		// Besides prophets, first priority is to convert all our non-puppet cities
		else if(!bTooManyMissionaries && !AreAllOurCitiesConverted(eReligion, false /*bIncludePuppets*/))
		{
			BuyMissionary(eReligion);

			if(GC.getLogging())
			{
				strLogMsg += ", Saving for Missionary, Need to Convert Non-Puppet Cities";
			}
		}

		// Next priority is to establish our faith building in every non-puppet city
#ifdef AUI_RELIGION_FIX_MULTIPLE_FAITH_BUILDINGS
		else if (eFaithBuilding.size() > 0 && !AreAllOurCitiesHaveFaithBuilding(eReligion, false /*bIncludePuppets*/))
		{
			for (std::vector<BuildingClassTypes>::iterator it = eFaithBuilding.begin(); it != eFaithBuilding.end(); ++it)
			{
				BuyFaithBuilding(eReligion, (*it));
			}
#else
		else if(eFaithBuilding != NO_BUILDINGCLASS && !AreAllOurCitiesHaveFaithBuilding(eReligion, false /*bIncludePuppets*/))
		{
			BuyFaithBuilding(eReligion, eFaithBuilding);
#endif

			if(GC.getLogging())
			{
				strLogMsg += ", Saving for Faith Building, For Our Non-Puppet Cities";
			}
		}
#ifdef AUI_RELIGION_FIX_DO_FAITH_PURCHASES_DO_HURRY_WITH_FAITH
		// Hurry with faith at all cities
		else if (DoHurryWithFaith())
		{
			if (GC.getLogging())
			{
				strLogMsg += ", Hurried with Faith, From Our Religion";
			}
		}
#endif

		// Try to build other buildings with Faith if we took that belief
		else if (CanBuyNonFaithBuilding())
		{
			if (!BuyAnyAvailableNonFaithBuilding())
			{
				if(GC.getLogging())
				{
					strLogMsg += ", Saving for Science Building, Have Reformation Belief";
				}
			}
		}

		// If in Industrial, see if we want to save for buying a great person
		else if (m_pPlayer->GetCurrentEra() >= GC.getInfoTypeForString("ERA_INDUSTRIAL") && GetDesiredFaithGreatPerson() != NO_UNIT)
		{
			UnitTypes eGPType = GetDesiredFaithGreatPerson();
			BuyGreatPerson(eGPType);

			if(GC.getLogging())
			{
				strLogMsg += ", Saving for Great Person, ";
#ifdef AUI_WARNING_FIXES
				CvUnitEntry* pGPInfo = GC.getUnitInfo(eGPType);
				if (pGPInfo)
					strLogMsg += pGPInfo->GetDescription();
#else
				strLogMsg += GC.getUnitInfo(eGPType)->GetDescription();
#endif
			}				
		}

		// Might as well convert puppet-cities to build our religious strength
#ifdef AUI_RELIGION_FIX_MULTIPLE_FAITH_BUILDINGS
		else if (!bTooManyMissionaries && eFaithBuilding.size() > 0 && !AreAllOurCitiesHaveFaithBuilding(eReligion, true /*bIncludePuppets*/))
#else
		else if(!bTooManyMissionaries && eFaithBuilding != NO_BUILDINGCLASS && !AreAllOurCitiesHaveFaithBuilding(eReligion, true /*bIncludePuppets*/))
#endif
		{
			BuyMissionary(eReligion);

			if(GC.getLogging())
			{
				strLogMsg += ", Saving for Missionary, Need to Convert Puppet Cities";
			}
		}

		// Have civs nearby to target who didn't start a religion?
		else if(!bTooManyMissionaries && HaveNearbyConversionTarget(eReligion, false /*bCanIncludeReligionStarter*/))
		{
			BuyMissionary(eReligion);

			if(GC.getLogging())
			{
				strLogMsg += ", Saving for Missionary, Need to Convert Cities of Non-Religion Starters";
			}
		}

		// Have cities Inquisitors can defend?
		else if(!HaveEnoughInquisitors(eReligion))
		{
			BuyInquisitor(eReligion);

			if(GC.getLogging())
			{
				strLogMsg += ", Saving for Inquisitors, Need to Defend Our Cities";
			}
		}

		// Have civs nearby to target who did start a religion?
		else if(!bTooManyMissionaries && HaveNearbyConversionTarget(eReligion, true /*bCanIncludeReligionStarter*/))
		{
			BuyMissionary(eReligion);

			if(GC.getLogging())
			{
				strLogMsg += ", Saving for Missionary, Need to Convert Cities of Religion Starters";
			}
		}

		// Any faith buildings from other religions we can buy?
		else
		{
			BuyAnyAvailableFaithBuilding();
		}

		if(GC.getLogging())
		{
			CvString strFaith;
			strFaith.Format(", Faith: %d", m_pPlayer->GetFaith());
			strLogMsg += strFaith;
			GC.getGame().GetGameReligions()->LogReligionMessage(strLogMsg);
		}
	}
}

/// Pick the right city to purchase a missionary in
void CvReligionAI::BuyMissionary(ReligionTypes eReligion)
{
	CvPlayer &kPlayer = GET_PLAYER(m_pPlayer->GetID());
	UnitTypes eMissionary = (UnitTypes)GC.getInfoTypeForString("UNIT_MISSIONARY");
	CvCity *pCapital = kPlayer.getCapitalCity();
	if (pCapital)
	{
		int iCost = pCapital->GetFaithPurchaseCost(eMissionary, true /*bIncludeBeliefDiscounts*/);
		if (iCost <= kPlayer.GetFaith())
		{
			CvCity *pBestCity = CvReligionAIHelpers::GetBestCityFaithUnitPurchase(kPlayer, eMissionary, eReligion);
			if (pBestCity)
			{
				pBestCity->Purchase(eMissionary, (BuildingTypes)-1, (ProjectTypes)-1, YIELD_FAITH);
			}
		}
	}
}

/// Pick the right city to purchase an inquisitor in
void CvReligionAI::BuyInquisitor(ReligionTypes eReligion)
{
	CvPlayer &kPlayer = GET_PLAYER(m_pPlayer->GetID());
	UnitTypes eInquisitor = (UnitTypes)GC.getInfoTypeForString("UNIT_INQUISITOR");
	CvCity *pCapital = kPlayer.getCapitalCity();
	if (pCapital)
	{
		int iCost = pCapital->GetFaithPurchaseCost(eInquisitor, true /*bIncludeBeliefDiscounts*/);
		if (iCost <= kPlayer.GetFaith())
		{
			CvCity *pBestCity = CvReligionAIHelpers::GetBestCityFaithUnitPurchase(kPlayer, eInquisitor, eReligion);
			if (pBestCity)
			{
				pBestCity->Purchase(eInquisitor, (BuildingTypes)-1, (ProjectTypes)-1, YIELD_FAITH);
			}
		}
	}
}

/// Pick the right city to purchase a great person in
void CvReligionAI::BuyGreatPerson(UnitTypes eUnit)
{
	CvPlayer &kPlayer = GET_PLAYER(m_pPlayer->GetID());
	CvCity *pCapital = kPlayer.getCapitalCity();
	if (pCapital)
	{
		int iCost = pCapital->GetFaithPurchaseCost(eUnit, true /*bIncludeBeliefDiscounts*/);
		if (iCost <= kPlayer.GetFaith())
		{
			CvCity *pBestCity = CvReligionAIHelpers::GetBestCityFaithUnitPurchase(kPlayer, eUnit, NO_RELIGION);
			if (pBestCity)
			{
				pBestCity->Purchase(eUnit, (BuildingTypes)-1, (ProjectTypes)-1, YIELD_FAITH);
			}
		}
	}
}

/// Pick the right city to purchase a faith building in
void CvReligionAI::BuyFaithBuilding(ReligionTypes eReligion, BuildingClassTypes eBuildingClass)
{
	CvPlayer &kPlayer = GET_PLAYER(m_pPlayer->GetID());
	BuildingTypes eBuilding = (BuildingTypes)m_pPlayer->getCivilizationInfo().getCivilizationBuildings(eBuildingClass);
	CvCity *pCapital = kPlayer.getCapitalCity();
	if (pCapital)
	{
		int iCost = pCapital->GetFaithPurchaseCost(eBuilding);
		if (iCost <= kPlayer.GetFaith())
		{
			CvCity *pBestCity = CvReligionAIHelpers::GetBestCityFaithBuildingPurchase(kPlayer, eBuilding, eReligion);
			if (pBestCity)
			{
				pBestCity->Purchase((UnitTypes)-1, eBuilding, (ProjectTypes)-1, YIELD_FAITH);
			}
		}
	}
}

/// Any building that we can build with Faith (not Faith-generating ones)
bool CvReligionAI::BuyAnyAvailableNonFaithBuilding()
{
	PlayerTypes ePlayer = m_pPlayer->GetID();

	int iLoop;
	CvCity* pLoopCity;
	for(pLoopCity = GET_PLAYER(ePlayer).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(ePlayer).nextCity(&iLoop))
	{
#ifdef AUI_WARNING_FIXES
		for (uint iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
#else
		for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
#endif
		{
			BuildingTypes eBuilding = (BuildingTypes)m_pPlayer->getCivilizationInfo().getCivilizationBuildings(iI);
			if(eBuilding != NO_BUILDING)
			{
				CvBuildingEntry* pBuildingEntry = GC.GetGameBuildings()->GetEntry(eBuilding);

				// Check to make sure this isn't a Faith-generating building
				if(pBuildingEntry && pBuildingEntry->GetYieldChange(YIELD_FAITH) == 0)
				{
					if(pLoopCity->IsCanPurchase(true, true, (UnitTypes)-1, eBuilding, (ProjectTypes)-1, YIELD_FAITH))
					{
						pLoopCity->Purchase((UnitTypes)-1, eBuilding, (ProjectTypes)-1, YIELD_FAITH);
						return true;
					}
				}
			}
		}
	}
	return false;
}

/// We didn't start a religion but we can still buy Faith buildings of other religions
bool CvReligionAI::BuyAnyAvailableFaithBuilding()
{
	PlayerTypes ePlayer = m_pPlayer->GetID();

	int iLoop;
	CvCity* pLoopCity;
	for(pLoopCity = GET_PLAYER(ePlayer).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(ePlayer).nextCity(&iLoop))
	{
		ReligionTypes eReligion = pLoopCity->GetCityReligions()->GetReligiousMajority();
		if(eReligion > RELIGION_PANTHEON)
		{
#ifdef AUI_RELIGION_FIX_MULTIPLE_FAITH_BUILDINGS
			std::vector<BuildingClassTypes> eBuildingClass = FaithBuildingAvailable(eReligion);
			if(eBuildingClass.size() > 0)
			{
				for (std::vector<BuildingClassTypes>::iterator it = eBuildingClass.begin(); it != eBuildingClass.end(); ++it)
				{
					BuildingTypes eBuilding = (BuildingTypes)m_pPlayer->getCivilizationInfo().getCivilizationBuildings((*it));
#else
			BuildingClassTypes eBuildingClass = FaithBuildingAvailable(eReligion);
			if(eBuildingClass != NO_BUILDINGCLASS)
			{
				BuildingTypes eBuilding = (BuildingTypes)m_pPlayer->getCivilizationInfo().getCivilizationBuildings(eBuildingClass);
#endif
				if(eBuilding != NO_BUILDING)
				{
					if(pLoopCity->IsCanPurchase(true, true, (UnitTypes)-1, eBuilding, (ProjectTypes)-1, YIELD_FAITH))
					{
						pLoopCity->Purchase((UnitTypes)-1, eBuilding, (ProjectTypes)-1, YIELD_FAITH);
						return true;
					}
				}
#ifdef AUI_RELIGION_FIX_MULTIPLE_FAITH_BUILDINGS
				}
#endif
			}
		}
	}
	return false;
}

#ifdef AUI_RELIGION_DO_FAITH_PURCHASES_PRIORITIZE_OTHER_RELIGION_HAPPINESS_BUILDINGS
/// Purchase buildings with faith that provide happiness and are unlocked by a religion other than our own
bool CvReligionAI::BuyOtherReligionHappinessBuilding(ReligionTypes eAvoidReligion)
{
	PlayerTypes ePlayer = m_pPlayer->GetID();

	int iLoop;
	CvCity* pLoopCity;
	for (pLoopCity = GET_PLAYER(ePlayer).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(ePlayer).nextCity(&iLoop))
	{
		if (pLoopCity->GetCityReligions()->GetReligiousMajority() != eAvoidReligion)
		{
#ifdef AUI_WARNING_FIXES
			for (uint iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
#else
			for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
#endif
			{
				BuildingTypes eBuilding = (BuildingTypes)m_pPlayer->getCivilizationInfo().getCivilizationBuildings(iI);
				if (eBuilding != NO_BUILDING)
				{
					CvBuildingEntry* pBuildingEntry = GC.GetGameBuildings()->GetEntry(eBuilding);

					// Check to make sure this is a happiness-generating building
					if (pBuildingEntry && pBuildingEntry->GetHappiness() + pBuildingEntry->GetHappinessPerCity() + pBuildingEntry->GetHappinessPerXPolicies() > 0)
					{
						if (pLoopCity->IsCanPurchase(true, true, (UnitTypes)-1, eBuilding, (ProjectTypes)-1, YIELD_FAITH))
						{
							pLoopCity->Purchase((UnitTypes)-1, eBuilding, (ProjectTypes)-1, YIELD_FAITH);
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}

bool CvReligionAI::CanBuyOtherReligionHappinessBuilding(ReligionTypes eAvoidReligion)
{
	PlayerTypes ePlayer = m_pPlayer->GetID();

	int iLoop;
	CvCity* pLoopCity;
	for (pLoopCity = GET_PLAYER(ePlayer).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(ePlayer).nextCity(&iLoop))
	{
		if (pLoopCity->GetCityReligions()->GetReligiousMajority() != eAvoidReligion)
		{
#ifdef AUI_WARNING_FIXES
			for (uint iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
#else
			for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
#endif
			{
				BuildingTypes eBuilding = (BuildingTypes)m_pPlayer->getCivilizationInfo().getCivilizationBuildings(iI);
				if (eBuilding != NO_BUILDING)
				{
					CvBuildingEntry* pBuildingEntry = GC.GetGameBuildings()->GetEntry(eBuilding);

					// Check to make sure this is a happiness-generating building
					if (pBuildingEntry && pBuildingEntry->GetHappiness() + pBuildingEntry->GetHappinessPerCity() + pBuildingEntry->GetHappinessPerXPolicies() > 0)
					{
						if (pLoopCity->IsCanPurchase(true, true, (UnitTypes)-1, eBuilding, (ProjectTypes)-1, YIELD_FAITH))
						{
							return true;
						}
					}
				}
			}
		}
	}

	return false;
}
#endif

#ifdef AUI_RELIGION_FIX_DO_FAITH_PURCHASES_DO_HURRY_WITH_FAITH
/// See if we want to finish any of our builds by rushing with faith (similar to DoHurry() in CvEconomicAI)
bool CvReligionAI::DoHurryWithFaith(ReligionTypes eAvoidReligion)
{
	int iLoop = 0;
	OrderData* pOrder = 0;
	CvCity* pLoopCity = 0;
	double dTurnsSaved = 0;
	int iFaithCost = 0;
	double dFaithPerTurnHammers = 0;
	CvCity* pBestHurryCity = NULL;
	OrderData* pBestHurryOrder = NULL;
	double dBestFaithPerTurnHammers = MAX_INT;
	// Look at each of our cities
	for (pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
	{
		if (pLoopCity->GetCityReligions()->GetReligiousMajority() == eAvoidReligion)
			continue;

		pLoopCity->GetCityStrategyAI()->ConstructRushList(YIELD_FAITH);

		// Loop through all orders in the city
		for (int iIndex = 0; iIndex < pLoopCity->GetCityStrategyAI()->GetRushListLength(); iIndex++)
		{
			// What are we currently looking at?
			pOrder = pLoopCity->GetCityStrategyAI()->GetRushListItem(iIndex);

			// Can we rush it?
			if (pOrder && pLoopCity->IsCanPurchase(pOrder, YIELD_FAITH))
			{
				int iProductionNeeded = MAX_INT;
				switch (pOrder->eOrderType)
				{
				case ORDER_TRAIN:
				{
					UnitTypes eUnit = (UnitTypes)pOrder->iData1;
					CvUnitEntry* pUnitEntry = GC.GetGameUnits()->GetEntry(eUnit);
					if (!pUnitEntry || pUnitEntry->GetProductionCost() > 0)
						iProductionNeeded = m_pPlayer->getProductionNeeded(eUnit) * 100 - pLoopCity->getUnitProductionTimes100(eUnit);
				}
				break;
				case ORDER_CONSTRUCT:
				{
					BuildingTypes eBuilding = (BuildingTypes)pOrder->iData1;
					CvBuildingEntry* pBuildingEntry = GC.GetGameBuildings()->GetEntry(eBuilding);
					if (!pBuildingEntry || pBuildingEntry->GetProductionCost() > 0)
						iProductionNeeded = m_pPlayer->getProductionNeeded(eBuilding)*100 - pLoopCity->GetCityBuildings()->GetBuildingProductionTimes100(eBuilding);
				}
				break;
				case ORDER_CREATE:
				{
					ProjectTypes eProject = (ProjectTypes)pOrder->iData1;
					CvProjectEntry* pProjectEntry = GC.GetGameProjects()->GetEntry(eProject);
					if (!pProjectEntry || pProjectEntry->GetProductionCost() > 0)
						iProductionNeeded = m_pPlayer->getProductionNeeded(eProject) * 100 - pLoopCity->getProjectProductionTimes100(eProject);
				}
				break;
				case ORDER_PREPARE:
				{
					SpecialistTypes eSpecialist = (SpecialistTypes)pOrder->iData1;
					CvSpecialistInfo* pSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
					if (!pSpecialistInfo || pSpecialistInfo->getCost() > 0)
						iProductionNeeded = m_pPlayer->getProductionNeeded(eSpecialist) * 100 - pLoopCity->getSpecialistProductionTimes100(eSpecialist);
				}
				break;
				}
				iFaithCost = pLoopCity->GetPurchaseCost(pOrder, YIELD_FAITH);
				if (iProductionNeeded < MAX_INT)
					dTurnsSaved = (double)iProductionNeeded / pLoopCity->getRawProductionDifferenceTimes100(true, false);
				else
					dTurnsSaved = MAX_INT;
				dFaithPerTurnHammers = iFaithCost / dTurnsSaved;
				if (dFaithPerTurnHammers < dBestFaithPerTurnHammers)
				{
					dBestFaithPerTurnHammers = dFaithPerTurnHammers;
					pBestHurryCity = pLoopCity;
					pBestHurryOrder = pOrder;
					if (GC.getLogging() && GC.getAILogging())
					{
						static const char* orderTypeStrings[] = { "ORDER_TRAIN", "ORDER_CONSTRUCT", "ORDER_CREATE", "ORDER_PREPARE", "ORDER_MAINTAIN", "NO_ORDER" };
						int orderIndex = ((pOrder->eOrderType < 0) || (pOrder->eOrderType > 4)) ? 5 : pOrder->eOrderType;
						CvString strLogString;
						strLogString.Format("DoHurry Option: order type %s, Turns Saved: %d, Gold Spent Per Turn of Production: %d", orderTypeStrings[orderIndex], dTurnsSaved, dBestFaithPerTurnHammers);
						m_pPlayer->GetHomelandAI()->LogHomelandMessage(strLogString);
					}
				}
			}
		}
	}

	// Now enact the best hurry we've found (only hurry one item per turn for now)
	if (pBestHurryCity != NULL)
	{
		pBestHurryCity->PurchaseOrder(pBestHurryOrder, YIELD_FAITH);
		if (pBestHurryCity->getOrderQueueLength() == 0)
		{
			pBestHurryCity->AI_chooseProduction(false);
		}
		return true;
	}
	else
	{
		return false;
	}
}
#endif

/// AI's perceived worth of a belief
#if defined(AUI_CONSTIFY) || defined(AUI_RELIGION_SCORE_BELIEF_AT_PLOT_REMADE) || defined(AUI_RELIGION_SCORE_BELIEF_AT_CITY_REMADE) || defined(AUI_RELIGION_USE_DOUBLES)
int CvReligionAI::ScoreBelief(const CvBeliefEntry* pEntry) const
#else
int CvReligionAI::ScoreBelief(CvBeliefEntry* pEntry)
#endif
{
#ifdef AUI_RELIGION_USE_DOUBLES
	double dRtnValue = 5.0;  // Base value since everything has SOME value
#else
	int iRtnValue = 5;  // Base value since everything has SOME value
#endif

	// Loop through each plot on map
#ifdef AUI_WARNING_FIXES
	CvPlot* pPlot;
	for (uint iPlotLoop = 0; iPlotLoop < GC.getMap().numPlots(); iPlotLoop++)
#else
	int iPlotLoop;
	CvPlot* pPlot;
	for(iPlotLoop = 0; iPlotLoop < GC.getMap().numPlots(); iPlotLoop++)
#endif
	{
		pPlot = GC.getMap().plotByIndexUnchecked(iPlotLoop);

		// Skip if not revealed or in enemy territory
		PlayerTypes ePlotOwner = pPlot->getOwner();
		if(pPlot->isRevealed(m_pPlayer->getTeam()) && (ePlotOwner == NO_PLAYER || ePlotOwner == m_pPlayer->GetID()))
		{
#ifdef AUI_RELIGION_SCORE_BELIEF_SCALE_PLOTS_WITH_DISTANCE
			// Also skip if closest city of ours is not within 4 (from 3, to help expansion stuff)
			CvCity* pClosestCity = m_pPlayer->GetClosestFriendlyCity(*pPlot, AUI_RELIGION_SCORE_BELIEF_SCALE_PLOTS_WITH_DISTANCE);
#else
			// Also skip if closest city of ours is not within 3
			CvCity* pClosestCity = m_pPlayer->GetClosestFriendlyCity(*pPlot, 3);
#endif
			if(pClosestCity)
			{
				// Score it
#ifdef AUI_RELIGION_SCORE_BELIEF_AT_PLOT_REMADE
#ifdef AUI_RELIGION_USE_DOUBLES
				dRtnValue += ScoreBeliefAtPlot(pEntry, pPlot, pClosestCity);
#else
				iRtnValue += int(ScoreBeliefAtPlot(pEntry, pPlot, pClosestCity) + 0.5);
#endif
#else
#ifdef AUI_RELIGION_USE_DOUBLES
				double dScoreAtPlot = ScoreBeliefAtPlot(pEntry, pPlot);
#else
				int iScoreAtPlot = ScoreBeliefAtPlot(pEntry, pPlot);
#endif
#ifdef AUI_RELIGION_SCORE_BELIEF_SCALE_PLOTS_WITH_DISTANCE
				CvCity* pLoopCity = m_pPlayer->GetClosestFriendlyCity(*pPlot, 3);
				if (pLoopCity)
				{
#ifdef AUI_RELIGION_USE_DOUBLES
					dScoreAtPlot *= AUI_RELIGION_SCORE_BELIEF_SCALE_PLOTS_WITH_DISTANCE;
#else
					iScoreAtPlot *= 3;
#endif
				}
#ifdef AUI_RELIGION_USE_DOUBLES
				dScoreAtPlot /= AUI_RELIGION_SCORE_BELIEF_SCALE_PLOTS_WITH_DISTANCE;
#else
				iScoreAtPlot /= 3;
#endif
#endif

				// Apply multiplier based on whether or not being worked, within culture borders, or not
				if(pPlot->isBeingWorked())
				{
#ifdef AUI_RELIGION_USE_DOUBLES
					dScoreAtPlot *= GC.getRELIGION_BELIEF_SCORE_WORKED_PLOT_MULTIPLIER();
#else
					iScoreAtPlot *= GC.getRELIGION_BELIEF_SCORE_WORKED_PLOT_MULTIPLIER();
#endif
				}
				else if(ePlotOwner != NO_PLAYER)
				{
#ifdef AUI_RELIGION_USE_DOUBLES
					dScoreAtPlot *= GC.getRELIGION_BELIEF_SCORE_OWNED_PLOT_MULTIPLIER();
#else
					iScoreAtPlot *= GC.getRELIGION_BELIEF_SCORE_OWNED_PLOT_MULTIPLIER();
#endif
				}
				else
				{
#ifdef AUI_RELIGION_USE_DOUBLES
					dScoreAtPlot *= GC.getRELIGION_BELIEF_SCORE_UNOWNED_PLOT_MULTIPLIER();
#else
					iScoreAtPlot *= GC.getRELIGION_BELIEF_SCORE_UNOWNED_PLOT_MULTIPLIER();
#endif
				}

#ifdef AUI_RELIGION_USE_DOUBLES
				dRtnValue += dScoreAtPlot;
#else
				iRtnValue += iScoreAtPlot;
#endif
#endif
			}
		}
	}

	// Add in value at city level
	int iLoop;
	CvCity* pLoopCity;
	for(pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
	{
#ifdef AUI_RELIGION_USE_DOUBLES
		double dScoreAtCity = ScoreBeliefAtCity(pEntry, pLoopCity);
		dScoreAtCity *= GC.getRELIGION_BELIEF_SCORE_CITY_MULTIPLIER();

		dRtnValue += dScoreAtCity;
#else
		int iScoreAtCity = ScoreBeliefAtCity(pEntry, pLoopCity);
		iScoreAtCity *= GC.getRELIGION_BELIEF_SCORE_CITY_MULTIPLIER();

		iRtnValue += iScoreAtCity;
#endif
	}

	// Add in player-level value
#ifdef AUI_RELIGION_USE_DOUBLES
	dRtnValue += ScoreBeliefForPlayer(pEntry);
#else
	iRtnValue += ScoreBeliefForPlayer(pEntry);
#endif

	// Randomizer is 3rd degree polynomial, so large changes are unlikely
#ifdef AUI_BINOM_RNG
	dRtnValue *= 1 + pow((double)GC.getGame().getJonRandNumBinom(257, "Belief score randomizer.") / 128.0 - 1.0, 3.0);
#else
	dRtnValue *= 1 + pow((double)GC.getGame().getJonRandNum(257, "Belief score randomizer.") / 128.0 - 1.0, 3.0);
#endif

	// Divide by 2 if a Pantheon belief (to deemphasize these to Byzantine bonus belief)
	if (pEntry->IsPantheonBelief())
	{
#ifdef AUI_RELIGION_USE_DOUBLES
#ifdef AUI_RELIGION_SCORE_BELIEF_TWEAK_PANTHEON_DIVIDER
		dRtnValue /= AUI_RELIGION_SCORE_BELIEF_TWEAK_PANTHEON_DIVIDER;
#else
		dRtnValue /= 2.0;
#endif
#else
#ifdef AUI_RELIGION_SCORE_BELIEF_TWEAK_PANTHEON_DIVIDER
		iRtnValue = int(iRtnValue / AUI_RELIGION_SCORE_BELIEF_TWEAK_PANTHEON_DIVIDER + 0.5);
#else
		iRtnValue /= 2;
#endif
#endif
	}

#ifdef AUI_RELIGION_SCORE_BELIEF_AT_CITY_REMADE
	if (pEntry->RequiresPeace())
	{
#ifdef AUI_GS_PRIORITY_RATIO
		dRtnValue /= (1.5 + m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST")));
#else
		dRtnValue /= (1.5 + (m_pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST") ? 1 : 0));
#endif
	}

	if (pEntry->GetObsoleteEra() > 0)
	{
		dRtnValue /= pow(double(GC.getNumEraInfos() - pEntry->GetObsoleteEra()), m_pPlayer->GetCurrentEra() / (double)pEntry->GetObsoleteEra());
	}
#endif

#ifdef AUI_RELIGION_USE_DOUBLES
	return int(dRtnValue + 0.5);
#else
	return iRtnValue;
#endif
}

/// AI's evaluation of this belief's usefulness at this one plot
#ifdef AUI_RELIGION_SCORE_BELIEF_AT_PLOT_REMADE
double CvReligionAI::ScoreBeliefAtPlot(const CvBeliefEntry* pEntry, const CvPlot* pPlot, const CvCity* pForCity) const
#elif defined(AUI_RELIGION_USE_DOUBLES)
double CvReligionAI::ScoreBeliefAtPlot(const CvBeliefEntry* pEntry, const CvPlot* pPlot) const
#elif defined(AUI_CONSTIFY)
int CvReligionAI::ScoreBeliefAtPlot(const CvBeliefEntry* pEntry, const CvPlot* pPlot) const
#else
int CvReligionAI::ScoreBeliefAtPlot(CvBeliefEntry* pEntry, CvPlot* pPlot)
#endif
{
#ifdef AUI_RELIGION_SCORE_BELIEF_AT_PLOT_REMADE
	if (!pForCity)
	{
		pForCity = m_pPlayer->getCapitalCity();
		if (!pForCity)
			return 0;
	}
	
	double dReduceFactor = 0;

	double adExtraYields[NUM_YIELD_TYPES] = {};

	CvPlayerTraits* pTraits = m_pPlayer->GetPlayerTraits();

	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		TerrainTypes eTerrain = pPlot->getTerrainType();
		FeatureTypes eFeature = pPlot->getFeatureType();
		ResourceTypes eResource = pPlot->getResourceType(m_pPlayer->getTeam());
		int iYieldMod = 0;
		int iBeliefYieldMod = 0;

		// Terrain
		if (eTerrain != NO_TERRAIN && (eFeature == NO_FEATURE || !GC.getFeatureInfo(eFeature)->isYieldNotAdditive()))
			adExtraYields[iI] += pEntry->GetTerrainYieldChange(eTerrain, iI);

		// Feature
		if (eFeature != NO_FEATURE)
		{
			adExtraYields[iI] += pEntry->GetFeatureYieldChange(eFeature, iI);

			if (pPlot->IsNaturalWonder())
			{
				adExtraYields[iI] += pEntry->GetYieldChangeNaturalWonder(iI);
				iYieldMod += pTraits->GetNaturalWonderYieldModifier();
				iBeliefYieldMod += pEntry->GetYieldModifierNaturalWonder(iI);
			}

			// Dirty code to account for forest or jungle that will be cut later due to resource; effect is -75%
			if ((eFeature == FEATURE_FOREST || eFeature == FEATURE_JUNGLE) && eResource != NO_RESOURCE && GC.getResourceInfo(eResource)->getResourceUsage() != RESOURCEUSAGE_BONUS)
				++dReduceFactor;
		}

		// Resource
		if (eResource != NO_RESOURCE)
		{
			adExtraYields[iI] += pEntry->GetResourceYieldChange(eResource, iI);

			// Improvement
			int iMaxYieldChange = 0;
			int iCurrentYieldChange = 0;
			bool bIsReduce = false;
#ifdef AUI_WARNING_FIXES
			for (uint jJ = 0; jJ < GC.getNumImprovementInfos(); jJ++)
#else
			for (int jJ = 0; jJ < GC.getNumImprovementInfos(); jJ++)
#endif
			{
				if (pPlot->canHaveImprovement((ImprovementTypes)jJ, m_pPlayer->getTeam()))
				{
					iCurrentYieldChange = pEntry->GetImprovementYieldChange((ImprovementTypes)jJ, (YieldTypes)iI);
					if (iCurrentYieldChange > iMaxYieldChange || (bIsReduce && iCurrentYieldChange == iMaxYieldChange))
					{
						iMaxYieldChange = iCurrentYieldChange;
						// Fishing boats are a lot more expensive hammer-wise because work boats are single-use
						if ((pPlot->isWater() && pPlot->getImprovementType() != (ImprovementTypes)jJ))
							bIsReduce = true;
						else
							bIsReduce = false;
					}
				}
			}
			if (bIsReduce)
				++dReduceFactor;
			adExtraYields[iI] += iMaxYieldChange;
		}

		if (iYieldMod != 0)
		{
			adExtraYields[iI] *= 100 + iBeliefYieldMod + iYieldMod;
			adExtraYields[iI] /= 100;
			adExtraYields[iI] += (pPlot->getYield((YieldTypes)iI) * (100 + iBeliefYieldMod) - pPlot->getYield((YieldTypes)iI) * 100) / 100;
		}
	}

	dReduceFactor /= NUM_YIELD_TYPES;

	int iNewPlotValue = pForCity->GetCityCitizens()->GetPlotValue(pPlot, true, adExtraYields);
	double dRtnValue = iNewPlotValue - pForCity->GetCityCitizens()->GetPlotValue(pPlot, true);
	int iLowestWorkedPlotValue;
	pForCity->GetCityCitizens()->GetBestCityPlotWithValue(iLowestWorkedPlotValue, false, true);
	if (iNewPlotValue <= iLowestWorkedPlotValue)
		++dReduceFactor;

	if (dReduceFactor == 0)
	{
		dRtnValue *= GC.getRELIGION_BELIEF_SCORE_WORKED_PLOT_MULTIPLIER();
	}
	else if (dReduceFactor <= 1)
	{
		dRtnValue *= GC.getRELIGION_BELIEF_SCORE_OWNED_PLOT_MULTIPLIER();
	}
	else
	{
		dRtnValue *= GC.getRELIGION_BELIEF_SCORE_UNOWNED_PLOT_MULTIPLIER();
		dRtnValue /= dReduceFactor;
	}

	return dRtnValue;
#else
#ifdef AUI_RELIGION_USE_DOUBLES
	double dRtnValue = 0;
#else
	int iRtnValue = 0;
#endif

	for(int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		// Terrain
		TerrainTypes eTerrain = pPlot->getTerrainType();
		if(eTerrain != NO_TERRAIN)
		{
#ifdef AUI_RELIGION_USE_DOUBLES
			dRtnValue += pEntry->GetTerrainYieldChange(eTerrain, iI);
#else
			iRtnValue += pEntry->GetTerrainYieldChange(eTerrain, iI);
#endif
		}

		// Feature
		FeatureTypes eFeature = pPlot->getFeatureType();
		if(eFeature != NO_FEATURE)
		{
#ifdef AUI_RELIGION_USE_DOUBLES
			dRtnValue += pEntry->GetFeatureYieldChange(eFeature, iI);
#else
			iRtnValue += pEntry->GetFeatureYieldChange(eFeature, iI);
#endif

			if(pPlot->IsNaturalWonder())
			{
#ifdef AUI_RELIGION_USE_DOUBLES
				dRtnValue += pEntry->GetYieldChangeNaturalWonder(iI);
				dRtnValue += (pEntry->GetYieldModifierNaturalWonder(iI) / 25);
#else
				iRtnValue += pEntry->GetYieldChangeNaturalWonder(iI);
				iRtnValue += (pEntry->GetYieldModifierNaturalWonder(iI) / 25);
#endif
			}
		}

		// Resource
		ResourceTypes eResource = pPlot->getResourceType();
		if(eResource != NO_RESOURCE)
		{
#ifdef AUI_RELIGION_USE_DOUBLES
			dRtnValue += pEntry->GetResourceYieldChange(eResource, iI);
#else
			iRtnValue += pEntry->GetResourceYieldChange(eResource, iI);
#endif

			// Improvement
#ifdef AUI_WARNING_FIXES
			for (uint jJ = 0; jJ < GC.getNumImprovementInfos(); jJ++)
#else
			int iNumImprovementInfos = GC.getNumImprovementInfos();
			for(int jJ = 0; jJ < iNumImprovementInfos; jJ++)
#endif
			{
				if(pPlot->canHaveImprovement((ImprovementTypes)jJ, m_pPlayer->getTeam()))
				{
#ifdef AUI_RELIGION_USE_DOUBLES
					dRtnValue += (pEntry->GetImprovementYieldChange((ImprovementTypes)jJ, (YieldTypes)iI) * 2);
#else
					iRtnValue += (pEntry->GetImprovementYieldChange((ImprovementTypes)jJ, (YieldTypes)iI) * 2);
#endif
				}
			}
		}
	}

#ifdef AUI_RELIGION_USE_DOUBLES
	return dRtnValue;
#else
	return iRtnValue;
#endif
#endif
}

/// AI's evaluation of this belief's usefulness at this one plot
#if defined(AUI_RELIGION_SCORE_BELIEF_AT_CITY_REMADE) || defined(AUI_RELIGION_USE_DOUBLES)
double CvReligionAI::ScoreBeliefAtCity(const CvBeliefEntry* pEntry, const CvCity* pCity) const
{
	double dRtnValue = 0;
#else
#if defined(AUI_CONSTIFY)
int CvReligionAI::ScoreBeliefAtCity(const CvBeliefEntry* pEntry, const CvCity* pCity) const
#else
int CvReligionAI::ScoreBeliefAtCity(CvBeliefEntry* pEntry, CvCity* pCity)
#endif
{
	int iRtnValue = 0;
#endif
#ifdef AUI_RELIGION_SCORE_BELIEF_AT_CITY_REMADE
	CvFlavorManager* pFlavorManager = m_pPlayer->GetFlavorManager();
	int iFlavorDefense = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_DEFENSE"));
	int iFlavorCityDefense = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_CITY_DEFENSE"));
	int iFlavorExpansion = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_EXPANSION"));
	if (GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE))
		iFlavorExpansion = GC.getFLAVOR_MIN_VALUE();
	int iFlavorWonder = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_WONDER"));

	int iCultureCostMod = pCity->getPlotCultureCostModifier() + pEntry->GetPlotCultureCostModifier();
	if (iCultureCostMod < GC.getCULTURE_PLOT_COST_MOD_MINIMUM())
		iCultureCostMod = GC.getCULTURE_PLOT_COST_MOD_MINIMUM();
	dRtnValue += 4.0 * (double(100 + pCity->getPlotCultureCostModifier()) / double(100 + iCultureCostMod)) / log10(GC.getFLAVOR_MAX_VALUE() + 1.0 - (double)iFlavorExpansion);

	if (m_pPlayer->GetDiplomacyAI()->GetStateAllWars() == STATE_ALL_WARS_LOSING && pCity->getThreatValue() >= 1)
	{
		dRtnValue += 4.0 * (pEntry->GetCityRangeStrikeModifier() * pCity->getStrengthValue(true) / 10000.0) / log10(GC.getFLAVOR_MAX_VALUE() + 1.0 - iFlavorCityDefense) 
			* (1.0 + log((double)pCity->getThreatValue()));
		int iNumUnitsAffected = 1;
		if (pCity->getThreatValue() > 0)
			iNumUnitsAffected += pCity->plot()->getNumDefenders(pCity->getOwner());
		for (int jJ = 0; jJ < NUM_DIRECTION_TYPES; jJ++)
		{
			CvPlot* pLoopPlot = plotDirection(pCity->plot()->getX(), pCity->plot()->getY(), (DirectionTypes)jJ);
			if (pLoopPlot && m_pPlayer->IsPlotUnderImmediateThreat(*pLoopPlot))
				iNumUnitsAffected += pLoopPlot->getNumDefenders(pCity->getOwner());
		}
		dRtnValue += (NUM_DIRECTION_TYPES + 1) / 2.0 * (1.0 + (double)pEntry->GetFriendlyHealChange() / (double)GC.getFRIENDLY_HEAL_RATE()) / log10(GC.getFLAVOR_MAX_VALUE() + 1.0 - iFlavorDefense)
			* (1.0 + log((double)iNumUnitsAffected));
	}
	else
	{
		dRtnValue += 4.0 * (pEntry->GetCityRangeStrikeModifier() * pCity->getStrengthValue(true) / 10000.0) / log10(GC.getFLAVOR_MAX_VALUE() + 1.0 - iFlavorCityDefense);
		dRtnValue += (NUM_DIRECTION_TYPES + 1) / 2.0 * (1.0 + (double)pEntry->GetFriendlyHealChange() / (double)GC.getFRIENDLY_HEAL_RATE()) / log10(GC.getFLAVOR_MAX_VALUE() + 1.0 - iFlavorDefense);
	}

	// Here begins the stuff that will go into the CityCitizens PlotValue function

	int iHappiness = pEntry->GetHappinessPerCity();
	if (pCity->plot()->isRiver())
		iHappiness += pEntry->GetRiverHappiness();
	for (uint jJ = 0; jJ < GC.getNumBuildingClassInfos(); jJ++)
	{
		iHappiness += pEntry->GetBuildingClassHappiness(jJ);
	}

	double adExtraYields[NUM_YIELD_TYPES] = {};
	double adExtraYieldMods[NUM_YIELD_TYPES] = {};

#ifdef AUI_PLAYER_CAN_CONSTRUCT_AI_HELPERS
	int iEras;
#endif
	for (int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		// City yield change
		adExtraYields[iI] += pEntry->GetCityYieldChange(iI);

		// Trade route yield change
		adExtraYields[iI] += (pCity->IsRouteToCapitalConnected() ? pEntry->GetYieldChangeTradeRoute(iI) : pEntry->GetYieldChangeTradeRoute(iI) / 2.0);

		// Specialist yield change
		adExtraYields[iI] += (pCity->GetCityCitizens()->GetTotalSpecialistCount() > 0 ? pEntry->GetYieldChangeAnySpecialist(iI) : pEntry->GetYieldChangeAnySpecialist(iI) / 2.0);

		// Building class yield change
		for (uint jJ = 0; jJ < GC.getNumBuildingClassInfos(); jJ++)
		{
			CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo((BuildingClassTypes)jJ);
			if(!pkBuildingClassInfo)
			{
				continue;
			}
			if (pkBuildingClassInfo->getMaxPlayerInstances() > m_pPlayer->getNumCities())
			{
				bool bSkip = true;
				for (int iK = 0; iK < pkBuildingClassInfo->getMaxPlayerInstances(); iK++)
				{
					if (m_pPlayer->getCity(iK) == pCity)
					{
						bSkip = false;
						break;
					}
				}
				if (bSkip)
					continue;
			}			

			BuildingTypes eBuilding = (BuildingTypes)m_pPlayer->getCivilizationInfo().getCivilizationBuildings(jJ);
			if (eBuilding != NO_BUILDING)
			{
#ifdef AUI_PLAYER_CAN_CONSTRUCT_AI_HELPERS
				iEras = 1;
				if (m_pPlayer->canConstruct(eBuilding, false, true, true, NULL, true, &iEras))
				{
					iEras++;
					adExtraYields[iI] += (pCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0 ? pEntry->GetBuildingClassYieldChange(jJ, iI) : pEntry->GetBuildingClassYieldChange(jJ, iI) / 2.0) / (double)iEras;
					if (pkBuildingClassInfo->getMaxGlobalInstances() != -1)
					{
						adExtraYields[iI] += (pCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0 ? pEntry->GetYieldChangeWorldWonder(iI) : pEntry->GetYieldChangeWorldWonder(iI) / 2.0) / (double)iEras;
#else
				if (m_pPlayer->canConstruct(eBuilding, false, true, true))
				{
					adExtraYields[iI] += (pCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0 ? pEntry->GetBuildingClassYieldChange(jJ, iI) : pEntry->GetBuildingClassYieldChange(jJ, iI) / 2.0);
					if (pkBuildingClassInfo->getMaxGlobalInstances() != -1)
					{
						adExtraYields[iI] += (pCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0 ? pEntry->GetYieldChangeWorldWonder(iI) : pEntry->GetYieldChangeWorldWonder(iI) / 2.0);
#endif
					}
				}
			}
		}

		// Production per follower
		if (pEntry->GetMaxYieldModifierPerFollower(iI) > 0)
		{
			adExtraYieldMods[iI] += MIN(pEntry->GetMaxYieldModifierPerFollower(iI), pCity->getPopulation());
		}
	}

	adExtraYieldMods[YIELD_PRODUCTION] += pEntry->GetWonderProductionModifier() / log10(GC.getFLAVOR_MAX_VALUE() + 1.0 - iFlavorWonder);

	dRtnValue = pCity->GetCityCitizens()->GetPlotValue(pCity->plot(), true, adExtraYields, adExtraYieldMods, iHappiness, pEntry->GetCityGrowthModifier()) - pCity->GetCityCitizens()->GetPlotValue(pCity->plot(), true);

	int iMinPop = MAX(pEntry->GetMinPopulation(), pEntry->GetMinFollowers());
	if (pCity->getPopulation() < iMinPop)
	{
		dRtnValue /= log(iMinPop - pCity->getPopulation() + M_E);
	}
#else
#ifdef AUI_RELIGION_USE_DOUBLES
	double dTempValue;
#else
	int iTempValue;
#endif
	double dTempValue;
	int iMinPop;
	int iMinFollowers;
#ifdef AUI_RELIGION_USE_DOUBLES
	double dHappinessMultiplier = 2;
#else
	int iHappinessMultiplier = 2;
#endif

	CvFlavorManager* pFlavorManager = m_pPlayer->GetFlavorManager();
	int iFlavorOffense = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_OFFENSE"));
	int iFlavorDefense = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_DEFENSE"));
	int iFlavorCityDefense = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_CITY_DEFENSE"));
	int iFlavorHappiness = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_HAPPINESS"));

#ifdef AUI_RELIGION_USE_DOUBLES
	double dHappinessNeedFactor = iFlavorOffense * 2 + iFlavorHappiness - iFlavorDefense;
	if (dHappinessNeedFactor > 15)
	{
		dHappinessMultiplier = 3;
	}
	else if (dHappinessNeedFactor < 6)
	{
		dHappinessMultiplier = 1;
	}
#else
	int iHappinessNeedFactor = iFlavorOffense * 2 + iFlavorHappiness - iFlavorDefense;
	if (iHappinessNeedFactor > 15)
	{
		iHappinessMultiplier = 3;
	}
	else if (iHappinessNeedFactor < 6)
	{
		iHappinessMultiplier = 1;
	}
#endif

	iMinPop = pEntry->GetMinPopulation();
	iMinFollowers = pEntry->GetMinFollowers();

	// Simple ones
#ifdef AUI_RELIGION_USE_DOUBLES
	dRtnValue += pEntry->GetCityGrowthModifier() / 3;
	if(pEntry->RequiresPeace())
	{
		dRtnValue /= 2.0;
	}
	dRtnValue += (-pEntry->GetPlotCultureCostModifier() * 2) / 10.0;
	dRtnValue += (pEntry->GetCityRangeStrikeModifier() / 10.0) * MAX(iFlavorDefense, iFlavorCityDefense);
	dRtnValue += pEntry->GetFriendlyHealChange() / 10.0;
#else
	iRtnValue += pEntry->GetCityGrowthModifier() / 3;
	if(pEntry->RequiresPeace())
	{
		iRtnValue /= 2;
	}
	iRtnValue += (-pEntry->GetPlotCultureCostModifier() * 2) / 10;
	iRtnValue += (pEntry->GetCityRangeStrikeModifier() / 10) * MAX(iFlavorDefense,iFlavorCityDefense);
	iRtnValue += pEntry->GetFriendlyHealChange() / 10;
#endif

	// Wonder production multiplier
	if(pEntry->GetObsoleteEra() > 0)
	{	
#ifdef AUI_RELIGION_FIX_SCORE_BELIEF_AT_CITY_USE_PLAYER_ERA
		double dEraMultiplier = pow((double)pEntry->GetObsoleteEra(), 1.0 - m_pPlayer->GetCurrentEra() / (double)pEntry->GetObsoleteEra());
#ifdef AUI_RELIGION_USE_DOUBLES
		dRtnValue += (pEntry->GetWonderProductionModifier() * dEraMultiplier) / 10.0;
#else
		iRtnValue += int(pEntry->GetWonderProductionModifier() * dEraMultiplier) / 10.0 + 0.5);
#endif
#else
		if (pEntry->GetObsoleteEra() > GC.getGame().getCurrentEra())
		{
#ifdef AUI_RELIGION_USE_DOUBLES
			dRtnValue += (pEntry->GetWonderProductionModifier() * pEntry->GetObsoleteEra()) / 10.0;
#else
			iRtnValue += (pEntry->GetWonderProductionModifier() * pEntry->GetObsoleteEra()) / 10;
#endif
		}
#endif
	}
	else
	{
#ifdef AUI_RELIGION_USE_DOUBLES
		dRtnValue += pEntry->GetWonderProductionModifier() / 3.0;
#else
		iRtnValue += pEntry->GetWonderProductionModifier() / 3;
#endif
	}

	// River happiness
#ifdef AUI_RELIGION_FIX_SCORE_BELIEF_AT_CITY_RIVER_HAPPINESS
	if (pCity->plot()->isRiver())
	{
#endif
#ifdef AUI_RELIGION_USE_DOUBLES
		dTempValue = pEntry->GetRiverHappiness() * dHappinessMultiplier;
#else
	iTempValue = pEntry->GetRiverHappiness() * iHappinessMultiplier;
#endif
	if(iMinPop > 0)
	{
		if(pCity->getPopulation() >= iMinPop)
		{
#ifdef AUI_RELIGION_USE_DOUBLES
			dTempValue *= 2;
#else
			iTempValue *= 2;
#endif
		}
	}
#ifdef AUI_RELIGION_USE_DOUBLES
	dRtnValue += dTempValue;
#else
	iRtnValue += iTempValue;
#endif
#ifdef AUI_RELIGION_FIX_SCORE_BELIEF_AT_CITY_RIVER_HAPPINESS
	}
#endif

	// Happiness per city
#ifdef AUI_RELIGION_USE_DOUBLES
	dTempValue = pEntry->GetHappinessPerCity() * dHappinessMultiplier;
#else
	iTempValue = pEntry->GetHappinessPerCity() * iHappinessMultiplier;
#endif
	if(iMinPop > 0)
	{
		if(pCity->getPopulation() >= iMinPop)
		{
#ifdef AUI_RELIGION_USE_DOUBLES
			dTempValue *= 2;
#else
			iTempValue *= 2;
#endif
		}
	}
#ifdef AUI_RELIGION_USE_DOUBLES
	dRtnValue += dTempValue;
#else
	iRtnValue += iTempValue;
#endif

	// Building class happiness
#ifdef AUI_WARNING_FIXES
	for (uint jJ = 0; jJ < GC.getNumBuildingClassInfos(); jJ++)
#else
	for(int jJ = 0; jJ < GC.getNumBuildingClassInfos(); jJ++)
#endif
	{
#ifdef AUI_RELIGION_USE_DOUBLES
		dTempValue = pEntry->GetBuildingClassHappiness(jJ) * dHappinessMultiplier;
#else
		iTempValue = pEntry->GetBuildingClassHappiness(jJ) * iHappinessMultiplier;
#endif
		if(iMinFollowers > 0)
		{
			if(pCity->getPopulation() >= iMinFollowers)
			{
#ifdef AUI_RELIGION_USE_DOUBLES
				dTempValue *= 2;
#else
				iTempValue *= 2;
#endif
			}
		}
#ifdef AUI_RELIGION_USE_DOUBLES
		dRtnValue += dTempValue;
#else
		iRtnValue += iTempValue;
#endif
	}

	for(int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		// City yield change
#ifdef AUI_RELIGION_USE_DOUBLES
		dTempValue = pEntry->GetCityYieldChange(iI);
#else
		iTempValue = pEntry->GetCityYieldChange(iI);
#endif
		if(iMinPop > 0)
		{
			if(pCity->getPopulation() >= iMinPop)
			{
#ifdef AUI_RELIGION_USE_DOUBLES
				dTempValue *= 2;
#else
				iTempValue *= 2;
#endif
			}
		}
#ifdef AUI_RELIGION_USE_DOUBLES
		dRtnValue += dTempValue;
#else
		iRtnValue += iTempValue;
#endif
		

		// Trade route yield change
#ifdef AUI_RELIGION_USE_DOUBLES
		dTempValue = pEntry->GetYieldChangeTradeRoute(iI);
#else
		iTempValue = pEntry->GetYieldChangeTradeRoute(iI);
#endif
		if(iMinPop > 0)
		{
			if(pCity->getPopulation() >= iMinPop)
			{
#ifdef AUI_RELIGION_USE_DOUBLES
				dTempValue *= 2;
#else
				iTempValue *= 2;
#endif
			}
		}
		if(pCity->IsRouteToCapitalConnected())
		{
#ifdef AUI_RELIGION_USE_DOUBLES
			dTempValue *= 2;
#else
			iTempValue *= 2;
#endif
		}
#ifdef AUI_RELIGION_USE_DOUBLES
		dRtnValue += dTempValue;
#else
		iRtnValue += iTempValue;
#endif

		// Specialist yield change
#ifdef AUI_RELIGION_USE_DOUBLES
		dTempValue = pEntry->GetYieldChangeAnySpecialist(iI);
		if(pCity->getPopulation() >= 8)  // Like it more with large cities
		{
			dTempValue *= 3;
		}
		dRtnValue += dTempValue;
#else
		iTempValue = pEntry->GetYieldChangeAnySpecialist(iI);
		if(pCity->getPopulation() >= 8)  // Like it more with large cities
		{
			iTempValue *= 3;
		}
		iRtnValue += iTempValue;
#endif

		// Building class yield change
#ifdef AUI_WARNING_FIXES
		for (uint jJ = 0; jJ < GC.getNumBuildingClassInfos(); jJ++)
#else
		for(int jJ = 0; jJ < GC.getNumBuildingClassInfos(); jJ++)
#endif
		{
			CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo((BuildingClassTypes)jJ);
			if(!pkBuildingClassInfo)
			{
				continue;
			}

#ifdef AUI_RELIGION_USE_DOUBLES
			dTempValue = pEntry->GetBuildingClassYieldChange(jJ, iI);
#else
			iTempValue = pEntry->GetBuildingClassYieldChange(jJ, iI);
#endif
			if(iMinFollowers > 0)
			{
				if(pCity->getPopulation() < iMinFollowers)
				{
#ifdef AUI_RELIGION_USE_DOUBLES
					dTempValue /= 2.0;
#else
					iTempValue /= 2;
#endif
				}
			}

			BuildingTypes eBuilding = (BuildingTypes)m_pPlayer->getCivilizationInfo().getCivilizationBuildings(jJ);
			if (eBuilding != NO_BUILDING)
			{
				if(pCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
				{
#ifdef AUI_RELIGION_USE_DOUBLES
					dTempValue *= 2;
#else
					iTempValue *= 2;
#endif
				}
			}

			if(pkBuildingClassInfo->getMaxPlayerInstances() == 1)
			{
#ifdef AUI_RELIGION_USE_DOUBLES
				dTempValue /= 2.0;
#else
				iTempValue /= 2;
#endif
			}

#ifdef AUI_RELIGION_USE_DOUBLES
			dRtnValue += dTempValue;
#else
			iRtnValue += iTempValue;
#endif
		}

		// World wonder change
#ifdef AUI_RELIGION_USE_DOUBLES
		dRtnValue += pEntry->GetYieldChangeWorldWonder(iI) * 3 / 2.0;
#else
		iRtnValue += pEntry->GetYieldChangeWorldWonder(iI) * 3 / 2;
#endif

		// Production per follower
		if (pEntry->GetMaxYieldModifierPerFollower(iI) > 0)
		{
#ifdef AUI_RELIGION_USE_DOUBLES
			dTempValue = MIN(pEntry->GetMaxYieldModifierPerFollower(iI), pCity->getPopulation());
			dTempValue /= 2.0;
			dRtnValue += dTempValue;
#else
#ifdef AUI_FAST_COMP
			iTempValue = MIN(pEntry->GetMaxYieldModifierPerFollower(iI), pCity->getPopulation());
#else
			iTempValue = min(pEntry->GetMaxYieldModifierPerFollower(iI), pCity->getPopulation());
#endif
			iTempValue /= 2;
			iRtnValue += iTempValue;
#endif
		}
	}
#endif

#if defined(AUI_RELIGION_SCORE_BELIEF_AT_CITY_REMADE) || defined(AUI_RELIGION_USE_DOUBLES)
	return dRtnValue;
#else
	return iRtnValue;
#endif
}

/// AI's evaluation of this belief's usefulness to this player
#ifdef AUI_RELIGION_USE_DOUBLES
double CvReligionAI::ScoreBeliefForPlayer(const CvBeliefEntry* pEntry) const
{
	double dRtnValue = 0;
#else
#if defined(AUI_CONSTIFY)
int CvReligionAI::ScoreBeliefForPlayer(const CvBeliefEntry* pEntry) const
#else
int CvReligionAI::ScoreBeliefForPlayer(CvBeliefEntry* pEntry)
#endif
{
	int iRtnValue = 0;
#endif
	CvFlavorManager* pFlavorManager = m_pPlayer->GetFlavorManager();
#ifndef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_BETTER_ENHANCER_SCALING
	CvGameReligions* pGameReligions = GC.getGame().GetGameReligions();
#endif

	//--------------------
	// GET BACKGROUND DATA
	//--------------------
	int iFlavorOffense = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_OFFENSE"));
	int iFlavorDefense = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_DEFENSE"));
	int iFlavorHappiness = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_HAPPINESS"));
	int iFlavorCulture = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_CULTURE"));
	int iFlavorGold = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_GOLD"));
	int iFlavorGP = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_GREAT_PEOPLE"));
	int iFlavorScience = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_SCIENCE"));
	int iFlavorDiplomacy = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_DIPLOMACY"));
	int iFlavorExpansion = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_EXPANSION"));
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_TWEAKED_FLAVORS
	if (GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE))
		iFlavorExpansion = GC.getFLAVOR_MIN_VALUE();
	int iFlavorGrowth = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_GROWTH"));
	int iFlavorProduction = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_PRODUCTION"));
#else
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_BETTER_HAPPINESS_FLAVOR
	int iFlavorGrowth = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_GROWTH"));
#endif
#endif

#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_BETTER_HAPPINESS_FLAVOR
	double dHappinessNeedFactor = (iFlavorOffense + iFlavorExpansion + iFlavorHappiness - iFlavorDefense) * log((double)MAX(1, MAX(iFlavorGrowth, iFlavorExpansion)));
#endif

#ifdef AUI_MINOR_CIV_RATIO
	// Adjusts city state beliefs based on how many city states there actually are
	double dCityStateDeviation = 1.0 + log(MAX((double)GC.getGame().getCurrentMinorCivDeviation(), exp(-1.0)));
#endif

#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_BETTER_ENHANCER_SCALING
	double dTurnsCompletedPercent = 50.0 * (double)GC.getGame().getGameTurn() / (double)GC.getGame().getEstimateEndTurn();
#else
	int iNumEnhancedReligions = pGameReligions->GetNumReligionsEnhanced();
	int iReligionsEnhancedPercent = (100 * iNumEnhancedReligions) / GC.getMap().getWorldInfo().getMaxActiveReligions();
#endif

	//------------------------------
	// PLAYER-LEVEL PANTHEON BELIEFS
	//------------------------------
	if(pEntry->GetFaithFromKills() > 0)
	{
#ifdef AUI_RELIGION_USE_DOUBLES
		double dTemp = pEntry->GetFaithFromKills() * pEntry->GetMaxDistance() * iFlavorOffense / 100.0;
#ifdef AUI_GS_PRIORITY_RATIO
		dTemp *= (0.5 + 1.5 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST")));
#else
		if (m_pPlayer->GetDiplomacyAI()->IsGoingForWorldConquest())
		{
			dTemp *= 2;
		}
#endif
		dRtnValue += dTemp;
#else
		int iTemp = pEntry->GetFaithFromKills() * pEntry->GetMaxDistance() * iFlavorOffense / 100;
#ifdef AUI_GS_PRIORITY_RATIO
		iTemp = int(iTemp * (0.5 + 1.5 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"))) + 0.5);
#else
		if (m_pPlayer->GetDiplomacyAI()->IsGoingForWorldConquest())
		{
			iTemp *= 2;
		}
#endif
		iRtnValue += iTemp;
#endif
	}

	//-------
	// UNUSED
	//-------
#ifdef AUI_RELIGION_USE_DOUBLES
	dRtnValue += pEntry->GetPlayerHappiness() * iFlavorHappiness;
	dRtnValue += pEntry->GetPlayerCultureModifier() * iFlavorCulture;
#else
	iRtnValue += pEntry->GetPlayerHappiness() * iFlavorHappiness;
	iRtnValue += pEntry->GetPlayerCultureModifier() * iFlavorCulture;
#endif

	//-----------------
	// FOLLOWER BELIEFS
	//-----------------
	// Unlocks a building
#ifdef AUI_WARNING_FIXES
	for (uint iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
#else
	for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
#endif
	{
		if (pEntry->IsBuildingClassEnabled(iI))
		{
			BuildingTypes eBuilding = (BuildingTypes)m_pPlayer->getCivilizationInfo().getCivilizationBuildings(iI);
			CvBuildingEntry* pBuildingEntry = GC.GetGameBuildings()->GetEntry(eBuilding);

			if(pBuildingEntry && pFlavorManager)
			{
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_TWEAKED_FLAVORS
				int iLoop = 0;
				double dTotalExtraFlavor = 0.0;
				for (CvCity* pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
				{
					dTotalExtraFlavor += pLoopCity->GetCityStrategyAI()->GetBuildingProductionAI()->GetWeight(eBuilding) / 10.0;
				}
				// If can also be built with hammers, much less valuable
				if (pBuildingEntry->GetProductionCost() > 0)
				{
					dTotalExtraFlavor /= 10.0;
				}
#else
				for(int iFlavorLoop = 0; iFlavorLoop < GC.getNumFlavorTypes(); iFlavorLoop++)
				{
#ifdef AUI_RELIGION_USE_DOUBLES
					double dFlavorValue = (double)pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)iFlavorLoop) * pBuildingEntry->GetFlavorValue(iFlavorLoop);

					// If can also be built with hammers, much less valuable
					if (pBuildingEntry->GetProductionCost() > 0)
					{
						dFlavorValue /= 10.0;
					}

					dRtnValue += dFlavorValue;
#else
					int iFlavorValue = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)iFlavorLoop) * pBuildingEntry->GetFlavorValue(iFlavorLoop);

					// If can also be built with hammers, much less valuable
					if (pBuildingEntry->GetProductionCost() > 0)
					{
						iFlavorValue /= 10;
					}

					iRtnValue += iFlavorValue;
#endif
				}
#endif
			}
		}
	}

	// Unlocks units?
#ifdef AUI_RELIGION_FIX_SCORE_BELIEF_FOR_PLAYER_UNLOCKS_UNITS_DISREGARD_OLD_ERAS
	for (uint i = (uint)m_pPlayer->GetCurrentEra(); i < GC.getNumEraInfos(); i++)
#elif defined(AUI_WARNING_FIXES)
	for (uint i = 0; i < GC.getNumEraInfos(); i++)
#else
	for(int i = 0; i < GC.getNumEraInfos(); i++)
#endif
	{
		// Add in for each era enabled
		if (pEntry->IsFaithUnitPurchaseEra(i))
		{
#ifdef AUI_RELIGION_USE_DOUBLES
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_TWEAKED_FLAVORS
			double dTemp = (iFlavorOffense) + (iFlavorDefense / 2.0) - (iFlavorProduction * 2.0);
#else
			double dTemp = (iFlavorOffense) + (iFlavorDefense / 2.0);
#endif
#ifdef AUI_GS_PRIORITY_RATIO
			dTemp *= (0.5 + 1.5 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST")));
#else
			if (m_pPlayer->GetDiplomacyAI()->IsGoingForWorldConquest())
			{
				dTemp *= 2;
			}
#endif
			dRtnValue += MAX(0.0, dTemp);
#else
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_TWEAKED_FLAVORS
			int iTemp = (iFlavorOffense) + (iFlavorDefense / 2) - (iFlavorProduction);
#else
			int iTemp = (iFlavorOffense) + (iFlavorDefense / 2);
#endif
#ifdef AUI_GS_PRIORITY_RATIO
			iTemp = int(iTemp * (0.5 + 1.5 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"))) + 0.5);
#else
			if (m_pPlayer->GetDiplomacyAI()->IsGoingForWorldConquest())
			{
				iTemp *= 2;
			}
#endif
#ifdef AUI_RELIGION_FIX_SCORE_BELIEF_FOR_PLAYER_UNLOCKS_UNITS_DISREGARD_OLD_ERAS
			iTemp *= AUI_RELIGION_FIX_SCORE_BELIEF_FOR_PLAYER_UNLOCKS_UNITS_DISREGARD_OLD_ERAS;
#endif
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_TWEAKED_FLAVORS
			iRtnValue += MAX(0, iTemp);
#else
			iRtnValue += iTemp;
#endif
#endif
		}
	}

	//----------------
	// FOUNDER BELIEFS
	//----------------
#ifdef AUI_RELIGION_USE_DOUBLES
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_BETTER_HAPPINESS_FLAVOR
	dRtnValue += pEntry->GetHappinessPerFollowingCity() * (dHappinessNeedFactor);
	if (pEntry->GetHappinessPerXPeacefulForeignFollowers() > 0)
	{
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_CONSIDER_GRAND_STRATEGY
#ifdef AUI_GS_PRIORITY_RATIO
		dRtnValue += (dHappinessNeedFactor * 2.5) / (pEntry->GetHappinessPerXPeacefulForeignFollowers() * MAX(iFlavorOffense, 1) *
			(1.0 + 3.0 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"))));
#else
		dRtnValue += (dHappinessNeedFactor * 2.5) / (pEntry->GetHappinessPerXPeacefulForeignFollowers() * MAX(iFlavorOffense, 1) *
			(1.0 + (m_pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST") ? 3 : 0)));
#endif
#else
		dRtnValue += (dHappinessNeedFactor) / double(pEntry->GetHappinessPerXPeacefulForeignFollowers() * MAX(iFlavorOffense, 1));
#endif
	}
#else
	dRtnValue += pEntry->GetHappinessPerFollowingCity() * double(iFlavorHappiness + 2 * iFlavorOffense - iFlavorDefense);
	if (pEntry->GetHappinessPerXPeacefulForeignFollowers() > 0 && iFlavorDefense > 0)
	{
		dRtnValue += (iFlavorHappiness * 10 * iFlavorDefense) / double(pEntry->GetHappinessPerXPeacefulForeignFollowers() * iFlavorDefense);
	}
#endif

#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_TWEAKED_FLAVORS
	dRtnValue += pEntry->GetSciencePerOtherReligionFollower() * iFlavorScience * (1.0 + log10((double)iFlavorScience) / 4.0) / 50.0 * 
		(GC.getNumEraInfos() - m_pPlayer->GetCurrentEra()) / GC.getNumEraInfos();
	dRtnValue += pEntry->GetGoldPerFollowingCity() * (iFlavorGold + iFlavorExpansion / 4.0);
#else
	dRtnValue += iFlavorScience * pEntry->GetSciencePerOtherReligionFollower() / 5.0;
	dRtnValue += pEntry->GetGoldPerFollowingCity() * iFlavorGold;
#endif
	if(pEntry->GetGoldPerXFollowers() > 0)
	{
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_TWEAKED_FLAVORS
		dRtnValue += ((iFlavorGold + iFlavorGrowth + iFlavorExpansion) * 40.0 / (double)pEntry->GetGoldPerXFollowers());
#else
		dRtnValue += (iFlavorGold * 4.0 / (double)pEntry->GetGoldPerXFollowers());
#endif
	}
	dRtnValue += iFlavorGold * pEntry->GetGoldWhenCityAdopts() / 50.0;

	// Minimum influence with city states
#ifdef AUI_MINOR_CIV_RATIO
	dRtnValue += iFlavorDiplomacy * pEntry->GetCityStateMinimumInfluence() / 64.0 * dCityStateDeviation;
#else
	dRtnValue += iFlavorDiplomacy * pEntry->GetCityStateMinimumInfluence() / 64.0;
#endif
#else
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_BETTER_HAPPINESS_FLAVOR
	iRtnValue += int(pEntry->GetHappinessPerFollowingCity() * (dHappinessNeedFactor) + 0.5);
	if (pEntry->GetHappinessPerXPeacefulForeignFollowers() > 0)
	{
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_CONSIDER_GRAND_STRATEGY
#ifdef AUI_GS_PRIORITY_RATIO
		iRtnValue += int((dHappinessNeedFactor * 2.5) / (pEntry->GetHappinessPerXPeacefulForeignFollowers() * MAX(iFlavorOffense, 1) *
			(1.0 + 3.0 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST")))) + 0.5);
#else
		iRtnValue += int((dHappinessNeedFactor * 2.5) / (pEntry->GetHappinessPerXPeacefulForeignFollowers() * MAX(iFlavorOffense, 1) *
			(1.0 + (m_pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST") ? 3 : 0))) + 0.5);
#endif
#else
		iRtnValue += int((dHappinessNeedFactor) / double(pEntry->GetHappinessPerXPeacefulForeignFollowers() * MAX(iFlavorOffense, 1)) + 0.5);
#endif
	}
#else
	iRtnValue += (int)(pEntry->GetHappinessPerFollowingCity() * (float)(iFlavorHappiness + 2 * iFlavorOffense - iFlavorDefense));
	if (pEntry->GetHappinessPerXPeacefulForeignFollowers() > 0 && iFlavorDefense > 0)
	{
		iRtnValue += (iFlavorHappiness * 10 * iFlavorDefense) / (pEntry->GetHappinessPerXPeacefulForeignFollowers() * iFlavorDefense);

	}
#endif

#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_TWEAKED_FLAVORS
	iRtnValue += int(pEntry->GetSciencePerOtherReligionFollower() * iFlavorScience * (1.0 + log10((double)iFlavorScience) / 4.0) * log((double)GC.getAI_CITIZEN_VALUE_SCIENCE()) / log(M_PI) / 10.0 + 0.5);
	iRtnValue += int(pEntry->GetGoldPerFollowingCity() * (iFlavorGold + iFlavorExpansion / 4.0) + 0.5);
#else
	iRtnValue += iFlavorScience * pEntry->GetSciencePerOtherReligionFollower() / 5;
	iRtnValue += pEntry->GetGoldPerFollowingCity() * iFlavorGold;
#endif
	if(pEntry->GetGoldPerXFollowers() > 0)
	{
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_TWEAKED_FLAVORS
		iRtnValue += int((iFlavorGold + iFlavorGrowth + iFlavorExpansion) * 4.0 / (double)pEntry->GetGoldPerXFollowers() + 0.5);
#else
		iRtnValue += (iFlavorGold * 4 / pEntry->GetGoldPerXFollowers());
#endif
	}
	iRtnValue += iFlavorGold * pEntry->GetGoldWhenCityAdopts() / 50;

	// Minimum influence with city states
#ifdef AUI_MINOR_CIV_RATIO
	iRtnValue += int(iFlavorDiplomacy * pEntry->GetCityStateMinimumInfluence() / 8.0 * dCityStateDeviation + 0.5);
#else
	iRtnValue += iFlavorDiplomacy * pEntry->GetCityStateMinimumInfluence() / 7;
#endif
#endif

	// Yields for foreign followers
	for(int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
#ifdef AUI_RELIGION_USE_DOUBLES
		if (iI == YIELD_CULTURE)
		{
			dRtnValue += pEntry->GetYieldChangePerForeignCity(iI) * 2 * iFlavorCulture;
			if (pEntry->GetYieldChangePerXForeignFollowers(iI) > 0)
			{
				dRtnValue += 10.0 * iFlavorCulture / (double)pEntry->GetYieldChangePerXForeignFollowers(iI);
			}
		}
		else
		{
			dRtnValue += pEntry->GetYieldChangePerForeignCity(iI) * 10;
			if (pEntry->GetYieldChangePerXForeignFollowers(iI) > 0)
			{
				dRtnValue += 50.0 / (double)pEntry->GetYieldChangePerXForeignFollowers(iI);
			}
		}
#else
		if (iI == YIELD_CULTURE)
		{
			iRtnValue += pEntry->GetYieldChangePerForeignCity(iI) * 2 * iFlavorCulture;
			if (pEntry->GetYieldChangePerXForeignFollowers(iI) > 0)
			{
				iRtnValue += 10 * iFlavorCulture / pEntry->GetYieldChangePerXForeignFollowers(iI);
			}
		}
		else
		{
			iRtnValue += pEntry->GetYieldChangePerForeignCity(iI) * 10;
			if (pEntry->GetYieldChangePerXForeignFollowers(iI) > 0)
			{
				iRtnValue += 50 / pEntry->GetYieldChangePerXForeignFollowers(iI);
			}
		}
#endif
	}

	//-----------------
	// ENHANCER BELIEFS
	//-----------------
#ifdef AUI_RELIGION_USE_DOUBLES
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_TWEAKED_FLAVORS
	dRtnValue += iFlavorGP * pEntry->GetGreatPersonExpendedFaith() / 40.0;
#else
	dRtnValue += iFlavorGP * pEntry->GetGreatPersonExpendedFaith() / 10.0;
#endif
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_CONSIDER_GRAND_STRATEGY
#ifdef AUI_GS_PRIORITY_RATIO
#ifdef AUI_MINOR_CIV_RATIO
	dRtnValue += iFlavorDiplomacy * pEntry->GetFriendlyCityStateSpreadModifier() / 45.0 * dCityStateDeviation * (0.5 + 2 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS")));
#else
	dRtnValue += iFlavorDiplomacy * pEntry->GetFriendlyCityStateSpreadModifier() / 45.0 * (0.5 + 2 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS")));
#endif
	dRtnValue += iFlavorDefense * pEntry->GetCombatModifierFriendlyCities() / 6.0 * (1.5 + 2 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST")));
	dRtnValue += iFlavorOffense * pEntry->GetCombatModifierEnemyCities() / 12.0 * (0.5 + 2 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST")));
#else
#ifdef AUI_MINOR_CIV_RATIO
	dRtnValue += iFlavorDiplomacy * pEntry->GetFriendlyCityStateSpreadModifier() / 45.0 * dCityStateDeviation * (m_pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS") ? 2.5 : 0.5);
#else
	dRtnValue += iFlavorDiplomacy * pEntry->GetFriendlyCityStateSpreadModifier() / 45.0 * (m_pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS") ? 2.5 : 0.5);
#endif
	dRtnValue += iFlavorDefense * pEntry->GetCombatModifierFriendlyCities() / 6.0 * (m_pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST") ? 0.5 : 2.5);
	dRtnValue += iFlavorOffense * pEntry->GetCombatModifierEnemyCities() / 6.0 * (m_pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST") ? 2.5 : 0.5);;
#endif
#else
#ifdef AUI_MINOR_CIV_RATIO
	dRtnValue += iFlavorDiplomacy * pEntry->GetFriendlyCityStateSpreadModifier() / 20.0 * dCityStateDeviation;
#else
	dRtnValue += iFlavorDiplomacy * pEntry->GetFriendlyCityStateSpreadModifier() / 20.0;
#endif
	dRtnValue += iFlavorDefense * pEntry->GetCombatModifierFriendlyCities() / 4.0;
	dRtnValue += iFlavorOffense * pEntry->GetCombatModifierEnemyCities() / 4.0;
#endif

#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_BETTER_ENHANCER_SCALING
	bool bHasMissionaryBonus = false;
	CvGameReligions* pReligions = GC.getGame().GetGameReligions();
	const CvReligion* pMyReligion = pReligions->GetReligion(m_pPlayer->GetReligions()->GetReligionCreatedByPlayer(), m_pPlayer->GetID());
	if (pMyReligion && pMyReligion->m_Beliefs.GetSciencePerOtherReligionFollower() > 0)
	{
		bHasMissionaryBonus = true;
	}

	// Good early, poor late
	dRtnValue += ((100.0 - dTurnsCompletedPercent) * ((2 * MIN(iFlavorGrowth, iFlavorExpansion) + iFlavorGrowth) + pEntry->GetSpreadDistanceModifier()) +
		dTurnsCompletedPercent * (((2 * MIN(iFlavorGrowth, iFlavorExpansion) + iFlavorGrowth) + pEntry->GetSpreadDistanceModifier()) / 4)) / 100.0;
	dRtnValue += ((100.0 - dTurnsCompletedPercent) * ((2 * iFlavorScience + MAX(iFlavorGrowth, iFlavorExpansion)) + pEntry->GetSpreadStrengthModifier()) +
		dTurnsCompletedPercent * (((2 * iFlavorScience + MAX(iFlavorGrowth, iFlavorExpansion)) + pEntry->GetSpreadStrengthModifier()) / 4)) / 100.0;
	dRtnValue += ((100.0 - dTurnsCompletedPercent) * (pEntry->GetMissionaryStrengthModifier() * 0.5) +
		dTurnsCompletedPercent * (pEntry->GetMissionaryStrengthModifier() * 0.375)) / 100.0 / (bHasMissionaryBonus ? 2 : 1);
	dRtnValue += ((100.0 - dTurnsCompletedPercent) * ((-1 * pEntry->GetMissionaryCostModifier()) * 2) +
		dTurnsCompletedPercent * ((-1 * pEntry->GetMissionaryCostModifier()) / 2)) / 100.0 * (bHasMissionaryBonus ? 2 : 1);

	// Poor early, good late
	dRtnValue += ((100.0 - dTurnsCompletedPercent) * (pEntry->GetProphetStrengthModifier() / 4.0) +
		dTurnsCompletedPercent * (pEntry->GetProphetStrengthModifier())) / 100.0;
	dRtnValue += ((100.0 - dTurnsCompletedPercent) * ((-1 * pEntry->GetProphetCostModifier()) / 2.0) +
		dTurnsCompletedPercent * (-1 * pEntry->GetProphetCostModifier())) / 100.0;
#else
	// Chosen EARLY?
	if (iReligionsEnhancedPercent < 33)
	{
		// Good early
		dRtnValue += (2 * iFlavorExpansion) + pEntry->GetSpreadDistanceModifier();
		dRtnValue += (2 * iFlavorScience) + pEntry->GetSpreadStrengthModifier();
		dRtnValue += pEntry->GetMissionaryStrengthModifier() * 2;
		dRtnValue += (-1 * pEntry->GetMissionaryCostModifier()) * 2;

		// Poor early
		dRtnValue += pEntry->GetProphetStrengthModifier() / 4.0;
		dRtnValue += (-1 * pEntry->GetProphetCostModifier()) / 4.0;
	}

	// Chosen in the MIDDLE?
	else if (iReligionsEnhancedPercent < 66)
	{
		dRtnValue += iFlavorExpansion + pEntry->GetSpreadDistanceModifier() / 2.0;
		dRtnValue += iFlavorScience + pEntry->GetSpreadStrengthModifier() / 2.0;
		dRtnValue += pEntry->GetProphetStrengthModifier() / 2.0;
		dRtnValue += (-1 * pEntry->GetProphetCostModifier()) / 2.0;
		dRtnValue += pEntry->GetMissionaryStrengthModifier();
		dRtnValue += (-1 * pEntry->GetMissionaryCostModifier());
	}

	// Chosen LATE?
	else
	{
		// Poor late
		dRtnValue += ((2 * iFlavorExpansion) + pEntry->GetSpreadDistanceModifier()) / 4.0;
		dRtnValue += ((2 * iFlavorScience) + pEntry->GetSpreadStrengthModifier()) / 4.0;
		dRtnValue += pEntry->GetMissionaryStrengthModifier() / 2.0;
		dRtnValue += (-1 * pEntry->GetMissionaryCostModifier()) / 2.0;

		// Good late
		dRtnValue += pEntry->GetProphetStrengthModifier();
		dRtnValue += (-1 * pEntry->GetProphetCostModifier());
	}
#endif
#else
	iRtnValue += iFlavorGP * pEntry->GetGreatPersonExpendedFaith() / 10;
	iRtnValue += iFlavorDiplomacy * pEntry->GetFriendlyCityStateSpreadModifier() / 20;
	iRtnValue += iFlavorDefense * pEntry->GetCombatModifierFriendlyCities() / 4;
	iRtnValue += iFlavorOffense * pEntry->GetCombatModifierEnemyCities() / 4;
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_BETTER_ENHANCER_SCALING
	// Good early, poor late
	iRtnValue += int(((100.0 - dTurnsCompletedPercent) * ((2 * (MIN(iFlavorGrowth, iFlavorExpansion) + iFlavorGrowth)) + pEntry->GetSpreadDistanceModifier()) +
		dTurnsCompletedPercent * (((2 * iFlavorExpansion + iFlavorGrowth) + pEntry->GetSpreadDistanceModifier()) / 4)) / 100.0 + 0.5);
	iRtnValue += int(((100.0 - dTurnsCompletedPercent) * ((2 * (iFlavorScience + MAX(iFlavorGrowth, iFlavorExpansion))) + pEntry->GetSpreadStrengthModifier()) +
		dTurnsCompletedPercent * (((2 * iFlavorScience + iFlavorGrowth) + pEntry->GetSpreadStrengthModifier()) / 4)) / 100.0 + 0.5);
	iRtnValue += int(((100.0 - dTurnsCompletedPercent) * (pEntry->GetMissionaryStrengthModifier() * 2) +
		dTurnsCompletedPercent * (pEntry->GetMissionaryStrengthModifier() / 2)) / 100.0 + 0.5);
	iRtnValue += int(((100.0 - dTurnsCompletedPercent) * ((-1 * pEntry->GetMissionaryCostModifier()) * 2) +
		dTurnsCompletedPercent * ((-1 * pEntry->GetMissionaryCostModifier()) / 2)) / 100.0 + 0.5);

	// Poor early, good late
	iRtnValue += int(((100.0 - dTurnsCompletedPercent) * (pEntry->GetProphetStrengthModifier() / 4.0) +
		dTurnsCompletedPercent * (pEntry->GetProphetStrengthModifier())) / 100.0 + 0.5);
	iRtnValue += int(((100.0 - dTurnsCompletedPercent) * ((-1 * pEntry->GetProphetCostModifier()) / 4.0) +
		dTurnsCompletedPercent * (-1 * pEntry->GetProphetCostModifier())) / 100.0 + 0.5);
#else
	// Chosen EARLY?
	if (iReligionsEnhancedPercent < 33)
	{
		// Good early
		iRtnValue += (2 * iFlavorExpansion) + pEntry->GetSpreadDistanceModifier();
		iRtnValue += (2 * iFlavorScience) + pEntry->GetSpreadStrengthModifier();
		iRtnValue += pEntry->GetMissionaryStrengthModifier() * 2;
		iRtnValue += (-1 * pEntry->GetMissionaryCostModifier()) * 2;

		// Poor early
		iRtnValue += pEntry->GetProphetStrengthModifier() / 4;
		iRtnValue += (-1 * pEntry->GetProphetCostModifier()) / 4;
	}

	// Chosen in the MIDDLE?
	else if (iReligionsEnhancedPercent < 66)
	{
		iRtnValue += iFlavorExpansion + pEntry->GetSpreadDistanceModifier() / 2;
		iRtnValue += iFlavorScience + pEntry->GetSpreadStrengthModifier() / 2;
		iRtnValue += pEntry->GetProphetStrengthModifier() / 2;
		iRtnValue += (-1 * pEntry->GetProphetCostModifier()) / 2;
		iRtnValue += pEntry->GetMissionaryStrengthModifier();
		iRtnValue += (-1 * pEntry->GetMissionaryCostModifier());
	}

	// Chosen LATE?
	else
	{
		// Poor late
		iRtnValue += ((2 * iFlavorExpansion) + pEntry->GetSpreadDistanceModifier()) / 4;
		iRtnValue += ((2 * iFlavorScience) + pEntry->GetSpreadStrengthModifier()) / 4;
		iRtnValue += pEntry->GetMissionaryStrengthModifier() / 2;
		iRtnValue += (-1 * pEntry->GetMissionaryCostModifier()) / 2;

		// Good late
		iRtnValue += pEntry->GetProphetStrengthModifier();
		iRtnValue += (-1 * pEntry->GetProphetCostModifier());
	}
#endif
#endif

	//-----------------
	// REFORMATION BELIEFS
	//-----------------
	if (pEntry->FaithPurchaseAllGreatPeople())
	{
		// Count number of GP branches we have still to open and score based on that
		int iTemp = 0;
		PolicyBranchTypes eBranch;
		eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_TRADITION", true /*bHideAssert*/);
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_TWEAKED_FLAVORS
		if (eBranch != NO_POLICY_BRANCH_TYPE && (m_pPlayer->GetPlayerPolicies()->IsPolicyBranchBlocked(eBranch) ||
			(!m_pPlayer->GetPlayerPolicies()->IsPolicyBranchFinished(eBranch) && (m_pPlayer->GetPlayerPolicies()->GetNumPoliciesOwnedInBranch(eBranch) < 2))))
		{
			iTemp++;
		}
		eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_HONOR", true /*bHideAssert*/);
		if (eBranch != NO_POLICY_BRANCH_TYPE && (m_pPlayer->GetPlayerPolicies()->IsPolicyBranchBlocked(eBranch) ||
			(!m_pPlayer->GetPlayerPolicies()->IsPolicyBranchFinished(eBranch) && (m_pPlayer->GetPlayerPolicies()->GetNumPoliciesOwnedInBranch(eBranch) < 2))))
		{
			iTemp++;
		}
		eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_AESTHETICS", true /*bHideAssert*/);
		if (eBranch != NO_POLICY_BRANCH_TYPE && (m_pPlayer->GetPlayerPolicies()->IsPolicyBranchBlocked(eBranch) ||
			(!m_pPlayer->GetPlayerPolicies()->IsPolicyBranchFinished(eBranch) && (m_pPlayer->GetPlayerPolicies()->GetNumPoliciesOwnedInBranch(eBranch) < 2))))
		{
			iTemp++;
		}
		eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_COMMERCE", true /*bHideAssert*/);
		if (eBranch != NO_POLICY_BRANCH_TYPE && (m_pPlayer->GetPlayerPolicies()->IsPolicyBranchBlocked(eBranch) ||
			(!m_pPlayer->GetPlayerPolicies()->IsPolicyBranchFinished(eBranch) && (m_pPlayer->GetPlayerPolicies()->GetNumPoliciesOwnedInBranch(eBranch) < 2))))
		{
			iTemp++;
		}
		eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_EXPLORATION", true /*bHideAssert*/);
		if (eBranch != NO_POLICY_BRANCH_TYPE && (m_pPlayer->GetPlayerPolicies()->IsPolicyBranchBlocked(eBranch) ||
			(!m_pPlayer->GetPlayerPolicies()->IsPolicyBranchFinished(eBranch) && (m_pPlayer->GetPlayerPolicies()->GetNumPoliciesOwnedInBranch(eBranch) < 2))))
		{
			iTemp++;
		}
		eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_RATIONALISM", true /*bHideAssert*/);
		if (eBranch != NO_POLICY_BRANCH_TYPE && (m_pPlayer->GetPlayerPolicies()->IsPolicyBranchBlocked(eBranch) ||
			(!m_pPlayer->GetPlayerPolicies()->IsPolicyBranchFinished(eBranch) && (m_pPlayer->GetPlayerPolicies()->GetNumPoliciesOwnedInBranch(eBranch) < 2))))
		{
			iTemp++;
		}

#ifdef AUI_RELIGION_USE_DOUBLES
		dRtnValue += iTemp * iTemp * 2.0 * iFlavorGP / (double)GC.getPERSONALITY_FLAVOR_MAX_VALUE();
#else
		dRtnValue += int(iTemp * iTemp * 2.0 * iFlavorGP / (double)GC.getPERSONALITY_FLAVOR_MAX_VALUE() + 0.5);
#endif
#else
		if (eBranch != NO_POLICY_BRANCH_TYPE && (!m_pPlayer->GetPlayerPolicies()->IsPolicyBranchFinished(eBranch) || m_pPlayer->GetPlayerPolicies()->IsPolicyBranchBlocked(eBranch)))
		{
			iTemp++;
		}
		eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_HONOR", true /*bHideAssert*/);
		if (eBranch != NO_POLICY_BRANCH_TYPE && (!m_pPlayer->GetPlayerPolicies()->IsPolicyBranchFinished(eBranch) || m_pPlayer->GetPlayerPolicies()->IsPolicyBranchBlocked(eBranch)))
		{
			iTemp++;
		}
		eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_AESTHETICS", true /*bHideAssert*/);
		if (eBranch != NO_POLICY_BRANCH_TYPE && (!m_pPlayer->GetPlayerPolicies()->IsPolicyBranchFinished(eBranch) || m_pPlayer->GetPlayerPolicies()->IsPolicyBranchBlocked(eBranch)))
		{
			iTemp++;
		}
		eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_COMMERCE", true /*bHideAssert*/);
		if (eBranch != NO_POLICY_BRANCH_TYPE && (!m_pPlayer->GetPlayerPolicies()->IsPolicyBranchFinished(eBranch) || m_pPlayer->GetPlayerPolicies()->IsPolicyBranchBlocked(eBranch)))
		{
			iTemp++;
		}
		eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_EXPLORATION", true /*bHideAssert*/);
		if (eBranch != NO_POLICY_BRANCH_TYPE && (!m_pPlayer->GetPlayerPolicies()->IsPolicyBranchFinished(eBranch) || m_pPlayer->GetPlayerPolicies()->IsPolicyBranchBlocked(eBranch)))
		{
			iTemp++;
		}
		eBranch = (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_RATIONALISM", true /*bHideAssert*/);
		if (eBranch != NO_POLICY_BRANCH_TYPE && (!m_pPlayer->GetPlayerPolicies()->IsPolicyBranchFinished(eBranch) || m_pPlayer->GetPlayerPolicies()->IsPolicyBranchBlocked(eBranch)))
		{
			iTemp++;
		}

#ifdef AUI_RELIGION_USE_DOUBLES
		dRtnValue += (iTemp * 6);
#else
		iRtnValue += (iTemp * 6);
#endif
#endif
	}
	if (pEntry->ConvertsBarbarians())
	{
		MilitaryAIStrategyTypes eStrategyBarbs = (MilitaryAIStrategyTypes) GC.getInfoTypeForString("MILITARYAISTRATEGY_ERADICATE_BARBARIANS");
		if (m_pPlayer->GetMilitaryAI()->IsUsingStrategy(eStrategyBarbs))
		{
#ifdef AUI_RELIGION_USE_DOUBLES
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_TWEAKED_FLAVORS
			dRtnValue += 10;
		}
		else
		{
			dRtnValue += 5;
		}
		dRtnValue += m_pPlayer->GetMilitaryAI()->GetBarbarianThreatTotal();
#else
			dRtnValue += 40;
	}
		else
		{
			dRtnValue += 15;
		}
#endif
#else
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_TWEAKED_FLAVORS
			iRtnValue += 10;
		}
		else
		{
			iRtnValue += 5;
		}
		iRtnValue += m_pPlayer->GetMilitaryAI()->GetBarbarianThreatTotal();
#else
			iRtnValue += 40;
		}
		else
		{
			iRtnValue += 15;
		}
#endif
#endif
	}
#ifdef AUI_RELIGION_USE_DOUBLES
	if (pEntry->GetCityStateInfluenceModifier() > 0)
	{
#if defined(AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_CONSIDER_GRAND_STRATEGY) && defined(AUI_GS_PRIORITY_RATIO)
#ifdef AUI_MINOR_CIV_RATIO
		dRtnValue += pEntry->GetCityStateInfluenceModifier() * iFlavorDiplomacy * dCityStateDeviation / 5.0 *
			(0.5 + 1.5 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS")));
#else
		dRtnValue += pEntry->GetCityStateInfluenceModifier() * iFlavorDiplomacy / 5.0 *
			(0.5 + 1.5 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS")));
#endif
#else
#ifdef AUI_MINOR_CIV_RATIO
		dRtnValue += pEntry->GetCityStateInfluenceModifier() * iFlavorDiplomacy * dCityStateDeviation / 5.0;
#else
		dRtnValue += pEntry->GetCityStateInfluenceModifier() * iFlavorDiplomacy / 5.0;
#endif
#endif
	}
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_TWEAKED_FLAVORS
	if (pEntry->GetInquisitorPressureRetention() > 0)
	{
		dRtnValue += pEntry->GetInquisitorPressureRetention() / 16.0;
	}
	if (pEntry->GetSpyPressure() > 0)
	{
#if defined(AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_CONSIDER_GRAND_STRATEGY) && defined(AUI_GS_PRIORITY_RATIO)
#ifdef AUI_MINOR_CIV_RATIO
		dRtnValue += pEntry->GetSpyPressure() * iFlavorDiplomacy / 20.0 * dCityStateDeviation *
			(m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS")));
#else
		dRtnValue += pEntry->GetSpyPressure() * iFlavorDiplomacy / 20.0 *
			(m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS")));
#endif
#else
#ifdef AUI_MINOR_CIV_RATIO
		dRtnValue += pEntry->GetSpyPressure() * iFlavorDiplomacy / 20.0 * dCityStateDeviation;
#else
		dRtnValue += pEntry->GetSpyPressure() * iFlavorDiplomacy / 20.0;
#endif
#endif
	}
	if (pEntry->GetFaithBuildingTourism() > 0)
	{
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_TOURISM_FROM_BUILDINGS_COUNTS_BUILDINGS
#ifdef AUI_RELIGION_FIX_MULTIPLE_FAITH_BUILDINGS
		int iBuildingCount = FaithBuildingAvailable(m_pPlayer->GetReligions()->GetReligionCreatedByPlayer()).size() * m_pPlayer->getNumCities();
#else
		int iBuildingCount = (FaithBuildingAvailable(m_pPlayer->GetReligions()->GetReligionCreatedByPlayer()) != NO_BUILDINGCLASS ? 1 : 0) * m_pPlayer->getNumCities();
#endif
#if defined(AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_CONSIDER_GRAND_STRATEGY) && defined(AUI_GS_PRIORITY_RATIO)
		dRtnValue += (pEntry->GetFaithBuildingTourism() * iBuildingCount * iFlavorCulture * (0.5 + 2 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))));
#elif defined(AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_CONSIDER_GRAND_STRATEGY)
		dRtnValue += (pEntry->GetFaithBuildingTourism() * iBuildingCount * iFlavorCulture * (m_pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE") ? 2.5 : 0.5));
#else
		dRtnValue += (pEntry->GetFaithBuildingTourism() * iBuildingCount * 1.5 * iFlavorCulture);
#endif
#elif defined(AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_CONSIDER_GRAND_STRATEGY) && defined(AUI_GS_PRIORITY_RATIO)
		dRtnValue += (pEntry->GetFaithBuildingTourism() * 2 * iFlavorCulture * (0.5 + 2 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))));
#elif defined(AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_CONSIDER_GRAND_STRATEGY)
		dRtnValue += (pEntry->GetFaithBuildingTourism() * 2 * iFlavorCulture * (m_pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE") ? 2.5 : 0.5));
#else
		dRtnValue += (pEntry->GetFaithBuildingTourism() * 3.0 * iFlavorCulture);
#endif
	}
#else
	if (pEntry->GetInquisitorPressureRetention() > 0)
	{
		dRtnValue += (pEntry->GetInquisitorPressureRetention() / 2.0);
	}
	if (pEntry->GetSpyPressure() > 0)
	{
		dRtnValue += (pEntry->GetSpyPressure() * 5.0);
	}
	if (pEntry->GetFaithBuildingTourism() > 0)
	{
		dRtnValue += (pEntry->GetFaithBuildingTourism() * 20.0);
	}
#endif

	return dRtnValue;
#else
	if (pEntry->GetCityStateInfluenceModifier() > 0)
	{
#ifdef AUI_MINOR_CIV_RATIO
		iRtnValue += int(pEntry->GetCityStateInfluenceModifier() * iFlavorDiplomacy * dCityStateDeviation / 5.0 + 0.5);
#else
		iRtnValue += (pEntry->GetCityStateInfluenceModifier() * iFlavorDiplomacy / 5);
#endif
	}
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_TWEAKED_FLAVORS
	if (pEntry->GetInquisitorPressureRetention() > 0)
	{
		iRtnValue += int(pEntry->GetInquisitorPressureRetention() / 2.5 + 0.5);
	}
	if (pEntry->GetSpyPressure() > 0)
	{
#ifdef AUI_MINOR_CIV_RATIO
		iRtnValue += int(pEntry->GetSpyPressure() * iFlavorDiplomacy / 2.0 * dCityStateDeviation + 0.5);
#else
		iRtnValue += (pEntry->GetSpyPressure() * iFlavorDiplomacy / 2);
#endif
	}
	if (pEntry->GetFaithBuildingTourism() > 0)
	{
#ifdef AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_CONSIDER_GRAND_STRATEGY
#ifdef AUI_GS_PRIORITY_RATIO
		iRtnValue += int((pEntry->GetFaithBuildingTourism() * 2 * iFlavorCulture * (0.5 + 2 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE")))) + 0.5);
#else
		iRtnValue += int((pEntry->GetFaithBuildingTourism() * 2 * iFlavorCulture * (m_pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE") ? 2.5 : 0.5)) + 0.5);
#endif
#else
		iRtnValue += (pEntry->GetFaithBuildingTourism() * 3 * iFlavorCulture);
#endif
	}
#else
	if (pEntry->GetInquisitorPressureRetention() > 0)
	{
		iRtnValue += (pEntry->GetInquisitorPressureRetention() / 2);
	}
	if (pEntry->GetSpyPressure() > 0)
	{
		iRtnValue += (pEntry->GetSpyPressure()* 5);
	}
	if (pEntry->GetFaithBuildingTourism() > 0)
	{
		iRtnValue += (pEntry->GetFaithBuildingTourism() * 20);
	}
#endif

	return iRtnValue;
#endif
}

/// AI's evaluation of this city as a target for a missionary
#ifdef AUI_RELIGION_TWEAKED_MISSIONARY_INQUISITOR_SCORING
int CvReligionAI::ScoreCityForMissionary(const CvCity* pCity, UnitHandle pUnit, const CvPlot* pUnitPlot, int iMissionaryMoves, int iMissionaryStrength) const
#elif defined(AUI_CONSTIFY)
int CvReligionAI::ScoreCityForMissionary(const CvCity* pCity, UnitHandle pUnit) const
#else
int CvReligionAI::ScoreCityForMissionary(CvCity* pCity, UnitHandle pUnit)
#endif
{
	int iScore = 0;
	ReligionTypes eMyReligion = GetReligionToSpread();

#ifdef AUI_RELIGION_TWEAKED_MISSIONARY_INQUISITOR_SCORING
	if (eMyReligion <= RELIGION_PANTHEON)
	{
		return 0;
	}

	const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(eMyReligion, pCity->getOwner());
	if (!pReligion)
	{
		return 0;
	}
#endif

	// Skip if not revealed
	if(!pCity->plot()->isRevealed(m_pPlayer->getTeam()))
	{
		return iScore;
	}

#ifdef AUI_RELIGION_FIX_SCORE_CITY_FOR_MISSIONARY_NO_WAR_TARGETTING
	// Skip if at war with city owner
	if (GET_TEAM(m_pPlayer->getTeam()).isAtWar(GET_PLAYER(pCity->getOwner()).getTeam()))
	{
		return 0;
	}
#endif

	// Skip if already our religion
	if(pCity->GetCityReligions()->GetReligiousMajority() == eMyReligion)
	{
		return iScore;
	}

#ifdef AUI_RELIGION_TWEAKED_MISSIONARY_INQUISITOR_SCORING
	iScore = 100;

	CvPlayer& kCityPlayer = GET_PLAYER(pCity->getOwner());
	if (!kCityPlayer.isMinorCiv())
		if (m_pPlayer->GetDiplomacyAI()->IsPlayerAgreeNotToConvert(pCity->getOwner()))
			return 0;

	// Holy cities shouldn't be converted by missionaries, there should be enough passive pressure to keep them converted without having to use missionaries
	ReligionTypes eCityOwnersReligion = kCityPlayer.GetReligions()->GetReligionCreatedByPlayer();
	if (eCityOwnersReligion > RELIGION_PANTHEON && pCity->GetCityReligions()->IsHolyCityForReligion(eCityOwnersReligion))
		return 0;

	// We don't actually need a missionary if passive pressure is enough to eventually convert the city
	if (IsPressureLeader(pCity, eMyReligion))
		return 0;

	// Look for cities we would gain pressure on by converting this city
	int iRange = GC.getRELIGION_ADJACENT_CITY_DISTANCE();
	// Boost to distance due to belief?
	int iDistanceMod = pReligion->m_Beliefs.GetSpreadDistanceModifier();
	if(iDistanceMod > 0)
	{
		iRange *= (100 + iDistanceMod);
		iRange /= 100;
	}

	// The pressure this city would output if converted
	int iBasePressure = GC.getGame().getGameSpeedInfo().getReligiousPressureAdjacentCity();

	// Have a belief that always strengthens spread?
	int iStrengthMod = pReligion->m_Beliefs.GetSpreadStrengthModifier();
	if(iStrengthMod > 0)
	{
		TechTypes eDoublingTech = pReligion->m_Beliefs.GetSpreadModifierDoublingTech();
		if(eDoublingTech != NO_TECH)
		{
			if(GET_TEAM(m_pPlayer->getTeam()).GetTeamTechs()->HasTech(eDoublingTech))
			{
				iStrengthMod *= 2;
			}
		}
		iBasePressure *= (100 + iStrengthMod);
		iBasePressure /= 100;
	}

	// Strengthened spread from World Congress? (World Religion)
	int iLeaguesMod = GC.getGame().GetGameLeagues()->GetReligionSpreadStrengthModifier(pCity->getOwner(), eMyReligion);
	if (iLeaguesMod > 0)
	{
		iBasePressure *= (100 + iLeaguesMod);
		iBasePressure /= 100;
	}

	// Building that boosts pressure from originating city?
	int iModifier = pCity->GetCityReligions()->GetReligiousPressureModifier();
	if (iModifier > 0)
	{
		iBasePressure *= (100 + iModifier);
		iBasePressure /= 100;
	}

	CvPlot* pLoopPlot;
	CvCity* pLoopCity;
	int iPressuredCitiesScore = 0;
	int iPressure = 0;
	int iTempScore = 0;
	for(int iDY = -iRange; iDY <= iRange; iDY++)
	{
		int iMaxDX = iRange - MAX(0, iDY);
		for (int iDX = -iRange - MIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
		{
			pLoopPlot = plotXY(pCity->getX(), pCity->getY(), iDX, iDY);
			if (pLoopPlot)
			{
				pLoopCity = pLoopPlot->getPlotCity();
				if (pLoopCity && (pLoopPlot->isRevealed(m_pPlayer->getTeam()) || pCity->GetCityReligions()->GetReligiousMajority() > RELIGION_PANTHEON))
				{
					iTempScore = 1;
					iPressure = iBasePressure;
					if (GC.getGame().GetGameTrade()->IsCityConnectedFromCityToCity(pCity, pLoopCity))
					{
						int iTradeReligionModifer = kCityPlayer.GetPlayerTraits()->GetTradeReligionModifier();
						if (iTradeReligionModifer != 0)
						{
							iPressure *= 100 + iTradeReligionModifer;
							iPressure /= 100;
						}
					}

					// If we are spreading to a friendly city state, increase the effectiveness if we have the right belief
					if (GC.getGame().GetGameReligions()->IsCityStateFriendOfReligionFounder(eMyReligion, pLoopCity->getOwner()))
					{
						int iFriendshipMod = pReligion->m_Beliefs.GetFriendlyCityStateSpreadModifier();
						if(iFriendshipMod > 0)
						{
							iPressure *= (100 + iFriendshipMod);
							iPressure /= 100;
						}
					}
					
					if (pCity->GetCityReligions()->GetReligiousMajority() != eMyReligion)
						iTempScore += 1;
					if (pCity->GetCityReligions()->IsHolyCityAnyReligion() && !pCity->GetCityReligions()->IsHolyCityForReligion(eMyReligion))
						iTempScore += 1;
					if (!IsPressureLeader(pLoopCity, eMyReligion))
					{
						if (IsPressureLeader(pLoopCity, eMyReligion, iPressure))
							iTempScore *= 2;
						else
							iTempScore += 1;
					}
					iPressuredCitiesScore += iTempScore;
				}
			}
		}
	}
	iScore += iPressuredCitiesScore * iPressuredCitiesScore / 2;

	int iMissionaryPressure = iMissionaryStrength * GC.getRELIGION_MISSIONARY_PRESSURE_MULTIPLIER();
	if (pUnit)
		iMissionaryPressure = pUnit->GetReligionData()->GetReligiousStrength() * GC.getRELIGION_MISSIONARY_PRESSURE_MULTIPLIER();
	if (pReligion->m_Beliefs.GetMissionaryStrengthModifier() > 0)
		iMissionaryPressure = iMissionaryPressure * (100 + pReligion->m_Beliefs.GetMissionaryStrengthModifier()) / 100;
	if (ShouldBecomeNewMajority(pCity, eMyReligion, iMissionaryPressure))
		iScore *= 2;
	if (pCity->getPopulation() - pCity->GetCityReligions()->GetFollowersOtherReligions(NO_RELIGION) <= iMissionaryPressure / 100)
	{
		iScore /= 2;
		if (pCity->getPopulation() - pCity->GetCityReligions()->GetFollowersOtherReligions(NO_RELIGION) <= 0)
			iScore /= 4;
	}
	if (pCity->getOwner() == m_pPlayer->GetID() || !kCityPlayer.GetReligions()->HasCreatedReligion())
		iScore *= 2;
	else if (!kCityPlayer.isMinorCiv() && !GET_TEAM(kCityPlayer.getTeam()).IsAllowsOpenBordersToTeam(m_pPlayer->getTeam()))
		iScore /= 2;

	int iDistance = MAX_INT;
	if (pUnit)
#ifdef AUI_ASTAR_TURN_LIMITER
		iDistance = TurnsToReachTarget(pUnit, pCity->plot(), true /*bReusePaths*/, true /*bIgnoreUnits*/, false, iScore);
#else
		iDistance = TurnsToReachTarget(pUnit, pCity->plot(), true /*bReusePaths*/, true /*bIgnoreUnits*/);
#endif
	else if (pUnitPlot)
	{
		if (GC.getStepFinder().GeneratePath(pUnitPlot->getX(), pUnitPlot->getY(), pCity->getX(), pCity->getY(), MOVE_UNITS_IGNORE_DANGER, true))
			if (GC.getStepFinder().GetLastNode())
				iDistance = GC.getStepFinder().GetLastNode()->m_iData2 / iMissionaryMoves + 1;
	}
	// Then subtract turn time squared
	iScore -= iDistance * iDistance / 2;
	if (iScore < 0)
		return 0;
#else

	// Base score based on if we are establishing majority
	iScore = 100;
	if(ShouldBecomeNewMajority(pCity, eMyReligion, pUnit->GetReligionData()->GetReligiousStrength() * GC.getRELIGION_MISSIONARY_PRESSURE_MULTIPLIER()))
	{
		iScore *= 2;
	}

	if (!GET_PLAYER(pCity->getOwner()).isMinorCiv())
	{
		if (m_pPlayer->GetDiplomacyAI()->IsPlayerAgreeNotToConvert(pCity->getOwner()))
		{
			return 0;
		}
	}

	CvPlayer& kCityPlayer = GET_PLAYER(pCity->getOwner());
	// Much better score if our own city or if this city owner isn't starting a religion
	if(pCity->getOwner() == m_pPlayer->GetID())
	{
		iScore *= 5;
	}
	else if(!kCityPlayer.GetReligions()->HasCreatedReligion())
	{
		iScore *= 3;
	}

	// Then subtract distance
	iScore -= plotDistance(pUnit->getX(), pUnit->getY(), pCity->getX(), pCity->getY());
#endif

#ifndef AUI_RELIGION_FIX_SCORE_CITY_FOR_MISSIONARY_NO_WAR_TARGETTING
	// Multiplier by how safe it is
	if(!atWar(m_pPlayer->getTeam(), kCityPlayer.getTeam()))
	{
		iScore *= 2;
	}
#endif

#ifndef AUI_RELIGION_TWEAKED_MISSIONARY_INQUISITOR_SCORING
	// Holy city will anger folks, let's not do that one right away
	ReligionTypes eCityOwnersReligion = kCityPlayer.GetReligions()->GetReligionCreatedByPlayer();
	if (eCityOwnersReligion > RELIGION_PANTHEON && pCity->GetCityReligions()->IsHolyCityForReligion(eCityOwnersReligion))
	{
		iScore /= 2;
	}
#endif

	return iScore;
}

/// AI's evaluation of this city as a target for an inquisitor
#ifdef AUI_RELIGION_TWEAKED_MISSIONARY_INQUISITOR_SCORING
int CvReligionAI::ScoreCityForInquisitor(const CvCity* pCity, UnitHandle pUnit) const
{
	CvCityReligions* pCityReligions = pCity->GetCityReligions();
	ReligionTypes eMyReligion = GetReligionToSpread();
	ReligionTypes eCityReligion = pCityReligions->GetReligiousMajority();

	// Cleanse heretical holy cities!
	if(pCity->GetCityReligions()->IsHolyCityAnyReligion() && !pCity->GetCityReligions()->IsHolyCityForReligion(eMyReligion))
		return 1000;

	// Skip if already our religion
	if(eCityReligion == eMyReligion)
		return 0;

	// Base score based on if we are establishing majority
	int iScore = 100;

	if (IsPressureLeader(pCity, eMyReligion))
	{
		int iNumTradeRoutes = 0;
		int iOurPressure = pCityReligions->GetPressure(eMyReligion);
		int iOurPressurePerTurn = pCityReligions->GetPressurePerTurn(eMyReligion, iNumTradeRoutes);
		int iTotalPressure = pCityReligions->GetTotalPressure();
		int iTotalPressurePerTurn = 0;
		for(ReligionInCityList::iterator it = pCityReligions->m_ReligionStatus.begin(); it != pCityReligions->m_ReligionStatus.end(); ++it)
		{
			iTotalPressurePerTurn += pCityReligions->GetPressurePerTurn(it->m_eReligion, iNumTradeRoutes);
		}
		int iDivisor = 2 * iOurPressurePerTurn - iTotalPressurePerTurn;
		if (iDivisor != 0)
		{
			int iTurnsToConvert = (iTotalPressure - 2 * iOurPressure) / iDivisor;
			if (iTurnsToConvert >= 0 && iTurnsToConvert < 100)
				iScore = 50 + iTurnsToConvert / 2;
		}
	}

	int iLoop;
	int iTempScore = 0;
	for(CvCity* pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
	{
		int iNumTradeRoutes;
		if(0 < GC.getGame().GetGameReligions()->GetAdjacentCityReligiousPressure(eCityReligion, pCity, pLoopCity, iNumTradeRoutes, false))
		{
			iTempScore++;
			if (!IsPressureLeader(pLoopCity, eMyReligion))
				iTempScore++;
		}
	}
	iScore += iTempScore * iTempScore / 2;

	// Then subtract distance
#ifdef AUI_ASTAR_TURN_LIMITER
	int iDistance = TurnsToReachTarget(pUnit, pCity->plot(), true /*bReusePaths*/, true /*bIgnoreUnits*/, false, iScore);
#else
	int iDistance = TurnsToReachTarget(pUnit, pCity->plot(), true /*bReusePaths*/, true /*bIgnoreUnits*/);
#endif
	iScore -= iDistance * iDistance / 2;
	if (iScore < 0)
		return 0;
#else
#if defined(AUI_CONSTIFY)
int CvReligionAI::ScoreCityForInquisitor(const CvCity* pCity, UnitHandle pUnit) const
#else
int CvReligionAI::ScoreCityForInquisitor(CvCity* pCity, UnitHandle pUnit) const
#endif
{
	int iScore = 0;
	ReligionTypes eMyReligion = GetReligionToSpread();

	// Skip if already our religion
	if(pCity->GetCityReligions()->GetReligiousMajority() == eMyReligion)
	{
		return iScore;
	}

	// Base score based on if we are establishing majority
	iScore = 100;

	// Then subtract distance
	iScore -= plotDistance(pUnit->getX(), pUnit->getY(), pCity->getX(), pCity->getY());
#endif

	return iScore;
}

#ifdef AUI_RELIGION_TWEAKED_MISSIONARY_INQUISITOR_SCORING
/// Is a religion the leader in passive pressure? (ie. would a city flip religion passively from pressure?)
bool CvReligionAI::IsPressureLeader(const CvCity* pCity, ReligionTypes eReligion, int iExtraPressure) const
{
	CvCityReligions* pCityReligions = pCity->GetCityReligions();
	int iNumTradeRoutesInvolved = 0;
	int iMyPressure = pCityReligions->GetPressurePerTurn(eReligion, iNumTradeRoutesInvolved) + iExtraPressure;
	if (iMyPressure <= 0)
		return false;

	for(ReligionInCityList::iterator it = pCityReligions->m_ReligionStatus.begin(); it != pCityReligions->m_ReligionStatus.end(); ++it)
	{
		if (it->m_eReligion != eReligion && pCityReligions->GetPressurePerTurn(it->m_eReligion, iNumTradeRoutesInvolved) >= iMyPressure)
			return false;
	}
	return true;
}
#endif

/// Is this enough new pressure to establish this as the new religious majority?
#ifdef AUI_CONSTIFY
bool CvReligionAI::ShouldBecomeNewMajority(const CvCity* pCity, ReligionTypes eReligion, int iNewPressure) const
#else
bool CvReligionAI::ShouldBecomeNewMajority(CvCity* pCity, ReligionTypes eReligion, int iNewPressure) const
#endif
{
	CvCityReligions* pCityReligions = pCity->GetCityReligions();
#ifdef AUI_RELIGION_TWEAKED_MISSIONARY_INQUISITOR_SCORING
	if (GC.getGame().GetGameReligions()->GetReligion(eReligion, pCity->getOwner())->m_Beliefs.GetOtherReligionPressureErosion() > 0)
		return pCityReligions->GetMajorityReligionAfterSpread(eReligion, iNewPressure) == eReligion;
#endif
	int iTotalPressure = pCityReligions->GetTotalPressure() + iNewPressure;
	int iReligionPressure = pCityReligions->GetPressure(eReligion) + iNewPressure;

	return (iReligionPressure * 2) > iTotalPressure;
}

/// Are all of our own cities our religion?
bool CvReligionAI::AreAllOurCitiesConverted(ReligionTypes eReligion, bool bIncludePuppets) const
{
	bool bRtnValue = true;

	int iLoop;
	CvCity* pLoopCity;
	for(pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
	{
		if(pLoopCity->GetCityReligions()->GetReligiousMajority() != eReligion)
		{
			if(bIncludePuppets || !pLoopCity->IsPuppet())
			{
				bRtnValue = false;
				break;
			}
		}
	}

	return bRtnValue;
}

/// Do all of our own cities have this religion's faith building if possible?
bool CvReligionAI::AreAllOurCitiesHaveFaithBuilding(ReligionTypes eReligion, bool bIncludePuppets) const
{
	bool bRtnValue = true;
#ifdef AUI_RELIGION_FIX_ARE_ALL_OUR_CITIES_HAVE_FAITH_BUILDING_VENICE_PUPPETS
	if (m_pPlayer->GetPlayerTraits()->IsNoAnnexing())
		bIncludePuppets = true;
#endif
#ifdef AUI_RELIGION_FIX_MULTIPLE_FAITH_BUILDINGS
	std::vector<BuildingClassTypes> eFaithBuildingClass = FaithBuildingAvailable(eReligion);
	if (eFaithBuildingClass.size() == 0)
	{
		return true;
	}
	for (std::vector<BuildingClassTypes>::iterator it = eFaithBuildingClass.begin(); it != eFaithBuildingClass.end() && bRtnValue; ++it)
	{
		BuildingTypes eFaithBuilding = (BuildingTypes)m_pPlayer->getCivilizationInfo().getCivilizationBuildings((*it));

		if (eFaithBuilding == NO_BUILDING)
		{
			continue;
		}
#else
	BuildingClassTypes eFaithBuildingClass = FaithBuildingAvailable(eReligion);
	BuildingTypes eFaithBuilding = (BuildingTypes)m_pPlayer->getCivilizationInfo().getCivilizationBuildings(eFaithBuildingClass);

	if(eFaithBuildingClass == NO_BUILDINGCLASS || eFaithBuilding == NO_BUILDING)
	{
		return true;
	}
#endif

	int iLoop;
	CvCity* pLoopCity;
	for(pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
	{
		if(pLoopCity->GetCityReligions()->GetReligiousMajority() == eReligion)
		{
			if(bIncludePuppets || !pLoopCity->IsPuppet())
			{
				if(pLoopCity->GetCityBuildings()->GetNumBuilding(eFaithBuilding) < 1)
				{
					bRtnValue = false;
					break;
				}
			}
		}
	}
#ifdef AUI_RELIGION_FIX_MULTIPLE_FAITH_BUILDINGS
	}
#endif

	return bRtnValue;
}

// Is there a civ nearby that isn't pressing religion?
#ifdef AUI_RELIGION_TWEAKED_MISSIONARY_INQUISITOR_SCORING
bool CvReligionAI::HaveNearbyConversionTarget(ReligionTypes /*eReligion*/, bool bCanIncludeReligionStarter) const
#else
bool CvReligionAI::HaveNearbyConversionTarget(ReligionTypes eReligion, bool bCanIncludeReligionStarter) const
#endif
{
	int iLoop;
	bool bStartedOwnReligion;
#ifndef AUI_RELIGION_TWEAKED_MISSIONARY_INQUISITOR_SCORING
	TeamTypes eTeam = m_pPlayer->getTeam();
#endif
	UnitTypes eMissionary = (UnitTypes)GC.getInfoTypeForString("UNIT_MISSIONARY");
	int iMissionaryMoves = GC.getUnitInfo(eMissionary)->GetMoves();
#ifdef AUI_RELIGION_TWEAKED_MISSIONARY_INQUISITOR_SCORING
	int iLoop2;
	CvCity* pLoopCity;
	int iMissionaryStrength = GC.getUnitInfo(eMissionary)->GetReligiousStrength();
#endif

	CvCity* pCapital = m_pPlayer->getCapitalCity();
	if(pCapital == NULL)
	{
		return false;
	}

	for(int iPlayer = 0; iPlayer < MAX_PLAYERS; iPlayer++)
	{
		PlayerTypes ePlayer = (PlayerTypes)iPlayer;

		// Must be someone else
		if(ePlayer == m_pPlayer->GetID())
		{
			continue;

		}
		CvPlayer& kPlayer = GET_PLAYER(ePlayer);
		if(kPlayer.isAlive())
		{
			if(kPlayer.isMinorCiv())
			{
				bStartedOwnReligion = false;
			}
			else
			{
				if (!kPlayer.isBarbarian())
				{
					if (m_pPlayer->GetDiplomacyAI()->IsPlayerAgreeNotToConvert(ePlayer))
					{
						continue;
					}
				}

				if(kPlayer.GetReligionAI()->GetReligionToSpread() > RELIGION_PANTHEON)
				{
					bStartedOwnReligion = true;
				}
				else
				{
					bStartedOwnReligion = false;
				}
			}

			if(!bStartedOwnReligion || bCanIncludeReligionStarter)
			{
				for(CvCity* pCity = kPlayer.firstCity(&iLoop); pCity != NULL; pCity = kPlayer.nextCity(&iLoop))
				{
#ifdef AUI_RELIGION_TWEAKED_MISSIONARY_INQUISITOR_SCORING
#ifdef AUI_RELIGION_CONVERSION_TARGET_NOT_JUST_CAPITAL
					for(pLoopCity = m_pPlayer->firstCity(&iLoop2); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop2))
					{
						if (!pLoopCity->IsPuppet() || m_pPlayer->GetPlayerTraits()->IsNoAnnexing())
							if (plotDistance(pLoopCity->getX(), pLoopCity->getY(), pCity->getX(), pCity->getY()) <= iMissionaryMoves * GC.getRELIGION_MISSIONARY_RANGE_IN_TURNS())
								if (ScoreCityForMissionary(pCity, NULL, pLoopCity->plot(), iMissionaryMoves, iMissionaryStrength) > 0)
									return true;
					}
#else
					if (ScoreCityForMissionary(pCity, NULL, pCapital->plot(), iMissionaryMoves, iMissionaryStrength) > 0)
						return true;
#endif
#else
#ifdef AUI_RELIGION_HAVE_NEARBY_CONVERSION_TARGET_IGNORE_TARGET_THAT_WILL_CONVERT_PASSIVELY
					ReligionTypes eMajorityReligion = pCity->GetCityReligions()->GetReligiousMajority();
					// Revealed, not at war, not currently our religion
					if (pCity->isRevealed(eTeam, false) && !GET_TEAM(m_pPlayer->getTeam()).isAtWar(GET_PLAYER(pCity->getOwner()).getTeam())
						&& eMajorityReligion != eReligion)
					{
						CvCityReligions* pCityReligions = pCity->GetCityReligions();
						// Religion wouldn't take over city passively
						if (!IsPressureLeader(pCity, eReligion))
						{
#ifdef AUI_RELIGION_CONVERSION_TARGET_NOT_JUST_CAPITAL
							int iLoop2;
							CvCity* pLoopCity;
							for(pLoopCity = m_pPlayer->firstCity(&iLoop2); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop2))
							{
								if (!pLoopCity->IsPuppet() || m_pPlayer->GetPlayerTraits()->IsNoAnnexing())
								{
									// Within 8 times Missionary movement allowance of one of our non-puppet cities
									if (plotDistance(pLoopCity->getX(), pLoopCity->getY(), pCity->getX(), pCity->getY()) <= (iMissionaryMoves * GC.getRELIGION_MISSIONARY_RANGE_IN_TURNS()))
									{
										return true;
									}
								}
							}
#else
							// Within 8 times Missionary movement allowance of our capital
							if (plotDistance(pCapital->getX(), pCapital->getY(), pCity->getX(), pCity->getY()) <= (iMissionaryMoves * GC.getRELIGION_MISSIONARY_RANGE_IN_TURNS()))
							{
								return true;
							}
#endif
						}
					}
#else
					// Revealed and not currently our religion
					if(pCity->isRevealed(eTeam, false) && pCity->GetCityReligions()->GetReligiousMajority() != eReligion)
					{
#ifdef AUI_RELIGION_CONVERSION_TARGET_NOT_JUST_CAPITAL
						int iLoop;
						CvCity* pLoopCity;
						for(pLoopCity = m_pPlayer->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = m_pPlayer->nextCity(&iLoop))
						{
							if (!pLoopCity->IsPuppet() || m_pPlayer->GetPlayerTraits()->IsNoAnnexing())
							{
								// Within 8 times Missionary movement allowance of one of our non-puppet cities
								if (plotDistance(pLoopCity->getX(), pLoopCity->getY(), pCity->getX(), pCity->getY()) <= (iMissionaryMoves * GC.getRELIGION_MISSIONARY_RANGE_IN_TURNS()))
								{
									return true;
								}
							}
						}
#else
						// Within 10 times Missionary movement allowance of our
						if(plotDistance(pCapital->getX(), pCapital->getY(), pCity->getX(), pCity->getY()) <= (iMissionaryMoves * GC.getRELIGION_MISSIONARY_RANGE_IN_TURNS()))
						{
							return true;
						}
#endif
					}
#endif
#endif
				}
			}
		}
	}
	return false;
}

// Do we have as many Inquisitors as we need
bool CvReligionAI::HaveEnoughInquisitors(ReligionTypes eReligion) const
{
	int iLoop;

#ifdef AUI_RELIGION_HAVE_ENOUGH_INQUISITORS_TWEAKED_BASE_NUMER
	// Need one for every city in our realm that is of another religion
	int iNumNeeded = AUI_RELIGION_HAVE_ENOUGH_INQUISITORS_TWEAKED_BASE_NUMER;
#else
	// Need one for every city in our realm that is of another religion, plus one for defense
	int iNumNeeded = 1;
#endif
	for(CvCity* pCity = m_pPlayer->firstCity(&iLoop); pCity != NULL; pCity = m_pPlayer->nextCity(&iLoop))
	{
		ReligionTypes eCityReligion = pCity->GetCityReligions()->GetReligiousMajority();
#ifdef AUI_RELIGION_TWEAKED_MISSIONARY_INQUISITOR_SCORING
		if(eCityReligion > RELIGION_PANTHEON && eCityReligion != eReligion && (!IsPressureLeader(pCity, eReligion) ||
			(pCity->GetCityReligions()->IsHolyCityAnyReligion() && !pCity->GetCityReligions()->IsHolyCityForReligion(eReligion))))
#else
		if(eCityReligion > RELIGION_PANTHEON && eCityReligion != eReligion)
#endif
		{
			iNumNeeded++;
		}
	}

	// Count Inquisitors of our religion
	int iNumInquisitors = 0;
	for(CvUnit* pUnit = m_pPlayer->firstUnit(&iLoop); pUnit != NULL; pUnit = m_pPlayer->nextUnit(&iLoop))
	{
		if(pUnit->getUnitInfo().IsRemoveHeresy())
		{
			if(pUnit->GetReligionData()->GetReligion() == eReligion)
			{
				iNumInquisitors++;
			}
		}
	}

	return iNumInquisitors >= iNumNeeded;
}

/// Do we have a belief that allows a faith generating building to be constructed?
#ifdef AUI_RELIGION_FIX_MULTIPLE_FAITH_BUILDINGS
std::vector<BuildingClassTypes> CvReligionAI::FaithBuildingAvailable(ReligionTypes eReligion) const
#else
BuildingClassTypes CvReligionAI::FaithBuildingAvailable(ReligionTypes eReligion) const
#endif
{
#ifdef AUI_RELIGION_FIX_MULTIPLE_FAITH_BUILDINGS
	std::vector<BuildingClassTypes> aeFaithBuildings;
#endif
	CvGameReligions* pReligions = GC.getGame().GetGameReligions();
	const CvReligion* pMyReligion = pReligions->GetReligion(eReligion, m_pPlayer->GetID());

	if (pMyReligion)
	{
#ifdef AUI_WARNING_FIXES
		for (uint iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
#else
		for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
#endif
		{
			if (pMyReligion->m_Beliefs.IsBuildingClassEnabled((BuildingClassTypes)iI))
			{
				BuildingTypes eBuilding = (BuildingTypes)m_pPlayer->getCivilizationInfo().getCivilizationBuildings((BuildingClassTypes)iI);
				if(eBuilding != NO_BUILDING)
				{
					CvBuildingEntry* pBuildingEntry = GC.getBuildingInfo(eBuilding);
					if (pBuildingEntry && pBuildingEntry->GetYieldChange(YIELD_FAITH) > 0)
					{
#ifdef AUI_RELIGION_FIX_MULTIPLE_FAITH_BUILDINGS
						aeFaithBuildings.push_back((BuildingClassTypes)iI);
#else
						return (BuildingClassTypes)iI;
#endif
					}
				}
			}
		}
	}

#ifdef AUI_RELIGION_FIX_MULTIPLE_FAITH_BUILDINGS
	return aeFaithBuildings;
#else
	return NO_BUILDINGCLASS;
#endif
}

/// Can we buy a non-Faith generating building?
bool CvReligionAI::CanBuyNonFaithBuilding() const
{
	PlayerTypes ePlayer = m_pPlayer->GetID();

	int iLoop;
	CvCity* pLoopCity;
	for(pLoopCity = GET_PLAYER(ePlayer).firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GET_PLAYER(ePlayer).nextCity(&iLoop))
	{
#ifdef AUI_WARNING_FIXES
		for (uint iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
#else
		for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
#endif
		{
			BuildingTypes eBuilding = (BuildingTypes)m_pPlayer->getCivilizationInfo().getCivilizationBuildings(iI);
			if(eBuilding != NO_BUILDING)
			{
				CvBuildingEntry* pBuildingEntry = GC.GetGameBuildings()->GetEntry(eBuilding);

				// Check to make sure this isn't a Faith-generating building
				if(pBuildingEntry && pBuildingEntry->GetYieldChange(YIELD_FAITH) == 0)
				{
					if(pLoopCity->IsCanPurchase(true, true, (UnitTypes)-1, eBuilding, (ProjectTypes)-1, YIELD_FAITH))
					{
						return true;
					}
				}
			}
		}
	}
	return false;
}

/// Which Great Person should we buy with Faith?
UnitTypes CvReligionAI::GetDesiredFaithGreatPerson() const
{
	SpecialUnitTypes eSpecialUnitGreatPerson = (SpecialUnitTypes) GC.getInfoTypeForString("SPECIALUNIT_PEOPLE");
	UnitTypes eRtnValue = NO_UNIT;
#ifdef AUI_RELIGION_USE_DOUBLES
	double dBestScore = 0;
#else
	int iBestScore = 0;
#endif
	ReligionTypes eReligion = GetReligionToSpread();

	// Loop through all Units and see if they're possible
#ifdef AUI_WARNING_FIXES
	for (uint iUnitLoop = 0; iUnitLoop < GC.getNumUnitInfos(); iUnitLoop++)
#else
	for(int iUnitLoop = 0; iUnitLoop < GC.getNumUnitInfos(); iUnitLoop++)
#endif
	{
		const UnitTypes eUnit = static_cast<UnitTypes>(iUnitLoop);
		CvUnitEntry* pkUnitInfo = GC.getUnitInfo(eUnit);

		if(pkUnitInfo == NULL)
			continue;

		UnitClassTypes eUnitClass = (UnitClassTypes)pkUnitInfo->GetUnitClassType();

		// Can't be able to train it
		if(pkUnitInfo->GetProductionCost() != -1)
		{
			continue;
		}

		// Must be a Great Person (defined in SpecialUnitType in Unit XML)
		if(pkUnitInfo->GetSpecialUnitType() != eSpecialUnitGreatPerson)
		{
			continue;
		}

		// Must be a Great Person for this player's civ
		if(!m_pPlayer->canTrain(eUnit, false /*bContinue*/, false /*bTestVisible*/, true /*bIgnoreCost*/))
		{
			continue;
		}

		// Can we purchase this one in the capital?
		CvCity *pCapital = m_pPlayer->getCapitalCity();
		if (pCapital)
		{
			if (pCapital->IsCanPurchase(false/*bTestPurchaseCost*/, false/*bTestTrainable*/, eUnit, NO_BUILDING, NO_PROJECT, YIELD_FAITH))
			{
#ifdef AUI_RELIGION_USE_DOUBLES
				double dScore = 0;
#else
				int iScore = 0;
#endif
#ifndef AUI_GS_PRIORITY_RATIO
				AIGrandStrategyTypes eVictoryStrategy = m_pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy();
#endif

				// Score it
				if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_PROPHET"))
				{
					if (GetReligionToSpread() > RELIGION_PANTHEON)
					{
						if (ChooseProphetConversionCity(true/*bOnlyBetterThanEnhancingReligion*/))
						{
#ifdef AUI_RELIGION_USE_DOUBLES
							dScore = 1500.0;
						}
						else if (ChooseProphetConversionCity(false/*bOnlyBetterThanEnhancingReligion*/))
						{
							dScore = 750.0;
						}
						dScore /= (1.0 + m_pPlayer->GetReligions()->GetNumProphetsSpawned());
#ifdef AUI_RELIGION_FIX_DO_FAITH_PURCHASES_ENHANCE_INDUSTRIAL_RELIGION
						const CvReligion* pMyReligion = GC.getGame().GetGameReligions()->GetReligion(eReligion, m_pPlayer->GetID());
						if (pMyReligion && !pMyReligion->m_bEnhanced)
							dScore *= 2;
#endif
#else
							iScore = 1500;
						}
						else if (ChooseProphetConversionCity(false/*bOnlyBetterThanEnhancingReligion*/))
						{
							iScore = 750;
						}
						iScore /= (1+ m_pPlayer->GetReligions()->GetNumProphetsSpawned());
#ifdef AUI_RELIGION_FIX_DO_FAITH_PURCHASES_ENHANCE_INDUSTRIAL_RELIGION
						const CvReligion* pMyReligion = GC.getGame().GetGameReligions()->GetReligion(eReligion, m_pPlayer->GetID());
						if (pMyReligion && !pMyReligion->m_bEnhanced)
							iScore *= 2;
#endif
#endif
					}
				}
				else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_WRITER"))
				{
#ifdef AUI_RELIGION_USE_DOUBLES
#ifdef AUI_GS_PRIORITY_RATIO
					dScore = 400 + 600.0 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"));
#else
					if (eVictoryStrategy == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
					{
						dScore = 1000;
					}
					else
					{
						dScore = 400;
					}
#endif
					dScore /= (1.0 + m_pPlayer->getWritersFromFaith());
#else
#ifdef AUI_GS_PRIORITY_RATIO
					iScore = int(400 + 600.0 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE")) + 0.5);
#else
					if (eVictoryStrategy == (AIGrandStrategyTypes) GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
					{
						iScore = 1000;
					}
					else
					{
						iScore = 400;
					}
#endif
					iScore /= (1+ m_pPlayer->getWritersFromFaith());
#endif
				}
				else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_ARTIST"))
				{
#ifdef AUI_RELIGION_USE_DOUBLES
#ifdef AUI_GS_PRIORITY_RATIO
					dScore = 400 + 600.0 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"));
#else
					if (eVictoryStrategy == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
					{
						dScore = 1000;
					}
					else
					{
						dScore = 400;
					}
#endif
					dScore /= (1.0 + m_pPlayer->getArtistsFromFaith());
#else
#ifdef AUI_GS_PRIORITY_RATIO
					iScore = int(400 + 600.0 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE")) + 0.5);
#else
					if (eVictoryStrategy == (AIGrandStrategyTypes) GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
					{
						iScore = 1000;
					}
					else
					{
						iScore = 400;
					}
#endif
					iScore /= (1+ m_pPlayer->getArtistsFromFaith());
#endif
				}
				else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_MUSICIAN"))
				{
#ifdef AUI_RELIGION_USE_DOUBLES
#ifdef AUI_GS_PRIORITY_RATIO
					dScore = 400 + 600.0 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"));
#else
					if (eVictoryStrategy == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
					{
						dScore = 1000;
					}
					else
					{
						dScore = 400;
					}
#endif
					dScore /= (1.0 + m_pPlayer->getMusiciansFromFaith());
#else
#ifdef AUI_GS_PRIORITY_RATIO
					iScore = int(400 + 600.0 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE")) + 0.5);
#else
					if (eVictoryStrategy == (AIGrandStrategyTypes) GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
					{
						iScore = 1000;
					}
					else
					{
						iScore = 400;
					}
#endif
					iScore /= (1+ m_pPlayer->getMusiciansFromFaith());
#endif
				}
				else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_SCIENTIST"))
				{
#ifdef AUI_RELIGION_USE_DOUBLES
#ifdef AUI_GS_PRIORITY_RATIO
					dScore = 400 + 600.0 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP"));
#else
					if (eVictoryStrategy == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP"))
					{
						dScore = 1000;
					}
					else
					{
						dScore = 400;
					}
#endif
#ifdef AUI_GS_SCIENCE_FLAVOR_BOOST
					dScore = (1000.0 * m_pPlayer->GetGrandStrategyAI()->ScienceFlavorBoost());
#else
					dScore = 1000;
#endif
					dScore /= (1.0 + m_pPlayer->getScientistsFromFaith());
#else
#ifdef AUI_GS_PRIORITY_RATIO
					iScore = int(400 + 600.0 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP")) + 0.5);
#else
					if (eVictoryStrategy == (AIGrandStrategyTypes) GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP"))
					{
						iScore = 1000;
					}
					else
					{
						iScore = 400;
					}
#endif
#ifdef AUI_GS_SCIENCE_FLAVOR_BOOST
					iScore = (1000 * m_pPlayer->GetGrandStrategyAI()->ScienceFlavorBoost());
#else
					iScore = 1000;
#endif
					iScore /= (1+ m_pPlayer->getScientistsFromFaith());
#endif
				}
				else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_MERCHANT"))
				{
#ifdef AUI_RELIGION_USE_DOUBLES
#ifdef AUI_GS_PRIORITY_RATIO
					dScore = 400 + 600.0 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS"));
#else
					if (eVictoryStrategy == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS"))
					{
						dScore = 1000;
					}
					else
					{
						dScore = 400;
					}
#endif
#ifdef AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_VENICE_MERCHANT_BOOST
					if (!GET_PLAYER(m_pPlayer->GetID()).GreatMerchantWantsCash())
					{
#ifdef AUI_GS_SCIENCE_FLAVOR_BOOST
						dScore *= m_pPlayer->GetGrandStrategyAI()->ScienceFlavorBoost() + 2;
#else
						dScore *= AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_VENICE_MERCHANT_BOOST;
#endif
					}
#endif
					dScore /= (1.0 + m_pPlayer->getMerchantsFromFaith());
#else
#ifdef AUI_GS_PRIORITY_RATIO
					iScore = int(400 + 600.0 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS")) + 0.5);
#else
					if (eVictoryStrategy == (AIGrandStrategyTypes) GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS"))
					{
						iScore = 1000;
					}
					else
					{
						iScore = 400;
					}
#endif
#ifdef AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_VENICE_MERCHANT_BOOST
					if (!GET_PLAYER(m_pPlayer->GetID()).GreatMerchantWantsCash())
					{
						iScore *= AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_VENICE_MERCHANT_BOOST;
					}
#endif
					iScore /= (1+ m_pPlayer->getMerchantsFromFaith());
#endif
				}
				else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_ENGINEER"))
				{
					EconomicAIStrategyTypes eStrategy = (EconomicAIStrategyTypes) GC.getInfoTypeForString("ECONOMICAISTRATEGY_GS_SPACESHIP_HOMESTRETCH");
					if (eStrategy != NO_ECONOMICAISTRATEGY && m_pPlayer->GetEconomicAI()->IsUsingStrategy(eStrategy))
					{
#ifdef AUI_RELIGION_USE_DOUBLES
						dScore = 2000.0;
					}
					else
					{
#ifdef AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_ENGINEER_USES_WONDER_COMPETITIVENESS
						dScore = MAX(2000.0, AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_ENGINEER_USES_WONDER_COMPETITIVENESS * m_pPlayer->GetDiplomacyAI()->GetWonderCompetitiveness());
#else
						dScore = 500.0;
#endif
					}
					dScore /= (1.0 + m_pPlayer->getEngineersFromFaith());
#else
						iScore = 2000;
					}
					else
					{
#ifdef AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_ENGINEER_USES_WONDER_COMPETITIVENESS
						iScore = MAX(2000, int(AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_ENGINEER_USES_WONDER_COMPETITIVENESS * m_pPlayer->GetDiplomacyAI()->GetWonderCompetitiveness() + 0.5));
#else
						iScore = 500;
#endif
					}
					iScore /= (1+ m_pPlayer->getEngineersFromFaith());
#endif
				}
				else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_GREAT_GENERAL"))
				{
#ifdef AUI_RELIGION_USE_DOUBLES
#ifdef AUI_GS_PRIORITY_RATIO
					dScore = 400.0;
					if (!(GC.getMap().GetAIMapHint() & 1))
					{
						dScore += 600.0 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"));
					}
#else
					if (eVictoryStrategy == (AIGrandStrategyTypes) GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST")&& !(GC.getMap().GetAIMapHint() & 1))
					{
						dScore = 1000;
					}
					else
					{
						dScore = 400;
					}
#endif
					dScore /= (1.0 + m_pPlayer->getGeneralsFromFaith() + m_pPlayer->GetNumUnitsWithUnitAI(UNITAI_GENERAL));
#else
#ifdef AUI_GS_PRIORITY_RATIO
					iScore = 400;
					if (!(GC.getMap().GetAIMapHint() & 1))
					{
						iScore += int(600.0 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST")) + 0.5);
					}
#else
					if (eVictoryStrategy == (AIGrandStrategyTypes) GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST") && !(GC.getMap().GetAIMapHint() & 1))
					{
						iScore = 1000;
					}
					else
					{
						iScore = 400;
					}
#endif
					iScore /= (1+ m_pPlayer->getGeneralsFromFaith() + m_pPlayer->GetNumUnitsWithUnitAI(UNITAI_GENERAL));
#endif
				}
				else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_GREAT_ADMIRAL"))
				{
#ifdef AUI_RELIGION_USE_DOUBLES
#ifdef AUI_GS_PRIORITY_RATIO
					dScore = 400.0;
					if (!(GC.getMap().GetAIMapHint() & 1))
					{
						dScore += 600.0 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"));
					}
#else
					if (eVictoryStrategy == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST") && !(GC.getMap().GetAIMapHint() & 1))
					{
						dScore = 1000;
					}
					else
					{
						dScore = 400;
					}
#endif
					dScore /= (1.0 + m_pPlayer->getAdmiralsFromFaith() + m_pPlayer->GetNumUnitsWithUnitAI(UNITAI_ADMIRAL));
#else
#ifdef AUI_GS_PRIORITY_RATIO
					iScore = 400;
					if (GC.getMap().GetAIMapHint() & 1)
					{
						iScore += int(600.0 * m_pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST")) + 0.5);
					}
#else
					if (eVictoryStrategy == (AIGrandStrategyTypes) GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST") && GC.getMap().GetAIMapHint() & 1)
					{
						iScore = 1000;
					}
					else
					{
						iScore = 400;
					}
#endif
					iScore /= (1+ m_pPlayer->getAdmiralsFromFaith() + m_pPlayer->GetNumUnitsWithUnitAI(UNITAI_ADMIRAL));
#endif
				}
				else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_MISSIONARY"))
				{
					if (HaveNearbyConversionTarget(eReligion, false /*bCanIncludeReligionStarter*/))
					{
#ifdef AUI_RELIGION_USE_DOUBLES
#ifdef AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_SCALE_MISSIONARY_INQUISITOR_WITH_TURNS_ELAPSED
						dScore = AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_SCALE_MISSIONARY_INQUISITOR_WITH_TURNS_ELAPSED * GC.getGame().getGameTurn() / 
							double((m_pPlayer->GetNumUnitsWithUnitAI(UNITAI_MISSIONARY) + 1.0) * GC.getGame().getEstimateEndTurn());
#else
						dScore = 1000.0 / (m_pPlayer->GetNumUnitsWithUnitAI(UNITAI_MISSIONARY) + 1.0);
#endif
#else
#ifdef AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_SCALE_MISSIONARY_INQUISITOR_WITH_TURNS_ELAPSED
						iScore = AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_SCALE_MISSIONARY_INQUISITOR_WITH_TURNS_ELAPSED * GC.getGame().getGameTurn() / 
							((m_pPlayer->GetNumUnitsWithUnitAI(UNITAI_MISSIONARY) + 1) * GC.getGame().getEstimateEndTurn());
#else
						iScore = 1000 / (m_pPlayer->GetNumUnitsWithUnitAI(UNITAI_MISSIONARY) + 1);		
#endif
#endif
					}
				}
				else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_INQUISITOR"))
				{
#ifdef AUI_RELIGION_FIX_GET_DESIRED_FAITH_GREAT_PERSON_INQUISITOR_CHECK
					if (!HaveEnoughInquisitors(eReligion))
#else
					if (HaveEnoughInquisitors(eReligion))
#endif
					{
#ifdef AUI_RELIGION_USE_DOUBLES
#ifdef AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_SCALE_MISSIONARY_INQUISITOR_WITH_TURNS_ELAPSED
						dScore = AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_SCALE_MISSIONARY_INQUISITOR_WITH_TURNS_ELAPSED * GC.getGame().getGameTurn() /
							double((m_pPlayer->GetNumUnitsWithUnitAI(UNITAI_INQUISITOR) + 1.0) * GC.getGame().getEstimateEndTurn());
#else
						dScore = 1000.0 / (m_pPlayer->GetNumUnitsWithUnitAI(UNITAI_INQUISITOR) + 1.0);
#endif
#else
#ifdef AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_SCALE_MISSIONARY_INQUISITOR_WITH_TURNS_ELAPSED
						iScore = AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_SCALE_MISSIONARY_INQUISITOR_WITH_TURNS_ELAPSED * GC.getGame().getGameTurn() /
							((m_pPlayer->GetNumUnitsWithUnitAI(UNITAI_INQUISITOR) + 1) * GC.getGame().getEstimateEndTurn());
#else
						iScore = 1000 / (m_pPlayer->GetNumUnitsWithUnitAI(UNITAI_INQUISITOR) + 1);
#endif
#endif
					}
				}

#ifdef AUI_RELIGION_USE_DOUBLES
				if (dScore > dBestScore)
				{
					dBestScore = dScore;
#else
				if (iScore > iBestScore)
				{
					iBestScore = iScore;
#endif
					eRtnValue = eUnit;
				}
			}
		}
	}

	return eRtnValue;
}

/// Log choices considered for beliefs
void CvReligionAI::LogBeliefChoices(CvWeightedVector<BeliefTypes, SAFE_ESTIMATE_NUM_BELIEFS, true>& beliefChoices, int iChoice)
{
	if(GC.getLogging() && GC.getAILogging())
	{
		CvString strOutBuf;
		CvString strBaseString;
		CvString strTemp;
		CvString playerName;
		CvString strDesc;
		BeliefTypes eBelief;

		// Find the name of this civ
		playerName = m_pPlayer->getCivilizationShortDescription();

		// Open the log file
		FILogFile* pLog;
		pLog = LOGFILEMGR.GetLog(GC.getGame().GetGameReligions()->GetLogFileName(), FILogFile::kDontTimeStamp);

		// Get the leading info for this line
		strBaseString.Format("%03d, %d, ", GC.getGame().getElapsedGameTurns(), GC.getGame().getGameTurnYear());
		strBaseString += playerName + ", ";

		// Dump out the weight of each belief item
		for(int iI = 0; iI < beliefChoices.size(); iI++)
		{
			eBelief = beliefChoices.GetElement(iI);
			strDesc = GetLocalizedText(GC.GetGameBeliefs()->GetEntry(eBelief)->getShortDescription());
			strTemp.Format("Belief, %s, %d", strDesc.GetCString(), beliefChoices.GetWeight(iI));
			strOutBuf = strBaseString + strTemp;
			pLog->Msg(strOutBuf);
		}

		// Finally the chosen one
		eBelief = (BeliefTypes)iChoice;
		strDesc = GetLocalizedText(GC.GetGameBeliefs()->GetEntry(eBelief)->getShortDescription());
		strTemp.Format("CHOSEN, %s", strDesc.GetCString());
		strOutBuf = strBaseString + strTemp;
		pLog->Msg(strOutBuf);
	}
}

// AI HELPER ROUTINES

CvCity *CvReligionAIHelpers::GetBestCityFaithUnitPurchase(CvPlayer &kPlayer, UnitTypes eUnit, ReligionTypes eReligion)
{
	bool bReligious = false;
	CvCity *pHolyCity = NULL;
	SpecialUnitTypes eSpecialUnitGreatPerson = (SpecialUnitTypes) GC.getInfoTypeForString("SPECIALUNIT_PEOPLE");
	int iLoop;
	CvCity* pLoopCity;

	CvGameReligions* pReligions = GC.getGame().GetGameReligions();
	const CvReligion* pMyReligion = pReligions->GetReligion(eReligion, kPlayer.GetID());

	CvUnitEntry *pkUnitEntry = GC.getUnitInfo(eUnit);
	if (pkUnitEntry)
	{
		// Religious unit?
		if (pkUnitEntry->IsSpreadReligion() || pkUnitEntry->IsRemoveHeresy())
		{
			bReligious = true;
		}
	
		// Great person?
		else if (pkUnitEntry->GetSpecialUnitType() == eSpecialUnitGreatPerson)
		{
#ifdef AUI_RELIGION_FIX_BEST_CITY_HELPER_GREAT_PERSON
			int iCost = MAX_INT;
			CvCity* pBestCity;
			int iLoopCost;
			for (pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
			{
				if (pLoopCity->IsCanPurchase(true/*bTestPurchaseCost*/, true/*bTestTrainable*/, eUnit, NO_BUILDING, NO_PROJECT, YIELD_FAITH))
				{
					iLoopCost = pLoopCity->GetFaithPurchaseCost(eUnit, true);
					if (iLoopCost > 0 && iLoopCost < iCost)
					{
						pBestCity = pLoopCity;
						iCost = iLoopCost;
					}
				}
			}
			if (pBestCity)
			{
				return pBestCity;
			}
#else
			for (pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
			{
				if (pLoopCity->IsCanPurchase(true/*bTestPurchaseCost*/, true/*bTestTrainable*/, eUnit, NO_BUILDING, NO_PROJECT, YIELD_FAITH))
				{
					return pLoopCity;
				}
			}
#endif
			return NULL;
		}
	}

	// If religious, try to buy in the city with the Great Mosque first if a Missionary
	if (bReligious && pMyReligion)
	{
		for(pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
		{
			if (pLoopCity->getOwner() != kPlayer.GetID())
			{
				continue;
			}

			if (pLoopCity->GetCityBuildings()->GetMissionaryExtraSpreads() < 1 || pkUnitEntry->GetReligionSpreads() < 1)
			{
				continue;
			}

			if (pLoopCity->GetCityReligions()->GetReligiousMajority() == eReligion)
			{
				if(pLoopCity->IsCanPurchase(true, true, eUnit, NO_BUILDING, NO_PROJECT, YIELD_FAITH))
				{
					return pLoopCity;
				}
			}
		}
	}

	// If religious, next try to buy in the holy city, assuming it hasn't been converted
	if (bReligious && pMyReligion)
	{
		CvPlot* pHolyCityPlot = GC.getMap().plot(pMyReligion->m_iHolyCityX, pMyReligion->m_iHolyCityY);
		if (pHolyCityPlot)
		{
			pHolyCity = pHolyCityPlot->getPlotCity();

			if (pHolyCity && (pHolyCity->getOwner() == kPlayer.GetID()))
			{
				if (pHolyCity->GetCityReligions()->GetReligiousMajority() == eReligion && pHolyCity->IsCanPurchase(true, true, eUnit, NO_BUILDING, NO_PROJECT, YIELD_FAITH))
				{
					return pHolyCity;
				}
			}
		}
	}

	// Now see if there is another city with our majority religion
	for(pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
	{
		if (bReligious && pMyReligion && pLoopCity == pHolyCity)
		{
			continue;
		}

		if (pLoopCity->getOwner() != kPlayer.GetID())
		{
			continue;
		}

		if (pLoopCity->GetCityReligions()->GetReligiousMajority() == eReligion)
		{
			if(pLoopCity->IsCanPurchase(true, true, eUnit, NO_BUILDING, NO_PROJECT, YIELD_FAITH))
			{
				return pLoopCity;
			}
		}
	}

	return NULL;
}

CvCity *CvReligionAIHelpers::GetBestCityFaithBuildingPurchase(CvPlayer &kPlayer, BuildingTypes eBuilding, ReligionTypes eReligion)
{
	CvCity *pHolyCity = NULL;
	CvGameReligions* pReligions = GC.getGame().GetGameReligions();
	const CvReligion* pMyReligion = pReligions->GetReligion(eReligion, kPlayer.GetID());

	// Try to buy in the holy city first
	if (pMyReligion)
	{
		CvPlot* pHolyCityPlot = GC.getMap().plot(pMyReligion->m_iHolyCityX, pMyReligion->m_iHolyCityY);
		if (pHolyCityPlot)
		{
			pHolyCity = pHolyCityPlot->getPlotCity();

			if (pHolyCity && (pHolyCity->getOwner() == kPlayer.GetID()) && pHolyCity->IsCanPurchase(true, true, NO_UNIT, eBuilding, NO_PROJECT, YIELD_FAITH))
			{
				return pHolyCity;
			}
		}
	}

	// Now see if there is another city with our majority religion
	int iLoop;
	CvCity* pLoopCity;
	for(pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
	{
		if(pLoopCity == pHolyCity)
		{
			continue;
		}

		if(pLoopCity->getOwner() != kPlayer.GetID())
		{
			continue;
		}

		if(pLoopCity->IsCanPurchase(true, true, NO_UNIT, eBuilding, NO_PROJECT, YIELD_FAITH))
		{
			return pLoopCity;
		}
	}

	return NULL;
}

bool CvReligionAIHelpers::DoesUnitPassFaithPurchaseCheck(CvPlayer &kPlayer, UnitTypes eUnit)
{
	bool bRtnValue = true;

	CvUnitEntry *pkUnitEntry = GC.getUnitInfo(eUnit);
	if (pkUnitEntry)
	{
		if (pkUnitEntry->IsSpreadReligion() || pkUnitEntry->IsRemoveHeresy())
		{
			bRtnValue = false;

			// Religious unit, have to find a city with the majority religion we started
			// Now see if there is another city with our majority religion
			int iLoop;
			CvCity* pLoopCity;

			CvGameReligions* pReligions = GC.getGame().GetGameReligions();
			ReligionTypes eReligion = pReligions->GetReligionCreatedByPlayer(kPlayer.GetID());
			if (eReligion > RELIGION_PANTHEON)
			{
				for(pLoopCity = kPlayer.firstCity(&iLoop); pLoopCity != NULL; pLoopCity = kPlayer.nextCity(&iLoop))
				{
					if (pLoopCity->getOwner() != kPlayer.GetID())
					{
						continue;
					}

					if (pLoopCity->GetCityReligions()->GetReligiousMajority() == eReligion)
					{
						if(pLoopCity->IsCanPurchase(false /*bTestPurchaseCost*/, true, eUnit, NO_BUILDING, NO_PROJECT, YIELD_FAITH))
						{
							return true;
						}
					}
				}
			}
		}
	}

	return bRtnValue;
}
