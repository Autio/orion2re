#include "pch.h"
#include "game/racesel.h"

namespace RACESEL {
    s_stock_race stock_race[STOCK_RACE_COUNT] = {
        {.government = GOVERNMENT_DICTATORSHIP, .defense = 3, .arti_world = 1},
        {.government = GOVERNMENT_DICTATORSHIP, .attack = 2, .ground = 2, .highg_world = 1},
        {.government = GOVERNMENT_DICTATORSHIP, .spying = 3, .stealthy_ships = 1},
        {.defense = 2, .attack = 2, .telepathic = 1, .omniscient = 1},
        {.government = GOVERNMENT_DICTATORSHIP, .money = 3, .lowg_world = 1, .fantastic_traders = 1, .lucky = 1},
        {.government = GOVERNMENT_DEMOCRACY, .charismatic = 1},
        {.government = GOVERNMENT_UNIFICATION, .farming = 2, .industry = 2, .large_hw = 1, .uncreative = 1},
        {.government = GOVERNMENT_DICTATORSHIP, .industry = 3, .cybernetic = 1},
        {.government = GOVERNMENT_DICTATORSHIP, .attack = 3, .hw_richness = 1, .warlord = 1},
        {.government = GOVERNMENT_DICTATORSHIP, .science = 3, .lowg_world = 1, .large_hw = 1, .creative = 1},
        {.growth = 3, .farming = 2, .spying = 1, .subterranean = 1, .large_hw = 1},
        {.government = GOVERNMENT_DICTATORSHIP, .growth = 1, .lithovore = 1, .repulsive = 1, .tolerant = 1},
        {.government = GOVERNMENT_DICTATORSHIP,
            .aquatic = 1,
            .trans_dimensional = 1},
    };
}