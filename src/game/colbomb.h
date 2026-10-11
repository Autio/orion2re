#pragma once

struct s_combat_bomb_result;

namespace COLBOMB {
    extern s_combat_bomb_result* results_ptr;

    extern s_combat_bomb_result* backup_results_ptr;

    /**
     * Builds and displays the colony bombing summary popup from the backed-up combat results.
     */
    void __cdecl Do_Summary_String_Popup();

    /**
     * Snapshots the bombing results, shows the summary popup, and restores colony-screen globals.
     *
     * @param results Bombardment results to display.
     */
    void __cdecl Colony_Bombing_Screen(s_combat_bomb_result* results);
} // namespace COLBOMB