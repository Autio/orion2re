#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace TEST_MAIN_SCREEN_HUD {
    struct s_draw {
        int16_t x;
        int16_t y;
        std::string text;
        int16_t style;
        int16_t spacing;
        uint8_t color;
    };

    struct s_fit {
        std::string text;
        int16_t initial_style;
        int16_t initial_spacing;
        int16_t variant;
        int16_t max_width;
        int16_t min_style;
    };

    extern std::vector<s_draw> draws;
    extern std::vector<s_fit> fits;
    void Reset();
}