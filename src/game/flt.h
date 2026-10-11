#pragma once

namespace FLT {
    /**
     * Computes screen-space positions for the small ship icons within a map viewport.
     * @param map_x Left edge of the map viewport in screen coordinates.
     * @param map_y Top edge of the map viewport in screen coordinates.
     * @param map_width Width of the map viewport.
     * @param map_height Height of the map viewport.
     */
    void __cdecl Set_Fltscrn_Small_Ship_Icon_XYs(int16_t map_x, int16_t map_y, int16_t map_width, int16_t map_height);
} // namespace FLT