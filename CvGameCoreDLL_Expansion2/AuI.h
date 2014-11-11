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

// AI Operations Stuff
/// Tweaks the boldness check for whether a settler should proceed without escort
#define AUI_OPERATION_FOUND_CITY_TWEAKED_NO_ESCORT_BOLDNESS (8)
/// Adds a random value to the boldness check so it doesn't always succeed or fail
#define AUI_OPERATION_FOUND_CITY_TWEAKED_NO_ESCORT_RANDOM_VALUE (7)
/// If it's available, opts for the binomial RNG for the boldness check's random factor instead of the flat RNG
#define AUI_OPERATION_FOUND_CITY_TWEAKED_NO_ESCORT_RANDOM_BINOMIAL

// Worker Automation Stuff
/// Automated Inca workers know that there is no maintenance on hills, so routines are adjusted as a result
#define AUI_WORKER_INCA_HILLS
/// Automated workers check for fallout before other improvements (instead of after) and do not care about the improvement's build time or cost
#define AUI_WORKER_FIX_FALLOUT
/// Removes the bias to chop forests after optics (since it doesn't actually offer a gameplay improvement
#define AUI_WORKER_NO_CHOP_BIAS
/// Faith now affects tile evaluation for workers, it pulls from culture multiplier though
#define AUI_WORKER_EVALUATE_FAITH
/// Automated Dutch workers now remove marshes on tiles with resources (since polders won't be built anyway)
#define AUI_WORKER_DUTCH_MARSH_RESOURCES
/// AI Celt workers will no longer leave forests unimproved once they enter the Industrial Era
#define AUI_WORKER_CELT_FOREST_IMPROVE_INDUSTRIAL
/// Automated workers value strategic resources that a player has none of higher than strategic resources that the player has used all of
#define AUI_WORKER_TWEAKED_DONT_HAVE_MULTIPLIER

// Danger Plots Stuff
/// Better danger calculation for ranged units (originally from Ninakoru's Smart AI, but heavily modified since)
#define AUI_DANGER_PLOTS_TWEAKED_RANGED

// Grand Strategy Stuff
/// Enables use of Grand Strategy Priority; the function returns the ratio of the requested GS's priority to the active GS's priority (1.0 if the requested GS is the active GS)
#define AUI_GS_PRIORITY_RATIO
/// VITAL FOR MOST FUNCTIONS! Use float instead of int for certain variables (to retain information during division)
#define AUI_GS_USE_FLOATS
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
/// Boosts the science flavor of buildings, wonders, and techs depending on how well the tech requirements of significant GS's is met (eg. have Archaeology, have Internet, etc.)
#define AUI_GS_SCIENCE_FLAVOR_BOOST
/// Replaces the logging function with one that allows for easy creation of graphs within Excel
#define AUI_GS_BETTER_LOGGING

// Military AI Stuff
/// VITAL FOR MOST FUNCTIONS! Use float instead of int for certain variables (to retain information during division); some double types are also replaced for increased speed
#define AUI_MILITARY_USE_FLOATS
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
/// Fixes a bug that would only put a log header in one AI's log, not all their logs
#define AUI_MILITARY_BETTER_LOGGING

// Religion/Belief Stuff
/// Checks all of player's cities for whether or not a city is within conversion target range, not just the player's capital
#define AUI_RELIGION_CONVERSION_TARGET_NOT_JUST_CAPITAL

// Tactical Analysis Map Stuff
/// Enables a minor adjustment for ranged units to account for possibly being able to move and shoot at a tile
#define AUI_TACTICAL_MAP_ANALYSIS_MARKING_ADJUST_RANGED
/// Checks for cities in addition to citadel improvements
#define AUI_TACTICAL_MAP_ANALYSIS_MARKING_INCLUDE_CITIES

// Unit Stuff
/// Adds a function to return a unit's movement range if it can attack after a move + the unit's range (originally from Ninakoru's Smart AI)
#define AUI_UNIT_RANGE_PLUS_MOVE
/// Fixes the check for whether ranged damage would be more than heal rate to happen AFTER total damage has been calculated (so lots of tiny still counts)
#define AUI_UNIT_FIX_UNDER_ENEMY_RANGED_ATTACK_HEALRATE
/// Adds a function to return whether a unit can range strike at a target from a plot (originally from Ninakoru's Smart AI)
#define AUI_UNIT_CAN_EVER_RANGE_STRIKE_AT_FROM_PLOT
/// Overloads the vanilla canEverRangeStrikeAt() function to call the new canEverRangeStrikeAtFromPlot() function if it's enabled (originally from Ninakoru's Smart AI)
#define AUI_UNIT_CAN_EVER_RANGE_STRIKE_AT_OVERLOAD
/// Adds two new functions that return whether a unit can move and ranged strike at a plot; depends on previous two defines (originally from Ninakoru's Smart AI)
#define AUI_UNIT_CAN_MOVE_AND_RANGED_STRIKE

// Promition Stuff (within CvUnit.cpp)
/// Use float instead of int for most variables (to retain information during division) inside AI_promotionValue()
#define AUI_UNIT_PROMOTION_USE_FLOATS
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
/// VITAL FOR MOST FUNCTIONS! Use float instead of int for certain variables (to retain information during division)
#define AUI_VOTING_USE_FLOATS
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