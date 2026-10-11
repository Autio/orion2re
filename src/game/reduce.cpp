#include "pch.h"

namespace reduce {
    s_palette_entry reduced_palette_list[512];
    int16_t ncubes;
    uint16_t active_cube_count;
    int16_t HistPtr[512];
    s_palette_cube cube_list[256];
    uint8_t color_weight[512];

    namespace {
        constexpr int16_t kReducedPaletteCapacity =
            (int16_t)(sizeof(reduced_palette_list) / sizeof(reduced_palette_list[0]));
        constexpr int16_t kReducedCubeCapacity =
            (int16_t)(sizeof(cube_list) / sizeof(cube_list[0]));
    } // namespace

    void __cdecl Reduce_Palette(s_palette_entry* palette, int16_t start_index, int16_t num_colors_src, int16_t num_colors_dst) {
        if (num_colors_src > kReducedPaletteCapacity) {
            exitm::Exit_With_Message("Too many colors to reduce from (Max 512)");
        }
        if (num_colors_dst > kReducedCubeCapacity) {
            exitm::Exit_With_Message("Too many colors to reduce to (Max 256)");
        }

        Initial_Cube(palette, num_colors_src);

        while (reduce::active_cube_count < num_colors_dst) {
            int16_t cube_idx = Find_Next_Lowest_Cube_Level();
            if (cube_idx == -1) {
                break;
            }

            int16_t axis = Find_Longest_Axis(cube_idx);

            int16_t first_hist_index = reduce::cube_list[cube_idx].first_hist_index;
            int16_t last_hist_index = reduce::cube_list[cube_idx].last_hist_index;

            Sort_Cube_Points(
                palette,
                &reduce::HistPtr[first_hist_index],
                (last_hist_index - first_hist_index) + 1,
                axis);

            Split_Cube(palette, cube_idx, axis);
        }

        int16_t i = 0;
        for (; i < reduce::active_cube_count; ++i) {
            s_palette_entry* entry = &palette[start_index + i];
            entry->changed = 1;
            entry->r = (uint8_t)((reduce::cube_list[i].r_min + reduce::cube_list[i].r_max) / 2);
            entry->g = (uint8_t)((reduce::cube_list[i].g_min + reduce::cube_list[i].g_max) / 2);
            entry->b = (uint8_t)((reduce::cube_list[i].b_min + reduce::cube_list[i].b_max) / 2);
        }

        for (; i < num_colors_dst; ++i) {
            s_palette_entry* entry = &palette[start_index + i];
            entry->changed = 1;
            entry->r = palette[start_index + i - 1].r;
            entry->g = palette[start_index + i - 1].g;
            entry->b = palette[start_index + i - 1].b;
        }
    }

    void __cdecl Clear_Reduced_Palette_Groups() {
        ncubes = 0;
    }

    void __cdecl Add_Reduced_Palette_Group(s_animation_header* anim) {
        animate::animation_header = anim;

        if ((anim->flags & ANIMATION_FLAG_HAS_PALETTE) == 0) {
            exitm::Exit_With_Message("Palette Reduction Pictures Must Have Palette");
        }

        animate::palette_header = Animation_Palette_Header_(anim);

        int16_t palette_count = animate::palette_header->count;

        if (palette_count > 0) {
            s_palette_entry* src_palette = (s_palette_entry*)(animate::palette_header + 1);

            for (int16_t i = 0; i < palette_count; ++i) {
                int16_t cube_idx = reduce::ncubes;
                reduce::reduced_palette_list[cube_idx].r = src_palette[i].r;
                reduce::reduced_palette_list[cube_idx].g = src_palette[i].g;
                reduce::reduced_palette_list[cube_idx].b = src_palette[i].b;

                reduce::ncubes++;

                if (reduce::ncubes >= kReducedPaletteCapacity) {
                    exitm::Exit_With_Message("Too many colors to reduce from (Max 512)");
                }
            }
        }
    }

    void __cdecl Compress_Reduced_Palette_Groups(int16_t start_index, int16_t color_count) {
        if (color_count < ncubes) {
            Reduce_Palette(reduced_palette_list, 0, ncubes, color_count);
        }

        if (color_count > 0) {
            for (int16_t i = 0; i < color_count; ++i) {
                int32_t dest_idx = start_index + i;
                video::current_palette[dest_idx].changed = 1;
                video::current_palette[dest_idx].r = reduced_palette_list[i].r;
                video::current_palette[dest_idx].g = reduced_palette_list[i].g;
                video::current_palette[dest_idx].b = reduced_palette_list[i].b;
            }
        }
    }

    void __cdecl Shrink_Cube(s_palette_cube* cube, s_palette_entry* palette_entries) {
        cube->r_min = 0xFF;
        cube->r_max = 0x00;
        cube->g_min = 0xFF;
        cube->g_max = 0x00;
        cube->b_min = 0xFF;
        cube->b_max = 0x00;

        for (int16_t i = cube->first_hist_index; i <= (int16_t)cube->last_hist_index; ++i) {
            int16_t hist_idx = HistPtr[i];

            uint8_t r = palette_entries[hist_idx].r;
            if (r < cube->r_min) {
                cube->r_min = r;
            }
            if (cube->r_max < r) {
                cube->r_max = r;
            }

            uint8_t g = palette_entries[hist_idx].g;
            if (g < cube->g_min) {
                cube->g_min = g;
            }
            if (cube->g_max < g) {
                cube->g_max = g;
            }

            uint8_t b = palette_entries[hist_idx].b;
            if (b < cube->b_min) {
                cube->b_min = b;
            }
            if (cube->b_max < b) {
                cube->b_max = b;
            }
        }
    }

    int16_t __cdecl Find_Next_Lowest_Cube_Level() {
        int16_t best_idx = -1;
        int16_t max_diff = 0;

        for (int16_t i = 0; i < (int32_t)active_cube_count; ++i) {
            if (cube_list[i].first_hist_index != cube_list[i].last_hist_index &&
                (int16_t)cube_list[i].level < 30000) {
                int16_t r_diff = cube_list[i].r_max - cube_list[i].r_min;
                int16_t g_diff = cube_list[i].g_max - cube_list[i].g_min;
                int16_t b_diff = cube_list[i].b_max - cube_list[i].b_min;

                if (max_diff < r_diff) {
                    max_diff = r_diff;
                    best_idx = i;
                }
                if (max_diff < g_diff) {
                    max_diff = g_diff;
                    best_idx = i;
                }
                if (max_diff < b_diff) {
                    max_diff = b_diff;
                    best_idx = i;
                }
            }
        }
        return best_idx;
    }

    void __cdecl Initial_Cube(s_palette_entry* palette, int16_t num_colors) {
        if (num_colors > 0) {
            for (int16_t i = 0; i < num_colors; ++i) {
                HistPtr[i] = i;
            }
        }

        cube_list[0].first_hist_index = 0;
        cube_list[0].last_hist_index = num_colors - 1;
        cube_list[0].level = 0;

        Shrink_Cube(&cube_list[0], palette);

        active_cube_count = 1;
    }

    int16_t __cdecl Find_Longest_Axis(int16_t cube_idx) {
        uint8_t r_diff = cube_list[cube_idx].r_max - cube_list[cube_idx].r_min;
        uint8_t g_diff = cube_list[cube_idx].g_max - cube_list[cube_idx].g_min;
        uint8_t b_diff = cube_list[cube_idx].b_max - cube_list[cube_idx].b_min;

        if (g_diff <= r_diff && b_diff <= r_diff) {
            return 0;
        }
        if (r_diff <= g_diff && b_diff <= g_diff) {
            return 1;
        }
        return 2;
    }

    void __cdecl Sort_Cube_Points(s_palette_entry* palette, int16_t* indices, int16_t num_indices, int16_t color_channel) {
        int16_t pass;
        bool swapped;
        int16_t i;

        if (color_channel == 0) {
            pass = 0;
            swapped = true;
            if (num_indices > 0) {
                while (swapped) {
                    swapped = false;
                    if (num_indices - 1 > 0) {
                        for (i = 0; i < num_indices - 1; ++i) {
                            if (palette[indices[i + 1]].r < palette[indices[i]].r) {
                                strings::Swap_Short(&indices[i], &indices[i + 1]);
                                swapped = true;
                            }
                        }
                    }
                    pass++;
                    if (pass >= num_indices) {
                        return;
                    }
                }
            }
        } else if (color_channel == 1) {
            pass = 0;
            swapped = true;
            if (num_indices > 0) {
                while (swapped) {
                    swapped = false;
                    if (num_indices - 1 > 0) {
                        for (i = 0; i < num_indices - 1; ++i) {
                            if (palette[indices[i + 1]].g < palette[indices[i]].g) {
                                strings::Swap_Short(&indices[i], &indices[i + 1]);
                                swapped = true;
                            }
                        }
                    }
                    pass++;
                    if (pass >= num_indices) {
                        return;
                    }
                }
            }
        } else if (color_channel == 2) {
            swapped = true;
            pass = 0;
            if (num_indices > 0) {
                while (swapped) {
                    swapped = false;
                    if (num_indices - 1 > 0) {
                        for (i = 0; i < num_indices - 1; ++i) {
                            if (palette[indices[i + 1]].b < palette[indices[i]].b) {
                                strings::Swap_Short(&indices[i], &indices[i + 1]);
                                swapped = true;
                            }
                        }
                    }
                    pass++;
                    if (pass >= num_indices) {
                        return;
                    }
                }
            }
        }
    }

    void __cdecl Split_Cube(s_palette_entry* palette_entries, int16_t cube_index, int16_t color_channel) {
        uint16_t first = cube_list[cube_index].first_hist_index;
        uint16_t last = cube_list[cube_index].last_hist_index;

        if (color_channel == 0) {
            if (first <= last) {
                for (uint16_t i = first; i <= last; ++i) {
                    if (i == first) {
                        color_weight[i] = palette_entries[HistPtr[i + 1]].r - palette_entries[HistPtr[i]].r;
                    } else if (i == last) {
                        color_weight[i] = palette_entries[HistPtr[i]].r - palette_entries[HistPtr[i - 1]].r;
                    } else {
                        uint8_t diff1 = palette_entries[HistPtr[i]].r - palette_entries[HistPtr[i - 1]].r;
                        uint8_t diff2 = palette_entries[HistPtr[i + 1]].r - palette_entries[HistPtr[i]].r;
                        color_weight[i] = (diff2 > diff1) ? diff2 : diff1;
                    }

                    if (color_weight[i] == 0) {
                        color_weight[i] = 1;
                    }
                }
            }
        } else if (color_channel == 1) {
            if (first <= last) {
                for (uint16_t i = first; i <= last; ++i) {
                    if (i == first) {
                        color_weight[i] = palette_entries[HistPtr[i + 1]].g - palette_entries[HistPtr[i]].g;
                    } else if (i == last) {
                        color_weight[i] = palette_entries[HistPtr[i]].g - palette_entries[HistPtr[i - 1]].g;
                    } else {
                        uint8_t diff1 = palette_entries[HistPtr[i]].g - palette_entries[HistPtr[i - 1]].g;
                        uint8_t diff2 = palette_entries[HistPtr[i + 1]].g - palette_entries[HistPtr[i]].g;
                        color_weight[i] = (diff2 > diff1) ? diff2 : diff1;
                    }

                    if (color_weight[i] == 0) {
                        color_weight[i] = 1;
                    }
                }
            }
        } else if (color_channel == 2) {
            if (first <= last) {
                for (uint16_t i = first; i <= last; ++i) {
                    if (i == first) {
                        color_weight[i] = palette_entries[HistPtr[i + 1]].b - palette_entries[HistPtr[i]].b;
                    } else if (i == last) {
                        color_weight[i] = palette_entries[HistPtr[i]].b - palette_entries[HistPtr[i - 1]].b;
                    } else {
                        uint8_t diff1 = palette_entries[HistPtr[i]].b - palette_entries[HistPtr[i - 1]].b;
                        uint8_t diff2 = palette_entries[HistPtr[i + 1]].b - palette_entries[HistPtr[i]].b;
                        color_weight[i] = (diff2 > diff1) ? diff2 : diff1;
                    }

                    if (color_weight[i] == 0) {
                        color_weight[i] = 1;
                    }
                }
            }
        }

        int32_t sum = 0;
        for (uint16_t i = first; i <= last; ++i) {
            sum += color_weight[i];
        }

        sum = (sum / 2) - color_weight[first];
        uint16_t split_index = first;

        while (sum > 0 && split_index < last) {
            split_index++;
            sum -= color_weight[split_index];
        }

        uint16_t new_cube_index = active_cube_count;
        cube_list[new_cube_index].first_hist_index = split_index + 1;
        cube_list[new_cube_index].last_hist_index = cube_list[cube_index].last_hist_index;
        cube_list[new_cube_index].level = cube_list[cube_index].level + 1;

        cube_list[cube_index].last_hist_index = split_index;
        cube_list[cube_index].level++;

        Shrink_Cube(&cube_list[cube_index], palette_entries);
        Shrink_Cube(&cube_list[new_cube_index], palette_entries);

        active_cube_count++;
    }
} // namespace reduce