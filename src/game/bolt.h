#pragma once
#include <cstdint>

namespace bolt {
    extern int16_t head_wide_bolts[192];
    extern int16_t tail_wide_bolts[192];

    /** Draws one plasma bolt segment by filling the scratch color buffer and rasterizing it along the line. */
    void __cdecl Bolt_Segment(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t color_base, int16_t color_range, int16_t color_jitter_chance, int16_t color_jitter_range, int16_t min_color, int16_t max_color, int16_t legacy_segment_flag, int16_t apply_sparkles, uint16_t sparkle_phase, int16_t tip_highlight);

    /** Draws a wide multi-strand bolt between the two endpoints. */
    void __cdecl Wide_Bolt_Line(
        int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t palette_base, int16_t palette_span, int16_t color_jitter_chance, int16_t color_jitter_range, uint16_t sparkle_phase, int16_t legacy_core_flag, int16_t point_source_end, int16_t tip_highlight, int16_t full_core_gradient_flag, int16_t apply_sparkles, int16_t legacy_side_flag, int16_t core_max_offset, int16_t core_min_offset, int16_t mid_max_offset, int16_t mid_min_offset, int16_t outer_max_offset, int16_t outer_min_offset);

    /** Draws a narrow bolt, optionally with side strands and endpoint caps. */
    void __cdecl Bolt_Line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t palette_base, int16_t palette_span, int16_t color_jitter_chance, int16_t color_jitter_range, uint16_t sparkle_phase, int16_t draw_caps, int16_t legacy_aux_flag, int16_t tip_highlight, int16_t full_core_gradient_flag, int16_t apply_sparkles, int16_t suppress_side_strands, int16_t core_max_offset, int16_t core_min_offset, int16_t side_max_offset, int16_t side_min_offset);
} // namespace bolt