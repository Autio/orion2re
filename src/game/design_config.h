#pragma once

#include <cstdint>

namespace DESIGN {
    enum e_weapon_arc_modifier : uint8_t {
        WEAPON_ARC_MODIFIER_EXTENDED,
        WEAPON_ARC_MODIFIER_ALL_ROUND,
        WEAPON_ARC_MODIFIER_COUNT,
    };

    enum {
        DESIGN_BUTTON_BEHAVIOR_UPDATE,
        DESIGN_BUTTON_BEHAVIOR_INTACT,
    };

    enum e_clear_button_computer_behavior : uint8_t {
        CLEAR_BUTTON_COMPUTER_UPDATE,
        CLEAR_BUTTON_COMPUTER_CLEAR,
        CLEAR_BUTTON_COMPUTER_INTACT,
    };

    enum e_clear_button_shield_behavior : uint8_t {
        CLEAR_BUTTON_SHIELD_INTACT,
        CLEAR_BUTTON_SHIELD_UPDATE,
        CLEAR_BUTTON_SHIELD_CLEAR,
    };

    enum e_miniaturization_category : uint8_t {
        MINIATURIZATION_CATEGORY_GROUP_1 = 0,
        MINIATURIZATION_CATEGORY_GROUP_2 = 1,
        MINIATURIZATION_CATEGORY_GROUP_3 = 2,
        MINIATURIZATION_CATEGORY_COUNT = 3,
    };

    constexpr int MINIATURIZATION_LEVEL_COUNT = 6;
    constexpr int MINIATURIZATION_TECHNOLOGY_COUNT = 212;
    constexpr int MISSILE_RACK_COUNT = 5;
    constexpr int SHIP_DESIGN_REBUILD_FIELD_COUNT = 83;

    struct s_missile_rack {
        uint8_t space;
        uint8_t cost;
    };

    struct s_weapon_arc_cost_and_space {
        uint8_t cost;
        uint8_t space;
    };

    struct s_miniaturization_settings {
        uint16_t cost[MINIATURIZATION_LEVEL_COUNT];
        uint16_t space[MINIATURIZATION_CATEGORY_COUNT][MINIATURIZATION_LEVEL_COUNT];
    };

    extern s_miniaturization_settings miniaturization;
    extern uint8_t miniaturization_categories[MINIATURIZATION_TECHNOLOGY_COUNT];
    extern uint8_t missile_x3_shot;
    extern bool missile_default_x2_rack;
    extern s_missile_rack missile_racks[MISSILE_RACK_COUNT];
    extern s_weapon_arc_cost_and_space
        arc_cost_and_space[WEAPON_ARC_MODIFIER_COUNT];
    extern uint8_t alternative_refit;
    extern uint8_t design_button_computer;
    extern uint8_t design_button_shield;
    extern e_clear_button_computer_behavior clear_button_computer;
    extern e_clear_button_shield_behavior clear_button_shield;
    extern uint16_t megafluxers_tactical_bonus;
    extern uint16_t megafluxers_strategic_bonus;
    extern uint8_t augmented_engines_speed_bonus;
    extern bool battle_pods_speed_bonus;
    extern uint8_t minimum_combat_speed;
    extern bool rebuild_ship_designs_after_field[SHIP_DESIGN_REBUILD_FIELD_COUNT];

    inline int32_t Apply_Megafluxers_Tactical_Bonus(int32_t space) {
        return (int32_t)((int64_t)space * (100 + megafluxers_tactical_bonus) / 100);
    }
    uint16_t Miniaturization_Cost_Multiplier(int16_t level);
    uint16_t Miniaturization_Space_Multiplier(int16_t level, int8_t category);
    int16_t Missile_Rack_Quantity(int16_t rack_index);
    int16_t Missile_Rack_Index(int16_t quantity);
    int16_t Missile_Rack_Size(int16_t quantity);
    int16_t Missile_Rack_Cost(int16_t quantity);
    bool Should_Autodesign_Use_X3_Rack(
        int16_t quantity, int16_t fitting_weapon_count, int16_t minimum_weapon_count);
} // namespace DESIGN