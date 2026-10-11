#pragma once
#include <cstdint>

struct s_colors;

namespace line {
    /**
     * Draws a line that cycles through the eight entries in the supplied color set.
     * @param x1 Start X coordinate.
     * @param y1 Start Y coordinate.
     * @param x2 End X coordinate.
     * @param y2 End Y coordinate.
     * @param colors Eight-color source used for the repeating pattern.
     * @param offset Starting index into the eight-color cycle.
     */
    void __cdecl Multi_Colored_Line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, s_colors* colors, int16_t offset);

    /**
     * Clips a line segment to the active screen window in place.
     * @param x1 Pointer to the start X coordinate.
     * @param y1 Pointer to the start Y coordinate.
     * @param x2 Pointer to the end X coordinate.
     * @param y2 Pointer to the end Y coordinate.
     * @return 0 if any portion of the line remains visible, 1 if the line is fully clipped.
     */
    int16_t __cdecl Clip_Line(int16_t* x1, int16_t* y1, int16_t* x2, int16_t* y2);

    /**
     * Draws a horizontal multicolor run and advances the destination pointer to the next row.
     * @param dest_ptr Pointer to the current destination pointer.
     * @param step Horizontal pixel step applied after each write.
     * @param length Number of pixels to draw.
     * @param colors Eight-color source used for the repeating pattern.
     * @param color_index Pointer to the current color-cycle index.
     */
    void __cdecl Draw_Horizontal_Run_Multi(uint8_t** dest_ptr, int16_t step, int16_t length, s_colors* colors, uint16_t* color_index);

    /**
     * Draws a vertical multicolor run and applies a post-run horizontal offset.
     * @param screen_ptr Pointer to the current destination pointer.
     * @param post_offset Offset applied after the vertical run completes.
     * @param height Number of pixels to draw.
     * @param colors Eight-color source used for the repeating pattern.
     * @param color_index Pointer to the current color-cycle index.
     */
    void __cdecl Draw_Vertical_Run_Multi(uint8_t** screen_ptr, int16_t post_offset, int16_t height, s_colors* colors, uint16_t* color_index);

    /**
     * Draws a solid-color line segment.
     * @param x1 Start X coordinate.
     * @param y1 Start Y coordinate.
     * @param x2 End X coordinate.
     * @param y2 End Y coordinate.
     * @param color Palette index written for each pixel.
     */
    void __cdecl Line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color);

    /**
     * Draws a vertical run of pixels and advances the caller's pointer by a post-run offset.
     * @param buf Pointer to the current destination pointer.
     * @param post_offset Offset applied after the vertical run completes.
     * @param count Number of pixels to draw.
     * @param color Palette index written for each pixel.
     */
    void __cdecl Draw_Vertical_Run(uint8_t** buf, int16_t post_offset, int16_t count, uint8_t color);

    /**
     * Draws a horizontal run of pixels and advances the caller's pointer to the next row.
     * @param buf Pointer to the current destination pointer.
     * @param step_x Horizontal pixel step applied after each write.
     * @param count Number of pixels to draw.
     * @param color Palette index written for each pixel.
     */
    void __cdecl Draw_Horizontal_Run(uint8_t** buf, int16_t step_x, int16_t count, uint8_t color);

    /**
     * Draws a plasma-colored rectangular border.
     * @param x Left edge of the rectangle.
     * @param y Top edge of the rectangle.
     * @param width Rectangle width in pixels.
     * @param height Rectangle height in pixels.
     * @param thickness Border thickness in pixels.
     * @param seed Random seed used to generate the plasma sequence.
     * @param start_value Initial plasma sample value.
     * @param color_map Palette mapping applied to generated plasma samples.
     */
    void __cdecl Plasma_Box(int16_t x, int16_t y, uint16_t width, int16_t height, int16_t thickness, int16_t seed, uint8_t start_value, const uint8_t* color_map);

    /**
     * Builds a palette-mapped plasma sequence in the shared graphics buffer.
     * @param seed Random seed used during sequence generation.
     * @param start_value Initial sample value before palette mapping.
     * @param color_map Palette mapping applied to each generated sample.
     */
    void __cdecl Create_Plasma_Line(int16_t seed, uint16_t start_value, const uint8_t* color_map);

    /**
     * Draws a line whose pixels are read from a cyclic plasma table.
     * @param x1 Start X coordinate.
     * @param y1 Start Y coordinate.
     * @param x2 End X coordinate.
     * @param y2 End Y coordinate.
     * @param color_map Plasma table used for pixel lookup.
     * @param color_index Starting plasma-table index.
     */
    void __cdecl Plasma_Line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t* color_map, uint16_t color_index);

    /**
     * Draws a horizontal plasma run and advances the destination pointer to the next row.
     * @param dest_ptr Pointer to the current destination pointer.
     * @param step Horizontal pixel step applied after each write.
     * @param length Number of pixels to draw.
     * @param plasma_table Plasma lookup table used for pixel values.
     * @param plasma_index_ptr Pointer to the current plasma-table index.
     */
    void __cdecl Draw_Horizontal_Run_Plasma(uint8_t** dest_ptr, int16_t step, int16_t length, uint8_t* plasma_table, uint16_t* plasma_index_ptr);

    /**
     * Draws a vertical plasma run and applies a post-run horizontal offset.
     * @param pixel_ptr_ref Pointer to the current destination pointer.
     * @param post_offset Offset applied after the vertical run completes.
     * @param length Number of pixels to draw.
     * @param plasma_table Plasma lookup table used for pixel values.
     * @param plasma_index_ptr Pointer to the current plasma-table index.
     */
    void __cdecl Draw_Vertical_Run_Plasma(uint8_t** pixel_ptr_ref, int16_t post_offset, int16_t length, uint8_t* plasma_table, uint16_t* plasma_index_ptr);
} // namespace line