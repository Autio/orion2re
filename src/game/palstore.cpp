#include "pch.h"

namespace palstore {
    void __cdecl Store_Palette_Block(int32_t first_entry, int32_t entry_count) {
        if (first_entry < first_entry + entry_count) {
            if (video::g_palette_mutex != nullptr) {
                SDL_LockMutex(video::g_palette_mutex);
            }

            int32_t palette_index = first_entry;
            int32_t remaining = entry_count;

            do {
                video::g_palette_entries[palette_index].r = video::current_palette[palette_index].r << 2;
                video::g_palette_entries[palette_index].g = video::current_palette[palette_index].g << 2;
                video::g_palette_entries[palette_index].b = video::current_palette[palette_index].b << 2;
                ++palette_index;
            } while (--remaining != 0);

            if (video::g_palette_mutex != nullptr) {
                SDL_UnlockMutex(video::g_palette_mutex);
            }
        }
    }

    void __cdecl Darken_Palette(int32_t darken_percent) {
        video::s_palette_color entries[256];
        int32_t multiplier = 100 - darken_percent;

        for (int32_t i = 1; i < 256; ++i) {
            entries[i].r = (uint8_t)(((video::current_palette[i].r * multiplier) / 100) << 2);
            entries[i].g = (uint8_t)(((video::current_palette[i].g * multiplier) / 100) << 2);
            entries[i].b = (uint8_t)(((video::current_palette[i].b * multiplier) / 100) << 2);
        }

        if (video::g_palette_mutex != nullptr) {
            SDL_LockMutex(video::g_palette_mutex);
        }

        for (int32_t i = 1; i < 256; ++i) {
            video::g_palette_entries[i] = entries[i];
        }

        if (video::g_palette_mutex != nullptr) {
            SDL_UnlockMutex(video::g_palette_mutex);
        }
    }

    void __cdecl Store_Single_Palette_Color(int32_t palette_index, uint8_t red, uint8_t green, uint8_t blue) {
        if (video::g_palette_mutex != nullptr) {
            SDL_LockMutex(video::g_palette_mutex);
        }

        video::g_palette_entries[palette_index].r = red << 2;
        video::g_palette_entries[palette_index].g = green << 2;
        video::g_palette_entries[palette_index].b = blue * 4;

        if (video::g_palette_mutex != nullptr) {
            SDL_UnlockMutex(video::g_palette_mutex);
        }

        video::current_palette[palette_index].r = red;
        video::current_palette[palette_index].g = green;
        video::current_palette[palette_index].b = blue;
    }

    int16_t Slow_Fade_In() {
        int16_t intensity = 100;
        int16_t input_occurred = 0;
        int16_t exit_loop = 0;

        video::Set_Refresh_Full();

        while (exit_loop == 0) {
            if (key::Keyboard_Status() == 1) {
                exit_loop = 1;
                key::Read_Key();
                input_occurred = 1;
            }

            if (mouse::Mouse_Button() != 0) {
                exit_loop = 1;
                input_occurred = 1;
            }

            Darken_Palette((int32_t)intensity);
            video::Submit_Palette();
            RUSS::Mox_Update();

            if (intensity-- == 0) {
                break;
            }
        }

        video::Set_Refresh_Stencil();

        if (input_occurred == 1) {
            JIM::darken_palette_value = intensity;
        }

        return input_occurred;
    }
} // namespace palstore