#pragma once
#include <cstdint>

namespace clipped {
    /**
     * Draws a clipped RLE-encoded animated sprite to the current render buffer.
     * @param x Left screen coordinate for the sprite origin.
     * @param y Top screen coordinate for the sprite origin.
     * @param sprite Encoded sprite command stream.
     */
    void __cdecl Clipped_Draw_Animated_Sprite(int16_t x, int16_t y, uint8_t* sprite);

    /**
     * Draws a clipped animated sprite with glass-remap pixels applied.
     * @param x Left screen coordinate for the sprite origin.
     * @param y Top screen coordinate for the sprite origin.
     * @param sprite Encoded sprite command stream.
     */
    void __cdecl Clipped_Draw_Glassed_Animated_Sprite(int32_t x, int32_t y, uint8_t* sprite);

    /**
     * Draws a clipped uncompressed bitmap sprite to the current render buffer.
     * @param x Left screen coordinate for the sprite origin.
     * @param y Top screen coordinate for the sprite origin.
     * @param pixels Source bitmap pixels in row-major order.
     */
    void __cdecl Clipped_Draw_Bitmap_Sprite(int16_t x, int16_t y, uint8_t* pixels);

    /**
     * Draws a clipped bitmap sprite with glass-remap pixels applied.
     * @param x Left screen coordinate for the sprite origin.
     * @param y Top screen coordinate for the sprite origin.
     * @param sprite_pixels Source bitmap pixels in row-major order.
     */
    void __cdecl Clipped_Draw_Glassed_Bitmap_Sprite(int32_t x, int32_t y, uint8_t* sprite_pixels);

    /**
     * Draws a clipped animated sprite and records touched scanlines for refresh.
     * @param x Left screen coordinate for the sprite origin.
     * @param y Top screen coordinate for the sprite origin.
     * @param sprite Encoded sprite command stream.
     */
    void __cdecl Clipped_Draw_Stenciled_Animated_Sprite(int16_t x, int16_t y, uint8_t* sprite);
} // namespace clipped