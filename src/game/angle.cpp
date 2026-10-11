#include "pch.h"

namespace angle {

    uint8_t atan_lookup_table[128] = {
        0,
        0,
        1,
        1,
        2,
        2,
        3,
        3,
        4,
        4,
        4,
        5,
        5,
        6,
        6,
        7,
        7,
        8,
        8,
        8,
        9,
        9,
        10,
        10,
        11,
        11,
        11,
        12,
        12,
        13,
        13,
        14,
        14,
        14,
        15,
        15,
        16,
        16,
        16,
        17,
        17,
        18,
        18,
        18,
        19,
        19,
        20,
        20,
        20,
        21,
        21,
        22,
        22,
        22,
        23,
        23,
        24,
        24,
        24,
        25,
        25,
        25,
        26,
        26,
        26,
        27,
        27,
        27,
        28,
        28,
        29,
        29,
        29,
        30,
        30,
        30,
        31,
        31,
        31,
        32,
        32,
        32,
        32,
        33,
        33,
        33,
        34,
        34,
        34,
        35,
        35,
        35,
        36,
        36,
        36,
        36,
        37,
        37,
        37,
        38,
        38,
        38,
        38,
        39,
        39,
        39,
        39,
        40,
        40,
        40,
        40,
        41,
        41,
        41,
        41,
        42,
        42,
        42,
        42,
        43,
        43,
        43,
        43,
        44,
        44,
        44,
        44,
        45,
    };

    uint16_t sin_table[90] = {
        0,
        1144,
        2287,
        3430,
        4572,
        5712,
        6850,
        7987,
        9121,
        10252,
        11380,
        12505,
        13626,
        14742,
        15855,
        16962,
        18064,
        19161,
        20252,
        21336,
        22415,
        23486,
        24550,
        25607,
        26656,
        27697,
        28729,
        29753,
        30767,
        31772,
        32768,
        33754,
        34729,
        35693,
        36647,
        37590,
        38521,
        39441,
        40348,
        41243,
        42126,
        42995,
        43852,
        44695,
        45525,
        46341,
        47143,
        47930,
        48703,
        49461,
        50203,
        50931,
        51643,
        52339,
        53020,
        53684,
        54332,
        54963,
        55578,
        56175,
        56756,
        57319,
        57865,
        58393,
        58903,
        59396,
        59870,
        60326,
        60764,
        61183,
        61584,
        61966,
        62328,
        62672,
        62997,
        63303,
        63589,
        63856,
        64104,
        64332,
        64540,
        64729,
        64898,
        65048,
        65177,
        65287,
        65376,
        65446,
        65496,
        65526,
    };

    int16_t __cdecl Get_Angle(int32_t dx, int32_t dy) {
        if (dx == 0) {
            if (dy < 0) return 270;
            if (dy > 0) return 90;
            return 0;
        }

        if (dy == 0) {
            if (dx < 0) return 180;
            return 0;
        }

        uint32_t normalized_x;
        uint32_t normalized_y;
        int16_t quadrant_offset;

        if (dx < 0) {
            if (dy < 0) {
                normalized_x = 0u - (uint32_t)dx;
                normalized_y = 0u - (uint32_t)dy;
                quadrant_offset = 180;
            } else {
                normalized_x = dy;
                normalized_y = 0u - (uint32_t)dx;
                quadrant_offset = 90;
            }
        } else {
            if (dy > 0) {
                normalized_x = dx;
                normalized_y = dy;
                quadrant_offset = 0;
            } else {
                normalized_x = 0u - (uint32_t)dy;
                normalized_y = dx;
                quadrant_offset = 270;
            }
        }

        int16_t angle;
        if (normalized_y == normalized_x) {
            angle = 45;
        } else if (normalized_y > normalized_x) {
            angle = 90 - atan_lookup_table[(normalized_x << 7) / normalized_y];
        } else {
            angle = atan_lookup_table[(normalized_y << 7) / normalized_x];
        }

        angle += quadrant_offset;
        if (angle == 360) {
            angle = 0;
        }

        return angle;
    }

    int32_t __cdecl Cos(int16_t angle, int32_t magnitude) {
        return Sin(angle + 90, magnitude);
    }

    int32_t __cdecl Sin(int16_t angle, int32_t magnitude) {
        int32_t angle_deg = angle;

        if (magnitude < 0) {
            if (magnitude != INT32_MIN) {
                magnitude = -magnitude;
            }

            uint16_t angle_bits = (uint16_t)angle;
            angle_bits = (uint16_t)(angle_bits + 180u);
            if (angle_bits < 0x8000u) {
                angle_deg = angle_bits;
            } else {
                angle_deg = (int32_t)angle_bits - 0x10000;
            }
        }

        if (angle_deg >= 360) {
            do {
                angle_deg -= 360;
            } while (angle_deg >= 360);
        } else {
            while (angle_deg < 0) {
                angle_deg += 360;
            }
        }

        if (angle_deg < 180) {
            if (angle_deg < 90) {
                return (int32_t)(((uint32_t)(uint16_t)sin_table[angle_deg] * (uint32_t)magnitude + 0x8000) >> 16);
            }
            if (angle_deg == 90) {
                return magnitude;
            }
            return (int32_t)(((uint32_t)(uint16_t)sin_table[180 - angle_deg] * (uint32_t)magnitude + 0x8000) >> 16);
        }

        if (angle_deg < 270) {
            return -(int32_t)(((uint32_t)(uint16_t)sin_table[angle_deg - 180] * (uint32_t)magnitude + 0x8000) >> 16);
        }
        if (angle_deg == 270) {
            if (magnitude == INT32_MIN) {
                return INT32_MIN;
            }
            return -magnitude;
        }

        return -(int32_t)(((uint32_t)(uint16_t)sin_table[360 - angle_deg] * (uint32_t)magnitude + 0x8000) >> 16);
    }

    void __cdecl Rotate_Coordinates(int16_t angle_deg, int32_t* x_ptr, int32_t* y_ptr) {
        const auto mul_lo32 = [](uint16_t a, int32_t b) {
            return (int32_t)((uint32_t)a * (uint32_t)b);
        };
        const auto neg_lo32 = [](int32_t value) {
            return (int32_t)(0u - (uint32_t)value);
        };
        const auto add_lo32 = [](int32_t a, int32_t b) {
            return (int32_t)((uint32_t)a + (uint32_t)b);
        };
        const auto sub_lo32 = [](int32_t a, int32_t b) {
            return (int32_t)((uint32_t)a - (uint32_t)b);
        };
        const auto round_shift_16 = [&](int32_t value) {
            if (value < 0) {
                return neg_lo32((int32_t)((0x8000u - (uint32_t)value) >> 16));
            }
            return (int32_t)(((uint32_t)value + 0x8000u) >> 16);
        };

        bool skip_calc = false;

        if (angle_deg >= 360) {
            do {
                angle_deg = (int16_t)(angle_deg - 360);
            } while (angle_deg >= 360);
        } else if (angle_deg < 0) {
            do {
                angle_deg = (int16_t)(angle_deg + 360);
            } while (angle_deg < 0);
        }

        int32_t x = *x_ptr;
        int32_t y = *y_ptr;
        int32_t y_sin = 0;
        int32_t x_cos = 0;
        int32_t x_sin = 0;
        int32_t y_cos = 0;

        if (angle_deg < 180) {
            if (angle_deg < 90) {
                if (angle_deg == 0) {
                    skip_calc = true;
                } else {
                    uint16_t sin_val = sin_table[angle_deg];
                    uint16_t cos_val = sin_table[90 - angle_deg];

                    y_sin = mul_lo32(sin_val, y);
                    x_cos = mul_lo32(cos_val, x);
                    x_sin = mul_lo32(sin_val, x);
                    y_cos = mul_lo32(cos_val, y);
                }
            } else if (angle_deg == 90) {
                *y_ptr = x;
                *x_ptr = neg_lo32(y);
                skip_calc = true;
            } else {
                uint16_t sin_val = sin_table[180 - angle_deg];
                uint16_t cos_val_abs = sin_table[angle_deg - 90];

                y_sin = mul_lo32(sin_val, y);
                x_cos = neg_lo32(mul_lo32(cos_val_abs, x));
                x_sin = mul_lo32(sin_val, x);
                y_cos = neg_lo32(mul_lo32(cos_val_abs, y));
            }
        } else if (angle_deg < 270) {
            if (angle_deg == 180) {
                *x_ptr = neg_lo32(x);
                *y_ptr = neg_lo32(y);
                skip_calc = true;
            } else {
                uint16_t sin_val_abs = sin_table[angle_deg - 180];
                uint16_t cos_val_abs = sin_table[270 - angle_deg];

                y_sin = neg_lo32(mul_lo32(sin_val_abs, y));
                x_cos = neg_lo32(mul_lo32(cos_val_abs, x));
                x_sin = neg_lo32(mul_lo32(sin_val_abs, x));
                y_cos = neg_lo32(mul_lo32(cos_val_abs, y));
            }
        } else if (angle_deg == 270) {
            *y_ptr = neg_lo32(x);
            *x_ptr = y;
            skip_calc = true;
        } else {
            uint16_t sin_val_abs = sin_table[360 - angle_deg];
            uint16_t cos_val = sin_table[angle_deg - 270];

            y_sin = neg_lo32(mul_lo32(sin_val_abs, y));
            x_cos = mul_lo32(cos_val, x);
            x_sin = neg_lo32(mul_lo32(sin_val_abs, x));
            y_cos = mul_lo32(cos_val, y);
        }

        if (!skip_calc) {
            int32_t x_prime = sub_lo32(x_cos, y_sin);
            int32_t y_prime = add_lo32(x_sin, y_cos);

            *x_ptr = round_shift_16(x_prime);
            *y_ptr = round_shift_16(y_prime);
        }
    }

} // namespace angle