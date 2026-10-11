#pragma once
#include <cstdint>

namespace regions {
    extern uint32_t rows_remaining;

    extern uint32_t saved_width;

    extern uint32_t interlaced_fill_next_pattern;

    /* Fills a rectangular byte span in successive screen rows with a solid color.
     * @param color Palette index written to each destination byte.
     * @param surface Pointer to the first destination pixel.
     * @param fill_width Number of bytes to fill per row.
     * @param fill_height Number of rows to fill.
     */
    void __cdecl Fill_Column(uint8_t color, uint8_t* surface, uint32_t fill_width, uint32_t fill_height);

    /* Fills a rectangular region whose row width is expressed in 32-bit pixel groups.
     * @param color Palette index written to each destination byte.
     * @param surface Pointer to the first destination pixel.
     * @param width_dwords Number of 32-bit pixel groups to fill per row.
     * @param height Number of rows to fill.
     */
    void __cdecl Fill_Region(uint8_t color, uint8_t* surface, uint32_t width_dwords, uint32_t height);

    /* Applies a tint table to a rectangular byte span row by row.
     * @param tint_table Lookup table used to remap source pixels.
     * @param pixels Pointer to the first pixel in the destination region.
     * @param width Number of bytes to tint per row.
     * @param height Number of rows to tint.
     */
    void __cdecl Tint_Fill_Column(uint8_t* tint_table, uint8_t* pixels, uint32_t width, uint32_t height);

    /* Applies a tint table to a rectangular region whose row width is expressed in 32-bit pixel groups.
     * @param tint_table Lookup table used to remap source pixels.
     * @param pixels Pointer to the first pixel in the destination region.
     * @param width_dwords Number of 32-bit pixel groups to tint per row.
     * @param row_count Number of rows to tint.
     */
    void __cdecl Tint_Fill_Region(uint8_t* tint_table, uint8_t* pixels, uint32_t width_dwords, int32_t row_count);

    /* Fills a rectangular byte span while swapping the low pattern bytes after each row.
     * @param interlace_pattern Row-fill pattern whose low byte is written by memset.
     * @param dst Pointer to the first destination pixel.
     * @param span Number of bytes to fill per row.
     * @param row_count Number of rows to fill.
     */
    void __cdecl Interlaced_Fill_Column(uint32_t interlace_pattern, uint8_t* dst, int32_t span, int32_t row_count);

    /* Fills a rectangular region with alternating 32-bit patterns derived from a 16-bit source pattern.
     * @param pattern Two-byte source pattern used to build alternating fill words.
     * @param dst Pointer to the first destination pixel.
     * @param width_dwords Number of 32-bit pixel groups to fill per row.
     * @param height Number of rows to fill.
     */
    void __cdecl Interlaced_Fill_Region(uint16_t pattern, uint8_t* dst, int32_t width_dwords, int32_t height);

    /* Fills a rectangular byte span while rotating the fill byte through a three-row pattern.
     * @param color Packed fill pattern whose bytes are rotated between rows.
     * @param dest Pointer to the first destination pixel.
     * @param width Number of bytes to fill per row.
     * @param height Number of rows to fill.
     */
    void __cdecl Tri_Interlaced_Fill_Column(uint32_t color, uint8_t* dest, int32_t width, int32_t height);

    /* Fills a rectangular region while rotating the 32-bit fill pattern in a three-row cycle.
     * @param next_pattern Initial packed fill pattern.
     * @param dst Pointer to the first destination pixel.
     * @param width_dwords Number of 32-bit pixel groups to fill per row.
     * @param height Number of rows to fill.
     */
    void __cdecl Tri_Interlaced_Fill_Region(uint32_t next_pattern, uint8_t* dst, uint32_t width_dwords, uint32_t height);
} // namespace regions