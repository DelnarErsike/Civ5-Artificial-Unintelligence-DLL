/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#pragma once

#ifndef CIV5_CITY_CITIZENS_H
#define CIV5_CITY_CITIZENS_H

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  CLASS:      CvCityCitizens
//!  \brief		Keeps track of Citizens and Specialists in a City
//
//!  Key Attributes:
//!  - One instance for each city
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
class CvCityCitizens
{
public:
	CvCityCitizens(void);
	~CvCityCitizens(void);

	void Init(CvCity* pCity);
	void Uninit();
	void Reset();
	void Read(FDataStream& kStream);
	void Write(FDataStream& kStream);

#ifdef AUI_CONSTIFY
	CvCity* GetCity() const;
	CvPlayer* GetPlayer() const;
#else
	CvCity* GetCity();
	CvPlayer* GetPlayer();
#endif
	PlayerTypes GetOwner() const;
	TeamTypes GetTeam() const;

	void DoFoundCity();
#ifdef AUI_PLAYER_RESOLVE_WORKED_PLOT_CONFLICTS
	void DoTurn(bool bDoSpecialist = true);
#else
	void DoTurn();
#endif

#ifdef AUI_CITIZENS_GET_VALUE_FROM_STATS
	int GetPlotValue(const CvPlot* pPlot, bool bUseAllowGrowthFlag, double* adBonusYields = NULL, double* adBonusYieldModifiers = NULL, int iExtraHappiness = 0, int iExtraGrowthMod = 0, bool bAfterGrowth = false) const;
#elif defined(AUI_CITIZENS_IGNORE_FOOD_FOR_CITIZEN_ASSIGN_AFTER_GROW)
	int GetPlotValue(const CvPlot* pPlot, bool bUseAllowGrowthFlag, bool bAfterGrowth = false) const;
#else
	int GetPlotValue(CvPlot* pPlot, bool bUseAllowGrowthFlag);
#endif

#ifdef AUI_CITIZENS_GET_VALUE_FROM_STATS
	int GetTotalValue(double* aiYields, UnitClassTypes eGreatPersonClass = NO_UNITCLASS, double dGPPYield = 0.0, int iExtraHappiness = 0, double dTourismYield = 0.0, int iExtraGrowthMod = 0, bool bAfterGrowth = false, bool bUseAvoidGrowth = true) const;
	int GetTurnsToGP(SpecialistTypes eSpecialist, double dExtraGPP = 0.0) const;
#endif

	// Are this City's Citizens automated? (always true for AI civs)
	bool IsAutomated() const;
	void SetAutomated(bool bValue);

	bool IsNoAutoAssignSpecialists() const;
	void SetNoAutoAssignSpecialists(bool bValue);

#ifdef AUI_CITIZENS_GET_VALUE_FROM_STATS
	bool IsAvoidGrowth(int iExtraHappiness = 0) const;
	bool IsForcedAvoidGrowth() const;
#elif defined(AUI_CONSTIFY)
	bool IsAvoidGrowth() const;
	bool IsForcedAvoidGrowth() const;
#else
	bool IsAvoidGrowth();
	bool IsForcedAvoidGrowth();
#endif
	void SetForcedAvoidGrowth(bool bAvoidGrowth);
	CityAIFocusTypes GetFocusType() const;
	void SetFocusType(CityAIFocusTypes eFocus);

	// Specialist AI
#ifndef AUI_PRUNING
	bool IsAIWantSpecialistRightNow();
#endif
#if defined(AUI_CITIZENS_IGNORE_FOOD_FOR_CITIZEN_ASSIGN_AFTER_GROW) || defined(AUI_CITIZENS_GET_VALUE_FROM_STATS)
	BuildingTypes GetAIBestSpecialistBuilding(int& iSpecialistValue, bool bAfterGrowth = false);
	int GetSpecialistValue(SpecialistTypes eSpecialist, bool bAfterGrowth = false) const;
#else
	BuildingTypes GetAIBestSpecialistBuilding(int& iSpecialistValue);
	int GetSpecialistValue(SpecialistTypes eSpecialist);
#endif
#ifdef AUI_CONSTIFY
	bool IsBetterThanDefaultSpecialist(SpecialistTypes eSpecialist) const;
#else
	bool IsBetterThanDefaultSpecialist(SpecialistTypes eSpecialist);
#endif

	// Citizen Assignment
	int GetNumUnassignedCitizens() const;
	void ChangeNumUnassignedCitizens(int iChange);
	int GetNumCitizensWorkingPlots() const;
	void ChangeNumCitizensWorkingPlots(int iChange);

#if defined(AUI_CITIZENS_IGNORE_FOOD_FOR_CITIZEN_ASSIGN_AFTER_GROW) || defined(AUI_CITIZENS_GET_VALUE_FROM_STATS)
	bool DoAddBestCitizenFromUnassigned(bool bAfterGrowth = false);
#else
	bool DoAddBestCitizenFromUnassigned();
#endif
	bool DoRemoveWorstCitizen(bool bRemoveForcedStatus = false, SpecialistTypes eDontChangeSpecialist = NO_SPECIALIST, int iCurrentCityPopulation = -1);

	void DoReallocateCitizens();

#if defined(AUI_CITIZENS_IGNORE_FOOD_FOR_CITIZEN_ASSIGN_AFTER_GROW) || defined(AUI_CITIZENS_GET_VALUE_FROM_STATS)
	CvPlot* GetBestCityPlotWithValue(int& iValue, bool bWantBest, bool bWantWorked, bool bAfterGrowth = false);
#else
	CvPlot* GetBestCityPlotWithValue(int& iValue, bool bWantBest, bool bWantWorked);
#endif

	// Worked Plots
	bool IsWorkingPlot(const CvPlot* pPlot) const;
	void SetWorkingPlot(CvPlot* pPlot, bool bNewValue, bool bUseUnassignedPool = true);
	void DoAlterWorkingPlot(int iIndex);

	// Forced Working Plots (human override)
	bool IsForcedWorkingPlot(const CvPlot* pPlot) const;
	void SetForcedWorkingPlot(CvPlot* pPlot, bool bNewValue);

	void DoValidateForcedWorkingPlots();
	void DoDemoteWorstForcedWorkingPlot();

	int GetNumForcedWorkingPlots() const;
	void ChangeNumForcedWorkingPlots(int iChange);

#ifdef AUI_PLAYER_RESOLVE_WORKED_PLOT_CONFLICTS
	bool IsCanWork(CvPlot* pPlot, bool bIgnoreOverride = false) const;
#else
	bool IsCanWork(CvPlot* pPlot) const;
#endif
	bool IsPlotBlockaded(CvPlot* pPlot) const;
	bool IsAnyPlotBlockaded() const;

	void DoVerifyWorkingPlot(CvPlot* pPlot);
	void DoVerifyWorkingPlots();

	// Helpful Stuff
	int GetCityIndexFromPlot(const CvPlot* pPlot) const;
	CvPlot* GetCityPlotFromIndex(int iIndex) const;

	// Specialists
	void DoSpecialists();

#ifdef AUI_CONSTIFY
	bool IsCanAddSpecialistToBuilding(BuildingTypes eBuilding) const;
#else
	bool IsCanAddSpecialistToBuilding(BuildingTypes eBuilding);
#endif
	void DoAddSpecialistToBuilding(BuildingTypes eBuilding, bool bForced);
	void DoRemoveSpecialistFromBuilding(BuildingTypes eBuilding, bool bForced, bool bEliminatePopulation = false);
	void DoRemoveAllSpecialistsFromBuilding(BuildingTypes eBuilding, bool bEliminatePopulation = false);
	bool DoRemoveWorstSpecialist(SpecialistTypes eDontChangeSpecialist, const BuildingTypes eDontRemoveFromBuilding = NO_BUILDING);

	int GetNumDefaultSpecialists() const;
	void ChangeNumDefaultSpecialists(int iChange);
	int GetNumForcedDefaultSpecialists() const;
	void ChangeNumForcedDefaultSpecialists(int iChange);

	int GetSpecialistCount(SpecialistTypes eIndex) const;
	int GetTotalSpecialistCount() const;

	int GetBuildingGreatPeopleRateChanges(SpecialistTypes eSpecialist) const;
	void ChangeBuildingGreatPeopleRateChanges(SpecialistTypes eSpecialist, int iChange);

	int GetSpecialistGreatPersonProgress(SpecialistTypes eIndex) const;
	int GetSpecialistGreatPersonProgressTimes100(SpecialistTypes eIndex) const;
	void ChangeSpecialistGreatPersonProgressTimes100(SpecialistTypes eIndex, int iChange);
	void DoResetSpecialistGreatPersonProgressTimes100(SpecialistTypes eIndex);

	int GetNumSpecialistsInBuilding(BuildingTypes eBuilding) const;
	int GetNumForcedSpecialistsInBuilding(BuildingTypes eBuilding) const;

	void DoClearForcedSpecialists();

#ifdef AUI_CONSTIFY
	int GetNumSpecialistsAllowedByBuilding(const CvBuildingEntry& kBuilding) const;

	int GetSpecialistUpgradeThreshold(UnitClassTypes eUnitClass) const;
#else
	int GetNumSpecialistsAllowedByBuilding(const CvBuildingEntry& kBuilding);

	int GetSpecialistUpgradeThreshold(UnitClassTypes eUnitClass);
#endif
	void DoSpawnGreatPerson(UnitTypes eUnit, bool bIncrementCount, bool bCountAsProphet);

private:

	CvCity* m_pCity;

	bool m_bAutomated;
	bool m_bNoAutoAssignSpecialists;

	int m_iNumUnassignedCitizens;
	int m_iNumCitizensWorkingPlots;
	int m_iNumForcedWorkingPlots;

	CityAIFocusTypes m_eCityAIFocusTypes;
	bool m_bForceAvoidGrowth;

	bool m_pabWorkingPlot[NUM_CITY_PLOTS];
	bool m_pabForcedWorkingPlot[NUM_CITY_PLOTS];

	int m_iNumDefaultSpecialists;
	int m_iNumForcedDefaultSpecialists;
	int* m_aiSpecialistCounts;
	int* m_aiSpecialistGreatPersonProgressTimes100;
	int* m_aiNumSpecialistsInBuilding;
	int* m_aiNumForcedSpecialistsInBuilding;
	int* m_piBuildingGreatPeopleRateChanges;

	bool m_bInited;

};

#endif // CIV5_CITY_CITIZENS_H