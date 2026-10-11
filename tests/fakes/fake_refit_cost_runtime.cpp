#include "pch.h"

#include <cstring>

namespace {
    s_player players[MAX_PLAYERS];
    s_colony colonies[1];
    s_planet_data planets[1];
    s_ai_colony_info ai_colonies[MAX_COLONIES];
    s_ai_star_info ai_stars[MAX_STARS];
    int8_t enroute_human_ships[MAX_STARS][10];
} // namespace

namespace TEST_AIBUILD_CONTEXT {
    void Reset_() {
        memset(players, 0, sizeof(players));
        memset(colonies, 0, sizeof(colonies));
        memset(planets, 0, sizeof(planets));
        memset(ai_colonies, 0, sizeof(ai_colonies));
        memset(ai_stars, 0, sizeof(ai_stars));
        memset(enroute_human_ships, 0, sizeof(enroute_human_ships));
        MOX::NUM_PLAYERS = 1;
        colonies[0].owner = 0;
        colonies[0].planet = 0;
        colonies[0].n_pops = 4;
        colonies[0].food2_needed_for_our_empire = 8;
        planets[0].star_index = 0;
        ai_colonies[0].pop_limit = 12;
        ai_colonies[0].food_buildings_applicable = 1;
    }
} // namespace TEST_AIBUILD_CONTEXT

namespace TECHDATA {
    s_tech_hull_data hull_data[HULL_CLASS_COUNT];
    s_tech_drive_data drives[DRIVE_COUNT];
    s_tech_specials_data specials[SPECIAL_COUNT];
}

namespace MOX {
    s_player* player = players;
    s_colony* colony = colonies;
    s_planet_data* planet = planets;
    s_current_design* design;
    int32_t stardate;
    int16_t NUM_PLAYERS;
} // namespace MOX

namespace AIDATA {
    s_ai_colony_info (*ai_colony_info)[MAX_COLONIES] = &ai_colonies;
    s_ai_star_info (*ai_star_info)[MAX_STARS] = &ai_stars;
    int8_t (*enroute_human_ships)[MAX_STARS][10] = &::enroute_human_ships;
} // namespace AIDATA

namespace struct_ {
    void __cdecl Set_Bit_Field(void* bit_field, uint16_t bit_index) {
        uint8_t* bytes = static_cast<uint8_t*>(bit_field);
        bytes[bit_index / 8] |= static_cast<uint8_t>(1u << (bit_index % 8));
    }
}