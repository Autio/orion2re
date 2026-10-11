#include "pch.h"

#include "raceopt.h"

namespace raceopt {
    s_pick_rules pick_rules = {
        10,
        -10,
        4,
        {
            {{-4, -50}, {3, 50}, {6, 100}},
            {{-3, -1}, {4, 2}, {7, 4}},
            {{-3, -1}, {3, 1}, {6, 2}},
            {{-3, -1}, {3, 1}, {6, 2}},
            {{-4, -1}, {5, 1}, {8, 2}},
            {{-2, -20}, {3, 25}, {7, 50}},
            {{-2, -20}, {2, 20}, {4, 50}},
            {{-2, -10}, {2, 10}, {4, 20}},
            {{-3, -10}, {3, 10}, {6, 20}},
        },
        {-4, 0, 7, 6},
        {
            -5,
            6,
            5,
            6,
            1,
            2,
            -1,
            3,
            4,
            10,
            -6,
            3,
            -4,
            8,
            10,
            4,
            6,
            3,
            3,
            4,
            5,
            4,
        },
    };

    const TRAIT primary_pick_traits[PRIMARY_PICK_COUNT] = {
        TRAIT_POP_GROWTH_BONUS,
        TRAIT_FOOD_BONUS,
        TRAIT_PRODUCTION_BONUS,
        TRAIT_RESEARCH_BONUS,
        TRAIT_TAX_BONUS,
        TRAIT_SHIP_DEFENSE,
        TRAIT_SHIP_ATTACK,
        TRAIT_GROUND_COMBAT_BONUS,
        TRAIT_SPYING_BONUS,
    };

    const GOVERNMENT selectable_governments[GOVERNMENT_PICK_COUNT] = {
        GOVERNMENT_FEUDAL,
        GOVERNMENT_DICTATORSHIP,
        GOVERNMENT_DEMOCRACY,
        GOVERNMENT_UNIFICATION,
    };

    const s_special_pick_identity special_pick_identities[SPECIAL_PICK_COUNT] = {
        {TRAIT_LOW_G_WORLD, 1},
        {TRAIT_HEAVY_G_WORLD, 1},
        {TRAIT_AQUATIC, 1},
        {TRAIT_SUBTERRANEAN, 1},
        {TRAIT_LARGE_HOME_WORLD, 1},
        {TRAIT_RICH_HOME_WORLD, 1},
        {TRAIT_RICH_HOME_WORLD, -1},
        {TRAIT_ANCIENT_HOME_WORLD, 1},
        {TRAIT_CYBERNETIC, 1},
        {TRAIT_EATS_MINERALS, 1},
        {TRAIT_REPULSIVE, 1},
        {TRAIT_CHARISMATIC, 1},
        {TRAIT_UNCREATIVE, 1},
        {TRAIT_CREATIVE, 1},
        {TRAIT_ENVIRONMENT_IMMUNE, 1},
        {TRAIT_FANTASTIC_TRADERS, 1},
        {TRAIT_TELEPATHIC, 1},
        {TRAIT_LUCKY, 1},
        {TRAIT_OMNISCIENCE, 1},
        {TRAIT_STEALTHY_SHIPS, 1},
        {TRAIT_TRANS_DIMENSIONAL, 1},
        {TRAIT_WARLORD, 1},
    };
} // namespace raceopt