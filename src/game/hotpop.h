#pragma once
#include <cstdint>

struct s_animation_header;

namespace HOTPOP {
    extern int16_t hotseat_human_players;
    extern s_animation_header* hot_pop_background_seg;
    extern s_animation_header* grid_background[4];

    /* Reloads the hotseat setup screen art assets into the screen segment. */
    void __cdecl Reload_Hotseat_Screen();

    /* Fades in after drawing the hotseat setup screen background. */
    void __cdecl Fade_Into_Hotseat_Screen();

    /*
     * Initializes the hotseat setup fields and returns their field ids.
     * out_join_button_field: Receives the Join button field id, or -1000 when joining is unavailable.
     * out_accept_button_field: Receives the Accept button field id.
     * out_cancel_button_field: Receives the Cancel button field id.
     */
    void __cdecl Init_Hotseat_Screen(int16_t* out_join_button_field, int16_t* out_accept_button_field, int16_t* out_cancel_button_field);

    /* Draws the hotseat setup background and player-number list. */
    void __cdecl Draw_Hotseat_Screen(void);

    /* Runs the hotseat setup loop until the player cancels or starts a new game. */
    void __cdecl Hotseat_Screen(void);
} // namespace HOTPOP