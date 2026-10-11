#pragma once

#include <cstdint>

namespace video {
    inline bool Is_Scan_Dot_Onscreen_(int32_t x, int32_t y, int32_t width, int32_t height) {
        return x >= 0 && x < width && y >= 0 && y < height;
    }

    inline bool Normalize_Scan_Span_(
        int32_t min_x,
        int32_t max_x,
        int32_t row_dword_width,
        int32_t* clamped_min_x,
        int32_t* clamped_max_x) {
        if (clamped_min_x == nullptr || clamped_max_x == nullptr || row_dword_width <= 0) {
            return false;
        }

        if (min_x < 0) {
            min_x = 0;
        }

        if (max_x >= row_dword_width) {
            max_x = row_dword_width - 1;
        }

        if (max_x < min_x) {
            return false;
        }

        *clamped_min_x = min_x;
        *clamped_max_x = max_x;
        return true;
    }
} // namespace video