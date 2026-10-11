#pragma once

#include <cstdint>

namespace timer {
    extern uint32_t time_mark;

    /** Stores the current tick count as the baseline for later elapsed-time queries. */
    void __cdecl Mark_Time();

    /** Returns the number of milliseconds elapsed since the last call to Mark_Time(). */
    int32_t __cdecl Get_Time_Since_Mark();

    /** Applies Page Up and Page Down animation-speed controls without consuming the key. */
    void __cdecl Handle_Animation_Speed_Key(uint32_t key_code);

    /** Returns the session animation-speed offset in the range -3 through 3. */
    int32_t __cdecl Get_Animation_Speedup();

    /** Returns true while the most recent animation-speed change should be displayed. */
    bool __cdecl Animation_Speed_Change_Is_Visible();

    /** Busy-waits until the requested number of timer ticks has elapsed since the last mark.
     *  @param ticks Number of 55 ms ticks to wait.
     */
    void __cdecl Release_Time(int32_t ticks);
} // namespace timer