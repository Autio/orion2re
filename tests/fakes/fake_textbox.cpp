#include "pch.h"

#include "fakes/fake_test_support.h"

namespace TEXTBOX {
    int16_t bottom_y = 0;
    uint8_t* textbox_seg = nullptr;
    s_animation_header* help_anim_bitmap = nullptr;
    uint16_t field_save_ix = 0;
    s_fieldsav_data textbox_field_save[2]{};
    uint8_t reload_saved_block = 0;
    int16_t box_x = 0;
    int16_t box_y = 0;
    uint16_t old_draw_fields_flag = 0;
    MOX2::action_func_t textbox_af = nullptr;
    uint16_t old_help_list_active = 0;
    int32_t base_colors[3]{};

    void __cdecl Get_Help_Lbx_Name(char* help_lbx_name) {
        if (help_lbx_name != nullptr) {
            help_lbx_name[0] = '\0';
        }
    }

    void __cdecl Get_Mox_Text_Box_Base_Colors(int16_t color_set, uint8_t* out_color_a, uint8_t* out_color_b) {
        (void)color_set;
        if (out_color_a != nullptr) {
            *out_color_a = 0;
        }
        if (out_color_b != nullptr) {
            *out_color_b = 0;
        }
    }

    void __cdecl Text_Box_Startup(int32_t required_buffer_size) {
        (void)required_buffer_size;
    }

    void __cdecl Box_Centered_On_XY(int16_t* x_ptr, int16_t* y_ptr, int16_t height) {
        (void)height;
        if (x_ptr != nullptr) {
            *x_ptr = 0;
        }
        if (y_ptr != nullptr) {
            *y_ptr = 0;
        }
    }

    void __cdecl Text_Box_Get_Input(uint16_t ticks) {
        (void)ticks;
    }

    void __cdecl Text_Box_Cleanup() {
    }

    void __cdecl Do_Text_Box(const char* title, const char* text, uint16_t input_ticks) {
        (void)title;
        (void)text;
        (void)input_ticks;
    }

    void __cdecl Text_Box(const char* title, const char* text) {
        (void)title;
        (void)text;
    }

    void __cdecl Draw_Help_Entry(int16_t help_id) {
        test::fakes::g_last_help_id = help_id;
    }

    void __cdecl Timed_Text_Box(const char* title, const char* message, uint16_t duration) {
        (void)title;
        (void)message;
        (void)duration;
    }
}