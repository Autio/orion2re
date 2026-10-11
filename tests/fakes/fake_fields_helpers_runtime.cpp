#include "pch.h"

namespace animate {
    int16_t clipping_flag = 0;
}

namespace fields {
    int16_t help_list_active = 0;
    int16_t help_list_count = 0;
    s_help_box* help_struct_pointer = nullptr;
    uint16_t mouse_cancel_disabled = 0;
    uint16_t field_box_mode = 0;
    uint16_t help_box_mode = 0;
    uint16_t draw_fields_flag = 1;
    uint16_t dynamic_field_refresh = 1;
    int16_t mouse_auto_exit = 0;
    uint32_t draw_override_flag = 0;
    int16_t selected_count = 0;
    uint16_t list_field_on = 0;
    uint16_t list_field_scan_flag = 1;
    int16_t pressed_button_flag = 0;
    int16_t last_button_number = 0;
    int16_t last_button_x = 0;
    int16_t last_button_y = 0;

    void __cdecl Clear_Fields() {
    }
}

namespace platform {
    void __cdecl Request_Exit(const char*) {
    }

    void __cdecl Request_Fatal_Exit(const char*) {
    }

    void __cdecl Check_Exit() {
    }
}