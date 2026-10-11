#pragma once

struct s_animation_header;

namespace remap {
    extern uint8_t picture_remap_color_list[256];

    /**
     * Rebuilds the glass remap tables for the current palette and configured glass colors.
     */
    void __cdecl Update_Glass_Remap_Colors();

    /**
     * Rebuilds the palette index tables used by closest-color searches.
     */
    void __cdecl Sort_Palette_List();

    /**
     * Returns the nearest nonzero palette index to the requested RGB triplet.
     * @param r Red channel intensity to match.
     * @param g Green channel intensity to match.
     * @param b Blue channel intensity to match.
     */
    uint8_t __cdecl Find_Closest_Color(uint8_t r, uint8_t g, uint8_t b);

    /**
     * Builds a remap table that maps a picture palette onto the current video palette.
     * @param src_palette Source palette data stored as 4-byte entries.
     * @param remap_table Output table that receives remapped palette indices.
     */
    void __cdecl Create_Picture_Remap_Palette(uint8_t* src_palette, uint8_t* remap_table);

    /**
     * Computes remap colors for an animation resource that carries an embedded palette.
     * @param animation_header Animation resource whose embedded palette should be remapped.
     */
    void __cdecl Calculate_Picture_Remap_Colors(s_animation_header* animation_header);

    /**
     * Returns a cached remapped palette entry by index.
     * @param index Remap-table index to read.
     */
    uint8_t __cdecl Get_Remap_Palette_Color(uint8_t index);

    /**
     * Reads one RGB palette entry from an animation resource with an embedded palette.
     * @param animation Animation resource that owns the palette data.
     * @param palette_index Palette index to read from the embedded palette.
     * @param out_red Receives the red channel.
     * @param out_green Receives the green channel.
     * @param out_blue Receives the blue channel.
     */
    void __cdecl Get_Palette_Animation_Color(
        s_animation_header* animation,
        int16_t palette_index,
        uint8_t* out_red,
        uint8_t* out_green,
        uint8_t* out_blue);
} // namespace remap