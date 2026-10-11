#pragma once

#include <cstdint>

struct s_mouse_list;

namespace SMACK {
    extern s_mouse_list smack_mouse;

    extern MOX2::action_func_t smack_auto_function;

    extern int16_t smack_x;

    extern int16_t smack_y;

    /**
     * Applies the palette fade used around Smacker cinematic playback.
     * @param fade_mode Observed fade selector used by the cinematic wrapper.
     */
    void __cdecl Smack_Fade(int16_t fade_mode);

    /**
     * Plays a Smacker cinematic and restores video/audio state afterward.
     * @param smacker_name Resource name passed to the Smacker loader.
     * @param play_flags Playback flags forwarded to the animation backend.
     * @param use_fade Nonzero to wrap playback in the cinematic fade effect.
     */
    void __cdecl Play_Cinematic(const char* smacker_name, uint32_t play_flags, char use_fade);
} // namespace SMACK