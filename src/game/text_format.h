#pragma once

#include <cstdint>

#include "strbuild.h"

namespace TEXT_FORMAT {
    enum e_row : uint8_t {
        ROW_MOD_NAME,
        ROW_VERSION,
        ROW_PROD,
        ROW_PROD2,
        ROW_LEADER_XP,
        ROW_RACE_SPIES,
        ROW_COUNT,
    };

    enum e_alignment : uint8_t {
        ALIGN_LEFT,
        ALIGN_RIGHT,
        ALIGN_CENTER,
    };

    struct s_format {
        char template_text[256];
        int16_t style;
        e_alignment align;
        int16_t x;
        int16_t y;
    };

    extern s_format format[ROW_COUNT];

    void Print(
        const s_format& row,
        const STRBUILD::s_named_argument* arguments,
        uint32_t argument_count,
        int16_t y_offset = 0,
        int16_t x_offset = 0);
} // namespace TEXT_FORMAT