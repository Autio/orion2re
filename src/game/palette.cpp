#include "pch.h"

namespace palette {
    s_palette_entry saved_palette[256];
    int32_t cycle_direction_flag = -1;
    int32_t cycle_step_value;
    int32_t cycle_color_value;

    void __cdecl Save_Palette() {
        struct_::Copy_Structure_Aligned(saved_palette, video::current_palette, sizeof(saved_palette));
    }

    void __cdecl Set_Palette_Changes(int16_t start_index, int16_t end_index) {
        int32_t start = start_index;
        int32_t end = end_index;

        if (start <= end) {
            s_palette_entry* entry = &video::current_palette[start];
            int32_t count = (end - start) + 1;

            do {
                entry->changed = 1;

                entry++;
                count--;
            } while (count != 0);
        }
    }

    void __cdecl Clear_Palette_Changes(int16_t start_index, int16_t end_index) {
        if (start_index <= end_index) {
            int32_t count = end_index - start_index + 1;
            s_palette_entry* entry = &video::current_palette[start_index];

            do {
                entry->changed = 0;
                entry++;
                count--;
            } while (count != 0);
        }
    }

    void __cdecl Reset_Cycle_Palette_Color() {
        cycle_direction_flag = -1;
    }

    void __cdecl Restore_Palette() {
        const int16_t last_palette_index =
            (int16_t)((sizeof(video::current_palette) / sizeof(video::current_palette[0])) - 1);

        struct_::Copy_Structure_Aligned(video::current_palette, saved_palette, sizeof(saved_palette));
        Set_Palette_Changes(0, last_palette_index);
    }

    void __cdecl Set_Palette_Gradient(int32_t start_index, int32_t count, int32_t start_r, int32_t start_g, int32_t start_b, int32_t end_r, int32_t end_g, int32_t end_b) {
        if (count > 0) {
            int32_t current_r_diff = 0;
            int32_t current_g_diff = 0;
            int32_t current_b_diff = 0;

            int32_t count_minus_1 = count - 1;

            int32_t r_diff = end_r - start_r;
            int32_t g_diff = end_g - start_g;
            int32_t b_diff = end_b - start_b;

            s_palette_entry* entry = &video::current_palette[start_index];
            do {
                entry->changed = 1;

                entry->r = start_r + (current_r_diff / count_minus_1);
                entry->g = start_g + (current_g_diff / count_minus_1);
                entry->b = start_b + (current_b_diff / count_minus_1);

                current_r_diff += r_diff;
                current_g_diff += g_diff;
                current_b_diff += b_diff;

                entry++;
                count--;
            } while (count != 0);
        }
    }

    void __cdecl Refresh_Palette() {
        const int32_t palette_entry_count =
            (int32_t)(sizeof(video::current_palette) / sizeof(video::current_palette[0]));
        int32_t i = 0;
        s_palette_entry* const palette_end = video::current_palette + palette_entry_count;

        while (i < palette_entry_count) {
            s_palette_entry* entry = &video::current_palette[i];
            while (entry < palette_end && entry->changed == 0) {
                ++i;
                ++entry;
            }

            if (entry >= palette_end) {
                break;
            }

            int32_t count = 1;
            s_palette_entry* run = entry + 1;
            while (run < palette_end && run->changed != 0) {
                ++count;
                ++run;
            }

            palstore::Store_Palette_Block(i, count);

            i += count;
        }

        Clear_Palette_Changes(0, (int16_t)(palette_entry_count - 1));
    }

    void __cdecl Update_Cycle(int16_t* start_value, int16_t* end_value) {
        if (cycle_direction_flag == 0) {
            if (*start_value < *end_value) {
                cycle_color_value += cycle_step_value;
                if (*end_value < cycle_color_value) {
                    cycle_direction_flag = 1;
                    cycle_color_value = *end_value - 1;
                }
            } else {
                cycle_color_value -= cycle_step_value;
                if (cycle_color_value < *start_value) {
                    cycle_direction_flag = 1;
                    cycle_color_value = *start_value + 1;
                }
            }
        } else {
            if (*start_value < *end_value) {
                cycle_color_value -= cycle_step_value;
                if (cycle_color_value < *start_value) {
                    cycle_direction_flag = 0;
                    cycle_color_value = *start_value + 1;
                }
            } else {
                cycle_color_value += cycle_step_value;
                if (*end_value < cycle_color_value) {
                    cycle_direction_flag = 0;
                    cycle_color_value = *end_value - 1;
                }
            }
        }
    }

    void __cdecl Cycle_Palette_Color(int16_t palette_index, int16_t start_r, int16_t start_g, int16_t start_b, int16_t end_r, int16_t end_g, int16_t end_b, int16_t step_value) {
        int32_t delta_r = end_r - start_r;
        int32_t delta_g = end_g - start_g;
        int32_t delta_b = end_b - start_b;

        int32_t abs_r = delta_r;
        if (abs_r < 0) {
            abs_r = -abs_r;
        }

        int32_t abs_g = delta_g;
        if (abs_g < 0) {
            abs_g = -abs_g;
        }

        int32_t abs_b = delta_b;
        if (abs_b < 0) {
            abs_b = -abs_b;
        }

        cycle_step_value = step_value;

        int32_t max_idx = 2;
        if ((abs_r < abs_g) || (abs_r < abs_b)) {
            if ((abs_r <= abs_g) && (abs_b <= abs_g)) {
                max_idx = 1;
            }
        } else {
            max_idx = 0;
        }

        if (cycle_direction_flag == -1) {
            cycle_direction_flag = 0;
            if (max_idx == 0) {
                cycle_color_value = start_r;
            } else if (max_idx == 1) {
                cycle_color_value = start_g;
            } else if (max_idx == 2) {
                cycle_color_value = start_b;
            }
        }

        int32_t out_r = 0;
        int32_t out_g = 0;
        int32_t out_b = 0;

        if (max_idx == 0) {
            out_r = cycle_color_value;
            out_g = (delta_g * (cycle_color_value - start_r)) / delta_r + start_g;
            out_b = (delta_b * (cycle_color_value - start_r)) / delta_r + start_b;
        } else if (max_idx == 1) {
            out_g = cycle_color_value;
            out_r = (delta_r * (cycle_color_value - start_g)) / delta_g + start_r;
            out_b = (delta_b * (cycle_color_value - start_g)) / delta_g + start_b;
        } else if (max_idx == 2) {
            out_b = cycle_color_value;
            out_g = (delta_g * (cycle_color_value - start_b)) / delta_b + start_g;
            out_r = (delta_r * (cycle_color_value - start_b)) / delta_b + start_r;
        }

        if (max_idx == 0) {
            Update_Cycle(&start_r, &end_r);
        } else if (max_idx == 1) {
            Update_Cycle(&start_g, &end_g);
        } else if (max_idx == 2) {
            Update_Cycle(&start_b, &end_b);
        }

        palstore::Store_Single_Palette_Color(palette_index, (uint8_t)out_r, (uint8_t)out_g, (uint8_t)out_b);
    }

    void __cdecl Save_Glass_Remap_Block(void* remap_buffer) {
        uint8_t* remap_buffer_bytes = (uint8_t*)remap_buffer;

        struct_::Copy_Structure(remap_buffer_bytes, video::glass_remap_colors, sizeof(video::glass_remap_colors));
        struct_::Copy_Structure(
            remap_buffer_bytes + sizeof(video::glass_remap_colors),
            video::glass_colors,
            sizeof(video::glass_colors));
    }

    void* __cdecl Allocate_Glass_Remap_Next_Block(void* current_block) {
        return allocate::Allocate_Next_Block(
            current_block,
            sizeof(video::glass_remap_colors) + sizeof(video::glass_colors));
    }

    void __cdecl Set_Glass_Remap_Color(int16_t color_index, uint8_t red, uint8_t green, uint8_t blue, uint8_t intensity) {
        video::glass_colors[color_index].intensity = intensity;
        video::glass_colors[color_index].r = red;
        video::glass_colors[color_index].g = green;
        video::glass_colors[color_index].b = blue;
    }

    void __cdecl Restore_Glass_Remap_Block(void* remap_buffer) {
        uint8_t* remap_buffer_bytes = (uint8_t*)remap_buffer;

        struct_::Copy_Structure(video::glass_remap_colors, remap_buffer_bytes, sizeof(video::glass_remap_colors));
        struct_::Copy_Structure(
            video::glass_colors,
            remap_buffer_bytes + sizeof(video::glass_remap_colors),
            sizeof(video::glass_colors));
    }

    void __cdecl RGB_To_HLS(int8_t red, int8_t green, int8_t blue, int32_t* hue, int32_t* lightness, int32_t* saturation) {
        int32_t r_scaled = (red * 1000) / 63;
        int32_t g_scaled = (green * 1000) / 63;
        int32_t b_scaled = (blue * 1000) / 63;

        int32_t max_val = g_scaled;
        if (g_scaled <= r_scaled) {
            max_val = r_scaled;
        }
        if (max_val <= b_scaled) {
            max_val = b_scaled;
        }

        int32_t min_val = g_scaled;
        if (r_scaled <= g_scaled) {
            min_val = r_scaled;
        }
        if (b_scaled <= min_val) {
            min_val = b_scaled;
        }

        int32_t lightness_value = (max_val + min_val) / 2;
        *lightness = lightness_value;

        if (min_val != max_val) {
            int32_t delta = max_val - min_val;

            if (lightness_value <= 500) {
                *saturation = (delta * 1000) / (max_val + min_val);
            } else {
                *saturation = (delta * 1000) / ((2000 - min_val) - max_val);
            }

            if (r_scaled == max_val) {
                *hue = ((g_scaled - b_scaled) * 1000) / delta;
            } else if (g_scaled == max_val) {
                *hue = ((b_scaled - r_scaled) * 1000) / delta + 2000;
            } else if (b_scaled == max_val) {
                *hue = ((r_scaled - g_scaled) * 1000) / delta + 4000;
            }

            int32_t hue_degrees = *hue * 60;
            *hue = hue_degrees;
            if (hue_degrees < 0) {
                *hue = hue_degrees + 360;
            }
            *hue = *hue / 1000;
        } else {
            *saturation = 0;
            *hue = -1;
        }
    }

    int32_t __cdecl Value(int32_t m1, int32_t m2, int32_t hue) {
        if (hue > 360) {
            hue -= 360;
        } else if (hue < 0) {
            hue += 360;
        }

        if (hue < 60) {
            return (((m2 - m1) * (hue * 1000 / 60)) / 1000 + m1) * 63 / 1000;
        }
        if (hue < 180) {
            return m2 * 63 / 1000;
        }
        if (hue < 240) {
            return ((((240 - hue) * 1000 / 60) * (m2 - m1)) / 1000 + m1) * 63 / 1000;
        }

        return m1 * 63 / 1000;
    }

    void __cdecl HLS_To_RGB(int32_t hue, int32_t lightness, int32_t saturation, uint8_t* red, uint8_t* green, uint8_t* blue) {
        int32_t m2;

        if (lightness <= 500) {
            m2 = ((saturation + 1000) * lightness) / 1000;
        } else {
            m2 = (saturation * lightness) / -1000 + saturation + lightness;
        }

        int32_t m1 = lightness * 2 - m2;

        if (saturation == 0) {
            if (hue == -1) {
                uint8_t value = (lightness * 63) / 1000;
                *blue = value;
                *green = value;
                *red = value;
            } else {
                *blue = 0;
                *green = 0;
                *red = 0;
            }
            return;
        }

        *red = Value(m1, m2, hue + 120);
        *green = Value(m1, m2, hue);
        *blue = Value(m1, m2, hue - 120);
    }
} // namespace palette
