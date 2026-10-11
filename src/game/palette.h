#pragma once

namespace palette {
    // Fills `count` entries starting at `start_index` with a gradient from the start RGB color to the end RGB color.
    void __cdecl Set_Palette_Gradient(int32_t start_index, int32_t count, int32_t start_r, int32_t start_g, int32_t start_b, int32_t end_r, int32_t end_g, int32_t end_b);

    extern s_palette_entry saved_palette[256];

    // Saves the current palette so it can be restored later.
    void __cdecl Save_Palette();

    // Marks palette entries in the inclusive [`start_index`, `end_index`] range as changed.
    void __cdecl Set_Palette_Changes(int16_t start_index, int16_t end_index);

    // Clears the changed flag for palette entries in the inclusive [`start_index`, `end_index`] range.
    void __cdecl Clear_Palette_Changes(int16_t start_index, int16_t end_index);

    extern int32_t cycle_direction_flag;

    extern int32_t cycle_step_value;

    extern int32_t cycle_color_value;

    // Resets palette cycling so the next cycle starts from its initial endpoint.
    void __cdecl Reset_Cycle_Palette_Color();

    // Restores the saved palette and marks every palette entry as changed.
    void __cdecl Restore_Palette();

    // Uploads contiguous runs of changed palette entries to the platform palette.
    void __cdecl Refresh_Palette();

    // Advances one palette entry along the RGB line from the start color to the end color using `step_value`.
    void __cdecl Cycle_Palette_Color(int16_t palette_index, int16_t start_r, int16_t start_g, int16_t start_b, int16_t end_r, int16_t end_g, int16_t end_b, int16_t step_value);

    // Updates the active cycle component between `start_value` and `end_value`.
    void __cdecl Update_Cycle(int16_t* start_value, int16_t* end_value);

    // Saves the glass remap tables into `remap_buffer`.
    void __cdecl Save_Glass_Remap_Block(void* remap_buffer);

    // Allocates the next block sized for one saved glass remap payload after `current_block`.
    void* __cdecl Allocate_Glass_Remap_Next_Block(void* current_block);

    // Sets one glass remap base color and its `intensity` value.
    void __cdecl Set_Glass_Remap_Color(int16_t color_index, uint8_t red, uint8_t green, uint8_t blue, uint8_t intensity);

    // Restores the glass remap tables from `remap_buffer`.
    void __cdecl Restore_Glass_Remap_Block(void* remap_buffer);

    // Converts an RGB palette color to HLS and stores the results in `hue`, `lightness`, and `saturation`.
    void __cdecl RGB_To_HLS(int8_t red, int8_t green, int8_t blue, int32_t* hue, int32_t* lightness, int32_t* saturation);

    // Converts an HLS color to RGB and stores the channel values in `red`, `green`, and `blue`.
    void __cdecl HLS_To_RGB(int32_t hue, int32_t lightness, int32_t saturation, uint8_t* red, uint8_t* green, uint8_t* blue);

    // Evaluates one RGB channel from HLS interpolation for the supplied `hue`.
    int32_t __cdecl Value(int32_t m1, int32_t m2, int32_t hue);
} // namespace palette