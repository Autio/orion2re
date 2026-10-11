#pragma once
#include <cstdint>

struct s_animation_header;

namespace bitmap {
    // Computes the opaque-pixel bounds of `header`.
    // `out_min_x`/`out_min_y` receive the top-left pixel, and `out_actual_width`/`out_actual_height` receive the bounding size.
    void __cdecl Get_Bitmap_Actual_Size(s_animation_header* header, int16_t* out_min_x, int16_t* out_min_y, int16_t* out_actual_width, int16_t* out_actual_height);

    // Returns the allocation size in bytes for a bitmap with `width` and `height`.
    int32_t __cdecl Get_Bitmap_Size(int16_t width, int16_t height);

    // Initializes `header` as a single-frame bitmap with the given `width` and `height`.
    void __cdecl Create_Bitmap_Header(int16_t width, int16_t height, s_animation_header* header);

    // Initializes `bitmap` as a blank single-frame bitmap with the given `width` and `height`.
    void __cdecl Create_Blank_Bitmap(int16_t width, int16_t height, void* bitmap);

    // Expands the current picture data from `src` into a flat bitmap stored in `dest`.
    void __cdecl Draw_Picture_To_Bitmap(s_animation_header* dest, s_animation_header* src);

    // Remaps the current frame in `animation_header` to a grayscale gradient from `start_color` to `end_color`.
    // `color_translation_table` remaps the gradient colors, and `unused_param` is retained for ABI compatibility.
    void __cdecl Gray_Scale_Bitmap(
        s_animation_header* animation_header,
        uint32_t unused_param,
        int16_t start_color,
        const uint8_t* color_translation_table,
        int16_t end_color);

    // Blits `src` onto `dest` at (`x`, `y`) using the source frame's transparency rules.
    void __cdecl Add_Picture_To_Bitmap(int16_t x, int16_t y, s_animation_header* src, s_animation_header* dest);

    // Captures the screen rectangle from (`x1`, `y1`) to (`x2`, `y2`) into `header`.
    void __cdecl Screen_Bitmap_Capture(int16_t x1, int16_t y1, int16_t x2, int16_t y2, s_animation_header* header);

    // Replaces pixels in `anim` whose values fall between `start_color` and `end_color`.
    void __cdecl Mask_Out_Colors(s_animation_header* anim, int16_t start_color, int16_t end_color);

    // Applies the glass remap for `tint_color_index` from `src` into `dest`.
    void __cdecl Tint_Bitmap(s_animation_header* dest, s_animation_header* src, int16_t tint_color_index);

    extern uint32_t pixel_count;

    extern uint8_t* bitmap_temp1;

    extern uint32_t bitmap_temp2;

    // Tints a `width` by `height` pixel block from `src` into `dest` using `tint_table`.
    // `unused_param` is retained for ABI compatibility.
    void __cdecl Tint_Bitmap_Pixels(uint8_t* src, uint8_t* tint_table, int32_t unused_param, uint8_t* dest, int32_t width, int32_t height);

    // Clears the mouse cursor image block for `image_idx`.
    void __cdecl Clear_Mouse_Image(int16_t image_idx);

    // Applies the vanish effect from `src` into `dest` with the given `intensity`.
    void __cdecl Vanish_Bitmap(s_animation_header* dest, s_animation_header* src, int16_t intensity);

    // Marks `anim` as glassed in its bitmap flags.
    void __cdecl Set_Picture_Glassed(s_animation_header* anim);

    // Flips `src_hdr` into `dst_hdr` according to `flip_mode`.
    void __cdecl Flip_Bitmap(s_animation_header* dst_hdr, s_animation_header* src_hdr, int16_t flip_mode);

    // Draws an outline of `src` into `dst` using `color`.
    // `no_glass` selects the no-glass variant when non-zero.
    void __cdecl Outline_Bitmap(s_animation_header* dst, s_animation_header* src, int16_t color, int16_t no_glass);

    // Masks `dest` with `src` at (`x`, `y`).
    // `type` selects the normal or inverse masking mode.
    void __cdecl Mask_Out_Bitmap_With_Bitmap(int16_t x, int16_t y, s_animation_header* dest, s_animation_header* src, int16_t type);

    // Copies the current frame from `src` into `dst` as a flat bitmap.
    void __cdecl Copy_Bitmap_To_Bitmap(s_animation_header* dst, s_animation_header* src);

    extern uint8_t* brighten_palette_seg;

    extern int8_t brighten_palette_levels;

    extern int32_t brighten_start_color;

    // Scales `src` into `dst` using the percentage factors `scale_x` and `scale_y`.
    void __cdecl Scale_Bitmap(s_animation_header* dst, s_animation_header* src, int16_t scale_x, int16_t scale_y);

    // Draws `img` as a rotated and scaled line sprite between (`x1`, `y1`) and (`x2`, `y2`).
    void __cdecl Draw_Bitmap_Line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, s_animation_header* img);

    // Draws `anim` into the mouse cursor image for `mouse_type` at (`x`, `y`).
    void __cdecl Draw_Bitmap_To_Mouse(int16_t x, int16_t y, s_animation_header* anim, int16_t mouse_type);

    // Builds brighten lookup palettes for `anim` from `levels`, with `level_count` entries.
    void __cdecl Create_Brighten_Bitmap_Palettes(s_animation_header* anim, int8_t* levels, int8_t level_count);

    // Masks and brightens `dst_anim` with `src_anim` at (`x`, `y`), starting from `color`.
    void __cdecl Mask_Brighten_Bitmap(int32_t x, int32_t y, s_animation_header* dst_anim, s_animation_header* src_anim, uint8_t color);

    // Applies the jumble effect from `src` into `dest` using the configured noise threshold and jumble mask.
    void __cdecl Jumble_Bitmap(s_animation_header* dest, s_animation_header* src, int16_t noise_threshold, int16_t jumble_mask);
} // namespace bitmap