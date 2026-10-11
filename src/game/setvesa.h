#pragma once
#include <cstdint>

namespace setvesa {
    /**
     * Adds a clipped screen-space rectangle to the refresh scan lists.
     * @param left Left pixel coordinate of the rectangle.
     * @param top Top pixel coordinate of the rectangle.
     * @param right Right pixel coordinate of the rectangle.
     * @param bottom Bottom pixel coordinate of the rectangle.
     */
    void __cdecl Add_Square_To_Scan_List(int32_t left, int32_t top, int32_t right, int32_t bottom);
} // namespace setvesa