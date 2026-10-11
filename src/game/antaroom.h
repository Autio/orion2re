#pragma once
#include <cstdint>

#define ANTARAN_ROOM_MESSAGE_CAPACITY 210

struct s_mouse_list;
struct s_colors;

namespace ANTAROOM {
    extern s_colors antaran_room_normal_colors;

    extern char* antaran_room_message;
    extern uint8_t* antaran_room_mouse_bitmap;

    extern s_mouse_list antaran_room_blank_mouse[1];

    /// Reloads the Antaran room background, animation, and mouse assets.
    void __cdecl Reload_Main_Antaran_Room_Screen();

    /// Loads one localized Antaran room message into the active text buffer.
    /// @param message_index Selects which message set to load from the LBX file.
    void __cdecl Get_Antaran_Room_Message(int16_t message_index);

    /// Draws the Antaran room screen and its current message.
    void __cdecl Draw_Main_Antaran_Room_Screen();

    /// Runs the Antaran room screen until dismissed by input.
    /// @param message_set_index Selects which message variant group to display.
    void __cdecl Main_Antaran_Room_Screen(int16_t message_set_index);
} // namespace ANTAROOM