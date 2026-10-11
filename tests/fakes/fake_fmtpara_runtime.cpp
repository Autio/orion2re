#include "pch.h"

#include <string>

namespace {
    std::string printed_text;
}

namespace test_fmtpara_runtime {
    void Reset_() {
        printed_text.clear();
        fonts::current_font_style = 0;
        fonts::font_header = {};
        for (int font = 0; font < 6; ++font) {
            fonts::font_header.font_heights[font] = 10;
            for (int ch = 0; ch < 256; ++ch) {
                fonts::font_header.font_glyph_widths[font][ch] = 1;
            }
        }
    }

    const char* Printed_Text_() {
        return printed_text.c_str();
    }
}

namespace fonts {
    s_font font_header;
    s_colors current_font_color_array;
    int16_t current_font_style;
    uint16_t outline_color;

    int16_t __cdecl Print_Character(int16_t, int16_t, int8_t ch) {
        printed_text += static_cast<char>(ch);
        return 0;
    }

    void __cdecl Print_Character_To_Bitmap(int16_t, int16_t, char, s_animation_header*) {
    }

    void __cdecl Set_Remap_Font_Style(int32_t font, s_colors*) {
        current_font_style = static_cast<int16_t>(font);
    }

    void __cdecl Set_Normal_Colors_On() {
    }

    void __cdecl Set_Highlight_Colors_On() {
    }

    void __cdecl Set_Special_Colors_On() {
    }
}

namespace paragrph {
    int16_t exclusion_count;
    int16_t exclusion_area_y1[4];
    int16_t exclusion_area_y2[4];
    int16_t exclusion_area_x1[4];
    int16_t exclusion_area_x2[4];
}

namespace setvesa {
    void __cdecl Add_Square_To_Scan_List(int32_t, int32_t, int32_t, int32_t) {
    }
}

void Legacy_Qsort(void* base, size_t count, size_t width, legacy_qsort_compare compare) {
    auto* bytes = static_cast<unsigned char*>(base);
    for (size_t i = 0; i < count; ++i) {
        for (size_t j = i + 1; j < count; ++j) {
            if (compare(bytes + i * width, bytes + j * width) > 0) {
                for (size_t k = 0; k < width; ++k) {
                    std::swap(bytes[i * width + k], bytes[j * width + k]);
                }
            }
        }
    }
}