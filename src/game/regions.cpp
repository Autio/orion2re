#include "pch.h"

namespace regions {
    uint32_t rows_remaining;
    uint32_t saved_width;
    uint32_t interlaced_fill_next_pattern;

    void __cdecl Fill_Region(uint8_t color, uint8_t* surface, uint32_t width_dwords, uint32_t height) {
        const uint32_t width_bytes = width_dwords * (uint32_t)sizeof(uint32_t);
        const int32_t stride_skip = video::g_screen_res_x - (int32_t)width_bytes;
        uint32_t remaining = height;

        do {
            memset(surface, color, width_bytes);
            surface += width_bytes;
            surface += stride_skip;
            --remaining;
        } while (remaining != 0);
    }

    void __cdecl Fill_Column(uint8_t color, uint8_t* surface, uint32_t fill_width, uint32_t fill_height) {
        int32_t stride_skip = video::g_screen_res_x - (int32_t)fill_width;
        uint32_t remaining = fill_height;

        do {
            memset(surface, color, fill_width);
            surface += fill_width;
            surface += stride_skip;
            --remaining;
        } while (remaining != 0);
    }

    void __cdecl Tint_Fill_Region(uint8_t* tint_table, uint8_t* pixels, uint32_t width_dwords, int32_t row_count) {
        uint8_t* dst = pixels;
        uint8_t* src = pixels;

        rows_remaining = row_count;

        uint32_t width_bytes = width_dwords * (uint32_t)sizeof(uint32_t);
        int32_t row_skip = video::g_screen_res_x - (int32_t)width_bytes;

        saved_width = width_dwords;

        do {
            uint32_t remaining_dwords = saved_width;

            do {
                *dst++ = tint_table[*src++];
                *dst++ = tint_table[*src++];
                *dst++ = tint_table[*src++];
                *dst++ = tint_table[*src++];
            } while (--remaining_dwords != 0);

            dst += row_skip;
            src += row_skip;
        } while (--rows_remaining != 0);
    }

    void __cdecl Tint_Fill_Column(uint8_t* tint_table, uint8_t* pixels, uint32_t width, uint32_t height) {
        uint8_t* dst = pixels;
        uint8_t* src = pixels;
        const int32_t row_skip = video::g_screen_res_x - (int32_t)width;

        rows_remaining = height;
        saved_width = width;

        do {
            uint32_t remaining = saved_width;

            do {
                *dst++ = tint_table[*src++];
                --remaining;
            } while (remaining != 0);

            dst += row_skip;
            src += row_skip;
            --rows_remaining;
        } while (rows_remaining != 0);
    }

    void __cdecl Interlaced_Fill_Column(uint32_t interlace_pattern, uint8_t* dst, int32_t span, int32_t row_count) {
        const uint32_t row_padding = (uint32_t)video::g_screen_res_x - (uint32_t)span;

        do {
            memset(dst, (int)(uint8_t)interlace_pattern, (size_t)span);
            dst += span;
            dst += row_padding;

            interlace_pattern =
                (interlace_pattern & 0xFFFF0000u) |
                ((interlace_pattern & 0x000000FFu) << 8) |
                ((interlace_pattern & 0x0000FF00u) >> 8);

            --row_count;
        } while (row_count != 0);
    }

    void __cdecl Interlaced_Fill_Region(uint16_t pattern, uint8_t* dst, int32_t width_dwords, int32_t height) {
        uint32_t current_pattern = (uint32_t)pattern;
        interlaced_fill_next_pattern = current_pattern;

        {
            uint8_t fill_byte = (uint8_t)current_pattern;
            uint32_t replicated_pattern = (uint32_t)fill_byte;
            replicated_pattern |= replicated_pattern << 8;
            replicated_pattern |= replicated_pattern << 16;

            uint32_t previous_pattern = interlaced_fill_next_pattern;
            interlaced_fill_next_pattern = replicated_pattern;
            current_pattern = previous_pattern;
        }

        {
            uint8_t fill_byte = (uint8_t)(current_pattern >> 8);
            uint32_t replicated_pattern = (uint32_t)fill_byte;
            replicated_pattern |= replicated_pattern << 8;
            replicated_pattern |= replicated_pattern << 16;

            uint32_t previous_pattern = interlaced_fill_next_pattern;
            interlaced_fill_next_pattern = replicated_pattern;
            current_pattern = previous_pattern;
        }

        const int32_t width_bytes = width_dwords * (int32_t)sizeof(current_pattern);
        const int32_t row_advance = video::g_screen_res_x - width_bytes;
        const int32_t fill_count = width_dwords;
        int32_t remaining_rows = height;

        do {
            int32_t remaining = fill_count;
            do {
                memcpy(dst, &current_pattern, sizeof(current_pattern));
                dst += sizeof(current_pattern);
            } while (--remaining != 0);

            dst += row_advance;

            uint32_t previous_pattern = interlaced_fill_next_pattern;
            interlaced_fill_next_pattern = current_pattern;
            current_pattern = previous_pattern;

        } while (--remaining_rows != 0);
    }

    void __cdecl Tri_Interlaced_Fill_Column(uint32_t color, uint8_t* dest, int32_t width, int32_t height) {
        const int32_t row_advance = video::g_screen_res_x - width;

        do {
            memset(dest, (uint8_t)color, width);
            dest += width;
            dest += row_advance;
            color = (color >> 8) | (color << 24);
            if (--height == 0) {
                return;
            }

            memset(dest, (uint8_t)color, width);
            dest += width;
            dest += row_advance;
            color = (color >> 8) | (color << 24);
            if (--height == 0) {
                return;
            }

            memset(dest, (uint8_t)color, width);
            dest += width;
            dest += row_advance;
            color = (color >> 16) | (color << 16);
        } while (--height != 0);
    }

    void __cdecl Tri_Interlaced_Fill_Region(
        uint32_t next_pattern,
        uint8_t* dst,
        uint32_t width_dwords,
        uint32_t height) {
        interlaced_fill_next_pattern = next_pattern;

        const uint32_t fill_width = width_dwords & 0x3FFFFFFFu;
        const uint32_t fill_width_bytes = fill_width * (uint32_t)sizeof(uint32_t);
        const uint32_t row_advance = video::g_screen_res_x - fill_width_bytes;

        while (true) {
            memset(dst, (int)(uint8_t)interlaced_fill_next_pattern, (size_t)fill_width_bytes);
            dst += fill_width_bytes;
            dst += row_advance;
            interlaced_fill_next_pattern =
                (interlaced_fill_next_pattern >> 8) |
                (interlaced_fill_next_pattern << 24);

            if (--height == 0) {
                break;
            }

            memset(dst, (int)(uint8_t)interlaced_fill_next_pattern, (size_t)fill_width_bytes);
            dst += fill_width_bytes;
            dst += row_advance;
            interlaced_fill_next_pattern =
                (interlaced_fill_next_pattern >> 8) |
                (interlaced_fill_next_pattern << 24);

            if (--height == 0) {
                break;
            }

            memset(dst, (int)(uint8_t)interlaced_fill_next_pattern, (size_t)fill_width_bytes);
            dst += fill_width_bytes;
            dst += row_advance;
            interlaced_fill_next_pattern =
                (interlaced_fill_next_pattern >> 16) |
                (interlaced_fill_next_pattern << 16);

            if (--height != 0) {
                continue;
            }

            break;
        }
    }
} // namespace regions