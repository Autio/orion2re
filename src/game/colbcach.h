#pragma once
#include <cstdint>

struct s_animation_header;

namespace COLBCACH {
    // Loads the cached building animation for the building at the given colony coordinates.
    // building_x: Building X coordinate in the colony grid.
    // building_y: Building Y coordinate in the colony grid.
    // building_type: Building type identifier used to select the LBX file and frame.
    s_animation_header* __cdecl Cache_Load_Bldg(int16_t building_x, int16_t building_y, int16_t building_type);
} // namespace COLBCACH