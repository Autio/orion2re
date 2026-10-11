#pragma once
#include <cstdint>

typedef struct s_tech_app_data s_tech_app_data, *Ps_tech_app_data;
typedef struct s_tech_armor_data s_tech_armor_data, *Ps_tech_armor_data;
typedef struct s_tech_building_data s_tech_building_data, *Ps_tech_building_data;
typedef struct s_tech_computer_data s_tech_computer_data, *Ps_tech_computer_data;
typedef struct s_tech_drive_data s_tech_drive_data, *Ps_tech_drive_data;
typedef struct s_tech_field_data s_tech_field_data, *Ps_tech_field_data;
typedef struct s_tech_fuel_data s_tech_fuel_data, *Ps_tech_fuel_data;
typedef struct s_tech_hull_data s_tech_hull_data, *Ps_tech_hull_data;
typedef struct s_tech_shield_data s_tech_shield_data, *Ps_tech_shield_data;
typedef struct s_tech_specials_data s_tech_specials_data, *Ps_tech_specials_data;
typedef struct s_tech_weapon_data s_tech_weapon_data, *Ps_tech_weapon_data;
typedef struct s_tech_weapon_mod_data s_tech_weapon_mod_data, *Ps_tech_weapon_mod_data;
typedef struct s_unit_data s_unit_data, *Ps_unit_data;

struct s_tech_app_data {
    char* name;
    uint16_t tech_app_id;
    int16_t tech_field_id;
    uint8_t unk_class;
    uint8_t ai_group;
    uint8_t unknown1;
    uint8_t in_strategic;
    uint8_t type;
};

struct s_tech_armor_data {
    char* name;
    int16_t armor_id;
    int16_t tech_app_id;
    undefined field3_0x8;
    uint8_t troops_bonus;
    int16_t ships_bonus;
    int16_t cost_percent;
    uint8_t strategic_hp;
};

struct s_tech_building_data {
    char* name;
    uint16_t building_id;
    uint16_t tech_app;
    uint16_t cost;
    uint16_t unknown1;
    uint16_t maintenance;
    uint8_t group;
    uint32_t unknown2;
};

struct s_tech_computer_data {
    char* name;
    uint16_t computer_id;
    uint16_t tech_app_id;
    uint16_t cost_by_ship_type[6];
    int16_t bonus;
};

struct s_tech_drive_data {
    char* name;
    uint16_t drive_id;
    uint16_t tech_app_id;
    uint16_t space_per_ship_type[6];
    uint16_t cost_per_ship_type[6];
    uint8_t combat_speed_min[6];
    uint8_t combat_speed_max[6];
    uint16_t warp_speed;
};

struct s_tech_field_data {
    char* name;
    int16_t tech_field_id;
    int16_t prev_field_id;
    int16_t next_field_id;
    int16_t tech[4];
    uint32_t cost;
    uint8_t ai_group;
};

struct s_tech_fuel_data {
    char* name;
    uint16_t fuel_id;
    uint16_t tech_app_id;
    uint16_t ship_range;
};

struct s_tech_hull_data {
    uint16_t cost;
    uint16_t size;
    uint16_t marines;
    uint16_t armor_hp;
    uint16_t structure_hp;
    uint16_t computer_hp;
    uint16_t drive_hp;
    uint16_t shield_hp;
    uint16_t strat_beam;
    uint16_t strat_missile;
    uint16_t strat_special;
    uint16_t strat_bomb;
    int16_t strat_def_bonus;
    uint16_t strat_hits;
    char* name;
    char* size_name;
};

struct s_tech_shield_data {
    char* name;
    uint16_t shield_id;
    uint16_t tech_app_id;
    uint16_t space_per_ship_type[6];
    uint16_t cost_per_ship_type[6];
    uint16_t shield_class;
    undefined field6_0x22;
    undefined field7_0x23;
    undefined field8_0x24;
    undefined field9_0x25;
    undefined field10_0x26;
    undefined field11_0x27;
    undefined field12_0x28;
    undefined field13_0x29;
    undefined field14_0x2a;
    undefined field15_0x2b;
    undefined field16_0x2c;
    undefined field17_0x2d;
    undefined field18_0x2e;
    undefined field19_0x2f;
    undefined field20_0x30;
    undefined field21_0x31;
    undefined field22_0x32;
    undefined field23_0x33;
    undefined field24_0x34;
    undefined field25_0x35;
    undefined field26_0x36;
    undefined field27_0x37;
    undefined field28_0x38;
    undefined field29_0x39;
    uint8_t unk_strat_value;
};

struct s_tech_specials_data {
    char* name;
    uint16_t special_id;
    uint16_t tech_app_id;
    int16_t space_per_ship_type[6];
    int16_t cost_per_ship_type[6];
    int16_t exclude[6];
    uint8_t category;
    uint16_t strategic;
};

struct s_tech_weapon_data {
    char* name;
    char* name_plural;
    uint16_t weapon_id;
    uint16_t tech_app_id;
    uint8_t type;
    int8_t ammo;
    uint16_t size;
    uint16_t cost;
    uint16_t natural_mods;
    uint16_t min_damage;
    uint16_t max_damage;
    uint8_t strat_min_damage;
    uint8_t strat_max_damage;
    uint16_t available_mods;
};

struct s_tech_weapon_mod_data {
    char* name;
    char* name_plural;
    uint8_t mod_id;
    uint8_t level;
    int16_t cost;
    int16_t space;
    uint8_t col5;
};

struct s_unit_data {
    char* name;
    undefined field1_0x4;
    undefined field2_0x5;
    undefined field3_0x6;
    undefined field4_0x7;
    undefined field5_0x8;
    undefined field6_0x9;
    undefined field7_0xa;
    undefined field8_0xb;
};

struct s_planetary_shield_config {
    s_techdata_planetary_shield& radiation;
    s_techdata_planetary_shield& flux;
    s_techdata_planetary_shield& barrier;
};

struct s_starting_techfield {
    int16_t& biology;
    int16_t& power;
    int16_t& physics;
    int16_t& construction;
    int16_t& force_fields;
    int16_t& chemistry;
    int16_t& computers;
    int16_t& sociology;
    int16_t& xenon;
};

namespace TECHDATA {
    extern s_techdata_planetary_shield planetary_shields[4];
    extern s_planetary_shield_config planetary_shield;
    extern s_starting_techfield starting_techfield;
    extern s_tech_armor_data armor[ARMOR_COUNT];
    extern s_tech_building_data buildings[BUILDING_COUNT];
    extern s_tech_hull_data hull_data[HULL_CLASS_COUNT];
    extern uint16_t shield_hull_multiplier[HULL_CLASS_COUNT];
    extern uint16_t multi_phased_shields_bonus;
    extern uint16_t esd_bonus;
    extern uint8_t hard_shields_damage_reduction;
    extern s_tech_fuel_data fuel[FUEL_COUNT];
    extern s_tech_app_data technology_applications[TECH_APP_COUNT];
    extern s_tech_field_data technology_fields[TECH_FIELD_COUNT];
    extern s_tech_drive_data drives[DRIVE_COUNT];
    extern s_tech_shield_data shields[SHIELD_COUNT];
    extern s_tech_computer_data computers[COMPUTER_COUNT];
    extern s_tech_specials_data specials[SPECIAL_COUNT];
    extern s_tech_weapon_data weapons[WEAPON_COUNT];
    extern s_tech_weapon_mod_data weapon_modifications[WEAPON_MOD_COUNT];

    extern int16_t starting_tech_field_ids[6];

    extern s_unit_data units[UNIT_COUNT];

    extern s_tech_data_rifle rifles[6];

    extern s_tech_data_personal_shield personal_shields[2];

    int32_t Shield_Strength(int16_t hull_class, int16_t shield_type);
    int32_t Apply_Multi_Phased_Shields_Bonus(int32_t shield_strength);
    int32_t Extra_Structural_Damage(int32_t damage);

    /**
     * Returns a reference to the ship-range value for a fuel technology entry.
     * @param fuel_index Index into the TECHDATA fuel table.
     */
    inline uint16_t& _fuel_ship_range(int32_t fuel_index) {
        return fuel[fuel_index].ship_range;
    }
} // namespace TECHDATA