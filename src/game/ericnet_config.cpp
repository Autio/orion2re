#include "pch.h"

namespace ERICNET {
    uint8_t starting_outpost_ship = 0;
    uint8_t starting_ships = 1;
    uint8_t crew_gains_turn_xp = 1;

    uint8_t combat_reengage_buildings[COMBAT_REENGAGE_BUILDING_CAPACITY] = {
        BUILDING_ARTEMIS_SYSTEM_NET,
        BUILDING_BATTLESTATION,
        BUILDING_MISSILE_BASE,
        BUILDING_GROUND_BATTERIES,
        BUILDING_STAR_BASE,
        BUILDING_STAR_FORTRESS,
        BUILDING_STELLAR_CONVERTER,
        BUILDING_FIGHTER_GARRISON,
        BUILDING_NO_BUILDING,
    };
} // namespace ERICNET