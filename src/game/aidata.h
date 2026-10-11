#pragma once

#include <cstddef>

namespace AIDATA {
    struct s_uncolonized_planet_food_worth {
        uint16_t biological;
        uint16_t cybernetic;
    };

    extern s_uncolonized_planet_food_worth uncolonized_planet_food_worth;

    struct s_uncolonized_planet_special_worth {
        uint32_t gold_deposits;
        uint32_t gem_deposits;
        uint32_t ancient_artifacts;
        uint32_t orion;
    };

    extern s_uncolonized_planet_special_worth uncolonized_planet_special_worth;

    struct s_turns_from_to_intermediate_cache_entry {
        uint8_t turns;
        int16_t intermediate_star;
        uint8_t extended_turns;
        int16_t extended_intermediate_star;
        int8_t computed;
    };

    struct s_ship_n_turns_to_intermediate_cache_entry {
        uint8_t turns;
        int16_t intermediate_star;
        int8_t computed;
    };

    extern uint16_t g_base_planet_values[MAX_PLAYERS][MAX_PLANETS];
    extern uint16_t (*ship_strengths)[MAX_SHIPS][MAX_PLAYERS];
    extern uint16_t (*ship_strengths_in_nebula)[MAX_SHIPS][MAX_PLAYERS];
    extern uint16_t (*ship_bomb_strengths)[MAX_SHIPS][4];
    extern int16_t (*planet_stage_weights)[MAX_PLANETS][MAX_PLAYERS];
    extern uint32_t (*base_star_stage_weights)[MAX_STARS][MAX_PLAYERS];
    extern uint32_t (*star_stage_weights)[MAX_STARS][MAX_PLAYERS];
    extern uint8_t* n_turns_from_to;
    extern s_turns_from_to_intermediate_cache_entry* n_turns_from_to_intermediate;
    extern int8_t* ship_n_turns_to;
    extern s_ship_n_turns_to_intermediate_cache_entry* ship_n_turns_to_intermediate;
    extern size_t route_star_count;
    extern size_t route_player_count;
    extern size_t route_ship_count;
    extern int32_t (*possible_ship_strengths)[MAX_STARS][MAX_PLAYERS][MAX_PLAYERS][10];
    extern s_ai_colony_info (*ai_colony_info)[MAX_COLONIES];
    extern s_ai_player_info (*ai_player_info)[MAX_PLAYERS];
    extern s_ai_star_info (*ai_star_info)[MAX_STARS];
    extern uint8_t (*star_in_range_of_player)[MAX_STARS];
    extern uint8_t (*star_in_extended_range_of_player)[MAX_STARS];
    extern int32_t (*enroute_ship_strengths)[MAX_STARS][MAX_PLAYERS][MAX_PLAYERS][10];
    extern int8_t (*enroute_ships)[MAX_STARS][MAX_PLAYERS][10];
    extern int8_t (*enroute_human_ships)[MAX_STARS][10];
    extern int8_t (*enroute_nonstealthy_ships)[MAX_STARS][MAX_PLAYERS];
    extern int16_t (*all_ships)[MAX_SHIPS];
    extern int16_t (*i_all_combat_ships)[MAX_PLAYERS][MAX_STARS + 1];
    extern s_ai_tech_info* aitech;
    extern s_ai_refit_player (*airefit)[MAX_PLAYERS];
    extern int32_t (*ship_refit_desirabilities)[MAX_SHIPS];
    extern uint16_t (*base_planet_values)[MAX_PLAYERS][MAX_PLANETS];
    extern uint16_t g_contextual_planet_values[MAX_PLAYERS * MAX_PLANETS];
    extern uint16_t (*contextual_planet_values)[MAX_PLAYERS * MAX_PLANETS];
    extern uint16_t n_all_ships;

    inline size_t Route_Index(size_t source_star, size_t dest_star, size_t player) {
        return (source_star * route_star_count + dest_star) * route_player_count + player;
    }

    inline size_t Ship_Route_Index(size_t ship, size_t dest_star) {
        return ship * route_star_count + dest_star;
    }

    inline uint8_t& N_Turns_From_To(size_t source_star, size_t dest_star, size_t player) {
        return n_turns_from_to[Route_Index(source_star, dest_star, player)];
    }

    inline s_turns_from_to_intermediate_cache_entry& N_Turns_From_To_Intermediate(
        size_t source_star, size_t dest_star, size_t player) {
        return n_turns_from_to_intermediate[Route_Index(source_star, dest_star, player)];
    }

    inline int8_t& Ship_N_Turns_To(size_t ship, size_t dest_star) {
        return ship_n_turns_to[Ship_Route_Index(ship, dest_star)];
    }

    inline s_ship_n_turns_to_intermediate_cache_entry& Ship_N_Turns_To_Intermediate(
        size_t ship, size_t dest_star) {
        return ship_n_turns_to_intermediate[Ship_Route_Index(ship, dest_star)];
    }

    inline size_t Route_Entry_Count() {
        return route_star_count * route_star_count * route_player_count;
    }

    inline size_t Ship_Route_Entry_Count() {
        return route_ship_count * route_star_count;
    }

    /*
     * Returns the player with the strongest population claim on a colony.
     * @param colony Colony to inspect.
     */
    int16_t __cdecl Colony_Dominant_Player(s_colony* colony);

    /*
     * Updates per-star AI range summaries for each player.
     */
    void __cdecl Compute_Star_Player_Range_Info();

    /*
     * Estimates how valuable a colony is to a specific player.
     * @param colony Colony being evaluated.
     * @param evaluating_player Player perspective used for the estimate.
     */
    int16_t __cdecl Colony_Worth_To_Player(s_colony* colony, s_player* evaluating_player);

    /*
     * Estimates the strategic range value of a colony to a specific player.
     * @param colony Colony being evaluated.
     * @param evaluating_player Player perspective used for the estimate.
     */
    int16_t __cdecl Proximity_Worth_To_Player(s_colony* colony, s_player* evaluating_player);

    /*
     * Computes the base per-player value table for all known planets.
     */
    void __cdecl Compute_Base_Planet_Values();

    /*
     * Computes per-player planet values adjusted by same-system context.
     */
    void __cdecl Compute_Contextual_Planet_Values();

    /*
     * Scores how worthwhile it is to refit a ship against current tech.
     * @param ship Ship to evaluate.
     */
    int32_t __cdecl Refit_Desirability(s_ship_data* ship);

    /*
     * Returns cached travel turns between two stars and an optional intermediate stop.
     * @param player_idx Player whose travel rules are used.
     * @param source_star_idx Starting star.
     * @param dest_star_idx Destination star.
     * @param intermediate_star_idx_out Receives the chosen intermediate star, or -1.
     * @param use_extended_range Whether to use the extended-range result.
     */
    int16_t __cdecl Player_N_Turns_From_Star_To_Star_With_Intermediate(
        int16_t player_idx, int16_t source_star_idx, int16_t dest_star_idx, int16_t* intermediate_star_idx_out, bool use_extended_range);

    /*
     * Returns cached travel turns from a ship to a destination star and an optional stop.
     * @param ship_idx Ship to evaluate.
     * @param dest_star_idx Destination star.
     * @param intermediate_star_idx_out Receives the chosen intermediate star, or -1.
     */
    int16_t __cdecl Ship_N_Turns_To_Star_With_Intermediate(int16_t ship_idx, int16_t dest_star_idx, int16_t* intermediate_star_idx_out);

    /*
     * Compares refit entries by desirability for qsort.
     * @param lhs Left entry.
     * @param rhs Right entry.
     */
    int __cdecl airefit_cmp(const void* lhs, const void* rhs);

    /*
     * Orders combat ships by owner and map position for qsort.
     * @param lhs Left ship index.
     * @param rhs Right ship index.
     */
    int __cdecl sort_ships(const void* lhs, const void* rhs);

    /*
     * Completes the AI turn and retains it when the debugger is enabled.
     */
    void __cdecl Deallocate_AI_Data();

    /*
     * Checks whether a target star is reachable from a source star or its wormhole exit.
     * @param target_star_idx Star being tested.
     * @param source_star_idx Source star.
     * @param max_range_parsecs Maximum allowed distance in parsecs.
     */
    bool __cdecl Star_In_Range_Of_Given_Star(int16_t target_star_idx, int16_t source_star_idx, int32_t max_range_parsecs);

    /*
     * Builds AI caches for ships, colonies, stars, and per-player planning data.
     */
    void __cdecl Compute_AI_Data();

    /*
     * Marks which players can reach a star within normal and extended range.
     * @param star_idx Star being tested.
     * @param in_normal_range_out Receives players within normal range.
     * @param in_extended_range_out Receives players within extended range.
     */
    void __cdecl Find_Players_In_Range(int16_t star_idx, int8_t* in_normal_range_out, int8_t* in_extended_range_out);

    /*
     * Estimates how valuable settling an uncolonized planet is to a player.
     * @param planet Planet being evaluated.
     * @param evaluating_player Player perspective used for the estimate.
     */
    int32_t __cdecl Uncolonized_Planet_Worth_To_Player(s_planet_data* planet, s_player* evaluating_player);
} // namespace AIDATA