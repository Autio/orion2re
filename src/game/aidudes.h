#pragma once

struct s_colony;
struct s_aidudes_colony_info;

namespace aidudes {
    extern bool builds_colony_bases_prewarp;
    extern int16_t n_our_blockaded;
    extern int16_t n_our_unblockaded;
    extern s_aidudes_colony_info* our_blockaded;

    extern s_player* p_ai;

    extern s_aidudes_colony_info* our_unblockaded;
    extern int16_t n_our_colonies;
    extern s_aidudes_colony_info* Sort_Pops_colony_info;

    extern s_colony* Sort_Pops_colony_data;

    extern int16_t i_ai;

    extern s_aidudes_colony_info (*our_colonies)[MAX_COLONIES];

    /**
     * Sorts colony-info entries so blockaded colonies are processed first.
     * @param a First colony-info entry.
     * @param b Second colony-info entry.
     */
    int __cdecl Sort_Colonies_Blockaded(const void* a, const void* b);

    /**
     * Collects the current AI player's owned, non-outpost colonies into the working lists.
     */
    void __cdecl Collect_AI_Colonies();

    /**
     * Clears build queues after a tech change when the colony is on a rebuild-eligible project.
     * @param force_clear Nonzero to force reevaluation immediately.
     */
    void __cdecl Clear_Build_Queues_If_New_Tech(bool force_clear);

    /**
     * Refreshes ships under construction so they use the current design template.
     */
    void __cdecl Keep_Ship_Designs_Up_To_Date();

    /**
     * Grants emergency freighters to an AI empire that is short on food logistics.
     * @param player_idx Player receiving the freighter check.
     */
    void __cdecl Freighter_Cheat(int32_t player_idx);

    /**
     * Opportunistically upgrades eligible AI ships using the current refit tables.
     */
    void __cdecl Do_Ship_Upgrade_Cheats();

    /**
     * Runs the colony-allocation passes after tax policy has already been chosen.
     */
    void __cdecl Colony_AI_Given_Tax();

    int __cdecl Sort_Pops_FI(const void* a, const void* b);

    int __cdecl Sort_Pops_F_IR(const void* a, const void* b);

    int16_t __cdecl FI_Worthiness(const s_colony* colony, const uint32_t* pop);

    int __cdecl F_IR_Worthiness(s_aidudes_colony_info* info, uint32_t* flags);

    /**
     * Processes a blockaded colony's farmer and worker assignments.
     * @param colony_info Working colony-info record for the colony.
     */
    void __cdecl Do_Blockaded_Colony(s_aidudes_colony_info* colony_info);

    /**
     * Processes an unblockaded colony's worker and researcher assignments.
     * @param info Working colony-info record for the colony.
     */
    void __cdecl Do_Unblockaded_Colony(s_aidudes_colony_info* info);

    /**
     * Assigns required fixed colonists before the colony-specific AI pass runs.
     * @param colony_info Working colony-info record to populate.
     */
    void __cdecl Assign_Required_Colonists(s_aidudes_colony_info* colony_info);

    void __cdecl Make_First_Unassigned_Into_Farmer(s_aidudes_colony_info* info);

    int __cdecl Sort_Pops_Fixed(const void* a, const void* b);

    void __cdecl Compute_Max_Min_R_I_Worthiness(s_aidudes_colony_info* info);

    int __cdecl _Sort_Pops_colony(const void* a, const void* b);

    int __cdecl Sort_Pops_FR_I(const void* a, const void* b);

    /**
     * Balances empire-wide worker and scientist assignments across unblockaded colonies.
     */
    void __cdecl Assign_Additional_Research_Industry();

    void __cdecl Grab_Scientist(s_aidudes_colony_info* colony);

    void __cdecl Grab_Worker(s_aidudes_colony_info* info);

    /**
     * Selects a shared emergency build project for colonies with open queues.
     * @param player_idx Player whose colonies are being updated.
     */
    void __cdecl Assign_Empire_Building(int16_t player_idx);

    /**
     * Revalidates researching colonies after assignment changes.
     */
    void __cdecl Industry_Sanity_Check();

    /**
     * Adds extra farmers until the empire's food and freighter shortages are resolved.
     */
    void __cdecl Assign_Additional_Unblockaded_Farmers();

    /**
     * Runs the full colony-AI pass for one player.
     * @param i_ai AI player index to process.
     */
    void __cdecl Colony_AI(int16_t i_ai);

    /**
     * Runs colony AI for every active player.
     */
    void __cdecl All_Colony_AI();

    /**
     * Lets the building AI finalize per-colony build choices after empire-wide decisions.
     */
    void __cdecl Assign_Buildings();
    int __cdecl colony_less_industry_cmp(const void* p1, const void* p2);

    int __cdecl Sort_Pops_R_I(const void* p1, const void* p2);
} // namespace aidudes