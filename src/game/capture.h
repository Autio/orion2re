#pragma once
#include <cstdint>

namespace capture {
    extern uint16_t release_flag;

    // Marks the build/runtime state as release mode.
    void Release_Version();

    // Handles the screen FLIC capture hook; currently performs no work.
    void __cdecl Screen_Flic_Capture();

    // Returns the current release-mode flag state.
    uint16_t __cdecl Check_Release_Version();
} // namespace capture