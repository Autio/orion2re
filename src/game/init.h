#pragma once

namespace init {
    extern s_animation_header* io_buffer;

    /** Allocates and zero-initializes the global 0x2000-byte I/O buffer. */
    void __cdecl Create_IO_Buffer();

    /**
     * Initializes core drivers, loads the requested font, and clears both video pages.
     * @param unused Observed unused driver-initialization argument.
     * @param font_name Font file name to load, or an empty string to use the system font.
     */
    int8_t __cdecl Init_Drivers(uint32_t unused, char* font_name);
} // namespace init