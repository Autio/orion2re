#include "pch.h"

namespace COLBCACH {
    s_animation_header* __cdecl Cache_Load_Bldg(int16_t building_x, int16_t building_y, int16_t building_type) {
        char filename[28];
        int32_t type_index = (int32_t)building_type - BUILDING_ALIEN_CONTROL_CENTER;
        int32_t file_number = type_index / 10;
        int32_t frame_index = type_index % 10;

        char* filename_format = ESTRINGS::E_Strings(ESTR_BLDGDLBX1);

        void* global_cache_seg = MOX::global_cache_seg;
        snprintf(filename, sizeof(filename), filename_format, file_number);

        int16_t base_building_frame = COLONY::Bldg_Coords_To_Effective_Frame(building_x, building_y);

        int32_t effective_frame = (int32_t)base_building_frame + (frame_index * 36);

        return (s_animation_header*)buffer::Buffer_Reload(filename, effective_frame, global_cache_seg);
    }
} // namespace COLBCACH