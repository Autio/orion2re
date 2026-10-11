#pragma once
#include <cstdint>

namespace special {
    /**
     * Returns an approximate distance between two points using the larger axis delta plus half of the smaller one.
     * @param x1 First point x coordinate.
     * @param y1 First point y coordinate.
     * @param x2 Second point x coordinate.
     * @param y2 Second point y coordinate.
     */
    int16_t __cdecl Range(int16_t x1, int16_t y1, int16_t x2, int16_t y2);

    /**
     * Moves the current point toward the target by up to step units using angle-based interpolation.
     * @param x_ptr Pointer to the current x coordinate to update.
     * @param y_ptr Pointer to the current y coordinate to update.
     * @param target_x Destination x coordinate.
     * @param target_y Destination y coordinate.
     * @param step Maximum movement distance for this step.
     */
    void __cdecl Interpolate_Line(int16_t* x_ptr, int16_t* y_ptr, int16_t target_x, int16_t target_y, int16_t step);

    /**
     * Advances the current point along the line segment from (x1, y1) to (x2, y2) by up to step units.
     * @param x_ptr Pointer to the current x coordinate to update.
     * @param y_ptr Pointer to the current y coordinate to update.
     * @param x1 Line start x coordinate.
     * @param y1 Line start y coordinate.
     * @param x2 Line end x coordinate.
     * @param y2 Line end y coordinate.
     * @param step Maximum movement distance for this step.
     */
    void __cdecl Absolute_Interpolate_Line(int16_t* x_ptr, int16_t* y_ptr, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t step);

    /**
     * Returns the integer square root of value, clamping the result to 0xFFFF for large inputs.
     * @param value Value to square-root.
     */
    uint32_t __cdecl isqrt(uint32_t value);
} // namespace special