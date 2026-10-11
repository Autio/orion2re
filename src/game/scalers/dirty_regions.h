#pragma once

#include "game/scalers/scaler.h"

namespace scalers {
    struct s_dirty_workspace {
        uint8_t changed[MAX_DIRTY_BLOCKS];
    };

    bool Find_Dirty_Regions(
        const uint32_t* current,
        int32_t current_stride,
        const uint32_t* previous,
        int32_t previous_stride,
        int32_t width,
        int32_t height,
        int32_t dependency_radius,
        s_dirty_workspace* workspace,
        s_region* regions,
        int32_t region_capacity,
        int32_t* region_count);
} // namespace scalers