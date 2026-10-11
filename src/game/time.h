#pragma once

namespace time_ {
    using Fn = int32_t (*)();
    extern Fn _time;

    extern Fn _time2;

    /** Returns the current `clock()` tick value using the existing millisecond scaling sequence. */
    int32_t __cdecl time_();
} // namespace time_