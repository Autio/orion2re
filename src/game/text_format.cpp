#include "pch.h"

#include "text_format.h"

namespace TEXT_FORMAT {
    void Print(
        const s_format& row,
        const STRBUILD::s_named_argument* arguments,
        uint32_t argument_count,
        int16_t y_offset,
        int16_t x_offset) {
        if (row.template_text[0] == '\0') {
            return;
        }

        char output[512];
        STRBUILD::Named_Snprintf(
            output,
            sizeof(output),
            row.template_text,
            arguments,
            argument_count);

        if (row.style > 0) {
            MISC::Change_Font_Style(row.style);
        }

        const int16_t x = (int16_t)(row.x + x_offset);
        const int16_t y = (int16_t)(row.y + y_offset);
        if (row.align == ALIGN_RIGHT) {
            fonts::Print_Right(x, y, output);
        } else if (row.align == ALIGN_CENTER) {
            fonts::Print_Centered(x, y, output);
        } else {
            fonts::Print(x, y, output);
        }
    }
} // namespace TEXT_FORMAT