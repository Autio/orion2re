#include "pch.h"

namespace {
    s_colony colonies[3];
    s_player players[MAX_PLAYERS];
    s_ship_data ships[2];
    s_leader_data leaders[MAX_LEADERS];
    s_planet_data planets[1];
    s_star_data stars[1];
}

namespace TEST_COLONY_REPLICATORS {
    void Reset_() {
        memset(colonies, 0, sizeof(colonies));
        memset(TECHDATA::buildings, 0, sizeof(TECHDATA::buildings));
        TECHDATA::buildings[BUILDING_AUTOMATED_FACTORY].maintenance = 2;
        COLCALC::replicator_food_pp = 2;
        COLCALC::replicator_food_bc = 1;
        COLCALC::trade_goods_ratio = {50, 100};
        COLCALC::planet_bc_bonus = {5, 10};
        const int8_t maintenance[PLANET_CLIMATE_COUNT] = {
            50, 25, 0, 25, 0, 0, 0, 0, 0, 0,
        };
        memcpy(COLCALC::climate_maintenance_modifiers, maintenance,
               sizeof(maintenance));
    }
}

namespace TEST_GOVERNMENT_BONUS {
    void Reset_() {
        memset(players, 0, sizeof(players));
        memset(colonies, 0, sizeof(colonies));
        memset(planets, 0, sizeof(planets));
        memset(stars, 0, sizeof(stars));
        for (int player = 0; player < MAX_PLAYERS; ++player) {
            stars[0].officer_index[player] = -1;
        }
        colonies[0].owner = 0;
        colonies[0].planet = 0;
        planets[0].star_index = 0;
        planets[0].gravity_class = PLANET_GRAVITY_NORMAL_G;
        planets[0].food_per_farmer = 2;
        planets[0].mineral_class = 0;
        COLCALC::minerals_per_mine[0] = 1;
        players[0].objectives = PLAYER_OBJECTIVE_HUMAN;
        COLCALC::android_morale = COLCALC::ANDROID_MORALE_WORKERS_ONLY;
        COLCALC::research_colony_bonus = {5, 10, 15, 30};
        COLCALC::research_scientist_bonus = {3, 2, 5, 1, 2, 3, 1, 3, 1};
        COLCALC::planet_bc_bonus = {5, 10};
        const int8_t maintenance[PLANET_CLIMATE_COUNT] = {
            50, 25, 0, 25, 0, 0, 0, 0, 0, 0,
        };
        memcpy(COLCALC::climate_maintenance_modifiers, maintenance,
               sizeof(maintenance));
        MOX::NUM_PLAYERS = 1;
        MOX::NUM_COLONIES = 1;
    }

    void Cleanup_() {
        memset(players, 0, sizeof(players));
        memset(colonies, 0, sizeof(colonies));
        memset(planets, 0, sizeof(planets));
        memset(stars, 0, sizeof(stars));
        COLCALC::minerals_per_mine[0] = 0;
        MOX::NUM_PLAYERS = 0;
        MOX::NUM_COLONIES = 0;
    }
}

namespace TEST_PLAYER_STATS {
    void Reset_() {
        memset(players, 0, sizeof(players));
        memset(ships, 0, sizeof(ships));
        memset(leaders, 0, sizeof(leaders));
        memset(planets, 0, sizeof(planets));
        memset(stars, 0, sizeof(stars));
        memset(&MOX::settings, 0, sizeof(MOX::settings));
        for (s_leader_data& leader : leaders) {
            leader.status = -1;
        }
        MOX::NUM_PLAYERS = 1;
        MOX::NUM_COLONIES = 0;
        MOX::NUM_SHIPS = 1;
        const uint8_t defaults[SHIP_SIZE_COUNT] = {1, 2, 4, 8, 16, 32};
        memcpy(COLCALC::scout_lab_research_bonus, defaults, sizeof(defaults));
        COLCALC::command_points = {5, 1, 2, 3, 1, 2, 3, 2};
        const uint8_t command_usage[SHIP_SIZE_COUNT] = {1, 2, 3, 4, 5, 6};
        memcpy(COLCALC::ship_command_points_usage, command_usage,
               sizeof(command_usage));
    }
}

namespace MOX {
    s_colony* colony = colonies;
    s_player* player = players;
    s_ship_data* ship = ships;
    s_leader_data* leaders = ::leaders;
    s_planet_data* planet = planets;
    s_star_data* star = stars;
    s_settings settings = {};
    int16_t NUM_PLAYERS = 0;
    int16_t NUM_COLONIES = 0;
    int16_t NUM_SHIPS = 0;
    uint8_t food_per_farmer_table[PLANET_CLIMATE_COUNT] = {};
}

namespace TECHDATA {
    s_tech_building_data buildings[BUILDING_COUNT];
    s_tech_hull_data hull_data[HULL_CLASS_COUNT];
    s_tech_app_data technology_applications[TECH_APP_COUNT];
    s_tech_field_data technology_fields[TECH_FIELD_COUNT];
}

namespace EVENTS {
    int16_t __cdecl Event_Check_Space_Anomaly(int16_t) {
        return 0;
    }

    int16_t __cdecl Event_Check_Space_Anomaly_At_Star(int16_t) {
        return 0;
    }

    int16_t __cdecl Event_Check_Colony_Researching(int16_t) {
        return 0;
    }
}

namespace struct_ {
    int16_t __cdecl Test_Bit_Field(void* bit_field, uint16_t bit_index) {
        uint8_t* bytes = (uint8_t*)bit_field;
        return (bytes[bit_index >> 3] >> (bit_index & 7)) & 1;
    }
}

namespace OFFICER {
    int16_t __cdecl Owned_Officer_Level(int16_t) {
        return 0;
    }

    int16_t __cdecl Officer_Maintenance(int16_t, int16_t) {
        return 0;
    }
}

namespace BILL {
    int16_t __cdecl Get_Their_Spy_Number(int16_t, int16_t) {
        return 0;
    }
}