#include "pch.h"

#include <string>

namespace {
    int print_kind;
    int16_t print_x;
    int16_t print_y;
    int16_t font_style;
    std::string print_text;

    void Capture_(
        int kind,
        int16_t x,
        int16_t y,
        const char* text
    ) {
        print_kind = kind;
        print_x = x;
        print_y = y;
        print_text = text != nullptr ? text : "";
    }
}

namespace test_text_format_runtime {
    void Reset_() {
        print_kind = 0;
        print_x = 0;
        print_y = 0;
        font_style = 0;
        print_text.clear();
    }

    int Print_Kind_() {
        return print_kind;
    }

    int16_t Print_X_() {
        return print_x;
    }

    int16_t Print_Y_() {
        return print_y;
    }

    const char* Print_Text_() {
        return print_text.c_str();
    }

    int16_t Font_Style_() {
        return font_style;
    }
}

namespace MISC {
    void __cdecl Change_Font_Style(int16_t font_id) {
        font_style = font_id;
    }
}

namespace fonts {
    void __cdecl Print(int16_t x, int16_t y, const char* text) {
        Capture_(1, x, y, text);
    }

    void __cdecl Print_Right(int16_t x, int16_t y, const char* text) {
        Capture_(2, x, y, text);
    }

    void __cdecl Print_Centered(int16_t x, int16_t y, const char* text) {
        Capture_(3, x, y, text);
    }
}