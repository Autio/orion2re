#pragma once
#include <cstdint>

namespace fade {
    /**
     * Apply a horizontal blinds-style fade over the inclusive rectangle bounds.
     * @param x1 Left x-coordinate of the fade region.
     * @param y1 Top y-coordinate of the fade region.
     * @param x2 Right x-coordinate of the fade region.
     * @param y2 Bottom y-coordinate of the fade region.
     * @param step Row spacing used for each blinds pass; values below 1 do nothing.
     */
    void __cdecl Blinds_Fade(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t step);
} // namespace fade