/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#include "CvGameCoreDLLPCH.h"
#include "ICvDLLUserInterface.h"
#include "CvGameCoreUtils.h"
#include "CvCitySpecializationAI.h"
#include "CvEconomicAI.h"
#include "CvGrandStrategyAI.h"
#include "CvDllInterfaces.h"
#include "CvInfosSerializationHelper.h"
#include "cvStopWatch.h"

// must be included after all other headers
#include "LintFree.h"


//=====================================
// CvCityCitizens
//=====================================

/// Constructor
CvCityCitizens::CvCityCitizens()
{
	m_aiSpecialistCounts = NULL;
	m_aiSpecialistGreatPersonProgressTimes100 = NULL;
	m_aiNumSpecialistsInBuilding = NULL;
	m_aiNumForcedSpecialistsInBuilding = NULL;
	m_piBuildingGreatPeopleRateChanges = NULL;
}

/// Destructor
CvCityCitizens::~CvCityCitizens()
{
	Uninit();
}

/// Initialize
void CvCityCitizens::Init(CvCity* pCity)
{
	m_pCity = pCity;

	// Clear variables
	Reset();

	m_bInited = true;
}

/// Deallocate memory created in initialize
void CvCityCitizens::Uninit()
{
	if(m_bInited)
	{
		SAFE_DELETE_ARRAY(m_aiSpecialistCounts);
		SAFE_DELETE_ARRAY(m_aiSpecialistGreatPersonProgressTimes100);
		SAFE_DELETE_ARRAY(m_aiNumSpecialistsInBuilding);
		SAFE_DELETE_ARRAY(m_aiNumForcedSpecialistsInBuilding);
		SAFE_DELETE_ARRAY(m_piBuildingGreatPeopleRateChanges);
	}
	m_bInited = false;
}

/// Reset member variables
void CvCityCitizens::Reset()
{
	m_bAutomated = false;
	m_bNoAutoAssignSpecialists = false;
	m_iNumUnassignedCitizens = 0;
	m_iNumCitizensWorkingPlots = 0;
	m_iNumForcedWorkingPlots = 0;

	m_eCityAIFocusTypes = NO_CITY_AI_FOCUS_TYPE;

	int iI;

	CvAssertMsg((0 < NUM_CITY_PLOTS),  "NUM_CITY_PLOTS is not greater than zero but an array is being allocated in CvCityCitizens::reset");
	for(iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		m_pabWorkingPlot[iI] = false;
		m_pabForcedWorkingPlot[iI] = false;
	}

	m_iNumDefaultSpecialists = 0;
	m_iNumForcedDefaultSpecialists = 0;

	CvAssertMsg(m_aiSpecialistCounts==NULL, "about to leak memory, CvCityCitizens::m_aiSpecialistCounts");
	m_aiSpecialistCounts = FNEW(int[GC.getNumSpecialistInfos()], c_eCiv5GameplayDLL, 0);
	for(iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
	{
		m_aiSpecialistCounts[iI] = 0;
	}

	CvAssertMsg(m_aiSpecialistGreatPersonProgressTimes100==NULL, "about to leak memory, CvCityCitizens::m_aiSpecialistGreatPersonProgressTimes100");
	m_aiSpecialistGreatPersonProgressTimes100 = FNEW(int[GC.getNumSpecialistInfos()], c_eCiv5GameplayDLL, 0);
	for(iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
	{
		m_aiSpecialistGreatPersonProgressTimes100[iI] = 0;
	}

	CvAssertMsg(m_aiNumSpecialistsInBuilding==NULL, "about to leak memory, CvCityCitizens::m_aiNumSpecialistsInBuilding");
	m_aiNumSpecialistsInBuilding = FNEW(int[GC.getNumBuildingInfos()], c_eCiv5GameplayDLL, 0);
	for(iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		m_aiNumSpecialistsInBuilding[iI] = 0;
	}

	CvAssertMsg(m_aiNumForcedSpecialistsInBuilding==NULL, "about to leak memory, CvCityCitizens::m_aiNumForcedSpecialistsInBuilding");
	m_aiNumForcedSpecialistsInBuilding = FNEW(int[GC.getNumBuildingInfos()], c_eCiv5GameplayDLL, 0);
	for(iI = 0; iI < GC.getNumBuildingInfos(); iI++)
	{
		m_aiNumForcedSpecialistsInBuilding[iI] = 0;
	}

	CvAssertMsg(m_piBuildingGreatPeopleRateChanges==NULL, "about to leak memory, CvCityCitizens::m_piBuildingGreatPeopleRateChanges");
	m_piBuildingGreatPeopleRateChanges = FNEW(int[GC.getNumSpecialistInfos()], c_eCiv5GameplayDLL, 0);
	for(iI = 0; iI < GC.getNumSpecialistInfos(); iI++)
	{
		m_piBuildingGreatPeopleRateChanges[iI] = 0;
	}

	m_bForceAvoidGrowth = false;
}

/// Serialization read
void CvCityCitizens::Read(FDataStream& kStream)
{
	// Version number to maintain backwards compatibility
	uint uiVersion;
	kStream >> uiVersion;

	kStream >> m_bAutomated;
	kStream >> m_bNoAutoAssignSpecialists;
	kStream >> m_iNumUnassignedCitizens;
	kStream >> m_iNumCitizensWorkingPlots;
	kStream >> m_iNumForcedWorkingPlots;

	kStream >> m_eCityAIFocusTypes;

	kStream >> 	m_bForceAvoidGrowth;

	kStream >> m_pabWorkingPlot;
	kStream >> m_pabForcedWorkingPlot;

	kStream >> m_iNumDefaultSpecialists;
	kStream >> m_iNumForcedDefaultSpecialists;

	CvInfosSerializationHelper::ReadHashedDataArray(kStream, m_aiSpecialistCounts, GC.getNumSpecialistInfos());
	CvInfosSerializationHelper::ReadHashedDataArray(kStream, m_aiSpecialistGreatPersonProgressTimes100, GC.getNumSpecialistInfos());

	BuildingArrayHelpers::Read(kStream, m_aiNumSpecialistsInBuilding);
	BuildingArrayHelpers::Read(kStream, m_aiNumForcedSpecialistsInBuilding);

	CvInfosSerializationHelper::ReadHashedDataArray(kStream, m_piBuildingGreatPeopleRateChanges, GC.getNumSpecialistInfos());
}

/// Serialization write
void CvCityCitizens::Write(FDataStream& kStream)
{
	// Current version number
	uint uiVersion = 1;
	kStream << uiVersion;

	kStream << m_bAutomated;
	kStream << m_bNoAutoAssignSpecialists;
	kStream << m_iNumUnassignedCitizens;
	kStream << m_iNumCitizensWorkingPlots;
	kStream << m_iNumForcedWorkingPlots;

	kStream << m_eCityAIFocusTypes;

	kStream << 	m_bForceAvoidGrowth;

	kStream << m_pabWorkingPlot;
	kStream << m_pabForcedWorkingPlot;

	kStream << m_iNumDefaultSpecialists;
	kStream << m_iNumForcedDefaultSpecialists;

	CvInfosSerializationHelper::WriteHashedDataArray<SpecialistTypes, int>(kStream, m_aiSpecialistCounts, GC.getNumSpecialistInfos());
	CvInfosSerializationHelper::WriteHashedDataArray<SpecialistTypes, int>(kStream, m_aiSpecialistGreatPersonProgressTimes100, GC.getNumSpecialistInfos());

	BuildingArrayHelpers::Write(kStream, m_aiNumSpecialistsInBuilding, GC.getNumBuildingInfos());
	BuildingArrayHelpers::Write(kStream, m_aiNumForcedSpecialistsInBuilding, GC.getNumBuildingInfos());

	CvInfosSerializationHelper::WriteHashedDataArray<SpecialistTypes, int>(kStream, m_piBuildingGreatPeopleRateChanges, GC.getNumSpecialistInfos());
}

/// Returns the City object this set of Citizens is associated with
#ifdef AUI_CONSTIFY
CvCity* CvCityCitizens::GetCity() const
#else
CvCity* CvCityCitizens::GetCity()
#endif
{
	return m_pCity;
}

/// Returns the Player object this City belongs to
#ifdef AUI_CONSTIFY
CvPlayer* CvCityCitizens::GetPlayer() const
#else
CvPlayer* CvCityCitizens::GetPlayer()
#endif
{
	return &GET_PLAYER(GetOwner());
}

/// Helper function to return Player owner of our City
PlayerTypes CvCityCitizens::GetOwner() const
{
	return m_pCity->getOwner();
}

/// Helper function to return Team owner of our City
TeamTypes CvCityCitizens::GetTeam() const
{
	return m_pCity->getTeam();
}

/// What happens when a City is first founded?
void CvCityCitizens::DoFoundCity()
{
	// always work the home plot (center)
	CvPlot* pHomePlot = GetCityPlotFromIndex(CITY_HOME_PLOT);
	if(pHomePlot != NULL)
	{
		bool bWorkPlot = IsCanWork(pHomePlot);
		SetWorkingPlot(pHomePlot, bWorkPlot, /*bUseUnassignedPool*/ false);
	}
}

/// Processed every turn
#ifdef AUI_PLAYER_RESOLVE_WORKED_PLOT_CONFLICTS
void CvCityCitizens::DoTurn(bool bDoSpecialist)
#else
void CvCityCitizens::DoTurn()
#endif
{
	AI_PERF_FORMAT("City-AI-perf.csv", ("CvCityCitizens::DoTurn, Turn %03d, %s, %s", GC.getGame().getElapsedGameTurns(), m_pCity->GetPlayer()->getCivilizationShortDescription(), m_pCity->getName().c_str()) );
	DoVerifyWorkingPlots();

	CvPlayerAI& thisPlayer = GET_PLAYER(GetOwner());

	if(m_pCity->IsPuppet())
	{
		SetFocusType(CITY_AI_FOCUS_TYPE_GOLD);
		SetNoAutoAssignSpecialists(false);
		SetForcedAvoidGrowth(false);
		int iExcessFoodTimes100 = m_pCity->getYieldRateTimes100(YIELD_FOOD, false) - (m_pCity->foodConsumption() * 100);
		if(iExcessFoodTimes100 < 0)
		{
			SetFocusType(NO_CITY_AI_FOCUS_TYPE);
			//SetNoAutoAssignSpecialists(true);
			SetForcedAvoidGrowth(false);
		}
	}
	else if(!thisPlayer.isHuman())
	{
		CitySpecializationTypes eWonderSpecializationType = thisPlayer.GetCitySpecializationAI()->GetWonderSpecialization();

		if(GC.getGame().getGameTurn() % 8 == 0)
		{
			SetFocusType(CITY_AI_FOCUS_TYPE_GOLD_GROWTH);
			SetNoAutoAssignSpecialists(true);
			SetForcedAvoidGrowth(false);
			int iExcessFoodTimes100 = m_pCity->getYieldRateTimes100(YIELD_FOOD, false) - (m_pCity->foodConsumption() * 100);
			if(iExcessFoodTimes100 < 200)
			{
				SetFocusType(NO_CITY_AI_FOCUS_TYPE);
				//SetNoAutoAssignSpecialists(true);
			}
		}
		if(m_pCity->isCapital() && !thisPlayer.isMinorCiv() && m_pCity->GetCityStrategyAI()->GetSpecialization() != eWonderSpecializationType)
		{
			SetFocusType(NO_CITY_AI_FOCUS_TYPE);
			SetNoAutoAssignSpecialists(false);
			SetForcedAvoidGrowth(false);
			int iExcessFoodTimes100 = m_pCity->getYieldRateTimes100(YIELD_FOOD, false) - (m_pCity->foodConsumption() * 100);
			if(iExcessFoodTimes100 < 400)
			{
				SetFocusType(CITY_AI_FOCUS_TYPE_FOOD);
				//SetNoAutoAssignSpecialists(true);
			}
		}
		else if(m_pCity->GetCityStrategyAI()->GetSpecialization() == eWonderSpecializationType)
		{
			SetFocusType(CITY_AI_FOCUS_TYPE_PRODUCTION);
			SetNoAutoAssignSpecialists(false);
			//SetForcedAvoidGrowth(true);
			int iExcessFoodTimes100;// = m_pCity->getYieldRateTimes100(YIELD_FOOD) - (m_pCity->foodConsumption() * 100);
			//if (iExcessFoodTimes100 < 200)
			//{
			SetForcedAvoidGrowth(false);
			//}
			iExcessFoodTimes100 = m_pCity->getYieldRateTimes100(YIELD_FOOD, false) - (m_pCity->foodConsumption() * 100);
			if(iExcessFoodTimes100 < 200)
			{
				SetFocusType(CITY_AI_FOCUS_TYPE_PROD_GROWTH);
				//SetNoAutoAssignSpecialists(true);
				SetForcedAvoidGrowth(false);
			}
			iExcessFoodTimes100 = m_pCity->getYieldRateTimes100(YIELD_FOOD, false) - (m_pCity->foodConsumption() * 100);
			if(iExcessFoodTimes100 < 200)
			{
				SetFocusType(NO_CITY_AI_FOCUS_TYPE);
				//SetNoAutoAssignSpecialists(true);
				SetForcedAvoidGrowth(false);
			}
		}
		else if(m_pCity->getPopulation() < 5)  // we want a balanced growth
		{
			SetFocusType(NO_CITY_AI_FOCUS_TYPE);
			//SetNoAutoAssignSpecialists(true);
			SetForcedAvoidGrowth(false);
		}
		else
		{
			// Are we running at a deficit?
			EconomicAIStrategyTypes eStrategyLosingMoney = (EconomicAIStrategyTypes) GC.getInfoTypeForString("ECONOMICAISTRATEGY_LOSING_MONEY", true);
			bool bInDeficit = false;
			if (eStrategyLosingMoney != NO_ECONOMICAISTRATEGY)
			{
				bInDeficit = thisPlayer.GetEconomicAI()->IsUsingStrategy(eStrategyLosingMoney);
			}

			if(bInDeficit)
			{
				SetFocusType(CITY_AI_FOCUS_TYPE_GOLD_GROWTH);
				SetNoAutoAssignSpecialists(false);
				SetForcedAvoidGrowth(false);
				int iExcessFoodTimes100 = m_pCity->getYieldRateTimes100(YIELD_FOOD, false) - (m_pCity->foodConsumption() * 100);
				if(iExcessFoodTimes100 < 200)
				{
					SetFocusType(NO_CITY_AI_FOCUS_TYPE);
					//SetNoAutoAssignSpecialists(true);
				}
			}
			else if(GC.getGame().getGameTurn() % 3 == 0 && thisPlayer.GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes) GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
			{
				SetFocusType(CITY_AI_FOCUS_TYPE_CULTURE);
				SetNoAutoAssignSpecialists(true);
				SetForcedAvoidGrowth(false);
				int iExcessFoodTimes100 = m_pCity->getYieldRateTimes100(YIELD_FOOD, false) - (m_pCity->foodConsumption() * 100);
				if(iExcessFoodTimes100 < 200)
				{
					SetFocusType(NO_CITY_AI_FOCUS_TYPE);
					//SetNoAutoAssignSpecialists(true);
				}
			}
			else // we aren't a small city, building a wonder, or going broke
			{
				SetNoAutoAssignSpecialists(false);
				SetForcedAvoidGrowth(false);
				CitySpecializationTypes eSpecialization = m_pCity->GetCityStrategyAI()->GetSpecialization();
				if(eSpecialization != -1)
				{
					CvCitySpecializationXMLEntry* pCitySpecializationEntry =  GC.getCitySpecializationInfo(eSpecialization);
					if(pCitySpecializationEntry)
					{
						YieldTypes eYield = pCitySpecializationEntry->GetYieldType();
						if(eYield == YIELD_FOOD)
						{
							SetFocusType(CITY_AI_FOCUS_TYPE_FOOD);
						}
						else if(eYield == YIELD_PRODUCTION)
						{
							SetFocusType(CITY_AI_FOCUS_TYPE_PROD_GROWTH);
						}
						else if(eYield == YIELD_GOLD)
						{
							SetFocusType(CITY_AI_FOCUS_TYPE_GOLD);
						}
						else if(eYield == YIELD_SCIENCE)
						{
							SetFocusType(CITY_AI_FOCUS_TYPE_SCIENCE);
						}
						else
						{
							SetFocusType(NO_CITY_AI_FOCUS_TYPE);
						}
					}
					else
					{
						SetFocusType(NO_CITY_AI_FOCUS_TYPE);
					}
				}
				else
				{
					SetFocusType(NO_CITY_AI_FOCUS_TYPE);
				}
			}
		}
#ifdef AUI_CITIZENS_DO_TURN_NO_FOOD_FOCUS_IF_UNHAPPY
		if (thisPlayer.IsEmpireUnhappy())
		{
			CityAIFocusTypes eCurrentFocus = GetFocusType();
			if (eCurrentFocus == CITY_AI_FOCUS_TYPE_FOOD)
				SetFocusType(NO_CITY_AI_FOCUS_TYPE);
			else if (eCurrentFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
				SetFocusType(CITY_AI_FOCUS_TYPE_PRODUCTION);
			else if (eCurrentFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH)
				SetFocusType(CITY_AI_FOCUS_TYPE_GOLD);
		}
#endif
	}

	CvAssertMsg((GetNumCitizensWorkingPlots() + GetTotalSpecialistCount() + GetNumUnassignedCitizens()) <= GetCity()->getPopulation(), "Gameplay: More workers than population in the city.");
	DoReallocateCitizens();
	CvAssertMsg((GetNumCitizensWorkingPlots() + GetTotalSpecialistCount() + GetNumUnassignedCitizens()) <= GetCity()->getPopulation(), "Gameplay: More workers than population in the city.");
#ifdef AUI_PLAYER_RESOLVE_WORKED_PLOT_CONFLICTS
	if (bDoSpecialist)
#endif
	DoSpecialists();

	CvAssertMsg((GetNumCitizensWorkingPlots() + GetTotalSpecialistCount() + GetNumUnassignedCitizens()) <= GetCity()->getPopulation(), "Gameplay: More workers than population in the city.");
}

/// What is the overall value of the current Plot?
#ifdef AUI_CITIZENS_GET_VALUE_FROM_STATS
int CvCityCitizens::GetPlotValue(CvPlot* pPlot, bool bUseAllowGrowthFlag, double* adBonusYields, double* adBonusYieldModifiers, int iExtraHappiness, int iExtraGrowthMod, bool bAfterGrowth) const
#elif defined(AUI_CITIZENS_IGNORE_FOOD_FOR_CITIZEN_ASSIGN_AFTER_GROW)
int CvCityCitizens::GetPlotValue(CvPlot* pPlot, bool bUseAllowGrowthFlag, bool bAfterGrowth) const
#else
int CvCityCitizens::GetPlotValue(CvPlot* pPlot, bool bUseAllowGrowthFlag)
#endif
{
#ifdef AUI_CITIZENS_GET_VALUE_FROM_STATS
	double dYieldArray[NUM_YIELD_TYPES] = {};

	dYieldArray[YIELD_FOOD] += pPlot->calculateYield(YIELD_FOOD, false, true, m_pCity);
	dYieldArray[YIELD_PRODUCTION] += pPlot->calculateYield(YIELD_PRODUCTION, false, true, m_pCity);
	dYieldArray[YIELD_GOLD] += pPlot->calculateYield(YIELD_GOLD, false, true, m_pCity);
	dYieldArray[YIELD_SCIENCE] += pPlot->calculateYield(YIELD_SCIENCE, false, true, m_pCity);
	dYieldArray[YIELD_CULTURE] += pPlot->calculateYield(YIELD_CULTURE, false, true, m_pCity);
	dYieldArray[YIELD_FAITH] += pPlot->calculateYield(YIELD_FAITH, false, true, m_pCity);

	if (adBonusYields != NULL)
	{
		dYieldArray[YIELD_FOOD] += adBonusYields[YIELD_FOOD];
		dYieldArray[YIELD_PRODUCTION] += adBonusYields[YIELD_PRODUCTION];
		dYieldArray[YIELD_GOLD] += adBonusYields[YIELD_GOLD];
		dYieldArray[YIELD_SCIENCE] += adBonusYields[YIELD_SCIENCE];
		dYieldArray[YIELD_CULTURE] += adBonusYields[YIELD_CULTURE];
		dYieldArray[YIELD_FAITH] += adBonusYields[YIELD_FAITH];
	}

	if (adBonusYieldModifiers != NULL)
	{
		dYieldArray[YIELD_FOOD] *= (m_pCity->getBaseYieldRateModifier(YIELD_FOOD) + adBonusYieldModifiers[YIELD_FOOD]) / 100.0;
		dYieldArray[YIELD_PRODUCTION] *= (m_pCity->getBaseYieldRateModifier(YIELD_PRODUCTION) + adBonusYieldModifiers[YIELD_PRODUCTION]) / 100.0;
		dYieldArray[YIELD_GOLD] *= (m_pCity->getBaseYieldRateModifier(YIELD_GOLD) + adBonusYieldModifiers[YIELD_GOLD]) / 100.0;
		dYieldArray[YIELD_SCIENCE] *= (m_pCity->getBaseYieldRateModifier(YIELD_SCIENCE) + adBonusYieldModifiers[YIELD_SCIENCE]) / 100.0;
		dYieldArray[YIELD_CULTURE] *= (m_pCity->getBaseYieldRateModifier(YIELD_CULTURE) + adBonusYieldModifiers[YIELD_CULTURE]) / 100.0;
		dYieldArray[YIELD_FAITH] *= (m_pCity->getBaseYieldRateModifier(YIELD_FAITH) + adBonusYieldModifiers[YIELD_FAITH]) / 100.0;
	}
	else
	{
		dYieldArray[YIELD_FOOD] *= m_pCity->getBaseYieldRateModifier(YIELD_FOOD) / 100.0;
		dYieldArray[YIELD_PRODUCTION] *= m_pCity->getBaseYieldRateModifier(YIELD_PRODUCTION) / 100.0;
		dYieldArray[YIELD_GOLD] *= m_pCity->getBaseYieldRateModifier(YIELD_GOLD) / 100.0;
		dYieldArray[YIELD_SCIENCE] *= m_pCity->getBaseYieldRateModifier(YIELD_SCIENCE) / 100.0;
		dYieldArray[YIELD_CULTURE] *= m_pCity->getBaseYieldRateModifier(YIELD_CULTURE) / 100.0;
		dYieldArray[YIELD_FAITH] *= m_pCity->getBaseYieldRateModifier(YIELD_FAITH) / 100.0;
	}

	double dTourismYield = dYieldArray[YIELD_CULTURE] * m_pCity->GetCityBuildings()->GetLandmarksTourismPercent() / 100.0;
	dTourismYield = m_pCity->GetCityCulture()->GetBaseTourism(true, int(m_pCity->GetCityCulture()->GetBaseTourismBeforeModifiers() + dTourismYield)) - m_pCity->GetCityCulture()->GetBaseTourism();
	
	return GetTotalValue(dYieldArray, NO_UNITCLASS, 0.0, iExtraHappiness, dTourismYield, iExtraGrowthMod, bAfterGrowth, bUseAllowGrowthFlag);
#else
	int iValue = 0;

	// Yield Values
	int iFoodYieldValue = (/*12*/ GC.getAI_CITIZEN_VALUE_FOOD() * pPlot->getYield(YIELD_FOOD));
#ifdef AUI_CITIZENS_IGNORE_FOOD_FOR_CITIZEN_ASSIGN_AFTER_GROW
	if (bAfterGrowth)
		iFoodYieldValue = 0;
#endif
	int iProductionYieldValue = (/*8*/ GC.getAI_CITIZEN_VALUE_PRODUCTION() * pPlot->getYield(YIELD_PRODUCTION));
	int iGoldYieldValue = (/*10*/ GC.getAI_CITIZEN_VALUE_GOLD() * pPlot->getYield(YIELD_GOLD));
	int iScienceYieldValue = (/*6*/ GC.getAI_CITIZEN_VALUE_SCIENCE() * pPlot->getYield(YIELD_SCIENCE));
	int iCultureYieldValue = (GC.getAI_CITIZEN_VALUE_CULTURE() * pPlot->getYield(YIELD_CULTURE));
	int iFaithYieldValue = (GC.getAI_CITIZEN_VALUE_FAITH() * pPlot->getYield(YIELD_FAITH));
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_YIELD_RATE_MODIFIERS
	iFoodYieldValue *= m_pCity->getBaseYieldRateModifier(YIELD_FOOD);
	iFoodYieldValue /= 100;
	iProductionYieldValue *= m_pCity->getBaseYieldRateModifier(YIELD_PRODUCTION);
	iProductionYieldValue /= 100;
	iGoldYieldValue *= m_pCity->getBaseYieldRateModifier(YIELD_GOLD);
	iGoldYieldValue /= 100;
	iScienceYieldValue *= m_pCity->getBaseYieldRateModifier(YIELD_SCIENCE);
	iScienceYieldValue /= 100;
	iCultureYieldValue *= m_pCity->getBaseYieldRateModifier(YIELD_CULTURE);
	iCultureYieldValue /= 100;
	iFaithYieldValue *= m_pCity->getBaseYieldRateModifier(YIELD_FAITH);
	iFaithYieldValue /= 100;
#endif
#ifdef AUI_CITIZENS_GET_VALUE_HIGHER_FAITH_VALUE_IF_BEFORE_RELIGION
	int iReligionsToFound = GC.getGame().GetGameReligions()->GetNumReligionsStillToFound();
	if (iReligionsToFound > 0)
	{
		int iMaxReligions = GC.getMap().getWorldInfo().getMaxActiveReligions() + 1;
		iFaithYieldValue += iFaithYieldValue * iReligionsToFound * iReligionsToFound / (iMaxReligions * iMaxReligions);
	}
#endif

	// How much surplus food are we making?
	int iExcessFoodTimes100 = m_pCity->getYieldRateTimes100(YIELD_FOOD, false) - (m_pCity->foodConsumption() * 100);
#if defined(AUI_CITIZENS_GET_VALUE_SPLIT_EXCESS_FOOD_MUTLIPLIER) || defined(AUI_CITIZENS_GET_VALUE_ALTER_FOOD_VALUE_IF_FOOD_PRODUCTION) || defined(AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS)
	int iExcessFoodWithPlotTimes100 = m_pCity->getYieldRateTimes100(YIELD_FOOD, false) - (m_pCity->foodConsumption() * 100) + pPlot->getYield(YIELD_FOOD) * m_pCity->getBaseYieldRateModifier(YIELD_FOOD);
#endif

	bool bAvoidGrowth = IsAvoidGrowth();

	// City Focus
	CityAIFocusTypes eFocus = GetFocusType();
	if(eFocus == CITY_AI_FOCUS_TYPE_FOOD)
		iFoodYieldValue *= 3;
	else if(eFocus == CITY_AI_FOCUS_TYPE_PRODUCTION)
		iProductionYieldValue *= 3;
	else if(eFocus == CITY_AI_FOCUS_TYPE_GOLD)
		iGoldYieldValue *= 3;
	else if(eFocus == CITY_AI_FOCUS_TYPE_SCIENCE)
		iScienceYieldValue *= 3;
	else if(eFocus == CITY_AI_FOCUS_TYPE_CULTURE)
		iCultureYieldValue *= 3;
	else if(eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH)
	{
		iFoodYieldValue *= 2;
		iGoldYieldValue *= 2;
	}
	else if(eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
	{
		iFoodYieldValue *= 2;
		iProductionYieldValue *= 2;
	}
	else if(eFocus == CITY_AI_FOCUS_TYPE_FAITH)
	{
		iFaithYieldValue *= 3;
	}

#ifdef AUI_CITIZENS_GET_VALUE_ALTER_FOOD_VALUE_IF_FOOD_PRODUCTION
	if (m_pCity->isFoodProduction())
	{
		iFoodYieldValue = 0;
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
		int iCurrProdFromFood = m_pCity->foodDifference(true, true, m_pCity->GetFoodProduction(iExcessFoodTimes100 / 100));
		int iProdFromFoodWithTile = m_pCity->foodDifference(true, true, m_pCity->GetFoodProduction(iExcessFoodWithPlotTimes100 / 100));
#else
		int iCurrProdFromFood = m_pCity->GetFoodProduction(iExcessFoodTimes100 / 100);
		int iProdFromFoodWithTile = m_pCity->GetFoodProduction(iExcessFoodWithPlotTimes100 / 100);
#endif
		iProductionYieldValue += (iProdFromFoodWithTile - iCurrProdFromFood) * GC.getAI_CITIZEN_VALUE_PRODUCTION() * (eFocus == CITY_AI_FOCUS_TYPE_PRODUCTION ? 3 : 0);
	}
	else
#endif
	// Food can be worth less if we don't want to grow
	if(bUseAllowGrowthFlag && iExcessFoodTimes100 >= 0 && bAvoidGrowth)
	{
		// If we at least have enough Food to feed everyone, zero out the value of additional food
		iFoodYieldValue = 0;
	}
	// We want to grow here
	else
	{
		// If we have a non-default and non-food focus, only worry about getting to 0 food
		if(eFocus != NO_CITY_AI_FOCUS_TYPE && eFocus != CITY_AI_FOCUS_TYPE_FOOD && eFocus != CITY_AI_FOCUS_TYPE_PROD_GROWTH && eFocus != CITY_AI_FOCUS_TYPE_GOLD_GROWTH)
		{
			int iFoodT100NeededFor0 = -iExcessFoodTimes100;

			if(iFoodT100NeededFor0 > 0)
			{
				iFoodYieldValue *= 8;
#ifdef AUI_CITIZENS_GET_VALUE_SPLIT_EXCESS_FOOD_MUTLIPLIER
				if (iExcessFoodWithPlotTimes100 > 0 && iFoodYieldValue != 0)
				{
					int iFutureExcessFoodYieldValue = /*12*/ GC.getAI_CITIZEN_VALUE_FOOD() * iExcessFoodWithPlotTimes100;
					if (eFocus == CITY_AI_FOCUS_TYPE_FOOD)
						iFutureExcessFoodYieldValue *= 3;
					else if (eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH || eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
						iFutureExcessFoodYieldValue *= 2;
					iFoodYieldValue -= iFutureExcessFoodYieldValue * 8 / 100;
					if (!bAvoidGrowth || !bUseAllowGrowthFlag)
					{
						iFoodYieldValue += iFutureExcessFoodYieldValue / 200;
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
						iFutureExcessFoodYieldValue = (m_pCity->foodDifferenceTimes100(true, NULL, true, iExcessFoodWithPlotTimes100) - iExcessFoodWithPlotTimes100) * /*12*/ GC.getAI_CITIZEN_VALUE_FOOD();
						if (eFocus == CITY_AI_FOCUS_TYPE_FOOD)
							iFutureExcessFoodYieldValue *= 3;
						else if (eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH || eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
							iFutureExcessFoodYieldValue *= 2;
						iFoodYieldValue += iFutureExcessFoodYieldValue / 200;
#endif
					}
				}
#endif
			}
			else
			{
				iFoodYieldValue /= 2;
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
				int iExtraFoodValueT100 = (m_pCity->foodDifferenceTimes100(true, NULL, true, iExcessFoodWithPlotTimes100 - iExcessFoodTimes100) -
					(iExcessFoodWithPlotTimes100 - iExcessFoodTimes100)) * /*12*/ GC.getAI_CITIZEN_VALUE_FOOD();
				if (eFocus == CITY_AI_FOCUS_TYPE_FOOD)
					iExtraFoodValueT100 *= 3;
				else if (eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH || eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
					iExtraFoodValueT100 *= 2;
				iFoodYieldValue += iExtraFoodValueT100 / 200;
#endif
			}
		}
		// If our surplus is not at least 2, really emphasize food plots
		else if(!bAvoidGrowth)
		{
#ifdef AUI_CITIZENS_GET_VALUE_SPLIT_EXCESS_FOOD_MUTLIPLIER
			int iFoodT100NeededFor2 = 100 * GC.getFOOD_CONSUMPTION_PER_POPULATION() - iExcessFoodTimes100;
#else
			int iFoodT100NeededFor2 = 200 - iExcessFoodTimes100;
#endif
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
			int iExtraFoodValueT100 = (m_pCity->foodDifferenceTimes100(true, NULL, true, iExcessFoodWithPlotTimes100 - iExcessFoodTimes100) -
				(iExcessFoodWithPlotTimes100 - iExcessFoodTimes100)) * /*12*/ GC.getAI_CITIZEN_VALUE_FOOD();
			if (eFocus == CITY_AI_FOCUS_TYPE_FOOD)
				iExtraFoodValueT100 *= 3;
			else if (eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH || eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
				iExtraFoodValueT100 *= 2;
#endif

			if(iFoodT100NeededFor2 > 0)
			{
				iFoodYieldValue *= 8;
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
				iExtraFoodValueT100 = 0;
#endif
#ifdef AUI_CITIZENS_GET_VALUE_SPLIT_EXCESS_FOOD_MUTLIPLIER
				if (iExcessFoodWithPlotTimes100 > 100 * GC.getFOOD_CONSUMPTION_PER_POPULATION() && iFoodYieldValue != 0)
				{
					int iFutureExcessFoodYieldValue = /*12*/ GC.getAI_CITIZEN_VALUE_FOOD() * (iExcessFoodWithPlotTimes100 - 100 * GC.getFOOD_CONSUMPTION_PER_POPULATION());
					if (eFocus == CITY_AI_FOCUS_TYPE_FOOD)
						iFutureExcessFoodYieldValue *= 3;
					else if (eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH || eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
						iFutureExcessFoodYieldValue *= 2;
					iFoodYieldValue -= iFutureExcessFoodYieldValue * 8 / 100;
					if (eFocus != CITY_AI_FOCUS_TYPE_FOOD)
						iFoodYieldValue += iFutureExcessFoodYieldValue / 200;
					else
						iFoodYieldValue += iFutureExcessFoodYieldValue / 100;
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
					iExtraFoodValueT100 = (m_pCity->foodDifferenceTimes100(true, NULL, true, iExcessFoodWithPlotTimes100 - 100 * GC.getFOOD_CONSUMPTION_PER_POPULATION()) -
						(iExcessFoodWithPlotTimes100 - 100 * GC.getFOOD_CONSUMPTION_PER_POPULATION())) * /*12*/ GC.getAI_CITIZEN_VALUE_FOOD();
					if (eFocus != CITY_AI_FOCUS_TYPE_FOOD)
						iExtraFoodValueT100 /= 2;
#endif
				}
#endif
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
#ifdef AUI_FAST_COMP
				iExtraFoodValueT100 += (m_pCity->foodDifferenceTimes100(true, NULL, true, FASTMIN(iExcessFoodWithPlotTimes100, 100 * GC.getFOOD_CONSUMPTION_PER_POPULATION())) -
					FASTMIN(iExcessFoodWithPlotTimes100, 100 * GC.getFOOD_CONSUMPTION_PER_POPULATION())) * 8 * /*12*/ GC.getAI_CITIZEN_VALUE_FOOD();
#else
				iExtraFoodValueT100 += (m_pCity->foodDifferenceTimes100(true, NULL, true, MIN(iExcessFoodWithPlotTimes100, 100 * GC.getFOOD_CONSUMPTION_PER_POPULATION())) -
					MIN(iExcessFoodWithPlotTimes100, 100 * GC.getFOOD_CONSUMPTION_PER_POPULATION())) * 8 * /*12*/ GC.getAI_CITIZEN_VALUE_FOOD();
#endif
				if (eFocus == CITY_AI_FOCUS_TYPE_FOOD)
					iExtraFoodValueT100 *= 3;
				else if (eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH || eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
					iExtraFoodValueT100 *= 2;
				iFoodYieldValue += iExtraFoodValueT100 / 100;
#endif
			}
			else if (eFocus != CITY_AI_FOCUS_TYPE_FOOD)
			{
				iFoodYieldValue /= 2;
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
				iFoodYieldValue += iExtraFoodValueT100 / 200;
			}
			else
			{
				iFoodYieldValue += iExtraFoodValueT100 / 100;
#endif
			}
		}
#ifdef AUI_CITIZENS_FIX_GET_VALUE_FOOD_YIELD_VALUE_WHEN_STARVATION_WITH_AVOID_GROWTH
		// Food focus and negative food, but with avoid growth enabled for some reason
		else
		{
			iFoodYieldValue *= 8;
#ifdef AUI_CITIZENS_GET_VALUE_SPLIT_EXCESS_FOOD_MUTLIPLIER
			if (iExcessFoodWithPlotTimes100 > 0 && iFoodYieldValue != 0)
			{
				int iFutureExcessFoodYieldValue = /*12*/ GC.getAI_CITIZEN_VALUE_FOOD() * iExcessFoodWithPlotTimes100;
				if (eFocus == CITY_AI_FOCUS_TYPE_FOOD)
					iFutureExcessFoodYieldValue *= 3;
				else if (eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH || eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
					iFutureExcessFoodYieldValue *= 2;
				iFoodYieldValue -= iFutureExcessFoodYieldValue * 8 / 100;
			}
#endif
		}
#endif
	}

	if((eFocus == NO_CITY_AI_FOCUS_TYPE || eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH || eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH) && !bAvoidGrowth && m_pCity->getPopulation() < 5)
	{
		iFoodYieldValue *= 4;
	}

	iValue += iFoodYieldValue;
	iValue += iProductionYieldValue;
	iValue += iGoldYieldValue;
	iValue += iScienceYieldValue;
	iValue += iCultureYieldValue;
	iValue += iFaithYieldValue;

	return iValue;
#endif
}

#ifdef AUI_CITIZENS_GET_VALUE_FROM_STATS
/// Get the total value of a set of yields, great person points, tourism, and happiness
int CvCityCitizens::GetTotalValue(double* aiYields, UnitClassTypes eGreatPersonClass, double dGPPYield, int iExtraHappiness, double dTourismYield, int iExtraGrowthMod, bool bAfterGrowth, bool bUseAvoidGrowth) const
{
	double dValue = 0.0;

	// Yield Values (note that generic modifiers are not calculated in this function, since not all set of yields will get the same modifiers)
	double dFoodYield = aiYields[YIELD_FOOD];
	double dProductionYield = aiYields[YIELD_PRODUCTION];
	double dGoldYield = aiYields[YIELD_GOLD];
	double dScienceYield = aiYields[YIELD_SCIENCE];
	double dCultureYield = aiYields[YIELD_CULTURE];
	double dFaithYield = aiYields[YIELD_FAITH];

	double dCurrentExcessFood = m_pCity->getYieldRateTimes100(YIELD_FOOD, false) / 100.0 - m_pCity->foodConsumption();
	double dExcessFoodYield = 0.0;
	if (dCurrentExcessFood + dFoodYield > 0)
	{
		if (dCurrentExcessFood > 0)
		{
			dExcessFoodYield = dFoodYield;
			dFoodYield = 0.0;
		}
		else
		{
			dExcessFoodYield = dFoodYield + dCurrentExcessFood;
			dFoodYield = -dCurrentExcessFood;
		}
	}
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
	dExcessFoodYield += (m_pCity->foodDifferenceTimes100(iExtraHappiness, iExtraGrowthMod) - m_pCity->foodDifferenceTimes100()) / 100.0;
#endif

	if (m_pCity->isFoodProduction())
	{
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
		dProductionYield += (m_pCity->foodDifferenceTimes100(iExtraHappiness, iExtraGrowthMod, true, NULL, true, m_pCity->GetFoodProduction(int(100 * (dCurrentExcessFood + dExcessFoodYield)))) -
			m_pCity->foodDifferenceTimes100(0, 0, true, NULL, true, m_pCity->GetFoodProduction(int(100 * dCurrentExcessFood)))) / 100.0;
#else
		dProductionYield += (m_pCity->GetFoodProduction(int(100 * (dCurrentExcessFood + dExcessFoodYield))) - m_pCity->GetFoodProduction(int(100 * dCurrentExcessFood))) / 100.0;
#endif
		dExcessFoodYield = 0.0;
	}
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
	else
		dExcessFoodYield = m_pCity->foodDifferenceTimes100(iExtraHappiness, iExtraGrowthMod, true, NULL, true, int(100 * dExcessFoodYield)) / 100.0;
#endif

	double dPlayerIncome = GetPlayer()->GetTreasury()->CalculateBaseNetGoldTimes100() / 100.0;
	double dPercentGoldToScience = 0.0;
	if (dPlayerIncome < 0)
	{
		if (dPlayerIncome + dGoldYield > 0)
		{
			// Income is negative, so adding it to gold yield reduces the gold yield
			dGoldYield += dPlayerIncome;
			dScienceYield -= dPlayerIncome;
			dPercentGoldToScience = 1.0 - dGoldYield / (dGoldYield - dPlayerIncome);
		}
		else
		{
			dScienceYield += dGoldYield;
			dGoldYield = 0;
			dPercentGoldToScience = 1.0;
		}
	}

	double dFoodYieldValue = GC.getAI_CITIZEN_VALUE_FOOD();
	double dExcessFoodYieldValue = dFoodYieldValue;
	dFoodYieldValue *= 8.0;
	double dProductionYieldValue = GC.getAI_CITIZEN_VALUE_PRODUCTION();
	double dGoldYieldValue = GC.getAI_CITIZEN_VALUE_GOLD();
	double dScienceYieldValue = GC.getAI_CITIZEN_VALUE_SCIENCE();
	double dCultureYieldValue = GC.getAI_CITIZEN_VALUE_CULTURE();
	double dFaithYieldValue = GC.getAI_CITIZEN_VALUE_FAITH();
	double dTourismYieldValue = dCultureYieldValue;
	double dGPPYieldValue = 2.0;
	double dExcessHappiness = m_pCity->GetPlayer()->GetExcessHappiness();
	double dHappinessValue = 4.0 * GC.getAI_CITIZEN_VALUE_FOOD() / (1.0 + exp(dExcessHappiness / 4.0 * (1.0 + exp(-dExcessHappiness / 10.0))));

	// Extra multiplier (or divider) based on food needed to grow
	dExcessFoodYieldValue *= 1.0 + 6.0 / (1.0 + exp(double(m_pCity->GetPlayer()->getGrowthThreshold(m_pCity->getPopulation())) / double(m_pCity->GetPlayer()->getGrowthThreshold(1)) - 1));
	if (bAfterGrowth)
		dFoodYieldValue = 0.0;
	int iUnhappinessOnGrowth = 0;
	if (!m_pCity->IsIgnoreCityForHappiness())
	{
		iUnhappinessOnGrowth = GC.getUNHAPPINESS_PER_POPULATION();
		if (m_pCity->IsOccupied() && !m_pCity->IsNoOccupiedUnhappiness())
			iUnhappinessOnGrowth = (int((m_pCity->getPopulation() + 1) * GC.getUNHAPPINESS_PER_OCCUPIED_POPULATION() * 100) -
				int(m_pCity->getPopulation() * GC.getUNHAPPINESS_PER_OCCUPIED_POPULATION() * 100)) / 100;
	}
	if (IsAvoidGrowth(iExtraHappiness - iUnhappinessOnGrowth) && bUseAvoidGrowth)
		dExcessFoodYieldValue = 0.0;

#ifdef AUI_CITIZENS_GET_VALUE_HIGHER_FAITH_VALUE_IF_BEFORE_RELIGION
	int iReligionsToFound = GC.getGame().GetGameReligions()->GetNumReligionsStillToFound();
	if (iReligionsToFound > 0)
	{
		int iMaxReligions = GC.getMap().getWorldInfo().getMaxActiveReligions() + 1;
		dFaithYieldValue += dFaithYieldValue * iReligionsToFound * iReligionsToFound / double(iMaxReligions * iMaxReligions);
	}
#endif

	CvGrandStrategyAI* pGrandStrategyAI = GetPlayer()->GetGrandStrategyAI();
#ifdef AUI_GS_PRIORITY_RATIO
	dTourismYieldValue *= pGrandStrategyAI->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"));
#else
	if (pGrandStrategyAI->GetActiveGrandStrategy() != (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
	{
		dTourismYieldValue = 0.0;
	}
#endif

	if (eGreatPersonClass != NO_UNITCLASS)
	{
		SpecialistTypes eSpecialist = NO_SPECIALIST;
		CvSpecialistInfo* pkSpecialistInfo;
		for (int iSpecialistLoop = 0; iSpecialistLoop < GC.getNumSpecialistInfos(); iSpecialistLoop++)
		{
			const SpecialistTypes eLoopSpecialist = static_cast<SpecialistTypes>(iSpecialistLoop);
			pkSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
			if (pkSpecialistInfo && pkSpecialistInfo->getGreatPeopleUnitClass() == eGreatPersonClass)
			{
				eSpecialist = eLoopSpecialist;
				break;
			}
		}
		if (eSpecialist != NO_SPECIALIST)
		{
			int iMyTurnsToGP = GetTurnsToGP(eSpecialist, dGPPYield);
			int iBaseTurnsToGP = GetTurnsToGP(eSpecialist);
			int iBestOtherTurnsToGP = MAX_INT;
			int iLoop = 0;
			for (CvCity* pLoopCity = GetPlayer()->firstCity(&iLoop); pLoopCity != NULL; pLoopCity = GetPlayer()->nextCity(&iLoop))
			{
				if (pLoopCity != m_pCity)
				{
					int iLoopTurns = pLoopCity->GetCityCitizens()->GetTurnsToGP(eSpecialist);
					if (iLoopTurns < iBestOtherTurnsToGP)
						iBestOtherTurnsToGP = iLoopTurns;
				}
			}
			if (iMyTurnsToGP > iBestOtherTurnsToGP)
			{
				dGPPYieldValue /= log(M_E + iMyTurnsToGP - iBestOtherTurnsToGP);
			}
			else if (iMyTurnsToGP < MAX_INT)
			{
				if (iBaseTurnsToGP < MAX_INT)
					dGPPYieldValue *= log(1.0 + iBaseTurnsToGP - iMyTurnsToGP);
				else
					dGPPYieldValue *= log(1.0 + iMyTurnsToGP);
			}
		}
		UnitTypes eGPUnitType = (UnitTypes)GetPlayer()->getCivilizationInfo().getCivilizationUnits(eGreatPersonClass);
		CvUnitEntry *pkGPEntry = GC.GetGameUnits()->GetEntry(eGPUnitType);
		if (pkGPEntry)
		{
			int iBestFlavor = 0;
			for (int iFlavorLoop = 0; iFlavorLoop < GC.getNumFlavorTypes(); iFlavorLoop++)
			{
				if (pkGPEntry->GetFlavorValue(iFlavorLoop) > 0)
				{
					iBestFlavor = MAX(iBestFlavor, pGrandStrategyAI->GetPersonalityAndGrandStrategy((FlavorTypes)iFlavorLoop));
				}
			}
			if (iBestFlavor > 0)
			{
				dGPPYieldValue *= pow(1.05, (double)(iBestFlavor - GC.getDEFAULT_FLAVOR_VALUE()));
			}
		}
		if (eGreatPersonClass == GC.getInfoTypeForString("UNITCLASS_SCIENTIST"))
		{
			dGPPYieldValue *= 2.0;
			if (GetPlayer()->GetUniqueGreatPersons() & UNIQUE_GP_SCIENTIST)
				dGPPYieldValue *= 2.0;
		}
		else if (eGreatPersonClass == GC.getInfoTypeForString("UNITCLASS_WRITER"))
		{
#ifdef AUI_GS_PRIORITY_RATIO
			dGPPYieldValue *= 1.0 + pGrandStrategyAI->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"));
#else
			if (pGrandStrategyAI->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
			{
				dGPPYieldValue *= 2.0;
			}
#endif
			if (GetPlayer()->GetUniqueGreatPersons() & UNIQUE_GP_WRITER)
				dGPPYieldValue *= 2.0;
		}
		else if (eGreatPersonClass == GC.getInfoTypeForString("UNITCLASS_ARTIST"))
		{
#ifdef AUI_GS_PRIORITY_RATIO
			dGPPYieldValue *= 1.0 + pGrandStrategyAI->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"));
#else
			if (pGrandStrategyAI->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
			{
				dGPPYieldValue *= 2.0;
			}
#endif
			if (GetPlayer()->GetUniqueGreatPersons() & UNIQUE_GP_ARTIST)
				dGPPYieldValue *= 2.0;
		}
		else if (eGreatPersonClass == GC.getInfoTypeForString("UNITCLASS_MUSICIAN"))
		{
#ifdef AUI_GS_PRIORITY_RATIO
			dGPPYieldValue *= 1.0 + pGrandStrategyAI->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"));
#else
			if (pGrandStrategyAI->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
			{
				dGPPYieldValue *= 2.0;
			}
#endif
			if (GetPlayer()->GetUniqueGreatPersons() & UNIQUE_GP_MUSICIAN)
				dGPPYieldValue *= 2.0;
		}
		else if (eGreatPersonClass == GC.getInfoTypeForString("UNITCLASS_MERCHANT"))
		{
#ifdef AUI_GS_PRIORITY_RATIO
			dGPPYieldValue *= 1.0 + pGrandStrategyAI->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS"));
#else
			if (pGrandStrategyAI->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS"))
			{
				dGPDesireFactor *= 2.0;
			}
#endif
			if (GetPlayer()->GetUniqueGreatPersons() & UNIQUE_GP_MERCHANT)
				dGPPYieldValue *= 2.0;
		}
		else if (eGreatPersonClass == GC.getInfoTypeForString("UNITCLASS_ENGINEER"))
		{
			dGPPYieldValue *= 1.5;
		}
		else if (eGreatPersonClass == GC.getInfoTypeForString("UNITCLASS_GENERAL"))
		{
#ifdef AUI_GS_PRIORITY_RATIO
			dGPPYieldValue *= 1.0 + pGrandStrategyAI->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"));
#else
			if (pGrandStrategyAI->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"))
			{
				dGPDesireFactor *= 2.0;
			}
#endif
			if (GetPlayer()->GetUniqueGreatPersons() & UNIQUE_GP_GENERAL)
				dGPPYieldValue *= 2.0;
		}
		else if (eGreatPersonClass == GC.getInfoTypeForString("UNITCLASS_ADMIRAL"))
		{
#ifdef AUI_GS_PRIORITY_RATIO
			dGPPYieldValue *= 1.0 + pGrandStrategyAI->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"));
#else
			if (GetPlayer()->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"))
			{
				dGPDesireFactor *= 2.0;
			}
#endif
			if (GetPlayer()->GetUniqueGreatPersons() & UNIQUE_GP_ADMIRAL)
				dGPPYieldValue *= 2.0;
		}
	}

	// City Focus
	CityAIFocusTypes eFocus = GetFocusType();
	if (eFocus == CITY_AI_FOCUS_TYPE_FOOD)
	{
		dFoodYieldValue *= 3.0;
		dExcessFoodYieldValue *= 3.0;
	}
	else if (eFocus == CITY_AI_FOCUS_TYPE_PRODUCTION)
		dProductionYieldValue *= 3.0;
	else if (eFocus == CITY_AI_FOCUS_TYPE_GOLD)
	{
		dGoldYieldValue *= 3.0;
		dScienceYieldValue *= 1.0 + 2.0 * dPercentGoldToScience;
	}
	else if (eFocus == CITY_AI_FOCUS_TYPE_SCIENCE)
	{
		dFoodYieldValue *= 2.0;
		dExcessFoodYieldValue *= 2.0;
		dScienceYieldValue *= 3.0;
	}
	else if (eFocus == CITY_AI_FOCUS_TYPE_CULTURE)
	{
		dCultureYieldValue *= 3.0;
		dTourismYieldValue *= 3.0;
	}
	else if (eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH)
	{
		dFoodYieldValue *= 2.0;
		dExcessFoodYieldValue *= 2.0;
		dGoldYieldValue *= 2.0;
	}
	else if (eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
	{
		dFoodYieldValue *= 2.0;
		dExcessFoodYieldValue *= 2.0;
		dProductionYieldValue *= 2.0;
	}
	else if (eFocus == CITY_AI_FOCUS_TYPE_FAITH)
	{
		dFaithYieldValue *= 3.0;
	}
	else if (eFocus == CITY_AI_FOCUS_TYPE_GREAT_PEOPLE)
	{
		dGPPYieldValue *= 3.0;
	}

	// Flavor modifiers that are roughly +/-10%
	if (!GetPlayer()->isHuman())
	{
		double dFoodFlavor = (double)pGrandStrategyAI->GetPersonalityAndGrandStrategy((FlavorTypes)GC.getInfoTypeForString("FLAVOR_GROWTH"));
		dFoodFlavor += (double)pGrandStrategyAI->GetPersonalityAndGrandStrategy((FlavorTypes)GC.getInfoTypeForString("FLAVOR_EXPANSION"));
		dFoodFlavor /= 2.0;
		dExcessFoodYieldValue *= pow(1.02, dFoodFlavor - GC.getDEFAULT_FLAVOR_VALUE());

		double dProductionFlavor = (double)pGrandStrategyAI->GetPersonalityAndGrandStrategy((FlavorTypes)GC.getInfoTypeForString("FLAVOR_PRODUCTION"));
		dProductionFlavor += (double)pGrandStrategyAI->GetPersonalityAndGrandStrategy((FlavorTypes)GC.getInfoTypeForString("FLAVOR_WONDER"));
		dProductionFlavor += (double)FASTMAX(pGrandStrategyAI->GetPersonalityAndGrandStrategy((FlavorTypes)GC.getInfoTypeForString("FLAVOR_OFFENSE")), 
										 pGrandStrategyAI->GetPersonalityAndGrandStrategy((FlavorTypes)GC.getInfoTypeForString("FLAVOR_DEFENSE")));
		dProductionFlavor /= 3.0;
		dProductionYieldValue *= pow(1.02, dProductionFlavor - GC.getDEFAULT_FLAVOR_VALUE());

		double dGoldFlavor = (double)pGrandStrategyAI->GetPersonalityAndGrandStrategy((FlavorTypes)GC.getInfoTypeForString("FLAVOR_GOLD"));
		dGoldFlavor += (double)pGrandStrategyAI->GetPersonalityAndGrandStrategy((FlavorTypes)GC.getInfoTypeForString("FLAVOR_DIPLOMACY"));
		dGoldFlavor /= 2.0;
		dGoldYieldValue *= pow(1.02, dGoldFlavor - GC.getDEFAULT_FLAVOR_VALUE());

		dScienceYieldValue *= pow(1.02, (double)pGrandStrategyAI->GetPersonalityAndGrandStrategy((FlavorTypes)GC.getInfoTypeForString("FLAVOR_SCIENCE")) - GC.getDEFAULT_FLAVOR_VALUE());
		dCultureYieldValue *= pow(1.02, (double)pGrandStrategyAI->GetPersonalityAndGrandStrategy((FlavorTypes)GC.getInfoTypeForString("FLAVOR_CULTURE")) - GC.getDEFAULT_FLAVOR_VALUE());
		dFaithYieldValue *= pow(1.02, (double)pGrandStrategyAI->GetPersonalityAndGrandStrategy((FlavorTypes)GC.getInfoTypeForString("FLAVOR_RELIGION")) - GC.getDEFAULT_FLAVOR_VALUE());
		dTourismYieldValue *= pow(1.02, (double)pGrandStrategyAI->GetPersonalityAndGrandStrategy((FlavorTypes)GC.getInfoTypeForString("FLAVOR_TOURISM")) - GC.getDEFAULT_FLAVOR_VALUE());
		dGPPYieldValue *= pow(1.02, (double)pGrandStrategyAI->GetPersonalityAndGrandStrategy((FlavorTypes)GC.getInfoTypeForString("FLAVOR_GREAT_PEOPLE")) - GC.getDEFAULT_FLAVOR_VALUE());
		dHappinessValue *= pow(1.02, (double)pGrandStrategyAI->GetPersonalityAndGrandStrategy((FlavorTypes)GC.getInfoTypeForString("FLAVOR_HAPPINESS")) - GC.getDEFAULT_FLAVOR_VALUE());
	}

	dValue += dFoodYield * dFoodYieldValue;
	dValue += dExcessFoodYield * dExcessFoodYieldValue;
	dValue += dProductionYield * dProductionYieldValue;
	dValue += dGoldYield * dGoldYieldValue;
	dValue += dScienceYield * dScienceYieldValue;
	dValue += dCultureYield * dCultureYieldValue;
	dValue += dTourismYield * dTourismYieldValue;
	dValue += dFaithYield * dFaithYieldValue;
	dValue += dGPPYield * dGPPYieldValue;
	dValue += iExtraHappiness * dHappinessValue;

	return int(dValue + 0.5);
}

int CvCityCitizens::GetTurnsToGP(SpecialistTypes eSpecialist, double dExtraGPP) const
{
	CvSpecialistInfo* pkSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
	if (pkSpecialistInfo)
	{
		UnitClassTypes eGPUnitClass = (UnitClassTypes)pkSpecialistInfo->getGreatPeopleUnitClass();
		// Does this Specialist spawn a GP?
		if (eGPUnitClass != NO_UNITCLASS)
		{
			double dGPThreshold = GetSpecialistUpgradeThreshold(eGPUnitClass) - GetSpecialistGreatPersonProgressTimes100(eSpecialist) / 100.0;
			int iCount = GetSpecialistCount(eSpecialist);

			// GPP from Specialists
			dExtraGPP += pkSpecialistInfo->getGreatPeopleRateChange() * iCount;

			// GPP from Buildings
			dExtraGPP += GetBuildingGreatPeopleRateChanges(eSpecialist);

			if (dExtraGPP > 0)
			{
				int iMod = 0;

				// City mod
				iMod += GetCity()->getGreatPeopleRateModifier();

				// Player mod
				iMod += GetPlayer()->getGreatPeopleRateModifier();

				// Player and Golden Age mods to this specific class
				if (eGPUnitClass == GC.getInfoTypeForString("UNITCLASS_SCIENTIST"))
				{
					iMod += GetPlayer()->getGreatScientistRateModifier();
				}
				else if (eGPUnitClass == GC.getInfoTypeForString("UNITCLASS_WRITER"))
				{
					if (GetPlayer()->isGoldenAge())
					{
						iMod += GetPlayer()->GetPlayerTraits()->GetGoldenAgeGreatWriterRateModifier();
					}
					iMod += GetPlayer()->getGreatWriterRateModifier();
				}
				else if (eGPUnitClass == GC.getInfoTypeForString("UNITCLASS_ARTIST"))
				{
					if (GetPlayer()->isGoldenAge())
					{
						iMod += GetPlayer()->GetPlayerTraits()->GetGoldenAgeGreatArtistRateModifier();
					}
					iMod += GetPlayer()->getGreatArtistRateModifier();
				}
				else if (eGPUnitClass == GC.getInfoTypeForString("UNITCLASS_MUSICIAN"))
				{
					if (GetPlayer()->isGoldenAge())
					{
						iMod += GetPlayer()->GetPlayerTraits()->GetGoldenAgeGreatMusicianRateModifier();
					}
					iMod += GetPlayer()->getGreatMusicianRateModifier();
				}
				else if (eGPUnitClass == GC.getInfoTypeForString("UNITCLASS_MERCHANT"))
				{
					iMod += GetPlayer()->getGreatMerchantRateModifier();
				}
				else if (eGPUnitClass == GC.getInfoTypeForString("UNITCLASS_ENGINEER"))
				{
					iMod += GetPlayer()->getGreatEngineerRateModifier();
				}

				// Apply mod
				dExtraGPP *= (100.0 + iMod) / 100.0;

				return int(dGPThreshold / dExtraGPP + 0.5);
			}
			else
				return MAX_INT;
		}
	}

	return MAX_INT;
}
#endif

/// Are this City's Citizens under automation?
bool CvCityCitizens::IsAutomated() const
{
	return m_bAutomated;
}

/// Sets this City's Citizens to be under automation
void CvCityCitizens::SetAutomated(bool bValue)
{
	m_bAutomated = bValue;
}

/// Are this City's Specialists under automation?
bool CvCityCitizens::IsNoAutoAssignSpecialists() const
{
	return m_bNoAutoAssignSpecialists;
}

/// Sets this City's Specialists to be under automation
void CvCityCitizens::SetNoAutoAssignSpecialists(bool bValue)
{
	if(m_bNoAutoAssignSpecialists != bValue)
	{
		m_bNoAutoAssignSpecialists = bValue;

		// If we're giving the AI control clear all manually assigned Specialists
		if(!bValue)
		{
			DoClearForcedSpecialists();
		}

		DoReallocateCitizens();
	}
}

/// Is this City avoiding growth?
#ifdef AUI_CITIZENS_GET_VALUE_FROM_STATS
bool CvCityCitizens::IsAvoidGrowth(int iExtraHappiness) const
#else
#ifdef AUI_CONSTIFY
bool CvCityCitizens::IsAvoidGrowth() const
#else
bool CvCityCitizens::IsAvoidGrowth()
#endif
#endif
{
#ifdef AUI_CITIZENS_FIX_AVOID_GROWTH_FLAG_NOT_IGNORED_IF_NO_HAPPINESS
#ifdef AUI_CITIZENS_GET_VALUE_FROM_STATS
	if(GetPlayer()->IsEmpireUnhappy(iExtraHappiness))
#else
	if(GetPlayer()->IsEmpireUnhappy())
#endif
#else
	if(GC.getGame().isOption(GAMEOPTION_NO_HAPPINESS))
	{
		return false;
	}

#ifdef AUI_CITIZENS_GET_VALUE_FROM_STATS
	if (GetPlayer()->GetExcessHappiness() + iExtraHappiness < 0)
#else
	if(GetPlayer()->GetExcessHappiness() < 0)
#endif
#endif
	{
#ifdef AUI_CITIZENS_FIX_FORCED_AVOID_GROWTH_ONLY_WHEN_GROWING_LOWERS_HAPPINESS
		int iPopulation = m_pCity->getPopulation();
		int iLocalHappinessCap = iPopulation;

		// India has unique way to compute local happiness cap
		if (GetPlayer()->GetPlayerTraits()->GetCityUnhappinessModifier() != 0)
		{
			// 0.67 per population, rounded up
			iLocalHappinessCap = (iLocalHappinessCap * 20) + 15;
			iLocalHappinessCap /= 30;
		}
		// Growing would not be covered by local happiness
		if (m_pCity->GetLocalHappiness() < iLocalHappinessCap)
		{
			int iHappinessPerXPopulation = GetPlayer()->GetHappinessPerXPopulation();
			// Growing would not be covered by happiness per X population
			if (iHappinessPerXPopulation == 0 || m_pCity->IsPuppet() || (iPopulation + 1) / iHappinessPerXPopulation <= iPopulation / iHappinessPerXPopulation)
			{
				// Growing would not be covered by reduced unhappiness from population
				if ((GetPlayer()->GetUnhappinessFromCityPopulation() + GetPlayer()->GetUnhappinessFromOccupiedCities()) / 100 <
					(GetPlayer()->GetUnhappinessFromCityPopulation(NULL, NULL, m_pCity) + GetPlayer()->GetUnhappinessFromOccupiedCities(NULL, NULL, m_pCity)) / 100)
					return true;
			}
		}
#else
		return true;
#endif
	}

	return IsForcedAvoidGrowth();
}

#if defined(AUI_CONSTIFY) || defined(AUI_CITIZENS_GET_VALUE_FROM_STATS)
bool CvCityCitizens::IsForcedAvoidGrowth() const
#else
bool CvCityCitizens::IsForcedAvoidGrowth()
#endif
{
	return m_bForceAvoidGrowth;
}

void CvCityCitizens::SetForcedAvoidGrowth(bool bAvoidGrowth)
{
	if(m_bForceAvoidGrowth != bAvoidGrowth)
	{
		m_bForceAvoidGrowth = bAvoidGrowth;
		DoReallocateCitizens();
	}
}

/// What is this city focusing on?
CityAIFocusTypes CvCityCitizens::GetFocusType() const
{
	return m_eCityAIFocusTypes;
}

/// What is this city focusing on?
void CvCityCitizens::SetFocusType(CityAIFocusTypes eFocus)
{
	FAssert(eFocus >= NO_CITY_AI_FOCUS_TYPE);
	FAssert(eFocus < NUM_CITY_AI_FOCUS_TYPES);

	if(eFocus != m_eCityAIFocusTypes)
	{
		m_eCityAIFocusTypes = eFocus;
		// Reallocate with our new focus
		DoReallocateCitizens();
	}
}

#ifndef AUI_PRUNING
/// Does the AI want a Specialist?
bool CvCityCitizens::IsAIWantSpecialistRightNow()
{
	int iWeight = 100;

	// If the City is Size 1 or 2 then we probably don't want Specialists
	if(m_pCity->getPopulation() < 3)
	{
		iWeight /= 2;
	}

	int iFoodPerTurn = m_pCity->getYieldRate(YIELD_FOOD, false);
	int iFoodEatenPerTurn = m_pCity->foodConsumption();
	int iSurplusFood = iFoodPerTurn - iFoodEatenPerTurn;

	CityAIFocusTypes eFocusType = GetFocusType();

	// Don't want specialists until we've met our food needs
	if(iSurplusFood < 0)
	{
		return false;
	}
	else if(IsAvoidGrowth() && (eFocusType == NO_CITY_AI_FOCUS_TYPE || eFocusType == CITY_AI_FOCUS_TYPE_GREAT_PEOPLE))
	{
		iWeight *= 2;
	}
	else if(iSurplusFood <= 2)
	{
		iWeight /= 2;
	}
	else if(iSurplusFood > 2)
	{
		if(eFocusType == NO_CITY_AI_FOCUS_TYPE || eFocusType == CITY_AI_FOCUS_TYPE_GREAT_PEOPLE || eFocusType == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
		{
			iWeight *= 100 + (20 * (iSurplusFood - 4));
			iWeight /= 100;
		}
	}

	// If we're deficient in Production then we're less likely to want Specialists
	if(m_pCity->GetCityStrategyAI()->IsYieldDeficient(YIELD_PRODUCTION))
	{
		iWeight *= 50;
		iWeight /= 100;
	}
	// if we've got some slackers in town (since they provide Production)
	else if(GetNumDefaultSpecialists() > 0 && eFocusType != CITY_AI_FOCUS_TYPE_PRODUCTION && eFocusType != CITY_AI_FOCUS_TYPE_PROD_GROWTH)
	{
		iWeight *= 150;
		iWeight /= 100;
	}

	// Someone told this AI it should be focused on something that is usually gotten from specialists
	if(eFocusType == CITY_AI_FOCUS_TYPE_GREAT_PEOPLE)
	{
		// Loop through all Buildings
		BuildingTypes eBuilding;
		for(int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
		{
			eBuilding = (BuildingTypes) iBuildingLoop;

			// Have this Building in the City?
			if(m_pCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
			{
				// Can't add more than the max
				if(IsCanAddSpecialistToBuilding(eBuilding))
				{
					iWeight *= 3;
					break;
				}
			}
		}
	}
	else if(eFocusType == CITY_AI_FOCUS_TYPE_CULTURE)
	{
		// Loop through all Buildings
		for(int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
		{
			const BuildingTypes eBuilding = static_cast<BuildingTypes>(iBuildingLoop);
			CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
			if(pkBuildingInfo)
			{
				// Have this Building in the City?
				if(m_pCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
				{
					// Can't add more than the max
					if(IsCanAddSpecialistToBuilding(eBuilding))
					{
						const SpecialistTypes eSpecialist = (SpecialistTypes) pkBuildingInfo->GetSpecialistType();
						CvSpecialistInfo* pSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
						if(pSpecialistInfo && pSpecialistInfo->getCulturePerTurn() > 0)
						{
							iWeight *= 3;
							break;
						}
					}
				}
			}
		}
	}
	else if(eFocusType == CITY_AI_FOCUS_TYPE_SCIENCE)
	{
		// Loop through all Buildings
		BuildingTypes eBuilding;
		for(int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
		{
			eBuilding = (BuildingTypes) iBuildingLoop;

			// Have this Building in the City?
			if(m_pCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
			{
				// Can't add more than the max
				if(IsCanAddSpecialistToBuilding(eBuilding))
				{
					SpecialistTypes eSpecialist = (SpecialistTypes) GC.getBuildingInfo(eBuilding)->GetSpecialistType();
					CvSpecialistInfo* pSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
					if(pSpecialistInfo && pSpecialistInfo->getYieldChange(YIELD_SCIENCE) > 0)
					{
						iWeight *= 3;
					}

					if(GetPlayer()->getSpecialistExtraYield(YIELD_SCIENCE) > 0)
					{
						iWeight *= 3;
					}

					if(GetPlayer()->GetPlayerTraits()->GetSpecialistYieldChange(eSpecialist, YIELD_SCIENCE) > 0)
					{
						iWeight *= 3;
					}
				}
			}
		}
	}
	else if(eFocusType == CITY_AI_FOCUS_TYPE_PRODUCTION)
	{
		// Loop through all Buildings
		for(int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
		{
			const BuildingTypes eBuilding = static_cast<BuildingTypes>(iBuildingLoop);
			CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
			if(pkBuildingInfo)
			{
				// Have this Building in the City?
				if(m_pCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
				{
					// Can't add more than the max
					if(IsCanAddSpecialistToBuilding(eBuilding))
					{
						SpecialistTypes eSpecialist = (SpecialistTypes) pkBuildingInfo->GetSpecialistType();
						CvSpecialistInfo* pSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
						if(NULL != pSpecialistInfo && pSpecialistInfo->getYieldChange(YIELD_PRODUCTION) > 0)
						{
							iWeight *= 150;
							iWeight /= 100;
						}

						if(GetPlayer()->getSpecialistExtraYield(YIELD_PRODUCTION) > 0)
						{
							iWeight *= 2;
						}

						if(GetPlayer()->GetPlayerTraits()->GetSpecialistYieldChange(eSpecialist, YIELD_PRODUCTION) > 0)
						{
							iWeight *= 2;
						}
					}
				}
			}
		}
	}
	else if(eFocusType == CITY_AI_FOCUS_TYPE_GOLD)
	{
		// Loop through all Buildings
		BuildingTypes eBuilding;
		for(int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
		{
			eBuilding = (BuildingTypes) iBuildingLoop;

			// Have this Building in the City?
			if(m_pCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
			{
				// Can't add more than the max
				if(IsCanAddSpecialistToBuilding(eBuilding))
				{
					SpecialistTypes eSpecialist = (SpecialistTypes) GC.getBuildingInfo(eBuilding)->GetSpecialistType();
					CvSpecialistInfo* pSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
					if(pSpecialistInfo && pSpecialistInfo->getYieldChange(YIELD_GOLD) > 0)
					{
						iWeight *= 150;
						iWeight /= 100;
						break;
					}
				}
			}
		}
	}
	else if(eFocusType == CITY_AI_FOCUS_TYPE_FOOD)
	{
		iWeight *= 50;
		iWeight /= 100;
	}
	else if(eFocusType == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
	{
		// Loop through all Buildings
		for(int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
		{
			const BuildingTypes eBuilding = static_cast<BuildingTypes>(iBuildingLoop);
			CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
			if(pkBuildingInfo)
			{
				// Have this Building in the City?
				if(m_pCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
				{
					// Can't add more than the max
					if(IsCanAddSpecialistToBuilding(eBuilding))
					{
						SpecialistTypes eSpecialist = (SpecialistTypes) pkBuildingInfo->GetSpecialistType();
						CvSpecialistInfo* pSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
						if(pSpecialistInfo && pSpecialistInfo->getYieldChange(YIELD_PRODUCTION) > 0)
						{
							iWeight *= 150;
							iWeight /= 100;
							break;
						}
					}
				}
			}
		}
	}
	else if(eFocusType == CITY_AI_FOCUS_TYPE_GOLD_GROWTH)
	{
		// Loop through all Buildings
		for(int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
		{
			const BuildingTypes eBuilding = static_cast<BuildingTypes>(iBuildingLoop);
			CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
			if(pkBuildingInfo)
			{
				// Have this Building in the City?
				if(m_pCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
				{
					// Can't add more than the max
					if(IsCanAddSpecialistToBuilding(eBuilding))
					{
						SpecialistTypes eSpecialist = (SpecialistTypes) pkBuildingInfo->GetSpecialistType();
						CvSpecialistInfo* pSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
						if(pSpecialistInfo && pSpecialistInfo->getYieldChange(YIELD_GOLD) > 0)
						{
							iWeight *= 150;
							iWeight /= 100;
						}

						if(GetPlayer()->getSpecialistExtraYield(YIELD_GOLD) > 0)
						{
							iWeight *= 2;
						}

						if(GetPlayer()->GetPlayerTraits()->GetSpecialistYieldChange(eSpecialist, YIELD_GOLD) > 0)
						{
							iWeight *= 2;
						}
					}
				}
			}
		}
	}
	else if(eFocusType == CITY_AI_FOCUS_TYPE_FAITH)
	{
		// Loop through all Buildings
		for(int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
		{
			const BuildingTypes eBuilding = (BuildingTypes) iBuildingLoop;
			CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
			if(pkBuildingInfo)
			{
				// Have this Building in the City?
				if(m_pCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
				{
					// Can't add more than the max
					if(IsCanAddSpecialistToBuilding(eBuilding))
					{
						const SpecialistTypes eSpecialist = (SpecialistTypes) pkBuildingInfo->GetSpecialistType();
						CvSpecialistInfo* pSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
						if(pSpecialistInfo && pSpecialistInfo->getYieldChange(YIELD_FAITH) > 0)
						{
							iWeight *= 3;
							break;
						}
					}
				}
			}
		}
	}

	// specialists are cheaper somehow
	if (m_pCity->GetPlayer()->isHalfSpecialistUnhappiness() || m_pCity->GetPlayer()->isHalfSpecialistFood())
	{
		iWeight *= 150;
		iWeight /= 100;
	}

	// Does the AI want it enough?
	if(iWeight >= 150)
	{
		return true;
	}

	return false;
}
#endif

/// What is the Building Type the AI likes the Specialist of most right now?
#if defined(AUI_CITIZENS_IGNORE_FOOD_FOR_CITIZEN_ASSIGN_AFTER_GROW) || defined(AUI_CITIZENS_GET_VALUE_FROM_STATS)
BuildingTypes CvCityCitizens::GetAIBestSpecialistBuilding(int& iSpecialistValue, bool bAfterGrowth)
#else
BuildingTypes CvCityCitizens::GetAIBestSpecialistBuilding(int& iSpecialistValue)
#endif
{
	BuildingTypes eBestBuilding = NO_BUILDING;
	int iBestSpecialistValue = -1;
	int iBestUnmodifiedSpecialistValue = -1;

	SpecialistTypes eSpecialist;
	int iValue;

	// Loop through all Buildings
	for(int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
	{
		const BuildingTypes eBuilding = static_cast<BuildingTypes>(iBuildingLoop);
		CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);

		if(pkBuildingInfo)
		{
			// Have this Building in the City?
			if(GetCity()->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
			{
				// Can't add more than the max
				if(IsCanAddSpecialistToBuilding(eBuilding))
				{
					eSpecialist = (SpecialistTypes) pkBuildingInfo->GetSpecialistType();

#if defined(AUI_CITIZENS_IGNORE_FOOD_FOR_CITIZEN_ASSIGN_AFTER_GROW) || defined(AUI_CITIZENS_GET_VALUE_FROM_STATS)
					iValue = GetSpecialistValue(eSpecialist, bAfterGrowth);
#else
					iValue = GetSpecialistValue(eSpecialist);
#endif

					// Add a bit more weight to a Building if it has more slots (10% per).  This will bias the AI to fill a single building over spreading Specialists out
					int iTemp = ((GetNumSpecialistsAllowedByBuilding(*pkBuildingInfo) - 1) * iValue * 10);
					iTemp /= 100;
					iValue += iTemp;

					if(iValue > iBestSpecialistValue)
					{
						eBestBuilding = eBuilding;
						iBestSpecialistValue = iValue;
						iBestUnmodifiedSpecialistValue = iValue - iTemp;
					}
				}
			}
		}
	}

	iSpecialistValue = iBestUnmodifiedSpecialistValue;
	return eBestBuilding;
}

/// How valuable is eSpecialist?
#if defined(AUI_CITIZENS_IGNORE_FOOD_FOR_CITIZEN_ASSIGN_AFTER_GROW) || defined(AUI_CITIZENS_GET_VALUE_FROM_STATS)
int CvCityCitizens::GetSpecialistValue(SpecialistTypes eSpecialist, bool bAfterGrowth) const
#else
int CvCityCitizens::GetSpecialistValue(SpecialistTypes eSpecialist)
#endif
{

	CvSpecialistInfo* pSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
	if (pSpecialistInfo == NULL)
	{
		//This function should NEVER be called with an invalid specialist info type.
		CvAssert(pSpecialistInfo);
		return 0;
	}

#ifndef AUI_CITIZENS_GET_VALUE_FROM_STATS
	int iValue = 0;
#endif

	CvPlayer* pPlayer = m_pCity->GetPlayer();
#ifdef AUI_CITIZENS_GET_VALUE_FROM_STATS
	double dYieldArray[NUM_YIELD_TYPES] = {};

	dYieldArray[YIELD_FOOD] += pPlayer->specialistYield(eSpecialist, YIELD_FOOD);
	if (pPlayer->isHalfSpecialistFood())
		dYieldArray[YIELD_FOOD] += GC.getFOOD_CONSUMPTION_PER_POPULATION() / 2.0;
	dYieldArray[YIELD_PRODUCTION] += pPlayer->specialistYield(eSpecialist, YIELD_PRODUCTION);
	dYieldArray[YIELD_GOLD] += pPlayer->specialistYield(eSpecialist, YIELD_GOLD);
	dYieldArray[YIELD_SCIENCE] += pPlayer->specialistYield(eSpecialist, YIELD_SCIENCE);
	dYieldArray[YIELD_CULTURE] += pPlayer->specialistYield(eSpecialist, YIELD_CULTURE);
	dYieldArray[YIELD_FAITH] += pPlayer->specialistYield(eSpecialist, YIELD_FAITH);

	int iHappiness = 0;
	if (pPlayer->isHalfSpecialistUnhappiness() && !m_pCity->IsIgnoreCityForHappiness())
	{
		iHappiness = GC.getUNHAPPINESS_PER_POPULATION() * 50;
		if (m_pCity->IsOccupied() && !m_pCity->IsNoOccupiedUnhappiness())
			iHappiness = int(GC.getUNHAPPINESS_PER_OCCUPIED_POPULATION() * 50);
		if (pPlayer->GetCapitalUnhappinessMod() != 0 && m_pCity->isCapital())
		{
			iHappiness *= 100;
			iHappiness /= (100 + pPlayer->GetCapitalUnhappinessMod());
		}
		iHappiness *= 100;
		iHappiness /= (100 + pPlayer->GetUnhappinessMod());
		iHappiness *= 100;
		iHappiness /= (100 + pPlayer->GetPlayerTraits()->GetPopulationUnhappinessModifier());
		// Handicap mod
		iHappiness *= 100;
		iHappiness /= pPlayer->getHandicapInfo().getPopulationUnhappinessMod();

		// For the initial *50
		iHappiness /= 100;
	}

	if (GetTotalSpecialistCount() > 0)
	{
		const CvReligion* pReligion = GC.getGame().GetGameReligions()->GetReligion(m_pCity->GetCityReligions()->GetReligiousMajority(), pPlayer->GetID());
		dYieldArray[YIELD_FOOD] += pReligion->m_Beliefs.GetYieldChangeAnySpecialist(YIELD_FOOD);
		dYieldArray[YIELD_PRODUCTION] += pReligion->m_Beliefs.GetYieldChangeAnySpecialist(YIELD_PRODUCTION);
		dYieldArray[YIELD_GOLD] += pReligion->m_Beliefs.GetYieldChangeAnySpecialist(YIELD_GOLD);
		dYieldArray[YIELD_SCIENCE] += pReligion->m_Beliefs.GetYieldChangeAnySpecialist(YIELD_SCIENCE);
		dYieldArray[YIELD_CULTURE] += pReligion->m_Beliefs.GetYieldChangeAnySpecialist(YIELD_CULTURE);
		dYieldArray[YIELD_FAITH] += pReligion->m_Beliefs.GetYieldChangeAnySpecialist(YIELD_FAITH);

		BeliefTypes eSecondaryPantheon = m_pCity->GetCityReligions()->GetSecondaryReligionPantheonBelief();
		if (eSecondaryPantheon != NO_BELIEF)
		{
			CvBeliefEntry* pBeliefEntry = GC.GetGameBeliefs()->GetEntry(eSecondaryPantheon);
			if (pBeliefEntry)
			{
				dYieldArray[YIELD_FOOD] += pBeliefEntry->GetYieldChangeAnySpecialist(YIELD_FOOD);
				dYieldArray[YIELD_PRODUCTION] += pBeliefEntry->GetYieldChangeAnySpecialist(YIELD_PRODUCTION);
				dYieldArray[YIELD_GOLD] += pBeliefEntry->GetYieldChangeAnySpecialist(YIELD_GOLD);
				dYieldArray[YIELD_SCIENCE] += pBeliefEntry->GetYieldChangeAnySpecialist(YIELD_SCIENCE);
				dYieldArray[YIELD_CULTURE] += pBeliefEntry->GetYieldChangeAnySpecialist(YIELD_CULTURE);
				dYieldArray[YIELD_FAITH] += pBeliefEntry->GetYieldChangeAnySpecialist(YIELD_FAITH);
			}
		}
	}

	dYieldArray[YIELD_FOOD] *= m_pCity->getBaseYieldRateModifier(YIELD_FOOD, iHappiness) / 100.0;
	dYieldArray[YIELD_PRODUCTION] *= m_pCity->getBaseYieldRateModifier(YIELD_PRODUCTION, iHappiness) / 100.0;
	dYieldArray[YIELD_GOLD] *= m_pCity->getBaseYieldRateModifier(YIELD_GOLD, iHappiness) / 100.0;
	dYieldArray[YIELD_SCIENCE] *= m_pCity->getBaseYieldRateModifier(YIELD_SCIENCE, iHappiness) / 100.0;
	dYieldArray[YIELD_CULTURE] *= m_pCity->getBaseYieldRateModifier(YIELD_CULTURE, iHappiness) / 100.0;
	dYieldArray[YIELD_FAITH] *= m_pCity->getBaseYieldRateModifier(YIELD_FAITH, iHappiness) / 100.0;

	dYieldArray[YIELD_FOOD] += (m_pCity->getYieldRateTimes100(YIELD_FOOD, false, iHappiness) - m_pCity->getYieldRateTimes100(YIELD_FOOD, false)) / 100.0;
	dYieldArray[YIELD_PRODUCTION] += (m_pCity->getYieldRateTimes100(YIELD_PRODUCTION, false, iHappiness) - m_pCity->getYieldRateTimes100(YIELD_PRODUCTION, false)) / 100.0;
	dYieldArray[YIELD_GOLD] += (m_pCity->getYieldRateTimes100(YIELD_GOLD, false, iHappiness) - m_pCity->getYieldRateTimes100(YIELD_GOLD, false)) / 100.0;
	dYieldArray[YIELD_SCIENCE] += (m_pCity->getYieldRateTimes100(YIELD_SCIENCE, false, iHappiness) - m_pCity->getYieldRateTimes100(YIELD_SCIENCE, false)) / 100.0;
	dYieldArray[YIELD_CULTURE] += (m_pCity->getYieldRateTimes100(YIELD_CULTURE, false, iHappiness) - m_pCity->getYieldRateTimes100(YIELD_CULTURE, false)) / 100.0;
	dYieldArray[YIELD_FAITH] += (m_pCity->getYieldRateTimes100(YIELD_FAITH, false, iHappiness) - m_pCity->getYieldRateTimes100(YIELD_FAITH, false)) / 100.0;

	dYieldArray[YIELD_SCIENCE] += (pPlayer->GetScienceFromHappinessTimes100(iHappiness) - pPlayer->GetScienceFromHappinessTimes100()) / 100.0;
	dYieldArray[YIELD_CULTURE] += (pPlayer->GetJONSCulturePerTurnFromExcessHappiness(iHappiness) - pPlayer->GetJONSCulturePerTurnFromExcessHappiness()) / 100.0;

	UnitClassTypes eGPUnitClass = (UnitClassTypes)pSpecialistInfo->getGreatPeopleUnitClass();
	double dGPPChange = pSpecialistInfo->getGreatPeopleRateChange();
	if (dGPPChange > 0)
	{
		// City mod
		int iMod = GetCity()->getGreatPeopleRateModifier();

		// Player mod
		iMod += GetPlayer()->getGreatPeopleRateModifier();

		// Player and Golden Age mods to this specific class
		if (eGPUnitClass == GC.getInfoTypeForString("UNITCLASS_SCIENTIST"))
		{
			iMod += GetPlayer()->getGreatScientistRateModifier();
		}
		else if (eGPUnitClass == GC.getInfoTypeForString("UNITCLASS_WRITER"))
		{
			if (GetPlayer()->isGoldenAge())
			{
				iMod += GetPlayer()->GetPlayerTraits()->GetGoldenAgeGreatWriterRateModifier();
			}
			iMod += GetPlayer()->getGreatWriterRateModifier();
		}
		else if (eGPUnitClass == GC.getInfoTypeForString("UNITCLASS_ARTIST"))
		{
			if (GetPlayer()->isGoldenAge())
			{
				iMod += GetPlayer()->GetPlayerTraits()->GetGoldenAgeGreatArtistRateModifier();
			}
			iMod += GetPlayer()->getGreatArtistRateModifier();
		}
		else if (eGPUnitClass == GC.getInfoTypeForString("UNITCLASS_MUSICIAN"))
		{
			if (GetPlayer()->isGoldenAge())
			{
				iMod += GetPlayer()->GetPlayerTraits()->GetGoldenAgeGreatMusicianRateModifier();
			}
			iMod += GetPlayer()->getGreatMusicianRateModifier();
		}
		else if (eGPUnitClass == GC.getInfoTypeForString("UNITCLASS_MERCHANT"))
		{
			iMod += GetPlayer()->getGreatMerchantRateModifier();
		}
		else if (eGPUnitClass == GC.getInfoTypeForString("UNITCLASS_ENGINEER"))
		{
			iMod += GetPlayer()->getGreatEngineerRateModifier();
		}

		// Apply mod
		dGPPChange *= (100.0 + iMod) / 100.0;
	}
	
	return GetTotalValue(dYieldArray, eGPUnitClass, dGPPChange, iHappiness, 0.0, 0, bAfterGrowth);
#else

	// factor in the fact that specialists may need less food
#ifdef AUI_CITIZENS_FIX_SPECIALIST_VALUE_HALF_FOOD_CONSUMPTION
	int iFoodConsumptionBonus = (pPlayer->isHalfSpecialistFood()) ? GC.getFOOD_CONSUMPTION_PER_POPULATION() / 2 : 0;
#else
	int iFoodConsumptionBonus = (pPlayer->isHalfSpecialistFood()) ? 1 : 0;
#endif

	// Yield Values
	int iFoodYieldValue = (GC.getAI_CITIZEN_VALUE_FOOD() * (pPlayer->specialistYield(eSpecialist, YIELD_FOOD) + iFoodConsumptionBonus));
#ifdef AUI_CITIZENS_IGNORE_FOOD_FOR_CITIZEN_ASSIGN_AFTER_GROW
	if (bAfterGrowth)
		iFoodYieldValue = 0;
#endif
	int iProductionYieldValue = (GC.getAI_CITIZEN_VALUE_PRODUCTION() * pPlayer->specialistYield(eSpecialist, YIELD_PRODUCTION));
	int iGoldYieldValue = (GC.getAI_CITIZEN_VALUE_GOLD() * pPlayer->specialistYield(eSpecialist, YIELD_GOLD));
	int iScienceYieldValue = (GC.getAI_CITIZEN_VALUE_SCIENCE() * pPlayer->specialistYield(eSpecialist, YIELD_SCIENCE));
	int iCultureYieldValue = (GC.getAI_CITIZEN_VALUE_CULTURE() * m_pCity->GetCultureFromSpecialist(eSpecialist)); 
	int iFaithYieldValue = (GC.getAI_CITIZEN_VALUE_FAITH() * pPlayer->specialistYield(eSpecialist, YIELD_FAITH));
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_YIELD_RATE_MODIFIERS
	iFoodYieldValue *= m_pCity->getBaseYieldRateModifier(YIELD_FOOD);
	iFoodYieldValue /= 100;
	iProductionYieldValue *= m_pCity->getBaseYieldRateModifier(YIELD_PRODUCTION);
	iProductionYieldValue /= 100;
	iGoldYieldValue *= m_pCity->getBaseYieldRateModifier(YIELD_GOLD);
	iGoldYieldValue /= 100;
	iScienceYieldValue *= m_pCity->getBaseYieldRateModifier(YIELD_SCIENCE);
	iScienceYieldValue /= 100;
	iCultureYieldValue *= m_pCity->getBaseYieldRateModifier(YIELD_CULTURE);
	iCultureYieldValue /= 100;
	iFaithYieldValue *= m_pCity->getBaseYieldRateModifier(YIELD_FAITH);
	iFaithYieldValue /= 100;
#endif
#ifdef AUI_CITIZENS_GET_VALUE_HIGHER_FAITH_VALUE_IF_BEFORE_RELIGION
	int iReligionsToFound = GC.getGame().GetGameReligions()->GetNumReligionsStillToFound();
	if (iReligionsToFound > 0)
	{
		int iMaxReligions = GC.getMap().getWorldInfo().getMaxActiveReligions() + 1;
		iFaithYieldValue += iFaithYieldValue * iReligionsToFound * iReligionsToFound / (iMaxReligions * iMaxReligions);
	}
#endif
#ifndef AUI_CITIZENS_UNHARDCODE_SPECIALIST_VALUE_GREAT_PERSON_POINTS
	int iGPPYieldValue = pSpecialistInfo->getGreatPeopleRateChange() * 3; // TODO: un-hardcode this
#endif
#ifdef AUI_CITIZENS_UNHARDCODE_SPECIALIST_VALUE_HAPPINESS
	int iHappinessYieldValue = 0;
	if (pPlayer->isHalfSpecialistUnhappiness())
	{
		iHappinessYieldValue = GC.getUNHAPPINESS_PER_POPULATION() * 50 * AUI_CITIZENS_UNHARDCODE_SPECIALIST_VALUE_HAPPINESS;
		if (pPlayer->GetCapitalUnhappinessMod() != 0 && m_pCity->isCapital())
		{
			iHappinessYieldValue *= 100;
			iHappinessYieldValue /= (100 + pPlayer->GetCapitalUnhappinessMod());
		}
		iHappinessYieldValue *= 100;
		iHappinessYieldValue /= (100 + pPlayer->GetUnhappinessMod());
		iHappinessYieldValue *= 100;
		iHappinessYieldValue /= (100 + pPlayer->GetPlayerTraits()->GetPopulationUnhappinessModifier());
		// Handicap mod
		iHappinessYieldValue *= 100;
		iHappinessYieldValue /= pPlayer->getHandicapInfo().getPopulationUnhappinessMod();

		if (pPlayer->GetExcessHappiness() <= 0)
		{
			iHappinessYieldValue = int(iHappinessYieldValue * pow(2.0, 1.0 - (double)pPlayer->GetExcessHappiness() / 10.0) + 0.5);
		}

		// For the initial *50
		iHappinessYieldValue /= 100;
	}
#else
	int iHappinessYieldValue = (m_pCity->GetPlayer()->isHalfSpecialistUnhappiness()) ? 5 : 0; // TODO: un-hardcode this
	iHappinessYieldValue = m_pCity->GetPlayer()->IsEmpireUnhappy() ? iHappinessYieldValue * 2 : iHappinessYieldValue; // TODO: un-hardcode this
#endif

	// How much surplus food are we making?
	int iExcessFoodTimes100 = m_pCity->getYieldRateTimes100(YIELD_FOOD, false) - (m_pCity->foodConsumption() * 100);
#if defined(AUI_CITIZENS_GET_VALUE_SPLIT_EXCESS_FOOD_MUTLIPLIER) || defined(AUI_CITIZENS_GET_VALUE_ALTER_FOOD_VALUE_IF_FOOD_PRODUCTION) || defined(AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS)
	int iExcessFoodWithPlotTimes100 = m_pCity->getYieldRateTimes100(YIELD_FOOD, false) - (m_pCity->foodConsumption() * 100) + (GC.getAI_CITIZEN_VALUE_FOOD() * (pPlayer->specialistYield(eSpecialist, YIELD_FOOD) + iFoodConsumptionBonus)) * m_pCity->getBaseYieldRateModifier(YIELD_FOOD);
#endif

#ifdef AUI_CITIZENS_UNHARDCODE_SPECIALIST_VALUE_GREAT_PERSON_POINTS
	int iGPPYieldValue = pSpecialistInfo->getGreatPeopleRateChange() * AUI_CITIZENS_UNHARDCODE_SPECIALIST_VALUE_GREAT_PERSON_POINTS;
	UnitClassTypes eGPUnitClass = (UnitClassTypes)pSpecialistInfo->getGreatPeopleUnitClass();
	if (eGPUnitClass != NO_UNITCLASS)
	{
		// Multiplier based on flavor
		double dFlavorMod = 1.0;
		// Doubles GPP value if the great person we'd generate is unique to our civ
		CvUnitClassInfo* pGPUnitClassInfo = GC.getUnitClassInfo(eGPUnitClass);
		if (pGPUnitClassInfo)
		{
			UnitTypes eGPUnitType = (UnitTypes)pPlayer->getCivilizationInfo().getCivilizationUnits(eGPUnitClass);
			UnitTypes eDefault = (UnitTypes)pGPUnitClassInfo->getDefaultUnitIndex();
			if (eGPUnitType != eDefault)
			{
				iGPPYieldValue *= 2;
				// Venice compensation (since their unique merchant is only special in that it can acquire minors, so the AI no longer wants minors, boost is deactivated)
				if (eGPUnitClass == (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_MERCHANT", true) && GET_PLAYER(m_pCity->getOwner()).GreatMerchantWantsCash())
				{
					iGPPYieldValue /= 2;
				}
			}
			CvUnitEntry *pkGPEntry = GC.GetGameUnits()->GetEntry(eGPUnitType);
			if (pkGPEntry)
			{
				int iBestFlavor = 0;
				for (int iFlavorLoop = 0; iFlavorLoop < GC.getNumFlavorTypes(); iFlavorLoop++)
				{
					if (pkGPEntry->GetFlavorValue(iFlavorLoop) > 0)
					{
						iBestFlavor = MAX(iBestFlavor, pPlayer->GetGrandStrategyAI()->GetPersonalityAndGrandStrategy((FlavorTypes)iFlavorLoop));
					}
				}
				if (iBestFlavor > 0)
				{
					dFlavorMod += log10((double)iBestFlavor);
				}
				else
				{
					dFlavorMod += log10((double)GC.getDEFAULT_FLAVOR_VALUE());
				}
			}
		}
		// Tweaking based on actual GP bonues and penalties
		int iGPPModifier = pPlayer->getGreatPeopleRateModifier() + GetCity()->getGreatPeopleRateModifier();
		// Multiplier based on how many of this unit do we already have
		double dAlreadyHaveCountMod = 3.0;
		// Tweaking based on grand strategy (ranges from 1 to 2)
		double dGrandStrategyGPPMod = 1.0;
		if (eGPUnitClass == (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_WRITER", true))
		{
			iGPPModifier += pPlayer->getGreatWriterRateModifier();
			dAlreadyHaveCountMod /= (dAlreadyHaveCountMod + pPlayer->GetNumUnitsWithUnitAI(UNITAI_WRITER));
#ifdef AUI_GS_PRIORITY_RATIO
			dGrandStrategyGPPMod += pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"));
#else
			if (pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
			{
				fGrandStrategyGPPMod += 1.0f;
			}
#endif
		}
		else if (eGPUnitClass == (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_ARTIST", true))
		{
			iGPPModifier += pPlayer->getGreatArtistRateModifier();
			dAlreadyHaveCountMod /= (dAlreadyHaveCountMod + pPlayer->GetNumUnitsWithUnitAI(UNITAI_ARTIST));
#ifdef AUI_GS_PRIORITY_RATIO
			dGrandStrategyGPPMod += pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"));
#else
			if (pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
			{
				fGrandStrategyGPPMod += 1.0f;
			}
#endif
		}
		else if (eGPUnitClass == (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_MUSICIAN", true))
		{
			iGPPModifier += pPlayer->getGreatMusicianRateModifier();
			dAlreadyHaveCountMod /= (dAlreadyHaveCountMod + pPlayer->GetNumUnitsWithUnitAI(UNITAI_MUSICIAN));
#ifdef AUI_GS_PRIORITY_RATIO
			dGrandStrategyGPPMod += pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"));
#else
			if (pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
			{
				fGrandStrategyGPPMod += 1.0f;
			}
#endif
		}
		else if (eGPUnitClass == (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_SCIENTIST", true))
		{
			iGPPModifier += pPlayer->getGreatScientistRateModifier();
			dAlreadyHaveCountMod /= (dAlreadyHaveCountMod + pPlayer->GetNumUnitsWithUnitAI(UNITAI_SCIENTIST));
#ifdef AUI_GS_PRIORITY_RATIO
			dGrandStrategyGPPMod += pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP"));
#else
			if (pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP"))
			{
				fGrandStrategyGPPMod += 1.0f;
			}
#endif
#ifdef AUI_GS_SCIENCE_FLAVOR_BOOST
			if (pPlayer->GetGrandStrategyAI()->ScienceFlavorBoost() >= AUI_GS_SCIENCE_FLAVOR_BOOST)
				dGrandStrategyGPPMod *= 2;
#endif
		}
		else if (eGPUnitClass == (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_MERCHANT", true))
		{
			iGPPModifier += pPlayer->getGreatMerchantRateModifier();
			dAlreadyHaveCountMod /= (dAlreadyHaveCountMod + pPlayer->GetNumUnitsWithUnitAI(UNITAI_MERCHANT));
#ifdef AUI_GS_PRIORITY_RATIO
			dGrandStrategyGPPMod += pow((1.0 + pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE")))
				* (1.0 + pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS")))
				* (1.0 + pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP")))
				* (1.0 + pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"))) / 2.0, 1.0/3.0);
#else
			if (pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS"))
			{
				fGrandStrategyGPPMod += 1.0f;
			}
#endif
			if (pPlayer->GetTreasury()->AverageIncome(1) < 0)
				dGrandStrategyGPPMod *= 2;
		}
		else if (eGPUnitClass == (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_ENGINEER", true))
		{
			iGPPModifier += pPlayer->getGreatEngineerRateModifier();
			dAlreadyHaveCountMod /= (dAlreadyHaveCountMod + pPlayer->GetNumUnitsWithUnitAI(UNITAI_ENGINEER));
#ifdef AUI_GS_PRIORITY_RATIO
			dGrandStrategyGPPMod += pow((1.0 + pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE")))
				* (1.0 + pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS")))
				* (1.0 + pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP")))
				* (1.0 + pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"))) / 2.0, 1.0 / 3.0);
#else
			if (pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP"))
			{
				fGrandStrategyGPPMod += 1.0f;
			}
#endif
		}
		else if (eGPUnitClass == (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_GREAT_GENERAL", true))
		{
			iGPPModifier += pPlayer->getGreatAdmiralRateModifier();
			dAlreadyHaveCountMod /= (dAlreadyHaveCountMod + pPlayer->GetNumUnitsWithUnitAI(UNITAI_GENERAL));
#ifdef AUI_GS_PRIORITY_RATIO
			dGrandStrategyGPPMod += pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"));
#else
			if (pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"))
			{
				fGrandStrategyGPPMod += 1.0f;
			}
#endif
		}
		else if (eGPUnitClass == (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_GREAT_ADMIRAL", true))
		{
			iGPPModifier += pPlayer->getGreatGeneralRateModifier();
			dAlreadyHaveCountMod /= (dAlreadyHaveCountMod + pPlayer->GetNumUnitsWithUnitAI(UNITAI_ADMIRAL));
#ifdef AUI_GS_PRIORITY_RATIO
			dGrandStrategyGPPMod += pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"));
#else
			if (pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy() == (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"))
			{
				fGrandStrategyGPPMod += 1.0f;
			}
#endif
		}

		iGPPYieldValue = int(iGPPYieldValue * dGrandStrategyGPPMod * dAlreadyHaveCountMod * dFlavorMod * log(MAX(100.0 + iGPPModifier, 1.0)) / log(100.0) + 0.5);
	}
#endif

	bool bAvoidGrowth = IsAvoidGrowth();

	// City Focus
	CityAIFocusTypes eFocus = GetFocusType();
	if(eFocus == CITY_AI_FOCUS_TYPE_FOOD)
		iFoodYieldValue *= 3;
	else if(eFocus == CITY_AI_FOCUS_TYPE_PRODUCTION)
		iProductionYieldValue *= 3;
	else if(eFocus == CITY_AI_FOCUS_TYPE_GOLD)
		iGoldYieldValue *= 3;
	else if(eFocus == CITY_AI_FOCUS_TYPE_SCIENCE)
		iScienceYieldValue *= 3;
	else if(eFocus == CITY_AI_FOCUS_TYPE_CULTURE)
		iCultureYieldValue *= 3;
	else if(eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH)
	{
		iFoodYieldValue *= 2;
		iGoldYieldValue *= 2;
	}
	else if(eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
	{
		iFoodYieldValue *= 2;
		iProductionYieldValue *= 2;
	}
	else if(eFocus == CITY_AI_FOCUS_TYPE_FAITH)
	{
		iFaithYieldValue *= 3;
	}
	else if(eFocus == CITY_AI_FOCUS_TYPE_GREAT_PEOPLE)
	{
		iGPPYieldValue *= 3;
	}

#ifdef AUI_CITIZENS_GET_VALUE_ALTER_FOOD_VALUE_IF_FOOD_PRODUCTION
	if (m_pCity->isFoodProduction())
	{
		iFoodYieldValue = 0;
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
		int iCurrProdFromFood = m_pCity->foodDifference(true, true, m_pCity->GetFoodProduction(iExcessFoodTimes100 / 100));
		int iProdFromFoodWithTile = m_pCity->foodDifference(true, true, m_pCity->GetFoodProduction(iExcessFoodWithPlotTimes100 / 100));
#else
		int iCurrProdFromFood = m_pCity->GetFoodProduction(iExcessFoodTimes100 / 100);
		int iProdFromFoodWithTile = m_pCity->GetFoodProduction(iExcessFoodWithPlotTimes100 / 100);
#endif
		iProductionYieldValue += (iProdFromFoodWithTile - iCurrProdFromFood) * GC.getAI_CITIZEN_VALUE_PRODUCTION() * (eFocus == CITY_AI_FOCUS_TYPE_PRODUCTION ? 3 : 0);
	}
	else
#endif
	// Food can be worth less if we don't want to grow
	if(iExcessFoodTimes100 >= 0 && bAvoidGrowth)
	{
		// If we at least have enough Food to feed everyone, zero out the value of additional food
		iFoodYieldValue = 0;
	}
	// We want to grow here
	else
	{
		// If we have a non-default and non-food focus, only worry about getting to 0 food
		if(eFocus != NO_CITY_AI_FOCUS_TYPE && eFocus != CITY_AI_FOCUS_TYPE_FOOD && eFocus != CITY_AI_FOCUS_TYPE_PROD_GROWTH && eFocus != CITY_AI_FOCUS_TYPE_GOLD_GROWTH)
		{
			int iFoodT100NeededFor0 = -iExcessFoodTimes100;

			if(iFoodT100NeededFor0 > 0)
			{
				iFoodYieldValue *= 8;
#ifdef AUI_CITIZENS_GET_VALUE_SPLIT_EXCESS_FOOD_MUTLIPLIER
				if (iExcessFoodWithPlotTimes100 > 0 && iFoodYieldValue != 0)
				{
					int iFutureExcessFoodYieldValue = /*12*/ GC.getAI_CITIZEN_VALUE_FOOD() * iExcessFoodWithPlotTimes100;
					if (eFocus == CITY_AI_FOCUS_TYPE_FOOD)
						iFutureExcessFoodYieldValue *= 3;
					else if (eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH || eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
						iFutureExcessFoodYieldValue *= 2;
					iFoodYieldValue -= iFutureExcessFoodYieldValue * 8 / 100;
					if (!bAvoidGrowth)
					{
						iFoodYieldValue += iFutureExcessFoodYieldValue / 200;
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
						iFutureExcessFoodYieldValue = (m_pCity->foodDifferenceTimes100(true, NULL, true, iExcessFoodWithPlotTimes100) - iExcessFoodWithPlotTimes100) * /*12*/ GC.getAI_CITIZEN_VALUE_FOOD();
						if (eFocus == CITY_AI_FOCUS_TYPE_FOOD)
							iFutureExcessFoodYieldValue *= 3;
						else if (eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH || eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
							iFutureExcessFoodYieldValue *= 2;
						iFoodYieldValue += iFutureExcessFoodYieldValue / 200;
#endif
					}
				}
#endif
			}
			else
			{
				iFoodYieldValue /= 2;
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
				int iExtraFoodValueT100 = (m_pCity->foodDifferenceTimes100(true, NULL, true, iExcessFoodWithPlotTimes100 - iExcessFoodTimes100) -
					(iExcessFoodWithPlotTimes100 - iExcessFoodTimes100)) * /*12*/ GC.getAI_CITIZEN_VALUE_FOOD();
				if (eFocus == CITY_AI_FOCUS_TYPE_FOOD)
					iExtraFoodValueT100 *= 3;
				else if (eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH || eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
					iExtraFoodValueT100 *= 2;
				iFoodYieldValue += iExtraFoodValueT100 / 200;
#endif
			}
		}
		// If our surplus is not at least 2, really emphasize food plots
		else if(!bAvoidGrowth)
		{
#ifdef AUI_CITIZENS_FIX_SPECIALIST_VALUE_HALF_FOOD_CONSUMPTION
			int iFoodT100NeededFor2 = 100 * GC.getFOOD_CONSUMPTION_PER_POPULATION() - iExcessFoodTimes100;
#else
			int iFoodT100NeededFor2 = 200 - iExcessFoodTimes100;
#endif
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
			int iExtraFoodValueT100 = (m_pCity->foodDifferenceTimes100(true, NULL, true, iExcessFoodWithPlotTimes100 - iExcessFoodTimes100) -
				(iExcessFoodWithPlotTimes100 - iExcessFoodTimes100)) * /*12*/ GC.getAI_CITIZEN_VALUE_FOOD();
			if (eFocus == CITY_AI_FOCUS_TYPE_FOOD)
				iExtraFoodValueT100 *= 3;
			else if (eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH || eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
				iExtraFoodValueT100 *= 2;
#endif

			if(iFoodT100NeededFor2 > 0)
			{
				iFoodYieldValue *= 8;
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
				iExtraFoodValueT100 = 0;
#endif
#ifdef AUI_CITIZENS_GET_VALUE_SPLIT_EXCESS_FOOD_MUTLIPLIER
				if (iExcessFoodWithPlotTimes100 > 100 * GC.getFOOD_CONSUMPTION_PER_POPULATION() && iFoodYieldValue != 0)
				{
					int iFutureExcessFoodYieldValue = /*12*/ GC.getAI_CITIZEN_VALUE_FOOD() * (iExcessFoodWithPlotTimes100 - 100 * GC.getFOOD_CONSUMPTION_PER_POPULATION());
					if (eFocus == CITY_AI_FOCUS_TYPE_FOOD)
						iFutureExcessFoodYieldValue *= 3;
					else if (eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH || eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
						iFutureExcessFoodYieldValue *= 2;
					iFoodYieldValue -= iFutureExcessFoodYieldValue * 8 / 100;
					if (eFocus != CITY_AI_FOCUS_TYPE_FOOD)
						iFoodYieldValue += iFutureExcessFoodYieldValue / 200;
					else
						iFoodYieldValue += iFutureExcessFoodYieldValue / 100;
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
					iExtraFoodValueT100 = (m_pCity->foodDifferenceTimes100(true, NULL, true, iExcessFoodWithPlotTimes100 - 100 * GC.getFOOD_CONSUMPTION_PER_POPULATION()) -
						(iExcessFoodWithPlotTimes100 - 100 * GC.getFOOD_CONSUMPTION_PER_POPULATION())) * /*12*/ GC.getAI_CITIZEN_VALUE_FOOD();
					if (eFocus != CITY_AI_FOCUS_TYPE_FOOD)
						iExtraFoodValueT100 /= 2;
#endif
				}
#endif
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
#ifdef AUI_FAST_COMP
				iExtraFoodValueT100 += (m_pCity->foodDifferenceTimes100(true, NULL, true, FASTMIN(iExcessFoodWithPlotTimes100, 100 * GC.getFOOD_CONSUMPTION_PER_POPULATION())) -
					FASTMIN(iExcessFoodWithPlotTimes100, 100 * GC.getFOOD_CONSUMPTION_PER_POPULATION())) * 8 * /*12*/ GC.getAI_CITIZEN_VALUE_FOOD();
#else
				iExtraFoodValueT100 += (m_pCity->foodDifferenceTimes100(true, NULL, true, MIN(iExcessFoodWithPlotTimes100, 100 * GC.getFOOD_CONSUMPTION_PER_POPULATION())) -
					MIN(iExcessFoodWithPlotTimes100, 100 * GC.getFOOD_CONSUMPTION_PER_POPULATION())) * 8 * /*12*/ GC.getAI_CITIZEN_VALUE_FOOD();
#endif
				if (eFocus == CITY_AI_FOCUS_TYPE_FOOD)
					iExtraFoodValueT100 *= 3;
				else if (eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH || eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
					iExtraFoodValueT100 *= 2;
				iFoodYieldValue += iExtraFoodValueT100 / 100;
#endif
			}
			else if (eFocus != CITY_AI_FOCUS_TYPE_FOOD)
			{
				iFoodYieldValue /= 2;
#ifdef AUI_CITIZENS_GET_VALUE_CONSIDER_GROWTH_MODIFIERS
				iFoodYieldValue += iExtraFoodValueT100 / 200;
			}
			else
			{
				iFoodYieldValue += iExtraFoodValueT100 / 100;
#endif
			}
		}
#ifdef AUI_CITIZENS_FIX_GET_VALUE_FOOD_YIELD_VALUE_WHEN_STARVATION_WITH_AVOID_GROWTH
		// Food focus and negative food, but with avoid growth enabled for some reason
		else
		{
			iFoodYieldValue *= 8;
#ifdef AUI_CITIZENS_GET_VALUE_SPLIT_EXCESS_FOOD_MUTLIPLIER
			if (iExcessFoodWithPlotTimes100 > 0 && iFoodYieldValue != 0)
			{
				int iFutureExcessFoodYieldValue = /*12*/ GC.getAI_CITIZEN_VALUE_FOOD() * iExcessFoodWithPlotTimes100;
				if (eFocus == CITY_AI_FOCUS_TYPE_FOOD)
					iFutureExcessFoodYieldValue *= 3;
				else if (eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH || eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH)
					iFutureExcessFoodYieldValue *= 2;
				iFoodYieldValue -= iFutureExcessFoodYieldValue * 8 / 100;
			}
#endif
		}
#endif
	}

	if((eFocus == NO_CITY_AI_FOCUS_TYPE || eFocus == CITY_AI_FOCUS_TYPE_PROD_GROWTH || eFocus == CITY_AI_FOCUS_TYPE_GOLD_GROWTH) && !bAvoidGrowth && m_pCity->getPopulation() < 5)
	{
		iFoodYieldValue *= 4;
	}

	iValue += iFoodYieldValue;
	iValue += iProductionYieldValue;
	iValue += iGoldYieldValue;
	iValue += iScienceYieldValue;
	iValue += iCultureYieldValue;
	iValue += iFaithYieldValue;
	iValue += iGPPYieldValue;
	iValue += iHappinessYieldValue;

	return iValue;
#endif
}

/// Determine if eSpecialist is preferable to a default specialist, based on our focus
#ifdef AUI_CONSTIFY
bool CvCityCitizens::IsBetterThanDefaultSpecialist(SpecialistTypes eSpecialist) const
#else
bool CvCityCitizens::IsBetterThanDefaultSpecialist(SpecialistTypes eSpecialist)
#endif
{
#ifdef AUI_CITIZENS_IS_BETTER_THAN_DEFAULT_SPECIALIST_USE_REGULAR_VALUES
	return GetSpecialistValue(eSpecialist) >= GetSpecialistValue((SpecialistTypes) GC.getDEFAULT_SPECIALIST());
#else
	CvSpecialistInfo* pSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
	CvAssertMsg(pSpecialistInfo, "Invalid specialist type when assigning citizens. Please send Anton your save file and version.");
	if(!pSpecialistInfo) return false; // Assumes that default specialist will work out

	SpecialistTypes eDefaultSpecialist = (SpecialistTypes) GC.getDEFAULT_SPECIALIST();
	CvSpecialistInfo* pDefaultSpecialistInfo = GC.getSpecialistInfo(eDefaultSpecialist);
	CvAssertMsg(pDefaultSpecialistInfo, "Invalid default specialist type when assigning citizens. Please send Anton your save file and version.");
	if(!pDefaultSpecialistInfo) return false;

	//antonjs: consider: deficient yield

	CityAIFocusTypes eFocus = GetFocusType();
	YieldTypes eYield = NO_YIELD;
	switch (eFocus)
	{
	case CITY_AI_FOCUS_TYPE_FOOD:
		eYield = YIELD_FOOD;
		break;
	case CITY_AI_FOCUS_TYPE_PRODUCTION:
		eYield = YIELD_PRODUCTION;
		break;
	case CITY_AI_FOCUS_TYPE_GOLD:
		eYield = YIELD_GOLD;
		break;
	case CITY_AI_FOCUS_TYPE_GREAT_PEOPLE:
		eYield = NO_YIELD;
		break;
	case CITY_AI_FOCUS_TYPE_SCIENCE:
		eYield = YIELD_SCIENCE;
		break;
	case CITY_AI_FOCUS_TYPE_CULTURE:
		eYield = YIELD_CULTURE;
		break;
	case CITY_AI_FOCUS_TYPE_PROD_GROWTH:
	case CITY_AI_FOCUS_TYPE_GOLD_GROWTH:
		eYield = YIELD_FOOD;
		break;
	case CITY_AI_FOCUS_TYPE_FAITH:
		eYield = YIELD_FAITH;
		break;
	default:
		eYield = NO_YIELD;
		break;
	}

	if (eYield == NO_YIELD)
		return true;

	int iSpecialistYield = pSpecialistInfo->getYieldChange(eYield);
	int iDefaultSpecialistYield = pDefaultSpecialistInfo->getYieldChange(eYield);

	if (m_pCity->GetPlayer()->isHalfSpecialistUnhappiness() || m_pCity->GetPlayer()->isHalfSpecialistFood())
	{
		iSpecialistYield *= 2;
	}

	return (iSpecialistYield >= iDefaultSpecialistYield); // Unless default Specialist has strictly more, this Specialist is better
#endif
}

/// How many Citizens need to be given a job?
int CvCityCitizens::GetNumUnassignedCitizens() const
{
	return m_iNumUnassignedCitizens;
}

/// Changes how many Citizens need to be given a job
void CvCityCitizens::ChangeNumUnassignedCitizens(int iChange)
{
	m_iNumUnassignedCitizens += iChange;
	CvAssert(m_iNumUnassignedCitizens >= 0);
}

/// How many Citizens are working Plots?
int CvCityCitizens::GetNumCitizensWorkingPlots() const
{
	return m_iNumCitizensWorkingPlots;
}

/// Changes how many Citizens are working Plots
void CvCityCitizens::ChangeNumCitizensWorkingPlots(int iChange)
{
	m_iNumCitizensWorkingPlots += iChange;
}

/// Pick the best Plot to work from one of our unassigned pool
#if defined(AUI_CITIZENS_IGNORE_FOOD_FOR_CITIZEN_ASSIGN_AFTER_GROW) || defined(AUI_CITIZENS_GET_VALUE_FROM_STATS)
bool CvCityCitizens::DoAddBestCitizenFromUnassigned(bool bAfterGrowth)
#else
bool CvCityCitizens::DoAddBestCitizenFromUnassigned()
#endif
{
	// We only assign the unassigned here, folks
	if (GetNumUnassignedCitizens() == 0)
	{
		return false;
	}

	// First Specialist Pass
	int iSpecialistValue = 0;
	BuildingTypes eBestSpecialistBuilding = NO_BUILDING;
	if (!IsNoAutoAssignSpecialists())
	{
#if defined(AUI_CITIZENS_IGNORE_FOOD_FOR_CITIZEN_ASSIGN_AFTER_GROW) || defined(AUI_CITIZENS_GET_VALUE_FROM_STATS)
		eBestSpecialistBuilding = GetAIBestSpecialistBuilding(iSpecialistValue, bAfterGrowth);
#else
		eBestSpecialistBuilding = GetAIBestSpecialistBuilding(iSpecialistValue);
#endif
	}

	bool bBetterThanSlacker = false;
	if (eBestSpecialistBuilding != NO_BUILDING)
	{
		CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBestSpecialistBuilding);
		if(pkBuildingInfo)
		{
			SpecialistTypes eSpecialist = (SpecialistTypes) pkBuildingInfo->GetSpecialistType();
			// Must not be worse than a default Specialist for our focus!
			if (IsBetterThanDefaultSpecialist(eSpecialist))
			{
				bBetterThanSlacker = true;
			}
		}
	}

	int iBestPlotValue = 0;
#if defined(AUI_CITIZENS_IGNORE_FOOD_FOR_CITIZEN_ASSIGN_AFTER_GROW) || defined(AUI_CITIZENS_GET_VALUE_FROM_STATS)
	CvPlot* pBestPlot = GetBestCityPlotWithValue(iBestPlotValue, /*bBest*/ true, /*bWorked*/ false, bAfterGrowth);
#else
	CvPlot* pBestPlot = GetBestCityPlotWithValue(iBestPlotValue, /*bBest*/ true, /*bWorked*/ false);
#endif

	bool bSpecialistBetterThanPlot = (eBestSpecialistBuilding != NO_BUILDING && iSpecialistValue >= iBestPlotValue);

	// Is there a Specialist we can assign?
	if (bSpecialistBetterThanPlot && bBetterThanSlacker)
	{
		DoAddSpecialistToBuilding(eBestSpecialistBuilding, /*bForced*/ false);
		return true;
	}
	// Found a Valid Plot to place a guy?
#ifdef AUI_CITIZENS_IS_PLOT_BETTER_THAN_DEFAULT_SPECIALIST
#if defined(AUI_CITIZENS_IGNORE_FOOD_FOR_CITIZEN_ASSIGN_AFTER_GROW) || defined(AUI_CITIZENS_GET_VALUE_FROM_STATS)
	else if (pBestPlot != NULL && iBestPlotValue >= GetSpecialistValue((SpecialistTypes)GC.getDEFAULT_SPECIALIST(), bAfterGrowth))
#else
	else if (pBestPlot != NULL && iBestPlotValue >= GetSpecialistValue((SpecialistTypes)GC.getDEFAULT_SPECIALIST()))
#endif
#else
	else if (!bSpecialistBetterThanPlot && pBestPlot != NULL)
#endif
	{
		// Now assign the guy to the best possible Plot
		SetWorkingPlot(pBestPlot, true);
		return true;
	}
	// No Valid Plots left - and no good specialists
	else
	{
		CvPlayer* pOwner = &GET_PLAYER(GetOwner());
		CvAssertMsg(pOwner, "Could not find owner of city when assigning citizens. Please send Anton your save file and version.");

		// Assign a cool Specialist! Only do this for AI players, or humans who do not have manual specialist control set
		if (pOwner)
		{
			if (!GET_PLAYER(GetOwner()).isHuman() || !IsNoAutoAssignSpecialists())
			{
#if defined(AUI_CITIZENS_IGNORE_FOOD_FOR_CITIZEN_ASSIGN_AFTER_GROW) || defined(AUI_CITIZENS_GET_VALUE_FROM_STATS)
				BuildingTypes eBestBuilding = GetAIBestSpecialistBuilding(iSpecialistValue, bAfterGrowth);
#else
				BuildingTypes eBestBuilding = GetAIBestSpecialistBuilding(iSpecialistValue);
#endif
				if(eBestBuilding != NO_BUILDING)
				{
					CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBestBuilding);
					if(pkBuildingInfo)
					{
						SpecialistTypes eSpecialist = (SpecialistTypes) pkBuildingInfo->GetSpecialistType();
						// Must not be worse than a default Specialist for our focus!
						if (IsBetterThanDefaultSpecialist(eSpecialist))
						{
							DoAddSpecialistToBuilding(eBestBuilding, false);
							return true;
						}
					}
				}
			}
		}

		// Default Specialist if we can't do anything else
		ChangeNumDefaultSpecialists(1);
	}

	return false;
}

/// Pick the worst Plot to stop working
bool CvCityCitizens::DoRemoveWorstCitizen(bool bRemoveForcedStatus, SpecialistTypes eDontChangeSpecialist, int iCurrentCityPopulation)
{
	if (iCurrentCityPopulation == -1)
	{
		iCurrentCityPopulation = GetCity()->getPopulation();
	}

	// Are all of our guys already not working Plots?
	if(GetNumUnassignedCitizens() == GetCity()->getPopulation())
	{
		return false;
	}

	// Find default Specialist to pull off, if there is one
	if(GetNumDefaultSpecialists() > 0)
	{
		// Do we either have unforced default specialists we can remove?
		if(GetNumDefaultSpecialists() > GetNumForcedDefaultSpecialists())
		{
			ChangeNumDefaultSpecialists(-1);
			return true;
		}
		if(GetNumDefaultSpecialists() > iCurrentCityPopulation)
		{
			ChangeNumForcedDefaultSpecialists(-1);
			ChangeNumDefaultSpecialists(-1);
			return true;
		}
	}

	// No Default Specialists, remove a working Pop, if there is one
	int iWorstPlotValue = 0;
	CvPlot* pWorstPlot = GetBestCityPlotWithValue(iWorstPlotValue, /*bBest*/ false, /*bWorked*/ true);

	if(pWorstPlot != NULL)
	{
		SetWorkingPlot(pWorstPlot, false);

		// If we were force-working this Plot, turn it off
		if(bRemoveForcedStatus)
		{
			if(IsForcedWorkingPlot(pWorstPlot))
			{
				SetForcedWorkingPlot(pWorstPlot, false);
			}
		}

		return true;
	}
	// Have to resort to pulling away a good Specialist
	else
	{
		if(DoRemoveWorstSpecialist(eDontChangeSpecialist))
		{
			return true;
		}
	}

	return false;
}

/// Find a Plot the City is either working or not, and the best/worst value for it - this function does "double duty" depending on what the user wants to find
#if defined(AUI_CITIZENS_IGNORE_FOOD_FOR_CITIZEN_ASSIGN_AFTER_GROW) || defined(AUI_CITIZENS_GET_VALUE_FROM_STATS)
CvPlot* CvCityCitizens::GetBestCityPlotWithValue(int& iValue, bool bWantBest, bool bWantWorked, bool bAfterGrowth)
#else
CvPlot* CvCityCitizens::GetBestCityPlotWithValue(int& iValue, bool bWantBest, bool bWantWorked)
#endif
{
	bool bPlotForceWorked;

	int iBestPlotValue = -1;
	int iBestPlotID = -1;

	CvPlot* pLoopPlot;

	// Look at all workable Plots
	for(int iPlotLoop = 0; iPlotLoop < NUM_CITY_PLOTS; iPlotLoop++)
	{
		if(iPlotLoop != CITY_HOME_PLOT)
		{
			pLoopPlot = GetCityPlotFromIndex(iPlotLoop);

			if(pLoopPlot != NULL)
			{
				// Is this a Plot this City controls?
				if(pLoopPlot->getWorkingCity() != NULL && pLoopPlot->getWorkingCity()->GetID() == GetCity()->GetID())
				{
					// Working the Plot and wanting to work it, or Not working it and wanting to find one to work?
					if((IsWorkingPlot(pLoopPlot) && bWantWorked) ||
					        (!IsWorkingPlot(pLoopPlot) && !bWantWorked))
					{
						// Working the Plot or CAN work the Plot?
						if(bWantWorked || IsCanWork(pLoopPlot))
						{
#ifdef AUI_CITIZENS_GET_VALUE_FROM_STATS
							iValue = GetPlotValue(pLoopPlot, bWantBest, NULL, NULL, 0, 0, bAfterGrowth);
#elif defined(AUI_CITIZENS_IGNORE_FOOD_FOR_CITIZEN_ASSIGN_AFTER_GROW)
							iValue = GetPlotValue(pLoopPlot, bWantBest, bAfterGrowth);
#else
							iValue = GetPlotValue(pLoopPlot, bWantBest);
#endif

							bPlotForceWorked = IsForcedWorkingPlot(pLoopPlot);

							if(bPlotForceWorked)
							{
								// Looking for best, unworked Plot: Forced plots are FIRST to be picked
								if(bWantBest && !bWantWorked)
								{
									iValue += 10000;
								}
								// Looking for worst, worked Plot: Forced plots are LAST to be picked, so make it's value incredibly high
								if(!bWantBest && bWantWorked)
								{
									iValue += 10000;
								}
							}

							if(iBestPlotValue == -1 ||							// First Plot?
							        (bWantBest && iValue > iBestPlotValue) ||		// Best Plot so far?
							        (!bWantBest && iValue < iBestPlotValue))			// Worst Plot so far?
							{
								iBestPlotValue = iValue;
								iBestPlotID = iPlotLoop;
							}
						}
					}
				}
			}
		}
	}

	// Passed in by reference
	iValue = iBestPlotValue;

	if(iBestPlotID == -1)
	{
		return NULL;
	}

	return GetCityPlotFromIndex(iBestPlotID);
}

/// Optimize our Citizen Placement
void CvCityCitizens::DoReallocateCitizens()
{
	// Make sure we don't have more forced working plots than we have citizens working.  If so, clean it up before reallocating
	DoValidateForcedWorkingPlots();

	// Remove all of the allocated guys
	int iNumCitizensToRemove = GetNumCitizensWorkingPlots();
	for(int iWorkerLoop = 0; iWorkerLoop < iNumCitizensToRemove; iWorkerLoop++)
	{
		DoRemoveWorstCitizen();
	}

	int iSpecialistLoop;

	// Remove Non-Forced Specialists in Buildings
	int iNumSpecialistsToRemove;
	BuildingTypes eBuilding;
	for(int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
	{
		eBuilding = (BuildingTypes) iBuildingLoop;

		// Have this Building in the City?
		if(GetCity()->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
		{
			iNumSpecialistsToRemove = GetNumSpecialistsInBuilding(eBuilding) - GetNumForcedSpecialistsInBuilding(eBuilding);	// Don't include Forced guys

			// Loop through guys to remove (if there are any)
			for(iSpecialistLoop = 0; iSpecialistLoop < iNumSpecialistsToRemove; iSpecialistLoop++)
			{
				DoRemoveSpecialistFromBuilding(eBuilding, /*bForced*/ false);
			}
		}
	}

	// Remove Default Specialists
	int iNumDefaultsToRemove = GetNumDefaultSpecialists() - GetNumForcedDefaultSpecialists();
	for(iSpecialistLoop = 0; iSpecialistLoop < iNumDefaultsToRemove; iSpecialistLoop++)
	{
		ChangeNumDefaultSpecialists(-1);
	}

	// Now put all of the unallocated guys back
	int iNumToAllocate = GetNumUnassignedCitizens();
	for(int iUnallocatedLoop = 0; iUnallocatedLoop < iNumToAllocate; iUnallocatedLoop++)
	{
		DoAddBestCitizenFromUnassigned();
	}
}



///////////////////////////////////////////////////
// Worked Plots
///////////////////////////////////////////////////



/// Is our City working a CvPlot?
bool CvCityCitizens::IsWorkingPlot(const CvPlot* pPlot) const
{
	int iIndex;

	iIndex = GetCityIndexFromPlot(pPlot);

	if(iIndex != -1)
	{
		return m_pabWorkingPlot[iIndex];
	}

	return false;
}

/// Tell a City to start or stop working a Plot.  Citizens will go to/from the Unassigned Pool if the 3rd argument is true
void CvCityCitizens::SetWorkingPlot(CvPlot* pPlot, bool bNewValue, bool bUseUnassignedPool)
{
	int iI;

	int iIndex = GetCityIndexFromPlot(pPlot);

	CvAssertMsg(iIndex >= 0, "iIndex expected to be >= 0");
	CvAssertMsg(iIndex < NUM_CITY_PLOTS, "iIndex expected to be < NUM_CITY_PLOTS");

	if(IsWorkingPlot(pPlot) != bNewValue && iIndex >= 0 && iIndex < NUM_CITY_PLOTS)
	{
		m_pabWorkingPlot[iIndex] = bNewValue;

		// Don't look at the center Plot of a City, because we always work it for free
		if(iIndex != CITY_HOME_PLOT)
		{
			// Alter the count of Plots being worked by Citizens
			if(bNewValue)
			{
				ChangeNumCitizensWorkingPlots(1);

				if(bUseUnassignedPool)
				{
					ChangeNumUnassignedCitizens(-1);
				}
			}
			else
			{
				ChangeNumCitizensWorkingPlots(-1);

				if(bUseUnassignedPool)
				{
					ChangeNumUnassignedCitizens(1);
				}
			}
		}

		if(pPlot != NULL)
		{
			// investigate later
			//CvAssertMsg(pPlot->getWorkingCity() == GetCity(), "WorkingCity is expected to be this");

			// Now working pPlot
			if(IsWorkingPlot(pPlot))
			{
				//if (iIndex != CITY_HOME_PLOT)
				//{
				//	GetCity()->changeWorkingPopulation(1);
				//}

				for(iI = 0; iI < NUM_YIELD_TYPES; iI++)
				{
					GetCity()->ChangeBaseYieldRateFromTerrain(((YieldTypes)iI), pPlot->getYield((YieldTypes)iI));
				}
			}
			// No longer working pPlot
			else
			{
				//if (iIndex != CITY_HOME_PLOT)
				//{
				//	GetCity()->changeWorkingPopulation(-1);
				//}

				for(iI = 0; iI < NUM_YIELD_TYPES; iI++)
				{
					GetCity()->ChangeBaseYieldRateFromTerrain(((YieldTypes)iI), -pPlot->getYield((YieldTypes)iI));
				}
			}
		}

		if(GetCity()->isCitySelected())
		{
			GC.GetEngineUserInterface()->setDirty(CityInfo_DIRTY_BIT, true);
			//GC.GetEngineUserInterface()->setDirty(InfoPane_DIRTY_BIT, true );
			GC.GetEngineUserInterface()->setDirty(CityScreen_DIRTY_BIT, true);
			GC.GetEngineUserInterface()->setDirty(ColoredPlots_DIRTY_BIT, true);
		}

		GC.GetEngineUserInterface()->setDirty(CityInfo_DIRTY_BIT, true);
	}
}

/// Tell City to work a Plot, pulling a Citizen from the worst location we can
void CvCityCitizens::DoAlterWorkingPlot(int iIndex)
{
	CvAssertMsg(iIndex >= 0, "iIndex expected to be >= 0");
	CvAssertMsg(iIndex < NUM_CITY_PLOTS, "iIndex expected to be < NUM_CITY_PLOTS");

	// Clicking ON the city "resets" it to default setup
	if(iIndex == CITY_HOME_PLOT)
	{
		CvPlot* pLoopPlot;

		// If we've forced any plots to be worked, reset them to the normal state
		for(int iPlotLoop = 0; iPlotLoop < NUM_CITY_PLOTS; iPlotLoop++)
		{
			if(iPlotLoop != CITY_HOME_PLOT)
			{
				pLoopPlot = GetCityPlotFromIndex(iPlotLoop);

				if(pLoopPlot != NULL)
				{
					if(IsForcedWorkingPlot(pLoopPlot))
					{
						SetForcedWorkingPlot(pLoopPlot, false);
					}
				}
			}
		}

		// Reset Forced Default Specialists
		ChangeNumForcedDefaultSpecialists(-GetNumForcedDefaultSpecialists());

		DoReallocateCitizens();
	}
	else
	{
		CvPlot* pPlot = GetCityPlotFromIndex(iIndex);

		if(pPlot != NULL)
		{
			if(IsCanWork(pPlot))
			{
//				GetCity()->setCitizensAutomated(false);

				// If we're already working the Plot, then take the guy off and turn him into a Default Specialist
				if(IsWorkingPlot(pPlot))
				{
					SetWorkingPlot(pPlot, false);
					SetForcedWorkingPlot(pPlot, false);
					ChangeNumDefaultSpecialists(1);
					ChangeNumForcedDefaultSpecialists(1);
				}
				// Player picked a new Plot to work
				else
				{
					// Pull from the Default Specialist pool, if possible
					if(GetNumDefaultSpecialists() > 0)
					{
						ChangeNumDefaultSpecialists(-1);
						// Player is forcibly telling city to work a plot, so reduce count of forced default specialists
						if(GetNumForcedDefaultSpecialists() > 0)
							ChangeNumForcedDefaultSpecialists(-1);

						SetWorkingPlot(pPlot, true);
						SetForcedWorkingPlot(pPlot, true);
					}
					// No Default Specialists, so grab a better allocated guy
					else
					{
						// Working Plot
						if(DoRemoveWorstCitizen(true))
						{
							SetWorkingPlot(pPlot, true);
							SetForcedWorkingPlot(pPlot, true);
							//ChangeNumUnassignedCitizens(-1);
						}
						// Good Specialist
						else
						{
							CvAssert(false);
						}
					}
					//if ((GetCity()->extraSpecialists() > 0) || GetCity()->AI_removeWorstCitizen())
					//{
					//	SetWorkingPlot(pPlot, true);
					//}
				}
			}
			// JON: Need to update this block to work with new system
			else if(pPlot->getOwner() == GetOwner())
			{
				// Can't take away forced plots from puppet Cities
				if(pPlot->getWorkingCityOverride() != NULL)
				{
					if(pPlot->getWorkingCityOverride()->IsPuppet())
					{
						return;
					}
				}

				pPlot->setWorkingCityOverride(GetCity());
			}
		}
	}
}



/// Has our City been told it MUST a particular CvPlot?
bool CvCityCitizens::IsForcedWorkingPlot(const CvPlot* pPlot) const
{
	int iIndex;

	iIndex = GetCityIndexFromPlot(pPlot);

	if(iIndex != -1)
	{
		return m_pabForcedWorkingPlot[iIndex];
	}

	return false;
}

/// Tell our City it MUST work a particular CvPlot
void CvCityCitizens::SetForcedWorkingPlot(CvPlot* pPlot, bool bNewValue)
{
	int iIndex = GetCityIndexFromPlot(pPlot);

	CvAssertMsg(iIndex >= 0, "iIndex expected to be >= 0");
	CvAssertMsg(iIndex < NUM_CITY_PLOTS, "iIndex expected to be < NUM_CITY_PLOTS");

	if(IsForcedWorkingPlot(pPlot) != bNewValue && iIndex >= 0 && iIndex < NUM_CITY_PLOTS)
	{
		m_pabForcedWorkingPlot[iIndex] = bNewValue;

		// Change the count of how many are forced
		if(bNewValue)
		{
			ChangeNumForcedWorkingPlots(1);

			// More forced plots than we have citizens working?  If so, then pick someone to lose their forced status
			if(GetNumForcedWorkingPlots() > GetNumCitizensWorkingPlots())
			{
				DoValidateForcedWorkingPlots();
			}
		}
		else
		{
			ChangeNumForcedWorkingPlots(-1);
		}
	}
}

/// Make sure we don't have more forced working plots than we have citizens to work
void CvCityCitizens::DoValidateForcedWorkingPlots()
{
	int iNumForcedWorkingPlotsToDemote = GetNumForcedWorkingPlots() - GetNumCitizensWorkingPlots();

	if(iNumForcedWorkingPlotsToDemote > 0)
	{
		for(int iLoop = 0; iLoop < iNumForcedWorkingPlotsToDemote; iLoop++)
		{
			DoDemoteWorstForcedWorkingPlot();
		}
	}
}

/// Remove the Forced status from the worst ForcedWorking plot
void CvCityCitizens::DoDemoteWorstForcedWorkingPlot()
{
	int iValue;

	int iBestPlotValue = -1;
	int iBestPlotID = -1;

	CvPlot* pLoopPlot;

	// Look at all workable Plots
	for(int iPlotLoop = 0; iPlotLoop < NUM_CITY_PLOTS; iPlotLoop++)
	{
		if(iPlotLoop != CITY_HOME_PLOT)
		{
			pLoopPlot = GetCityPlotFromIndex(iPlotLoop);

			if(pLoopPlot != NULL)
			{
				if(IsForcedWorkingPlot(pLoopPlot))
				{
					iValue = GetPlotValue(pLoopPlot, false);

					// First, or worst yet?
					if(iBestPlotValue == -1 || iValue < iBestPlotValue)
					{
						iBestPlotValue = iValue;
						iBestPlotID = iPlotLoop;
					}
				}
			}
		}
	}

	if(iBestPlotID > -1)
	{
		pLoopPlot = GetCityPlotFromIndex(iBestPlotID);
		SetForcedWorkingPlot(pLoopPlot, false);
	}
}

/// How many plots have we forced to be worked?
int CvCityCitizens::GetNumForcedWorkingPlots() const
{
	return m_iNumForcedWorkingPlots;
}

/// Changes how many plots we have forced to be worked
void CvCityCitizens::ChangeNumForcedWorkingPlots(int iChange)
{
	if(iChange != 0)
	{
		m_iNumForcedWorkingPlots += iChange;
	}
}

/// Can our City work a particular CvPlot?
#ifdef AUI_PLAYER_RESOLVE_WORKED_PLOT_CONFLICTS
bool CvCityCitizens::IsCanWork(CvPlot* pPlot, bool bIgnoreOverride) const
{
	if (pPlot->getWorkingCity() != m_pCity && !bIgnoreOverride)
#else
bool CvCityCitizens::IsCanWork(CvPlot* pPlot) const
{
	if(pPlot->getWorkingCity() != m_pCity)
#endif
	{
		return false;
	}

	CvAssertMsg(GetCityIndexFromPlot(pPlot) != -1, "GetCityIndexFromPlot(pPlot) is expected to be assigned (not -1)");

	if(pPlot->plotCheck(PUF_canSiege, GetOwner()) != NULL)
	{
		return false;
	}

	if(pPlot->isWater())
	{
		if(!(GET_TEAM(GetTeam()).isWaterWork()))
		{
			return false;
		}

	}

	if(!pPlot->hasYield())
	{
		return false;
	}

	if(IsPlotBlockaded(pPlot))
	{
		return false;
	}

	return true;
}

// Is there a naval blockade on this water tile?
bool CvCityCitizens::IsPlotBlockaded(CvPlot* pPlot) const
{
	// See if there are any enemy boats near us that are blockading this plot
	int iBlockadeDistance = /*2*/ GC.getNAVAL_PLOT_BLOCKADE_RANGE();
	int iDX, iDY;
	CvPlot* pNearbyPlot;

	PlayerTypes ePlayer = m_pCity->getOwner();

	// Might be a better way to do this that'd be slightly less CPU-intensive
#ifdef AUI_HEXSPACE_DX_LOOPS
	int iMaxDX;
	for (iDY = -iBlockadeDistance; iDY <= iBlockadeDistance; iDY++)
	{
		iMaxDX = iBlockadeDistance - MAX(0, iDY);
		for (iDX = -iBlockadeDistance - MIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
		{
#else
	for(iDX = -(iBlockadeDistance); iDX <= iBlockadeDistance; iDX++)
	{
		for(iDY = -(iBlockadeDistance); iDY <= iBlockadeDistance; iDY++)
		{
#endif
			pNearbyPlot = plotXY(pPlot->getX(), pPlot->getY(), iDX, iDY);

			if(pNearbyPlot != NULL)
			{
				// Must be water in the same Area
				if(pNearbyPlot->isWater() && pNearbyPlot->getArea() == pPlot->getArea())
				{
#ifdef AUI_FIX_HEX_DISTANCE_INSTEAD_OF_PLOT_DISTANCE
					if(hexDistance(iDX, iDY) <= iBlockadeDistance)
#else
					if(plotDistance(pNearbyPlot->getX(), pNearbyPlot->getY(), pPlot->getX(), pPlot->getY()) <= iBlockadeDistance)
#endif
					{
						// Enemy boat within range to blockade our plot?
						if(pNearbyPlot->IsActualEnemyUnit(ePlayer))
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

// Is there a naval blockade on any of this city's water tiles?
bool CvCityCitizens::IsAnyPlotBlockaded() const
{
	CvPlot* pLoopPlot;

	// Look at all workable Plots
	for(int iPlotLoop = 0; iPlotLoop < NUM_CITY_PLOTS; iPlotLoop++)
	{
		if(iPlotLoop != CITY_HOME_PLOT)
		{
			pLoopPlot = GetCityPlotFromIndex(iPlotLoop);

			if(pLoopPlot != NULL)
			{
				if(IsPlotBlockaded(pLoopPlot))
				{
					return true;
				}
			}
		}
	}

	return false;
}

/// If we're working this plot make sure we're allowed, and if we're not then correct the situation
void CvCityCitizens::DoVerifyWorkingPlot(CvPlot* pPlot)
{
	if(pPlot != NULL)
	{
		if(IsWorkingPlot(pPlot))
		{
			if(!IsCanWork(pPlot))
			{
				SetWorkingPlot(pPlot, false);
				DoAddBestCitizenFromUnassigned();
			}
		}
	}
}

/// Check all Plots by this City to see if we can actually be working them (if we are)
void CvCityCitizens::DoVerifyWorkingPlots()
{
	int iI;
	CvPlot* pPlot;

	for(iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		pPlot = GetCityPlotFromIndex(iI);

		DoVerifyWorkingPlot(pPlot);
	}
}




///////////////////////////////////////////////////
// Helpful Stuff
///////////////////////////////////////////////////




/// Returns the Plot Index from a CvPlot
int CvCityCitizens::GetCityIndexFromPlot(const CvPlot* pPlot) const
{
	return plotCityXY(m_pCity, pPlot);
}


/// Returns the CvPlot from a Plot Index
CvPlot* CvCityCitizens::GetCityPlotFromIndex(int iIndex) const
{
	return plotCity(m_pCity->getX(), m_pCity->getY(), iIndex);
}




///////////////////////////////////////////////////
// Specialists
///////////////////////////////////////////////////



/// Called at the end of every turn: Looks at the specialists in this City and levels them up
void CvCityCitizens::DoSpecialists()
{
	int iGPPChange;
	int iCount;
	int iMod;
	for(int iSpecialistLoop = 0; iSpecialistLoop < GC.getNumSpecialistInfos(); iSpecialistLoop++)
	{
		const SpecialistTypes eSpecialist = static_cast<SpecialistTypes>(iSpecialistLoop);
		CvSpecialistInfo* pkSpecialistInfo = GC.getSpecialistInfo(eSpecialist);
		if(pkSpecialistInfo)
		{
			int iGPThreshold = GetSpecialistUpgradeThreshold((UnitClassTypes)pkSpecialistInfo->getGreatPeopleUnitClass());

			// Does this Specialist spawn a GP?
			if(pkSpecialistInfo->getGreatPeopleUnitClass() != NO_UNITCLASS)
			{
				iCount = GetSpecialistCount(eSpecialist);

				// GPP from Specialists
				iGPPChange = pkSpecialistInfo->getGreatPeopleRateChange() * iCount * 100;

				// GPP from Buildings
				iGPPChange += GetBuildingGreatPeopleRateChanges(eSpecialist) * 100;

				if(iGPPChange > 0)
				{
					iMod = 0;

					// City mod
					iMod += GetCity()->getGreatPeopleRateModifier();

					// Player mod
					iMod += GetPlayer()->getGreatPeopleRateModifier();

					// Player and Golden Age mods to this specific class
					if((UnitClassTypes)pkSpecialistInfo->getGreatPeopleUnitClass() == GC.getInfoTypeForString("UNITCLASS_SCIENTIST"))
					{
						iMod += GetPlayer()->getGreatScientistRateModifier();
					}
					else if((UnitClassTypes)pkSpecialistInfo->getGreatPeopleUnitClass() == GC.getInfoTypeForString("UNITCLASS_WRITER"))
					{ 
						if (GetPlayer()->isGoldenAge())
						{
							iMod += GetPlayer()->GetPlayerTraits()->GetGoldenAgeGreatWriterRateModifier();
						}
						iMod += GetPlayer()->getGreatWriterRateModifier();
					}
					else if((UnitClassTypes)pkSpecialistInfo->getGreatPeopleUnitClass() == GC.getInfoTypeForString("UNITCLASS_ARTIST"))
					{
						if (GetPlayer()->isGoldenAge())
						{
							iMod += GetPlayer()->GetPlayerTraits()->GetGoldenAgeGreatArtistRateModifier();
						}
						iMod += GetPlayer()->getGreatArtistRateModifier();
					}
					else if((UnitClassTypes)pkSpecialistInfo->getGreatPeopleUnitClass() == GC.getInfoTypeForString("UNITCLASS_MUSICIAN"))
					{
						if (GetPlayer()->isGoldenAge())
						{
							iMod += GetPlayer()->GetPlayerTraits()->GetGoldenAgeGreatMusicianRateModifier();
						}
						iMod += GetPlayer()->getGreatMusicianRateModifier();
					}
					else if((UnitClassTypes)pkSpecialistInfo->getGreatPeopleUnitClass() == GC.getInfoTypeForString("UNITCLASS_MERCHANT"))
					{
						iMod += GetPlayer()->getGreatMerchantRateModifier();
					}
					else if((UnitClassTypes)pkSpecialistInfo->getGreatPeopleUnitClass() == GC.getInfoTypeForString("UNITCLASS_ENGINEER"))
					{
						iMod += GetPlayer()->getGreatEngineerRateModifier();
					}

					// Apply mod
					iGPPChange *= (100 + iMod);
					iGPPChange /= 100;

					ChangeSpecialistGreatPersonProgressTimes100(eSpecialist, iGPPChange);
				}

				// Enough to spawn a GP?
				if(GetSpecialistGreatPersonProgress(eSpecialist) >= iGPThreshold)
				{
					// No Minors
					if(!GET_PLAYER(GetCity()->getOwner()).isMinorCiv())
					{
						// Reset progress on this Specialist
						DoResetSpecialistGreatPersonProgressTimes100(eSpecialist);

						// Now... actually create the GP!
						const UnitClassTypes eUnitClass = (UnitClassTypes) pkSpecialistInfo->getGreatPeopleUnitClass();
						const CivilizationTypes eCivilization = GetCity()->getCivilizationType();
						CvCivilizationInfo* pCivilizationInfo = GC.getCivilizationInfo(eCivilization);
						if(pCivilizationInfo != NULL)
						{
							UnitTypes eUnit = (UnitTypes) pCivilizationInfo->getCivilizationUnits(eUnitClass);

							DoSpawnGreatPerson(eUnit, true, false);
						}
					}
				}
			}
		}
	}
}

/// How many Specialists are assigned to this Building Type?
#ifdef AUI_CONSTIFY
int CvCityCitizens::GetNumSpecialistsAllowedByBuilding(const CvBuildingEntry& kBuilding) const
#else
int CvCityCitizens::GetNumSpecialistsAllowedByBuilding(const CvBuildingEntry& kBuilding)
#endif
{
	return kBuilding.GetSpecialistCount();
}

/// Are we in the position to add another Specialist to eBuilding?
#ifdef AUI_CONSTIFY
bool CvCityCitizens::IsCanAddSpecialistToBuilding(BuildingTypes eBuilding) const
#else
bool CvCityCitizens::IsCanAddSpecialistToBuilding(BuildingTypes eBuilding)
#endif
{
	CvAssert(eBuilding > -1);
	CvAssert(eBuilding < GC.getNumBuildingInfos());

	int iNumSpecialistsAssigned = GetNumSpecialistsInBuilding(eBuilding);

	if(iNumSpecialistsAssigned < GetCity()->getPopulation() &&	// Limit based on Pop of City
	        iNumSpecialistsAssigned < GC.getBuildingInfo(eBuilding)->GetSpecialistCount() &&				// Limit for this particular Building
	        iNumSpecialistsAssigned < GC.getMAX_SPECIALISTS_FROM_BUILDING())	// Overall Limit
	{
		return true;
	}

	return false;
}

/// Adds and initializes a Specialist for this building
void CvCityCitizens::DoAddSpecialistToBuilding(BuildingTypes eBuilding, bool bForced)
{
	CvAssert(eBuilding > -1);
	CvAssert(eBuilding < GC.getNumBuildingInfos());

	CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
	if(pkBuildingInfo == NULL)
	{
		return;
	}

	SpecialistTypes eSpecialist = (SpecialistTypes) pkBuildingInfo->GetSpecialistType();

	// Can't add more than the max
	if(IsCanAddSpecialistToBuilding(eBuilding))
	{
		// If we're force-assigning a specialist, then we can reduce the count on forced default specialists
		if(bForced)
		{
			if(GetNumForcedDefaultSpecialists() > 0)
				ChangeNumForcedDefaultSpecialists(-1);
		}

		// If we don't already have an Unassigned Citizen to turn into a Specialist, find one from somewhere
		if(GetNumUnassignedCitizens() == 0)
		{
			DoRemoveWorstCitizen(true, /*Don't remove this type*/ eSpecialist);
			if(GetNumUnassignedCitizens() == 0)
			{
				// Still nobody, all the citizens may be assigned to the eSpecialist we are looking for, try again
				if(!DoRemoveWorstSpecialist(NO_SPECIALIST, eBuilding))
				{
					return; // For some reason we can't do this, we must exit, else we will be going over the population count
				}
			}
		}

		// Increase count for the whole city
		m_aiSpecialistCounts[eSpecialist]++;
		m_aiNumSpecialistsInBuilding[eBuilding]++;

		if(bForced)
		{
			m_aiNumForcedSpecialistsInBuilding[eBuilding]++;
		}

		GetCity()->processSpecialist(eSpecialist, 1);
		GetCity()->UpdateReligion(GetCity()->GetCityReligions()->GetReligiousMajority());

		ChangeNumUnassignedCitizens(-1);

		ICvUserInterface2* pkIFace = GC.GetEngineUserInterface();
		pkIFace->setDirty(GameData_DIRTY_BIT, true);
		pkIFace->setDirty(CityInfo_DIRTY_BIT, true);
		//pkIFace->setDirty(InfoPane_DIRTY_BIT, true );
		pkIFace->setDirty(CityScreen_DIRTY_BIT, true);
		pkIFace->setDirty(ColoredPlots_DIRTY_BIT, true);

		CvCity* pkCity = GetCity();
		auto_ptr<ICvCity1> pCity = GC.WrapCityPointer(pkCity);

		pkIFace->SetSpecificCityInfoDirty(pCity.get(), CITY_UPDATE_TYPE_SPECIALISTS);
	}
}

/// Removes and uninitializes a Specialist for this building
void CvCityCitizens::DoRemoveSpecialistFromBuilding(BuildingTypes eBuilding, bool bForced, bool bEliminatePopulation)
{
	CvAssert(eBuilding > -1);
	CvAssert(eBuilding < GC.getNumBuildingInfos());

	CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
	if(pkBuildingInfo == NULL)
	{
		return;
	}

	SpecialistTypes eSpecialist = (SpecialistTypes) pkBuildingInfo->GetSpecialistType();

	int iNumSpecialistsAssigned = GetNumSpecialistsInBuilding(eBuilding);

	// Need at least 1 assigned to remove
	if(iNumSpecialistsAssigned > 0)
	{
		// Decrease count for the whole city
		m_aiSpecialistCounts[eSpecialist]--;
		m_aiNumSpecialistsInBuilding[eBuilding]--;

		if(bForced)
		{
			m_aiNumForcedSpecialistsInBuilding[eBuilding]--;
		}

		GetCity()->processSpecialist(eSpecialist, -1);
		GetCity()->UpdateReligion(GetCity()->GetCityReligions()->GetReligiousMajority());

		// Do we kill this population or reassign him?
		if(bEliminatePopulation)
		{
			GetCity()->changePopulation(-1, /*bReassignPop*/ false);
		}
		else
		{
			ChangeNumUnassignedCitizens(1);
		}

		GC.GetEngineUserInterface()->setDirty(GameData_DIRTY_BIT, true);
		GC.GetEngineUserInterface()->setDirty(CityInfo_DIRTY_BIT, true);
		//GC.GetEngineUserInterface()->setDirty(InfoPane_DIRTY_BIT, true );
		GC.GetEngineUserInterface()->setDirty(CityScreen_DIRTY_BIT, true);
		GC.GetEngineUserInterface()->setDirty(ColoredPlots_DIRTY_BIT, true);

		auto_ptr<ICvCity1> pCity = GC.WrapCityPointer(GetCity());

		GC.GetEngineUserInterface()->SetSpecificCityInfoDirty(pCity.get(), CITY_UPDATE_TYPE_SPECIALISTS);
	}
}

//	----------------------------------------------------------------------------
/// Clear EVERYONE from this Building
/// Any one in the building will be put in the unassigned citizens list.
/// It is up to the caller to reassign population.
void CvCityCitizens::DoRemoveAllSpecialistsFromBuilding(BuildingTypes eBuilding, bool bEliminatePopulation)
{
	CvAssert(eBuilding > -1);
	CvAssert(eBuilding < GC.getNumBuildingInfos());

	CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
	if(pkBuildingInfo == NULL)
	{
		return;
	}

	SpecialistTypes eSpecialist = (SpecialistTypes) pkBuildingInfo->GetSpecialistType();
	int iNumSpecialists = GetNumSpecialistsInBuilding(eBuilding);

	m_aiNumForcedSpecialistsInBuilding[eBuilding] = 0;

	// Pick the worst to remove
	for(int iAssignedLoop = 0; iAssignedLoop < iNumSpecialists; iAssignedLoop++)
	{
		// Decrease count for the whole city
		m_aiSpecialistCounts[eSpecialist]--;
		m_aiNumSpecialistsInBuilding[eBuilding]--;
		GetCity()->processSpecialist(eSpecialist, -1);

		// Do we kill this population or reassign him?
		if(bEliminatePopulation)
		{
			GetCity()->changePopulation(-1, /*bReassignPop*/ false);
		}
		else
		{
			ChangeNumUnassignedCitizens(1);
		}

		GC.GetEngineUserInterface()->setDirty(CityInfo_DIRTY_BIT, true);
		//GC.GetEngineUserInterface()->setDirty(InfoPane_DIRTY_BIT, true );
		GC.GetEngineUserInterface()->setDirty(CityScreen_DIRTY_BIT, true);
		GC.GetEngineUserInterface()->setDirty(ColoredPlots_DIRTY_BIT, true);

		auto_ptr<ICvCity1> pCity = GC.WrapCityPointer(GetCity());
		GC.GetEngineUserInterface()->SetSpecificCityInfoDirty(pCity.get(), CITY_UPDATE_TYPE_SPECIALISTS);
	}
}


/// Find the worst Specialist and remove him from duty
bool CvCityCitizens::DoRemoveWorstSpecialist(SpecialistTypes eDontChangeSpecialist, const BuildingTypes eDontRemoveFromBuilding /* = NO_BUILDING */)
{
	for(int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
	{
		const BuildingTypes eBuilding = static_cast<BuildingTypes>(iBuildingLoop);

		if(eBuilding == eDontRemoveFromBuilding)
		{
			continue;
		}

		CvBuildingEntry* pkBuildingInfo = GC.getBuildingInfo(eBuilding);
		if(pkBuildingInfo == NULL)
		{
			continue;
		}

		// We might not be allowed to change this Building's Specialists
		if(eDontChangeSpecialist == pkBuildingInfo->GetSpecialistType())
		{
			continue;
		}

		if(GetNumSpecialistsInBuilding(eBuilding) > 0)
		{
			DoRemoveSpecialistFromBuilding(eBuilding, true);

			return true;
		}
	}

	return false;
}

/// How many Default Specialists are assigned in this City?
int CvCityCitizens::GetNumDefaultSpecialists() const
{
	return m_iNumDefaultSpecialists;
}

/// Changes how many Default Specialists are assigned in this City
void CvCityCitizens::ChangeNumDefaultSpecialists(int iChange)
{
	m_iNumDefaultSpecialists += iChange;

	SpecialistTypes eSpecialist = (SpecialistTypes) GC.getDEFAULT_SPECIALIST();
	m_aiSpecialistCounts[eSpecialist] += iChange;

	GetCity()->processSpecialist(eSpecialist, iChange);

	ChangeNumUnassignedCitizens(-iChange);
}

/// How many Default Specialists have been forced assigned in this City?
int CvCityCitizens::GetNumForcedDefaultSpecialists() const
{
	return m_iNumForcedDefaultSpecialists;
}

/// How many Default Specialists have been forced assigned in this City?
void CvCityCitizens::ChangeNumForcedDefaultSpecialists(int iChange)
{
	m_iNumForcedDefaultSpecialists += iChange;
}

/// How many Specialists do we have assigned of this type in our City?
int CvCityCitizens::GetSpecialistCount(SpecialistTypes eIndex) const
{
	CvAssert(eIndex > -1);
	CvAssert(eIndex < GC.getNumSpecialistInfos());

	return m_aiSpecialistCounts[eIndex];
}

/// Count up all the Specialists we have here
int CvCityCitizens::GetTotalSpecialistCount() const
{
	int iNumSpecialists = 0;
	SpecialistTypes eSpecialist;

	for(int iSpecialistLoop = 0; iSpecialistLoop < GC.getNumSpecialistInfos(); iSpecialistLoop++)
	{
		eSpecialist = (SpecialistTypes) iSpecialistLoop;

		if (eSpecialist != (SpecialistTypes) GC.getDEFAULT_SPECIALIST())
		{
			iNumSpecialists += GetSpecialistCount(eSpecialist);
		}
	}

	return iNumSpecialists;
}

/// GPP changes from Buildings
int CvCityCitizens::GetBuildingGreatPeopleRateChanges(SpecialistTypes eSpecialist) const
{
	CvAssert(eSpecialist > -1);
	CvAssert(eSpecialist < GC.getNumSpecialistInfos());

	return m_piBuildingGreatPeopleRateChanges[eSpecialist];
}

/// Change GPP from Buildings
void CvCityCitizens::ChangeBuildingGreatPeopleRateChanges(SpecialistTypes eSpecialist, int iChange)
{
	CvAssert(eSpecialist > -1);
	CvAssert(eSpecialist < GC.getNumSpecialistInfos());

	m_piBuildingGreatPeopleRateChanges[eSpecialist] += iChange;
}

/// How much progress does this City have towards a Great Person from eIndex?
int CvCityCitizens::GetSpecialistGreatPersonProgress(SpecialistTypes eIndex) const
{
	CvAssert(eIndex > -1);
	CvAssert(eIndex < GC.getNumSpecialistInfos());

	return GetSpecialistGreatPersonProgressTimes100(eIndex) / 100;
}

/// How much progress does this City have towards a Great Person from eIndex? (in hundreds)
int CvCityCitizens::GetSpecialistGreatPersonProgressTimes100(SpecialistTypes eIndex) const
{
	CvAssert(eIndex > -1);
	CvAssert(eIndex < GC.getNumSpecialistInfos());

	return m_aiSpecialistGreatPersonProgressTimes100[eIndex];
}

/// How much progress does this City have towards a Great Person from eIndex?
void CvCityCitizens::ChangeSpecialistGreatPersonProgressTimes100(SpecialistTypes eIndex, int iChange)
{
	CvAssert(eIndex > -1);
	CvAssert(eIndex < GC.getNumSpecialistInfos());

	m_aiSpecialistGreatPersonProgressTimes100[eIndex] += iChange;
}

/// Reset Specialist progress
void CvCityCitizens::DoResetSpecialistGreatPersonProgressTimes100(SpecialistTypes eIndex)
{
	CvAssert(eIndex > -1);
	CvAssert(eIndex < GC.getNumSpecialistInfos());

	m_aiSpecialistGreatPersonProgressTimes100[eIndex] = 0;
}

/// How many Specialists are assigned to eBuilding?
int CvCityCitizens::GetNumSpecialistsInBuilding(BuildingTypes eBuilding) const
{
	CvAssert(eBuilding > -1);
	CvAssert(eBuilding < GC.getNumBuildingInfos());

	return m_aiNumSpecialistsInBuilding[eBuilding];
}

/// How many Forced Specialists are assigned to eBuilding?
int CvCityCitizens::GetNumForcedSpecialistsInBuilding(BuildingTypes eBuilding) const
{
	CvAssert(eBuilding > -1);
	CvAssert(eBuilding < GC.getNumBuildingInfos());

	return m_aiNumForcedSpecialistsInBuilding[eBuilding];
}

/// Remove forced status from all Specialists
void CvCityCitizens::DoClearForcedSpecialists()
{
	// Loop through all Buildings
	BuildingTypes eBuilding;
	for(int iBuildingLoop = 0; iBuildingLoop < GC.getNumBuildingInfos(); iBuildingLoop++)
	{
		eBuilding = (BuildingTypes) iBuildingLoop;

		// Have this Building in the City?
		if(GetCity()->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
		{
			m_aiNumForcedSpecialistsInBuilding[eBuilding] = 0;
		}
	}
}

/// What upgrade progress does a Specialist need to level up?
#ifdef AUI_CONSTIFY
int CvCityCitizens::GetSpecialistUpgradeThreshold(UnitClassTypes eUnitClass) const
#else
int CvCityCitizens::GetSpecialistUpgradeThreshold(UnitClassTypes eUnitClass)
#endif
{
	int iThreshold = /*100*/ GC.getGREAT_PERSON_THRESHOLD_BASE();
	int iNumCreated;

	if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_WRITER", true))
	{
		iNumCreated = GET_PLAYER(GetCity()->getOwner()).getGreatWritersCreated();
	}
	else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_ARTIST", true))
	{
		iNumCreated = GET_PLAYER(GetCity()->getOwner()).getGreatArtistsCreated();
	}
	else if (eUnitClass == GC.getInfoTypeForString("UNITCLASS_MUSICIAN", true))
	{
		iNumCreated = GET_PLAYER(GetCity()->getOwner()).getGreatMusiciansCreated();
	}
	else
	{
		iNumCreated = GET_PLAYER(GetCity()->getOwner()).getGreatPeopleCreated();
	}

	// Increase threshold based on how many GP have already been spawned
	iThreshold += (/*50*/ GC.getGREAT_PERSON_THRESHOLD_INCREASE() * iNumCreated);

	// Game Speed mod
	iThreshold *= GC.getGame().getGameSpeedInfo().getGreatPeoplePercent();
	iThreshold /= 100;

	// Start era mod
	iThreshold *= GC.getGame().getStartEraInfo().getGreatPeoplePercent();
	iThreshold /= 100;

	return iThreshold;
}

/// Create a GP!
void CvCityCitizens::DoSpawnGreatPerson(UnitTypes eUnit, bool bIncrementCount, bool bCountAsProphet)
{
	CvAssert(eUnit != NO_UNIT);

	if (eUnit == NO_UNIT)
		return;	// Better than crashing.

	// If it's the active player then show the popup
	if(GetCity()->getOwner() == GC.getGame().getActivePlayer())
	{
		// Don't show in MP
		if(!GC.getGame().isNetworkMultiPlayer())	// KWG: Candidate for !GC.getGame().IsOption(GAMEOPTION_SIMULTANEOUS_TURNS)
		{
			CvPopupInfo kPopupInfo(BUTTONPOPUP_GREAT_PERSON_REWARD, eUnit, GetCity()->GetID());
			GC.GetEngineUserInterface()->AddPopup(kPopupInfo);
		}
	}

	CvPlayer& kPlayer = GET_PLAYER(GetCity()->getOwner());
	CvUnit* newUnit = kPlayer.initUnit(eUnit, GetCity()->getX(), GetCity()->getY());

	// Bump up the count
	if(bIncrementCount && !bCountAsProphet)
	{
		if(newUnit->IsGreatGeneral())
		{
			kPlayer.incrementGreatGeneralsCreated();
		}
		else if(newUnit->IsGreatAdmiral())
		{
			kPlayer.incrementGreatAdmiralsCreated();
			CvPlot *pSpawnPlot = kPlayer.GetGreatAdmiralSpawnPlot(newUnit);
			if (newUnit->plot() != pSpawnPlot)
			{
				newUnit->setXY(pSpawnPlot->getX(), pSpawnPlot->getY());
			}
		}
		else if (newUnit->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_WRITER"))
		{
			kPlayer.incrementGreatWritersCreated();
		}							
		else if (newUnit->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_ARTIST"))
		{
			kPlayer.incrementGreatArtistsCreated();
		}							
		else if (newUnit->getUnitInfo().GetUnitClassType() == GC.getInfoTypeForString("UNITCLASS_MUSICIAN"))
		{
			kPlayer.incrementGreatMusiciansCreated();
		}		
		else
		{
			kPlayer.incrementGreatPeopleCreated();
		}
	}
	if(bCountAsProphet || newUnit->getUnitInfo().IsFoundReligion())
	{
		kPlayer.GetReligions()->ChangeNumProphetsSpawned(1);
	}

	// Setup prophet properly
	if(newUnit->getUnitInfo().IsFoundReligion())
	{
		ReligionTypes eReligion = kPlayer.GetReligions()->GetReligionCreatedByPlayer();
		int iReligionSpreads = newUnit->getUnitInfo().GetReligionSpreads();
		int iReligiousStrength = newUnit->getUnitInfo().GetReligiousStrength();
		if(iReligionSpreads > 0 && eReligion > RELIGION_PANTHEON)
		{
			newUnit->GetReligionData()->SetSpreadsLeft(iReligionSpreads);
			newUnit->GetReligionData()->SetReligiousStrength(iReligiousStrength);
			newUnit->GetReligionData()->SetReligion(eReligion);
		}
	}

	if (newUnit->getUnitInfo().GetOneShotTourism() > 0)
	{
		newUnit->SetTourismBlastStrength(kPlayer.GetCulture()->GetTourismBlastStrength(newUnit->getUnitInfo().GetOneShotTourism()));
	}

	// Notification
	if(GET_PLAYER(GetOwner()).GetNotifications())
	{
		Localization::String strText = Localization::Lookup("TXT_KEY_NOTIFICATION_GREAT_PERSON_ACTIVE_PLAYER");
		Localization::String strSummary = Localization::Lookup("TXT_KEY_NOTIFICATION_SUMMARY_GREAT_PERSON");
		GET_PLAYER(GetOwner()).GetNotifications()->Add(NOTIFICATION_GREAT_PERSON_ACTIVE_PLAYER, strText.toUTF8(), strSummary.toUTF8(), GetCity()->getX(), GetCity()->getY(), eUnit);
	}
}