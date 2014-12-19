/*	-------------------------------------------------------------------------------------------------------
	© 1991-2012 Take-Two Interactive Software and its subsidiaries.  Developed by Firaxis Games.  
	Sid Meier's Civilization V, Civ, Civilization, 2K Games, Firaxis Games, Take-Two Interactive Software 
	and their respective logos are all trademarks of Take-Two interactive Software, Inc.  
	All other marks and trademarks are the property of their respective owners.  
	All rights reserved. 
	------------------------------------------------------------------------------------------------------- */
#include "CvGameCoreDLLPCH.h"
#include "CvGameCoreDLLUtil.h"
#include "CvPolicyAI.h"
#include "CvGrandStrategyAI.h"
#include "CvInfosSerializationHelper.h"

// Include this after all other headers.
#include "LintFree.h"

#ifdef AUI_POLICY_DIVIDE_RELIGION_WEIGHT_WHEN_NO_RELIGION
#include "CvReligionClasses.h"
#endif // AUI_POLICY_DIVIDE_RELIGION_WEIGHT_WHEN_NO_RELIGION

/// Constructor
CvPolicyAI::CvPolicyAI(CvPlayerPolicies* currentPolicies):
	m_pCurrentPolicies(currentPolicies)
{
}

/// Destructor
CvPolicyAI::~CvPolicyAI(void)
{
}

/// Clear out AI local variables
void CvPolicyAI::Reset()
{
	m_PolicyAIWeights.clear();
	m_iPolicyWeightPropagationLevels = GC.getPOLICY_WEIGHT_PROPAGATION_LEVELS();
	m_iPolicyWeightPercentDropNewBranch = GC.getPOLICY_WEIGHT_PERCENT_DROP_NEW_BRANCH();

	CvAssertMsg(m_pCurrentPolicies != NULL, "Policy AI init failure: player policy data is NULL");
	if(m_pCurrentPolicies != NULL)
	{
		CvPolicyXMLEntries* pPolicyEntries = m_pCurrentPolicies->GetPolicies();
		CvAssertMsg(pPolicyEntries != NULL, "Policy AI init failure: no policy data");
		if(pPolicyEntries != NULL)
		{
			// Loop through reading each one and add an entry with 0 weight to our vector
			const int nPolicyEntries = pPolicyEntries->GetNumPolicies();
			for(int i = 0; i < nPolicyEntries; i++)
			{
				m_PolicyAIWeights.push_back(i, 0);
			}
		}
	}

#ifdef AUI_POLICY_MULTIPLY_FLAVOR_WEIGHT_FOR_UNIQUE_GREAT_PERSON
	m_bUniqueGreatPersons.clear();
#endif // AUI_POLICY_MULTIPLY_FLAVOR_WEIGHT_FOR_UNIQUE_GREAT_PERSON
}

/// Serialization read
void CvPolicyAI::Read(FDataStream& kStream)
{
	// Version number to maintain backwards compatibility
	uint uiVersion;
	kStream >> uiVersion;

	int iWeight;

	CvAssertMsg(m_pCurrentPolicies->GetPolicies() != NULL, "Policy AI serialization failure: no policy data");
	CvAssertMsg(m_pCurrentPolicies->GetPolicies()->GetNumPolicies() > 0, "Policy AI serialization failure: number of policies not greater than 0");

	// Reset vector
	m_PolicyAIWeights.clear();

	uint uiPolicyArraySize = m_pCurrentPolicies->GetPolicies()->GetNumPolicies();
	// Must set to the final size because we might not be adding in sequentially
	m_PolicyAIWeights.resize(uiPolicyArraySize);
	// Clear the contents in case we are loading a smaller set
	for(uint uiIndex = 0; uiIndex < uiPolicyArraySize; ++uiIndex)
		m_PolicyAIWeights.SetWeight(uiIndex, 0);

	uint uiPolicyCount;
	kStream >> uiPolicyCount;

	for(uint uiIndex = 0; uiIndex < uiPolicyCount; ++uiIndex)
	{
		PolicyTypes ePolicy = (PolicyTypes)CvInfosSerializationHelper::ReadHashed(kStream);
		kStream >> iWeight;
		if(ePolicy != NO_POLICY && (uint)ePolicy < uiPolicyArraySize)
			m_PolicyAIWeights.SetWeight((uint)ePolicy, iWeight);
	}

#ifdef AUI_POLICY_MULTIPLY_FLAVOR_WEIGHT_FOR_UNIQUE_GREAT_PERSON
	kStream >> m_bUniqueGreatPersons;
#endif // AUI_POLICY_MULTIPLY_FLAVOR_WEIGHT_FOR_UNIQUE_GREAT_PERSON
}

/// Serialization write
void CvPolicyAI::Write(FDataStream& kStream)
{
	// Current version number
	uint uiVersion = 1;
	kStream << uiVersion;

	CvAssertMsg(m_pCurrentPolicies->GetPolicies() != NULL, "Policy AI serialization failure: no policy data");
	CvAssertMsg(m_pCurrentPolicies->GetPolicies()->GetNumPolicies() > 0, "Policy AI serialization failure: number of policies not greater than 0");

	// Loop through writing each entry
	uint uiPolicyCount = m_pCurrentPolicies->GetPolicies()->GetNumPolicies();
	kStream << uiPolicyCount;

	for(int i = 0; i < m_pCurrentPolicies->GetPolicies()->GetNumPolicies(); i++)
	{
		CvInfosSerializationHelper::WriteHashed(kStream, static_cast<const PolicyTypes>(i));
		kStream << m_PolicyAIWeights.GetWeight(i);
	}

#ifdef AUI_POLICY_MULTIPLY_FLAVOR_WEIGHT_FOR_UNIQUE_GREAT_PERSON
	kStream << m_bUniqueGreatPersons;
#endif // AUI_POLICY_MULTIPLY_FLAVOR_WEIGHT_FOR_UNIQUE_GREAT_PERSON
}

/// Establish weights for one flavor; can be called multiple times to layer strategies
void CvPolicyAI::AddFlavorWeights(FlavorTypes eFlavor, int iWeight, int iPropagationPercent)
{
#ifdef AUI_POLICY_MULTIPLY_FLAVOR_WEIGHT_FOR_UNIQUE_GREAT_PERSON
	UpdateUniqueGPVector();
#endif // AUI_POLICY_MULTIPLY_FLAVOR_WEIGHT_FOR_UNIQUE_GREAT_PERSON
	int iPolicy;
	CvPolicyEntry* entry;
	int* paiTempWeights;
#if defined(AUI_MINOR_CIV_RATIO) || defined(AUI_GS_SCIENCE_FLAVOR_BOOST) || defined(AUI_POLICY_MULTIPLY_HAPPINESS_WEIGHT_WHEN_UNHAPPY) || defined(AUI_POLICY_DIVIDE_RELIGION_WEIGHT_WHEN_NO_RELIGION) || defined(AUI_POLICY_DIVIDE_MILITARY_WEIGHT_FOR_OPENER) || defined(AUI_POLICY_NULLIFY_EXPANSION_WEIGHT_FOR_OCC)
	double dWeight = (double)iWeight;
#endif // AUI_MINOR_CIV_RATIO

	CvPolicyXMLEntries* pkPolicyEntries = m_pCurrentPolicies->GetPolicies();
	// Create a temporary array of weights
	paiTempWeights = (int*)_alloca(sizeof(int*) * pkPolicyEntries->GetNumPolicies());

#ifdef AUI_MINOR_CIV_RATIO
	// Adjustments to Diplomacy flavor based on City State count
	if (eFlavor == (FlavorTypes)GC.getInfoTypeForString("FLAVOR_DIPLOMACY"))
	{
		dWeight *= (1.0 + log(MAX(GC.getGame().getCurrentMinorCivDeviation(), exp(-1.0))));
	}
#endif // AUI_MINOR_CIV_RATIO
#ifdef AUI_GS_SCIENCE_FLAVOR_BOOST
	// Spaceship instead of Science to make sure Patronage isn't grabbed all the time (proper science flavor for ideological tenents addressed later)
	if (eFlavor == (FlavorTypes)GC.getInfoTypeForString("FLAVOR_SPACESHIP"))
	{
		dWeight *= m_pCurrentPolicies->GetPlayer()->GetGrandStrategyAI()->ScienceFlavorBoost();
	}
#endif // AUI_GS_SCIENCE_FLAVOR_BOOST
#ifdef AUI_POLICY_MULTIPLY_HAPPINESS_WEIGHT_WHEN_UNHAPPY
	// Makes sure AI picks up happiness policies if it starts becoming unhappy
	if (eFlavor == (FlavorTypes)GC.getInfoTypeForString("FLAVOR_HAPPINESS"))
	{
		if (m_pCurrentPolicies->GetPlayer()->IsEmpireUnhappy())
		{
			dWeight *= AUI_POLICY_MULTIPLY_HAPPINESS_WEIGHT_WHEN_UNHAPPY;
		}
		// Effect is doubled if they dip below "very unhappy" threshold
		if (m_pCurrentPolicies->GetPlayer()->IsEmpireVeryUnhappy())
		{
			dWeight *= AUI_POLICY_MULTIPLY_HAPPINESS_WEIGHT_WHEN_UNHAPPY;
		}
	}
#endif // AUI_POLICY_MULTIPLY_HAPPINESS_WEIGHT_WHEN_UNHAPPY
#ifdef AUI_POLICY_DIVIDE_RELIGION_WEIGHT_WHEN_NO_RELIGION
	// Stops the AI from picking up Piety unless it has founded or will found a religion
	if (eFlavor == (FlavorTypes)GC.getInfoTypeForString("FLAVOR_RELIGION"))
	{
		CvPlayerReligions* pPlayerReligions = m_pCurrentPolicies->GetPlayer()->GetReligions();
		if (!pPlayerReligions->HasCreatedReligion() && !(pPlayerReligions->HasCreatedPantheon() && GC.getGame().GetGameReligions()->GetNumReligionsStillToFound() > 0))
		{
			dWeight /= AUI_POLICY_DIVIDE_RELIGION_WEIGHT_WHEN_NO_RELIGION;
		}
		else if (pPlayerReligions->HasCreatedReligion())
		{
			dWeight *= AUI_POLICY_DIVIDE_RELIGION_WEIGHT_WHEN_NO_RELIGION;
		}
	}
#endif // AUI_POLICY_DIVIDE_RELIGION_WEIGHT_WHEN_NO_RELIGION
#ifdef AUI_POLICY_DIVIDE_MILITARY_WEIGHT_FOR_OPENER
	// Stops the AI from opening Honor (or other military branches
	if (m_pCurrentPolicies->GetNumPolicyBranchesUnlocked() < AUI_POLICY_DIVIDE_MILITARY_WEIGHT_FOR_OPENER / 2)
	{
		if (eFlavor == (FlavorTypes)GC.getInfoTypeForString("FLAVOR_OFFENSE") || eFlavor == (FlavorTypes)GC.getInfoTypeForString("FLAVOR_DEFENSE") || 
			eFlavor == (FlavorTypes)GC.getInfoTypeForString("FLAVOR_MILITARY_TRAINING"))
		{
			dWeight /= AUI_POLICY_DIVIDE_MILITARY_WEIGHT_FOR_OPENER * (m_pCurrentPolicies->GetNumPolicyBranchesUnlocked() + 1);
		}
	}
#endif // AUI_POLICY_DIVIDE_MILITARY_WEIGHT_FOR_OPENER
#ifdef AUI_POLICY_NULLIFY_EXPANSION_WEIGHT_FOR_OCC
	if (GC.getGame().isOption(GAMEOPTION_ONE_CITY_CHALLENGE) && eFlavor == (FlavorTypes)GC.getInfoTypeForString("FLAVOR_EXPANSION"))
		dWeight = 0;
#endif // AUI_POLICY_NULLIFY_EXPANSION_WEIGHT_FOR_OCC

	// Loop through all our policies
	for(iPolicy = 0; iPolicy < pkPolicyEntries->GetNumPolicies(); iPolicy++)
	{
		entry = pkPolicyEntries->GetPolicyEntry(iPolicy);

		// Set its weight by looking at policy's weight for this flavor and using iWeight multiplier passed in
		if(entry)
#ifdef AUI_GS_SCIENCE_FLAVOR_BOOST
		{
#ifdef AUI_POLICY_MULTIPLY_FLAVOR_WEIGHT_FOR_UNIQUE_GREAT_PERSON
			if (entry->GetLevel() > 0 && eFlavor == (FlavorTypes)GC.getInfoTypeForString("FLAVOR_SCIENCE"))
			{
				paiTempWeights[iPolicy] = (int)(entry->GetFlavorValue(eFlavor) * dWeight * m_pCurrentPolicies->GetPlayer()->GetGrandStrategyAI()->ScienceFlavorBoost()
					* BoostFlavorDueToUniqueGP(entry) + 0.5);
			}
			else
			{
				paiTempWeights[iPolicy] = (int)(entry->GetFlavorValue(eFlavor) * dWeight * BoostFlavorDueToUniqueGP(entry) + 0.5);
#else
			if (entry->GetLevel() > 0 && eFlavor == (FlavorTypes)GC.getInfoTypeForString("FLAVOR_SCIENCE"))
			{
				paiTempWeights[iPolicy] = (int)(entry->GetFlavorValue(eFlavor) * dWeight * m_pCurrentPolicies->GetPlayer()->GetGrandStrategyAI()->ScienceFlavorBoost() + 0.5);
			}
			else
			{
				paiTempWeights[iPolicy] = (int)(entry->GetFlavorValue(eFlavor) * dWeight + 0.5);
#endif // AUI_POLICY_MULTIPLY_FLAVOR_WEIGHT_FOR_UNIQUE_GREAT_PERSON
			}
		}
#else
#ifdef AUI_POLICY_MULTIPLY_FLAVOR_WEIGHT_FOR_UNIQUE_GREAT_PERSON
			paiTempWeights[iPolicy] = (int)(entry->GetFlavorValue(eFlavor) * dWeight * BoostFlavorDueToUniqueGP(entry) + 0.5);
#else
#if defined(AUI_MINOR_CIV_RATIO) || defined(AUI_POLICY_MULTIPLY_HAPPINESS_WEIGHT_WHEN_UNHAPPY) || defined(AUI_POLICY_DIVIDE_RELIGION_WEIGHT_WHEN_NO_RELIGION) || defined(AUI_POLICY_DIVIDE_MILITARY_WEIGHT_FOR_OPENER)
			paiTempWeights[iPolicy] = (int)(entry->GetFlavorValue(eFlavor) * dWeight + 0.5);
#else
			paiTempWeights[iPolicy] = entry->GetFlavorValue(eFlavor) * iWeight;
#endif // AUI_MINOR_CIV_RATIO
#endif // AUI_POLICY_MULTIPLY_FLAVOR_WEIGHT_FOR_UNIQUE_GREAT_PERSON
#endif // AUI_GS_SCIENCE_FLAVOR_BOOST
		else
			paiTempWeights[iPolicy] = 0;
	}

	// Propagate these values left in the tree so prereqs get bought
	if(iPropagationPercent > 0)
	{
		WeightPrereqs(paiTempWeights, iPropagationPercent);
	}

	// Add these weights over previous ones
	for(iPolicy = 0; iPolicy < m_pCurrentPolicies->GetPolicies()->GetNumPolicies(); iPolicy++)
	{
		m_PolicyAIWeights.IncreaseWeight(iPolicy, paiTempWeights[iPolicy]);
	}
}

/// Choose a player's next policy purchase (could be opening a branch)
int CvPolicyAI::ChooseNextPolicy(CvPlayer* pPlayer)
{
	RandomNumberDelegate fcn;
	fcn = MakeDelegate(&GC.getGame(), &CvGame::getJonRandNum);
	int iRtnValue = (int)NO_POLICY;
	int iPolicyLoop;
#ifdef AUI_GS_PRIORITY_RATIO
	CvWeightedVector<int> aLevel3Tenets;
#else
	vector<int> aLevel3Tenets;
#endif // AUI_GS_PRIORITY_RATIO

	bool bMustChooseTenet = (pPlayer->GetNumFreeTenets() > 0);

	// Create a new vector holding only policies we can currently adopt
	m_AdoptablePolicies.clear();

	// Loop through adding the adoptable policies
	for(iPolicyLoop = 0; iPolicyLoop < m_pCurrentPolicies->GetPolicies()->GetNumPolicies(); iPolicyLoop++)
	{
		if(m_pCurrentPolicies->CanAdoptPolicy((PolicyTypes) iPolicyLoop) && (!bMustChooseTenet || m_pCurrentPolicies->GetPolicies()->GetPolicyEntry(iPolicyLoop)->GetLevel() > 0))
		{
			int iWeight = 0;

			iWeight += m_PolicyAIWeights.GetWeight(iPolicyLoop);

			// Does this policy finish a branch for us?
			if(m_pCurrentPolicies->WillFinishBranchIfAdopted((PolicyTypes) iPolicyLoop))
			{
				int iPolicyBranch = m_pCurrentPolicies->GetPolicies()->GetPolicyEntry(iPolicyLoop)->GetPolicyBranchType();
				if(iPolicyBranch != NO_POLICY_BRANCH_TYPE)
				{
					int iFinisherPolicy = m_pCurrentPolicies->GetPolicies()->GetPolicyBranchEntry(iPolicyBranch)->GetFreeFinishingPolicy();
					if(iFinisherPolicy != NO_POLICY)
					{
						iWeight += m_PolicyAIWeights.GetWeight(iFinisherPolicy);
					}
				}
			}
			m_AdoptablePolicies.push_back(iPolicyLoop + GC.getNumPolicyBranchInfos(), iWeight);

			if (m_pCurrentPolicies->GetPolicies()->GetPolicyEntry(iPolicyLoop)->GetLevel() == 3)
			{
#ifdef AUI_GS_PRIORITY_RATIO
				aLevel3Tenets.push_back(iPolicyLoop, iWeight);
#else
				aLevel3Tenets.push_back(iPolicyLoop);
#endif // AUI_GS_PRIORITY_RATIO
			}
		}
	}

	// Did we already start a branch in the set that is mutually exclusive?
	bool bStartedAMutuallyExclusiveBranch = false;
	for(int iBranchLoop = 0; iBranchLoop < GC.getNumPolicyBranchInfos(); iBranchLoop++)
	{
		const PolicyBranchTypes ePolicyBranch = static_cast<PolicyBranchTypes>(iBranchLoop);
		CvPolicyBranchEntry* pkPolicyBranchInfo = GC.getPolicyBranchInfo(ePolicyBranch);
		if(pkPolicyBranchInfo)
		{
			if(pPlayer->GetPlayerPolicies()->IsPolicyBranchUnlocked(ePolicyBranch))
			{
				if(pkPolicyBranchInfo->IsMutuallyExclusive())
				{
					bStartedAMutuallyExclusiveBranch = true;
				}
			}
		}
	}

	AIGrandStrategyTypes eCultureGrandStrategy = (AIGrandStrategyTypes) GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE");
#ifdef AUI_POLICY_CHOOSE_NEXT_POLICY_TWEAKED_OPEN_NEW_BRANCH
#ifdef AUI_GS_PRIORITY_RATIO
	AIGrandStrategyTypes eConquestGrandStrategy = (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST");
	AIGrandStrategyTypes eDiplomaticGrandStrategy = (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS");
	AIGrandStrategyTypes eSpaceshipGrandStrategy = (AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP");
#endif // AUI_GS_PRIORITY_RATIO
#else
	AIGrandStrategyTypes eCurrentGrandStrategy = pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy();
#endif // AUI_POLICY_CHOOSE_NEXT_POLICY_TWEAKED_OPEN_NEW_BRANCH

	// Loop though the branches adding each as another possibility
	if (!bMustChooseTenet)
	{
		for(int iBranchLoop = 0; iBranchLoop < GC.getNumPolicyBranchInfos(); iBranchLoop++)
		{
			const PolicyBranchTypes ePolicyBranch = static_cast<PolicyBranchTypes>(iBranchLoop);
			CvPolicyBranchEntry* pkPolicyBranchInfo = GC.getPolicyBranchInfo(ePolicyBranch);
			if(pkPolicyBranchInfo)
			{
				if(bStartedAMutuallyExclusiveBranch && pkPolicyBranchInfo->IsMutuallyExclusive())
				{
					continue;
				}

				if(pPlayer->GetPlayerPolicies()->CanUnlockPolicyBranch(ePolicyBranch) && !pPlayer->GetPlayerPolicies()->IsPolicyBranchUnlocked(ePolicyBranch))
				{
#ifdef AUI_POLICY_USE_DOUBLES
					double dBranchWeight = 0.0;

					// Does this branch actually help us, based on game options?
					if(IsBranchEffectiveInGame(ePolicyBranch))
					{
#ifdef AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
						dBranchWeight += WeighBranch(pPlayer, ePolicyBranch);
#else
#ifdef AUI_POLICY_WEIGH_BRANCH_INCLUDES_WONDER
						dBranchWeight += WeighBranch(pPlayer, ePolicyBranch);
#else
						dBranchWeight += WeighBranch(ePolicyBranch);
#endif // AUI_POLICY_WEIGH_BRANCH_INCLUDES_WONDER
#endif // AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
						dBranchWeight *= (100.0 - m_iPolicyWeightPercentDropNewBranch);
						dBranchWeight /= 100.0;

#ifdef AUI_POLICY_CHOOSE_NEXT_POLICY_TWEAKED_OPEN_NEW_BRANCH
#ifdef AUI_GS_PRIORITY_RATIO
						// Factor ranges from 1/4 to 1/2 based on how "determined" the AI is towards a specific GS
						// This is done via geometric mean of all GS ratios; highly determined = lower mean (since all ratios other than active are low)
						// Determined AI's should not branch out as much because they already know what victory they're going for
						dBranchWeight *= pow((1.0 + (double)pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio(eCultureGrandStrategy))
							* (1.0 + (double)pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio(eConquestGrandStrategy))
							* (1.0 + (double)pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio(eDiplomaticGrandStrategy))
							* (1.0 + (double)pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio(eSpaceshipGrandStrategy)), 1.0/4.0) / 4.0;
#endif // AUI_GS_PRIORITY_RATIO
#else
						if(eCurrentGrandStrategy == eCultureGrandStrategy)
						{
							dBranchWeight /= 3;
						}
#endif // AUI_POLICY_CHOOSE_NEXT_POLICY_TWEAKED_OPEN_NEW_BRANCH
					}

					m_AdoptablePolicies.push_back(iBranchLoop, (int)(dBranchWeight + 0.5));
#else
					int iBranchWeight = 0;

					// Does this branch actually help us, based on game options?
					if(IsBranchEffectiveInGame(ePolicyBranch))
					{
#ifdef AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
						iBranchWeight += WeighBranch(pPlayer, ePolicyBranch);
#else
#ifdef AUI_POLICY_WEIGH_BRANCH_INCLUDES_WONDER
						iBranchWeight += WeighBranch(pPlayer, ePolicyBranch);
#else
						iBranchWeight += WeighBranch(ePolicyBranch);
#endif // AUI_POLICY_WEIGH_BRANCH_INCLUDES_WONDER
#endif // AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE

						iBranchWeight *= (100 - m_iPolicyWeightPercentDropNewBranch);
						iBranchWeight /= 100;
#ifndef AUI_POLICY_CHOOSE_NEXT_POLICY_TWEAKED_OPEN_NEW_BRANCH
						if(eCurrentGrandStrategy == eCultureGrandStrategy)
						{
							iBranchWeight /= 3;
						}
#endif // AUI_POLICY_CHOOSE_NEXT_POLICY_TWEAKED_OPEN_NEW_BRANCH
					}

					m_AdoptablePolicies.push_back(iBranchLoop, iBranchWeight);
#endif // AUI_POLICY_USE_DOUBLES
				}
			}
		}
	}

	m_AdoptablePolicies.SortItems();
	LogPossiblePolicies();

	// If there were any Level 3 tenets found, consider going for the one that matches our victory strategy
	if (aLevel3Tenets.size() > 0)
	{
#ifdef AUI_GS_PRIORITY_RATIO
		aLevel3Tenets.SortItems();
		CvWeightedVector<int> aiPossibleChoices;
		CvPolicyEntry *pEntry;
		for (int iI = 0; iI < aLevel3Tenets.size(); iI++)
		{
			pEntry = m_pCurrentPolicies->GetPolicies()->GetPolicyEntry(aLevel3Tenets.GetElement(iI));
			if (pEntry)
			{
				if (pEntry->GetFlavorValue((FlavorTypes)GC.getInfoTypeForString("FLAVOR_OFFENSE")) > 0)
				{
					if (pPlayer->GetGrandStrategyAI()->IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST")))
					{
						aiPossibleChoices.push_back(aLevel3Tenets.GetElement(iI) + GC.getNumPolicyBranchInfos(), aLevel3Tenets.GetWeight(iI));
					}
				}
				if (pEntry->GetFlavorValue((FlavorTypes)GC.getInfoTypeForString("FLAVOR_CULTURE")) > 0)
				{
					if (pPlayer->GetGrandStrategyAI()->IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE")))
					{
						aiPossibleChoices.push_back(aLevel3Tenets.GetElement(iI) + GC.getNumPolicyBranchInfos(), aLevel3Tenets.GetWeight(iI));
					}
				}
				if (pEntry->GetFlavorValue((FlavorTypes)GC.getInfoTypeForString("FLAVOR_DIPLOMACY")) > 0)
				{
					if (pPlayer->GetGrandStrategyAI()->IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS")))
					{
						aiPossibleChoices.push_back(aLevel3Tenets.GetElement(iI) + GC.getNumPolicyBranchInfos(), aLevel3Tenets.GetWeight(iI));
					}
				}
				if (pEntry->GetFlavorValue((FlavorTypes)GC.getInfoTypeForString("FLAVOR_SPACESHIP")) > 0)
				{
					if (pPlayer->GetGrandStrategyAI()->IsGrandStrategySignificant((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP")))
					{
						aiPossibleChoices.push_back(aLevel3Tenets.GetElement(iI) + GC.getNumPolicyBranchInfos(), aLevel3Tenets.GetWeight(iI));
					}
				}
			}
		}

		if (aiPossibleChoices.size() > 0)
		{
			aiPossibleChoices.SortItems();
			int iRtnValue = aiPossibleChoices.ChooseByWeight(&fcn, "Choosing Level 3 Tenent");
			if (iRtnValue != (int)NO_POLICY)
			{
				if (iRtnValue >= GC.getNumPolicyBranchInfos())
				{
					LogPolicyChoice((PolicyTypes)(iRtnValue - GC.getNumPolicyBranchInfos()));
				}
				else
				{
					LogBranchChoice((PolicyBranchTypes)iRtnValue);
				}
			}
			return iRtnValue;
		}
#else
		vector<int>::const_iterator it;
		for (it = aLevel3Tenets.begin(); it != aLevel3Tenets.end(); it++)
		{
			CvPolicyEntry *pEntry;
			pEntry = m_pCurrentPolicies->GetPolicies()->GetPolicyEntry(*it);
			if (pEntry)
			{
				AIGrandStrategyTypes eGrandStrategy = pPlayer->GetGrandStrategyAI()->GetActiveGrandStrategy();
				if (eGrandStrategy == GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST"))
				{
					if (pEntry->GetFlavorValue((FlavorTypes)GC.getInfoTypeForString("FLAVOR_OFFENSE")) > 0)
					{
						LogPolicyChoice((PolicyTypes)*it);
						return (*it) + GC.getNumPolicyBranchInfos();
					}
				}
				else if(eGrandStrategy == GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP"))
				{
					if (pEntry->GetFlavorValue((FlavorTypes)GC.getInfoTypeForString("FLAVOR_SPACESHIP")) > 0)
					{
						LogPolicyChoice((PolicyTypes)*it);
						return (*it) + GC.getNumPolicyBranchInfos();
					}
				}
				else if(eGrandStrategy == GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS"))
				{
					if (pEntry->GetFlavorValue((FlavorTypes)GC.getInfoTypeForString("FLAVOR_DIPLOMACY")) > 0)
					{
						LogPolicyChoice((PolicyTypes)*it);
						return (*it) + GC.getNumPolicyBranchInfos();
					}
				}
				else if(eGrandStrategy == GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE"))
				{
					if (pEntry->GetFlavorValue((FlavorTypes)GC.getInfoTypeForString("FLAVOR_CULTURE")) > 0)
					{
						LogPolicyChoice((PolicyTypes)*it);
						return (*it) + GC.getNumPolicyBranchInfos();
					}
				}
			}
		}
#endif // AUI_GS_PRIORITY_RATIO
	}

	CvAssertMsg(m_AdoptablePolicies.GetTotalWeight() >= 0, "Total weights of considered policies should not be negative! Please send Anton your save file and version.");

	// If total weight is above 0, choose one above a threshold
	if(m_AdoptablePolicies.GetTotalWeight() > 0)
	{
		int iNumChoices = GC.getGame().getHandicapInfo().GetPolicyNumOptions();
		iRtnValue = m_AdoptablePolicies.ChooseFromTopChoices(iNumChoices, &fcn, "Choosing policy from Top Choices");
	}
	// Total weight may be 0 if the only branches and policies left are ones that are ineffective in our game, but we gotta pick something
	else if(m_AdoptablePolicies.GetTotalWeight() == 0 && m_AdoptablePolicies.size() > 0)
	{
		iRtnValue = m_AdoptablePolicies.ChooseAtRandom(&fcn, "Choosing policy at random (no good choices)");
	}

	// Log our choice
	if(iRtnValue != (int)NO_POLICY)
	{
		if(iRtnValue >= GC.getNumPolicyBranchInfos())
		{
			LogPolicyChoice((PolicyTypes)(iRtnValue - GC.getNumPolicyBranchInfos()));
		}
		else
		{
			LogBranchChoice((PolicyBranchTypes)iRtnValue);
		}
	}

	return iRtnValue;
}

void CvPolicyAI::DoChooseIdeology(CvPlayer *pPlayer)
{
	int iFreedomPriority = 0;
	int iAutocracyPriority = 0;
	int iOrderPriority = 0;
	int iFreedomMultiplier = 1;
	int iAutocracyMultiplier = 1;
	int iOrderMultiplier = 1;
	PolicyBranchTypes eFreedomBranch = (PolicyBranchTypes)GC.getPOLICY_BRANCH_FREEDOM();
	PolicyBranchTypes eAutocracyBranch = (PolicyBranchTypes)GC.getPOLICY_BRANCH_AUTOCRACY();
	PolicyBranchTypes eOrderBranch = (PolicyBranchTypes)GC.getPOLICY_BRANCH_ORDER();
	if (eFreedomBranch == NO_POLICY_BRANCH_TYPE || eAutocracyBranch == NO_POLICY_BRANCH_TYPE || eOrderBranch == NO_POLICY_BRANCH_TYPE)
	{
		return;
	}

	// First consideration is our victory type
#ifdef AUI_GS_PRIORITY_OVERHAUL
	int iConquestPriority = max(0.0f, pPlayer->GetGrandStrategyAI()->GetConquestETAPriority()
		+ pPlayer->GetGrandStrategyAI()->GetBaseGrandStrategyPriority((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST")));
	int iDiploPriority = max(0.0f, pPlayer->GetGrandStrategyAI()->GetUnitedNationsETAPriority()
		+ pPlayer->GetGrandStrategyAI()->GetBaseGrandStrategyPriority((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS")));
	int iTechPriority = max(0.0f, pPlayer->GetGrandStrategyAI()->GetSpaceshipETAPriority()
		+ pPlayer->GetGrandStrategyAI()->GetBaseGrandStrategyPriority((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP")));
	int iCulturePriority = max(0.0f, pPlayer->GetGrandStrategyAI()->GetCultureETAPriority()
		+ pPlayer->GetGrandStrategyAI()->GetBaseGrandStrategyPriority((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE")));
#else
	int iConquestPriority = max(0, pPlayer->GetGrandStrategyAI()->GetConquestPriority());
	int iDiploPriority = max(0, pPlayer->GetGrandStrategyAI()->GetUnitedNationsPriority());
	int iTechPriority = max(0, pPlayer->GetGrandStrategyAI()->GetSpaceshipPriority());
	int iCulturePriority = max(0, pPlayer->GetGrandStrategyAI()->GetCulturePriority());
#endif // AUI_GS_PRIORITY_OVERHAUL

	// Rule out one ideology if we are clearly (at least 25% more priority) going for the victory this ideology doesn't support
	int iClearPrefPercent = GC.getIDEOLOGY_PERCENT_CLEAR_VICTORY_PREF();
#ifdef AUI_POLICY_DO_CHOOSE_IDEOLOGY_TWEAKED_CLEAR_PREFS
	if (iConquestPriority * 100 > iDiploPriority   * (100 + iClearPrefPercent) &&
		iConquestPriority * 100 > iTechPriority    * (100 + iClearPrefPercent) &&
		iConquestPriority * 100 > iCulturePriority * (100 + iClearPrefPercent))
	{
		iFreedomMultiplier = 0;
	}
	else if (iDiploPriority * 100 > iConquestPriority * (100 + iClearPrefPercent) &&
		iDiploPriority * 100 > iTechPriority     * (100 + iClearPrefPercent) &&
		iDiploPriority * 100 > iCulturePriority  * (100 + iClearPrefPercent))
	{
		iOrderMultiplier = 0;
	}
	else if (iTechPriority * 100 > iConquestPriority * (100 + iClearPrefPercent) &&
		iTechPriority * 100 > iDiploPriority    * (100 + iClearPrefPercent) &&
		iTechPriority * 100 > iCulturePriority  * (100 + iClearPrefPercent))
	{
		iAutocracyMultiplier = 0;
	}
#else
	if (iConquestPriority > (iDiploPriority   * (100 + iClearPrefPercent) / 100) &&
		iConquestPriority > (iTechPriority    * (100 + iClearPrefPercent) / 100) &&
		iConquestPriority > (iCulturePriority * (100 + iClearPrefPercent) / 100))
	{
		iFreedomMultiplier = 0;
	}
	else if (iDiploPriority > (iConquestPriority * (100 + iClearPrefPercent) / 100) &&
		iDiploPriority > (iTechPriority     * (100 + iClearPrefPercent) / 100) &&
		iDiploPriority > (iCulturePriority  * (100 + iClearPrefPercent) / 100))
	{
		iOrderMultiplier = 0;
	}
	else if (iTechPriority > (iConquestPriority * (100 + iClearPrefPercent) / 100) &&
		iTechPriority > (iDiploPriority    * (100 + iClearPrefPercent) / 100) &&
		iTechPriority > (iCulturePriority  * (100 + iClearPrefPercent) / 100))
	{
		iAutocracyMultiplier = 0;
	}
#endif // AUI_POLICY_DO_CHOOSE_IDEOLOGY_TWEAKED_CLEAR_PREFS

	int iFreedomTotal = iDiploPriority + iTechPriority + iCulturePriority;
	int iAutocracyTotal = iDiploPriority + iConquestPriority + iCulturePriority;
	int iOrderTotal = iTechPriority + iConquestPriority + iCulturePriority;
	int iGrandTotal = iFreedomTotal + iAutocracyTotal + iOrderTotal;

	if (iGrandTotal > 0)
	{
		int iPriorityToDivide = GC.getIDEOLOGY_SCORE_GRAND_STRATS();
		iFreedomPriority = (iFreedomTotal * iPriorityToDivide) / iGrandTotal;
		iAutocracyPriority = (iAutocracyTotal * iPriorityToDivide) / iGrandTotal;
		iOrderPriority = (iOrderTotal * iPriorityToDivide) / iGrandTotal;
	}

	CvString stage = "After Grand Strategies";
	LogIdeologyChoice(stage, iFreedomPriority, iAutocracyPriority, iOrderPriority);

	// Next look at free policies we can get
	iFreedomPriority += PolicyHelpers::GetNumFreePolicies(eFreedomBranch) * GC.getIDEOLOGY_SCORE_PER_FREE_TENET();
	iAutocracyPriority += PolicyHelpers::GetNumFreePolicies(eAutocracyBranch) * GC.getIDEOLOGY_SCORE_PER_FREE_TENET();
	iOrderPriority += PolicyHelpers::GetNumFreePolicies(eOrderBranch) * GC.getIDEOLOGY_SCORE_PER_FREE_TENET();;

	stage = "After Free Policies";
	LogIdeologyChoice(stage, iFreedomPriority, iAutocracyPriority, iOrderPriority);

	// Finally see what our friends (and enemies) have already chosen
	PlayerTypes eLoopPlayer;
	for (int iPlayerLoop = 0; iPlayerLoop < MAX_MAJOR_CIVS; iPlayerLoop++)
	{
		eLoopPlayer = (PlayerTypes) iPlayerLoop;
		if (eLoopPlayer != pPlayer->GetID() && pPlayer->GetDiplomacyAI()->IsPlayerValid(eLoopPlayer))
		{
			CvPlayer &kOtherPlayer = GET_PLAYER(eLoopPlayer);
			PolicyBranchTypes eOtherPlayerIdeology;
			eOtherPlayerIdeology = kOtherPlayer.GetPlayerPolicies()->GetLateGamePolicyTree();

			switch(pPlayer->GetDiplomacyAI()->GetMajorCivApproach(eLoopPlayer, /*bHideTrueFeelings*/ true))
			{
			case MAJOR_CIV_APPROACH_HOSTILE:
				if (eOtherPlayerIdeology == eFreedomBranch)
				{
					iAutocracyPriority += GC.getIDEOLOGY_SCORE_HOSTILE();
					iOrderPriority += GC.getIDEOLOGY_SCORE_HOSTILE();
				}
				else if (eOtherPlayerIdeology == eAutocracyBranch)
				{
					iFreedomPriority += GC.getIDEOLOGY_SCORE_HOSTILE();
					iOrderPriority += GC.getIDEOLOGY_SCORE_HOSTILE();
				}
				else if (eOtherPlayerIdeology == eOrderBranch)
				{
					iAutocracyPriority += GC.getIDEOLOGY_SCORE_HOSTILE();;
					iFreedomPriority += GC.getIDEOLOGY_SCORE_HOSTILE();
				}
				break;
			case MAJOR_CIV_APPROACH_GUARDED:
				if (eOtherPlayerIdeology == eFreedomBranch)
				{
					iAutocracyPriority += GC.getIDEOLOGY_SCORE_GUARDED();
					iOrderPriority += GC.getIDEOLOGY_SCORE_GUARDED();
				}
				else if (eOtherPlayerIdeology == eAutocracyBranch)
				{
					iFreedomPriority += GC.getIDEOLOGY_SCORE_GUARDED();
					iOrderPriority += GC.getIDEOLOGY_SCORE_GUARDED();
				}
				else if (eOtherPlayerIdeology == eOrderBranch)
				{
					iAutocracyPriority += GC.getIDEOLOGY_SCORE_GUARDED();
					iFreedomPriority += GC.getIDEOLOGY_SCORE_GUARDED();
				}
				break;
			case MAJOR_CIV_APPROACH_AFRAID:
				if (eOtherPlayerIdeology == eFreedomBranch)
				{
					iFreedomPriority += GC.getIDEOLOGY_SCORE_AFRAID();
				}
				else if (eOtherPlayerIdeology == eAutocracyBranch)
				{
					iAutocracyPriority += GC.getIDEOLOGY_SCORE_AFRAID();
				}
				else if (eOtherPlayerIdeology == eOrderBranch)
				{
					iOrderPriority += GC.getIDEOLOGY_SCORE_AFRAID();
				}
				break;
			case MAJOR_CIV_APPROACH_FRIENDLY:
				if (eOtherPlayerIdeology == eFreedomBranch)
				{
					iFreedomPriority += GC.getIDEOLOGY_SCORE_FRIENDLY();
				}
				else if (eOtherPlayerIdeology == eAutocracyBranch)
				{
					iAutocracyPriority += GC.getIDEOLOGY_SCORE_FRIENDLY();
				}
				else if (eOtherPlayerIdeology == eOrderBranch)
				{
					iOrderPriority += GC.getIDEOLOGY_SCORE_FRIENDLY();
				}
				break;
			case MAJOR_CIV_APPROACH_NEUTRAL:
				// No changes
				break;
			}
		}
	}

	stage = "After Relations";
	LogIdeologyChoice(stage, iFreedomPriority, iAutocracyPriority, iOrderPriority);

	// Look at Happiness impacts
	int iHappinessModifier = GC.getIDEOLOGY_SCORE_HAPPINESS();

	// -- Happiness we could add through tenets
	int iHappinessDelta;
	int iHappinessPoliciesInBranch;
	iHappinessDelta = GetBranchBuildingHappiness(pPlayer, eFreedomBranch);
	iHappinessPoliciesInBranch = GetNumHappinessPolicies(pPlayer, eFreedomBranch);
	if (iHappinessPoliciesInBranch > 0)
	{
		iFreedomPriority += iHappinessDelta * iHappinessModifier / iHappinessPoliciesInBranch;		
	}
	iHappinessDelta = GetBranchBuildingHappiness(pPlayer, eAutocracyBranch);
	iHappinessPoliciesInBranch = GetNumHappinessPolicies(pPlayer, eAutocracyBranch);
	if (iHappinessPoliciesInBranch > 0)
	{
		iAutocracyPriority += iHappinessDelta * iHappinessModifier / iHappinessPoliciesInBranch;		
	}
	iHappinessDelta = GetBranchBuildingHappiness(pPlayer, eOrderBranch);
	iHappinessPoliciesInBranch = GetNumHappinessPolicies(pPlayer, eOrderBranch);
	if (iHappinessPoliciesInBranch > 0)
	{
		iOrderPriority += iHappinessDelta * iHappinessModifier / iHappinessPoliciesInBranch;		
	}

	stage = "After Tenet Happiness Boosts";
	LogIdeologyChoice(stage, iFreedomPriority, iAutocracyPriority, iOrderPriority);

	// -- Happiness we'd lose through Public Opinion
	iHappinessDelta = max (0, 100 - pPlayer->GetCulture()->ComputeHypotheticalPublicOpinionUnhappiness(eFreedomBranch));
	iFreedomPriority += iHappinessDelta * iHappinessModifier;
	iHappinessDelta = max (0, 100 - pPlayer->GetCulture()->ComputeHypotheticalPublicOpinionUnhappiness(eAutocracyBranch));
	iAutocracyPriority += iHappinessDelta * iHappinessModifier;
	iHappinessDelta = max (0, 100 - pPlayer->GetCulture()->ComputeHypotheticalPublicOpinionUnhappiness(eOrderBranch));
	iOrderPriority += iHappinessDelta * iHappinessModifier;

	stage = "After Public Opinion Happiness";
	LogIdeologyChoice(stage, iFreedomPriority, iAutocracyPriority, iOrderPriority);

	// Small random add-on
#ifdef AUI_POLICY_DO_CHOOSE_IDEOLOGY_USES_BINOM_RNG
	iFreedomPriority += GC.getGame().getJonRandNumBinom(AUI_POLICY_DO_CHOOSE_IDEOLOGY_USES_BINOM_RNG, "Freedom random priority bump");
	iAutocracyPriority += GC.getGame().getJonRandNumBinom(AUI_POLICY_DO_CHOOSE_IDEOLOGY_USES_BINOM_RNG, "Autocracy random priority bump");
	iOrderPriority += GC.getGame().getJonRandNumBinom(AUI_POLICY_DO_CHOOSE_IDEOLOGY_USES_BINOM_RNG, "Order random priority bump");
#else
	iFreedomPriority += GC.getGame().getJonRandNum(10, "Freedom random priority bump");
	iAutocracyPriority += GC.getGame().getJonRandNum(10, "Autocracy random priority bump");
	iOrderPriority += GC.getGame().getJonRandNum(10, "Order random priority bump");
#endif // AUI_POLICY_DO_CHOOSE_IDEOLOGY_USES_BINOM_RNG

	stage = "After Random (1 to 10)";
	LogIdeologyChoice(stage, iFreedomPriority, iAutocracyPriority, iOrderPriority);

	// Rule out any branches that are totally out of consideration
	iFreedomPriority = iFreedomPriority * iFreedomMultiplier;
	iAutocracyPriority = iAutocracyPriority * iAutocracyMultiplier;
	iOrderPriority = iOrderPriority * iOrderMultiplier;

	stage = "Final (after Clear Victory Preference)";
	LogIdeologyChoice(stage, iFreedomPriority, iAutocracyPriority, iOrderPriority);

#ifdef AUI_POLICY_DO_CHOOSE_IDEOLOGY_TIEBREAKER
	// Tiebreaker
	while (iFreedomPriority == iAutocracyPriority && iFreedomPriority == iOrderPriority && iAutocracyPriority == iOrderPriority)
	{
		// Small random add-on
#ifdef AUI_POLICY_DO_CHOOSE_IDEOLOGY_USES_BINOM_RNG
		iFreedomPriority += GC.getGame().getJonRandNumBinom(AUI_POLICY_DO_CHOOSE_IDEOLOGY_USES_BINOM_RNG, "Freedom random priority bump");
		iAutocracyPriority += GC.getGame().getJonRandNumBinom(AUI_POLICY_DO_CHOOSE_IDEOLOGY_USES_BINOM_RNG, "Autocracy random priority bump");
		iOrderPriority += GC.getGame().getJonRandNumBinom(AUI_POLICY_DO_CHOOSE_IDEOLOGY_USES_BINOM_RNG, "Order random priority bump");
#else
		iFreedomPriority += GC.getGame().getJonRandNum(10, "Freedom random priority bump");
		iAutocracyPriority += GC.getGame().getJonRandNum(10, "Autocracy random priority bump");
		iOrderPriority += GC.getGame().getJonRandNum(10, "Order random priority bump");
#endif // AUI_POLICY_DO_CHOOSE_IDEOLOGY_USES_BINOM_RNG
	}
#endif // AUI_POLICY_DO_CHOOSE_IDEOLOGY_TIEBREAKER

	// Pick the ideology
	PolicyBranchTypes eChosenBranch;
#ifdef AUI_POLICY_DO_CHOOSE_IDEOLOGY_TIEBREAKER
	if (iFreedomPriority > iAutocracyPriority && iFreedomPriority > iOrderPriority)
#else
	if (iFreedomPriority >= iAutocracyPriority && iFreedomPriority >= iOrderPriority)
#endif // AUI_POLICY_DO_CHOOSE_IDEOLOGY_TIEBREAKER
	{
		eChosenBranch = eFreedomBranch;
	}
#ifdef AUI_POLICY_DO_CHOOSE_IDEOLOGY_TIEBREAKER
	else if (iAutocracyPriority > iFreedomPriority && iAutocracyPriority > iOrderPriority)
#else
	else if (iAutocracyPriority >= iFreedomPriority && iAutocracyPriority >= iOrderPriority)
#endif // AUI_POLICY_DO_CHOOSE_IDEOLOGY_TIEBREAKER
	{
		eChosenBranch = eAutocracyBranch;
	}
	else
	{
		eChosenBranch = eOrderBranch;
	}
	pPlayer->GetPlayerPolicies()->SetPolicyBranchUnlocked(eChosenBranch, true, false);
	LogBranchChoice(eChosenBranch);
}

/// Should the AI look at switching ideology branches?
void CvPolicyAI::DoConsiderIdeologySwitch(CvPlayer* pPlayer)
{
	// Gather basic Ideology info
	int iCurrentHappiness = pPlayer->GetExcessHappiness();
	int iPublicOpinionUnhappiness = pPlayer->GetCulture()->GetPublicOpinionUnhappiness();
	PolicyBranchTypes ePreferredIdeology = pPlayer->GetCulture()->GetPublicOpinionPreferredIdeology();
	PolicyBranchTypes eCurrentIdeology = pPlayer->GetPlayerPolicies()->GetLateGamePolicyTree();
	PlayerTypes eMostPressure = pPlayer->GetCulture()->GetPublicOpinionBiggestInfluence();
	
	// Possible enough that we need to look at this in detail?
	if (iCurrentHappiness <= GC.getSUPER_UNHAPPY_THRESHOLD() && iPublicOpinionUnhappiness >= 10)
	{
		// How much Happiness could we gain from a switch?
		int iHappinessCurrentIdeology = GetBranchBuildingHappiness(pPlayer, eCurrentIdeology);
		int iHappinessPreferredIdeology = GetBranchBuildingHappiness(pPlayer, ePreferredIdeology);

		// Does the switch fight against our clearly preferred victory path?
		bool bDontSwitchFreedom = false;
		bool bDontSwitchOrder = false;
		bool bDontSwitchAutocracy = false;
#ifdef AUI_GS_PRIORITY_OVERHAUL
		int iConquestPriority = max(0.0f, pPlayer->GetGrandStrategyAI()->GetConquestETAPriority()
			+ pPlayer->GetGrandStrategyAI()->GetBaseGrandStrategyPriority((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CONQUEST")));
		int iDiploPriority = max(0.0f, pPlayer->GetGrandStrategyAI()->GetUnitedNationsETAPriority()
			+ pPlayer->GetGrandStrategyAI()->GetBaseGrandStrategyPriority((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS")));
		int iTechPriority = max(0.0f, pPlayer->GetGrandStrategyAI()->GetSpaceshipETAPriority()
			+ pPlayer->GetGrandStrategyAI()->GetBaseGrandStrategyPriority((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_SPACESHIP")));
		int iCulturePriority = max(0.0f, pPlayer->GetGrandStrategyAI()->GetCultureETAPriority()
			+ pPlayer->GetGrandStrategyAI()->GetBaseGrandStrategyPriority((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_CULTURE")));
#else
		int iConquestPriority = pPlayer->GetGrandStrategyAI()->GetConquestPriority();
		int iDiploPriority = pPlayer->GetGrandStrategyAI()->GetUnitedNationsPriority();
		int iTechPriority = pPlayer->GetGrandStrategyAI()->GetSpaceshipPriority();
		int iCulturePriority = pPlayer->GetGrandStrategyAI()->GetCulturePriority();
#endif // AUI_GS_PRIORITY_OVERHAUL
		int iClearPrefPercent = GC.getIDEOLOGY_PERCENT_CLEAR_VICTORY_PREF();
#ifdef AUI_POLICY_DO_CONSIDER_IDEOLOGY_SWITCH_TWEAKED_CLEAR_PREFS
		if (pPlayer->IsEmpireVeryUnhappy())
			iClearPrefPercent *= 1000;
		if (iConquestPriority * 100 > iDiploPriority   * (100 + iClearPrefPercent) &&
			iConquestPriority * 100 > iTechPriority    * (100 + iClearPrefPercent) &&
			iConquestPriority * 100 > iCulturePriority * (100 + iClearPrefPercent))
		{
			bDontSwitchFreedom = true;
		}
		else if (iDiploPriority * 100 > iConquestPriority * (100 + iClearPrefPercent) &&
			iDiploPriority * 100 > iTechPriority     * (100 + iClearPrefPercent) &&
			iDiploPriority * 100 > iCulturePriority  * (100 + iClearPrefPercent))
		{
			bDontSwitchOrder = true;
		}
		else if (iTechPriority * 100 > iConquestPriority * (100 + iClearPrefPercent) &&
			iTechPriority * 100 > iDiploPriority    * (100 + iClearPrefPercent) &&
			iTechPriority * 100 > iCulturePriority  * (100 + iClearPrefPercent))
		{
			bDontSwitchAutocracy = true;
		}
#else
		if (iConquestPriority > (iDiploPriority   * (100 + iClearPrefPercent) / 100) &&
			iConquestPriority > (iTechPriority    * (100 + iClearPrefPercent) / 100) &&
			iConquestPriority > (iCulturePriority * (100 + iClearPrefPercent) / 100))
		{
			bDontSwitchFreedom = true;
		}
		else if (iDiploPriority > (iConquestPriority * (100 + iClearPrefPercent) / 100) &&
			iDiploPriority > (iTechPriority     * (100 + iClearPrefPercent) / 100) &&
			iDiploPriority > (iCulturePriority  * (100 + iClearPrefPercent) / 100))
		{
			bDontSwitchOrder = true;
		}
		else if (iTechPriority > (iConquestPriority * (100 + iClearPrefPercent) / 100) &&
			iTechPriority > (iDiploPriority    * (100 + iClearPrefPercent) / 100) &&
			iTechPriority > (iCulturePriority  * (100 + iClearPrefPercent) / 100))
		{
			bDontSwitchAutocracy = true;
		}
#endif // AUI_POLICY_DO_CONSIDER_IDEOLOGY_SWITCH_TWEAKED_CLEAR_PREFS

		int iTotalHappinessImprovement = iPublicOpinionUnhappiness + iHappinessPreferredIdeology - iHappinessCurrentIdeology;
		if (iTotalHappinessImprovement >= 10)
		{
			if (bDontSwitchFreedom && ePreferredIdeology == GC.getPOLICY_BRANCH_FREEDOM())
			{
				return;
			}
			if (bDontSwitchAutocracy && ePreferredIdeology == GC.getPOLICY_BRANCH_AUTOCRACY())
			{
				return;
			}
			if (bDontSwitchOrder && ePreferredIdeology == GC.getPOLICY_BRANCH_ORDER())
			{
				return;
			}

			// Cleared all obstacles -- REVOLUTION!
			pPlayer->SetAnarchyNumTurns(GC.getSWITCH_POLICY_BRANCHES_ANARCHY_TURNS());
			pPlayer->GetPlayerPolicies()->DoSwitchIdeologies(ePreferredIdeology);	

			if (ePreferredIdeology == GC.getPOLICY_BRANCH_FREEDOM() && eCurrentIdeology == GC.getPOLICY_BRANCH_ORDER())
			{
				if (GET_PLAYER(eMostPressure).GetID() == GC.getGame().getActivePlayer())
				{
					gDLL->UnlockAchievement(ACHIEVEMENT_XP2_39);
				}
			}
		}
	}
}

/// What's the total Happiness benefit we could get from all policies/tenets in the branch based on our current buildings?
int CvPolicyAI::GetBranchBuildingHappiness(CvPlayer* pPlayer, PolicyBranchTypes eBranch)
{
	// Policy Building Mods
	int iSpecialPolicyBuildingHappiness = 0;
	int iBuildingClassLoop;
	BuildingClassTypes eBuildingClass;
	for(int iPolicyLoop = 0; iPolicyLoop < GC.getNumPolicyInfos(); iPolicyLoop++)
	{
		PolicyTypes ePolicy = (PolicyTypes)iPolicyLoop;
		CvPolicyEntry* pkPolicyInfo = GC.getPolicyInfo(ePolicy);
		if(pkPolicyInfo)
		{
			if (pkPolicyInfo->GetPolicyBranchType() == eBranch)
			{
				for(iBuildingClassLoop = 0; iBuildingClassLoop < GC.getNumBuildingClassInfos(); iBuildingClassLoop++)
				{
					eBuildingClass = (BuildingClassTypes) iBuildingClassLoop;

					CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo(eBuildingClass);
					if (!pkBuildingClassInfo)
					{
						continue;
					}

					if (pkPolicyInfo->GetBuildingClassHappiness(eBuildingClass) != 0)
					{
						BuildingTypes eBuilding = (BuildingTypes)pPlayer->getCivilizationInfo().getCivilizationBuildings(eBuildingClass);
						if (eBuilding != NO_BUILDING)
						{
							CvCity *pCity;
							int iLoop;
							for (pCity = pPlayer->firstCity(&iLoop); pCity != NULL; pCity = pPlayer->nextCity(&iLoop))
							{
								if (pCity->GetCityBuildings()->GetNumBuilding(eBuilding) > 0)
								{
									iSpecialPolicyBuildingHappiness += pkPolicyInfo->GetBuildingClassHappiness(eBuildingClass);
								}
							}
						}
					}
				}
#ifdef AUI_POLICY_GET_BRANCH_BUILDING_HAPPINESS_GET_ALL_HAPPINESS_SOURCES
				// happiness from other sources
				int iCapitalPopulation = 0;
				int iTotalPopulation = 0;
				int iSpecialistPopulation = 0;
				int iConnectionCount = 0;
				CvCity *pCity;
				int iLoop;
				for (pCity = pPlayer->firstCity(&iLoop); pCity != NULL; pCity = pPlayer->nextCity(&iLoop))
				{
					if (pCity->isCapital())
					{
						iCapitalPopulation = pCity->getPopulation();
					}
					iTotalPopulation += pCity->getPopulation();
					iSpecialistPopulation += pCity->GetCityCitizens()->GetTotalSpecialistCount();
					iSpecialistPopulation += iSpecialistPopulation % 2;
					if (pCity->IsRouteToCapitalConnected())
					{
						iConnectionCount++;
					}
				}
				if (pkPolicyInfo->GetUnhappinessMod() != 0)
				{
					iSpecialPolicyBuildingHappiness += int(iTotalPopulation / 100.0 + 0.5) * -pkPolicyInfo->GetUnhappinessMod();
				}
				if (pkPolicyInfo->GetHappinessPerTradeRoute() != 0)
				{
					iSpecialPolicyBuildingHappiness += iConnectionCount * pkPolicyInfo->GetHappinessPerTradeRoute();
				}
				if (pkPolicyInfo->GetHappinessPerXPopulation() != 0)
				{
					iSpecialPolicyBuildingHappiness += iTotalPopulation / pkPolicyInfo->GetHappinessPerXPopulation();
				}
				if (pkPolicyInfo->GetCapitalUnhappinessMod() != 0)
				{
					iSpecialPolicyBuildingHappiness += iCapitalPopulation * -pkPolicyInfo->GetCapitalUnhappinessMod();
				}
				if (pkPolicyInfo->IsHalfSpecialistUnhappiness())
				{
					iSpecialPolicyBuildingHappiness += iSpecialistPopulation / 2;
				}
#endif // AUI_POLICY_GET_BRANCH_BUILDING_HAPPINESS_GET_ALL_HAPPINESS_SOURCES
			}
		}
	}
	return iSpecialPolicyBuildingHappiness;
}

/// How many policies in this branch help happiness?
int CvPolicyAI::GetNumHappinessPolicies(CvPlayer* pPlayer, PolicyBranchTypes eBranch)
{
	int iRtnValue = 0;
	int iBuildingClassLoop;
	BuildingClassTypes eBuildingClass;
	for(int iPolicyLoop = 0; iPolicyLoop < GC.getNumPolicyInfos(); iPolicyLoop++)
	{
		PolicyTypes ePolicy = (PolicyTypes)iPolicyLoop;
		CvPolicyEntry* pkPolicyInfo = GC.getPolicyInfo(ePolicy);
		if(pkPolicyInfo)
		{
			if (pkPolicyInfo->GetPolicyBranchType() == eBranch)
			{
				for(iBuildingClassLoop = 0; iBuildingClassLoop < GC.getNumBuildingClassInfos(); iBuildingClassLoop++)
				{
					eBuildingClass = (BuildingClassTypes) iBuildingClassLoop;

					CvBuildingClassInfo* pkBuildingClassInfo = GC.getBuildingClassInfo(eBuildingClass);
					if (!pkBuildingClassInfo)
					{
						continue;
					}

					BuildingTypes eBuilding = (BuildingTypes)pPlayer->getCivilizationInfo().getCivilizationBuildings(eBuildingClass);
					if (eBuilding != NO_BUILDING)
					{
						// Don't count a building that can only be built in conquered cities
						CvBuildingEntry *pkEntry = GC.getBuildingInfo(eBuilding);
						if (!pkEntry || pkEntry->IsNoOccupiedUnhappiness())
						{
							continue;
						}

						if (pkPolicyInfo->GetBuildingClassHappiness(eBuildingClass) != 0)
						{
							iRtnValue++;
							break;
						}
					}
				}
#ifdef AUI_POLICY_GET_BRANCH_BUILDING_HAPPINESS_GET_ALL_HAPPINESS_SOURCES
				// happiness from other sources
				if ((pkPolicyInfo->GetUnhappinessMod() != 0) || (pkPolicyInfo->GetHappinessPerTradeRoute() != 0) || (pkPolicyInfo->GetHappinessPerXPopulation() != 0) ||
					(pkPolicyInfo->GetCapitalUnhappinessMod() != 0) || (pkPolicyInfo->IsHalfSpecialistUnhappiness()))
				{
					iRtnValue++;
				}
#endif // AUI_POLICY_GET_BRANCH_BUILDING_HAPPINESS_GET_ALL_HAPPINESS_SOURCES
			}
		}
	}
	return iRtnValue;
}

//=====================================
// PRIVATE METHODS
//=====================================
/// Add weights to policies that are prereqs for the ones already weighted in this strategy
void CvPolicyAI::WeightPrereqs(int* paiTempWeights, int iPropagationPercent)
{
	int iPolicyLoop;

	// Loop through policies looking for ones that are just getting some new weight
	for(iPolicyLoop = 0; iPolicyLoop < m_pCurrentPolicies->GetPolicies()->GetNumPolicies(); iPolicyLoop++)
	{
		// If found one, call our recursive routine to weight everything to the left in the tree
		if(paiTempWeights[iPolicyLoop] > 0)
		{
			PropagateWeights(iPolicyLoop, paiTempWeights[iPolicyLoop], iPropagationPercent, 0);
		}
	}
}

/// Recursive routine to weight all prerequisite policies
void CvPolicyAI::PropagateWeights(int iPolicy, int iWeight, int iPropagationPercent, int iPropagationLevel)
{
	if(iPropagationLevel < m_iPolicyWeightPropagationLevels)
	{
		int iPropagatedWeight = iWeight * iPropagationPercent / 100;

		// Loop through all prerequisites
		for(int iI = 0; iI < GC.getNUM_OR_TECH_PREREQS(); iI++)
		{
			// Did we find a prereq?
			int iPrereq = m_pCurrentPolicies->GetPolicies()->GetPolicyEntry(iPolicy)->GetPrereqAndPolicies(iI);
			if(iPrereq != NO_POLICY)
			{
				// Apply reduced weight here.  Note that we apply these to the master weight array, not
				// the temporary one.  The temporary one is just used to hold the newly weighted policies
				// (from which this weight propagation must originate).
				m_PolicyAIWeights.IncreaseWeight(iPrereq, iPropagatedWeight);

				// Recurse to its prereqs (assuming we have any weight left)
				if(iPropagatedWeight > 0)
				{
					PropagateWeights(iPrereq, iPropagatedWeight, iPropagationPercent, iPropagationLevel++);
				}
			}
			else
			{
				break;
			}
		}
	}
}

/// Priority for opening up this branch
#ifdef AUI_POLICY_USE_DOUBLES
#ifdef AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
double CvPolicyAI::WeighBranch(CvPlayer* pPlayer, PolicyBranchTypes eBranch)
#else
#ifdef AUI_POLICY_WEIGH_BRANCH_INCLUDES_WONDER
double CvPolicyAI::WeighBranch(CvPlayer* pPlayer, PolicyBranchTypes eBranch)
#else
double CvPolicyAI::WeighBranch(PolicyBranchTypes eBranch)
#endif // AUI_POLICY_WEIGH_BRANCH_INCLUDES_WONDER
#endif // AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
#else
#ifdef AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
int CvPolicyAI::WeighBranch(CvPlayer* pPlayer, PolicyBranchTypes eBranch)
#else
#ifdef AUI_POLICY_WEIGH_BRANCH_INCLUDES_WONDER
int CvPolicyAI::WeighBranch(CvPlayer* pPlayer, PolicyBranchTypes eBranch)
#else
int CvPolicyAI::WeighBranch(PolicyBranchTypes eBranch)
#endif // AUI_POLICY_WEIGH_BRANCH_INCLUDES_WONDER
#endif // AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
#endif // AUI_POLICY_USE_DOUBLES
{
#ifdef AUI_POLICY_USE_DOUBLES
	double dWeight = 0.0;
#else
	int iWeight = 0;
#endif // AUI_POLICY_USE_DOUBLES
#ifdef AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
	double dDivider = 1.0;
#else
#ifdef AUI_POLICY_WEIGH_BRANCH_INCLUDES_WONDER
	double dDivider = 1.0;
#endif // AUI_POLICY_WEIGH_BRANCH_INCLUDES_WONDER
#endif // AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE

	CvPolicyBranchEntry* pkPolicyBranchInfo = GC.getPolicyBranchInfo(eBranch);
	if(pkPolicyBranchInfo)
	{
#ifdef AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
		// Older branches get less weight because there have been more chances to open them
		if (!pkPolicyBranchInfo->IsLockedWithoutReligion() || pPlayer->GetReligions()->GetReligionCreatedByPlayer() == NO_RELIGION)
			dDivider = MAX(1.0, sqrt((double)pPlayer->GetCurrentEra() - pkPolicyBranchInfo->GetEraPrereq()));
#endif // AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
		
		for(int iPolicyLoop = 0; iPolicyLoop < m_pCurrentPolicies->GetPolicies()->GetNumPolicies(); iPolicyLoop++)
		{
			const PolicyTypes ePolicyLoop = static_cast<PolicyTypes>(iPolicyLoop);
			CvPolicyEntry* pkLoopPolicyInfo = GC.getPolicyInfo(ePolicyLoop);
			if(pkLoopPolicyInfo)
			{
				// Policy we don't have?
				if(!m_pCurrentPolicies->HasPolicy(ePolicyLoop))
				{
					// From this branch we are considering opening?
					if(pkLoopPolicyInfo->GetPolicyBranchType() == eBranch)
					{
#ifdef AUI_POLICY_WEIGH_BRANCH_COUNT_POLICIES_WITH_PREREQ
#ifdef AUI_POLICY_USE_DOUBLES
#ifdef AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
						dWeight += (m_PolicyAIWeights.GetWeight(iPolicyLoop) / dDivider);
#else
						dWeight += m_PolicyAIWeights.GetWeight(iPolicyLoop);
#endif // AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
#else
#ifdef AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
						iWeight += int(m_PolicyAIWeights.GetWeight(iPolicyLoop) / dDivider + 0.5);
#else
						iWeight += m_PolicyAIWeights.GetWeight(iPolicyLoop);
#endif // AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
#endif // AUI_POLICY_USE_DOUBLES
#else
						// With no prereqs?
						if(pkLoopPolicyInfo->GetPrereqAndPolicies(0) == NO_POLICY)
						{
#ifdef AUI_POLICY_USE_DOUBLES
#ifdef AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
							dWeight += (m_PolicyAIWeights.GetWeight(iPolicyLoop) / dDivider);
#else
							dWeight += m_PolicyAIWeights.GetWeight(iPolicyLoop);
#endif // AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
#else
#ifdef AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
							iWeight += int(m_PolicyAIWeights.GetWeight(iPolicyLoop) / dDivider + 0.5);
#else
							iWeight += m_PolicyAIWeights.GetWeight(iPolicyLoop);
#endif // AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
#endif // AUI_POLICY_USE_DOUBLES
						}
#endif // AUI_POLICY_WEIGH_BRANCH_COUNT_POLICIES_WITH_PREREQ
					}
				}
			}
		}

		// Add weight of free policy from branch
#ifdef AUI_POLICY_USE_DOUBLES
#ifdef AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
		dWeight += (m_PolicyAIWeights.GetWeight(pkPolicyBranchInfo->GetFreePolicy()) / dDivider);
#else
		dWeight += m_PolicyAIWeights.GetWeight(pkPolicyBranchInfo->GetFreePolicy());
#endif // AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
#else
#ifdef AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
		iWeight += int(m_PolicyAIWeights.GetWeight(pkPolicyBranchInfo->GetFreePolicy()) / dDivider + 0.5);
#else
		iWeight += m_PolicyAIWeights.GetWeight(pkPolicyBranchInfo->GetFreePolicy());
#endif // AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
#endif AUI_POLICY_USE_DOUBLES

#ifdef AUI_POLICY_WEIGH_BRANCH_INCLUDES_WONDER
		// Add weight from wonder for this branch
		dDivider *= 10;
		CvFlavorManager* pFlavorManager = pPlayer->GetFlavorManager();
		if (pFlavorManager)
		{
			for (int iI = 0; iI < GC.getNumBuildingClassInfos(); iI++)
			{
				BuildingTypes eBuildingType = (BuildingTypes)pPlayer->getCivilizationInfo().getCivilizationBuildings(iI);
				if (eBuildingType != NO_BUILDING)
				{
					CvBuildingEntry* pkBuildingInfo = GC.GetGameBuildings()->GetEntry(eBuildingType);
					if (pkBuildingInfo && pkBuildingInfo->GetPolicyBranchType() != NO_POLICY_BRANCH_TYPE && 
						(pkBuildingInfo->GetPrereqAndTech() == NO_TECH || GET_TEAM(pPlayer->getTeam()).GetTeamTechs()->HasTech((TechTypes)pkBuildingInfo->GetPrereqAndTech())))
					{
						if (!GC.getGame().isBuildingClassMaxedOut((BuildingClassTypes)pkBuildingInfo->GetBuildingClassType()))
						{
							double dWeightFromFlavor = 0.0;
							double dWeightFromVotes = 0.0;
							for (int iFlavorLoop = 0; iFlavorLoop < GC.getNumFlavorTypes(); iFlavorLoop++)
							{
								double dFlavorValue = pFlavorManager->GetPersonalityIndividualFlavor((FlavorTypes)iFlavorLoop) * pkBuildingInfo->GetFlavorValue(iFlavorLoop);
								dWeightFromFlavor += (dFlavorValue / dDivider);
							}
							// More weight for wonders that add league votes based on player's UN strategy
							if (pkBuildingInfo->GetExtraLeagueVotes() > 0)
							{
								int iFlavorDiplomacy = pPlayer->GetFlavorManager()->GetPersonalityIndividualFlavor((FlavorTypes)GC.getInfoTypeForString("FLAVOR_DIPLOMACY"));
#ifdef AUI_MINOR_CIV_RATIO
								dWeightFromVotes = pkBuildingInfo->GetExtraLeagueVotes() / dDivider * (1.01 + iFlavorDiplomacy * 2) / (0.01 + GC.getGame().getCurrentMinorCivDeviation()) *
									(double)pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS"));
#else
								dWeightFromVotes = pkBuildingInfo->GetExtraLeagueVotes() / dDivider * (1.01 + iFlavorDiplomacy * 2) / 1.01 *
									(double)pPlayer->GetGrandStrategyAI()->GetGrandStrategyPriorityRatio((AIGrandStrategyTypes)GC.getInfoTypeForString("AIGRANDSTRATEGY_UNITED_NATIONS"));
#endif // AUI_MINOR_CIV_RATIO
							}
#ifdef AUI_POLICY_USE_DOUBLES
							dWeight += MAX(dWeightFromFlavor, dWeightFromVotes);
#else
							iWeight += int(MAX(dWeightFromFlavor, dWeightFromVotes) + 0.5);
#endif // AUI_POLICY_USE_DOUBLES
						}
					}
				}
			}
		}
#endif // AUI_POLICY_WEIGH_BRANCH_INCLUDES_WONDER
	}

#ifdef AUI_POLICY_USE_DOUBLES
	return dWeight;
#else
	return iWeight;
#endif // AUI_POLICY_USE_DOUBLES
}

/// Based on game options (religion off, science off, etc.), would this branch do us any good?
bool CvPolicyAI::IsBranchEffectiveInGame(PolicyBranchTypes eBranch)
{
	CvPolicyBranchEntry* pBranchInfo = GC.getPolicyBranchInfo(eBranch);
	CvAssertMsg(pBranchInfo, "Branch info not found! Please send Anton your save file and version.");
	if (!pBranchInfo) return false;
	
	if (pBranchInfo->IsDelayWhenNoReligion())
		if (GC.getGame().isOption(GAMEOPTION_NO_RELIGION))
			return false;

	if (pBranchInfo->IsDelayWhenNoCulture())
		if (GC.getGame().isOption(GAMEOPTION_NO_POLICIES))
			return false;

	if (pBranchInfo->IsDelayWhenNoScience())
		if (GC.getGame().isOption(GAMEOPTION_NO_SCIENCE))
			return false;

	if (pBranchInfo->IsDelayWhenNoCityStates())
		if (GC.getGame().GetNumMinorCivsEver() <= 0)
			return false;

	return true;
}

#ifdef AUI_POLICY_MULTIPLY_FLAVOR_WEIGHT_FOR_UNIQUE_GREAT_PERSON
/// Returns number to multiply flavor weight by if the policy would boost generation of a unique unit great person
double CvPolicyAI::BoostFlavorDueToUniqueGP(CvPolicyEntry* pEntry)
{	
	double dMultiplier = 1;

	for (std::vector<bool>::iterator it = m_bUniqueGreatPersons.begin(); it != m_bUniqueGreatPersons.end(); ++it)
	{
		if ((*it))
		{
			switch (it - m_bUniqueGreatPersons.begin())
			{
			// Great Writer
			case 0:
				if (pEntry->GetPolicyBranchType() == (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_AESTHETICS", true /*bHideAssert*/))
				{
					dMultiplier *= 2;
				}
				if (pEntry->GetGreatWriterRateModifier() != 0)
				{
					dMultiplier *= (pEntry->GetGreatWriterRateModifier() > 0 ? 2.0 : 0.5);
				}
				break;
			// Great Artist
			case 1:
				if (pEntry->GetPolicyBranchType() == (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_AESTHETICS", true /*bHideAssert*/))
				{
					dMultiplier *= 2;
				}
				if (pEntry->GetGreatArtistRateModifier() != 0)
				{
					dMultiplier *= (pEntry->GetGreatArtistRateModifier() > 0 ? 2.0 : 0.5);
				}
				break;
			// Great Musician
			case 2:
				if (pEntry->GetPolicyBranchType() == (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_AESTHETICS", true /*bHideAssert*/))
				{
					dMultiplier *= 2;
				}
				if (pEntry->GetGreatMusicianRateModifier() != 0)
				{
					dMultiplier *= (pEntry->GetGreatMusicianRateModifier() > 0 ? 2.0 : 0.5);
				}
				break;
			// Great Scientist
			case 3:
				if (pEntry->GetPolicyBranchType() == (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_RATIONALISM", true /*bHideAssert*/))
				{
					dMultiplier *= 2;
				}
				if (pEntry->GetGreatScientistRateModifier() != 0)
				{
					dMultiplier *= (pEntry->GetGreatScientistRateModifier() > 0 ? 2.0 : 0.5);
				}
				break;
			// Great Merchant
			case 4:
				if (pEntry->GetPolicyBranchType() == (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_COMMERCE", true /*bHideAssert*/))
				{
					dMultiplier *= 2;
				}
				if (pEntry->GetGreatMerchantRateModifier() != 0)
				{
					dMultiplier *= (pEntry->GetGreatMerchantRateModifier() > 0 ? 2.0 : 0.5);
				}
				break;
			// Great Engineer
			case 5:
				if (pEntry->GetPolicyBranchType() == (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_TRADITION", true /*bHideAssert*/))
				{
					dMultiplier *= 2;
				}
				break;
			// Great General
			case 6:
				if (pEntry->GetPolicyBranchType() == (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_HONOR", true /*bHideAssert*/))
				{
					dMultiplier *= 2;
				}
				if (pEntry->GetGreatGeneralRateModifier() != 0)
				{
					dMultiplier *= (pEntry->GetGreatGeneralRateModifier() > 0 ? 2.0 : 0.5);
				}
				if (pEntry->GetDomesticGreatGeneralRateModifier() != 0)
				{
					dMultiplier *= (pEntry->GetDomesticGreatGeneralRateModifier() > 0 ? 2.0 : 0.5);
				}
				break;
			// Great Admiral
			case 7:
				if (pEntry->GetPolicyBranchType() == (PolicyBranchTypes)GC.getInfoTypeForString("POLICY_BRANCH_EXPLORATION", true /*bHideAssert*/))
				{
					dMultiplier *= 2;
				}
				if (pEntry->GetGreatAdmiralRateModifier() != 0)
				{
					dMultiplier *= (pEntry->GetGreatAdmiralRateModifier() > 0 ? 2.0 : 0.5);
				}
				break;
			}
		}
	}

	return dMultiplier;
}

/// Updates the vector for UU Great Persons if it's not been set
void  CvPolicyAI::UpdateUniqueGPVector(bool bAlwaysUpdate)
{
	if (bAlwaysUpdate || m_bUniqueGreatPersons.size() != 8)
	{
		UnitClassTypes eGPUnitClass;
		CvPlayer* pPlayer = m_pCurrentPolicies->GetPlayer();
		// Too lazy to do the next 8 steps in loops
		m_bUniqueGreatPersons.push_back(false);
		eGPUnitClass = (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_WRITER", true);
		if (eGPUnitClass != NO_UNITCLASS)
		{
			CvUnitClassInfo* pGPUnitClassInfo = GC.getUnitClassInfo(eGPUnitClass);
			if (pGPUnitClassInfo)
			{
				UnitTypes eGPUnitType = (UnitTypes)pPlayer->getCivilizationInfo().getCivilizationUnits(eGPUnitClass);
				UnitTypes eDefault = (UnitTypes)pGPUnitClassInfo->getDefaultUnitIndex();
				if (eGPUnitType != eDefault)
				{
					m_bUniqueGreatPersons.pop_back();
					m_bUniqueGreatPersons.push_back(true);
				}
			}
		}
		m_bUniqueGreatPersons.push_back(false);
		eGPUnitClass = (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_ARTIST", true);
		if (eGPUnitClass != NO_UNITCLASS)
		{
			CvUnitClassInfo* pGPUnitClassInfo = GC.getUnitClassInfo(eGPUnitClass);
			if (pGPUnitClassInfo)
			{
				UnitTypes eGPUnitType = (UnitTypes)pPlayer->getCivilizationInfo().getCivilizationUnits(eGPUnitClass);
				UnitTypes eDefault = (UnitTypes)pGPUnitClassInfo->getDefaultUnitIndex();
				if (eGPUnitType != eDefault)
				{
					m_bUniqueGreatPersons.pop_back();
					m_bUniqueGreatPersons.push_back(true);
				}
			}
		}
		m_bUniqueGreatPersons.push_back(false);
		eGPUnitClass = (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_MUSICIAN", true);
		if (eGPUnitClass != NO_UNITCLASS)
		{
			CvUnitClassInfo* pGPUnitClassInfo = GC.getUnitClassInfo(eGPUnitClass);
			if (pGPUnitClassInfo)
			{
				UnitTypes eGPUnitType = (UnitTypes)pPlayer->getCivilizationInfo().getCivilizationUnits(eGPUnitClass);
				UnitTypes eDefault = (UnitTypes)pGPUnitClassInfo->getDefaultUnitIndex();
				if (eGPUnitType != eDefault)
				{
					m_bUniqueGreatPersons.pop_back();
					m_bUniqueGreatPersons.push_back(true);
				}
			}
		}
		m_bUniqueGreatPersons.push_back(false);
		eGPUnitClass = (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_SCIENTIST", true);
		if (eGPUnitClass != NO_UNITCLASS)
		{
			CvUnitClassInfo* pGPUnitClassInfo = GC.getUnitClassInfo(eGPUnitClass);
			if (pGPUnitClassInfo)
			{
				UnitTypes eGPUnitType = (UnitTypes)pPlayer->getCivilizationInfo().getCivilizationUnits(eGPUnitClass);
				UnitTypes eDefault = (UnitTypes)pGPUnitClassInfo->getDefaultUnitIndex();
				if (eGPUnitType != eDefault)
				{
					m_bUniqueGreatPersons.pop_back();
					m_bUniqueGreatPersons.push_back(true);
				}
			}
		}
		m_bUniqueGreatPersons.push_back(false);
		eGPUnitClass = (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_MERCHANT", true);
		if (eGPUnitClass != NO_UNITCLASS)
		{
			CvUnitClassInfo* pGPUnitClassInfo = GC.getUnitClassInfo(eGPUnitClass);
			if (pGPUnitClassInfo)
			{
				UnitTypes eGPUnitType = (UnitTypes)pPlayer->getCivilizationInfo().getCivilizationUnits(eGPUnitClass);
				UnitTypes eDefault = (UnitTypes)pGPUnitClassInfo->getDefaultUnitIndex();
				if (eGPUnitType != eDefault)
				{
					m_bUniqueGreatPersons.pop_back();
					m_bUniqueGreatPersons.push_back(true);
				}
			}
		}
		m_bUniqueGreatPersons.push_back(false);
		eGPUnitClass = (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_ENGINEER", true);
		if (eGPUnitClass != NO_UNITCLASS)
		{
			CvUnitClassInfo* pGPUnitClassInfo = GC.getUnitClassInfo(eGPUnitClass);
			if (pGPUnitClassInfo)
			{
				UnitTypes eGPUnitType = (UnitTypes)pPlayer->getCivilizationInfo().getCivilizationUnits(eGPUnitClass);
				UnitTypes eDefault = (UnitTypes)pGPUnitClassInfo->getDefaultUnitIndex();
				if (eGPUnitType != eDefault)
				{
					m_bUniqueGreatPersons.pop_back();
					m_bUniqueGreatPersons.push_back(true);
				}
			}
		}
		m_bUniqueGreatPersons.push_back(false);
		eGPUnitClass = (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_GREAT_GENERAL", true);
		if (eGPUnitClass != NO_UNITCLASS)
		{
			CvUnitClassInfo* pGPUnitClassInfo = GC.getUnitClassInfo(eGPUnitClass);
			if (pGPUnitClassInfo)
			{
				UnitTypes eGPUnitType = (UnitTypes)pPlayer->getCivilizationInfo().getCivilizationUnits(eGPUnitClass);
				UnitTypes eDefault = (UnitTypes)pGPUnitClassInfo->getDefaultUnitIndex();
				if (eGPUnitType != eDefault)
				{
					m_bUniqueGreatPersons.pop_back();
					m_bUniqueGreatPersons.push_back(true);
				}
			}
		}
		m_bUniqueGreatPersons.push_back(false);
		eGPUnitClass = (UnitClassTypes)GC.getInfoTypeForString("UNITCLASS_GREAT_ADMIRAL", true);
		if (eGPUnitClass != NO_UNITCLASS)
		{
			CvUnitClassInfo* pGPUnitClassInfo = GC.getUnitClassInfo(eGPUnitClass);
			if (pGPUnitClassInfo)
			{
				UnitTypes eGPUnitType = (UnitTypes)pPlayer->getCivilizationInfo().getCivilizationUnits(eGPUnitClass);
				UnitTypes eDefault = (UnitTypes)pGPUnitClassInfo->getDefaultUnitIndex();
				if (eGPUnitType != eDefault)
				{
					m_bUniqueGreatPersons.pop_back();
					m_bUniqueGreatPersons.push_back(true);
				}
			}
		}
	}
}
#endif // AUI_POLICY_MULTIPLY_FLAVOR_WEIGHT_FOR_UNIQUE_GREAT_PERSON

/// Log all possible policy choices
void CvPolicyAI::LogPossiblePolicies()
{
	if(GC.getLogging() && GC.getAILogging())
	{
		CvString strOutBuf;
		CvString strBaseString;
		CvString strTemp;
		CvString playerName;
		CvString strDesc;

		// Find the name of this civ and city
		playerName = m_pCurrentPolicies->GetPlayer()->getCivilizationShortDescription();

		FILogFile* pLog;
		pLog = LOGFILEMGR.GetLog(GetLogFileName(playerName), FILogFile::kDontTimeStamp);

		// Get the leading info for this line
		strBaseString.Format("%03d, ", GC.getGame().getElapsedGameTurns());
		strBaseString += playerName + ", ";

		int iNumBranches = GC.getNumPolicyBranchInfos();

		// Dump out the weight of each possible policy
		for(int iI = 0; iI < m_AdoptablePolicies.size(); iI++)
		{
			int iWeight = m_AdoptablePolicies.GetWeight(iI);

			if(m_AdoptablePolicies.GetElement(iI) < iNumBranches)
			{
				strTemp.Format("Branch %d, %d", m_AdoptablePolicies.GetElement(iI), iWeight);
			}
			else
			{

				PolicyTypes ePolicy = (PolicyTypes)(m_AdoptablePolicies.GetElement(iI) - iNumBranches);
				CvPolicyEntry* pPolicyEntry = GC.getPolicyInfo(ePolicy);
				const char* szPolicyType = (pPolicyEntry != NULL)? pPolicyEntry->GetType() : "Unknown";
				strTemp.Format("%s, %d", szPolicyType, iWeight);
			}
			strOutBuf = strBaseString + strTemp;
			pLog->Msg(strOutBuf);
		}
	}
}

/// Log chosen policy
void CvPolicyAI::LogPolicyChoice(PolicyTypes ePolicy)
{
	if(GC.getLogging() && GC.getAILogging())
	{
		CvString strOutBuf;
		CvString strBaseString;
		CvString strTemp;
		CvString playerName;
		CvString strDesc;

		// Find the name of this civ and city
		playerName = m_pCurrentPolicies->GetPlayer()->getCivilizationShortDescription();

		FILogFile* pLog;
		pLog = LOGFILEMGR.GetLog(GetLogFileName(playerName), FILogFile::kDontTimeStamp);

		// Get the leading info for this line
		strBaseString.Format("%03d, ", GC.getGame().getElapsedGameTurns());
		strBaseString += playerName + ", ";

		CvPolicyEntry* pPolicyEntry = GC.getPolicyInfo(ePolicy);
		const char* szPolicyType = (pPolicyEntry != NULL)? pPolicyEntry->GetType() : "Unknown";
		strTemp.Format("CHOSEN, %s", szPolicyType);

		strOutBuf = strBaseString + strTemp;
		pLog->Msg(strOutBuf);
	}
}

/// Log chosen policy
void CvPolicyAI::LogBranchChoice(PolicyBranchTypes eBranch)
{
	if(GC.getLogging() && GC.getAILogging())
	{
		CvString strOutBuf;
		CvString strBaseString;
		CvString strTemp;
		CvString playerName;
		CvString strDesc;

		// Find the name of this civ and city
		playerName = m_pCurrentPolicies->GetPlayer()->getCivilizationShortDescription();

		FILogFile* pLog;
		pLog = LOGFILEMGR.GetLog(GetLogFileName(playerName), FILogFile::kDontTimeStamp);

		// Get the leading info for this line
		strBaseString.Format("%03d, ", GC.getGame().getElapsedGameTurns());
		strBaseString += playerName + ", ";

		strTemp.Format("CHOSEN, Branch %d", eBranch);

		strOutBuf = strBaseString + strTemp;
		pLog->Msg(strOutBuf);
	}
}

/// Logging function to write out info on Ideology choices
void CvPolicyAI::LogIdeologyChoice(CvString &decisionState, int iWeightFreedom, int iWeightAutocracy, int iWeightOrder)
{
	if(GC.getLogging() && GC.getAILogging())
	{
		CvString strOutBuf;
		CvString strBaseString;
		CvString strTemp;
		CvString playerName;

		// Find the name of this civ
		playerName = m_pCurrentPolicies->GetPlayer()->getCivilizationShortDescription();

		FILogFile* pLog;
		pLog = LOGFILEMGR.GetLog(GetLogFileName(playerName), FILogFile::kDontTimeStamp);

		// Get the leading info for this line
		strBaseString.Format("%03d, ", GC.getGame().getElapsedGameTurns());
		strBaseString += playerName + ", ";

		strTemp.Format("%s, Freedom: %d, Order: %d, Autocracy: %d", decisionState.c_str(), iWeightFreedom, iWeightOrder, iWeightAutocracy);

		strOutBuf = strBaseString + strTemp;
		pLog->Msg(strOutBuf);
	}
}

/// Build log filename
CvString CvPolicyAI::GetLogFileName(CvString& playerName) const
{
	CvString strLogName;

	// Open the log file
	if(GC.getPlayerAndCityAILogSplit())
	{
		strLogName = "PolicyAILog_" + playerName + ".csv";
	}
	else
	{
		strLogName = "PolicyAILog.csv";
	}

	return strLogName;
}