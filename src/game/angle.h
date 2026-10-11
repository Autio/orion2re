#pragma once
#include <cstdint>

namespace angle {
    /** Returns the polar angle in degrees for the supplied vector components.
     * @param dx Horizontal vector component.
     * @param dy Vertical vector component.
     */
    int16_t __cdecl Get_Angle(int32_t dx, int32_t dy);

    extern uint8_t atan_lookup_table[128];

    extern uint16_t sin_table[90];

    /** Returns the cosine-scaled value for an angle in degrees.
     * @param angle Angle in degrees.
     * @param magnitude Value multiplied by the cosine result.
     */
    int32_t __cdecl Cos(int16_t angle, int32_t magnitude);
    /** Returns the sine-scaled value for an angle in degrees.
     * @param angle Angle in degrees.
     * @param magnitude Value multiplied by the sine result.
     */
    int32_t __cdecl Sin(int16_t angle, int32_t magnitude);

    /** Rotates a coordinate pair in place by the given angle in degrees.
     * @param angle Rotation angle in degrees.
     * @param x Receives the rotated x coordinate.
     * @param y Receives the rotated y coordinate.
     */
    void __cdecl Rotate_Coordinates(int16_t angle, int32_t* x, int32_t* y);
} // namespace angle