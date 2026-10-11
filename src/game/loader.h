#pragma once

namespace LOADER {
    /** Loads the main-screen button, fleet-box, star, and player-light animations into the global screen segment. */
    void __cdecl Load_Main_Screen_Pictures();
    /** Loads shared UI picture resources and initializes the main screen backing animation buffer. */
    void __cdecl Load_Pictures();
    /** Loads the localized special-ability name strings and indexes them into the global pointer table. */
    void __cdecl Load_Player_Special_Names();
} // namespace LOADER