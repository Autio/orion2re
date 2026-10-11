#include "pch.h"

namespace MOX {
    int16_t NUM_PLAYERS;
    int16_t NUM_STARS;
    int16_t planet_count;
    int16_t NUM_COLONIES;
    int16_t NUM_SHIPS;
    int32_t stardate;
}

namespace AIDATA {
    uint16_t g_base_planet_values[MAX_PLAYERS][MAX_PLANETS];
    uint16_t (*ship_strengths)[MAX_SHIPS][MAX_PLAYERS];
    uint16_t (*ship_strengths_in_nebula)[MAX_SHIPS][MAX_PLAYERS];
    uint16_t (*ship_bomb_strengths)[MAX_SHIPS][4];
    int16_t (*planet_stage_weights)[MAX_PLANETS][MAX_PLAYERS];
    uint32_t (*base_star_stage_weights)[MAX_STARS][MAX_PLAYERS];
    uint32_t (*star_stage_weights)[MAX_STARS][MAX_PLAYERS];
    uint8_t* n_turns_from_to;
    s_turns_from_to_intermediate_cache_entry* n_turns_from_to_intermediate;
    int8_t* ship_n_turns_to;
    s_ship_n_turns_to_intermediate_cache_entry* ship_n_turns_to_intermediate;
    size_t route_star_count;
    size_t route_player_count;
    size_t route_ship_count;
    int32_t (*possible_ship_strengths)[MAX_STARS][MAX_PLAYERS][MAX_PLAYERS][10];
    s_ai_colony_info (*ai_colony_info)[MAX_COLONIES];
    s_ai_player_info (*ai_player_info)[MAX_PLAYERS];
    s_ai_star_info (*ai_star_info)[MAX_STARS];
    uint8_t (*star_in_range_of_player)[MAX_STARS];
    uint8_t (*star_in_extended_range_of_player)[MAX_STARS];
    int32_t (*enroute_ship_strengths)[MAX_STARS][MAX_PLAYERS][MAX_PLAYERS][10];
    int8_t (*enroute_ships)[MAX_STARS][MAX_PLAYERS][10];
    int8_t (*enroute_human_ships)[MAX_STARS][10];
    int8_t (*enroute_nonstealthy_ships)[MAX_STARS][MAX_PLAYERS];
    int16_t (*all_ships)[MAX_SHIPS];
    int16_t (*i_all_combat_ships)[MAX_PLAYERS][MAX_STARS + 1];
    s_ai_tech_info* aitech;
    s_ai_refit_player (*airefit)[MAX_PLAYERS];
    int32_t (*ship_refit_desirabilities)[MAX_SHIPS];
    uint16_t (*base_planet_values)[MAX_PLAYERS][MAX_PLANETS] = &g_base_planet_values;
    uint16_t g_contextual_planet_values[MAX_PLAYERS * MAX_PLANETS];
    uint16_t (*contextual_planet_values)[MAX_PLAYERS * MAX_PLANETS] =
        &g_contextual_planet_values;
    uint16_t n_all_ships;
}

namespace debug_overlay {
    uint8_t g_enabled;
}

namespace AIBUILD {
    s_player_ship_sizes player_ship_sizes[MAX_PLAYERS];
    int32_t player_total_strength_vs_colonies[MAX_PLAYERS];
    uint8_t player_n_transports[MAX_PLAYERS][2];
    int8_t player_n_freighter_groups_wanted[MAX_PLAYERS];
    uint8_t player_n_spies[MAX_PLAYERS][2];
    uint8_t player_n_colony_ships[MAX_PLAYERS][3];
    int32_t player_total_strength_vs_ships[MAX_PLAYERS][3];
}

namespace allocate {
    void __cdecl Allocation_Error(int32_t requested_bytes) {
        fprintf(stderr, "unexpected AI state allocation failure: %d\n", requested_bytes);
        abort();
    }
}

namespace TEST_AI_STATE {
    void Reset_() {
        ai_state::Shutdown();
        MOX::NUM_PLAYERS = 4;
        MOX::NUM_STARS = 72;
        MOX::planet_count = 360;
        MOX::NUM_COLONIES = 100;
        MOX::NUM_SHIPS = 500;
        MOX::stardate = 35010;
        debug_overlay::g_enabled = 1;
    }
}