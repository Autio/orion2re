#include "pch.h"

#include "video_scan_bounds.h"

namespace setvesa {
    void __cdecl Add_Square_To_Scan_List(int32_t left, int32_t top, int32_t right, int32_t bottom) {
        if (video::g_screen_res_x <= left || video::g_screen_res_y <= top || right < 0 || bottom < 0) {
            return;
        }

        if (left < 0) {
            left = 0;
        }

        if (top < 0) {
            top = 0;
        }

        if (video::g_screen_res_x <= right) {
            right = video::g_screen_res_x - 1;
        }

        if (video::g_screen_res_y <= bottom) {
            bottom = video::g_screen_res_y - 1;
        }

        if (bottom < top) {
            return;
        }

        int32_t min_scan_x32 = left >> 2;
        int32_t max_scan_x32 = right >> 2;
        const int32_t row_dword_width = (video::g_screen_res_x + 3) >> 2;
        if (!video::Normalize_Scan_Span_(
                min_scan_x32,
                max_scan_x32,
                row_dword_width,
                &min_scan_x32,
                &max_scan_x32)) {
            return;
        }

        const int16_t min_scan_x = (int16_t)min_scan_x32;
        const int16_t max_scan_x = (int16_t)max_scan_x32;

        if (video::screen_refresh_flag == 2) {
            for (int32_t row = top; row <= bottom; ++row) {
                if (video::refresh_scan_list[row].min_x == -1) {
                    video::refresh_scan_list[row].min_x = min_scan_x;
                    video::refresh_scan_list[row].max_x = max_scan_x;
                } else {
                    if (min_scan_x < video::refresh_scan_list[row].min_x) {
                        video::refresh_scan_list[row].min_x = min_scan_x;
                    }
                    if (video::refresh_scan_list[row].max_x < max_scan_x) {
                        video::refresh_scan_list[row].max_x = max_scan_x;
                    }
                }
            }
        } else {
            for (int32_t row = top; row <= bottom; ++row) {
                if (video::refresh_scan_list[row].min_x == -1) {
                    video::refresh_scan_list[row].min_x = min_scan_x;
                    video::refresh_scan_list[row].max_x = max_scan_x;
                } else {
                    if (min_scan_x < video::refresh_scan_list[row].min_x) {
                        video::refresh_scan_list[row].min_x = min_scan_x;
                    }
                    if (video::refresh_scan_list[row].max_x < max_scan_x) {
                        video::refresh_scan_list[row].max_x = max_scan_x;
                    }
                }

                if (video::refresh_stencil_list[row].min_x == -1) {
                    video::refresh_stencil_list[row].min_x = min_scan_x;
                    video::refresh_stencil_list[row].max_x = max_scan_x;
                } else {
                    if (min_scan_x < video::refresh_stencil_list[row].min_x) {
                        video::refresh_stencil_list[row].min_x = min_scan_x;
                    }
                    if (video::refresh_stencil_list[row].max_x < max_scan_x) {
                        video::refresh_stencil_list[row].max_x = max_scan_x;
                    }
                }
            }
        }
    }
} // namespace setvesa