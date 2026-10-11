#include "pch.h"

namespace DESIGN {
    static_assert(MINIATURIZATION_TECHNOLOGY_COUNT == TECH_APP_COUNT);

    uint8_t missile_x3_shot = 0;
    bool missile_default_x2_rack = false;
    uint8_t alternative_refit = 0;
    uint8_t design_button_computer = DESIGN_BUTTON_BEHAVIOR_UPDATE;
    uint8_t design_button_shield = DESIGN_BUTTON_BEHAVIOR_UPDATE;
    uint8_t death_spore_list_damage = 0;
    uint8_t bio_terminator_list_damage = 0;
    e_clear_button_computer_behavior clear_button_computer =
        CLEAR_BUTTON_COMPUTER_UPDATE;
    e_clear_button_shield_behavior clear_button_shield =
        CLEAR_BUTTON_SHIELD_INTACT;

    static const int16_t missile_rack_quantities[2][MISSILE_RACK_COUNT] = {
        {2, 5, 10, 15, 20},
        {2, 3, 5, 10, 20},
    };
    s_missile_rack missile_racks[MISSILE_RACK_COUNT] = {
        {10, 10},
        {20, 20},
        {30, 30},
        {35, 35},
        {40, 40},
    };
    s_weapon_arc_cost_and_space
        arc_cost_and_space[WEAPON_ARC_MODIFIER_COUNT] = {
            {25, 25},
            {50, 50},
    };

    s_miniaturization_settings miniaturization = {
        {100, 75, 55, 40, 30, 25},
        {
            {1000, 800, 650, 500, 350, 250},
            {1000, 800, 700, 600, 500, 400},
            {1000, 1000, 1000, 1000, 1000, 1000},
        },
    };

    uint8_t miniaturization_categories[MINIATURIZATION_TECHNOLOGY_COUNT] = {
        0,
        1,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        1,
        0,
        0,
        0,
        2,
        0,
        0,
        2,
        0,
        0,
        1,
        0,
        0,
        1,
        0,
        0,
        0,
        1,
        2,
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        0,
        0,
        0,
        0,
        0,
        1,
        1,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        0,
        0,
        2,
        1,
        1,
        0,
        1,
        0,
        0,
        2,
        1,
        0,
        2,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        1,
        2,
        2,
        0,
        1,
        0,
        0,
        0,
        0,
        1,
        0,
        0,
        1,
        1,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        1,
        1,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        0,
        0,
        1,
        1,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        1,
        0,
        0,
        1,
        0,
        0,
        1,
        0,
        1,
        0,
        1,
        1,
        0,
        1,
        0,
        0,
        0,
        0,
        1,
        1,
        0,
        1,
        0,
        0,
        0,
        1,
        0,
        0,
        0,
        0,
        0,
        1,
        1,
        0,
        1,
        1,
        0,
        1,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        0,
        0,
        1,
        0,
        1,
        0,
        2,
        0,
        0,
        0,
        1,
        0,
        0,
        1,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    };

    int16_t Missile_Rack_Quantity(int16_t rack_index) {
        if (rack_index < 0 || rack_index >= MISSILE_RACK_COUNT) {
            return 0;
        }
        return missile_rack_quantities[missile_x3_shot != 0][rack_index];
    }

    int16_t Missile_Rack_Index(int16_t quantity) {
        for (int16_t rack_index = 0; rack_index < MISSILE_RACK_COUNT; ++rack_index) {
            if (Missile_Rack_Quantity(rack_index) == quantity) {
                return rack_index;
            }
        }
        return -1;
    }

    int16_t Missile_Rack_Size(int16_t quantity) {
        int16_t rack_index = Missile_Rack_Index(quantity);
        return rack_index >= 0 ? missile_racks[rack_index].space : 0;
    }

    int16_t Missile_Rack_Cost(int16_t quantity) {
        int16_t rack_index = Missile_Rack_Index(quantity);
        return rack_index >= 0 ? missile_racks[rack_index].cost : 0;
    }

    bool Should_Autodesign_Use_X3_Rack(
        int16_t quantity, int16_t fitting_weapon_count, int16_t minimum_weapon_count) {
        return missile_x3_shot != 0 && quantity == 5 &&
            fitting_weapon_count <= minimum_weapon_count;
    }

    uint16_t Miniaturization_Cost_Multiplier(int16_t level) {
        int index = level >= 0 && level < MINIATURIZATION_LEVEL_COUNT - 1
            ? level
            : MINIATURIZATION_LEVEL_COUNT - 1;
        return miniaturization.cost[index];
    }

    uint16_t Miniaturization_Space_Multiplier(int16_t level, int8_t category) {
        int level_index = level >= 0 && level < MINIATURIZATION_LEVEL_COUNT - 1
            ? level
            : MINIATURIZATION_LEVEL_COUNT - 1;
        int category_index = category >= 0 && category < MINIATURIZATION_CATEGORY_COUNT
            ? category
            : MINIATURIZATION_CATEGORY_GROUP_3;
        return miniaturization.space[category_index][level_index];
    }
} // namespace DESIGN