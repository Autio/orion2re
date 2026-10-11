#include "pch.h"

#include "game/scalers/mmpx.h"

/*
 * Copyright 2020 Morgan McGuire & Mara Gagiu.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

// Adapted from https://github.com/ITotalJustice/mmpx. The packed-buffer entry
// point was replaced with a validated, stride-aware region API.
namespace mmpx {
    namespace {
        inline uint32_t Luma(uint32_t color) {
            const uint32_t alpha = color >> 24;
            return (((color >> 16) & 0xff) + ((color >> 8) & 0xff) +
                       (color & 0xff) + 1) *
                (256 - alpha);
        }

        inline bool All_Equal_2(uint32_t color, uint32_t first, uint32_t second) {
            return ((color ^ first) | (color ^ second)) == 0;
        }

        inline bool All_Equal_3(uint32_t color, uint32_t first, uint32_t second, uint32_t third) {
            return ((color ^ first) | (color ^ second) | (color ^ third)) == 0;
        }

        inline bool All_Equal_4(uint32_t color, uint32_t first, uint32_t second, uint32_t third, uint32_t fourth) {
            return ((color ^ first) | (color ^ second) | (color ^ third) |
                       (color ^ fourth)) == 0;
        }

        inline bool Any_Equal_3(uint32_t color, uint32_t first, uint32_t second, uint32_t third) {
            return color == first || color == second || color == third;
        }

        inline bool None_Equal_2(uint32_t color, uint32_t first, uint32_t second) {
            return color != first && color != second;
        }

        inline bool None_Equal_4(uint32_t color, uint32_t first, uint32_t second, uint32_t third, uint32_t fourth) {
            return color != first && color != second && color != third && color != fourth;
        }

        inline int32_t Clamp(int32_t value, int32_t maximum) {
            if (value < 0) {
                return 0;
            }
            return value > maximum ? maximum : value;
        }

        inline const uint32_t* Row(
            const uint32_t* source,
            int32_t source_height,
            int32_t source_stride,
            int32_t y) {
            return source + Clamp(y, source_height - 1) * source_stride;
        }

        inline uint32_t Row_Pixel(const uint32_t* row, int32_t source_width, int32_t x) {
            return row[Clamp(x, source_width - 1)];
        }
    } // namespace

    bool Scale(const uint32_t* source, int32_t source_width, int32_t source_height, int32_t source_stride, uint32_t* destination, int32_t destination_stride) {
        const scalers::s_region region = {0, 0, source_width, source_height};
        return Scale_Region(
            source,
            source_width,
            source_height,
            source_stride,
            destination,
            destination_stride,
            &region);
    }

    bool Scale_Region(const uint32_t* source, int32_t source_width, int32_t source_height, int32_t source_stride, uint32_t* destination, int32_t destination_stride, const scalers::s_region* region) {
        if (source == nullptr || destination == nullptr || source_width <= 0 ||
            source_height <= 0 || source_width > INT32_MAX / 2 ||
            source_height > INT32_MAX / 2 || source_stride < source_width ||
            destination_stride < source_width * 2 || region == nullptr ||
            source_stride > INT32_MAX / source_height ||
            destination_stride > INT32_MAX / (source_height * 2) ||
            region->x < 0 || region->y < 0 || region->width <= 0 ||
            region->height <= 0 || region->x > source_width - region->width ||
            region->y > source_height - region->height) {
            return false;
        }

        for (int32_t y = region->y; y < region->y + region->height; ++y) {
            int32_t x = region->x;
            const uint32_t* row_n2 = Row(source, source_height, source_stride, y - 2);
            const uint32_t* row_n1 = Row(source, source_height, source_stride, y - 1);
            const uint32_t* row_n3 = Row(source, source_height, source_stride, y - 3);
            const uint32_t* row = Row(source, source_height, source_stride, y);
            const uint32_t* row_s1 = Row(source, source_height, source_stride, y + 1);
            const uint32_t* row_s2 = Row(source, source_height, source_stride, y + 2);
            const uint32_t* row_s3 = Row(source, source_height, source_stride, y + 3);
            uint32_t* output_top =
                destination + y * 2 * destination_stride + region->x * 2;
            uint32_t* output_bottom = output_top + destination_stride;
            uint32_t a = Row_Pixel(row_n1, source_width, x - 1);
            uint32_t b = Row_Pixel(row_n1, source_width, x);
            uint32_t c = Row_Pixel(row_n1, source_width, x + 1);
            uint32_t d = Row_Pixel(row, source_width, x - 1);
            uint32_t e = Row_Pixel(row, source_width, x);
            uint32_t f = Row_Pixel(row, source_width, x + 1);
            uint32_t g = Row_Pixel(row_s1, source_width, x - 1);
            uint32_t h = Row_Pixel(row_s1, source_width, x);
            uint32_t i = Row_Pixel(row_s1, source_width, x + 1);
            uint32_t q = Row_Pixel(row, source_width, x - 2);
            uint32_t r = Row_Pixel(row, source_width, x + 2);

            for (; x < region->x + region->width; ++x) {
                uint32_t j = e;
                uint32_t k = e;
                uint32_t l = e;
                uint32_t m = e;

                if (((a ^ e) | (b ^ e) | (c ^ e) | (d ^ e) | (f ^ e) |
                        (g ^ e) | (h ^ e) | (i ^ e)) != 0) {
                    const uint32_t p = row_n2[x];
                    const uint32_t s = row_s2[x];
                    const uint32_t bl = Luma(b);
                    const uint32_t dl = Luma(d);
                    const uint32_t el = Luma(e);
                    const uint32_t fl = Luma(f);
                    const uint32_t hl = Luma(h);

                    if (d == b && d != h && d != f && (el >= dl || e == a) &&
                        Any_Equal_3(e, a, c, g) &&
                        (el < dl || a != d || e != p || e != q))
                        j = d;
                    if (b == f && b != d && b != h && (el >= bl || e == c) &&
                        Any_Equal_3(e, a, c, i) &&
                        (el < bl || c != b || e != p || e != r))
                        k = b;
                    if (h == d && h != f && h != b && (el >= hl || e == g) &&
                        Any_Equal_3(e, a, g, i) &&
                        (el < hl || g != h || e != s || e != q))
                        l = h;
                    if (f == h && f != b && f != d && (el >= fl || e == i) &&
                        Any_Equal_3(e, c, g, i) &&
                        (el < fl || i != h || e != r || e != s))
                        m = f;

                    if (e != f && All_Equal_4(e, c, i, d, q) && All_Equal_2(f, b, h) &&
                        f != Row_Pixel(row, source_width, x + 3))
                        k = m = f;
                    if (e != d && All_Equal_4(e, a, g, f, r) && All_Equal_2(d, b, h) &&
                        d != Row_Pixel(row, source_width, x - 3))
                        j = l = d;
                    if (e != h && All_Equal_4(e, g, i, b, p) && All_Equal_2(h, d, f) &&
                        h != row_s3[x])
                        l = m = h;
                    if (e != b && All_Equal_4(e, a, c, h, s) && All_Equal_2(b, d, f) &&
                        b != row_n3[x])
                        j = k = b;
                    if (bl < el && All_Equal_4(e, g, h, i, s) && None_Equal_4(e, a, d, c, f))
                        j = k = b;
                    if (hl < el && All_Equal_4(e, a, b, c, p) && None_Equal_4(e, d, g, i, f))
                        l = m = h;
                    if (fl < el && All_Equal_4(e, a, d, g, q) && None_Equal_4(e, b, c, i, h))
                        k = m = f;
                    if (dl < el && All_Equal_4(e, c, f, i, r) && None_Equal_4(e, b, a, g, h))
                        j = l = d;

                    if (h != b) {
                        if (h != a && h != e && h != c) {
                            if (All_Equal_3(h, g, f, r) && None_Equal_2(h, d, Row_Pixel(row_n1, source_width, x + 2)))
                                l = m;
                            if (All_Equal_3(h, i, d, q) && None_Equal_2(h, f, Row_Pixel(row_n1, source_width, x - 2)))
                                m = l;
                        }
                        if (b != i && b != g && b != e) {
                            if (All_Equal_3(b, a, f, r) && None_Equal_2(b, d, Row_Pixel(row_s1, source_width, x + 2)))
                                j = k;
                            if (All_Equal_3(b, c, d, q) && None_Equal_2(b, f, Row_Pixel(row_s1, source_width, x - 2)))
                                k = j;
                        }
                    }
                    if (f != d) {
                        if (d != i && d != e && d != c) {
                            if (All_Equal_3(d, a, h, s) && None_Equal_2(d, b, Row_Pixel(row_s2, source_width, x + 1)))
                                j = l;
                            if (All_Equal_3(d, g, b, p) && None_Equal_2(d, h, Row_Pixel(row_n2, source_width, x + 1)))
                                l = j;
                        }
                        if (f != e && f != a && f != g) {
                            if (All_Equal_3(f, c, h, s) && None_Equal_2(f, b, Row_Pixel(row_s2, source_width, x - 1)))
                                k = m;
                            if (All_Equal_3(f, i, b, p) && None_Equal_2(f, h, Row_Pixel(row_n2, source_width, x - 1)))
                                m = k;
                        }
                    }
                }

                output_top[0] = j;
                output_top[1] = k;
                output_bottom[0] = l;
                output_bottom[1] = m;
                output_top += 2;
                output_bottom += 2;

                a = b;
                b = c;
                c = Row_Pixel(row_n1, source_width, x + 2);
                q = d;
                d = e;
                e = f;
                f = r;
                r = Row_Pixel(row, source_width, x + 3);
                g = h;
                h = i;
                i = Row_Pixel(row_s1, source_width, x + 2);
            }
        }
        return true;
    }

    bool Scale_4x(const uint32_t* source, int32_t source_width, int32_t source_height, int32_t source_stride, uint32_t* intermediate, int32_t intermediate_stride, uint32_t* destination, int32_t destination_stride) {
        const scalers::s_region region = {0, 0, source_width, source_height};
        return Scale_4x_Region(
            source,
            source_width,
            source_height,
            source_stride,
            intermediate,
            intermediate_stride,
            destination,
            destination_stride,
            &region);
    }

    bool Scale_4x_Region(const uint32_t* source, int32_t source_width, int32_t source_height, int32_t source_stride, uint32_t* intermediate, int32_t intermediate_stride, uint32_t* destination, int32_t destination_stride, const scalers::s_region* region) {
        if (source == nullptr || source_width <= 0 || source_height <= 0 ||
            source_width > INT32_MAX / 4 || source_height > INT32_MAX / 4 ||
            source_stride < source_width ||
            intermediate == nullptr || intermediate_stride < source_width * 2 ||
            destination == nullptr || destination_stride < source_width * 4 ||
            source_stride > INT32_MAX / source_height ||
            intermediate_stride > INT32_MAX / (source_height * 2) ||
            destination_stride > INT32_MAX / (source_height * 4) ||
            region == nullptr || region->x < 0 || region->y < 0 ||
            region->width <= 0 || region->height <= 0 ||
            region->x > source_width - region->width ||
            region->y > source_height - region->height) {
            return false;
        }
        const int32_t first_x = region->x > 2 ? region->x - 2 : 0;
        const int32_t first_y = region->y > 2 ? region->y - 2 : 0;
        const int32_t last_x = region->x + region->width < source_width - 2
            ? region->x + region->width + 2
            : source_width;
        const int32_t last_y = region->y + region->height < source_height - 2
            ? region->y + region->height + 2
            : source_height;
        const scalers::s_region intermediate_source_region = {
            first_x,
            first_y,
            last_x - first_x,
            last_y - first_y,
        };
        if (!Scale_Region(
                source,
                source_width,
                source_height,
                source_stride,
                intermediate,
                intermediate_stride,
                &intermediate_source_region)) {
            return false;
        }

        const scalers::s_region intermediate_region = {
            region->x * 2,
            region->y * 2,
            region->width * 2,
            region->height * 2,
        };
        return Scale_Region(
            intermediate,
            source_width * 2,
            source_height * 2,
            intermediate_stride,
            destination,
            destination_stride,
            &intermediate_region);
    }
} // namespace mmpx