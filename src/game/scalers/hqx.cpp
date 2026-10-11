#include "pch.h"

#include "game/scalers/hqx.h"

/*
 * Copyright (c) 2014 Clément Bœsch
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

// Adapted from FFmpeg n7.1 libavfilter/vf_hqx.c at commit
// b08d7969c550a804a59511c7b83f2dd8cc0499b8. The filter-framework and
// threading integration were replaced with a stride-aware pixel-buffer API.
namespace hqx {
    namespace {
        uint32_t g_rgb565_to_yuv[65536];
        thread_local uint32_t g_yuv_rows[scalers::MAX_SOURCE_WIDTH * 3];
        bool g_initialized;

        uint32_t RGB_To_YUV(int32_t red, int32_t green, int32_t blue) {
            const int32_t red_green = red - green;
            const int32_t blue_green = blue - green;
            const uint32_t y = (uint32_t)((299 * red_green + 1000 * green + 114 * blue_green) / 1000);
            const uint32_t u = (uint32_t)((-169 * red_green + 500 * blue_green) / 1000 + 128);
            const uint32_t v = (uint32_t)((500 * red_green - 81 * blue_green) / 1000 + 128);
            return (y << 16) | (u << 8) | v;
        }

        inline uint32_t Lookup_YUV(uint32_t color) {
            const uint32_t rgb565 =
                ((color & 0x00f80000) >> 8) |
                ((color & 0x0000fc00) >> 5) |
                ((color & 0x000000f8) >> 3);
            return g_rgb565_to_yuv[rgb565];
        }

        inline bool YUV_Diff(uint32_t yuv1, uint32_t yuv2) {
            const int32_t y1 = (yuv1 >> 16) & 0xff;
            const int32_t u1 = (yuv1 >> 8) & 0xff;
            const int32_t v1 = yuv1 & 0xff;
            const int32_t y2 = (yuv2 >> 16) & 0xff;
            const int32_t u2 = (yuv2 >> 8) & 0xff;
            const int32_t v2 = yuv2 & 0xff;
            return abs(y1 - y2) > 48 || abs(u1 - u2) > 7 || abs(v1 - v2) > 6;
        }

        inline uint32_t Interpolate_2(uint32_t color1, int32_t weight1, uint32_t color2, int32_t weight2, int32_t shift) {
            return (((((color1 & 0xff00ff00) >> 8) * weight1 +
                         ((color2 & 0xff00ff00) >> 8) * weight2)
                        << (8 - shift)) &
                       0xff00ff00) |
                (((((color1 & 0x00ff00ff)) * weight1 +
                      ((color2 & 0x00ff00ff)) * weight2) >>
                     shift) &
                    0x00ff00ff);
        }

        inline uint32_t Interpolate_3(uint32_t color1, int32_t weight1, uint32_t color2, int32_t weight2, uint32_t color3, int32_t weight3, int32_t shift) {
            return (((((color1 & 0xff00ff00) >> 8) * weight1 +
                         ((color2 & 0xff00ff00) >> 8) * weight2 +
                         ((color3 & 0xff00ff00) >> 8) * weight3)
                        << (8 - shift)) &
                       0xff00ff00) |
                (((((color1 & 0x00ff00ff)) * weight1 +
                      ((color2 & 0x00ff00ff)) * weight2 +
                      ((color3 & 0x00ff00ff)) * weight3) >>
                     shift) &
                    0x00ff00ff);
        }

#define P(mask, result) ((shuffled_pattern & (mask)) == (result))
#define DROP4(index) ((index) > 4 ? (index) - 1 : (index))
#define SHF(value, rotate, index) \
    ((((value) >> ((rotate) ? 7 - DROP4(index) : DROP4(index))) & 1) << DROP4(p##index))
#define WDIFF(yuv1, yuv2) YUV_Diff(yuv1, yuv2)
#define INTERP_BOOTSTRAP(rotate)                                                         \
    const int32_t shuffled_pattern = SHF(pattern, rotate, 0) | SHF(pattern, rotate, 1) | \
        SHF(pattern, rotate, 2) | SHF(pattern, rotate, 3) | SHF(pattern, rotate, 5) |    \
        SHF(pattern, rotate, 6) | SHF(pattern, rotate, 7) | SHF(pattern, rotate, 8);     \
    const uint32_t w0 = pixels[p0], w1 = pixels[p1], w3 = pixels[p3],                    \
                   w4 = pixels[p4], w5 = pixels[p5], w7 = pixels[p7];                    \
    const uint32_t y0 = yuv[p0], y1 = yuv[p1], y3 = yuv[p3],                             \
                   y5 = yuv[p5], y7 = yuv[p7]

        inline uint32_t HQ2x_Interpolate(int32_t pattern, const uint32_t* pixels, const uint32_t* yuv, int32_t p0, int32_t p1, int32_t p2, int32_t p3, int32_t p4, int32_t p5, int32_t p6, int32_t p7, int32_t p8) {
            INTERP_BOOTSTRAP(0);

            if ((P(0xbf, 0x37) || P(0xdb, 0x13)) && WDIFF(y1, y5))
                return Interpolate_2(w4, 3, w3, 1, 2);
            if ((P(0xdb, 0x49) || P(0xef, 0x6d)) && WDIFF(y7, y3))
                return Interpolate_2(w4, 3, w1, 1, 2);
            if ((P(0x0b, 0x0b) || P(0xfe, 0x4a) || P(0xfe, 0x1a)) && WDIFF(y3, y1))
                return w4;
            if ((P(0x6f, 0x2a) || P(0x5b, 0x0a) || P(0xbf, 0x3a) || P(0xdf, 0x5a) ||
                    P(0x9f, 0x8a) || P(0xcf, 0x8a) || P(0xef, 0x4e) || P(0x3f, 0x0e) ||
                    P(0xfb, 0x5a) || P(0xbb, 0x8a) || P(0x7f, 0x5a) || P(0xaf, 0x8a) ||
                    P(0xeb, 0x8a)) &&
                WDIFF(y3, y1))
                return Interpolate_2(w4, 3, w0, 1, 2);
            if (P(0x0b, 0x08))
                return Interpolate_3(w4, 2, w0, 1, w1, 1, 2);
            if (P(0x0b, 0x02))
                return Interpolate_3(w4, 2, w0, 1, w3, 1, 2);
            if (P(0x2f, 0x2f))
                return Interpolate_3(w4, 14, w3, 1, w1, 1, 4);
            if (P(0xbf, 0x37) || P(0xdb, 0x13))
                return Interpolate_3(w4, 5, w1, 2, w3, 1, 3);
            if (P(0xdb, 0x49) || P(0xef, 0x6d))
                return Interpolate_3(w4, 5, w3, 2, w1, 1, 3);
            if (P(0x1b, 0x03) || P(0x4f, 0x43) || P(0x8b, 0x83) || P(0x6b, 0x43))
                return Interpolate_2(w4, 3, w3, 1, 2);
            if (P(0x4b, 0x09) || P(0x8b, 0x89) || P(0x1f, 0x19) || P(0x3b, 0x19))
                return Interpolate_2(w4, 3, w1, 1, 2);
            if (P(0x7e, 0x2a) || P(0xef, 0xab) || P(0xbf, 0x8f) || P(0x7e, 0x0e))
                return Interpolate_3(w4, 2, w3, 3, w1, 3, 3);
            if (P(0xfb, 0x6a) || P(0x6f, 0x6e) || P(0x3f, 0x3e) || P(0xfb, 0xfa) ||
                P(0xdf, 0xde) || P(0xdf, 0x1e))
                return Interpolate_2(w4, 3, w0, 1, 2);
            if (P(0x0a, 0x00) || P(0x4f, 0x4b) || P(0x9f, 0x1b) || P(0x2f, 0x0b) ||
                P(0xbe, 0x0a) || P(0xee, 0x0a) || P(0x7e, 0x0a) || P(0xeb, 0x4b) ||
                P(0x3b, 0x1b))
                return Interpolate_3(w4, 2, w3, 1, w1, 1, 2);
            return Interpolate_3(w4, 6, w3, 1, w1, 1, 3);
        }

        inline void HQ3x_Interpolate(uint32_t* destination, int32_t destination_stride, int32_t pattern, const uint32_t* pixels, const uint32_t* yuv, int32_t pos00, int32_t pos01, int32_t p0, int32_t p1, int32_t p2, int32_t p3, int32_t p4, int32_t p5, int32_t p6, int32_t p7, int32_t p8, int32_t rotate) {
            INTERP_BOOTSTRAP(rotate);

            uint32_t* dst00 = &destination[destination_stride * (pos00 >> 1) + (pos00 & 1)];
            uint32_t* dst01 = &destination[destination_stride * (pos01 >> 1) + (pos01 & 1)];

            if ((P(0xdb, 0x49) || P(0xef, 0x6d)) && WDIFF(y7, y3))
                *dst00 = Interpolate_2(w4, 3, w1, 1, 2);
            else if ((P(0xbf, 0x37) || P(0xdb, 0x13)) && WDIFF(y1, y5))
                *dst00 = Interpolate_2(w4, 3, w3, 1, 2);
            else if ((P(0x0b, 0x0b) || P(0xfe, 0x4a) || P(0xfe, 0x1a)) && WDIFF(y3, y1))
                *dst00 = w4;
            else if ((P(0x6f, 0x2a) || P(0x5b, 0x0a) || P(0xbf, 0x3a) || P(0xdf, 0x5a) ||
                         P(0x9f, 0x8a) || P(0xcf, 0x8a) || P(0xef, 0x4e) || P(0x3f, 0x0e) ||
                         P(0xfb, 0x5a) || P(0xbb, 0x8a) || P(0x7f, 0x5a) || P(0xaf, 0x8a) ||
                         P(0xeb, 0x8a)) &&
                WDIFF(y3, y1))
                *dst00 = Interpolate_2(w4, 3, w0, 1, 2);
            else if (P(0x4b, 0x09) || P(0x8b, 0x89) || P(0x1f, 0x19) || P(0x3b, 0x19))
                *dst00 = Interpolate_2(w4, 3, w1, 1, 2);
            else if (P(0x1b, 0x03) || P(0x4f, 0x43) || P(0x8b, 0x83) || P(0x6b, 0x43))
                *dst00 = Interpolate_2(w4, 3, w3, 1, 2);
            else if (P(0x7e, 0x2a) || P(0xef, 0xab) || P(0xbf, 0x8f) || P(0x7e, 0x0e))
                *dst00 = Interpolate_2(w3, 1, w1, 1, 1);
            else if (P(0x4f, 0x4b) || P(0x9f, 0x1b) || P(0x2f, 0x0b) || P(0xbe, 0x0a) ||
                P(0xee, 0x0a) || P(0x7e, 0x0a) || P(0xeb, 0x4b) || P(0x3b, 0x1b))
                *dst00 = Interpolate_3(w4, 2, w3, 7, w1, 7, 4);
            else if (P(0x0b, 0x08) || P(0xf9, 0x68) || P(0xf3, 0x62) || P(0x6d, 0x6c) ||
                P(0x67, 0x66) || P(0x3d, 0x3c) || P(0x37, 0x36) || P(0xf9, 0xf8) ||
                P(0xdd, 0xdc) || P(0xf3, 0xf2) || P(0xd7, 0xd6) || P(0xdd, 0x1c) ||
                P(0xd7, 0x16) || P(0x0b, 0x02))
                *dst00 = Interpolate_2(w4, 3, w0, 1, 2);
            else
                *dst00 = Interpolate_3(w4, 2, w3, 1, w1, 1, 2);

            if ((P(0xfe, 0xde) || P(0x9e, 0x16) || P(0xda, 0x12) || P(0x17, 0x16) ||
                    P(0x5b, 0x12) || P(0xbb, 0x12)) &&
                WDIFF(y1, y5))
                *dst01 = w4;
            else if ((P(0x0f, 0x0b) || P(0x5e, 0x0a) || P(0xfb, 0x7b) || P(0x3b, 0x0b) ||
                         P(0xbe, 0x0a) || P(0x7a, 0x0a)) &&
                WDIFF(y3, y1))
                *dst01 = w4;
            else if (P(0xbf, 0x8f) || P(0x7e, 0x0e) || P(0xbf, 0x37) || P(0xdb, 0x13))
                *dst01 = Interpolate_2(w1, 3, w4, 1, 2);
            else if (P(0x02, 0x00) || P(0x7c, 0x28) || P(0xed, 0xa9) || P(0xf5, 0xb4) ||
                P(0xd9, 0x90))
                *dst01 = Interpolate_2(w4, 3, w1, 1, 2);
            else if (P(0x4f, 0x4b) || P(0xfb, 0x7b) || P(0xfe, 0x7e) || P(0x9f, 0x1b) ||
                P(0x2f, 0x0b) || P(0xbe, 0x0a) || P(0x7e, 0x0a) || P(0xfb, 0x4b) ||
                P(0xfb, 0xdb) || P(0xfe, 0xde) || P(0xfe, 0x56) || P(0x57, 0x56) ||
                P(0x97, 0x16) || P(0x3f, 0x1e) || P(0xdb, 0x12) || P(0xbb, 0x12))
                *dst01 = Interpolate_2(w4, 7, w1, 1, 3);
            else
                *dst01 = w4;
        }

        inline void HQ4x_Interpolate(uint32_t* destination, int32_t destination_stride, int32_t pattern, const uint32_t* pixels, const uint32_t* yuv, int32_t pos00, int32_t pos01, int32_t pos10, int32_t pos11, int32_t p0, int32_t p1, int32_t p2, int32_t p3, int32_t p4, int32_t p5, int32_t p6, int32_t p7, int32_t p8) {
            INTERP_BOOTSTRAP(0);

            uint32_t* dst00 = &destination[destination_stride * (pos00 >> 1) + (pos00 & 1)];
            uint32_t* dst01 = &destination[destination_stride * (pos01 >> 1) + (pos01 & 1)];
            uint32_t* dst10 = &destination[destination_stride * (pos10 >> 1) + (pos10 & 1)];
            uint32_t* dst11 = &destination[destination_stride * (pos11 >> 1) + (pos11 & 1)];

            const bool cond00 = (P(0xbf, 0x37) || P(0xdb, 0x13)) && WDIFF(y1, y5);
            const bool cond01 = (P(0xdb, 0x49) || P(0xef, 0x6d)) && WDIFF(y7, y3);
            const bool cond02 = (P(0x6f, 0x2a) || P(0x5b, 0x0a) || P(0xbf, 0x3a) ||
                                    P(0xdf, 0x5a) || P(0x9f, 0x8a) || P(0xcf, 0x8a) ||
                                    P(0xef, 0x4e) || P(0x3f, 0x0e) || P(0xfb, 0x5a) ||
                                    P(0xbb, 0x8a) || P(0x7f, 0x5a) || P(0xaf, 0x8a) ||
                                    P(0xeb, 0x8a)) &&
                WDIFF(y3, y1);
            const bool cond03 = P(0xdb, 0x49) || P(0xef, 0x6d);
            const bool cond04 = P(0xbf, 0x37) || P(0xdb, 0x13);
            const bool cond05 = P(0x1b, 0x03) || P(0x4f, 0x43) || P(0x8b, 0x83) || P(0x6b, 0x43);
            const bool cond06 = P(0x4b, 0x09) || P(0x8b, 0x89) || P(0x1f, 0x19) || P(0x3b, 0x19);
            const bool cond07 = P(0x0b, 0x08) || P(0xf9, 0x68) || P(0xf3, 0x62) ||
                P(0x6d, 0x6c) || P(0x67, 0x66) || P(0x3d, 0x3c) ||
                P(0x37, 0x36) || P(0xf9, 0xf8) || P(0xdd, 0xdc) ||
                P(0xf3, 0xf2) || P(0xd7, 0xd6) || P(0xdd, 0x1c) ||
                P(0xd7, 0x16) || P(0x0b, 0x02);
            const bool cond08 = (P(0x0f, 0x0b) || P(0x2b, 0x0b) || P(0xfe, 0x4a) ||
                                    P(0xfe, 0x1a)) &&
                WDIFF(y3, y1);
            const bool cond09 = P(0x2f, 0x2f);
            const bool cond10 = P(0x0a, 0x00);
            const bool cond11 = P(0x0b, 0x09);
            const bool cond12 = P(0x7e, 0x2a) || P(0xef, 0xab);
            const bool cond13 = P(0xbf, 0x8f) || P(0x7e, 0x0e);
            const bool cond14 = P(0x4f, 0x4b) || P(0x9f, 0x1b) || P(0x2f, 0x0b) ||
                P(0xbe, 0x0a) || P(0xee, 0x0a) || P(0x7e, 0x0a) ||
                P(0xeb, 0x4b) || P(0x3b, 0x1b);
            const bool cond15 = P(0x0b, 0x03);

            if (cond00)
                *dst00 = Interpolate_2(w4, 5, w3, 3, 3);
            else if (cond01)
                *dst00 = Interpolate_2(w4, 5, w1, 3, 3);
            else if ((P(0x0b, 0x0b) || P(0xfe, 0x4a) || P(0xfe, 0x1a)) && WDIFF(y3, y1))
                *dst00 = w4;
            else if (cond02)
                *dst00 = Interpolate_2(w4, 5, w0, 3, 3);
            else if (cond03)
                *dst00 = Interpolate_2(w4, 3, w3, 1, 2);
            else if (cond04)
                *dst00 = Interpolate_2(w4, 3, w1, 1, 2);
            else if (cond05)
                *dst00 = Interpolate_2(w4, 5, w3, 3, 3);
            else if (cond06)
                *dst00 = Interpolate_2(w4, 5, w1, 3, 3);
            else if (P(0x0f, 0x0b) || P(0x5e, 0x0a) || P(0x2b, 0x0b) ||
                P(0xbe, 0x0a) || P(0x7a, 0x0a) || P(0xee, 0x0a))
                *dst00 = Interpolate_2(w1, 1, w3, 1, 1);
            else if (cond07)
                *dst00 = Interpolate_2(w4, 5, w0, 3, 3);
            else
                *dst00 = Interpolate_3(w4, 2, w1, 1, w3, 1, 2);

            if (cond00)
                *dst01 = Interpolate_2(w4, 7, w3, 1, 3);
            else if (cond08)
                *dst01 = w4;
            else if (cond02)
                *dst01 = Interpolate_2(w4, 3, w0, 1, 2);
            else if (cond09)
                *dst01 = w4;
            else if (cond10)
                *dst01 = Interpolate_3(w4, 5, w1, 2, w3, 1, 3);
            else if (P(0x0b, 0x08))
                *dst01 = Interpolate_3(w4, 5, w1, 2, w0, 1, 3);
            else if (cond11)
                *dst01 = Interpolate_2(w4, 5, w1, 3, 3);
            else if (cond04)
                *dst01 = Interpolate_2(w1, 3, w4, 1, 2);
            else if (cond12)
                *dst01 = Interpolate_3(w1, 2, w4, 1, w3, 1, 2);
            else if (cond13)
                *dst01 = Interpolate_2(w1, 5, w3, 3, 3);
            else if (cond05)
                *dst01 = Interpolate_2(w4, 7, w3, 1, 3);
            else if (P(0xf3, 0x62) || P(0x67, 0x66) || P(0x37, 0x36) ||
                P(0xf3, 0xf2) || P(0xd7, 0xd6) || P(0xd7, 0x16) || P(0x0b, 0x02))
                *dst01 = Interpolate_2(w4, 3, w0, 1, 2);
            else if (cond14)
                *dst01 = Interpolate_2(w1, 1, w4, 1, 1);
            else
                *dst01 = Interpolate_2(w4, 3, w1, 1, 2);

            if (cond01)
                *dst10 = Interpolate_2(w4, 7, w1, 1, 3);
            else if (cond08)
                *dst10 = w4;
            else if (cond02)
                *dst10 = Interpolate_2(w4, 3, w0, 1, 2);
            else if (cond09)
                *dst10 = w4;
            else if (cond10)
                *dst10 = Interpolate_3(w4, 5, w3, 2, w1, 1, 3);
            else if (P(0x0b, 0x02))
                *dst10 = Interpolate_3(w4, 5, w3, 2, w0, 1, 3);
            else if (cond15)
                *dst10 = Interpolate_2(w4, 5, w3, 3, 3);
            else if (cond03)
                *dst10 = Interpolate_2(w3, 3, w4, 1, 2);
            else if (cond13)
                *dst10 = Interpolate_3(w3, 2, w4, 1, w1, 1, 2);
            else if (cond12)
                *dst10 = Interpolate_2(w3, 5, w1, 3, 3);
            else if (cond06)
                *dst10 = Interpolate_2(w4, 7, w1, 1, 3);
            else if (P(0x0b, 0x08) || P(0xf9, 0x68) || P(0x6d, 0x6c) ||
                P(0x3d, 0x3c) || P(0xf9, 0xf8) || P(0xdd, 0xdc) || P(0xdd, 0x1c))
                *dst10 = Interpolate_2(w4, 3, w0, 1, 2);
            else if (cond14)
                *dst10 = Interpolate_2(w3, 1, w4, 1, 1);
            else
                *dst10 = Interpolate_2(w4, 3, w3, 1, 2);

            if ((P(0x7f, 0x2b) || P(0xef, 0xab) || P(0xbf, 0x8f) || P(0x7f, 0x0f)) &&
                WDIFF(y3, y1))
                *dst11 = w4;
            else if (cond02)
                *dst11 = Interpolate_2(w4, 7, w0, 1, 3);
            else if (cond15)
                *dst11 = Interpolate_2(w4, 7, w3, 1, 3);
            else if (cond11)
                *dst11 = Interpolate_2(w4, 7, w1, 1, 3);
            else if (P(0x0a, 0x00) || P(0x7e, 0x2a) || P(0xef, 0xab) ||
                P(0xbf, 0x8f) || P(0x7e, 0x0e))
                *dst11 = Interpolate_3(w4, 6, w3, 1, w1, 1, 3);
            else if (cond07)
                *dst11 = Interpolate_2(w4, 7, w0, 1, 3);
            else
                *dst11 = w4;
        }

        int32_t Difference_Pattern(const uint32_t* pixels, const uint32_t* yuv) {
            const uint32_t center_yuv = yuv[4];
            int32_t pattern = 0;
            int32_t output_bit = 0;
            for (int32_t index = 0; index < 9; ++index) {
                if (index == 4) {
                    continue;
                }
                if (pixels[4] != pixels[index] &&
                    YUV_Diff(center_yuv, yuv[index])) {
                    pattern |= 1 << output_bit;
                }
                ++output_bit;
            }
            return pattern;
        }

        void Convert_YUV_Row(const uint32_t* source, int32_t width, uint32_t* yuv) {
            for (int32_t x = 0; x < width; ++x) {
                yuv[x] = Lookup_YUV(source[x]);
            }
        }

        template<int32_t Factor>
        bool Scale_Region_Impl(
            const uint32_t* source,
            int32_t source_width,
            int32_t source_height,
            int32_t source_stride,
            uint32_t* destination,
            int32_t destination_stride,
            const scalers::s_region* region) {
            uint32_t* previous_yuv = g_yuv_rows;
            uint32_t* current_yuv = g_yuv_rows + source_width;
            uint32_t* next_yuv = g_yuv_rows + source_width * 2;

            const int32_t first_y = region->y;
            const int32_t previous_y = first_y > 0 ? first_y - 1 : first_y;
            const int32_t next_y = first_y < source_height - 1 ? first_y + 1 : first_y;
            Convert_YUV_Row(source + previous_y * source_stride, source_width, previous_yuv);
            Convert_YUV_Row(source + first_y * source_stride, source_width, current_yuv);
            Convert_YUV_Row(source + next_y * source_stride, source_width, next_yuv);

            for (int32_t y = first_y; y < region->y + region->height; ++y) {
                const int32_t source_previous_y = y > 0 ? y - 1 : y;
                const int32_t source_next_y = y < source_height - 1 ? y + 1 : y;
                const uint32_t* previous_row = source + source_previous_y * source_stride;
                const uint32_t* current_row = source + y * source_stride;
                const uint32_t* next_row = source + source_next_y * source_stride;
                const int32_t first_x = region->x;
                const int32_t previous_x = first_x > 0 ? first_x - 1 : first_x;
                const int32_t next_x = first_x < source_width - 1 ? first_x + 1 : first_x;
                uint32_t pixels[9] = {
                    previous_row[previous_x], previous_row[first_x], previous_row[next_x],
                    current_row[previous_x], current_row[first_x], current_row[next_x],
                    next_row[previous_x], next_row[first_x], next_row[next_x],
                };
                uint32_t yuv[9] = {
                    previous_yuv[previous_x], previous_yuv[first_x], previous_yuv[next_x],
                    current_yuv[previous_x], current_yuv[first_x], current_yuv[next_x],
                    next_yuv[previous_x], next_yuv[first_x], next_yuv[next_x],
                };

                for (int32_t x = first_x; x < region->x + region->width; ++x) {
                    const int32_t pattern = Difference_Pattern(pixels, yuv);
                    uint32_t* output = destination + y * Factor * destination_stride + x * Factor;

                    if constexpr (Factor == 2) {
                        output[0] = HQ2x_Interpolate(pattern, pixels, yuv, 0, 1, 2, 3, 4, 5, 6, 7, 8);
                        output[1] = HQ2x_Interpolate(pattern, pixels, yuv, 2, 1, 0, 5, 4, 3, 8, 7, 6);
                        output[destination_stride] = HQ2x_Interpolate(pattern, pixels, yuv, 6, 7, 8, 3, 4, 5, 0, 1, 2);
                        output[destination_stride + 1] = HQ2x_Interpolate(pattern, pixels, yuv, 8, 7, 6, 5, 4, 3, 2, 1, 0);
                    } else if constexpr (Factor == 3) {
                        HQ3x_Interpolate(output, destination_stride, pattern, pixels, yuv, 0, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0);
                        HQ3x_Interpolate(output + 1, destination_stride, pattern, pixels, yuv, 1, 3, 2, 5, 8, 1, 4, 7, 0, 3, 6, 1);
                        HQ3x_Interpolate(output + destination_stride, destination_stride, pattern, pixels, yuv, 2, 0, 6, 3, 0, 7, 4, 1, 8, 5, 2, 1);
                        HQ3x_Interpolate(output + destination_stride + 1, destination_stride, pattern, pixels, yuv, 3, 2, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0);
                        output[destination_stride + 1] = pixels[4];
                    } else {
                        HQ4x_Interpolate(output, destination_stride, pattern, pixels, yuv, 0, 1, 2, 3, 0, 1, 2, 3, 4, 5, 6, 7, 8);
                        HQ4x_Interpolate(output + 2, destination_stride, pattern, pixels, yuv, 1, 0, 3, 2, 2, 1, 0, 5, 4, 3, 8, 7, 6);
                        HQ4x_Interpolate(output + 2 * destination_stride, destination_stride, pattern, pixels, yuv, 2, 3, 0, 1, 6, 7, 8, 3, 4, 5, 0, 1, 2);
                        HQ4x_Interpolate(output + 2 * destination_stride + 2, destination_stride, pattern, pixels, yuv, 3, 2, 1, 0, 8, 7, 6, 5, 4, 3, 2, 1, 0);
                    }

                    const int32_t new_x = x < source_width - 2 ? x + 2 : source_width - 1;
                    pixels[0] = pixels[1]; pixels[1] = pixels[2]; pixels[2] = previous_row[new_x];
                    pixels[3] = pixels[4]; pixels[4] = pixels[5]; pixels[5] = current_row[new_x];
                    pixels[6] = pixels[7]; pixels[7] = pixels[8]; pixels[8] = next_row[new_x];
                    yuv[0] = yuv[1]; yuv[1] = yuv[2]; yuv[2] = previous_yuv[new_x];
                    yuv[3] = yuv[4]; yuv[4] = yuv[5]; yuv[5] = current_yuv[new_x];
                    yuv[6] = yuv[7]; yuv[7] = yuv[8]; yuv[8] = next_yuv[new_x];
                }

                if (y + 1 < region->y + region->height) {
                    uint32_t* recycled_yuv = previous_yuv;
                    previous_yuv = current_yuv;
                    current_yuv = next_yuv;
                    next_yuv = recycled_yuv;
                    const int32_t new_y = y < source_height - 2 ? y + 2 : source_height - 1;
                    Convert_YUV_Row(source + new_y * source_stride, source_width, next_yuv);
                }
            }

            return true;
        }
    } // namespace

    bool Initialize() {
        if (g_initialized) {
            return true;
        }

        for (uint32_t color = 0; color < 65536; ++color) {
            const int32_t red5 = (color >> 11) & 0x1f;
            const int32_t green6 = (color >> 5) & 0x3f;
            const int32_t blue5 = color & 0x1f;
            const int32_t red = (red5 << 3) | (red5 >> 2);
            const int32_t green = (green6 << 2) | (green6 >> 4);
            const int32_t blue = (blue5 << 3) | (blue5 >> 2);
            g_rgb565_to_yuv[color] = RGB_To_YUV(red, green, blue);
        }
        g_initialized = true;
        return true;
    }

    bool Scale(const uint32_t* source, int32_t source_width, int32_t source_height, int32_t source_stride, uint32_t* destination, int32_t destination_stride, int32_t factor) {
        const scalers::s_region region = {0, 0, source_width, source_height};
        return Scale_Region(
            source,
            source_width,
            source_height,
            source_stride,
            destination,
            destination_stride,
            factor,
            &region);
    }

    bool Scale_Region(const uint32_t* source, int32_t source_width, int32_t source_height, int32_t source_stride, uint32_t* destination, int32_t destination_stride, int32_t factor, const scalers::s_region* region) {
        if (!g_initialized || source == nullptr || destination == nullptr || source_width <= 0 ||
            source_height <= 0 || factor < 2 || factor > 4 ||
            source_width > scalers::MAX_SOURCE_WIDTH ||
            source_width > INT32_MAX / factor || source_height > INT32_MAX / factor ||
            source_stride < source_width || destination_stride < source_width * factor ||
            source_stride > INT32_MAX / source_height ||
            destination_stride > INT32_MAX / (source_height * factor) ||
            region == nullptr || region->x < 0 || region->y < 0 ||
            region->width <= 0 || region->height <= 0 ||
            region->x > source_width - region->width ||
            region->y > source_height - region->height) {
            return false;
        }

        switch (factor) {
            case 2:
                return Scale_Region_Impl<2>(source, source_width, source_height, source_stride, destination, destination_stride, region);
            case 3:
                return Scale_Region_Impl<3>(source, source_width, source_height, source_stride, destination, destination_stride, region);
            case 4:
                return Scale_Region_Impl<4>(source, source_width, source_height, source_stride, destination, destination_stride, region);
            default:
                return false;
        }
    }

#undef INTERP_BOOTSTRAP
#undef WDIFF
#undef SHF
#undef DROP4
#undef P
} // namespace hqx