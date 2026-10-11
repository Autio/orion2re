#include "pch.h"
#include "game/design_config.h"

namespace DESIGN {
    static_assert(SHIP_DESIGN_REBUILD_FIELD_COUNT == TECH_FIELD_COUNT);

    uint16_t megafluxers_tactical_bonus = 25;
    uint16_t megafluxers_strategic_bonus = 50;
    uint8_t augmented_engines_speed_bonus = 5;
    bool battle_pods_speed_bonus = true;
    uint8_t minimum_combat_speed = 255;
    bool rebuild_ship_designs_after_field[SHIP_DESIGN_REBUILD_FIELD_COUNT] = {
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        true,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        true,
        false,
        true,
    };
} // namespace DESIGN