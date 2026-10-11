#include "pch.h"

namespace remap {
    uint8_t picture_remap_color_list[256];

    void __cdecl Sort_Palette_List() {
        for (int32_t i = 0; i < 255; ++i) {
            int32_t insert_pos = i;
            bool found = false;

            for (int32_t j = 0; j < i; ++j) {
                if (video::current_palette[i].r < video::current_palette[video::palette_reds[j]].r) {
                    insert_pos = j;
                    found = true;
                    break;
                }
            }

            if (found) {
                for (int32_t k = i; k >= insert_pos; --k) {
                    video::palette_reds[k + 1] = video::palette_reds[k];
                }
            }
            video::palette_reds[insert_pos] = (uint8_t)i;
        }

        video::palette_list[0] = &video::palette_reds[0];

        for (int32_t intensity = 1; intensity < 64; ++intensity) {
            bool found = false;
            for (int32_t j = 0; j < 256; ++j) {
                if (video::current_palette[video::palette_reds[j]].r == intensity) {
                    video::palette_list[intensity] = &video::palette_reds[j];
                    found = true;
                    break;
                }
            }

            if (!found) {
                video::palette_list[intensity] = video::palette_list[intensity - 1];
            }
        }

        video::palette_list[64] = &video::palette_reds[255];
    }

    uint8_t __cdecl Find_Closest_Color(uint8_t r, uint8_t g, uint8_t b) {
        int32_t r_min_idx = (int32_t)r;
        int32_t r_max_idx = (int32_t)r;
        int32_t g_min = (int32_t)g;
        int32_t g_max = (int32_t)g;
        int32_t b_min = (int32_t)b;
        int32_t b_max = (int32_t)b;

        int32_t best_color_idx = -1;

        for (int32_t retry_count = 25; retry_count > 0; --retry_count) {
            if (r_min_idx <= 10) {
                r_min_idx = 0;
            } else {
                r_min_idx -= 10;
            }

            if (r_max_idx < 53) {
                r_max_idx += 10;
            } else {
                r_max_idx = 64;
            }

            if (g_min <= 10) {
                g_min = 0;
            } else {
                g_min -= 10;
            }

            if (g_max < 53) {
                g_max += 10;
            } else {
                g_max = 63;
            }

            if (b_min <= 10) {
                b_min = 0;
            } else {
                b_min -= 10;
            }

            if (b_max < 53) {
                b_max += 10;
            } else {
                b_max = 63;
            }

            int32_t min_dist_sq = 13000;

            uint8_t* start_ptr = video::palette_list[r_min_idx];
            uint8_t* end_ptr = video::palette_list[r_max_idx];
            int32_t count = (int32_t)(end_ptr - start_ptr);

            if (count > 0) {
                for (int32_t i = 0; i < count; ++i) {
                    uint8_t color_idx = start_ptr[i];

                    int32_t cur_g = (int32_t)video::current_palette[color_idx].g;
                    if (cur_g >= g_min && cur_g <= g_max) {
                        int32_t cur_b = (int32_t)video::current_palette[color_idx].b;
                        if (cur_b >= b_min && cur_b <= b_max) {
                            int32_t cur_r = (int32_t)video::current_palette[color_idx].r;

                            int32_t dr = cur_r - (int32_t)r;
                            int32_t dg = cur_g - (int32_t)g;
                            int32_t db = cur_b - (int32_t)b;
                            int32_t dist_sq = (dr * dr) + (dg * dg) + (db * db);

                            if (dist_sq < min_dist_sq && color_idx != 0) {
                                best_color_idx = (int32_t)color_idx;
                                min_dist_sq = dist_sq;
                            }
                        }
                    }
                }
            }

            if (best_color_idx != -1) {
                return (uint8_t)best_color_idx;
            }
        }

        return 0;
    }

    void __cdecl Create_Picture_Remap_Palette(uint8_t* src_palette, uint8_t* remap_table) {
        Sort_Palette_List();

        for (int32_t i = 0; i < 256; ++i) {
            remap_table[i] = (uint8_t)i;
        }

        s_palette_header* palette_header = animate::palette_header;
        if (palette_header->count > 0) {
            for (int32_t i = 0; i < (int32_t)palette_header->count; ++i) {
                uint8_t r = src_palette[i * 4 + 1];
                uint8_t g = src_palette[i * 4 + 2];
                uint8_t b = src_palette[i * 4 + 3];

                uint8_t closest_color = Find_Closest_Color(r, g, b);

                remap_table[palette_header->start_index + i] = closest_color;
            }
        }
    }

    void __cdecl Update_Glass_Remap_Colors() {
        video::glass_colors[0].intensity = 50;
        video::glass_colors[0].r = 0;
        video::glass_colors[0].g = 0;
        video::glass_colors[0].b = 0;

        Sort_Palette_List();

        for (int32_t pal_idx = 0; pal_idx < 256; ++pal_idx) {
            for (int32_t glass_idx = 0; glass_idx < 16; ++glass_idx) {
                uint8_t intensity = video::glass_colors[glass_idx].intensity;

                if (intensity == 0) {
                    video::glass_remap_colors[glass_idx][pal_idx] = (uint8_t)pal_idx;
                } else {
                    uint8_t target_r, target_g, target_b;

                    if (intensity == 100) {
                        target_r = video::glass_colors[glass_idx].r;
                        target_g = video::glass_colors[glass_idx].g;
                        target_b = video::glass_colors[glass_idx].b;
                    } else {
                        int32_t weight_glass = (int32_t)intensity;
                        int32_t weight_pal = 100 - weight_glass;

                        target_r = (uint8_t)((video::current_palette[pal_idx].r * weight_pal + video::glass_colors[glass_idx].r * weight_glass) / 100);
                        target_g = (uint8_t)((video::current_palette[pal_idx].g * weight_pal + video::glass_colors[glass_idx].g * weight_glass) / 100);
                        target_b = (uint8_t)((video::current_palette[pal_idx].b * weight_pal + video::glass_colors[glass_idx].b * weight_glass) / 100);
                    }

                    video::glass_remap_colors[glass_idx][pal_idx] = Find_Closest_Color(target_r, target_g, target_b);
                }
            }
        }
    }

    void __cdecl Calculate_Picture_Remap_Colors(s_animation_header* animation_header) {
        animate::animation_header = animation_header;

        if (!(animate::animation_header->flags & ANIMATION_FLAG_HAS_PALETTE)) {
            exitm::Exit_With_Message("Remapped Picture Must Have Palette");
        }

        animate::palette_header = Animation_Palette_Header_(animate::animation_header);

        Create_Picture_Remap_Palette(
            (uint8_t*)animate::palette_header + sizeof(*animate::palette_header),
            picture_remap_color_list);
    }

    uint8_t __cdecl Get_Remap_Palette_Color(uint8_t index) {
        return picture_remap_color_list[index];
    }

    void __cdecl Get_Palette_Animation_Color(s_animation_header* animation,
        int16_t palette_index,
        uint8_t* out_red,
        uint8_t* out_green,
        uint8_t* out_blue) {
        char error_text[100];

        animate::animation_header = animation;
        if ((animation->flags & ANIMATION_FLAG_HAS_PALETTE) == 0) {
            exitm::Exit_With_Message("Get_Picture_Color  Picture Must Have Palette");
        }

        animate::palette_header = Animation_Palette_Header_(animate::animation_header);

        const int32_t palette_start = animate::palette_header->start_index;
        const int32_t palette_end = palette_start + animate::palette_header->count;

        if (((int32_t)palette_index >= palette_start) &&
            ((int32_t)palette_index < palette_end)) {
            const uint8_t* palette_entry =
                (const uint8_t*)animate::palette_header +
                sizeof(*animate::palette_header) +
                ((int32_t)palette_index - palette_start) * (int32_t)sizeof(uint32_t);

            *out_red = palette_entry[1];
            *out_green = palette_entry[2];
            *out_blue = palette_entry[3];
            return;
        }

        snprintf(error_text, sizeof(error_text), "%d, %d, %d", palette_index, palette_start, palette_end);
        exitm::Exit_With_Message(error_text);
    }
} // namespace remap