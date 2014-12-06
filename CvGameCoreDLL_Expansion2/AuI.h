// AuI.h
#pragma once

#ifndef AUI_MODS_H
#define AUI_MODS_H

// Non-AI stuff (still used by AI routines, but could be used elsewhere as well)
/// AUI's new GUID
#define AUI_GUID
/// Can cache doubles from XML (DatabaseUtility actually supports double-type, don't know why Firaxis didn't bother putting this in for good measure)
#define AUI_CACHE_DOUBLE
/// Enables the Binomial Random Number Generator
#define AUI_BINOM_RNG
/// Minor Civ tracking (especially useful for cases with no minor civs or lots of minor civs relative to major civs)
#define AUI_MINOR_CIV_RATIO
/// Turns the "Has met Major Civ" check inside GS priority checks into a public function of CvGrandStrategyAI
#define AUI_PUBLIC_HAS_MET_MAJOR
/// Implements a new function that flips a unit's UnitAIType to a certain allowed value or off of a certain value
#define AUI_UNIT_DO_AITYPE_FLIP

// A* Pathfinding Stuff
/// Adds a new function that is a middle-of-the-road fix for allowing the functions to take account of roads and railroads without calling pathfinder too often
#define AUI_ASTAR_TWEAKED_OPTIMIZED_BUT_CAN_STILL_USE_ROADS
/// A* functions now identify paradropping as a valid move
#define AUI_ASTAR_PARADROP

// AI Operations Stuff
/// Always moves out with a settler if it cannot instantly find an escort
#define AUI_OPERATION_FOUND_CITY_ALWAYS_NO_ESCORT
/// Tweaks the boldness check for whether a settler should proceed without escort
#define AUI_OPERATION_FOUND_CITY_TWEAKED_NO_ESCORT_BOLDNESS (8)
/// Adds a random value to the boldness check so it doesn't always succeed or fail
#define AUI_OPERATION_FOUND_CITY_TWEAKED_NO_ESCORT_RANDOM_VALUE (7)
#ifdef AUI_BINOM_RNG
/// If it's available, opts for the binomial RNG for the boldness check's random factor instead of the flat RNG
#define AUI_OPERATION_FOUND_CITY_TWEAKED_NO_ESCORT_RANDOM_BINOMIAL
#endif // AUI_BINOM_RNG
/// FindBestFitReserveUnit() no longer ignores units that can paradrop
#define AUI_OPERATION_FIND_BEST_FIT_RESERVE_CONSIDER_PARATROOPERS
/// Before considering units that fit the primary type first in FindBestFitReserveUnit(), "perfect match" units that fit both primary and secondary type are looked for
#define AUI_OPERATION_FIND_BEST_FIT_RESERVE_CALCULATE_PERFECT_MATCH_FIRST
/// Resets the loop iterator before the secondary unit types are considered in FindBestFitReserveUnit()
#define AUI_OPERATION_FIX_FIND_BEST_FIT_RESERVE_ITERATOR
/// GetClosestUnit() will now consider whether a unit can paradrop to the target location
#define AUI_OPERATION_GET_CLOSEST_UNIT_PARADROP
/// GetClosestUnit() will no longer terminate early after finding a single unit with "good enough" range (very important once roads start being used)
#define AUI_OPERATION_GET_CLOSEST_UNIT_NO_EARLY_BREAK
/// If two units have the same path distance in GetClosestUnit(), the one with the higher current strength wins (most effective when paired up with no early break)
#define AUI_OPERATION_GET_CLOSEST_UNIT_GET_STRONGEST
/// Fixes bugs and tweaks certain values in the FindBestTarget() function for nuke targets
#define AUI_OPERATION_TWEAKED_FIND_BEST_TARGET_NUKE

// Worker Automation Stuff
/// Automated Inca workers know that there is no maintenance on hills, so routines are adjusted as a result
#define AUI_WORKER_INCA_HILLS
/// Automated workers do not care about the build time or cost of scrubbing fallout
#define AUI_WORKER_FIX_FALLOUT
/// AI City Focus/Specialization no longer affects improvement score
#define AUI_WORKER_SCORE_PLOT_EFFECT_FROM_CITY_FOCUS (0)
/// Divides score for improvement if built for a puppeted city
#define AUI_WORKER_SCORE_PLOT_REDUCED_PUPPET_SCORE (2)
/// Returns score of 0 for improvement if built for a city being razed
#define AUI_WORKER_SCORE_PLOT_NO_SCORE_FROM_RAZE
/// Embeds flavors and plot yield multipliers into the ScorePlot() function (copied from the chop directives function), value is base yield value
#define AUI_WORKER_SCORE_PLOT_FLAVORS (2.0)
/// If building an improvement also generates flat hammers, consider the effect as flat +parameter hammer yield
#define AUI_WORKER_SCORE_PLOT_CHOP (1.0)
/// Removes the bias to chop forests after optics (since it doesn't actually offer a gameplay improvement
#define AUI_WORKER_NO_CHOP_BIAS
/// Faith now affects tile evaluation for workers, it pulls from culture multiplier though
#define AUI_WORKER_EVALUATE_FAITH
/// For improvement evaluation, leader flavor now have ln() taken before being multiplied by everything else; this reduces cases were leader flavor makes a huge difference in worker automation logic 
#define AUI_WORKER_LOGARITHMIC_FLAVOR
/// Automated Dutch workers now remove marshes on tiles with resources (since polders won't be built anyway)
#define AUI_WORKER_DUTCH_MARSH_RESOURCES
/// AI Celt workers will no longer leave forests unimproved once they enter the Industrial Era
#define AUI_WORKER_CELT_FOREST_IMPROVE_INDUSTRIAL "ERA_INDUSTRIAL"
/// Automated workers value strategic resources that a player has none of higher than strategic resources that the player has used all of
#define AUI_WORKER_TWEAKED_DONT_HAVE_MULTIPLIER (6)
/// Combat workers will increase the maximum allowed plot danger value to their current strength considering HP
#define AUI_WORKER_SHOULD_BUILDER_CONSIDER_PLOT_MAXIMUM_DANGER_BASED_ON_UNIT_STRENGTH
/// FindTurnsAway() no longer returns raw distance, parameter dictates whether we're reusing paths and ignoring units (fast but rough) or not (slow but accurate)
#define AUI_WORKER_FIND_TURNS_AWAY_USES_PATHFINDER (true)

// City Stuff
/// Shifts the scout assignment code to EconomicAI
#define AUI_CITY_FIX_CREATE_UNIT_EXPLORE_ASSIGNMENT_TO_ECONOMIC
/// Reenables the purchasing of buildings with gold (originally from Ninakoru's Smart AI, but heavily modified since)
#define AUI_CITY_FIX_BUILDING_PURCHASES_WITH_GOLD

// City Citizens Stuff
/// Unhardcodes the value assigned to specialists for great person points (flat value is the base multiplier for value of a single GP point before modifications)
#define AUI_CITIZENS_UNHARDCODE_SPECIALIST_VALUE_GREAT_PERSON_POINTS (2)
/// Unhardcodes the value assigned to specialists for happiness (flat value is the base multiplier for value of a single happiness point before modifications)
#define AUI_CITIZENS_UNHARDCODE_SPECIALIST_VALUE_HAPPINESS (8)
/// Extra food value assigned to specialists for half food consumption now depends on the XML value for citizen food consumption (instead of assuming the default value)
#define AUI_CITIZENS_FIX_SPECIALIST_VALUE_HALF_FOOD_CONSUMPTION

// City Strategy Stuff
/// Scales the GetLastTurnWorkerDisbanded() computation to game speed
#define AUI_CITYSTRATEGY_FIX_TILE_IMPROVERS_LAST_DISBAND_WORKER_TURN_SCALE

// Danger Plots Stuff
/// Better danger calculation for ranged units (originally from Ninakoru's Smart AI, but heavily modified since)
#define AUI_DANGER_PLOTS_TWEAKED_RANGED
/// Majors will always "see" barbarians in tiles that have been revealed when plotting danger values (kind of a cheat, but it's a knowledge cheat, so it's OK-ish)
#define AUI_DANGER_PLOTS_SHOULD_IGNORE_UNIT_MAJORS_SEE_BARBARIANS_IN_FOG

// DiplomacyAI Stuff
/// If the first adjusted value is out of bounds, keep rerolling with the amount with which it is out of bounds until we remain in bounds
#define AUI_DIPLOMACY_GET_RANDOM_PERSONALITY_WEIGHT_USE_REROLLS
#ifdef AUI_BINOM_RNG
/// When modifying a personality value (eg. Boldness, Wonder Competitiveness), the AI will use the binomial RNG for a normal distribution instead of a flat one
#define AUI_DIPLOMACY_GET_RANDOM_PERSONALITY_WEIGHT_USES_BINOM_RNG
/// When rolling about whether to contact a player with a statement or not, the AI will use the binomial RNG for a normal distribution instead of a flat one
#define AUI_DIPLOMACY_DO_STATEMENT_USES_BINOM_RNG (7)
/// When adding an extra, random value to the score of a possible cooperative war, the AI will use the binomial RNG for a normal distribution instead of a flat one
#define AUI_DIPLOMACY_GET_COOP_WAR_SCORE_USES_BINOM_RNG (5)
/// When adding an extra, random value to the score of a possible cooperative war, the maximum value added will be the AI's boldness instead of a set number
#define AUI_DIPLOMACY_GET_COOP_WAR_SCORE_MAX_RANDOM_VALUE_IS_BOLDNESS
/// When adding an extra, random value to the turns the AI will wait before submitting to another demand, the AI will use the binomial RNG for a normal distribution instead of a flat one
#define AUI_DIPLOMACY_DO_DEMAND_MADE_USES_BINOM_RNG
/// When adding an extra, random value to the score of whether a request not to settle near lands is acceptable, the AI will use the binomial RNG for a normal distribution instead of a flat one
#define AUI_DIPLOMACY_IS_DONT_SETTLE_ACCEPTABLE_USES_BINOM_RNG
/// When adding an extra, random value to the score of whether a DoF is acceptable, the AI will use the binomial RNG for a normal distribution instead of a flat one
#define AUI_DIPLOMACY_IS_DOF_ACCEPTABLE_USES_BINOM_RNG (5)
/// When adding an extra, random value to the score of whether to denounce a player, the AI will use the binomial RNG for a normal distribution instead of a flat one
#define AUI_DIPLOMACY_GET_DENOUNCE_WEIGHT_USES_BINOM_RNG (5)
#endif // AUI_BINOM_RNG

// EconomicAI Stuff
/// VITAL FOR MOST FUNCTIONS! Use float instead of int for certain variables (to retain information during division)
#define AUI_ECONOMIC_USE_DOUBLES
#ifdef AUI_CITY_FIX_CREATE_UNIT_EXPLORE_ASSIGNMENT_TO_ECONOMIC
/// Assigning non-scout units to become explorers is now done in EconomicAI instead of at City
#define AUI_ECONOMIC_FIX_DO_RECON_STATE_EXPLORE_ASSIGNMENT_FROM_CITY
#endif // AUI_CITY_FIX_CREATE_UNIT_EXPLORE_ASSIGNMENT_TO_ECONOMIC
/// Resets the UnitLoop iterator before looping through units (would otherwise cause problems when both land and sea exploration was in the "enough" state)
#define AUI_ECONOMIC_FIX_DO_RECON_STATE_ITERATOR
/// The "skip first" code to remove scouting non-scouts only works if we don't actually have any dedicated scout units
#define AUI_ECONOMIC_FIX_DO_RECON_STATE_SKIP_FIRST_ONLY_IF_NO_EXPLORERS
/// The code to remove scouting non-scouts is more generalized (ie. doesn't depend on UNITAI_ATTACK)
#define AUI_ECONOMIC_FIX_DO_RECON_STATE_MALLEABLE_REMOVE_NONSCOUTS
/// Adds a setter for m_iLastTurnWorkerDisbanded (so if a worker is disbanded in another class, they can reference this)
#define AUI_ECONOMIC_SETTER_LAST_TURN_WORKER_DISBANDED
/// Checks that would force the function to return false happen earlier in the function
#define AUI_ECONOMIC_EARLY_EXPANSION_TWEAKED_EARLIER_CHECKS
/// Squares the amount of expansion flavor applied to get the desired amount of cities if the AI is the only major on its starting landmass
#define AUI_ECONOMIC_EARLY_EXPANSION_BOOST_EXPANSION_FLAVOR_IF_ALONE
/// Multiplies the desired city count by the ratio of the natural logs of expansion and growth flavors, with difficulty acting as an extra multiplier to expansion flavor
#define AUI_ECONOMIC_EARLY_EXPANSION_TWEAKED_FLAVOR_APPLICATION
/// Scales the desired amount of cities by the amount of major and minor players on the map
#define AUI_ECONOMIC_EARLY_EXPANSION_SCALE_BY_PLAYER_COUNT
/// Removes the check for a cultural grand strategy that's a holdover from pre-BNW when cultural victories were won through policies, not tourism
#define AUI_ECONOMIC_FIX_EXPAND_LIKE_CRAZY_REMOVE_HOLDOVER_CULTURE_CHECK
#ifdef AUI_CITY_FIX_BUILDING_PURCHASES_WITH_GOLD
/// Reenables the DoHurry() function, but using a reworked method instead of the original (originally from Ninakoru's Smart AI, but heavily modified since)
#define AUI_ECONOMIC_FIX_DO_HURRY_REENABLED_AND_REWORKED
#endif // AUI_CITY_FIX_BUILDING_PURCHASES_WITH_GOLD

// Flavor Manager Stuff
/// Players that start as human no longer load in default flavor values
#define AUI_FLAVOR_MANAGER_HUMANS_GET_FLAVOR
/// Adds a new function that randomizes weights like at the start of the game, but at half the usual value
#define AUI_FLAVOR_MANAGER_RANDOMIZE_WEIGHTS_ON_ERA_CHANGE
/// Randomization gets applied twice for all flavors that influence a Grand Strategy
#define AUI_FLAVOR_MANAGER_RANDOMIZE_WEIGHTS_APPLY_RANDOM_ON_GS_FLAVOR_TWICE
/// Fixes the "zero'ed out flavor" check to still accept 0 as a possible flavor value, but not accept negative values
#define AUI_FLAVOR_MANAGER_FIX_RANDOMIZE_WEIGHTS_ZEROED_OUT_FLAVOR
/// Fixes the function messing up and returning the wrong adjustment when the value to be added is actually negative (eg. for minor civs)
#define AUI_FLAVOR_MANAGER_FIX_GET_ADJUSTED_VALUE_NEGATIVE_PLUSMINUS
/// If the first adjusted value is out of bounds, keep rerolling with the amount with which it is out of bounds until we remain in bounds
#define AUI_FLAVOR_MANAGER_GET_ADJUSTED_VALUE_USE_REROLLS
#ifdef AUI_BINOM_RNG
/// When adding or subtracting flavor value, the binomial RNG is used to generate a normal distribution instead of a flat one
#define AUI_FLAVOR_MANAGER_GET_ADJUSTED_VALUE_USES_BINOM_RNG
#endif // AUI_BINOM_RNG

// Grand Strategy Stuff
/// Enables use of Grand Strategy Priority; the function returns the ratio of the requested GS's priority to the active GS's priority (1.0 if the requested GS is the active GS)
#define AUI_GS_PRIORITY_RATIO
/// VITAL FOR MOST FUNCTIONS! Use float instead of int for certain variables (to retain information during division)
#define AUI_GS_USE_DOUBLES
/// Uses a completely different system to calculate priorities for all Grand Strategies; WIP, XML hooks TBA
//#define AUI_GS_PRIORITY_OVERHAUL
/// Uses tweaked algorithm for Conquest GS priority's increase depending on general approach and current era
#define AUI_GS_CONQUEST_TWEAKED_ERAS
/// Uses tweaked algorithm for military ratio's effect on Conquest GS
#define AUI_GS_CONQUEST_TWEAKED_MILITARY_RATIO
/// Fixes the code that checks for cramped status (it always triggered originally, now it only triggers if we really are cramped)
#define AUI_GS_CONQUEST_FIX_CRAMPED
/// Ignores the fact that the enemy has nukes when calculating Conquest GS priority
#define AUI_GS_CONQUEST_IGNORE_ENEMY_NUKES
/// Uses tweaked algorithm for Culture GS priority's increase depending on flavors and current era
#define AUI_GS_CULTURE_TWEAKED_ERAS
/// Uses tweaked algorithm for calculating culture and tourism's effect on Culture GS
#define AUI_GS_CULTURE_TWEAKED_CULTURE_TOURISM_AHEAD
/// Uses tweaked algorithm for United Nations GS priority's increase depending on flavors and current era
#define AUI_GS_DIPLOMATIC_TWEAKED_ERAS
/// Uses tweaked algorithm for Spaceship GS priority's increase depending on flavors and current era
#define AUI_GS_SPACESHIP_TWEAKED_ERAS
/// For Spaceship GS, algorithm that processes additional flavors from Expansion, Production, and Growth gets added to Science flavor before final computation
#define AUI_GS_SPACESHIP_TWEAKED_FLAVORS
/// Additional Spaceship GS flavor based on Tech Ratio
#define AUI_GS_SPACESHIP_TECH_RATIO
/// GS does not instantly have full influence over this function until later into the game; increase is sinusoid
#define AUI_GS_SINUSOID_PERSONALITY_INFLUENCE
/// Multiplies the science flavor of buildings, wonders, and techs depending on how well the tech requirements of significant GS's is met (eg. have Archaeology, have Internet, etc.)
#define AUI_GS_SCIENCE_FLAVOR_BOOST (8)
/// Replaces the logging function with one that allows for easy creation of graphs within Excel
#define AUI_GS_BETTER_LOGGING

// Homeland AI Stuff
/// Adds a new function that lets aircraft go on intercept missions (originally from Ninakoru's Smart AI)
#define AUI_HOMELAND_AIRCRAFT_INTERCEPTIONS
/// Dials up priority for upgrading units by 50x every other turn primarily to help upgrade air units (originally from Ninakoru's Smart AI with slight modification)
#define AUI_HOMELAND_ESTABLISH_HOMELAND_PRIORITIES_50X_UPGRADE_PRIORITY_EVERY_OTHER_TURN
/// Disables the code that would start fortifying scouts if recon state was set as "enough"
#define AUI_HOMELAND_ALWAYS_MOVE_SCOUTS
/// Tweaks the algorithm for Plot Heal Moves to keep March promotions in mind and make sure we don't overheal if we're under threat
#define AUI_HOMELAND_TWEAKED_HEAL_MOVES
/// Changes the AcceptableDanger value in PlotDangerMoves to be a function of the unit's current HP percent
#define AUI_HOMELAND_TWEAKED_ACCEPTABLE_DANGER (1.0)
/// When retreating civilians to a safe plot, unit strength and city strength values are put to the power of this value
#define AUI_HOMELAND_TWEAKED_MOVE_CIVILIAN_TO_SAFETY_POW_ALLIED_STRENGTH (2.0)
/// When finding patrol targets for civilian units, subtract off danger value from plot score
#define AUI_HOMELAND_TWEAKED_FIND_PATROL_TARGET_CIVILIAN_NO_DANGER
#ifdef AUI_ASTAR_PARADROP
/// Paradropping is now inserted into relevant tactical moves and the AI will execute them when favorable
#define AUI_HOMELAND_PARATROOPERS_PARADROP
#endif // AUI_ASTAR_PARADROP
/// Flavors that weren't previously fetched but were still (attempted to be) used in processing later are now fetched
#define AUI_HOMELAND_FIX_ESTABLISH_HOMELAND_PRIORITIES_MISSING_FLAVORS
#ifdef AUI_UNIT_DO_AITYPE_FLIP
/// Allows units that can act as Sea Workers to flip to or from the UnitAIType using the MilitaryAI helper function
#define AUI_HOMELAND_PLOT_SEA_WORKER_MOVES_EMPLOYS_AITYPE_FLIP
/// Attempts to flip a scout's UnitAIType if it has no target
#define AUI_HOMELAND_EXECUTE_EXPLORER_MOVES_FLIP_AITYPE_ON_NOTARGET
#endif // AUI_UNIT_DO_AITYPE_FLIP
/// Allows units that can act as Workers to flip to or from the UnitAIType using the MilitaryAI helper function
#define AUI_HOMELAND_PLOT_WORKER_MOVES_EMPLOYS_AITYPE_FLIP
/// Disbanding explorers now uses the scrap() function instead of the kill() function
#define AUI_HOMELAND_FIX_EXECUTE_EXPLORER_MOVES_DISBAND
#ifdef AUI_ECONOMIC_SETTER_LAST_TURN_WORKER_DISBANDED
/// If a worker is idle and we have extra workers, disband him instead of sending him to the safest plot
#define AUI_HOMELAND_PLOT_WORKER_MOVES_DISBAND_EXTRA_IDLE_WORKERS
#endif // AUI_ECONOMIC_SETTER_LAST_TURN_WORKER_DISBANDED
/// This function is just filled with holes; I've now fixed (most of) them!
#define AUI_HOMELAND_FIX_EXECUTE_AIRCRAFT_MOVES
/// Units in armies that are waiting around are now eligible for upgrading
#define AUI_HOMELAND_FIX_PLOT_UPGRADE_MOVES_APPLIES_TO_WAITING_ARMIES
/// The AI will no longer stop upgrading units once it has upgraded more units in a turn than its Military Training Flavor
#define AUI_HOMELAND_PLOT_UPGRADE_MOVES_NO_COUNT_LIMIT
/// Stops the AI from suiciding units by embarking them onto tiles that can be attacked
#define AUI_HOMELAND_FIX_EXECUTE_MOVES_TO_SAFEST_PLOT_NO_EMBARK_SUICIDE
/// Disbands archaeologists if there are no more sites available (originally from Ninakoru's Smart AI)
#define AUI_HOMELAND_EXECUTE_ARCHAEOLOGIST_MOVES_DISBAND_IF_NO_AVAILABLE_SITES

// Military AI Stuff
/// VITAL FOR MOST FUNCTIONS! Use double instead of int for certain variables (to retain information during division)
#define AUI_MILITARY_USE_DOUBLES
/// Capitals will always be included in the list of potential targets if conquest victory is enabled
#define AUI_MILITARY_ALWAYS_TARGET_CAPITALS
/// When scoring targets by distance, use a continuum system instead of different tiers
#define AUI_MILITARY_TARGET_WEIGHT_DISTANCE_CONTINUUM
/// Applies flavor and current needs when checking the economic value of a target city
#define AUI_MILITARY_TARGET_WEIGHT_ECONOMIC_FLAVORED
/// Fixes bad code for visible barbarian units adding to "barbarian threat" value
#define AUI_MILITARY_FIX_BARBARIAN_THREAT
/// Adds barbarian threat to fMultiplier in addition to highest player threat and offense/defense flavors
#define AUI_MILITARY_UNITS_WANTED_ADD_BARBARIAN_THREAT
/// Squares the value added by player threat (and barbarian threat, if it's enabled) to fMultiplier
#define AUI_MILITARY_UNITS_WANTED_SQUARE_THREATS
/// Tweaks Disband Obsolete Units function to work more often, more intelligently, and also when player does not have negative GPT but still has obsolete units
#define AUI_MILITARY_DISBAND_OBSOLETE_TWEAKED
/// Tweaks Find Best Unit to Disband to work on more units and give higher priority to obsolete units
#define AUI_MILITARY_FIND_BEST_UNIT_TO_SCRAP_TWEAKED
/// Adds a function to calculate the maximum number of possible intercepts on a tile (originally from Ninakoru's Smart AI)
#define AUI_MILITARY_MAX_INTERCEPTS
/// No longer uses a fixed range for the GetNumEnemyAirUnitsInRange() function (originally from Ninakoru's Smart AI)
#define AUI_MILITARY_NUM_AIR_UNITS_IN_RANGE_DYNAMIC_RANGE
#ifdef AUI_UNIT_DO_AITYPE_FLIP
/// Adds a function that will check for and flip the UnitAITypes of all units (eg. to/from worker AI, to/from explore AI, etc.)
#define AUI_MILITARY_AITYPE_FLIP
#endif // AUI_UNIT_DO_AITYPE_FLIP
/// Fixes a bug that would only put a log header in one AI's log, not all their logs
#define AUI_MILITARY_BETTER_LOGGING
/// This function is just filled with holes; I've now fixed (most of) them!
#define AUI_MILITARY_FIX_WILL_AIR_UNIT_REBASE
/// Adds the difference between players' nukes to the nuke flavor for nuke rolls
#define AUI_MILITARY_ROLL_FOR_NUKES_CONSIDER_NUKE_COUNT
/// Adds the difference between players' strengths to the nuke flavor for nuke rolls
#define AUI_MILITARY_ROLL_FOR_NUKES_CONSIDER_STRENGTH_DIFFERENCE

// Player Stuff (PlayerAI is later)
/// Fixes AI Shoshone Pathfinders not getting any goody hut bonuses (TODO: have AI Shoshone actually choose their goody hut bonus instead of getting a random one)
#define AUI_PLAYER_FIX_GOODY_HUT_PICKER
/// Fixes GetNumUnitsWithUnitAI() counting units with delayed death (ie. they will die this turn) or are dead, but still in the list of the player's units
#define AUI_PLAYER_FIX_GET_NUM_UNITS_WITH_UNITAI_NO_DEAD
/// Fixes the score divider caused by enemies on a continent if not a naval expansion map to work the other way around: now a multiplier for when there are no enemies and is a naval expansion map
#define AUI_PLAYER_FIX_BEST_SETTLE_AREAS_NAVAL_EXPAND_HINT (3)
/// When the settler does not have an escort, its evaluation distance is not lowered as much depending on boldness and the current era
#define AUI_PLAYER_GET_BEST_SETTLE_PLOT_NO_ESCORT_BOLDNESS
/// Checks adjacent tiles for being targetted for city settling as well as the current one
#define AUI_PLAYER_GET_BEST_SETTLE_PLOT_CHECK_ADJACENT_FOR_CITY_TARGET
/// The evaluation distance depends on the distance from the player's city closest to the settler instead of the distance from the settler
#define AUI_PLAYER_GET_BEST_SETTLE_PLOT_EVALDISTANCE_FOR_CLOSEST_CITY
/// Calls a pathfinder instead of scaling values depending on whether or not the plot is on a different landmass
#define AUI_PLAYER_GET_BEST_SETTLE_PLOT_PATHFINDER_CALL

// PlayerAI Stuff
/// Great prophet will be chosen as a free great person if the AI can still found a religion with them
#define AUI_PLAYERAI_FREE_GP_EARLY_PROPHET
/// Venice will often choose a Great Merchant for its free GP irrespective of its wonderlust or Grand Strategy
#define AUI_PLAYERAI_FREE_GP_VENETIAN_MERCHANT
/// Lets the AI choose their free cultural GP based on the number of great work slots it has available
#define AUI_PLAYERAI_FREE_GP_CULTURE
/// Scales the wonder competitiveness check for choosing a free engineer instead of it being a binary thing
#define AUI_PLAYERAI_FREE_GP_DYNAMIC_WONDER_COMPETITIVENESS (10)
/// When deciding whether to raze a city, AI_conquerCity() now considers free and/or cheaper courthouses and has a lowerered happiness bar
#define AUI_PLAYERAI_CONQUER_CITY_TWEAKED_RAZE
/// do_annex will now terminate for ineligible civs (Venice, City States) much quicker (optimization)
#define AUI_PLAYERAI_DO_ANNEX_QUICK_FILTER
/// do_annex will now properly consider whether the AI gets courthouses for free
#define AUI_PLAYERAI_DO_ANNEX_CONSIDERS_FREE_COURTHOUSE
/// do_annex will no longer consider whether or not the player is going for a cultural victory (holdover from pre-BNW)
#define AUI_PLAYERAI_DO_ANNEX_IGNORES_CULTURAL_STRATEGY
/// do_annex is can now trigger even when the empire is unhappy under specific circumstances
#define AUI_PLAYERAI_DO_ANNEX_MORE_AGGRESSIVE
/// do_annex will no longer stop after it annexes the player's own capital
#define AUI_PLAYERAI_FIX_DO_ANNEX_NO_STOP_AFTER_CAPITAL_ANNEX
/// do_annex's resistance check will occur before a city is qualified, not after
#define AUI_PLAYERAI_FIX_DO_ANNEX_CHECK_FOR_RESISTANCE
/// Great Prophets will now create shrines more often if the player has unlocked piety
#define AUI_PLAYERAI_TWEAKED_GREAT_PROPHET_DIRECTIVE
/// Requires the AI to be influential with more civs before concert tour is prioritized before creating a great work
#define AUI_PLAYERAI_GREAT_MUSICIAN_DIRECTIVE_HIGHER_INFLUENTIALS_REQUIRED_BEFORE_CONCERT_TOUR_PRIORITY
/// Slight tweak to logic to make the function's mathematics more accurate
#define AUI_PLAYERAI_TWEAKED_GREAT_ENGINEER_DIRECTIVE
/// Uses different logic to check when a player is venice
#define AUI_PLAYERAI_GREAT_MERCHANT_DIRECTIVE_TWEAKED_VENICE_CHECK
/// Slight tweak to logic to make the function's mathematics more accurate; value is simply a boost from if it's still the first quarter of the game to if it's still the first half of the game
#define AUI_PLAYERAI_TWEAKED_GREAT_SCIENTIST_DIRECTIVE (2)
/// Makes the number of cities Venice desires a function of various parameters instead of just a set constant (same parameters as Early Expansion economic strategy)
#define AUI_PLAYERAI_TWEAKED_VENICE_CITY_TARGET
/// Disables Reuse Paths for pathfinding functions
#define AUI_PLAYERAI_NO_REUSE_PATHS_FOR_TARGET_PLOTS
/// Additional filters applied for Venice, eg. higher priority if city is coastal, lower priority if city is ally, priority based on distance to nearest ally city instead of path
#define AUI_PLAYERAI_FIND_BEST_MERCHANT_TARGET_PLOT_VENICE_FILTERS
/// When updating the settle value of a landmass with a new plot, MAX() is used instead of addition
#define AUI_PLAYERAI_FIX_UPDATE_FOUND_VALUES_NOT_ADDITIVE

// Plot Stuff
/// If a plot is unowned, CalculateNatureYield() will assume the plot is owned by a future player
#define AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_FUTURE_OWNER_IF_UNOWNED

// Policy Stuff
/// VITAL FOR MOST FUNCTIONS! Use double instead of int for certain variables (to retain information during division)
#define AUI_POLICY_USE_DOUBLES
/// When weighing a branch for unlocking, the difference between the player's era and the branch's prerequisite era is taken into account
#define AUI_POLICY_WEIGH_BRANCH_INCLUDES_ERA_DIFFERENCE
/// When weighing a branch for unlocking, the wonder the branch unlocks is also weighed into the branch flavor, but only if the wonder can still be constructed
#define AUI_POLICY_WEIGH_BRANCH_INCLUDES_WONDER
/// When weighing a branch for unlocking, all policies in the branch will be considered, not just ones without prerequisites
#define AUI_POLICY_WEIGH_BRANCH_COUNT_POLICIES_WITH_PREREQ
/// Doubles Happiness flavor weight if the civ is currently unhappy
#define AUI_POLICY_MULTIPLY_HAPPINESS_WEIGHT_WHEN_UNHAPPY (2)
/// If we have a great person UU, boost all the weights of any policy with effects that up its generation, including policies that are in branches that enable its faith purchase
#define AUI_POLICY_MULTIPLY_FLAVOR_WEIGHT_FOR_UNIQUE_GREAT_PERSON (2)
/// Divides the weight of religion flavor by this number if we have not founded a pantheon or if we've founded a pantheon but there are no more religions to found
#define AUI_POLICY_DIVIDE_RELIGION_WEIGHT_WHEN_NO_RELIGION (2)
/// Divides the weight of military flavors (offense, defense, military training) by this number if we do not have any policies
#define AUI_POLICY_DIVIDE_MILITARY_WEIGHT_FOR_OPENER (4)
/// Replaces the divider for opening new branches with a "determination" divider based on the geometric mean of grand strategy ratios (old one was a holdover from pre-BNW)
#define AUI_POLICY_CHOOSE_NEXT_POLICY_TWEAKED_OPEN_NEW_BRANCH
/// Slight tweak to logic of ClearPrefs part to make the function's mathematics more accurate
#define AUI_POLICY_DO_CHOOSE_IDEOLOGY_TWEAKED_CLEAR_PREFS
#ifdef AUI_BINOM_RNG
/// The binomial RNG is used for the extra random score given to all ideologies; value is the maximum amount an ideology can receive from the random portion
#define AUI_POLICY_DO_CHOOSE_IDEOLOGY_USES_BINOM_RNG (10)
#endif // AUI_BINOM_RNG
/// Implements a tiebreaker that adds random values to all ideologies until there is a clear winner
#define AUI_POLICY_DO_CHOOSE_IDEOLOGY_TIEBREAKER
/// Slight tweak to logic of ClearPrefs part to make the function's mathematics more accurate; ClearPrefs also gets overwritten if empire is very unhappy
#define AUI_POLICY_DO_CONSIDER_IDEOLOGY_SWITCH_TWEAKED_CLEAR_PREFS
/// Gets all possible happiness sources the branch can give, not just building-based ones (eg. specialists, trade routes, luxuries, etc.)
#define AUI_POLICY_GET_BRANCH_BUILDING_HAPPINESS_GET_ALL_HAPPINESS_SOURCES

// Religion/Belief Stuff
/// Checks all of player's cities for whether or not a city is within conversion target range, not just the player's capital
#define AUI_RELIGION_CONVERSION_TARGET_NOT_JUST_CAPITAL
/// If the AI's religion now unlocks multiple faith buildings, AI can now purchase all of them
#define AUI_RELIGION_FIX_MULTIPLE_FAITH_BUILDINGS
/// Actually checks the cost of purchasing great people at a city instead of just returning the oldest city founded (deactivated while this is equal across all cities)
// #define AUI_RELIGION_FIX_BEST_CITY_HELPER_GREAT_PERSON

// Site Evaluation Stuff
/// Tweaks the multiplier given to the happiness score luxury resources that the player does not have (multiplier is applied once for importing, twice and divided by 3 for don't have at all)
#define AUI_SITE_EVALUATION_COMPUTE_HAPPINESS_VALUE_TWEAKED_UNOWNED_LUXURY_MULTIPLIER (15)
/// Multiplies the settling value of coastal plots if we do not have a coastal city yet
#define AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FIRST_COASTAL_CITY_MULTIPLIER (2)
/// The flavor multiplier is now affected by grand strategies
#define AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_FLAVOR_MULTIPLIER_USES_GRAND_STRATEGY
/// When calculating flavor multipliers, the flavor input for the next city specialization gets multiplied by this number
#define AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_NEXT_CITY_SPECIALIZATION_FLAVOR_MULTIPLIER 1 / 2
/// A tile's culture yield is also considered when scoring it
#define AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_CONSIDER_CULTURE
/// Changes the amount the plot value is divided by if we already own the tile (from 4)
#define AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_TWEAKED_ALREADY_OWNED_DIVIDER (2)
/// The minimum "sweet spot" distance from another allied city is now this value (from 4); remember, the actual intercity distance will be this value minus one
#define AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_TWEAKED_MINIMUM_SWEET_SPOT (5)
/// The value of a plot is now multiplied by this value if it is closer than the sweet spot (from 1/2)
#define AUI_SITE_EVALUATION_PLOT_FOUND_VALUE_TWEAKED_CLOSER_THAN_SWEET_SPOT_MULTIPLIER 2 / 3
/// Resets the faith value of a tile back to 0 (like for all other yields) at the start of each loop
#define AUI_SITE_EVALUATION_FIX_PLOT_FOUND_VALUE_RESET_FAITH_VALUE_EACH_LOOP
/// Identification of a possible choke point is now much more accurate
#define AUI_SITE_EVALUATION_COMPUTE_STRATEGIC_VALUE_TWEAKED_CHOKEPOINT_CALCULATION

// Tactical AI Stuff
/// VITAL FOR MOST FUNCTIONS! Use double instead of int for certain variables (to retain information during division)
#define AUI_TACTICAL_USE_DOUBLES
/// Adds new functions that help with being able to conduct Air Sweeps and Intercepts
#define AUI_TACTICAL_HELPERS_AIR_SWEEP
/// Adds new functions that help with positioning units and ordering possible units to position
#define AUI_TACTICAL_HELPERS_POSITIONING_AND_ORDER
/// Adds a new function that is similar to ExecuteMoveToPlot(), but swaps two units instead (which is eg. useful when moving blocking units out of the way)
#define AUI_TACTICAL_EXECUTE_SWAP_TO_PLOT
/// Changes the AcceptableDanger value in PlotDangerMoves to be a function of the unit's current HP percent
#define AUI_TACTICAL_TWEAKED_ACCEPTABLE_DANGER (1.0)
/// Multiply total range strength with average range strength when considering ranged dominance
#define AUI_TACTICAL_SELECT_POSTURE_CONSIDER_AVERAGE_RANGED_STRENGTH
/// More aggressive city attack postures will consider melee unit count before executing
#define AUI_TACTICAL_CITY_ATTACK_POSTURE_CONSIDERS_MELEE_COUNT (1)
/// Priority randomizer is now applied to tactical moves for non-barbarians players as well
#define AUI_TACTICAL_USE_RANDOM_FOR_NON_BARBARIANS
#ifdef AUI_BINOM_RNG
/// Uses the binomial RNG for the randomness factor in tactical move priority
#define AUI_TACTICAL_TWEAKED_MOVE_PRIORITIES_RANDOM_BINOMIAL
#endif // AUI_BINOM_RNG
/// Tweaks capture/damage city moves so that ranged attacks aren't wasted on cities with 1 HP (originally from Ninakoru's Smart AI)
#define AUI_TACTICAL_TWEAKED_CAPTURE_DAMAGE_CITY_MOVES
/// Uses Boldness to determine whether long sieges are worth it instead of a set 8 turns
#define AUI_TACTICAL_TWEAKED_DAMAGE_CITY_MOVE_USE_BOLDNESS
/// Sets a different threshold for the amount of damage an enemy unit is required to take before the AI is willing to send its units on suicide attacks
#define AUI_TACTICAL_TWEAKED_DESTROY_UNIT_MOVE_SUICIDE_THRESHOLD (50)
/// Tweaks the algorithm for Plot Heal Moves to keep March promotions in mind and make sure we don't overheal if we're under threat
#define AUI_TACTICAL_TWEAKED_HEAL_MOVES
/// Uses a different algorithm for plotting intercept moves instead of relying on dominance zones (originally from Ninakoru's Smart AI)
#define AUI_TACTICAL_TWEAKED_AIR_INTERCEPT
#ifdef AUI_TACTICAL_HELPERS_POSITIONING_AND_ORDER
/// Reorders the parts of the ExecuteAttack() algorithm to push ranged units before melee ones (originally from Ninakoru's Smart AI, heavily modified since)
#define AUI_TACTICAL_TWEAKED_EXECUTE_ATTACK
#ifdef AUI_TACTICAL_TWEAKED_EXECUTE_ATTACK
/// If the target is a city, add the city's planned ranged attack to the expected self damage of melee units
#define AUI_TACTICAL_TWEAKED_EXECUTE_ATTACK_NO_MELEE_SUICIDE_AGAINST_CITY
/// Postpones the MoveToEmptySpaceNearTarget() parts of the code until after ranged attacks have processed so ranged units can move into range and fire
#define AUI_TACTICAL_TWEAKED_EXECUTE_ATTACK_POSTPONE_MELEE_MOVE
/// If a melee unit cannot move adjacent to the target, attempt to move within two tiles of target
#define AUI_TACTICAL_TWEAKED_EXECUTE_ATTACK_MELEE_MOVE_ALLOWS_OUTER_RING
#endif // AUI_TACTICAL_TWEAKED_EXECUTE_ATTACK
#endif // AUI_TACTICAL_HELPERS_POSITIONING_AND_ORDER
/// Fix for units that can attack multiple times per turn so they can now participate in the same ExecuteAttackMove()
#define AUI_TACTICAL_FIX_EXECUTE_ATTACK_BLITZ
/// Sets a very high danger value for water tiles if a unit needs to embark onto the tile (originally from Ninakoru's Smart AI)
#define AUI_TACTICAL_TWEAKED_MOVE_TO_SAFETY_HIGH_DANGER_EMBARK
/// Sets a lower danger value for city tiles depending on the health of the city
#define AUI_TACTICAL_TWEAKED_MOVE_TO_SAFETY_LOW_DANGER_CITY
#ifdef AUI_TACTICAL_TWEAKED_ACCEPTABLE_DANGER
/// Considers units of the same type when executing move to safety (important if health is a factor in selecting units for this movetype)
#define AUI_TACTICAL_TWEAKED_MOVE_TO_SAFETY_CONSIDER_SAME_UNIT_TYPE
#endif // AUI_TACTICAL_TWEAKED_ACCEPTABLE_DANGER
/// Immediately skips some heavy computation if the function is not looking for ranged units and unit being checked is a ranged unit (originally from Ninakoru's Smart AI)
#define AUI_TACTICAL_FIND_UNITS_WITHIN_STRIKING_DISTANCE_NO_RANGED_SHORTCUT
/// When calculating the expected damage on a target from a melee unit, the AI will now use pTargetPlot and pDefender parameters when appropriate (instead of having them as NULL)
#define AUI_TACTICAL_FIX_FIND_UNITS_WITHIN_STRIKING_DISTANCE_MELEE_STRENGTH
/// When calculating the expected damage on a target from a melee unit, the AI will now use pDefender and pCity parameters when appropriate (instead of having them as NULL)
#define AUI_TACTICAL_FIX_FIND_UNITS_WITHIN_STRIKING_DISTANCE_RANGED_STRENGTH
/// When calculating the expected damage on a target from a melee unit, the AI will now use pTargetPlot and pDefender parameters when appropriate (instead of having them as NULL)
#define AUI_TACTICAL_FIX_FIND_PARATROOPER_WITHIN_STRIKING_DISTANCE_MELEE_STRENGTH
/// Adds possible air sweeps to the Find Units within Striking Distance function (originally from Ninakoru's Smart AI)
#define AUI_TACTICAL_FIND_UNITS_WITHIN_STRIKING_DISTANCE_AIR_SWEEPS
/// When computing expected damage, units that are currently not in range but could be if they moved get their strength value divided by this (originally from Ninakoru's Smart AI)
#define AUI_TACTICAL_COMPUTE_EXPECTED_DAMAGE_FARAWAY_DIVISOR (1.5)
/// When calculating the expected damage on a target from a melee unit, the AI will now use pFromPlot and pDefender parameters when appropriate (instead of having them as NULL)
#define AUI_TACTICAL_FIX_COMPUTE_EXPECTED_DAMAGE_MELEE
/// Checks all of the medium priority list for closer units than the high priority list item instead of just the first one in the medium priority list
#define AUI_TACTICAL_FIX_EXECUTE_MOVE_TO_TARGET_ALL_MEDIUM_PRIORITY_CHECKED
/// Adds a new function that will order a unit to pillage its tile when appropriate (eg. in enemy territory, don't need to attack or move)
#define AUI_TACTICAL_FREE_PILLAGE
/// UnitProcessed() function will now only execute if the unit has no moves remaining, allowing units with blitz to execute multiple moves each turn
#define AUI_TACTICAL_FIX_UNIT_PROCESSED_BLITZ
/// Fixes poor placement of ranged units with a range of 1 (eg. machine guns)
#define AUI_TACTICAL_FIX_CLOSE_ON_TARGET_MELEE_RANGE
/// When checking the embark safety of a plot, use the plot the unit will be moving to instead of the target plot
#define AUI_TACTICAL_FIX_MOVE_TO_USING_SAFE_EMBARK_CORRECT_PLOT
/// Pathfinder no longer called twice when MoveToUsingSafeEmbark() is called in a function that already generated a path to the target
#define AUI_TACTICAL_FIX_MOVE_TO_USING_SAFE_EMBARK_SINGLE_PATHFINDER_CALL
/// The AI can now move and shoot in the same turn with a unit executing a safe bombard
#define AUI_TACTICAL_FIX_SAFE_BOMBARDS_MOVE_AND_SHOOT
/// For the safe bombard tactical move, process the ranged unit before the blocking melee units (since the actual bombard is the priority)
#define AUI_TACTICAL_FIX_SAFE_BOMBARDS_MOVE_RANGED_FIRST
/// Only land plots will now be considered as valid operation targets for land, combat units in 
#define AUI_TACTICAL_FIX_FIND_CLOSEST_OPERATION_UNIT_NO_EMBARK
/// If no melee units are available to defend a barbarian camp, ranged units will now be selected to move to the camp
#define AUI_TACTICAL_FIX_CAMP_DEFENSE_RANGED_CAN_DEFEND
/// Fixes the fact that ranged units cannot be used for long distance (out of ranged attack distance) calculations for FindUnitsWithinStrikingDistance
#define AUI_TACTICAL_FIX_FIND_UNITS_WITHIN_STRIKING_DISTANCE_RANGED_LONG_DISTANCE
/// Barbarian moves that don't actually rely on attacking a target will now use FindClosestTarget() instead of FindUnitsWithinStrikingDistance()
#define AUI_TACTICAL_FIX_USE_FIND_CLOSEST_TARGET
/// Fixes FindClosestUnit when looking for ranged units to only process within range checks if the target is an enemy
#define AUI_TACTICAL_FIX_FIND_CLOSEST_TARGET_RANGED_MOVEMENT
/// Only calls UnitProcessed() if SaveMoves is turned off (since the function removes the unit from the "units to be processed" list, so saving moves would be irrelevant)
#define AUI_TACTICAL_FIX_EXECUTE_MOVE_TO_PLOT_UNIT_PROCESSED
/// When assigning units to plunder trade routes, the pathfinder is called to find the closest route
#define AUI_TACTICAL_PLOT_PLUNDER_MOVES_USES_PATHFINDER
#ifdef AUI_ASTAR_PARADROP
/// Paradropping is now inserted into relevant tactical moves and the AI will execute them when favorable
#define AUI_TACTICAL_PARATROOPERS_PARADROP
#endif // AUI_ASTAR_PARADROP
#ifdef AUI_TACTICAL_EXECUTE_SWAP_TO_PLOT
/// Uses the new ExecuteSwapToPlot() function when moving blocking units
#define AUI_TACTICAL_EXECUTE_MOVE_BLOCKING_UNIT_USES_SWAP
#endif // AUI_TACTICAL_EXECUTE_SWAP_TO_PLOT
/// If a blocking unit is moved, it will no longer automatically end the just moved unit's turn
#define AUI_TACTICAL_FIX_EXECUTE_MOVE_BLOCKING_UNIT_SAVE_MOVES
/// When moving a blocking unit, allow it to prioritize tiles from which it can still move and/or attack this turn
#define AUI_TACTICAL_EXECUTE_MOVE_BLOCKING_UNIT_ALLOW_ZERO_MOVE_PRIORITY
/// Checks to see if the unit being moved by MoveToEmptySpaceNearTarget() or MoveToEmptySpaceTwoFromTarget() isn't already within 1 or 2 tiles of the target respectively
#define AUI_TACTICAL_FIX_MOVE_TO_EMPTY_SPACE_FROM_TARGET_CHECK_UNIT_PLOT_FIRST
/// When processing civilians in an escorted move, have them move to safety if they aren't currently defended and are in danger
#define AUI_TACTICAL_PLOT_SINGLE_HEX_OPERATION_MOVES_CIVILIAN_TO_SAFETY
/// Removes the finishMoves() call that gets executed for every unit executing this function
#define AUI_TACTICAL_FIX_EXECUTE_FORMATION_MOVES_NO_FINISHMOVE_COMMAND
/// Removes the finishMoves() call after a unit has looted a barbarian camp
#define AUI_TACTICAL_FIX_EXECUTE_BARBARIAN_CAMP_MOVE_NO_FINISHMOVE_COMMAND
/// Removes the finishMoves() call after a unit has capture a civilian
#define AUI_TACTICAL_FIX_EXECUTE_CIVILIAN_CAPTURE_NO_FINISHMOVE_COMMAND
/// Removes the finishMoves() call after a unit has completed a [paradrop] pillage mission
#define AUI_TACTICAL_FIX_EXECUTE_PILLAGE_NO_FINISHMOVE_COMMAND
/// Removes the finishMoves() call after a unit has completed a plunder trade route mission
#define AUI_TACTICAL_FIX_EXECUTE_PLUNDER_TRADE_UNIT_NO_FINISHMOVE_COMMAND

// Tactical Analysis Map Stuff
/// Enables a minor adjustment for ranged units to account for possibly being able to move and shoot at a tile
#define AUI_TACTICAL_MAP_ANALYSIS_MARKING_ADJUST_RANGED
/// Checks for cities in addition to citadel improvements
#define AUI_TACTICAL_MAP_ANALYSIS_MARKING_INCLUDE_CITIES

// Trade Stuff
/// Adds a minimum danger amount for each plot, to discourage long routes
#define AUI_TRADE_SCORE_TRADE_ROUTE_BASE_DANGER (1)
/// When scoring trade routes, divides by the base-(value) log of the total danger instead of by the actual total danger
#define AUI_TRADE_SCORE_TRADE_ROUTE_DIVIDE_BY_LOG_TOTAL_DANGER (2.0)
/// If the international trade route would be with a major, taper the (minus) for gold and tech delta with diplomatic relations
#define AUI_TRADE_SCORE_INTERNATIONAL_TAPER_DELTA_WITH_FRIENDLY_AND_INCOME
/// If the international trade route would be to a minor, the gold and tech received by the minor do not count
#define AUI_TRADE_SCORE_INTERNATIONAL_MAX_DELTA_WITH_MINORS
/// Score for a trade route from beakers is now relative to how much beakers we get from other sources
#define AUI_TRADE_SCORE_INTERNATIONAL_RELATIVE_TECH_SCORING
/// Score for a trade route from religious pressure is now relative to how much pressure there already is at the city
#define AUI_TRADE_SCORE_INTERNATIONAL_RELATIVE_RELIGION_SCORING
/// Individual deltas are now weighed by the player's flavor
#define AUI_TRADE_SCORE_INTERNATIONAL_FLAVORED_DELTAS
/// Instead of simply doubling score if we want the tourism boost, the multiplier is based on our grand strategy
#define AUI_TRADE_SCORE_INTERNATIONAL_TOURISM_SCORE_USES_GRAND_STRATEGY
/// Actually calculate the proper, international-equivalent value of a food trade route
#define AUI_TRADE_SCORE_FOOD_VALUE
/// Actually calculate the proper, international-equivalent value of a production trade route
#define AUI_TRADE_SCORE_PRODUCTION_VALUE
/// When prioritizing trade routes, the actual trade value of all three possible route types will be considered instead of prioritizing food > production > international
#define AUI_TRADE_UNBIASED_PRIORITIZE

// Unit Stuff
/// Adds a function to return a unit's movement range if it can attack after a move + the unit's range (originally from Ninakoru's Smart AI)
#define AUI_UNIT_RANGE_PLUS_MOVE
/// Fixes the check for whether ranged damage would be more than heal rate to use >= instead of >, adds a flat value to total damage at start (both make up for randomness), and treats cities as an expected damage source instead of a flat "yes"
#define AUI_UNIT_FIX_UNDER_ENEMY_RANGED_ATTACK_HEALRATE (1)
/// Adds a function to return whether a unit can range strike at a target from a plot (originally from Ninakoru's Smart AI)
#define AUI_UNIT_CAN_EVER_RANGE_STRIKE_AT_FROM_PLOT
#ifdef AUI_UNIT_CAN_EVER_RANGE_STRIKE_AT_FROM_PLOT
/// Overloads the vanilla canEverRangeStrikeAt() function to call the new canEverRangeStrikeAtFromPlot() function if it's enabled (originally from Ninakoru's Smart AI)
#define AUI_UNIT_CAN_EVER_RANGE_STRIKE_AT_OVERLOAD
#ifdef AUI_UNIT_CAN_EVER_RANGE_STRIKE_AT_OVERLOAD
#ifdef AUI_UNIT_RANGE_PLUS_MOVE
/// Adds two new functions that return whether a unit can move and ranged strike at a plot (originally from Ninakoru's Smart AI)
#define AUI_UNIT_CAN_MOVE_AND_RANGED_STRIKE
#endif // AUI_UNIT_RANGE_PLUS_MOVE
#endif // AUI_UNIT_CAN_EVER_RANGE_STRIKE_AT_OVERLOAD
#endif // AUI_UNIT_CAN_EVER_RANGE_STRIKE_AT_FROM_PLOT

// Promition Stuff (within CvUnit.cpp)
/// Use double instead of int for most variables (to retain information during division) inside AI_promotionValue()
#define AUI_UNIT_PROMOTION_USE_DOUBLES
/// Instaheal promotion will only be taken if the unit is under threat in addition to being below 50% HP
#define AUI_UNIT_PROMOTION_FIX_INSTAHEAL_ONLY_UNDER_THREAT
/// Ranged units now value this promotion higher depending on ranged flavor, while Counter and Defense (melee) units no longer value this promotion higher than usual
#define AUI_UNIT_PROMOTION_TWEAKED_CITY_DEFENSE
/// Specialist units get higher flavor influence for heal rate adjusting promotions in addition to their flat boost
#define AUI_UNIT_PROMOTION_TWEAKED_HEALRATE (3)
/// Cavalry units get higher flavor influence for amphibious promotions in addition to their flat boost
#define AUI_UNIT_PROMOTION_TWEAKED_AMPHIBIOUS (3)
/// Specialist units get higher flavor influence for move rate adjusting promotions in addition to their flat boost
#define AUI_UNIT_PROMOTION_TWEAKED_MOVECHANGE (3)
/// More flavors considered for March promotion (always heal) and larger flavor influence for specialists
#define AUI_UNIT_PROMOTION_TWEAKED_MARCH (3)
/// More flavors considered for Blitz promotion (multiple attacks per turn) and larger flavor influence for specialists
#define AUI_UNIT_PROMOTION_TWEAKED_BLITZ (3)
/// Specialist units get higher flavor influence for promotions that let them move after attacking in addition to their flat boost
#define AUI_UNIT_PROMOTION_TWEAKED_MOVE_AFTER_ATTACK (3)
/// Fixes promotions that give a boost to fighting on a terrain type to not apply their value multiplication to other effects stacked with the promotion
#define AUI_UNIT_PROMOTION_FIX_TERRAIN_BOOST
/// Uses a different randomization algorithm and also uses the binomial RNG instead of the standard one if it's enabled
#define AUI_UNIT_PROMOTION_TWEAKED_RANDOM (17)

// Voting/League Stuff
/// VITAL FOR MOST FUNCTIONS! Use double instead of int for certain variables (to retain information during division)
#define AUI_VOTING_USE_DOUBLES
/// Uses slightly modified algorithm for determining Diplomat Usefulness levels
#define AUI_VOTING_TWEAKED_DIPLOMAT_USEFULNESS
/// Uses a different algorithm for scoring voting on international projects
#define AUI_VOTING_TWEAKED_INTERNATIONAL_PROJECTS
/// Uses a different algorithm for scoring voting on embargoing city states
#define AUI_VOTING_TWEAKED_EMBARGO_MINOR_CIVS
/// Uses a different algorithm for scoring voting on banning a luxury
#define AUI_VOTING_TWEAKED_BAN_LUXURY
/// Uses a different algorithm for scoring voting on standing army tax
#define AUI_VOTING_TWEAKED_STANDING_ARMY
/// Uses a different algorithm for scoring voting on scholars in residence
#define AUI_VOTING_TWEAKED_SCHOLARS_IN_RESIDENCE
/// Uses a different algorithm for scoring voting on world ideology
#define AUI_VOTING_TWEAKED_WORLD_IDEOLOGY
/// Uses a different algorithm for scoring voting on world religion
#define AUI_VOTING_TWEAKED_WORLD_RELIGION
/// Uses a different algorithm and unifies the code for scoring voting on arts funding and sciences funding
#define AUI_VOTING_TWEAKED_ARTS_SCIENCES_FUNDING

// GlobalDefines (GD) wrappers
// INT
#define GD_INT_DECL(name)       int m_i##name
#define GD_INT_DEF(name)        inline int get##name() { return m_i##name; }
#define GD_INT_INIT(name, def)  m_i##name(def)
#define GD_INT_CACHE(name)      m_i##name = getDefineINT(#name)
#define GD_INT_GET(name)        GC.get##name()
// FLOAT
#define GD_FLOAT_DECL(name)       float m_f##name
#define GD_FLOAT_DEF(name)        inline float get##name() { return m_f##name; }
#define GD_FLOAT_INIT(name, def)  m_f##name(def)
#define GD_FLOAT_CACHE(name)      m_f##name = getDefineFLOAT(#name)
#define GD_FLOAT_GET(name)        GC.get##name()
// DOUBLE (high precision, but much slower than float)
#ifdef AUI_CACHE_DOUBLE
#define GD_DOUBLE_DECL(name)       double m_d##name
#define GD_DOUBLE_DEF(name)        inline double get##name() { return m_d##name; }
#define GD_DOUBLE_INIT(name, def)  m_d##name(def)
#define GD_DOUBLE_CACHE(name)      m_d##name = getDefineDOUBLE(#name)
#define GD_DOUBLE_GET(name)        GC.get##name()
#endif

#endif