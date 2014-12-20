/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#include "CvGameCoreDLLPCH.h"
#include "CvGameCoreUtils.h"
#include "CvSiteEvaluationClasses.h"
#include "CvImprovementClasses.h"
#include "CvCitySpecializationAI.h"
#include "CvDiplomacyAI.h"
#include "CvGrandStrategyAI.h"

// include this after all other headers!
#include "LintFree.h"

//=====================================
// CvCitySiteEvaluator
//=====================================
/// Constructor
CvCitySiteEvaluator::CvCitySiteEvaluator(void)
{
	m_iExpansionIndex = 12;
	m_iGrowthIndex = 13;
}

/// Destructor
CvCitySiteEvaluator::~CvCitySiteEvaluator(void)
{
}

/// Initialize
void CvCitySiteEvaluator::Init()
{
	// Set up city ring multipliers
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
	m_iRingModifier[0] = /*2*/ GC.getCITY_RING_3_MULTIPLIER();
#else
	m_iRingModifier[0] = 1;   // Items under city get handled separately
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
	m_iRingModifier[1] = /*6*/ GC.getCITY_RING_1_MULTIPLIER();
	m_iRingModifier[2] = /*3*/ GC.getCITY_RING_2_MULTIPLIER();
	m_iRingModifier[3] = /*2*/ GC.getCITY_RING_3_MULTIPLIER();
	m_iRingModifier[4] = /*1*/ GC.getCITY_RING_4_MULTIPLIER();
	m_iRingModifier[5] = /*1*/ GC.getCITY_RING_5_MULTIPLIER();
	m_iRingModifier[6] = 0;
	m_iRingModifier[7] = 0;

	m_iGrowthIndex = GC.getInfoTypeForString("FLAVOR_GROWTH");
	m_iExpansionIndex = GC.getInfoTypeForString("FLAVOR_EXPANSION");
	m_iNavalIndex = GC.getInfoTypeForString("FLAVOR_NAVAL");

	m_iBrazilMultiplier = 1000;	//fertility boost from jungles
	m_iSpainMultiplier = 55000;	//fertility boost from natural wonders
	m_iMorrocoMultiplier = 1000; //fertility boost from desert
	m_iNetherlandsMultiplier = 2000; //fertility boost from marshes and flood plains
	m_iIncaMultiplier = 500; //fertility boost for hill tiles surrounded my mountains
}

/// Is it valid for this player to found a city here?
bool CvCitySiteEvaluator::CanFound(CvPlot* pPlot, const CvPlayer* pPlayer, bool bTestVisible) const
{
	CvAssert(pPlot);
	if(!pPlot)
		return false;

	CvPlot* pLoopPlot(NULL);
	bool bValid(false);
	int iRange(0), iDX(0), iDY(0);

	// Used to have a Python hook: CANNOT_FOUND_CITY_CALLBACK

	if(GC.getGame().isFinalInitialized())
	{
		if(GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && pPlayer && pPlayer->isHuman())
		{
			if(pPlayer->getNumCities() > 0)
			{
				return false;
			}
		}
	}

	if(pPlot->isImpassable() || pPlot->isMountain())
	{
		return false;
	}

	if(pPlot->getFeatureType() != NO_FEATURE)
	{
		if(GC.getFeatureInfo(pPlot->getFeatureType())->isNoCity())
		{
			return false;
		}
	}

	if(pPlayer)
	{
		if(pPlot->isOwned() && (pPlot->getOwner() != pPlayer->GetID()))
		{
			return false;
		}
	}

	CvTerrainInfo* pTerrainInfo = GC.getTerrainInfo(pPlot->getTerrainType());

	if(!bValid)
	{
		if(pTerrainInfo->isFound())
		{
			bValid = true;
		}
	}

	if(!bValid)
	{
		if(pTerrainInfo->isFoundCoast())
		{
			if(pPlot->isCoastalLand())
			{
				bValid = true;
			}
		}
	}

	if(!bValid)
	{
		if(pTerrainInfo->isFoundFreshWater())
		{
			if(pPlot->isFreshWater())
			{
				bValid = true;
			}
		}
	}

	// Used to have a Python hook: CAN_FOUND_CITIES_ON_WATER_CALLBACK

	if(pPlot->isWater())
	{
		return false;
	}

	if(!bValid)
	{
		return false;
	}

	if(!bTestVisible)
	{
		// look at same land mass
		iRange = GC.getMIN_CITY_RANGE();

#ifdef AUI_HEXSPACE_DX_LOOPS
		int iMaxDX;
		for (iDY = -iRange; iDY <= iRange; iDY++)
		{
#ifdef AUI_FAST_COMP
			iMaxDX = iRange - FASTMAX(0, iDY);
			for (iDX = -iRange - FASTMIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#else
			iMaxDX = iRange - MAX(0, iDY);
			for (iDX = -iRange - MIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#endif // AUI_FAST_COMP
			{
				pLoopPlot = plotXY(pPlot->getX(), pPlot->getY(), iDX, iDY);
#else
		for(iDX = -(iRange); iDX <= iRange; iDX++)
		{
			for(iDY = -(iRange); iDY <= iRange; iDY++)
			{
				pLoopPlot = plotXYWithRangeCheck(pPlot->getX(), pPlot->getY(), iDX, iDY, iRange);
#endif // AUI_HEXSPACE_DX_LOOPS

				if(pLoopPlot != NULL)
				{
					if(pLoopPlot->isCity())
					{
						if(pLoopPlot->getLandmass() == pPlot->getLandmass())
						{
							return false;
						}
						else if(hexDistance(iDX, iDY) < iRange)  // one less for off shore
						{
							return false;
						}
					}
				}
			}
		}
	}

	return true;
}

/// Setup flavor multipliers - call once per player before PlotFoundValue() or PlotFertilityValue()
void CvCitySiteEvaluator::ComputeFlavorMultipliers(CvPlayer* pPlayer)
{
	// Set all to 0
	for(int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		m_iFlavorMultiplier[iI] = 0;
	}

	m_iFlavorMultiplier[SITE_EVALUATION_HAPPINESS] = 0;

	// Find out if player has a desired next city specialization
	CitySpecializationTypes eNextSpecialization = pPlayer->GetCitySpecializationAI()->GetNextSpecializationDesired();
	CvCitySpecializationXMLEntry* pkCitySpecializationEntry = NULL;
	if(eNextSpecialization != NO_CITY_SPECIALIZATION)
		pkCitySpecializationEntry = GC.getCitySpecializationInfo(eNextSpecialization);


	for(int iFlavorLoop = 0; iFlavorLoop < GC.getNumFlavorTypes(); iFlavorLoop++)
	{
		const FlavorTypes eFlavor = static_cast<FlavorTypes>(iFlavorLoop);
		const CvString& strFlavor = GC.getFlavorTypes(eFlavor);
		if(strFlavor == "FLAVOR_GROWTH" ||
		        strFlavor == "FLAVOR_EXPANSION")
		{
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FLAVOR_MULTIPLIER_USES_GRAND_STRATEGY
			m_iFlavorMultiplier[YIELD_FOOD] += pPlayer->GetGrandStrategyAI()->GetPersonalityAndGrandStrategy(eFlavor);
#else
			m_iFlavorMultiplier[YIELD_FOOD] += pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor(eFlavor);
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FLAVOR_MULTIPLIER_USES_GRAND_STRATEGY
			if(pkCitySpecializationEntry)
			{
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER
				m_iFlavorMultiplier[YIELD_FOOD] += pkCitySpecializationEntry->GetFlavorValue(eFlavor) * AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER;
#else
				m_iFlavorMultiplier[YIELD_FOOD] += pkCitySpecializationEntry->GetFlavorValue(eFlavor);
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER
			}
		}
		else if(strFlavor == "FLAVOR_GOLD" ||
		        strFlavor == "FLAVOR_TILE_IMPROVEMENT")
		{
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FLAVOR_MULTIPLIER_USES_GRAND_STRATEGY
			m_iFlavorMultiplier[YIELD_GOLD] += pPlayer->GetGrandStrategyAI()->GetPersonalityAndGrandStrategy(eFlavor);
#else
			m_iFlavorMultiplier[YIELD_GOLD] += pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor(eFlavor);
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FLAVOR_MULTIPLIER_USES_GRAND_STRATEGY
			if(pkCitySpecializationEntry)
			{
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER
				m_iFlavorMultiplier[YIELD_GOLD] += pkCitySpecializationEntry->GetFlavorValue(eFlavor) * AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER;
#else
				m_iFlavorMultiplier[YIELD_GOLD] += pkCitySpecializationEntry->GetFlavorValue(eFlavor);
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER
			}
		}
		else if(strFlavor == "FLAVOR_PRODUCTION" ||
		        strFlavor == "FLAVOR_WONDER")
		{
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FLAVOR_MULTIPLIER_USES_GRAND_STRATEGY
			m_iFlavorMultiplier[YIELD_PRODUCTION] += pPlayer->GetGrandStrategyAI()->GetPersonalityAndGrandStrategy(eFlavor);
#else
			m_iFlavorMultiplier[YIELD_PRODUCTION] += pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor(eFlavor);
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FLAVOR_MULTIPLIER_USES_GRAND_STRATEGY
			if(pkCitySpecializationEntry)
			{
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER
				m_iFlavorMultiplier[YIELD_PRODUCTION] += pkCitySpecializationEntry->GetFlavorValue(eFlavor) * AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER;
#else
				m_iFlavorMultiplier[YIELD_PRODUCTION] += pkCitySpecializationEntry->GetFlavorValue(eFlavor);
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER
			}
		}
		else if(strFlavor == "FLAVOR_SCIENCE")
		{
			// Doubled since only one flavor related to science
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FLAVOR_MULTIPLIER_USES_GRAND_STRATEGY
			m_iFlavorMultiplier[YIELD_SCIENCE] += pPlayer->GetGrandStrategyAI()->GetPersonalityAndGrandStrategy(eFlavor) * 2;
#else
			m_iFlavorMultiplier[YIELD_SCIENCE] += pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor(eFlavor) * 2;
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FLAVOR_MULTIPLIER_USES_GRAND_STRATEGY
			if(pkCitySpecializationEntry)
			{
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER
				m_iFlavorMultiplier[YIELD_SCIENCE] += pkCitySpecializationEntry->GetFlavorValue(eFlavor) * AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER;
#else
				m_iFlavorMultiplier[YIELD_SCIENCE] += pkCitySpecializationEntry->GetFlavorValue(eFlavor) * 2;
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER
			}
		}
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_CONSIDER_CULTURE
		else if (strFlavor == "FLAVOR_CULTURE")
		{
			// Doubled since only one flavor related to culture
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FLAVOR_MULTIPLIER_USES_GRAND_STRATEGY
			m_iFlavorMultiplier[YIELD_CULTURE] += pPlayer->GetGrandStrategyAI()->GetPersonalityAndGrandStrategy(eFlavor) * 2;
#else
			m_iFlavorMultiplier[YIELD_CULTURE] += pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor(eFlavor) * 2;
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FLAVOR_MULTIPLIER_USES_GRAND_STRATEGY
			if (pkCitySpecializationEntry)
			{
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER
				m_iFlavorMultiplier[YIELD_CULTURE] += pkCitySpecializationEntry->GetFlavorValue(eFlavor) * 2 * AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER;
#else
				m_iFlavorMultiplier[YIELD_CULTURE] += pkCitySpecializationEntry->GetFlavorValue(eFlavor) * 2;
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER
			}
		}
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_CONSIDER_CULTURE
		else if(strFlavor == "FLAVOR_HAPPINESS")
		{
			// Doubled since only one flavor related to Happiness
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FLAVOR_MULTIPLIER_USES_GRAND_STRATEGY
			m_iFlavorMultiplier[SITE_EVALUATION_HAPPINESS] += pPlayer->GetGrandStrategyAI()->GetPersonalityAndGrandStrategy(eFlavor) * 2;
#else
			m_iFlavorMultiplier[SITE_EVALUATION_HAPPINESS] += pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor(eFlavor) * 2;
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FLAVOR_MULTIPLIER_USES_GRAND_STRATEGY
			if(pkCitySpecializationEntry)
			{
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER
				m_iFlavorMultiplier[SITE_EVALUATION_HAPPINESS] += pkCitySpecializationEntry->GetFlavorValue(eFlavor) * 2 * AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER;
#else
				m_iFlavorMultiplier[SITE_EVALUATION_HAPPINESS] += pkCitySpecializationEntry->GetFlavorValue(eFlavor) * 2;
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER
			}
		}
		else if(strFlavor == "FLAVOR_RELIGION")
		{
			// Doubled since only one flavor related to faith
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FLAVOR_MULTIPLIER_USES_GRAND_STRATEGY
			m_iFlavorMultiplier[YIELD_FAITH] += pPlayer->GetGrandStrategyAI()->GetPersonalityAndGrandStrategy(eFlavor) * 2;
#else
			m_iFlavorMultiplier[YIELD_FAITH] += pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor(eFlavor) * 2;
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FLAVOR_MULTIPLIER_USES_GRAND_STRATEGY
			if (pkCitySpecializationEntry)
			{
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER
				m_iFlavorMultiplier[YIELD_FAITH] += pkCitySpecializationEntry->GetFlavorValue(eFlavor) * 2 * AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER;
#else
				m_iFlavorMultiplier[YIELD_FAITH] += pkCitySpecializationEntry->GetFlavorValue(eFlavor) * 2;
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER
			}
		}
	}

	// Make sure none are negative
	for(int iI = 0; iI < NUM_YIELD_TYPES; iI++)
	{
		if(m_iFlavorMultiplier[iI] < 0)
		{
			m_iFlavorMultiplier[iI] = 0;
		}
	}

	// Set tradable resources and strategic value to times 10 (so multiplying this by the number of map gives a number from 1 to 100)
	m_iFlavorMultiplier[SITE_EVALUATION_RESOURCES] = 10;
	m_iFlavorMultiplier[SITE_EVALUATION_STRATEGIC] = 10;
}

/// Retrieve the relative value of this plot (including plots that would be in city radius)
int CvCitySiteEvaluator::PlotFoundValue(CvPlot* pPlot, CvPlayer* pPlayer, YieldTypes eYield, bool)
{
	CvAssert(pPlot);
	if(!pPlot)
		return 0;

	// Make sure this player can even build a city here
	if(!CanFound(pPlot, pPlayer, false))
	{
		return 0;
	}

	int rtnValue = 0;

	int iFoodValue = 0;
	int iHappinessValue = 0;
	int iProductionValue = 0;
	int iGoldValue = 0;
	int iScienceValue = 0;
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_CONSIDER_CULTURE
	int iCultureValue = 0;
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_CONSIDER_CULTURE
	int iFaithValue = 0;
	int iResourceValue = 0;
	int iStrategicValue = 0;

	int iCelticForestCount = 0;
	int iIroquoisForestCount = 0;
	int iBrazilJungleCount = 0;
	int iNaturalWonderCount = 0;
	int iDesertCount = 0;
	int iWetlandsCount = 0;

	int iTotalFoodValue = 0;
	int iTotalHappinessValue = 0;
	int iTotalProductionValue = 0;
	int iTotalGoldValue = 0;
	int iTotalScienceValue = 0;
	int iTotalFaithValue = 0;
	int iTotalResourceValue = 0;
	int iTotalStrategicValue = 0;

	int iClosestCityOfMine = 999;
	int iClosestEnemyCity = 999;

	int iCapitalArea = NULL;

#ifndef AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_CIV_UNIQUE_IMPROVEMENT
	bool bIsInca = false;
	int iAdjacentMountains = 0;
#endif // AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_CIV_UNIQUE_IMPROVEMENT

	if ( pPlayer->getCapitalCity() )
		iCapitalArea = pPlayer->getCapitalCity()->getArea();

#ifndef AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_CIV_UNIQUE_IMPROVEMENT
	// Custom code for Inca ideal terrace farm locations
	ImprovementTypes eIncaImprovement = (ImprovementTypes)GC.getInfoTypeForString("IMPROVEMENT_TERRACE_FARM", true);  
	if(eIncaImprovement != NO_IMPROVEMENT)
	{
		CvImprovementEntry* pkEntry = GC.getImprovementInfo(eIncaImprovement);
		if(pkEntry != NULL && pkEntry->IsSpecificCivRequired())
		{
			CivilizationTypes eCiv = pkEntry->GetRequiredCivilization();
			if(eCiv == pPlayer->getCivilizationType())
			{
				bIsInca = true;
			}
		}
	}
#endif // This stuff has been taken care of in the plot value part in AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_CIV_UNIQUE_IMPROVEMENT

#ifdef AUI_HEXSPACE_DX_LOOPS
	int iDX, iMaxDX;
	for (int iDY = -7; iDY <= 7; iDY++)
	{
#ifdef AUI_FAST_COMP
		iMaxDX = 7 - FASTMAX(0, iDY);
		for (iDX = -7 - FASTMIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#else
		iMaxDX = 7 - MAX(0, iDY);
		for (iDX = -7 - MIN(0, iDY); iDX <= iMaxDX; iDX++) // MIN() and MAX() stuff is to reduce loops (hexspace!)
#endif // AUI_FAST_COMP
#else
	for (int iDX = -7; iDX <= 7; iDX++)
	{
		for (int iDY = -7; iDY <= 7; iDY++)
#endif // AUI_HEXSPACE_DX_LOOPS
		{
			CvPlot* pLoopPlot = plotXY(pPlot->getX(), pPlot->getY(), iDX, iDY);

			if (pLoopPlot != NULL)
			{
#ifdef AUI_FIX_HEX_DISTANCE_INSTEAD_OF_PLOT_DISTANCE
				int iDistance = hexDistance(iDX, iDY);
#else
				int iDistance = plotDistance(pPlot->getX(), pPlot->getY(), pLoopPlot->getX(), pLoopPlot->getY());
#endif // AUI_FIX_HEX_DISTANCE_INSTEAD_OF_PLOT_DISTANCE
#ifndef AUI_HEXSPACE_DX_LOOPS
				if (iDistance <= 7)
#endif // AUI_HEXSPACE_DX_LOOPS
				{
					if ((pLoopPlot->getOwner() == NO_PLAYER) || (pLoopPlot->getOwner() == pPlayer->GetID()))
					{
						// See if there are other cities nearby
						if (iClosestCityOfMine > iDistance)
						{
							if (pLoopPlot->isCity())
							{
								iClosestCityOfMine = iDistance;
							}
						}

						// Skip the city plot itself for now
						if (iDistance <= 5)
						{
							int iRingModifier = m_iRingModifier[iDistance];

							iFoodValue = 0;
							iProductionValue = 0;
							iGoldValue = 0;
							iScienceValue = 0;
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_CONSIDER_CULTURE
							iCultureValue = 0;
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_CONSIDER_CULTURE
#ifdef AUI_SITE_EVALUATION_FIX_PLOT_FOUND_VALUE_RESET_FAITH_VALUE_EACH_LOOP
							iFaithValue = 0;
#endif // AUI_SITE_EVALUATION_FIX_PLOT_FOUND_VALUE_RESET_FAITH_VALUE_EACH_LOOP
							iHappinessValue = 0;
							iResourceValue = 0;
							iStrategicValue = 0;

							if (iDistance > 0 && iDistance <= NUM_CITY_RINGS)
							{
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
								if (eYield == NO_YIELD || eYield == YIELD_FOOD)
								{
									iFoodValue = iRingModifier * ComputeFoodValue(pLoopPlot, pPlayer, iDistance == 0) * /*15*/ GC.getSETTLER_FOOD_MULTIPLIER();
								}
								if (eYield == NO_YIELD || eYield == YIELD_PRODUCTION)
								{
									iProductionValue = iRingModifier * ComputeProductionValue(pLoopPlot, pPlayer, iDistance == 0) * /*3*/ GC.getSETTLER_PRODUCTION_MULTIPLIER();
								}
								if (eYield == NO_YIELD || eYield == YIELD_GOLD)
								{
									iGoldValue = iRingModifier * ComputeGoldValue(pLoopPlot, pPlayer, iDistance == 0) * /*2*/ GC.getSETTLER_GOLD_MULTIPLIER();
								}
								if (eYield == NO_YIELD || eYield == YIELD_SCIENCE)
								{
									iScienceValue = iRingModifier * ComputeScienceValue(pLoopPlot, pPlayer, iDistance == 0) * /*1*/ GC.getSETTLER_SCIENCE_MULTIPLIER();
								}
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_CONSIDER_CULTURE
								if (eYield == NO_YIELD || eYield == YIELD_CULTURE)
								{
									iCultureValue = iRingModifier * ComputeCultureValue(pLoopPlot, pPlayer, iDistance == 0) * /*1*/ GC.getSETTLER_FAITH_MULTIPLIER();
								}
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_CONSIDER_CULTURE
								if (eYield == NO_YIELD || eYield == YIELD_FAITH)
								{
									iFaithValue = iRingModifier * ComputeFaithValue(pLoopPlot, pPlayer, iDistance == 0) * /*1*/ GC.getSETTLER_FAITH_MULTIPLIER();
								}
#else
								if (eYield == NO_YIELD || eYield == YIELD_FOOD)
								{
									iFoodValue = iRingModifier * ComputeFoodValue(pLoopPlot, pPlayer) * /*15*/ GC.getSETTLER_FOOD_MULTIPLIER();
								}
								if (eYield == NO_YIELD || eYield == YIELD_PRODUCTION)
								{
									iProductionValue = iRingModifier * ComputeProductionValue(pLoopPlot, pPlayer) * /*3*/ GC.getSETTLER_PRODUCTION_MULTIPLIER();
								}
								if (eYield == NO_YIELD || eYield == YIELD_GOLD)
								{
									iGoldValue = iRingModifier * ComputeGoldValue(pLoopPlot, pPlayer) * /*2*/ GC.getSETTLER_GOLD_MULTIPLIER();
								}
								if (eYield == NO_YIELD || eYield == YIELD_SCIENCE)
								{
									iScienceValue = iRingModifier * ComputeScienceValue(pLoopPlot, pPlayer) * /*1*/ GC.getSETTLER_SCIENCE_MULTIPLIER();
								}
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_CONSIDER_CULTURE
								if (eYield == NO_YIELD || eYield == YIELD_CULTURE)
								{
									iCultureValue = iRingModifier * ComputeCultureValue(pLoopPlot, pPlayer) * /*1*/ GC.getSETTLER_FAITH_MULTIPLIER();
								}
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_CONSIDER_CULTURE
								if (eYield == NO_YIELD || eYield == YIELD_FAITH)
								{
									iFaithValue = iRingModifier * ComputeFaithValue(pLoopPlot, pPlayer) * /*1*/ GC.getSETTLER_FAITH_MULTIPLIER();
								}
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
							}

							// whether or not we are working these we get the benefit as long as culture can grow to take them
							if (iDistance <= 5 && pLoopPlot->getOwner() == NO_PLAYER) // there is no benefit if we already own these tiles
							{
								iHappinessValue = iRingModifier * ComputeHappinessValue(pLoopPlot, pPlayer) * /*6*/ GC.getSETTLER_HAPPINESS_MULTIPLIER();
								iResourceValue = iRingModifier * ComputeTradeableResourceValue(pLoopPlot, pPlayer) * /*1*/ GC.getSETTLER_RESOURCE_MULTIPLIER();
								if (iDistance)
									iStrategicValue = ComputeStrategicValue(pLoopPlot, pPlayer, iDistance) * /*1*/ GC.getSETTLER_STRATEGIC_MULTIPLIER();  // the ring is included in the computation
							}

							iTotalFoodValue += iFoodValue;
							iTotalHappinessValue += iHappinessValue;
							iTotalProductionValue += iProductionValue;
							iTotalGoldValue += iGoldValue;
							iTotalScienceValue += iScienceValue;
							iTotalFaithValue += iFaithValue;
							iTotalResourceValue += iResourceValue;
							iTotalStrategicValue += iStrategicValue;

#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_CONSIDER_CULTURE
							int iPlotValue = iFoodValue + iHappinessValue + iProductionValue + iGoldValue + iScienceValue + iCultureValue + iFaithValue + iResourceValue;
#else
							int iPlotValue = iFoodValue + iHappinessValue + iProductionValue + iGoldValue + iScienceValue + iFaithValue + iResourceValue;
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_CONSIDER_CULTURE
							
							if (iPlotValue == 0)
							{
								// this tile is so bad it gets negatives
								iPlotValue -= iRingModifier * GC.getSETTLER_FOOD_MULTIPLIER() * 2;
							}
							iPlotValue += iStrategicValue;

							// if this tile is a NW boost the value just so that we force the AI to claim them (if we can work it)
							if (pLoopPlot->IsNaturalWonder() && iDistance > 0 && iDistance <= NUM_CITY_RINGS)
							{
								//iPlotValue += iPlotValue * 2 + 10;
								iPlotValue += iPlotValue * 2 + 500;
							}

							// lower value a lot if we already own this tile
							if (iPlotValue > 0 && pLoopPlot->getOwner() == pPlayer->GetID())
							{
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_TWEAKED_ALREADY_OWNED_DIVIDER
								iPlotValue /= AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_TWEAKED_ALREADY_OWNED_DIVIDER;
#else
								iPlotValue /= 4;
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_TWEAKED_ALREADY_OWNED_DIVIDER
							}

							// add this plot into the total
							rtnValue += iPlotValue;

							FeatureTypes ePlotFeature = pLoopPlot->getFeatureType();
							ImprovementTypes ePlotImprovement = pLoopPlot->getImprovementType();
							ResourceTypes ePlotResource = pLoopPlot->getResourceType();

							if (ePlotFeature == FEATURE_FOREST)
							{
								if (iDistance <= 5)
								{
									++iIroquoisForestCount;
									if (iDistance == 1)
									{
										if (ePlotImprovement == NO_IMPROVEMENT)
										{
											++iCelticForestCount;
										}
									}
								}
							}
							else if (ePlotFeature == FEATURE_JUNGLE)
							{
								if (iDistance <= NUM_CITY_RINGS)
								{
									++iBrazilJungleCount;
								}
							}
							else if (ePlotFeature == FEATURE_MARSH || ePlotFeature == FEATURE_FLOOD_PLAINS)
							{
								if (iDistance <= NUM_CITY_RINGS)
								{
									++iWetlandsCount;
								}
							}

							if (pLoopPlot->IsNaturalWonder())
							{
								if (iDistance <= 1)
								{
									++iNaturalWonderCount;
								}
							}

							if (pLoopPlot->getTerrainType() == TERRAIN_DESERT)
							{
								if (iDistance <= NUM_CITY_RINGS)
								{
									if (ePlotResource == NO_RESOURCE)
									{
										++iDesertCount;
									}
								}
							}

#ifndef AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_CIV_UNIQUE_IMPROVEMENT
							if (bIsInca)
							{
								if (pLoopPlot->isHills())
								{
									if (iDistance <= NUM_CITY_RINGS)
									{
										iAdjacentMountains = pLoopPlot->GetNumAdjacentMountains();
										if (iAdjacentMountains > 0 && iAdjacentMountains < 6)
										{
											//give the bonus if it's hills, with additional if bordered by mountains
											rtnValue += m_iIncaMultiplier + (iAdjacentMountains * m_iIncaMultiplier);
										}
									}
									
								}
							}
#endif // This stuff has been taken care of in the plot value part in AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_CIV_UNIQUE_IMPROVEMENT
						}
					}
					else // this tile is owned by someone else
					{
						// See if there are other cities nearby (only count major civs)
						if (iClosestEnemyCity > iDistance)
						{
							if (pLoopPlot->isCity() && (pLoopPlot->getOwner() < MAX_MAJOR_CIVS))
							{
								iClosestEnemyCity = iDistance;
							}
						}
					}
				}
			}
		}
	}

	if (pPlayer->GetPlayerTraits()->IsFaithFromUnimprovedForest())
	{
		if (iCelticForestCount >= 3)
		{
			rtnValue += 2 * 1000 * m_iFlavorMultiplier[YIELD_FAITH];
		}
		else if (iCelticForestCount >= 1)
		{
			rtnValue += 1 * 1000 * m_iFlavorMultiplier[YIELD_FAITH];
		}
	}
	else if (pPlayer->GetPlayerTraits()->IsMoveFriendlyWoodsAsRoad())
	{
		rtnValue += iIroquoisForestCount * 10;	
	}
#if !defined(AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_FUTURE_OWNER_IF_UNOWNED) || !defined(AUI_SITE_EVALUATION_FIX_COMPUTE_HAPPINESS_VALUE_NATURAL_WONDERS)
	else if (pPlayer->GetPlayerTraits()->GetNaturalWonderYieldModifier() > 0)	//ie: Spain
	{
		rtnValue += iNaturalWonderCount * m_iSpainMultiplier;	
	}
#endif // This bit's already taken care of by the combination of two new features in AuI

#ifndef AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_CIV_UNIQUE_IMPROVEMENT
	// Custom code for Brazil
	ImprovementTypes eBrazilImprovement = (ImprovementTypes)GC.getInfoTypeForString("IMPROVEMENT_BRAZILWOOD_CAMP", true);  
	if(eBrazilImprovement != NO_IMPROVEMENT)
	{
		CvImprovementEntry* pkEntry = GC.getImprovementInfo(eBrazilImprovement);
		if(pkEntry != NULL && pkEntry->IsSpecificCivRequired())
		{
			CivilizationTypes eCiv = pkEntry->GetRequiredCivilization();
			if(eCiv == pPlayer->getCivilizationType())
			{
				rtnValue += iBrazilJungleCount * m_iBrazilMultiplier;
			}
		}
	}

	// Custom code for Morocco
	ImprovementTypes eMoroccoImprovement = (ImprovementTypes)GC.getInfoTypeForString("IMPROVEMENT_KASBAH", true);  
	if(eMoroccoImprovement != NO_IMPROVEMENT)
	{
		CvImprovementEntry* pkEntry = GC.getImprovementInfo(eMoroccoImprovement);
		if(pkEntry != NULL && pkEntry->IsSpecificCivRequired())
		{
			CivilizationTypes eCiv = pkEntry->GetRequiredCivilization();
			if(eCiv == pPlayer->getCivilizationType())
			{
				rtnValue += iDesertCount * m_iMorrocoMultiplier;
			}
		}
	}

	//Custom code for Netherlands
	ImprovementTypes ePolderImprovement = (ImprovementTypes)GC.getInfoTypeForString("IMPROVEMENT_POLDER", true);  
	if(ePolderImprovement != NO_IMPROVEMENT)
	{
		CvImprovementEntry* pkEntry = GC.getImprovementInfo(ePolderImprovement);
		if(pkEntry != NULL && pkEntry->IsSpecificCivRequired())
		{
			CivilizationTypes eCiv = pkEntry->GetRequiredCivilization();
			if(eCiv == pPlayer->getCivilizationType())
			{
				rtnValue += iWetlandsCount * m_iNetherlandsMultiplier;
			}
		}
	}
#endif // This stuff has been taken care of in the plot value part in AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_CIV_UNIQUE_IMPROVEMENT

	if (rtnValue < 0) rtnValue = 0;

	// Finally, look at the city plot itself and use it as an overall multiplier
	if (pPlot->getResourceType(pPlayer->getTeam()) != NO_RESOURCE)
	{
		rtnValue += (int)rtnValue * /*-50*/ GC.getBUILD_ON_RESOURCE_PERCENT() / 100;
	}

	if (pPlot->isRiver())
	{
		rtnValue += (int)rtnValue * /*15*/ GC.getBUILD_ON_RIVER_PERCENT() / 100;
	}

	if (pPlot->isCoastalLand(GC.getMIN_WATER_SIZE_FOR_OCEAN()))
	{
		// okay, coast used to have lots of gold so players settled there "naturally", it doesn't any more, so I am going to give it a nudge in that direction
		// slewis - removed Brian(?)'s rtnValue adjustment and raised the BUILD_ON_COAST_PERCENT to 40 from 25
		//rtnValue += rtnValue > 0 ? 10 : 0;
		rtnValue += (int)rtnValue * /*40*/ GC.getSETTLER_BUILD_ON_COAST_PERCENT() / 100;
		int iNavalFlavor = pPlayer->GetGrandStrategyAI()->GetPersonalityAndGrandStrategy((FlavorTypes)m_iNavalIndex);
		if (iNavalFlavor > 7)
		{
			rtnValue += (int)rtnValue * /*40*/ GC.getSETTLER_BUILD_ON_COAST_PERCENT() / 100;
		}
		if (pPlayer->getCivilizationInfo().isCoastalCiv()) // we really like the coast (England, Norway, Polynesia, Carthage, etc.)
		{
			rtnValue += rtnValue > 0 ? 25 : 0;
			rtnValue *= 2;
		}
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FIRST_COASTAL_CITY_MULTIPLIER
		int iCityLoop = 0;
		CvCity* pLoopCity = NULL;
		bool bHasCoastal = false || pPlayer->getCapitalCity() == NULL;
		for (pLoopCity = pPlayer->firstCity(&iCityLoop); pLoopCity != NULL; pLoopCity = pPlayer->nextCity(&iCityLoop))
		{
			if (pLoopCity->isCoastal())
			{
				bHasCoastal = true;
				break;
			}
		}
		if (!bHasCoastal)
		{
			rtnValue *= AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FIRST_COASTAL_CITY_MULTIPLIER;
		}
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FIRST_COASTAL_CITY_MULTIPLIER
	}

	// Nearby Cities?

	// Human
	if (pPlayer != NULL && pPlayer->isHuman())
	{
		if (iClosestCityOfMine == 3)
		{
			rtnValue /= 2;
		}
	}
	// AI
	else
	{
		int iGrowthFlavor = pPlayer->GetGrandStrategyAI()->GetPersonalityAndGrandStrategy((FlavorTypes)m_iGrowthIndex);
		int iExpansionFlavor = pPlayer->GetGrandStrategyAI()->GetPersonalityAndGrandStrategy((FlavorTypes)m_iExpansionIndex);

		int iSweetSpot = 5;
		iSweetSpot += (iGrowthFlavor > 7) ?  1 : 0;
		iSweetSpot += (iExpansionFlavor > 7) ?  -1 : 0;
		iSweetSpot += (iGrowthFlavor < 4) ?  -1 : 0;
		iSweetSpot += (iExpansionFlavor < 4) ?  1 : 0;
#ifdef AUI_FAST_COMP
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_TWEAKED_MINIMUM_SWEET_SPOT
		iSweetSpot = FASTMAX(AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_TWEAKED_MINIMUM_SWEET_SPOT, iSweetSpot);
#else
		iSweetSpot = FASTMAX(4,iSweetSpot);
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_TWEAKED_MINIMUM_SWEET_SPOT
		iSweetSpot = FASTMIN(6,iSweetSpot);
#else
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_TWEAKED_MINIMUM_SWEET_SPOT
		iSweetSpot = max(AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_TWEAKED_MINIMUM_SWEET_SPOT, iSweetSpot);
#else
		iSweetSpot = max(4,iSweetSpot);
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_TWEAKED_MINIMUM_SWEET_SPOT
		iSweetSpot = min(6,iSweetSpot);
#endif // AUI_FAST_COMP

		if (iClosestCityOfMine == iSweetSpot) 
		{
			// 1.5 was not enough 2.0 was too much, so lets split the difference
			rtnValue *= 175;
			rtnValue /= 100;
		}
		else if (iClosestCityOfMine < iSweetSpot)
		{
#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_TWEAKED_CLOSER_THAN_SWEET_SPOT_MULTIPLIER
			rtnValue = (rtnValue * AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_TWEAKED_CLOSER_THAN_SWEET_SPOT_MULTIPLIER);
#else
			rtnValue /= 2;
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_TWEAKED_CLOSER_THAN_SWEET_SPOT_MULTIPLIER
		}
		else if (iClosestCityOfMine > 7)
		{
			rtnValue *= 2;
			rtnValue /= 3;
		}

		// use boldness to decide if we want to push close to enemies
		int iBoldness = pPlayer->GetDiplomacyAI()->GetBoldness();
		if (iBoldness < 4)
		{
			if (iClosestEnemyCity <= 4)
			{
				rtnValue /= 4;
			}
			else if (iClosestEnemyCity == 5)
			{
				rtnValue /= 2;
			}
		}
		else if (iBoldness > 7)
		{
			if (iClosestEnemyCity <= 5 && iClosestCityOfMine < 8)
			{
				rtnValue *= 3;
				rtnValue /= 2;
			}
		}
		else
		{
			if (iClosestEnemyCity < 5)
			{
				rtnValue *= 2;
				rtnValue /= 3;
			}
		}

		// if we are offshore, pull cities in tighter
		if (iCapitalArea != pPlot->getArea())
		{
			if (iClosestCityOfMine < 7)
			{
				rtnValue *= 3;
				rtnValue /= 2;
			}
		}
	}

	rtnValue = (rtnValue > 0) ? rtnValue : 0;

	return rtnValue;
}

/// Retrieve the relative fertility of this plot (alone)
int CvCitySiteEvaluator::PlotFertilityValue(CvPlot* pPlot)
{
	int rtnValue = 0;

	if(!pPlot->isWater() && !pPlot->isImpassable() && !pPlot->isMountain())
	{
		rtnValue += ComputeFoodValue(pPlot, NULL);
		rtnValue += ComputeProductionValue(pPlot, NULL);
		rtnValue += ComputeGoldValue(pPlot, NULL);
		rtnValue += ComputeScienceValue(pPlot, NULL);
		rtnValue += ComputeTradeableResourceValue(pPlot, NULL);
	}

	if(rtnValue < 0) rtnValue = 0;

	return rtnValue;
}

/// How strong a city site can we find nearby for this type of yield?
int CvCitySiteEvaluator::BestFoundValueForSpecificYield(CvPlayer* pPlayer, YieldTypes eYield)
{
	pPlayer;
	eYield;
	return 0;
}

// PROTECTED METHODS (can be overridden in derived classes)

/// Value of plot for providing food
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
int CvCitySiteEvaluator::ComputeFoodValue(CvPlot* pPlot, CvPlayer* pPlayer, bool bCityPlot)
#else
int CvCitySiteEvaluator::ComputeFoodValue(CvPlot* pPlot, CvPlayer* pPlayer)
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
{
	int rtnValue = 0;

	// From tile yield
	if(pPlayer == NULL)
	{
		rtnValue += pPlot->calculateNatureYield(YIELD_FOOD, NO_TEAM);
	}
	else
	{
#ifdef AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_FUTURE_OWNER_IF_UNOWNED
		rtnValue += pPlot->calculateNatureYield(YIELD_FOOD, pPlayer->getTeam(), false, pPlayer->GetID());
#else
		rtnValue += pPlot->calculateNatureYield(YIELD_FOOD, pPlayer->getTeam());
#endif // AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_FUTURE_OWNER_IF_UNOWNED
	}
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
	if (bCityPlot)
	{
		CvYieldInfo* pYieldInfo = GC.getYieldInfo(YIELD_FOOD);
		if (pYieldInfo)
#ifdef AUI_FAST_COMP
			rtnValue = FASTMAX(rtnValue, pYieldInfo->getMinCity());
#else
			rtnValue = MAX(rtnValue, pYieldInfo->getMinCity());
#endif // AUI_FAST_COMP
	}
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT

	// From resource
	TeamTypes eTeam = NO_TEAM;
	if(pPlayer != NULL)
	{
		eTeam = pPlayer->getTeam();
	}

	ResourceTypes eResource;
	eResource = pPlot->getResourceType(eTeam);
	if(eResource != NO_RESOURCE)
	{
		rtnValue += GC.getResourceInfo(eResource)->getYieldChange(YIELD_FOOD);

		CvImprovementEntry* pImprovement = GC.GetGameImprovements()->GetImprovementForResource(eResource);
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
		if (pImprovement && !bCityPlot)
#else
		if(pImprovement)
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
		{
			rtnValue += pImprovement->GetImprovementResourceYield(eResource, YIELD_FOOD);
		}
	}

	return rtnValue * m_iFlavorMultiplier[YIELD_FOOD];
}

/// Value of plot for providing Happiness
int CvCitySiteEvaluator::ComputeHappinessValue(CvPlot* pPlot, CvPlayer* pPlayer)
{
	int rtnValue = 0;

	// From resource
	TeamTypes eTeam = NO_TEAM;
	if(pPlayer != NULL)
	{
		eTeam = pPlayer->getTeam();
	}

	ResourceTypes eResource;
	eResource = pPlot->getResourceType(eTeam);
	if(eResource != NO_RESOURCE)
	{
		// Add a bonus if adds Happiness
		if(!pPlot->isOwned())
		{
			rtnValue += GC.getResourceInfo(eResource)->getHappiness();
#ifdef AUI_SITE_EVALUATION_FIX_COMPUTE_HAPPINESS_VALUE_PLAYER_SOURCES
			if (pPlayer)
				rtnValue += pPlayer->GetExtraHappinessPerLuxury();
#endif // AUI_SITE_EVALUATION_FIX_COMPUTE_HAPPINESS_VALUE_PLAYER_SOURCES
		}

		// If we don't have this resource yet, increase it's value
		if(pPlayer)
		{
#ifdef AUI_SITE_EVALUATION_COMPUTE_HAPPINESS_VALUE_TWEAKED_UNOWNED_LUXURY_MULTIPLIER
			if (pPlayer->getNumResourceTotal(eResource, false) == 0)
			{
#ifdef AUI_SITE_EVALUATION_FIX_COMPUTE_HAPPINESS_VALUE_PLAYER_SOURCES
				if (pPlayer->GetHappinessFromResources() > 0)
					rtnValue += GC.getHAPPINESS_PER_EXTRA_LUXURY();
#endif // AUI_SITE_EVALUATION_FIX_COMPUTE_HAPPINESS_VALUE_PLAYER_SOURCES
				rtnValue *= AUI_SITE_EVALUATION_COMPUTE_HAPPINESS_VALUE_TWEAKED_UNOWNED_LUXURY_MULTIPLIER;
				if (pPlayer->getNumResourceTotal(eResource) == 0)
				{
					rtnValue *= AUI_SITE_EVALUATION_COMPUTE_HAPPINESS_VALUE_TWEAKED_UNOWNED_LUXURY_MULTIPLIER * 2;
				}
			}
#else
			if(pPlayer->getNumResourceTotal(eResource) == 0)
#ifdef AUI_SITE_EVALUATION_FIX_COMPUTE_HAPPINESS_VALUE_PLAYER_SOURCES
			{
				if (pPlayer->GetHappinessFromResources() > 0)
					rtnValue += GC.getHAPPINESS_PER_EXTRA_LUXURY();
				rtnValue *= 5;
			}
#else
				rtnValue *= 5;
#endif AUI_SITE_EVALUATION_FIX_COMPUTE_HAPPINESS_VALUE_PLAYER_SOURCES
#endif // AUI_SITE_EVALUATION_COMPUTE_HAPPINESS_VALUE_TWEAKED_UNOWNED_LUXURY_MULTIPLIER
		}
	}

#ifdef AUI_SITE_EVALUATION_FIX_COMPUTE_HAPPINESS_VALUE_NATURAL_WONDERS
	// From natural wonders
	FeatureTypes eFeatureType = pPlot->getFeatureType();
	if (eFeatureType != NO_FEATURE)
	{
		CvFeatureInfo* pFeatureInfo = GC.getFeatureInfo(eFeatureType);
		if (pFeatureInfo &&  pFeatureInfo->IsNaturalWonder())
		{
			rtnValue += pFeatureInfo->getInBorderHappiness();
#ifdef AUI_SITE_EVALUATION_COMPUTE_HAPPINESS_VALUE_TWEAKED_UNOWNED_LUXURY_MULTIPLIER
			rtnValue *= AUI_SITE_EVALUATION_COMPUTE_HAPPINESS_VALUE_TWEAKED_UNOWNED_LUXURY_MULTIPLIER;
#else
			rtnValue *= 5;
#endif // AUI_SITE_EVALUATION_COMPUTE_HAPPINESS_VALUE_TWEAKED_UNOWNED_LUXURY_MULTIPLIER
			if (pPlayer->GetPlayerTraits()->GetNaturalWonderHappinessModifier() != 0)
				rtnValue = rtnValue * (100 + pPlayer->GetPlayerTraits()->GetNaturalWonderHappinessModifier()) / 100;
		}
	}
#endif // AUI_SITE_EVALUATION_FIX_COMPUTE_HAPPINESS_VALUE_NATURAL_WONDERS

	return rtnValue * m_iFlavorMultiplier[SITE_EVALUATION_HAPPINESS];
}

/// Value of plot for providing hammers
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
int CvCitySiteEvaluator::ComputeProductionValue(CvPlot* pPlot, CvPlayer* pPlayer, bool bCityPlot)
#else
int CvCitySiteEvaluator::ComputeProductionValue(CvPlot* pPlot, CvPlayer* pPlayer)
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
{
	int rtnValue = 0;

	// From tile yield
	if(pPlayer == NULL)
	{
		rtnValue += pPlot->calculateNatureYield(YIELD_PRODUCTION, NO_TEAM);
	}
	else
	{
#ifdef AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_FUTURE_OWNER_IF_UNOWNED
		rtnValue += pPlot->calculateNatureYield(YIELD_PRODUCTION, pPlayer->getTeam(), false, pPlayer->GetID());
#else
		rtnValue += pPlot->calculateNatureYield(YIELD_PRODUCTION, pPlayer->getTeam());
#endif // AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_FUTURE_OWNER_IF_UNOWNED
	}
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
	if (bCityPlot)
	{
		CvYieldInfo* pYieldInfo = GC.getYieldInfo(YIELD_PRODUCTION);
		if (pYieldInfo)
#ifdef AUI_FAST_COMP
			rtnValue = FASTMAX(rtnValue, pYieldInfo->getMinCity());
#else
			rtnValue = MAX(rtnValue, pYieldInfo->getMinCity());
#endif // AUI_FAST_COMP
	}
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT

	// From resource
	TeamTypes eTeam = NO_TEAM;
	if(pPlayer != NULL)
	{
		eTeam = pPlayer->getTeam();
	}

	ResourceTypes eResource;
	eResource = pPlot->getResourceType(eTeam);
	if(eResource != NO_RESOURCE)
	{
		rtnValue += GC.getResourceInfo(eResource)->getYieldChange(YIELD_PRODUCTION);

		CvImprovementEntry* pImprovement = GC.GetGameImprovements()->GetImprovementForResource(eResource);
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
		if (pImprovement && !bCityPlot)
#else
		if(pImprovement)
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
		{
			rtnValue += pImprovement->GetImprovementResourceYield(eResource, YIELD_PRODUCTION);
		}
	}

	return rtnValue * m_iFlavorMultiplier[YIELD_PRODUCTION];
}

/// Value of plot for providing gold
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
int CvCitySiteEvaluator::ComputeGoldValue(CvPlot* pPlot, CvPlayer* pPlayer, bool bCityPlot)
#else
int CvCitySiteEvaluator::ComputeGoldValue(CvPlot* pPlot, CvPlayer* pPlayer)
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
{
	int rtnValue = 0;

	// From tile yield
	if(pPlayer == NULL)
	{
		rtnValue += pPlot->calculateNatureYield(YIELD_GOLD, NO_TEAM);
	}
	else
	{
#ifdef AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_FUTURE_OWNER_IF_UNOWNED
		rtnValue += pPlot->calculateNatureYield(YIELD_GOLD, pPlayer->getTeam(), false, pPlayer->GetID());
#else
		rtnValue += pPlot->calculateNatureYield(YIELD_GOLD, pPlayer->getTeam());
#endif // AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_FUTURE_OWNER_IF_UNOWNED
	}
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
	if (bCityPlot)
	{
		CvYieldInfo* pYieldInfo = GC.getYieldInfo(YIELD_GOLD);
		if (pYieldInfo)
#ifdef AUI_FAST_COMP
			rtnValue = FASTMAX(rtnValue, pYieldInfo->getMinCity());
#else
			rtnValue = MAX(rtnValue, pYieldInfo->getMinCity());
#endif // AUI_FAST_COMP
	}
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT

	// From resource
	TeamTypes eTeam = NO_TEAM;
	if(pPlayer != NULL)
	{
		eTeam = pPlayer->getTeam();
	}

	ResourceTypes eResource;
	eResource = pPlot->getResourceType(eTeam);
	if(eResource != NO_RESOURCE)
	{
		rtnValue += GC.getResourceInfo(eResource)->getYieldChange(YIELD_GOLD);

		CvImprovementEntry* pImprovement = GC.GetGameImprovements()->GetImprovementForResource(eResource);
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
		if (pImprovement && !bCityPlot)
#else
		if(pImprovement)
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
		{
			rtnValue += pImprovement->GetImprovementResourceYield(eResource, YIELD_GOLD);
		}
	}

	return rtnValue * m_iFlavorMultiplier[YIELD_GOLD];
}

/// Value of plot for providing science
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
int CvCitySiteEvaluator::ComputeScienceValue(CvPlot* pPlot, CvPlayer* pPlayer, bool bCityPlot)
#else
int CvCitySiteEvaluator::ComputeScienceValue(CvPlot* pPlot, CvPlayer* pPlayer)
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
{
	int rtnValue = 0;

	CvAssert(pPlot);
	if(!pPlot) return rtnValue;

	// From tile yield
	if(pPlayer == NULL)
	{
		rtnValue += pPlot->calculateNatureYield(YIELD_SCIENCE, NO_TEAM);
	}
	else
	{
#ifdef AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_FUTURE_OWNER_IF_UNOWNED
		rtnValue += pPlot->calculateNatureYield(YIELD_SCIENCE, pPlayer->getTeam(), false, pPlayer->GetID());
#else
		rtnValue += pPlot->calculateNatureYield(YIELD_SCIENCE, pPlayer->getTeam());
#endif // AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_FUTURE_OWNER_IF_UNOWNED
	}
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
	if (bCityPlot)
	{
		CvYieldInfo* pYieldInfo = GC.getYieldInfo(YIELD_SCIENCE);
		if (pYieldInfo)
#ifdef AUI_FAST_COMP
			rtnValue = FASTMAX(rtnValue, pYieldInfo->getMinCity());
#else
			rtnValue = MAX(rtnValue, pYieldInfo->getMinCity());
#endif // AUI_FAST_COMP
	}
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT

	// From resource
	TeamTypes eTeam = NO_TEAM;
	if(pPlayer != NULL)
	{
		eTeam = pPlayer->getTeam();
	}

	ResourceTypes eResource;
	eResource = pPlot->getResourceType(eTeam);
	if(eResource != NO_RESOURCE)
	{
		rtnValue += GC.getResourceInfo(eResource)->getYieldChange(YIELD_SCIENCE);

		CvImprovementEntry* pImprovement = GC.GetGameImprovements()->GetImprovementForResource(eResource);
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
		if (pImprovement && !bCityPlot)
#else
		if(pImprovement)
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
		{
			rtnValue += pImprovement->GetImprovementResourceYield(eResource, YIELD_SCIENCE);
		}
	}

	return rtnValue * m_iFlavorMultiplier[YIELD_SCIENCE];
}

#ifdef AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_CONSIDER_CULTURE
/// Value of plot for providing culture
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
int CvCitySiteEvaluator::ComputeCultureValue(CvPlot* pPlot, CvPlayer* pPlayer, bool bCityPlot)
#else
int CvCitySiteEvaluator::ComputeCultureValue(CvPlot* pPlot, CvPlayer* pPlayer)
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
{
	int rtnValue = 0;

	CvAssert(pPlot);
	if (!pPlot) return rtnValue;

	// From tile yield
	if (pPlayer == NULL)
	{
		rtnValue += pPlot->calculateNatureYield(YIELD_CULTURE, NO_TEAM);
	}
	else
	{
#ifdef AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_FUTURE_OWNER_IF_UNOWNED
		rtnValue += pPlot->calculateNatureYield(YIELD_CULTURE, pPlayer->getTeam(), false, pPlayer->GetID());
#else
		rtnValue += pPlot->calculateNatureYield(YIELD_CULTURE, pPlayer->getTeam());
#endif // AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_FUTURE_OWNER_IF_UNOWNED
	}
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
	if (bCityPlot)
	{
		CvYieldInfo* pYieldInfo = GC.getYieldInfo(YIELD_CULTURE);
		if (pYieldInfo)
#ifdef AUI_FAST_COMP
			rtnValue = FASTMAX(rtnValue, pYieldInfo->getMinCity());
#else
			rtnValue = MAX(rtnValue, pYieldInfo->getMinCity());
#endif // AUI_FAST_COMP
	}
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT

	// From resource
	TeamTypes eTeam = NO_TEAM;
	if (pPlayer != NULL)
	{
		eTeam = pPlayer->getTeam();
	}

	ResourceTypes eResource;
	eResource = pPlot->getResourceType(eTeam);
	if (eResource != NO_RESOURCE)
	{
		rtnValue += GC.getResourceInfo(eResource)->getYieldChange(YIELD_CULTURE);

		CvImprovementEntry* pImprovement = GC.GetGameImprovements()->GetImprovementForResource(eResource);
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
		if (pImprovement && !bCityPlot)
#else
		if (pImprovement)
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
		{
			rtnValue += pImprovement->GetImprovementResourceYield(eResource, YIELD_CULTURE);
		}
	}

	return rtnValue * m_iFlavorMultiplier[YIELD_CULTURE];
}
#endif // AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_CONSIDER_CULTURE

/// Vale of plot for providing faith
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
int CvCitySiteEvaluator::ComputeFaithValue(CvPlot* pPlot, CvPlayer* pPlayer, bool bCityPlot)
#else
int CvCitySiteEvaluator::ComputeFaithValue(CvPlot* pPlot, CvPlayer* pPlayer)
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
{
	int rtnValue = 0;

	CvAssert(pPlot);
	if(!pPlot) return rtnValue;

	// From tile yield
	if(pPlayer == NULL)
	{
		rtnValue += pPlot->calculateNatureYield(YIELD_FAITH, NO_TEAM);
	}
	else
	{
#ifdef AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_FUTURE_OWNER_IF_UNOWNED
		rtnValue += pPlot->calculateNatureYield(YIELD_FAITH, pPlayer->getTeam(), false, pPlayer->GetID());
#else
		rtnValue += pPlot->calculateNatureYield(YIELD_FAITH, pPlayer->getTeam());
#endif // AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_FUTURE_OWNER_IF_UNOWNED
	}
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
	if (bCityPlot)
	{
		CvYieldInfo* pYieldInfo = GC.getYieldInfo(YIELD_FAITH);
		if (pYieldInfo)
#ifdef AUI_FAST_COMP
			rtnValue = FASTMAX(rtnValue, pYieldInfo->getMinCity());
#else
			rtnValue = MAX(rtnValue, pYieldInfo->getMinCity());
#endif // AUI_FAST_COMP
	}
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT

	// From resource
	TeamTypes eTeam = NO_TEAM;
	if(pPlayer != NULL)
	{
		eTeam = pPlayer->getTeam();
	}

	ResourceTypes eResource;
	eResource = pPlot->getResourceType(eTeam);
	if(eResource != NO_RESOURCE)
	{
		rtnValue += GC.getResourceInfo(eResource)->getYieldChange(YIELD_FAITH);

		CvImprovementEntry* pImprovement = GC.GetGameImprovements()->GetImprovementForResource(eResource);
#ifdef AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
		if (pImprovement && !bCityPlot)
#else
		if(pImprovement)
#endif // AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
		{
			rtnValue += pImprovement->GetImprovementResourceYield(eResource, YIELD_FAITH);
		}
	}

	return rtnValue * m_iFlavorMultiplier[YIELD_FAITH];
}


/// Value of plot for providing tradeable resources
int CvCitySiteEvaluator::ComputeTradeableResourceValue(CvPlot* pPlot, CvPlayer* pPlayer)
{
	int rtnValue = 0;

	CvAssert(pPlot);
	if(!pPlot) return rtnValue;

	// If we already own this Tile then we already have access to the Strategic Resource
	if(pPlot->isOwned())
	{
		return rtnValue;
	}

	TeamTypes eTeam = NO_TEAM;
	if(pPlayer != NULL)
	{
		eTeam = pPlayer->getTeam();
	}

	ResourceTypes eResource;
	eResource = pPlot->getResourceType(eTeam);

	if(eResource != NO_RESOURCE)
	{
		ResourceUsageTypes eResourceUsage = GC.getResourceInfo(eResource)->getResourceUsage();

		// Multiply number of tradeable resources by flavor value
		if(eResourceUsage == RESOURCEUSAGE_LUXURY || eResourceUsage == RESOURCEUSAGE_STRATEGIC)
		{
			rtnValue += pPlot->getNumResource() * m_iFlavorMultiplier[SITE_EVALUATION_RESOURCES];

			if(pPlayer)
			{
				// If we don't have this resource yet, increase it's value
				if(pPlayer->getNumResourceTotal(eResource) == 0)
					rtnValue *= 3;
			}
		}
	}

	return rtnValue;
}

/// Value of plot for providing strategic value
int CvCitySiteEvaluator::ComputeStrategicValue(CvPlot* pPlot, CvPlayer* pPlayer, int iPlotsFromCity)
{
	int rtnValue = 0;

	CvAssert(pPlot);
	if(!pPlot) return rtnValue;

#ifdef AUI_PLOT_CALCULATE_STRATEGIC_VALUE
	// Strategic value calculation now happens on the plot side
#ifdef AUI_FAST_COMP
	rtnValue += pPlot->getStrategicValue(true, (iPlotsFromCity == 0)) * (iPlotsFromCity == 0 ? 2 : 1) / FASTMAX(1, iPlotsFromCity);
#else
	rtnValue += pPlot->getStrategicValue(true, (iPlotsFromCity == 0)) * (iPlotsFromCity == 0 ? 2 : 1) / MAX(1, iPlotsFromCity);
#endif // AUI_FAST_COMP
#else
#ifdef AUI_SITE_EVALUATION_COMPUTE_STRATEGIC_VALUE_TWEAKED_CHOKEPOINT_CALCULATION
	// Each neighboring plot where the tiles both CW and CCW to that plot have a different land type are considered "strategic"
	// Eg. if the tiles surrounding this one are water-water-land*-water*-peak*-land*, this tile has four strategic plots indicated by the star
	// Since this plot is a land choke point between southeastern and northwestern land tiles and a water choke point between the three neighboring water tiles
	int iStrategicPlots = 0;
	// "High Value" strategic plots are those where a primary plot type is choked off (Water for naval maps, land for non-naval maps)
	int iHighValueStrategicPlots = 0;
	// Only passable terrain can have strategic value now
	if (!pPlot->isImpassable() && !pPlot->isMountain())
	{
		for (int iI = 0; iI < NUM_DIRECTION_TYPES; iI++)
		{
			CvPlot* pCheckPlot = plotDirection(pPlot->getX(), pPlot->getY(), (DirectionTypes)iI);
			CvPlot* pCWPlot = plotDirection(pPlot->getX(), pPlot->getY(), (DirectionTypes)((iI + 1) % NUM_DIRECTION_TYPES));
			CvPlot* pCCWPlot = plotDirection(pPlot->getX(), pPlot->getY(), (DirectionTypes)((NUM_DIRECTION_TYPES + iI - 1) % NUM_DIRECTION_TYPES));
			if (pCheckPlot && pCWPlot && pCCWPlot)
			{
				if (pCheckPlot->isImpassable() || pCheckPlot->isMountain())
				{
					if (!pCWPlot->isImpassable() && !pCWPlot->isMountain() && !pCCWPlot->isImpassable() && !pCCWPlot->isMountain() &&
						(iPlotsFromCity == 0 || (pPlot->isWater() == pCWPlot->isWater() && pPlot->isWater() == pCCWPlot->isWater())))
					{
						iStrategicPlots++;
						iHighValueStrategicPlots++;
					}
				}
				else if (pCheckPlot->isWater())
				{
					if (!pCWPlot->isWater() && !pCCWPlot->isWater() &&
						(iPlotsFromCity == 0 || pPlot->isWater() || (!pCWPlot->isImpassable() && !pCWPlot->isMountain() && !pCCWPlot->isImpassable() && !pCCWPlot->isMountain())))
					{
						iStrategicPlots++;
						if (GC.getMap().GetAIMapHint() & 1)
						{
							iHighValueStrategicPlots++;
						}
					}
				}
				else
				{
					if ((pCWPlot->isWater() || pCWPlot->isImpassable() || pCWPlot->isMountain()) && (pCCWPlot->isWater() || pCCWPlot->isImpassable() || pCCWPlot->isMountain()) &&
						(iPlotsFromCity == 0 || !pPlot->isWater()))
					{
						iStrategicPlots++;
						if (!(GC.getMap().GetAIMapHint() & 1))
						{
							iHighValueStrategicPlots++;
						}
					}
				}
			}
		}
	}
	// Plot is only a choke point if there are at least two strategic plots
	if (iStrategicPlots > 1)
	{
		rtnValue += /*5*/ GC.getCHOKEPOINT_STRATEGIC_VALUE() * (iStrategicPlots + iHighValueStrategicPlots) / MAX(iPlotsFromCity, 1) * (iPlotsFromCity == 0 ? 2 : 1);
	}
#else
	// Possible chokepoint if impassable terrain and exactly 2 plots from city
	if(iPlotsFromCity == 2 && (pPlot->isImpassable() || pPlot->isMountain()))
	{
		rtnValue += /*5*/ GC.getCHOKEPOINT_STRATEGIC_VALUE();
	}
#endif // AUI_SITE_EVALUATION_COMPUTE_STRATEGIC_VALUE_TWEAKED_CHOKEPOINT_CALCULATION
#endif // AUI_PLOT_CALCULATE_STRATEGIC_VALUE

	// Hills in first ring are useful for defense and production
	if(iPlotsFromCity == 1 && pPlot->isHills())
	{
		rtnValue += /*3*/ GC.getHILL_STRATEGIC_VALUE();
	}

	// Some Features are less attractive to settle in, (e.g. Jungles, since it takes a while before you can clear them and they slow down movement)
	if(pPlot->getFeatureType() != NO_FEATURE)
	{
		int iWeight = GC.getFeatureInfo(pPlot->getFeatureType())->getStartingLocationWeight();
		if(iWeight != 0 && iPlotsFromCity == 1)
		{
			rtnValue += iWeight;
		}
	}

	// Nearby City
	if(pPlayer != NULL && pPlot->isCity())
	{
//		if (pPlot->getOwner() == pPlayer->getID())
		{
			rtnValue += /*-1000*/ GC.getALREADY_OWNED_STRATEGIC_VALUE();
		}
	}

	// POSSIBLE FUTURE: Is there any way for us to know to grab land between us and another major civ?

	rtnValue *= m_iFlavorMultiplier[SITE_EVALUATION_STRATEGIC];

	return rtnValue;
}

//=====================================
// CvSiteEvaluatorForSettler
//=====================================
/// Constructor
CvSiteEvaluatorForSettler::CvSiteEvaluatorForSettler(void)
{
}

/// Destructor
CvSiteEvaluatorForSettler::~CvSiteEvaluatorForSettler(void)
{
}

/// Value of this site for a settler
int CvSiteEvaluatorForSettler::PlotFoundValue(CvPlot* pPlot, CvPlayer* pPlayer, YieldTypes eYield, bool bCoastOnly)
{
	CvAssert(pPlot);
	if(!pPlot) return 0;

	if(!CanFound(pPlot, pPlayer, true))
	{
		return 0;
	}

	// Is there any reason this site doesn't work for a settler?
	//
	// First must be on coast if settling a new continent
	bool bIsCoastal = pPlot->isCoastalLand(GC.getMIN_WATER_SIZE_FOR_OCEAN());
	CvArea* pArea = pPlot->area();
	CvAssert(pArea);
	if(!pArea) return 0;
	int iNumAreaCities = pArea->getCitiesPerPlayer(pPlayer->GetID());
	if(bCoastOnly && !bIsCoastal && iNumAreaCities == 0)
	{
		return 0;
	}

	// Seems okay for a settler, use base class to determine exact value
	else
	{
		// if the civ gets a benefit from settling on a new continent (ie: Indonesia)
		// double the fertility of that plot
		int iLuxuryModifier = 0;
		if (pPlayer->GetPlayerTraits()->WillGetUniqueLuxury(pArea))
		{
			iLuxuryModifier = CvCitySiteEvaluator::PlotFoundValue(pPlot, pPlayer, eYield) * 2;
			return iLuxuryModifier;
		}
		else
		{
			return CvCitySiteEvaluator::PlotFoundValue(pPlot, pPlayer, eYield);
		}
	}
}

//=====================================
// CvSiteEvaluatorForStart
//=====================================
/// Constructor
CvSiteEvaluatorForStart::CvSiteEvaluatorForStart(void)
{
}

/// Destructor
CvSiteEvaluatorForStart::~CvSiteEvaluatorForStart(void)
{
}

/// Overridden - ignore flavors for initial site selection
void CvSiteEvaluatorForStart::ComputeFlavorMultipliers(CvPlayer*)
{
	// Set all to 1; we assign start position without considering flavors yet
	for(int iI = 0; iI < NUM_SITE_EVALUATION_FACTORS; iI++)
	{
		m_iFlavorMultiplier[iI] = 1;
	}
}

/// Value of this site for a civ starting location
int CvSiteEvaluatorForStart::PlotFoundValue(CvPlot* pPlot, CvPlayer* pPlayer, YieldTypes, bool)
{
	int rtnValue = 0;
	int iI;
	CvPlot* pLoopPlot(NULL);
	int iCelticForestCount = 0;

	CvAssert(pPlot);
	if(!pPlot) return rtnValue;

	if(!CanFound(pPlot, pPlayer, false))
	{
		return rtnValue;
	}

	// Is there any reason this site doesn't work for a start location?
	//
	// Not on top of a goody hut
	if(pPlot->isGoody())
	{
		return 0;
	}

	// We have our own special method of scoring, so don't call the base class for that (like settler version does)
	for(iI = 0; iI < NUM_CITY_PLOTS; iI++)
	{
		pLoopPlot = plotCity(pPlot->getX(), pPlot->getY(), iI);

		// Too close to map edge?
		if(pLoopPlot == NULL)
		{
			return 0;
		}
		else
		{
			int iDistance = plotDistance(pPlot->getX(), pPlot->getY(), pLoopPlot->getX(), pLoopPlot->getY());
			CvAssert(iDistance <= NUM_CITY_RINGS);
			if(iDistance > NUM_CITY_RINGS) continue;
			int iRingModifier = m_iRingModifier[iDistance];

			// Skip the city plot itself for now
			if(iDistance != 0)
			{
				rtnValue += iRingModifier * ComputeFoodValue(pLoopPlot, pPlayer) * /*6*/ GC.getSTART_AREA_FOOD_MULTIPLIER();
				rtnValue += iRingModifier * ComputeHappinessValue(pLoopPlot, pPlayer) * /*12*/ GC.getSTART_AREA_HAPPINESS_MULTIPLIER();
				rtnValue += iRingModifier * ComputeProductionValue(pLoopPlot, pPlayer) * /*8*/ GC.getSTART_AREA_PRODUCTION_MULTIPLIER();
				rtnValue += iRingModifier * ComputeGoldValue(pLoopPlot, pPlayer) * /*2*/ GC.getSTART_AREA_GOLD_MULTIPLIER();
				rtnValue += iRingModifier * ComputeScienceValue(pLoopPlot, pPlayer) * /*1*/ GC.getSTART_AREA_SCIENCE_MULTIPLIER();
				rtnValue += iRingModifier * ComputeFaithValue(pLoopPlot, pPlayer) * /*1*/ GC.getSTART_AREA_FAITH_MULTIPLIER();
				rtnValue += iRingModifier * ComputeTradeableResourceValue(pLoopPlot, pPlayer) * /*1*/ GC.getSTART_AREA_RESOURCE_MULTIPLIER();
				rtnValue += iRingModifier * ComputeStrategicValue(pLoopPlot, pPlayer, iDistance) * /*1*/ GC.getSTART_AREA_STRATEGIC_MULTIPLIER();
			}

			if (pPlayer)
			{
				if (iDistance == 1 && pLoopPlot->getFeatureType() == FEATURE_FOREST)
				{
					if (pLoopPlot->getImprovementType() == NO_IMPROVEMENT && pPlayer->GetPlayerTraits()->IsFaithFromUnimprovedForest())
					{
						iCelticForestCount += 1;
					}
				}
			}
		}
	}

	if (iCelticForestCount >= 3)
	{
		rtnValue += 2 * 1000 * m_iFlavorMultiplier[YIELD_FAITH];
	}
	else if (iCelticForestCount >= 1)
	{
		rtnValue += 1 * 1000 * m_iFlavorMultiplier[YIELD_FAITH];
	}

	if(rtnValue < 0) rtnValue = 0;

	// Finally, look at the city plot itself and use it as an overall multiplier
	if(pPlot->getResourceType() != NO_RESOURCE)
	{
		rtnValue += rtnValue * GC.getBUILD_ON_RESOURCE_PERCENT() / 100;
	}

	if(pPlot->isRiver())
	{
		rtnValue += rtnValue * GC.getBUILD_ON_RIVER_PERCENT() / 100;
	}

	if(pPlot->isCoastalLand())
	{
		rtnValue += rtnValue * GC.getSTART_AREA_BUILD_ON_COAST_PERCENT() / 100;
	}

	return rtnValue;
}