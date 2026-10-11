#pragma once
#include <cstdint>

namespace shear {
    extern uint16_t x_step_8_8;

    extern uint16_t dst_width;

    extern uint32_t y_accum_8_8;

    extern int32_t y_step_8_8;

    extern uint32_t noise_threshold;

    extern uint32_t src_width;

    extern uint32_t src_height;

    extern uint32_t row_dword_count;

    extern uint32_t row_dword_counter;

    extern uint32_t noise_start_index;

    extern uint32_t jumble_mask;

    extern uint32_t jumble_direction_toggle;

    extern int32_t src_row_skip;

    extern int32_t dst_row_skip;

    extern uint32_t col_temp;

    extern uint32_t outline_color;

    extern uint32_t temp;

    extern uint8_t* src_bitmap_end;

    extern uint32_t row_temp;

    extern uint8_t noise_table[258];

    /**
     * Draws an outline around nonzero source pixels into the destination bitmap.
     * @param src Source bitmap pixels.
     * @param outline_color Palette index used for the outline.
     * @param unused_param Unused legacy parameter.
     * @param dest Destination bitmap pixels.
     * @param width Bitmap width in pixels.
     * @param height Bitmap height in pixels.
     */
    void __cdecl Outline_Bitmap_Pixels(uint8_t* src, uint32_t outline_color, uint32_t unused_param, uint8_t* dest, uint32_t width, uint32_t height);

    /**
     * Draws an outline while treating zero, outline-color, and high palette entries as transparent.
     * @param src Source bitmap pixels.
     * @param outline_color Palette index used for the outline.
     * @param unused_param Unused legacy parameter.
     * @param dest Destination bitmap pixels.
     * @param width Bitmap width in pixels.
     * @param height Bitmap height in pixels.
     */
    void __cdecl Outline_Bitmap_Pixels_No_Glass(uint8_t* src, uint32_t outline_color, uint32_t unused_param, uint8_t* dest, uint32_t width, uint32_t height);

    /**
     * Copies a rectangular block from the active screen page into a packed destination buffer.
     * @param src Source screen address.
     * @param width Row width in pixels.
     * @param height Row count in pixels.
     * @param dst Destination bitmap buffer.
     */
    void __cdecl Capture_Screen_Block(uint8_t* src, int32_t width, int32_t height, uint8_t* dst);
    /**
     * Copies each source row into a separate destination buffer with pixels reversed left-to-right.
     * @param src Source bitmap pixels.
     * @param width Row width in pixels.
     * @param height Row count in pixels.
     * @param dst Destination bitmap pixels.
     */
    void __cdecl Flip_Bitmap_Down_Different(uint8_t* src, int32_t width, int32_t height, uint8_t* dst);
    /**
     * Mirrors each row of a bitmap in place.
     * @param data Bitmap pixels to modify in place.
     * @param width Row width in pixels.
     * @param height Row count in pixels.
     */
    void __cdecl Flip_Bitmap_Down_Same(uint8_t* data, uint32_t width, uint32_t height);
    /**
     * Copies source rows into a separate destination buffer from bottom to top.
     * @param src Source bitmap pixels.
     * @param width Row width in pixels.
     * @param height Row count in pixels.
     * @param dst Destination bitmap pixels.
     */
    void __cdecl Flip_Bitmap_Left_Different(uint8_t* src, uint32_t width, uint32_t height, uint8_t* dst);
    /**
     * Flips a bitmap vertically in place by swapping top and bottom rows.
     * @param pixels Bitmap pixels to modify in place.
     * @param width Row width in pixels.
     * @param height Row count in pixels.
     */
    void __cdecl Flip_Bitmap_Left_Same(uint8_t* pixels, uint32_t width, uint32_t height);

    /**
     * Applies a lookup-table remap to every other scanline of an off-screen page.
     * @param page Off-screen page buffer.
     * @param lut Pixel remap lookup table.
     */
    void __cdecl Interlace_Scale_Off_Page_ASM(uint8_t* page, uint8_t* lut);

    /**
     * Clears destination pixels wherever the source bitmap is nonzero.
     * @param src Source mask pixels.
     * @param src_row_skip Bytes to skip in the source after each row.
     * @param dst_row_skip Bytes to skip in the destination after each row.
     * @param dst Destination bitmap pixels.
     * @param width Row width in pixels.
     * @param height Row count in pixels.
     */
    void __cdecl Mask_Bitmap_Pixels(uint8_t* src, int32_t src_row_skip, int32_t dst_row_skip, uint8_t* dst, int32_t width, int32_t height);
    /**
     * Clears destination pixels wherever the source bitmap is zero.
     * @param src Source mask pixels.
     * @param src_row_skip Bytes to skip in the source after each row.
     * @param dst_row_skip Bytes to skip in the destination after each row.
     * @param dst Destination bitmap pixels.
     * @param width Row width in pixels.
     * @param height Row count in pixels.
     */
    void __cdecl Mask_Bitmap_Pixels_Inverse(uint8_t* src, int32_t src_row_skip, int32_t dst_row_skip, uint8_t* dst, int32_t width, int32_t height);

    /**
     * Brightens destination pixels using the source mask and brighten palette table.
     * @param src Source mask pixels.
     * @param src_row_skip Bytes to skip in the source after each row.
     * @param dst_row_skip Bytes to skip in the destination after each row.
     * @param dst Destination bitmap pixels.
     * @param width Row width in pixels.
     * @param height Row count in pixels.
     */
    void __cdecl Mask_Brighten_Bitmap_Pixels(uint8_t* src, int32_t src_row_skip, int32_t dst_row_skip, uint8_t* dst, int32_t width, int32_t height);

    /**
     * Clears packed 8-bit pixels whose palette index falls within the inclusive color range.
     * @param pixels Packed pixel dwords processed four bytes at a time.
     * @param min_color Inclusive low palette bound.
     * @param max_color Inclusive high palette bound.
     * @param unused_param Unused legacy parameter.
     * @param width Bitmap width in pixels.
     * @param height Bitmap height in pixels.
     */
    void __cdecl Mask_Bitmap_Colors_Pixels(uint32_t* pixels, uint32_t min_color, uint32_t max_color, uint32_t unused_param, uint32_t width, uint32_t height);

    /**
     * Scales a source bitmap up into the destination using reverse fixed-point stepping.
     * @param src_ptr Source bitmap pixels.
     * @param scale_x Horizontal scaling input.
     * @param scale_y Vertical scaling input.
     * @param dst_ptr Destination bitmap pixels.
     * @param src_width Source bitmap width in pixels.
     * @param src_height Source bitmap height in pixels.
     * @param dst_width Destination bitmap width in pixels.
     * @param dst_height Destination bitmap height in pixels.
     */
    void __cdecl Enlarge_Bitmap(uint8_t* src_ptr, uint32_t scale_x, uint32_t scale_y, uint8_t* dst_ptr, uint32_t src_width, uint32_t src_height, uint32_t dst_width, uint32_t dst_height);

    /**
     * Scales a source bitmap down into the destination using the original fixed-point stepping behavior.
     * @param src_ptr Source bitmap pixels.
     * @param src_width Source bitmap width in pixels.
     * @param src_height Source bitmap height in pixels.
     * @param dst_ptr Destination bitmap pixels.
     * @param src_pitch Source row pitch used by the stepping logic.
     * @param unused_param Unused legacy parameter.
     * @param dst_width Destination bitmap width in pixels.
     * @param dst_height Destination bitmap height in pixels.
     */
    void __cdecl Reduce_Bitmap(uint8_t* src_ptr, uint32_t src_width, uint32_t src_height, uint8_t* dst_ptr, uint32_t src_pitch, uint32_t unused_param, uint32_t dst_width, uint32_t dst_height);

    /**
     * Randomly clears source pixels into the destination according to the supplied noise threshold.
     * @param src Source bitmap pixels.
     * @param noise_threshold Threshold applied to the noise table.
     * @param unused_param Unused legacy parameter.
     * @param dst Destination bitmap pixels.
     * @param width Bitmap width in pixels.
     * @param height Bitmap height in pixels.
     */
    void __cdecl Vanish_Bitmap_Pixels(uint8_t* src, uint32_t noise_threshold, uint32_t unused_param, uint8_t* dst, uint32_t width, uint32_t height);

    /**
     * Conditionally swaps nearby source pixels into the destination using jumble noise.
     * @param src_buf Source bitmap pixels.
     * @param jumble_mask Maximum neighbor offset mask.
     * @param noise_threshold Threshold applied to the noise table.
     * @param dst_buf Destination bitmap pixels.
     * @param width Bitmap width in pixels.
     * @param height Bitmap height in pixels.
     */
    void __cdecl Jumble_Bitmap_Pixels(uint8_t* src_buf, uint32_t jumble_mask, uint32_t noise_threshold, uint8_t* dst_buf, uint32_t width, uint32_t height);

    /**
     * Remaps bitmap pixels through the supplied grayscale table.
     * @param pixels Bitmap pixels to modify in place.
     * @param gray_scale_table Lookup table indexed by pixel value.
     * @param y_step_8_8 Legacy 8.8 fixed-point row step value.
     * @param pixels_per_row Number of pixels processed per row.
     * @param row_count Number of rows to process.
     */
    void __cdecl Gray_Scale_Fill_Pixels(uint8_t* pixels, uint8_t* gray_scale_table, int32_t y_step_8_8, uint32_t pixels_per_row, uint32_t row_count);
} // namespace shear