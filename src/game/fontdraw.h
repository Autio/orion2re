#pragma once

namespace fontdraw {
    /**
     * Draws a non-antialiased encoded glyph into the destination buffer.
     * @param dest Writable start of the destination scanline in the target buffer.
     * @param src Encoded glyph byte stream for the character being drawn.
     * @param palette Palette lookup table used for non-zero glyph pixels.
     * @param height Number of glyph rows to draw.
     */
    void __cdecl Print_Character_No_Alias_ASM(uint8_t* dest, const uint8_t* src, const uint8_t* palette, int32_t height);
} // namespace fontdraw