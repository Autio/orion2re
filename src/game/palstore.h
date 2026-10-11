#pragma once

#include <cstdint>

namespace palstore {
    /* Gradually restores the palette brightness until input occurs or the fade completes. */
    int16_t Slow_Fade_In();

    /* Darkens the active palette by the requested percentage.
     * darken_percent: Percentage of darkening to apply.
     */
    void __cdecl Darken_Palette(int32_t darken_percent);

    /* Copies a range of cached palette colors into the hardware palette.
     * first_entry: First palette slot to update.
     * entry_count: Number of palette entries to copy.
     */
    void __cdecl Store_Palette_Block(int32_t first_entry, int32_t entry_count);

    /* Updates one palette color in both the cached and hardware palettes.
     * palette_index: Palette slot to update.
     * red: Red component value.
     * green: Green component value.
     * blue: Blue component value.
     */
    void __cdecl Store_Single_Palette_Color(int32_t palette_index, uint8_t red, uint8_t green, uint8_t blue);
} // namespace palstore