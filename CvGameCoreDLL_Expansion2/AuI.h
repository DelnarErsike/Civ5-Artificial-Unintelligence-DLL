// AuI.h
#pragma once

#ifndef AUI_MODS_H
#define AUI_MODS_H

// New mathematical constants
#define M_E			2.71828182845904523536
#define fM_E		2.718281828f		//!< e (float)
#define M_SQRT2		1.41421356237309504880
#define fM_SQRT2	1.414213562f		//!< sqrt(2) (float)
#define M_SQRT3		1.73205080756887729353
#define fM_SQRT3	1.732050808f		//!< sqrt(3) (float)
#define M_LN2		0.693147180559945309417
#define fM_LN2		0.6931471806f		//!< ln(2) (float)
#define M_GLDNRT	1.61803398874989484820
#define fM_GLDNRT	1.618033989f		//!< (1 + sqrt(5))/2 (float), aka The Golden Ratio

// Non-AI stuff (still used by AI routines, but could be used elsewhere as well)
/// AUI's new GUID
#define AUI_GUID
/// Civilizations that are marked as coastal get the same coastal bias as maritime city-states
#define AUI_STARTPOSITIONER_COASTAL_CIV_WATER_BIAS
/// When calculating the founding value of a tile, tailor the SiteEvaluation function to the current player instead of the first one
#define AUI_STARTPOSITIONER_FLAVORED_STARTS
/// Enables performance logging based on ini settings like normal instead of hard-disabling
#define AUI_PERF_LOGGING_ENABLED
/// Fast comparison functions (to be used for built-in types like int, float, double, etc.)
#define AUI_FAST_COMP
/// Increases stopwatch (performance counter) precision by using long double types instead of double
#define AUI_STOPWATCH_LONG_DOUBLE_PRECISION
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
/// Implements a new function that returns the number of attacks a unit can make
#define AUI_UNIT_EXTRA_ATTACKS_GETTER
/// New parameter in healRate() that assumes the heal rate from units is a certain number (used to weigh healing great generals/civilians)
#define AUI_UNIT_HEALRATE_ASSUME_EXTRA_HEALRATE_FROM_UNIT
/// Removes the cap of 8 range for unit sight; this was only needed because the for() loops weren't set up properly, resulting in too many unused cycles
#define AUI_PLOT_SEE_FROM_SIGHT_NO_MAXIMUM_SIGHT_RANGE
/// When choosing the top n choices from a weighted vector, choices with weight equal to the last choice are also included
#define AUI_WEIGHTED_VECTOR_FIX_TOP_CHOICES_TIE
/// Adds a new function to CvPlot that calculates the strategic value of a plot based on river crossing count, whether it's hills, and chokepoint factor
#define AUI_PLOT_CALCULATE_STRATEGIC_VALUE
/// Adds a new function to CvPlot to count how many times the given plot is in a list
#define AUI_PLOT_COUNT_OCCURANCES_IN_LIST
/// Tweaks to make performance logs a bit more consistent and easier to read
#define AUI_PERF_LOGGING_FORMATTING_TWEAKS
/// Performance optimizations related to bit twiddling (http://www.graphics.stanford.edu/~seander/bithacks.html) 
#define AUI_GAME_CORE_UTILS_OPTIMIZATIONS
/// Optimizes loops that iterate over relative coordinates to hexspace
#define AUI_HEXSPACE_DX_LOOPS
/// New inline function that sets the plot distance if it passes the range check
#define AUI_PLOT_XY_WITH_RANGE_CHECK_REFERENCE_DISTANCE
/// Optimizations and fixes to reduce distance check overhead
#define AUI_FIX_HEX_DISTANCE_INSTEAD_OF_PLOT_DISTANCE
/// Implements the missing erase(iterator) function for FFastVector
#define AUI_FIX_FFASTVECTOR_ERASE

#ifdef AUI_FAST_COMP
// Avoids Visual Studio's compiler from generating inefficient code
// FastMax() and FastMin() taken from https://randomascii.wordpress.com/2013/11/24/stdmin-causing-three-times-slowdown-on-vc/
template<class T> inline T FastMax(const T& _Left, const T& _Right) { return (_DEBUG_LT(_Left, _Right) ? _Right : _Left); }
template<class T> inline T FastMin(const T& _Left, const T& _Right) { return (_DEBUG_LT(_Right, _Left) ? _Right : _Left); }
#define FASTMAX(a, b) FastMax(a, b)
#define FASTMIN(a, b) FastMin(a, b)
#endif // AUI_FAST_COMP

// A* Pathfinding Stuff
/// Adds a new function that is a middle-of-the-road fix for allowing the functions to take account of roads and railroads without calling pathfinder too often
#define AUI_ASTAR_TWEAKED_OPTIMIZED_BUT_CAN_STILL_USE_ROADS
/// A* functions now identify paradropping as a valid move
#define AUI_ASTAR_PARADROP
/// The danger of a tile will only be considered when checking path nodes, not when checking the destination (stops units from freezing in panic)
#define AUI_ASTAR_FIX_CONSIDER_DANGER_ONLY_PATH
/// The path's destination's danger value will be considered instead of the original plot's danger value, otherwise we're just immobilizing AI units (oddly enough, the Civ4 algorithm is fine, only the Civ5 ones needed to be fixed)
#define AUI_ASTAR_FIX_CONSIDER_DANGER_USES_TO_PLOT_NOT_FROM_PLOT
#ifdef AUI_ASTAR_FIX_CONSIDER_DANGER_USES_TO_PLOT_NOT_FROM_PLOT
/// If the pathfinder does not ignore danger, the plot we're moving from must pass the danger check before we consider the destination plot's danger 
#define AUI_ASTAR_FIX_CONSIDER_DANGER_ONLY_POSITIVE_DANGER_DELTA
#endif
/// If the pathfinder does not ignore danger, use the unit's combat strength times this value as the danger limit instead of 0 (important for combat units)
#define AUI_ASTAR_FIX_CONSIDER_DANGER_USES_COMBAT_STRENGTH (6)
/// AI-controlled units no longer ignore all paths with peaks; since the peak plots are check anyway for whether or not a unit can enter them, this check is pointless 
#define AUI_ASTAR_FIX_PATH_VALID_PATH_PEAKS_FOR_NONHUMAN

// AI Operations Stuff
/// If a settler tries and fails the no escort check, keep rerolling each turn
#define AUI_OPERATION_FOUND_CITY_SETTLER_REROLLS
/// Tweaks the boldness check for whether a settler should proceed without escort
#define AUI_OPERATION_FOUND_CITY_TWEAKED_NO_ESCORT_BOLDNESS (8)
/// Adds a random value to the boldness check so it doesn't always succeed or fail
#define AUI_OPERATION_FOUND_CITY_TWEAKED_NO_ESCORT_RANDOM_VALUE (7)
#ifdef AUI_BINOM_RNG
/// If it's available, opts for the binomial RNG for the boldness check's random factor instead of the flat RNG
#define AUI_OPERATION_FOUND_CITY_TWEAKED_NO_ESCORT_RANDOM_BINOMIAL
#endif
/// FindBestFitReserveUnit() no longer ignores units that can paradrop
#define AUI_OPERATION_FIND_BEST_FIT_RESERVE_CONSIDER_PARATROOPERS
/// The filter to filter out scouting units only applies to units whose default AI is scouting
#define AUI_OPERATION_FIX_FIND_BEST_FIT_RESERVE_CONSIDER_SCOUTING_NONSCOUTS
/// Operations will now recruit units who are in armies that are waiting for reinforcements
#define AUI_OPERATION_FIND_BEST_FIT_RESERVE_CONSIDER_UNITS_IN_WAITING_ARMIES
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
/// If a civilian retargets and an escort cannot get to the new target (ignoring units), then the operation is aborted
#define AUI_OPERATION_FIX_RETARGET_CIVILIAN_ABORT_IF_UNREACHABLE_ESCORT

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
/// Combat workers will increase the maximum allowed plot danger value to their current strength times this value
#define AUI_WORKER_SHOULD_BUILDER_CONSIDER_PLOT_MAXIMUM_DANGER_BASED_ON_UNIT_STRENGTH (6)
/// FindTurnsAway() no longer returns raw distance, parameter dictates whether we're reusing paths and ignoring units (fast but rough) or not (slow but accurate)
#define AUI_WORKER_FIND_TURNS_AWAY_USES_PATHFINDER (true)
#ifdef AUI_PLOT_CALCULATE_STRATEGIC_VALUE
/// AddImprovingPlotsDirective() now processes improvement defense rate
#define AUI_WORKER_ADD_IMPROVING_PLOTS_DIRECTIVE_DEFENSIVES
#endif
/// Shifts the check for whether there already is someone building something on the plot to the necessary AddDirectives() functions (so collaborative building is possible)
#define AUI_WORKER_FIX_SHOULD_BUILDER_CONSIDER_PLOT_EXISTING_BUILD_MISSIONS_SHIFT
/// New function that is called to construct non-road improvements in a minor's territory (eg. for Portugal)
#define AUI_WORKER_ADD_IMPROVING_MINOR_PLOTS_DIRECTIVES
/// Multiplies the weight of unowned luxury resources for plot directives depending on the empire's happiness (value is the multiplier at 0 happiness)
#define AUI_WORKER_GET_RESOURCE_WEIGHT_INCREASE_UNOWNED_LUXURY_WEIGHT (2.0)
/// Consider extra sources of happiness once a resource is obtained (eg. extra happiness from luxury resources via policy, extra happiness from resource variety)
#define AUI_WORKER_GET_RESOURCE_WEIGHT_CONSIDER_EXTRAS_FOR_HAPPINESS_FROM_RESOURCE

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
/// Replaces the rudimentary specialist-plot check with a plot vs. default citizen value check
#define AUI_CITIZENS_IS_PLOT_BETTER_THAN_DEFAULT_SPECIALIST
/// If the empire is unhappy, cities with full or partial food focus get their food focus removed
#define AUI_CITIZENS_DO_TURN_NO_FOOD_FOCUS_IF_UNHAPPY

// City Strategy Stuff
/// Scales the GetLastTurnWorkerDisbanded() computation to game speed
#define AUI_CITYSTRATEGY_FIX_TILE_IMPROVERS_LAST_DISBAND_WORKER_TURN_SCALE
/// Disables the minimum population requirement for the "first faith building" strategy
//#define AUI_CITYSTRATEGY_FIX_FIRST_FAITH_BUILDING_NO_MINIMUM_POP
/// The sanity check to ensure water units are not built on small inland seas now requires at least one other civ (minors too!) to have a city coastal to that area
#define AUI_CITYSTRATEGY_FIX_CHOOSE_PRODUCTION_ACCURATE_SEA_SANITY_CHECK
/// Instead of ignoring all military training buildings (eg. stables, kreposts, etc.), puppets will instead nullify the Military Training and Naval flavors
#define AUI_CITYSTRATEGY_FIX_CHOOSE_PRODUCTION_PUPPETS_NULLIFY_BARRACKS
/// Priorities for sneak attack military units are no longer artificially inflated at the highest difficulty levels
#define AUI_CITYSTRATEGY_CHOOSE_PRODUCTION_NO_HIGH_DIFFICULTY_SKEW
/// If the number of buildables is greater than the number of possible choices, normalize all weights in the list (ie. subtract the lowest score from all scores)
//#define AUI_CITYSTRATEGY_CHOOSE_PRODUCTION_NORMALIZE_LIST
/// If the player has yet to unlock an ideology, multiply the base weight of buildings that can unlock ideologies by this value
#define AUI_CITYSTRATEGY_EMPHASIZE_FACTORIES_IF_NO_IDEOLOGY (8)

// Culture Classes Stuff
/// AI only wants propaganda diplomats with players of different ideologies (since that's the only time they get the tourism bonus)
#define AUI_PLAYERCULTURE_FIX_WANTS_DIPLOMAT_DOING_PROPAGANDA_ONLY_NON_SAME_IDEOLOGY
/// No longer returns false if any one player's influence level is Unknown (it will now simply skip that player instead)
#define AUI_PLAYERCULTURE_WANTS_DIPLOMAT_DOING_PROPAGANDA_NO_EARLY_TERMINATION
/// Influence turns are used instead of influence levels (ie. the player will propaganda the two AI's who will take the longest to get to influential)
#define AUI_PLAYERCULTURE_WANTS_DIPLOMAT_DOING_PROPAGANDA_INFLUENCE_TURNS_USED
/// The AI will only want propaganda spies if its tourism is greater than a certain amount (will actually use Science Boost amount if it's active)
#define AUI_PLAYERCULTURE_GET_MAX_PROPAGANDA_DIPLOMATS_WANTED_FILTER_TOURISM (8)

// Danger Plots Stuff
/// Better danger calculation for ranged units (originally from Ninakoru's Smart AI, but heavily modified since)
#define AUI_DANGER_PLOTS_TWEAKED_RANGED
/// The ignore visibility switch also works on the plot visibility check
#define AUI_DANGER_PLOTS_FIX_SHOULD_IGNORE_UNIT_IGNORE_VISIBILITY_PLOT
/// Majors will always "see" barbarians in tiles that have been revealed when plotting danger values (kind of a cheat, but it's a knowledge cheat, so it's OK-ish)
#define AUI_DANGER_PLOTS_SHOULD_IGNORE_UNIT_MAJORS_SEE_BARBARIANS_IN_FOG
/// Minors will always "see" units of major civs in tiles (value) away from their city (since minors don't scout) when plotting danger values (stops excessive worker stealing)
#define AUI_DANGER_PLOTS_SHOULD_IGNORE_UNIT_MINORS_SEE_MAJORS (5)
/// Minors will ignore all units of players who are not at war with them
#define AUI_DANGER_PLOTS_FIX_IS_DANGER_BY_RELATIONSHIP_ZERO_MINORS_IGNORE_ALL_NONWARRED
/// When adding danger from a source, the attack strength of the source onto the plot is added instead of the base attack strength
#define AUI_DANGER_PLOTS_ADD_DANGER_CONSIDER_TERRAIN_STRENGTH_MODIFICATION
/// Counts air unit strength into danger (commented out for now)
//#define AUI_DANGER_PLOTS_COUNT_AIR_UNITS

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
#endif

// EconomicAI Stuff
/// VITAL FOR MOST FUNCTIONS! Use double instead of int for certain variables (to retain information during division)
#define AUI_ECONOMIC_USE_DOUBLES
#ifdef AUI_CITY_FIX_CREATE_UNIT_EXPLORE_ASSIGNMENT_TO_ECONOMIC
/// Assigning non-scout units to become explorers is now done in EconomicAI instead of at City
#define AUI_ECONOMIC_FIX_DO_RECON_STATE_EXPLORE_ASSIGNMENT_FROM_CITY
#endif
/// Resets the UnitLoop iterator before looping through units (would otherwise cause problems when both land and sea exploration was in the "enough" state)
#define AUI_ECONOMIC_FIX_DO_RECON_STATE_ITERATOR
/// The "skip first" code to remove scouting non-scouts only works if we don't actually have any dedicated scout units
#define AUI_ECONOMIC_FIX_DO_RECON_STATE_SKIP_FIRST_ONLY_IF_NO_EXPLORERS
/// The code to remove scouting non-scouts is more generalized (ie. doesn't depend on UNITAI_ATTACK)
#define AUI_ECONOMIC_FIX_DO_RECON_STATE_MALLEABLE_REMOVE_NONSCOUTS
/// When counting the number of tiles with adjacent fog, only count tiles that are on the same landmass as one of our cities (if land) or are accessible by one of our cities (if water)
#define AUI_ECONOMIC_FIX_DO_RECON_STATE_ONLY_STARTING_LANDMASS_FOG_TILES_COUNT
/// Adds a setter for m_iLastTurnWorkerDisbanded (so if a worker is disbanded in another class, they can reference this)
#define AUI_ECONOMIC_SETTER_LAST_TURN_WORKER_DISBANDED
/// Checks that would force the function to return false happen earlier in the function
#define AUI_ECONOMIC_EARLY_EXPANSION_TWEAKED_EARLIER_CHECKS
/// Early Expansion strategy is always active if the AI is the only player on a continent (well, until the good settling plots run out)
#define AUI_ECONOMIC_EARLY_EXPANSION_ALWAYS_ACTIVE_IF_ALONE
/// Multiplies the desired city count by the ratio of the natural logs of expansion and growth flavors, with difficulty acting as an extra multiplier to expansion flavor
#define AUI_ECONOMIC_EARLY_EXPANSION_TWEAKED_FLAVOR_APPLICATION
/// Scales the desired amount of cities by the amount of major and minor players on the map
#define AUI_ECONOMIC_EARLY_EXPANSION_SCALE_BY_PLAYER_COUNT
/// Instead of checking to see how many tiles are owned in the starting area, the AI will instead check whether it has any possible settle plots within (value) tiles of a city
#define AUI_ECONOMIC_EARLY_EXPANSION_REPLACE_OWNED_TILE_CHECKS_WITH_DISTANCE_CHECK (7)
/// Removes the check for a cultural grand strategy that's a holdover from pre-BNW when cultural victories were won through policies, not tourism
#define AUI_ECONOMIC_FIX_EXPAND_LIKE_CRAZY_REMOVE_HOLDOVER_CULTURE_CHECK
#ifdef AUI_CITY_FIX_BUILDING_PURCHASES_WITH_GOLD
/// Reenables the DoHurry() function, but using a reworked method instead of the original (originally from Ninakoru's Smart AI, but heavily modified since)
#define AUI_ECONOMIC_FIX_DO_HURRY_REENABLED_AND_REWORKED
#endif
/// Nearby cities that are damaged can also be targetted
#define AUI_ECONOMIC_FIX_GET_BEST_GREAT_WORK_CITY_NO_DAMAGE_FILTER

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
#endif

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
#ifdef AUI_GS_PRIORITY_RATIO
/// GS influence over flavor is multiplied by the difference between the GS's priority ratio and the lowest GS priority ratio if the GS is not the primary GS
#define AUI_GS_GET_PERSONALITY_AND_GRAND_STRATEGY_USE_COMPARE_TO_LOWEST_RATIO
#endif
/// Multiplies the science flavor of buildings, wonders, and techs depending on how well the tech requirements of significant GS's is met (eg. have Archaeology, have Internet, etc.)
#define AUI_GS_SCIENCE_FLAVOR_BOOST (6)
/// Replaces the logging function with one that allows for easy creation of graphs within Excel
#define AUI_GS_BETTER_LOGGING

// Homeland AI Stuff
#ifdef AUI_PLOT_COUNT_OCCURANCES_IN_LIST
/// Adds a new function that lets aircraft go on intercept missions (originally from Ninakoru's Smart AI)
#define AUI_HOMELAND_AIRCRAFT_INTERCEPTIONS
#endif
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
#endif
/// Flavors that weren't previously fetched but were still (attempted to be) used in processing later are now fetched
#define AUI_HOMELAND_FIX_ESTABLISH_HOMELAND_PRIORITIES_MISSING_FLAVORS
#ifdef AUI_UNIT_DO_AITYPE_FLIP
/// Allows units that can act as Sea Workers to flip to or from the UnitAIType using the MilitaryAI helper function
#define AUI_HOMELAND_PLOT_SEA_WORKER_MOVES_EMPLOYS_AITYPE_FLIP
/// Attempts to flip a scout's UnitAIType if it has no target
#define AUI_HOMELAND_EXECUTE_EXPLORER_MOVES_FLIP_AITYPE_ON_NOTARGET
#endif
/// Allows units that can act as Workers to flip to or from the UnitAIType using the MilitaryAI helper function
#define AUI_HOMELAND_PLOT_WORKER_MOVES_EMPLOYS_AITYPE_FLIP
/// Disbanding explorers now uses the scrap() function instead of the kill() function
#define AUI_HOMELAND_FIX_EXECUTE_EXPLORER_MOVES_DISBAND
#ifdef AUI_ECONOMIC_SETTER_LAST_TURN_WORKER_DISBANDED
/// If a worker is idle and we have extra workers, disband him instead of sending him to the safest plot
#define AUI_HOMELAND_PLOT_WORKER_MOVES_DISBAND_EXTRA_IDLE_WORKERS
#endif
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
/// Disbands work boats that cannot target anything, even if units are ignored
#define AUI_HOMELAND_PLOT_WORKER_SEA_MOVES_DISBAND_WORK_BOATS_WITHOUT_TARGET
/// If the AI wants to use a unit for a Great Work, check if the unit can create one right there and then (performance improvement)
#define AUI_HOMELAND_EXECUTE_GP_MOVE_INSTANT_GREAT_WORK_CHECK

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
#endif
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
/// Pathfinder used instead of raw distance, parameter dictates whether we're reusing paths (fast but rough) or not (slow but accurate)
#define AUI_PLAYER_GET_BEST_SETTLE_PLOT_USE_PATHFINDER_FOR_EVALDISTANCE (true)
/// If a tile is on the same continent as the player's capital and is closer to an enemy major capital than any allied cities, disregard the tile
#define AUI_PLAYER_GET_BEST_SETTLE_PLOT_CONSIDER_ENEMY_CAPITALS
/// Switches the function over to using the XML-loaded fertility value
#define AUI_PLAYER_GET_BEST_SETTLE_PLOT_USE_MINIMUM_FERTILITY

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
#ifdef AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_FUTURE_OWNER_IF_UNOWNED
/// If we're looking at an unowned plot, assume we're going to build a civ-specific improvement on it
#define AUI_PLOT_CALCULATE_NATURE_YIELD_USE_POTENTIAL_CIV_UNIQUE_IMPROVEMENT
#endif
/// If a plot is not visible to the attacking player and the unit being considered is not an air unit, disregard the unit
#define AUI_PLOT_FIX_GET_BEST_DEFENDER_CHECK_PLOT_VISIBILITY

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
/// When playing a One City Challenge game, expansion flavor for policies is nullified
#define AUI_POLICY_NULLIFY_EXPANSION_WEIGHT_FOR_OCC
/// Replaces the divider for opening new branches with a "determination" divider based on the geometric mean of grand strategy ratios (old one was a holdover from pre-BNW)
#define AUI_POLICY_CHOOSE_NEXT_POLICY_TWEAKED_OPEN_NEW_BRANCH
/// Slight tweak to logic of ClearPrefs part to make the function's mathematics more accurate
#define AUI_POLICY_DO_CHOOSE_IDEOLOGY_TWEAKED_CLEAR_PREFS
#ifdef AUI_BINOM_RNG
/// The binomial RNG is used for the extra random score given to all ideologies; value is the maximum amount an ideology can receive from the random portion
#define AUI_POLICY_DO_CHOOSE_IDEOLOGY_USES_BINOM_RNG (10)
#endif
/// Implements a tiebreaker that adds random values to all ideologies until there is a clear winner
#define AUI_POLICY_DO_CHOOSE_IDEOLOGY_TIEBREAKER
/// Slight tweak to logic of ClearPrefs part to make the function's mathematics more accurate; ClearPrefs also gets overwritten if empire is very unhappy
#define AUI_POLICY_DO_CONSIDER_IDEOLOGY_SWITCH_TWEAKED_CLEAR_PREFS
/// Gets all possible happiness sources the branch can give, not just building-based ones (eg. specialists, trade routes, luxuries, etc.)
#define AUI_POLICY_GET_BRANCH_BUILDING_HAPPINESS_GET_ALL_HAPPINESS_SOURCES

// Religion/Belief Stuff
/// VITAL FOR MOST FUNCTIONS! Use double instead of int for certain variables (to retain information during division)
#define AUI_RELIGION_USE_DOUBLES
/// Various bits of code that could cause null pointers have been fixed
#define AUI_RELIGION_FIX_POSSIBLE_NULL_POINTER
/// The function has been reformatted to look smaller
#define AUI_RELIGION_REFORMAT_HAS_CREATED_RELIGION
/// Checks all of player's cities for whether or not a city is within conversion target range, not just the player's capital
#define AUI_RELIGION_CONVERSION_TARGET_NOT_JUST_CAPITAL
/// If the AI's religion now unlocks multiple faith buildings, AI can now purchase all of them
#define AUI_RELIGION_FIX_MULTIPLE_FAITH_BUILDINGS
/// Actually checks the cost of purchasing great people at a city instead of just returning the oldest city founded (deactivated while this is equal across all cities)
// #define AUI_RELIGION_FIX_BEST_CITY_HELPER_GREAT_PERSON
/// Raises individual belief components (plots as a whole, each city, player twice) to the power of this number
#define AUI_RELIGION_SCORE_BELIEF_RAISE_COMPONENT_SCORES_TO_POWER (1.25)
/// Increases plot search distance, but scales the belief score of a plot based on distance to the closest friendly city
#define AUI_RELIGION_SCORE_BELIEF_SCALE_PLOTS_WITH_DISTANCE (4)
/// Tweaks the amount a pantheon belief's score is divided (to compensate for higher scoring of certain plots)
#define AUI_RELIGION_SCORE_BELIEF_TWEAK_PANTHEON_DIVIDER (6.0)
/// Weighs different yield types differently depending on flavor and citizen value
#define AUI_RELIGION_SCORE_BELIEF_AT_PLOT_FLAVOR_YIELDS
/// When adding the terrain yield change of a belief, only do so if the current feature on the plot is additive (so eg. Dance with the Aurora won't be overvalued)
#define AUI_RELIGION_SCORE_BELIEF_AT_PLOT_SCORE_TERRAIN_CONSIDER_FEATURE
/// Reduces the score given to a tile from a feature if it has a resource on it, as chance are the resource will require the feature to be removed
#define AUI_RELIGION_SCORE_BELIEF_AT_PLOT_REDUCE_FEATURE_SCORE_IF_WILL_BE_CHOPPED
/// When scoring beliefs, the AI will only consider resources that it can see
#define AUI_RELIGION_SCORE_BELIEF_AT_PLOT_NO_RESOURCE_OMNISCIENCE
/// When scoring beliefs, increased yields from improvements no longer get double their normal rating
#define AUI_RELIGION_SCORE_BELIEF_AT_PLOT_NO_DOUBLE_IMPROVEMENT_YIELD_VALUE
/// Weighs different yield types differently depending on flavor and citizen value
#define AUI_RELIGION_SCORE_BELIEF_AT_CITY_FLAVOR_YIELDS
/// Happiness need and multiplier have been tweaked to use more flavors
#define AUI_RELIGION_SCORE_BELIEF_AT_CITY_TWEAKED_HAPPINESS
/// Considers grand strategies when scoring things like beliefs that only function when at peace
#define AUI_RELIGION_SCORE_BELIEF_AT_CITY_CONSIDER_GRAND_STRATEGY
/// Tweaks the flavor values and inputs used by various belief effects
#define AUI_RELIGION_SCORE_BELIEF_AT_CITY_TWEAKED_FLAVORS
/// River happiness score will only be applied if the city being scored is actually on a river
#define AUI_RELIGION_FIX_SCORE_BELIEF_AT_CITY_RIVER_HAPPINESS
/// Instead of multiplying a score if the city is at or above the minimum population required, score is divided if the city is below the minimum population required
#define AUI_RELIGION_SCORE_BELIEF_AT_CITY_REVERSED_MINIMUM_POPULATION_MODIFIER
/// Removes the extra score given by large cities to beliefs that give yield if any specialist is present
#define AUI_RELIGION_SCORE_BELIEF_AT_CITY_DISABLE_ANY_SPECIALIST_YIELD_BIAS
/// If a building for which yield improvement is being calculated is a wonder of any kind, divide the yield by the city count (so there's effective only one instance being scored in the civ)
#define AUI_RELIGION_SCORE_BELIEF_AT_CITY_YIELDS_FROM_WONDERS_COUNT_ONCE
/// When a belief has an obsoleting era, uses the player's current era as the benchmark instead of the game's average era
#define AUI_RELIGION_FIX_SCORE_BELIEF_AT_CITY_USE_PLAYER_ERA
/// Tweaks the flavor values and inputs used by various belief effects
#define AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_TWEAKED_FLAVORS
/// Considers grand strategies when scoring things like beliefs that only function when at peace
#define AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_CONSIDER_GRAND_STRATEGY
/// More flavors contribute to a player's happiness need "factor" for this function
#define AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_BETTER_HAPPINESS_FLAVOR
/// Scales Enhancer belief scores with how far the game has progressed, not how many religions have already been enhanced
#define AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_BETTER_ENHANCER_SCALING
/// The score for a belief that generates tourism from faith buildings is multiplied by the number of eligible buildings
#define AUI_RELIGION_SCORE_BELIEF_FOR_PLAYER_TOURISM_FROM_BUILDINGS_COUNTS_BUILDINGS
/// When scoring a belief that unlocks faith purchases of units, disregard eras that have already passed
#define AUI_RELIGION_FIX_SCORE_BELIEF_FOR_PLAYER_UNLOCKS_UNITS_DISREGARD_OLD_ERAS
/// Missionaries will no longer target hostile cities
#define AUI_RELIGION_FIX_SCORE_CITY_FOR_MISSIONARY_NO_WAR_TARGETTING
/// Divides a city's targetting score for missionaries by this value if passive pressure is enough to eventually convert the city
#define AUI_RELIGION_SCORE_CITY_FOR_MISSIONARY_DIVIDER_IF_PASSIVE_PRESSURE_ENOUGH (10)
/// Subtracts twice the distance to target squared (instead of subtracting just the distance)
#define AUI_RELIGION_SCORE_CITY_FOR_MISSIONARY_SUBTRACT_DISTANCE_SQUARED
/// Pathfinder used instead of raw distance, parameter dictates whether we're reusing paths and ignoring units (fast but rough) or not (slow but accurate)
#define AUI_RELIGION_SCORE_CITY_FOR_MISSIONARY_USE_PATHFINDER_FOR_DISTANCE (true)
/// When targetting an enemy holy city, takes the square root of the current score if it's lower than halving it
#define AUI_RELIGION_SCORE_CITY_FOR_MISSIONARY_HOLY_CITY_TAKE_SQRT_SCORE_IF_RESULT_IS_LOWER
/// When finding a nearby conversion target, cities that will convert to the AI's religion passively are ignored
#define AUI_RELIGION_HAVE_NEARBY_CONVERSION_TARGET_IGNORE_TARGET_THAT_WILL_CONVERT_PASSIVELY
/// Tweaks the base number of inquisitors needed (default is 1)
#define AUI_RELIGION_HAVE_ENOUGH_INQUISITORS_TWEAKED_BASE_NUMER (0)
/// Multiplies the Great Merchant score by this value if the AI is Venice and will use the Great Merchant to acquire a city
#define AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_VENICE_MERCHANT_BOOST (10)
/// Scales the non-spaceship scoring of Great Engineers with Wonder Competitiveness
#define AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_ENGINEER_USES_WONDER_COMPETITIVENESS (1000.0 / 3.0)
/// Fixes the bug where the AI scores inquisitors if it already has enough, not when it needs them
#define AUI_RELIGION_FIX_GET_DESIRED_FAITH_GREAT_PERSON_INQUISITOR_CHECK
/// Scales the score of missionaries and inquisitors with how many turns have elapsed (value is the base score for missionaries and inquisitors)
#define AUI_RELIGION_GET_DESIRED_FAITH_GREAT_PERSON_SCALE_MISSIONARY_INQUISITOR_WITH_TURNS_ELAPSED (1000)
/// When comparing the final score for beliefs, the score of the lowest scored belief will be subtracted from all beliefs
#define AUI_RELIGION_RELATIVE_BELIEF_SCORE

// Site Evaluation Stuff
/// Tweaks the multiplier given to the happiness score luxury resources that the player does not have (multiplier is applied once for importing, twice and times 2 for don't have at all)
#define AUI_SITE_EVALUATION_COMPUTE_HAPPINESS_VALUE_TWEAKED_UNOWNED_LUXURY_MULTIPLIER (4)
/// Player bonuses from happiness (eg. extra happiness from luxuries, happiness from multiple luxury types) is included in the calculation
#define AUI_SITE_EVALUATION_FIX_COMPUTE_HAPPINESS_VALUE_PLAYER_SOURCES
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
/// The helper values to compute the yield values of plots will recognize when the plot being targetted will be settled, so future yield will be different
#define AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_RECOGNIZE_CITY_PLOT
/// Adds the happiness value of natural wonders to the happiness value of a plot
#define AUI_SITE_EVALUATION_FIX_COMPUTE_HAPPINESS_VALUE_NATURAL_WONDERS
/// Considers a player's traits when computing the yield value of a plot (eg. Russia gets bonus hammers if the plot is a strategic resource)
#define AUI_SITE_EVALUATION_COMPUTE_YIELD_VALUE_CONSIDER_PLAYER_TRAIT
/// Fixes missing code responsible for granting starting forest flavor to Iroquois
#define AUI_START_SITE_EVALUATION_FIX_MISSING_IROQUOIS_FLAVOR

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
/// Always withdraw from a city if the AI has no melee units left in the zone
#define AUI_TACTICAL_SELECT_POSTURE_ALWAYS_WITHDRAW_FROM_CITY_IF_NO_MELEE_UNITS
/// More aggressive city attack postures will consider melee unit count before executing
#define AUI_TACTICAL_CITY_ATTACK_POSTURE_CONSIDERS_MELEE_COUNT (1)
/// Priority randomizer is now applied to tactical moves for non-barbarians players as well
#define AUI_TACTICAL_USE_RANDOM_FOR_NON_BARBARIANS
#ifdef AUI_BINOM_RNG
/// Uses the binomial RNG for the randomness factor in tactical move priority
#define AUI_TACTICAL_TWEAKED_MOVE_PRIORITIES_RANDOM_BINOMIAL
#endif
/// Tweaks capture/damage city moves so that ranged attacks aren't wasted on cities with 1 HP (originally from Ninakoru's Smart AI)
#define AUI_TACTICAL_TWEAKED_CAPTURE_DAMAGE_CITY_MOVES
/// Uses Boldness to determine whether long sieges are worth it instead of a set 8 turns
#define AUI_TACTICAL_TWEAKED_DAMAGE_CITY_MOVE_USE_BOLDNESS
/// Sets a different threshold for the amount of damage an enemy unit is required to take before the AI is willing to send its units on suicide attacks
#define AUI_TACTICAL_TWEAKED_DESTROY_UNIT_MOVE_SUICIDE_THRESHOLD (50)
/// Tweaks the algorithm for Plot Heal Moves to keep March promotions in mind and make sure we don't overheal if we're under threat
#define AUI_TACTICAL_TWEAKED_HEAL_MOVES
#ifdef AUI_PLOT_COUNT_OCCURANCES_IN_LIST
/// Uses a different algorithm for plotting intercept moves instead of relying on dominance zones (originally from Ninakoru's Smart AI)
#define AUI_TACTICAL_TWEAKED_AIR_INTERCEPT
#endif
#ifdef AUI_TACTICAL_HELPERS_POSITIONING_AND_ORDER
/// If a barbarian is already at the targetted plot, patrol around the target instead
#define AUI_TACTICAL_EXECUTE_BARBARIAN_MOVES_PATROL_IF_ON_TARGET
/// If a unit has moves remaining at the end of its reposition move, patrol to the best possible position as well
#define AUI_TACTICAL_EXECUTE_REPOSITION_MOVES_PATROL_IF_MOVES_REMAIN
/// Reorders the parts of the ExecuteAttack() algorithm to push ranged units before melee ones (originally from Ninakoru's Smart AI, heavily modified since)
#define AUI_TACTICAL_TWEAKED_EXECUTE_ATTACK
#ifdef AUI_TACTICAL_TWEAKED_EXECUTE_ATTACK
/// If the target is a city, add the city's planned ranged attack to the expected self damage of melee units
#define AUI_TACTICAL_TWEAKED_EXECUTE_ATTACK_NO_MELEE_SUICIDE_AGAINST_CITY
/// Ranged units will always want to reposition, even if they can already attack at the target
#define AUI_TACTICAL_EXECUTE_ATTACK_FIDDLY_ARCHERS
/// If a target is not dead, units that can still move after attacking will retreat to a distance from where they can attack the target next turn
#define AUI_TACTICAL_EXECUTE_ATTACK_PARTHIAN_TACTICS
/// Postpones the MoveToEmptySpaceNearTarget() parts of the code until after ranged attacks have processed so ranged units can move into range and fire
#define AUI_TACTICAL_TWEAKED_EXECUTE_ATTACK_POSTPONE_MELEE_MOVE
#endif
#endif
#ifdef AUI_UNIT_EXTRA_ATTACKS_GETTER
/// Fix for units that can attack multiple times per turn so they can now participate in the same ExecuteAttackMove()
#define AUI_TACTICAL_FIX_EXECUTE_ATTACK_BLITZ
#endif
/// Sets a very high danger value for water tiles if a unit needs to embark onto the tile (originally from Ninakoru's Smart AI)
#define AUI_TACTICAL_TWEAKED_MOVE_TO_SAFETY_HIGH_DANGER_EMBARK
/// Sets a lower danger value for city tiles depending on the health of the city
#define AUI_TACTICAL_TWEAKED_MOVE_TO_SAFETY_LOW_DANGER_CITY
#ifdef AUI_TACTICAL_TWEAKED_ACCEPTABLE_DANGER
/// Considers units of the same type when executing move to safety (important if health is a factor in selecting units for this movetype)
#define AUI_TACTICAL_TWEAKED_MOVE_TO_SAFETY_CONSIDER_SAME_UNIT_TYPE
#endif
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
#endif
#ifdef AUI_TACTICAL_EXECUTE_SWAP_TO_PLOT
/// Uses the new ExecuteSwapToPlot() function when moving blocking units
#define AUI_TACTICAL_EXECUTE_MOVE_BLOCKING_UNIT_USES_SWAP
#endif
/// If a blocking unit is moved, it will no longer automatically end the just moved unit's turn
#define AUI_TACTICAL_FIX_EXECUTE_MOVE_BLOCKING_UNIT_SAVE_MOVES
/// When moving a blocking unit, allow it to prioritize tiles from which it can still move and/or attack this turn
#define AUI_TACTICAL_EXECUTE_MOVE_BLOCKING_UNIT_ALLOW_ZERO_MOVE_PRIORITY
/// Checks to see if the unit being moved by MoveToEmptySpaceNearTarget() or MoveToEmptySpaceTwoFromTarget() isn't already within 1 or 2 tiles of the target respectively
#define AUI_TACTICAL_FIX_MOVE_TO_EMPTY_SPACE_FROM_TARGET_CHECK_UNIT_PLOT_FIRST
/// Allow blocking friendly units to move out of the way when moving right next to a target (since 
#define AUI_TACTICAL_FIX_MOVE_TO_EMPTY_SPACE_FROM_TARGET_MOVE_BLOCKING
/// When moving to an empty tile near a target, weigh tiles by the amount of turns it will take to get there, then by plot danger
#define AUI_TACTICAL_MOVE_TO_EMPTY_SPACE_FROM_TARGET_WEIGH_BY_TURNS_AND_DANGER
/// When processing civilians in an escorted move, have them move to safety if they aren't currently defended and are in danger
#define AUI_TACTICAL_PLOT_SINGLE_HEX_OPERATION_MOVES_CIVILIAN_TO_SAFETY
/// Units that cannot caputre cities will not attempt to do so
#define AUI_TACTICAL_FIX_NO_CAPTURE
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
/// Only consider moving towards undefended barbarian camps as a "passive move"
#define AUI_TACTICAL_FIX_FIND_PASSIVE_BARBARIAN_LAND_MOVE_ONLY_UNDEFENDED_CAMPS
/// Units under max health are also eligible to retreat if the danger level is too high
#define AUI_TACTICAL_FIX_PLOT_MOVES_TO_SAFETY_NOT_MAX_HEALTH_CHECK_DANGER
/// Barbarians will move toward trade routes even if they're out of single-turn move range
#define AUI_TACTICAL_FIND_BARBARIAN_GANK_TRADE_ROUTE_TARGET_MULTITURN
/// Sea scouts are also no longer commandeered by the tactical AI, but all scouts in armies are 
#define AUI_TACTICAL_FIX_COMMANDEER_UNITS_SCOUTS
/// Air units are no longer skipped by the tactical AI (since homeland AI can send them out on intercepts and rebases)
#define AUI_TACTICAL_FIX_REVIEW_UNASSIGNED_UNITS_DO_NOT_SKIP_AIR
/// All non-combat units, not just Great Generals and Admirals, can be handled properly in operations (useful for mods!)
#define AUI_TACTICAL_FIX_GENERALIZED_CIVILIAN_SUPPORT
/// When choosing a plot to move to, the plot's score no longer needs to be positive, it just needs to be more than the great general's current plot score
#define AUI_TACTICAL_MOVE_GREAT_GENERAL_ONLY_REQUIRE_POSITIVE_DELTA
/// Loads range to check for queued attacks and movement of great general unit from XML instead of using a hardcoded value
#define AUI_TACTICAL_SCORE_GREAT_GENERAL_PLOT_USE_XML_RANGE
/// Scales the score from nearby queued attacks based on the general's combat modifier
#define AUI_TACTICAL_SCORE_GREAT_GENERAL_PLOT_SCALE_SCORE_BY_COMBAT_MODIFIER
#ifdef AUI_UNIT_HEALRATE_ASSUME_EXTRA_HEALRATE_FROM_UNIT
/// If the civilian support unit can heal adjacent tiles, consider the health of adjacent units as well
#define AUI_TACTICAL_SCORE_GREAT_GENERAL_PLOT_CONSIDER_MEDIC
#endif
/// Runs moves for each operation twice, in case the operation switches states mid-move
#define AUI_TACTICAL_PLOT_OPERATIONAL_ARMY_MOVES_MOVE_TWICE
/// A plot's extra explore score from being owned is check before the check for being adjacent to an owned tile, so it is applied properly
#define AUI_TACTICAL_FIX_FIND_BARBARIAN_EXPLORE_TARGET_OWNED_TILE_CHECKER
/// If the best barbarian land target is not a combat move, have the unit move onto the tile instead of next to it
#define AUI_TACTICAL_FIX_FIND_BEST_BARBARIAN_LAND_MOVE_NO_ADJACENT_IF_NOT_COMBAT
/// If a target type is not specified, all improvement types are now valid targets (not just non-citadel improvements without resources)
#define AUI_TACTICAL_FIX_FIND_NEARBY_TARGET_ALL_IMPROVEMENT_TYPES_POSSIBLE
/// Ignore tiles with a higher danger value than the barbarian's current tile when finding explore tiles
//#define AUI_TACTICAL_FIND_BARBARIAN_EXPLORE_TARGET_IGNORE_HIGH_DANGER_TILES
/// Fixes a possible null pointer when converting a target's coordinates to a plot
#define AUI_TACTICAL_FIX_FIND_BEST_BARBARIAN_SEA_MOVE_POSSIBLE_NULL_POINTER
/// Fixes a possible null pointer when selecting a naval escort operation's escort
#define AUI_TACTICAL_FIX_PLOT_NAVAL_ESCORT_OPERATION_MOVES_POSSIBLE_NULL_POINTER

// Tactical Analysis Map Stuff
/// Enables a minor adjustment for ranged units to account for possibly being able to move and shoot at a tile
#define AUI_TACTICAL_ANALYSIS_MAP_MARKING_ADJUST_RANGED
/// Checks for cities in addition to citadel improvements
#define AUI_TACTICAL_ANALYSIS_MAP_MARKING_INCLUDE_CITIES
/// City strength does not decrease completely with hitpoints
#define AUI_TACTICAL_ANALYSIS_MAP_CALCULATE_MILITARY_STRENGTHS_LIMITED_CITY_GIMPING
/// Distance dropoff only starts taking place at 4 tile range instead of immediately
#define AUI_TACTICAL_ANALYSIS_MAP_CALCULATE_MILITARY_STRENGTHS_LIMITED_DISTANCE_DROPOFF
/// Uses pathfinding turns instead of raw distance for strength multipliers
#define AUI_TACTICAL_ANALYSIS_MAP_CALCULATE_MILITARY_STRENGTHS_USE_PATHFINDER
/// When calculating unit strengths, the unit's city attack power will be used instead of its unmodified damage
#define AUI_TACTICAL_ANALYSIS_MAP_CALCULATE_MILITARY_STRENGTHS_CONSIDER_CITY_ATTACK_BONUS

// Team Stuff
#ifdef AUI_FLAVOR_MANAGER_RANDOMIZE_WEIGHTS_ON_ERA_CHANGE
/// Randomizes weights by half their usual amount on each era change
#define AUI_TEAM_SET_CURRENT_ERA_RANDOMIZE_WEIGHTS_BY_HALF_ON_ERA_CHANGE
#endif

// Tech Classes Stuff
#ifdef AUI_GS_SCIENCE_FLAVOR_BOOST
/// Applies the necessary science flavor boost to techs in addition to buildings, wonders, policies, and units
#define AUI_PLAYERTECHS_ADD_FLAVOR_AS_STRATEGIES_USE_SCIENCE_FLAVOR_BOOST
#endif
/// Buildings that contribute towards getting an ideology act as a unique building for the purposes of tech scoring
#define AUI_PLAYERTECHS_RESET_IDEOLOGY_UNLOCKERS_COUNT_AS_UNIQUE

// Tech AI Stuff
/// The AI wants an expensive tech if it's selecting a free tech
#define AUI_TECHAI_CHOOSE_NEXT_TECH_FREE_TECH_WANTS_EXPENSIVE

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

// Trait Classes Stuff
/// Scales the threshold wonder competitiveness for choosing an engineer with game turn instead of having it be two binary checks
#define AUI_PLAYERTRAITS_CHOOSE_MAYA_BOOST_ENGINEER_GRANULAR_WONDER_COMPETITIVENESS

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
#if defined(AUI_UNIT_CAN_EVER_RANGE_STRIKE_AT_OVERLOAD) && defined(AUI_UNIT_RANGE_PLUS_MOVE)
/// Adds two new functions that return whether a unit can move and ranged strike at a plot (originally from Ninakoru's Smart AI)
#define AUI_UNIT_CAN_MOVE_AND_RANGED_STRIKE
#endif
#endif

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
/// When voting for a player, the AI will now adjust for the fact that the voting system is First-Past-The-Post (so it will try to vote against player as well)
#define AUI_VOTING_SCORE_VOTING_CHOICE_PLAYER_ADJUST_FOR_FPTP
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

// Wonder Production AI Stuff
/// Does a flavor update each time a wonder is to be chosen (helps when multiple wonders are to be chosen in a single turn)
#define AUI_WONDER_PRODUCTION_CHOOSE_WONDER_FLAVOR_UPDATE
/// When determining how many turns a wonder will take to build, use the player's value for how much it will cost instead of the raw XML value
#define AUI_WONDER_PRODUCTION_FIX_CHOOSE_WONDER_TURNS_REQUIRED_USES_PLAYER_MOD
/// When choosing a wonder to be built by a great engineer, the AI will weigh wonders by how much production will be needed after the hurry
#define AUI_WONDER_PRODUCTION_CHOOSE_WONDER_FOR_GREAT_ENGINEER_WEIGH_COST
/// Divides base weight by this number for all non-world wonders
#define AUI_WONDER_PRODUCITON_CHOOSE_WONDER_FOR_GREAT_ENGINEER_WANT_WORLD_WONDER (10)

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