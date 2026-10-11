#include "pch.h"
#include "game/cmbtfire.h"

namespace CMBTFIRE {
    uint16_t civilian_armor[ARMOR_COUNT] = {
        100,
        100,
        100,
        100,
        100,
        100,
        100,
    };
    uint16_t ground_defense_armor_multiplier = 100;
    uint8_t marine_kill_damage = 5;
    uint8_t marine_kill_damage_blocker = SPECIAL_DAMPER_FIELD;
    uint16_t analyzer_bonus = 100;
    uint8_t fighter_beam_no_plus_one = 0;
    uint8_t fighters_use_point_defense = 0;
    uint8_t heavy_fighter_can_scramble = 0;
    uint8_t double_range_accuracy_penalty = 1;
    uint8_t double_range_damage_penalty = 0;
    uint8_t planet_non_enveloping = 0;
    uint8_t simplified_beam_formula = 0;
    uint8_t hit_threshold_fixed_value = 40;
    uint8_t ion_affects_antarans = 0;
    uint8_t absorber_damage_reduction = 25;
    uint8_t damper_field_ratio = 25;
    uint8_t defensive_range_bonus = 0;
    uint8_t broken_engine_explodes = 1;
    s_weapon_range weapon_range = {
        8,
        5,
        5,
        5,
        4,
        5,
        5,
        1,
        5,
    };
    s_spherical_range_to_ship spherical_range_to_ship = {
        6,
        4,
        6,
        6,
    };
    s_spherical_range_to_missile spherical_range_to_missile = {
        122,
        96,
        122,
        135,
    };
} // namespace CMBTFIRE