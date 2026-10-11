#pragma once
#include <cstdint>

namespace draw {
    /**
     * @brief Draws a packed animated-sprite frame to the current video page.
     * @param x Left destination position.
     * @param y Top destination position.
     * @param frame_data Packed frame data beginning with an s_packed_frame_header.
     */
    void __cdecl Draw_Animated_Sprite(int16_t x, int16_t y, uint8_t* frame_data);

    /**
     * @brief Draws a raw bitmap sprite to the current video page.
     * @param x Left destination position.
     * @param y Top destination position.
     * @param bitmap_data Source pixel data laid out row-by-row.
     */
    void __cdecl Draw_Bitmap_Sprite(int16_t x, int16_t y, uint8_t* bitmap_data);

    /**
     * @brief Copies non-zero source pixels into the destination buffer.
     * @param src Source pixel stream.
     * @param dst Destination pixel stream.
     * @param pixel_count Number of pixels to process.
     */
    void __cdecl Color_Stream_Copy(uint8_t* src, uint8_t* dst, int32_t pixel_count);
} // namespace draw