#pragma once

#include <cstdint>

struct s_mouse_list;

namespace sysfont {
    extern uint32_t thin_column_lookup[8];
    extern uint32_t font_pixel_data[4096];
    extern uint8_t default_mouse_palette[576];

    extern uint32_t mouse_cursor_color;
    extern s_mouse_list default_system_mouse_list;
    extern uint16_t default_mouse_image[576];
    extern uint8_t default_palette_colors[48];

    /* Initializes the built-in cursor palette, image, and default mouse list. */
    void __cdecl Load_System_Mouse();
    /* Installs the built-in mouse assets and builds the encoded default font glyph stream. */
    void __cdecl Load_System_Font();
} // namespace sysfont