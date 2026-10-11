#pragma once

namespace AIMOVE {
    inline constexpr int ENEMY_STRENGTH_TURN_WINDOW = 10;

#pragma pack(push, 1)
    struct s_ai_stage_point {
        int16_t star_idx;
        uint32_t strength;
    };

    struct s_attack_info {
        int16_t target_player;
        int16_t min_value;
        int16_t target_planet;
        int16_t value_a;
        int16_t value_b;
    };
#pragma pack(pop)

    extern s_player* p_ai;
    extern s_ai_colonizable_planet* colonizable;
    extern int8_t (*nonplayer_here)[MAX_STARS];
    extern int8_t* flee_stars;
    extern int16_t i_ai;
    extern int16_t n_colonizable;
    extern uint8_t eel_spawn_frequency;
    extern uint8_t max_eels;

    /**
     * Runs the full movement phase for one AI empire.
     * @param player_idx AI player to process.
     */
    void __cdecl Move_AI(int16_t player_idx);

    int __cdecl Sort_colonizable(const void* a, const void* b);

    /**
     * Removes colonizable planets that already have an assigned colony ship or queued colony ship.
     * @param colonizable_planets In-place list of colonizable planets.
     * @param num_planets In/out count for the colonizable list.
     */
    void __cdecl Delete_Already_Assigned_Planets(s_ai_colonizable_planet* colonizable_planets, int16_t* num_planets);

    /**
     * Launches the AI's prepared sneak attack if the target is still valid.
     */
    void __cdecl Launch_Sneak_Attack();

    /**
     * Launches attacks of the requested mission type from all eligible staging stars.
     * @param mission_type Mission code to assign to launched fleets.
     * @param min_pop Minimum remaining population threshold used by attack scoring.
     */
    void __cdecl Launch_Attacks(int mission_type, int min_pop);

    /**
     * Evaluates the best attack target reachable from a staging star.
     * @param star_idx Source staging star.
     * @param target_info Optional output filter and detail structure.
     * @param mission_type Mission code to search for.
     * @param min_pop Minimum remaining population threshold used by attack scoring.
     */
    void __cdecl Find_Best_Attack_From_Star(int16_t star_idx, s_attack_info* target_info, int mission_type, int min_pop);

    int8_t __cdecl AI_May_Attack_Player(int16_t player_idx, int16_t target_idx);

    uint16_t __cdecl Compute_Attack_Worthiness(
        int16_t colony_idx,
        int16_t* ship_list,
        uint8_t eta,
        int8_t* out_attack_type,
        uint8_t* out_flee,
        s_attack_info* target_info,
        int32_t min_pop);

    int8_t __cdecl Player_Is_Hostile_To_Player(int player_idx_a, int player_idx_b);

    int32_t __cdecl N_Enemy_Stars_Reachable(int16_t player_idx, int16_t player_bit, int16_t source_star_idx);

    void __cdecl Intercept_Invaders();

    void __cdecl Interceptions(s_aimove_intercept_entry* intercepts, int* count, int16_t* star_to_intercept_idx, int is_desperate);

    void __cdecl Assign_New_Colony_Ship_Destinations(s_ai_colonizable_planet* planets, int16_t* count);

    bool __cdecl Star_Has_Monster(int16_t star_idx);

    void __cdecl Get_Best_Unescorted_Colony_Ship(int16_t* ship_indices, int16_t* ship_count, int16_t destination);

    int16_t __cdecl I_Soonest_Enemy_Colony_Ship(int16_t* ship_indices, int16_t count, int16_t target_location);

    int8_t __cdecl Get_Best_Escorted_Colony_Ship(int16_t* colony_ships, int16_t* num_colony_ships, int16_t star_idx, int16_t max_turns);

    void __cdecl Move_Transports();

    void __cdecl Move_To_Stage_Points();

    void __cdecl Move_To_Unexplored_Stars();

    void __cdecl Move_NPCs();

    int __cdecl Sort_npc_dests(const void* a, const void* b);

    int16_t __cdecl Npc_Dest(int16_t ship_idx, uint8_t* star_flags);

    void __cdecl Move_All_AI();

    /**
     * Marks stars the AI should avoid because of temporary blockade restrictions.
     * @param player_idx AI player whose flee map is being built.
     */
    void __cdecl Mark_Flee_Stars(int16_t player_idx);

    /**
     * Gathers all currently colonizable planets within the AI player's extended range.
     * @param colonizable_planets Output list of candidate planets.
     * @param count Out number of candidate planets.
     */
    void __cdecl Get_Colonizable_Planets(s_ai_colonizable_planet* colonizable_planets, int16_t* count);

    int __cdecl Gather_Player_Ships_At_Star(int16_t player_id, int16_t star_id, int16_t* ships_list);

    void __cdecl Gather_Intercepting_Ships(int16_t* ships_list, int32_t* ship_count, int16_t* intercept_map);

    void __cdecl Knock_Out_Impossible_Interceptions(s_aimove_intercept_entry* intercepts, int* count, int16_t* temp_arr, int* temp_count, int keep_one_entry);

    uint16_t __cdecl Ship_Min_Strength_Versus_Players_At_Star(int16_t ship_idx, uint16_t player_mask, int16_t star_idx);

    void __cdecl Player_Threatens_Player(int plr1, int plr2, int16_t* star_idx, int* aux_info, int16_t* planet_idx);

    int16_t __cdecl Best_Uncolonized_Undestined_Planet(int16_t player_idx);

    uint32_t __cdecl Ship_Rough_Strength_At_Star(int16_t ship_idx, int16_t star_idx);

    void __cdecl Get_Enemy_Strength_On_Turn(int16_t star_id, int* turn_strengths, int* cumulative_strengths);

    int __cdecl Number_Of_Attacks_Underway(int16_t player_idx, int16_t target_idx);

    void __cdecl Find_Opportunity_Attack(int16_t player_idx, int16_t target_idx, int16_t threshold, int16_t* out_planet, int16_t* out_value, int32_t* out_ratio);

    /**
     * Finds a direct or intermediate waypoint movement order for the ship list.
     * @param ship_array Null-terminated ship index list.
     * @param dest_star Final destination star.
     * @param out_star Out chosen immediate destination star.
     * @param out_turns Out total travel time for the chosen route.
     */
    void __cdecl Move_Ships_With_Possible_Intermediate(int16_t* ship_array, int16_t dest_star, int16_t* out_star, int16_t* out_turns);

    int32_t __cdecl Enemy_Colony_Worth_To_Player(int16_t colony_idx, int16_t player_idx);

    /**
     * Looks for an opportunity attack from a staging star with a minimum score threshold.
     * @param player_id Attacking player.
     * @param target_player_idx Defending player to evaluate.
     * @param min_value Minimum acceptable attack value.
     * @param star_idx Source staging star.
     * @param out_target_star Out selected target planet.
     * @param out_value Out colony-worth value for the selected target.
     * @param out_ratio Out computed attack ratio/value.
     */
    void __cdecl Find_Opportunity_Attack_Aux(
        int16_t player_id,
        int16_t target_player_idx,
        int16_t min_value,
        int16_t star_idx,
        int16_t* out_target_star,
        int16_t* out_value,
        int32_t* out_ratio);

    int __cdecl Sort_candidate_stars(const void* candidate_star_a, const void* candidate_star_b);
} // namespace AIMOVE