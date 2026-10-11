#include "pch.h"

namespace time_ {

    Fn _time;

    Fn _time2;

    int32_t __cdecl time_() {
        int32_t clockTicks = clock();
        int32_t scaled = (int32_t)((uint32_t)clockTicks * 1000u);
        return scaled / 1000;
    }

} // namespace time_