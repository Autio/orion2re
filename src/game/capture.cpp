#include "pch.h"

namespace capture {
    uint16_t release_flag;

    void Release_Version() {
        release_flag = 1;
    }

    uint16_t __cdecl Check_Release_Version() {
        return release_flag;
    }

    void __cdecl Screen_Flic_Capture() {
    }
} // namespace capture