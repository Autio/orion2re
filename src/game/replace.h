#pragma once
#include <cstdint>

struct s_animation_header;

namespace replace {
    extern uint8_t graphics_buffer[1023];

    extern uint8_t* palette_color_list;

    /**
     * Replaces every non-zero pixel in each animation frame with one palette color.
     * @param anim Animation whose frame data is rewritten in place.
     * @param color Palette index written to non-zero pixels.
     */
    void __cdecl Replace_Color_All(s_animation_header* anim, uint8_t color);

    /**
     * Aborts when color replacement encounters an unsupported animation packing mode.
     */
    void __cdecl Exit_With_Message();

    /**
     * Remaps a contiguous palette range in every frame through a caller-provided lookup table.
     * @param anim Animation whose frame data is rewritten in place.
     * @param palette_map Replacement colors for the covered source range.
     * @param start_color First source palette index that may be replaced.
     * @param num_colors Number of palette entries covered by palette_map.
     */
    void __cdecl Replace_Color_Range(s_animation_header* anim, uint8_t* palette_map, int16_t start_color, int16_t num_colors);

    /**
     * Updates the animation palette header so its palette range starts at a shifted index.
     * @param anim Animation whose palette header is updated.
     * @param shift Palette start index to store in the header.
     */
    void __cdecl Shift_Palette_Range(s_animation_header* anim, int32_t shift);

    /**
     * Replaces one palette index with another across every frame of an animation.
     * @param anim Animation whose frame data is rewritten in place.
     * @param old_color Palette index to replace.
     * @param new_color Palette index written in its place.
     */
    void __cdecl Replace_Color(s_animation_header* anim, int16_t old_color, uint8_t new_color);

    /**
     * Builds a grayscale palette lookup from the current palette.
     * @param scale Output gradient size used to quantize each palette entry.
     * @param invert Set to 1 to invert the generated gradient.
     */
    void __cdecl Establish_Color_Gradient(int16_t scale, int16_t invert);

    /**
     * Applies two interlaced grayscale palette transforms to the off-page render buffers.
     * @param color_offset1 Palette offset added to the first generated gradient.
     * @param gradient_start1 Gradient scale used for the first pass.
     * @param color_offset2 Palette offset added to the second generated gradient.
     * @param gradient_start2 Gradient scale used for the second pass.
     * @param invert Invert flag passed through to gradient generation.
     */
    void __cdecl Interlaced_Gray_Scale_Off_Page(uint8_t color_offset1, int16_t gradient_start1, uint8_t color_offset2, int16_t gradient_start2, int16_t invert);
} // namespace replace