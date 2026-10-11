#pragma once

#include "production_rules.h"

namespace AIBUILD {
    constexpr int32_t AUTOBUILD_CANDIDATE_COUNT = BUILDING_COUNT + 2;
    constexpr int32_t STRATEGIC_ALLOCATION_CANDIDATE_COUNT = 6;
    constexpr int32_t EMERGENCY_DEFENSE_PRODUCT_COUNT = 8;

    struct s_autobuild_rules {
        production_rules::s_cutoff cutoff;
        production_rules::s_candidate candidates[AUTOBUILD_CANDIDATE_COUNT];
    };

    struct s_ordered_production_policy {
        bool enabled;
        int16_t order[EMERGENCY_DEFENSE_PRODUCT_COUNT];
    };

    struct s_economic_production_policy {
        bool enabled;
        int16_t product;
    };

    struct s_expansion_production_policy {
        bool enabled;
        int16_t colony_base_minimum_industry;
        int16_t colony_ship_minimum_industry;
    };

    struct s_strategic_allocation {
        production_rules::s_cutoff cutoff;
        production_rules::s_candidate candidates[STRATEGIC_ALLOCATION_CANDIDATE_COUNT];
    };

    struct s_strategic_production {
        s_ordered_production_policy emergency_defense;
        s_economic_production_policy economic_emergency;
        s_expansion_production_policy expansion;
        s_strategic_allocation allocation;
    };

    extern s_autobuild_rules autobuild_rules;
    extern s_strategic_production strategic_production;
    extern uint8_t difficulty_ship_design[12];
    extern int8_t difficulty_intel_bias[8];
    extern uint8_t player_ship_size_table[9];
    extern s_player_ship_sizes player_ship_sizes[MAX_PLAYERS];
    extern int32_t player_total_strength_vs_colonies[MAX_PLAYERS];
    extern uint8_t player_n_transports[MAX_PLAYERS][2];
    extern int8_t player_n_freighter_groups_wanted[MAX_PLAYERS];
    extern uint8_t player_n_spies[MAX_PLAYERS][2];
    extern uint8_t player_n_colony_ships[MAX_PLAYERS][3];
    extern int32_t player_total_strength_vs_ships[MAX_PLAYERS][3];

    /** Returns the refit cost for replacing one ship design with another.
     * @param player_idx Player paying for the refit.
     * @param old_design Existing ship design.
     * @param new_design Replacement ship design.
     */
    int16_t __cdecl Refit_Cost(int16_t player_idx, struct s_ship_design* old_design, struct s_ship_design* new_design);

    /** Returns an average strength estimate for a ship against relevant opponents.
     * @param ship Ship whose combat strength is being evaluated.
     */
    int16_t __cdecl Mean_Ship_Strength(struct s_ship_data* ship);

    /** Computes build-related needs for one AI player.
     * @param player_idx Player index to evaluate.
     */
    void __cdecl Compute_Needs(int16_t player_idx);

    /** Recomputes empire-wide build needs for all AI players. */
    void __cdecl Compute_Empire_Building_Needs();

    /** Returns the highest hostile treaty state currently affecting a player.
     * @param player_idx Player index to inspect.
     */
    int16_t __cdecl Player_War_Level(int16_t player_idx);

    /** Computes whether a player currently needs more colony ships.
     * @param player_idx Player index to evaluate.
     */
    void __cdecl Compute_Colony_Ship_Need(int16_t player_idx);

    /** Computes how many freighter groups a player still needs.
     * @param player_idx Player index to evaluate.
     */
    void __cdecl Compute_Freighter_Need(int16_t player_idx);

    /** Buys out eligible colony production for one player while funds allow.
     * @param player_idx Player index whose colonies may buy production outright.
     */
    void __cdecl Empire_Buy_Outright(int16_t player_idx);

    /** Marks which buildings a colony is currently allowed to construct.
     * @param colony Colony to inspect.
     * @param buildable_flags Output flags indexed by product id.
     */
    void __cdecl Find_Buildable_Buildings(struct s_colony* colony, uint16_t* buildable_flags);

    /** Scores one building choice for a colony's AI production logic.
     * @param colony Colony being evaluated.
     * @param building_id Building or product id to score.
     */
    int16_t __cdecl Colony_Building_Score(struct s_colony* colony, int16_t building_id);

    /** Builds normalized deterministic production facts for one colony.
     * @param colony Colony to inspect.
     * @param war_level Effective strategic war level, or minus one to calculate it.
     * @param context Output production context.
     */
    void Build_Production_Context(const struct s_colony* colony, int32_t war_level, production_rules::s_context* context);

    /** Reassigns colony production when the current build should be replaced.
     * @param colony Colony whose production is being checked.
     */
    void __cdecl Assign_Colony_Building(struct s_colony* colony);

    /** Chooses a new production target for a colony.
     * @param colony Colony whose production is being assigned.
     */
    void __cdecl Assign_Colony_New_Building(struct s_colony* colony);

    /** Returns whether a colony contains foreign population from a treaty partner.
     * @param colony Colony to inspect.
     */
    bool __cdecl Colony_Is_Conquered(struct s_colony* colony);

    /** Forces one listed colony to switch to a specific production item.
     * @param id_list List of colony indexes to update.
     * @param list_index Index within id_list to process.
     * @param list_count Number of active ids in the list.
     * @param production_id Product id to assign.
     */
    void __cdecl Force_Produce(int16_t* id_list, int16_t list_index, int32_t* list_count, int16_t production_id);

    /** Returns a simple industry value estimate for a colony.
     * @param colony Colony to evaluate.
     */
    int16_t __cdecl Industry_Worth(struct s_colony* colony);

    /** Returns the display name for a colony production item.
     * @param product_id Building or project id to name.
     */
    char* __cdecl Product_Name(int16_t product_id);

    /** Assigns a fresh autobuild target without recalculating current-turn production.
     * @param colony Colony to update.
     */
    void __cdecl Player_Colony_Autobuild(s_colony* colony);
} // namespace AIBUILD