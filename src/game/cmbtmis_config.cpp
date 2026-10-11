#include "pch.h"
#include "game/cmbtmis.h"

namespace CMBTMIS {
    bool missile_cross_round_merge = true;
    bool smarter_fighter_retargeting = false;
    uint16_t fighter_max_pursuit_turns = 3;
    e_fighter_pursuit_fallback fighter_pursuit_fallback = FIGHTER_PURSUIT_PURSUE;
    s_missile_speed_base speed_base = {
        12,
        12,
        12,
        12,
        20,
        20,
        24,
        24,
        10,
        6,
        8,
        8,
    };
    uint8_t proton_torpedo_apply_speed_base = 0;
    uint8_t speed_fast_bonus = 4;
    uint8_t plasma_torpedo_dissipation = 5;
    uint8_t lightning_field_ratio = 50;
    e_lightning_field_mode lightning_field_mode = LIGHTNING_FIELD_MODE_CLASSIC;
    s_flyer_hit_points flyer_hit_points = {
        4,
        8,
        12,
        16,
        4,
        6,
        8,
        10,
    };
    s_fighter_shots fighter_shots = {4, 1, 2};
    s_flyer_combat_modifier flyer_combat_modifier = {
        -10,
        15,
        40,
        70,
        50,
    };
    s_missile_defense missile_defense = {
        25,
        50,
        70,
        100,
        130,
        70,
        0,
        50,
        30,
        20,
        40,
        70,
    };
} // namespace CMBTMIS