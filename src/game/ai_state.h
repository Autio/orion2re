#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ai_state {
    struct s_colony_decision {
        int16_t colony_idx;
        uint8_t n_farmers;
        uint8_t n_workers;
        uint8_t first_assignable_pop_idx;
        uint8_t pop_end_idx;
        uint8_t pollution_at_start;
        int16_t best_worker_to_farmer_score;
        int16_t worst_worker_to_farmer_score;
        bool blockaded;
        bool eats_minerals;
    };

    struct s_movement_decision {
        int16_t player_idx = -1;
        std::vector<s_ai_colonizable_planet> colonizable_planets;
        std::vector<int8_t> flee_map;
        std::vector<int8_t> nonplayer_map;
        std::vector<int16_t> flee_stars;
        std::vector<int16_t> nonplayer_stars;
    };

    struct s_player_decisions {
        bool colony_phase_ran = false;
        bool movement_phase_ran = false;
        std::vector<s_aidudes_colony_info> colony_state;
        std::vector<s_colony_decision> colonies;
        s_movement_decision movement;
    };

    struct s_snapshot {
        int32_t stardate = 0;
        int16_t player_count = 0;
        int16_t star_count = 0;
        int16_t planet_count = 0;
        int16_t colony_count = 0;
        int16_t ship_count = 0;

        uint16_t (*ship_strengths)[MAX_SHIPS][MAX_PLAYERS] = nullptr;
        uint16_t (*ship_strengths_in_nebula)[MAX_SHIPS][MAX_PLAYERS] = nullptr;
        uint16_t (*ship_bomb_strengths)[MAX_SHIPS][4] = nullptr;
        int16_t (*planet_stage_weights)[MAX_PLANETS][MAX_PLAYERS] = nullptr;
        uint32_t (*base_star_stage_weights)[MAX_STARS][MAX_PLAYERS] = nullptr;
        uint32_t (*star_stage_weights)[MAX_STARS][MAX_PLAYERS] = nullptr;
        uint8_t* n_turns_from_to = nullptr;
        AIDATA::s_turns_from_to_intermediate_cache_entry* n_turns_from_to_intermediate = nullptr;
        int8_t* ship_n_turns_to = nullptr;
        AIDATA::s_ship_n_turns_to_intermediate_cache_entry* ship_n_turns_to_intermediate = nullptr;
        int32_t (*possible_ship_strengths)[MAX_STARS][MAX_PLAYERS][MAX_PLAYERS][10] = nullptr;
        s_ai_colony_info (*ai_colony_info)[MAX_COLONIES] = nullptr;
        s_ai_player_info (*ai_player_info)[MAX_PLAYERS] = nullptr;
        s_ai_star_info (*ai_star_info)[MAX_STARS] = nullptr;
        uint8_t (*star_in_range_of_player)[MAX_STARS] = nullptr;
        uint8_t (*star_in_extended_range_of_player)[MAX_STARS] = nullptr;
        int32_t (*enroute_ship_strengths)[MAX_STARS][MAX_PLAYERS][MAX_PLAYERS][10] = nullptr;
        int8_t (*enroute_ships)[MAX_STARS][MAX_PLAYERS][10] = nullptr;
        int8_t (*enroute_human_ships)[MAX_STARS][10] = nullptr;
        int8_t (*enroute_nonstealthy_ships)[MAX_STARS][MAX_PLAYERS] = nullptr;
        int16_t (*all_ships)[MAX_SHIPS] = nullptr;
        int16_t (*i_all_combat_ships)[MAX_PLAYERS][MAX_STARS + 1] = nullptr;
        s_ai_tech_info* aitech = nullptr;
        s_ai_refit_player (*airefit)[MAX_PLAYERS] = nullptr;
        int32_t (*ship_refit_desirabilities)[MAX_SHIPS] = nullptr;
        uint16_t (*base_planet_values)[MAX_PLAYERS][MAX_PLANETS] = nullptr;
        uint16_t (*contextual_planet_values)[MAX_PLAYERS * MAX_PLANETS] = nullptr;
        uint16_t n_all_ships = 0;

        s_player_ship_sizes player_ship_sizes[MAX_PLAYERS] = {};
        int32_t player_total_strength_vs_colonies[MAX_PLAYERS] = {};
        uint8_t player_n_transports[MAX_PLAYERS][2] = {};
        int8_t player_n_freighter_groups_wanted[MAX_PLAYERS] = {};
        uint8_t player_n_spies[MAX_PLAYERS][2] = {};
        uint8_t player_n_colony_ships[MAX_PLAYERS][3] = {};
        int32_t player_total_strength_vs_ships[MAX_PLAYERS][3] = {};
        s_player_decisions players[MAX_PLAYERS];

        size_t allocated_bytes = 0;

        size_t Route_Index(size_t source_star, size_t dest_star, size_t player) const {
            return (source_star * star_count + dest_star) * player_count + player;
        }

        size_t Ship_Route_Index(size_t ship, size_t dest_star) const {
            return ship * star_count + dest_star;
        }
    };

    void Begin_Turn();
    void Complete_Turn();
    void Shutdown();
    void Capture_Build_State();
    s_aidudes_colony_info* Prepare_Colony_State(int16_t player_idx);
    void Capture_Colony_State(int16_t player_idx,
        const s_aidudes_colony_info* colonies, int16_t colony_count,
        int16_t blockaded_count);
    void Prepare_Movement_State(int16_t player_idx,
        s_ai_colonizable_planet** planets, int8_t** flee_stars,
        int8_t** nonplayer_stars);
    void Capture_Movement_State(int16_t player_idx,
        const s_ai_colonizable_planet* planets, int16_t planet_count,
        const int8_t* flee_stars, const int8_t* nonplayer_stars);

    // Returned snapshots remain valid until the next Begin_Turn() or Shutdown().
    const s_snapshot* Get_Active();
    const s_snapshot* Get_Completed();
    bool Is_Valid(const s_snapshot* snapshot);
} // namespace ai_state