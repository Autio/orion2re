#include "pch.h"

namespace FMTPARA {
    extern const char legal_functions[] = "FRTXYSHVPIMOC^=";
    extern const char legal_fn_arg_chars[] = "1234567890+- ";
    extern const uint8_t legal_variable_types[6] = {1, 2, 3, 4, 5, 6};
    uint8_t font_base_offsets[6] = {5, 7, 10, 11, 14, 19};
    int8_t reset_parameters_lookup[16] = {1, 2, 4, 8, 12, 13, 16, 32, 29, 0, 0, 0, 0, 0, 0, 0};
    int16_t tabs[11];
    s_fmtpara_style saved_superscript;
    s_fmtpara_style* style_p;
    s_fmtpara_char* char_p;
    s_fmtpara_char* end_char_p;
    uint8_t last_fg_and_attr;
    s_fmtpara_para* para_p;
    int16_t insert_case;
    s_fmtpara_break* break_p;
    s_fmtpara_line* line_p;
    s_fmtpara_char* first_char_p;
    char justify_spread_flags[24] = {
        1,
        3,
        1,
        0,
        0,
        0,
        0,
        0,
        1,
        1,
        1,
        3,
        1,
        0,
        0,
        0,
        1,
        1,
        1,
        1,
        3,
        1,
        3,
        0,
    };
    uint16_t var_index;
    uint8_t last_font_id;
    s_fmtpara_global para_global;
    uint32_t variable[16];
    char tmp_str[16];
    char tmp_str2[32];
    extern const char* const ordinal_ending[10] = {
        "th",
        "st",
        "nd",
        "rd",
        "th",
        "th",
        "th",
        "th",
        "th",
        "th",
    };
    extern const char* const ordinal_word[11] = {
        "zeroeth",
        "first",
        "second",
        "third",
        "fourth",
        "fifth",
        "sixth",
        "seventh",
        "eighth",
        "ninth",
        "tenth",
    };
    static const char k_insert_item_kind[] = "11110101121212120000";
    uint32_t user_fn_arg;

    void __cdecl Init() {
        memset(tabs, 0, sizeof(tabs));
        memset(&para_global, 0, sizeof(para_global));
        Initialize_Formatted_Paragraph_System();
    }

    void __cdecl Reset_A_Paragraph_Variable(uint16_t index) {
        if (index < (sizeof(variable) / sizeof(variable[0]))) {
            uint32_t* var_ptr = &FMTPARA::variable[index];
            para_global.item_ptr[index] = var_ptr;
            para_global.item_type[index] = 2;
            *var_ptr = 0;
        }
    }

    void __cdecl Initialize_Formatted_Paragraph_System() {
        for (uint16_t i = 0; i < (sizeof(variable) / sizeof(variable[0])); ++i) {
            Reset_A_Paragraph_Variable(i);
        }
    }

    void __cdecl Initialize_Line_Start() {
        line_p->line_y_offset = 0;
        line_p->space_count = 0;
        line_p->char_count = 0;
        line_p->prev_x = 0;

        break_p->char_count = 0;
        break_p->space_count = 0;
    }

    void __cdecl Reset_Max_Min_Values() {
        para_global.max_x = 0;
        para_global.max_y = 0;
        para_global.min_x = para_p->x1;
        para_global.min_y = para_p->y;
    }

    void __cdecl Set_Para_Y2() {
        para_p->y2_limit = 480 - style_p->font_height_px;
    }

    void __cdecl Reset_Font_LF() {
        style_p->font_height_px = fonts::font_header.font_heights[style_p->font_id];
        para_p->line_advance_y = fonts::font_header.leading_extra[style_p->font_id] + style_p->font_height_px;
    }

    void __cdecl Reset_Font_Spacing_Width() {
        style_p->space_width = (int16_t)(int8_t)fonts::font_header.font_glyph_widths[style_p->font_id][32];
    }

    void __cdecl Reset_Font_Spacing() {
        int8_t font_id = style_p->font_id;

        line_p->font_spacing = fonts::font_header.font_spacing[font_id];

        style_p->char_widths = fonts::font_header.font_glyph_widths[font_id];

        Reset_Font_Spacing_Width();
    }

    uint8_t __cdecl Get_Current_Color_Set() {
        int normal_match_count = 0;
        int highlight_match_count = 0;

        for (int i = 0; i < 8; ++i) {
            uint8_t selected_color = fonts::font_header.selected_colors.colors[i];

            if (fonts::font_header.normal_colors.colors[i] == selected_color) {
                normal_match_count++;
            } else if (fonts::font_header.highlight_colors.colors[i] == selected_color) {
                highlight_match_count++;
            }
        }

        if (normal_match_count == 8) {
            return 0x00;
        }

        if (highlight_match_count == 8) {
            return 0x20;
        }

        return 0x40;
    }

    bool __cdecl Set_Line_Limits() {
        int16_t excl_left_x = 0;
        int16_t left_limit = para_p->x1 + para_p->left_indent;
        int16_t right_limit = para_p->x2 - para_p->right_indent;

        Initialize_Line_Start();

        int16_t excl_right_x = right_limit;

        if (para_p->paragraph_break != 0) {
            left_limit += para_p->first_line_indent;
        }

        if (paragrph::exclusion_count > 0) {
            int16_t font_height = style_p->font_height_px;
            if (font_height > 0) {
                // Stabilize line window by iterating exclusions
                while (right_limit > left_limit) {
                    for (int16_t i = 0; i < (int16_t)paragrph::exclusion_count; ++i) {
                        int16_t py = para_p->para_y;

                        // Check if paragraph Y coordinate is affected by this exclusion zone
                        if (py <= paragrph::exclusion_area_y2[i] && py <= paragrph::exclusion_area_y1[i]) {
                            int16_t x2_ex = paragrph::exclusion_area_x2[i];
                            int16_t x1_ex = paragrph::exclusion_area_x1[i];

                            // Check if current line window overlaps with exclusion zone edges
                            if ((left_limit < x2_ex && right_limit > x2_ex) ||
                                (left_limit < x1_ex && right_limit > x1_ex)) {
                                if (left_limit < x2_ex) {
                                    if (right_limit > x2_ex) {
                                        right_limit = x2_ex;
                                    } else if (excl_right_x > x2_ex) {
                                        excl_right_x = x2_ex;
                                    }
                                } else if (right_limit <= x2_ex && excl_right_x > x2_ex) {
                                    excl_right_x = x2_ex;
                                }

                                if (x1_ex > left_limit) {
                                    if (right_limit > x1_ex) {
                                        left_limit = x1_ex;
                                    } else if (right_limit <= x1_ex && excl_right_x > x1_ex) {
                                        excl_left_x = x1_ex;
                                    }
                                } else if (right_limit <= x1_ex && excl_right_x > x1_ex) {
                                    excl_left_x = x1_ex;
                                }
                            } else {
                                // Check if window is completely occluded by the exclusion zone
                                if (left_limit >= x2_ex && right_limit <= x1_ex) {
                                    right_limit = left_limit;
                                }
                            }
                        }

                        if (right_limit <= left_limit) {
                            break;
                        }
                    }

                    // Stabilization loop: re-check all exclusions if limits were modified.
                    // Note: This relies on limits eventually becoming invalid or stable.
                    if (font_height > 0) {
                        continue; // BUG: potential infinite loop
                    }
                    break;
                }
            }
        }

        if (left_limit < right_limit) {
            line_p->left_limit_x = left_limit;
            line_p->x = line_p->left_limit_x;
            line_p->x_candidate = line_p->x;
            line_p->right_limit_x = right_limit;
            line_p->exclusion_left_x = excl_left_x;
            line_p->exclusion_right_x = excl_right_x;

            para_p->line_start_x = line_p->left_limit_x;
            para_p->line_right_limit_x = line_p->right_limit_x;
            return true;
        }

        // Failure state: zero out exclusion data and set invalid right limit
        line_p->exclusion_left_x = 0;
        line_p->right_limit_x = line_p->left_limit_x;
        line_p->x = line_p->left_limit_x;
        line_p->x_candidate = line_p->x;

        return false;
    }

    void __cdecl Paragraph_HT() {
        for (int16_t i = 0; i < tabs[0]; ++i) {
            if (tabs[i + 1] > line_p->x) {
                line_p->x = tabs[i + 1];
                return;
            }
        }
    }

    bool __cdecl Legalize_Paragraph_Y() {
        do {
            if (Set_Line_Limits()) {
                return true;
            }
            para_p->para_y++;
        } while (para_p->para_y <= para_p->y2_limit);

        return false;
    }

    void __cdecl Reset_Parameters() {
        uint8_t reset_flags;
        uint16_t argument;

        para_p->str++;

        reset_flags = (uint8_t)reset_parameters_lookup[(uint8_t)*para_p->str & 0x0F];

        if (reset_flags & 0x01) {
            Reset_Font_Spacing();
        }

        if (reset_flags & 0x02) {
            Reset_Font_Spacing_Width();
        }

        if (reset_flags & 0x04) {
            Reset_Font_LF();
        }

        if (reset_flags & 0x08) {
            para_p->para_advance_y = para_p->line_advance_y;
        }

        if (reset_flags & 0x10) {
            Initialize_Formatted_Paragraph_System();
        }

        if (reset_flags & 0x20) {
            if (Get_Insert_Argument_Next(&argument)) {
                Reset_A_Paragraph_Variable(argument);
            }
        }
    }

    bool __cdecl Process_Command(char command_char) {
        if (command_char < 0x17) {
            if (command_char > 0xd) {
                return false;
            }

            switch (command_char) {
                case '\0':
                    insert_case = 0;
                    if (!Previous_Nesting_Level()) {
                        Complete_Line();
                        return true;
                    }
                    break;
                case '\a':
                    Process_Function();
                    if (*para_p->str == '\0') {
                        return true;
                    }
                    break;
                case '\b':
                    Record_Break_Position('\b');
                    return false;
                case '\t':
                    Record_Break_Position('\t');
                    Paragraph_HT();
                    return false;
                case '\n':
                    para_p->paragraph_break = 1;
                    // fall through
                case '\r':
                    Complete_Line();
                    char_p = first_char_p;
                    // fall through
                case '\v':
                    if (!Vertical_Move_Line_Advance()) {
                        Quit_Paragraph();
                        return false;
                    }
                    break;
                case '\f':
                    Complete_Line();
                    char_p = first_char_p;
                    if (!Vertical_Move_Paragraph_Advance()) {
                        Quit_Paragraph();
                        return false;
                    }
                    break;
                default:
                    break;
            }
        } else {
            uint8_t cmd = (uint8_t)command_char - 0x17;
            if (cmd > 8) {
                return false;
            }

            switch (command_char) {
                case 0x17:
                    Set_Value();
                    // fall through
                case 0x1F:
                    return Quit_Context();
                case 0x18:
                    Insert_Item();
                    para_p->str--;
                    return false;
                case 0x19:
                    Test_Item();
                    return false;
                case 0x1A:
                    Set_Justification();
                    return false;
                case 0x1B:
                    Set_Current_Colors();
                    return false;
                case 0x1C:
                    Set_Current_Style();
                    return false;
                case 0x1D:
                    Reset_Parameters();
                    return false;
                default:
                    break;
            }
        }
        return false;
    }

    void __cdecl Process_Function(void) {
        int32_t val;
        bool flag;
        bool sort_tabs = false;

        if (*para_p->str == '\a') {
            para_p->str++;
        }

        para_p->y_origin = 480 - style_p->font_height_px;

        while (true) {
            char cmd = toupper(*para_p->str);
            if (strchr(legal_functions, cmd) == NULL) {
                break;
            }

            if (cmd == 'T') {
                tabs[0] = 0;
            }

            sort_tabs = false;

            char c = cmd;

            int16_t arg_index = 0;

            do {
                arg_index++;
                para_p->str++;

                bool arg_ret = Get_Function_Argument(&val, &flag);

                switch (c) {
                    case '=': // 0x3d
                        c = FN_Set_Variable(c, val, arg_index, arg_ret, flag);
                        break;
                    case 'C': // 0x43
                        c = FN_User_Call(c, val, arg_index, arg_ret, flag);
                        break;
                    case 'F': // 0x46
                    case 'R': // 0x52
                        c = FN_Set_Font(c, val, arg_index, arg_ret, flag);
                        break;
                    case 'H': // 0x48
                        c = FN_Set_H_Spacing(c, val, arg_index, arg_ret, flag);
                        break;
                    case 'I': // 0x49
                        c = FN_Set_Indent(c, val, arg_index, arg_ret, flag);
                        Set_Line_Limits();
                        break;
                    case 'M': // 0x4d
                        c = FN_Set_Margins(c, val, arg_index, arg_ret, flag);
                        Set_Line_Limits();
                        break;
                    case 'O': // 0x4f
                        c = FN_Set_Origin(c, val, arg_index, arg_ret, flag);
                        break;
                    case 'P': // 0x50
                        c = FN_Set_FF_Size(c, val, arg_index, arg_ret, flag);
                        break;
                    case 'S': // 0x53
                        c = FN_Set_Space_Size(c, val, arg_index, arg_ret, flag);
                        break;
                    case 'T': // 0x54
                        sort_tabs = true;
                        c = FN_Set_Tabs(c, val, arg_index, arg_ret, flag);
                        break;
                    case 'V': // 0x56
                        c = FN_Set_V_Spacing(c, val, arg_index, arg_ret, flag);
                        break;
                    case 'X': // 0x58
                        c = FN_Set_X_Pos(c, val, arg_index, arg_ret, flag);
                        break;
                    case 'Y': // 0x59
                        c = FN_Set_Y_Pos(c, val, arg_index, arg_ret, flag);
                        break;
                    case '^': // 0x5e
                        c = FN_Superscript(c, val, arg_index, arg_ret, flag);
                        break;
                    default:
                        break;
                }

                para_p->str = Skip_Spaces(para_p->str);

            } while (*para_p->str == ',');

            if (c == 'O') {
                para_p->str = _Print_Formatted_Paragraph(
                    para_p->x1,
                    para_p->y,
                    para_p->width,
                    para_p->str,
                    para_p->justify_mode,
                    para_p->seg,
                    para_p->do_render);
            }
        }

        while (*para_p->str != '\a' && *para_p->str != '.' && *para_p->str != '\0') {
            para_p->str++;
        }

        if (sort_tabs) {
            Sort_Tabs();
        }
    }

    int8_t __cdecl Vertical_Move(int16_t delta) {
        int16_t original_y = para_p->para_y;
        para_p->para_y = original_y + delta;

        bool success = FMTPARA::Set_Line_Limits();

        // If move failed, try to find a valid position by incrementing para_y
        while (!success && para_p->para_y <= para_p->y2_limit) {
            para_p->para_y++;
            success = FMTPARA::Set_Line_Limits();
        }

        if (para_p->para_y <= para_p->y2_limit) {
            return 1;
        }

        // Restore original position if no valid position was found within limits
        para_p->para_y = original_y;
        return 0;
    }

    int8_t __cdecl Vertical_Move_Line_Advance() {
        return FMTPARA::Vertical_Move(FMTPARA::para_p->line_advance_y);
    }

    void __cdecl Quit_Paragraph() {
        if (para_p->nest_depth != 0) {
            para_p->str = para_p->nest_stack[0];
            para_p->nest_depth = 0;
        }

        while (*para_p->str != '\0') {
            para_p->str++;
        }

        if (*para_p->str == '\0') {
            para_p->str--;
        }
    }

    void __cdecl Process_Paragraph_Text() {
        char_p = first_char_p;
        bool stop_processing = false;

        while (true) {
            line_p->line_done = false;
            line_p->line_started = 1;
            line_p->prev_x = 0;
            line_p->x_candidate = line_p->x;
            Record_Break_Position(0);

            while (!line_p->line_done && !stop_processing) {
                uint8_t c = (uint8_t)*para_p->str;

                if (c >= 0x20) {
                    uint32_t type = isalpha(c);

                    if (type != 0 && insert_case != 0) {
                        if (insert_case == 1 || insert_case == 3) {
                            c = (uint8_t)toupper(c);
                        } else if (insert_case == 2) {
                            c = (uint8_t)tolower(c);
                        }
                    }

                    int16_t char_x_end;
                    if (c == ' ') {
                        char_x_end = style_p->space_width + line_p->x;
                    } else {
                        char_x_end = (int16_t)style_p->char_widths[c] + line_p->x;
                    }
                    char_x_end--; // x_candidate calculation

                    if (line_p->char_count < 150 && (char_x_end <= line_p->right_limit_x || line_p->line_started != 0)) {
                        line_p->x_candidate = char_x_end;
                        Add_Character_To_Line((char)c);
                        line_p->prev_x = line_p->x;
                        line_p->x = line_p->font_spacing + char_x_end + 1;
                        line_p->line_started = 0;

                        if (insert_case == 3) {
                            uint32_t alpha_check = isalpha(c);
                            if (alpha_check != 0) {
                                insert_case = 0;
                            }
                        }
                    } else {
                        // Word wrap or line break processing
                        if (c != ' ' && c != '\t') {
                            if (Return_To_Last_Break() && break_p->break_is_backspace) {
                                char_p->ch = '-';
                            }
                            para_p->str--;
                        }

                        if (line_p->x < line_p->exclusion_left_x) {
                            line_p->x = line_p->exclusion_left_x;
                            line_p->x_candidate = line_p->x;
                            line_p->left_limit_x = line_p->exclusion_left_x;
                            line_p->right_limit_x = line_p->exclusion_right_x;
                            line_p->exclusion_left_x = 0;
                        } else {
                            c = 0x0D; // Carriage Return to signal end of line to Process_Command
                        }
                    }
                }

                if (c < 0x20) {
                    stop_processing = Process_Command((char)c);
                }

                para_p->str++;
            }

            if (stop_processing) {
                setvesa::Add_Square_To_Scan_List(para_global.min_x, para_global.min_y, para_global.max_x, para_global.max_y);
                return;
            }
        }
    }

    bool __cdecl Vertical_Move_Paragraph_Advance() {
        para_p->paragraph_break = 1;
        return Vertical_Move(para_p->para_advance_y) != 0;
    }

    int16_t __cdecl Get_Last_Formatted_Paragraph_Max_Width() {
        return (int16_t)(para_global.max_x - para_global.min_x + 1);
    }

    int16_t __cdecl Get_Formatted_Paragraph_Max_Height(int16_t width, const char* text) {
        _Print_Formatted_Paragraph(0, 0, width, text, 0, nullptr, false);
        return (int16_t)((para_global.max_y - para_global.min_y) + 1);
    }

    int16_t __cdecl Get_Formatted_Paragraph_Max_Width(int16_t width, const char* text, int16_t justify) {
        _Print_Formatted_Paragraph(0, 0, width, text, justify, nullptr, false);
        return (int16_t)(para_global.max_x - para_global.min_x + 1);
    }

    const char* __cdecl _Print_Formatted_Paragraph(int16_t x, int16_t y, int16_t width, const char* str, int16_t justify, s_animation_header* seg, bool do_render) {
        s_fmtpara_para para;
        s_fmtpara_line linep;
        s_fmtpara_style stylep;
        s_fmtpara_break breakp;
        s_fmtpara_char endp[150];

        if (width >= 10 && width <= 640 &&
            x >= 0 && x < 640 &&
            y >= 0 && y < 480 &&
            str != nullptr) {
            para_p = &para;
            line_p = &linep;
            style_p = &stylep;
            break_p = &breakp;
            first_char_p = endp;
            end_char_p = endp;

            para.para_y = y;
            para.x1 = x;
            para.y = y;
            para.width = width;
            para.x2 = x + width - 1;
            para.str = str;
            para.seg = seg;
            para.do_render = do_render;
            para.first_line_indent = 0;
            para.right_indent = 0;
            para.left_indent = 0;
            para.justify_mode = (int8_t)justify;
            para.paragraph_break = 1;
            para.nest_depth = 0;
            para.line = &linep;

            para.y_origin = 479;

            stylep.font_id = (int8_t)fonts::current_font_style;
            stylep.highlight = 0;
            stylep.fg_color = (uint8_t)fonts::font_header.font_style;

            style_p->color_attr = Get_Current_Color_Set();

            Reset_Font_LF();
            Reset_Font_Spacing();

            para.para_advance_y = para.line_advance_y;

            Set_Para_Y2();

            if (Legalize_Paragraph_Y()) {
                Reset_Max_Min_Values();

                int16_t saved_current_font_style = fonts::current_font_style;
                s_font saved_font;
                memcpy(&saved_font, &fonts::font_header, sizeof(s_font));

                Process_Paragraph_Text();

                fonts::current_font_style = saved_current_font_style;
                memcpy(&fonts::font_header, &saved_font, sizeof(s_font));

                return para.str;
            } else {
                return nullptr;
            }
        }
        return str;
    }

    const char* __cdecl Print_Formatted_Paragraph(int16_t x, int16_t y, int16_t width, const char* str, int16_t justify) {
        return _Print_Formatted_Paragraph(x, y, width, str, justify, nullptr, true);
    }

    void __cdecl Add_Character_To_Line(char ch) {
        char_p->ch = ch;
        char_p->y = para_p->para_y;
        char_p->x_advance = line_p->x - line_p->prev_x;
        char_p->fg_and_attr = style_p->fg_color | style_p->color_attr;
        char_p->font_id = style_p->font_id;
        char_p->bg_color = style_p->bg_color;

        if (style_p->highlight != 0) {
            char_p->font_id |= 0x80;
        }

        char_p++;

        if (ch == ' ' /* 0x20 */) {
            Record_Break_Position(' ' /* 0x20 */);
            line_p->space_count++;
        }

        line_p->char_count++;
    }

    void __cdecl Record_Break_Position(char char_code) {
        if (line_p->line_started == 0 && char_code != '\0') {
            break_p->break_valid = true;

            int16_t hyphen_x = 0;
            if (char_code == '\b') {
                hyphen_x = (int16_t)style_p->char_widths['-'] + line_p->x;
                if (hyphen_x > line_p->right_limit_x) {
                    return;
                }
            }

            break_p->break_str_next = para_p->str + 1;
            break_p->break_x = line_p->x;
            break_p->break_x_candidate = line_p->x_candidate;
            break_p->insert_case = insert_case;
            break_p->line_y_offset = line_p->line_y_offset;
            break_p->font_spacing = line_p->font_spacing;
            break_p->nest_depth = para_p->nest_depth;
            break_p->char_count = line_p->char_count;
            break_p->space_count = line_p->space_count;

            // The original stack frame copies the 13 bytes immediately following s_fmtpara_line.
            // In this function those bytes are occupied by the adjacent s_fmtpara_style local.
            memcpy(&break_p->style, style_p, sizeof(s_fmtpara_style));

            break_p->break_is_backspace = false;
            if (char_code == '\b') {
                break_p->break_is_backspace = true;
                break_p->break_x = hyphen_x;
                break_p->break_x_candidate = hyphen_x;
                break_p->char_count++;
            } else if (char_code == ' ') {
                break_p->break_x_candidate -= (style_p->char_widths[' '] + line_p->font_spacing);
            }
        } else {
            break_p->break_valid = false;
        }
    }

    bool __cdecl Previous_Nesting_Level() {
        if (para_p->nest_depth > 0) {
            para_p->nest_depth--;
            para_p->str = para_p->nest_stack[para_p->nest_depth];
            para_p->str--;
            return true;
        }
        return false;
    }

    bool __cdecl Get_Insert_Argument_Next(uint16_t* value) {
        para_p->str++;
        para_p->str = Skip_Spaces(para_p->str);

        int c = toupper((unsigned char)*para_p->str);
        uint16_t digit;

        if (c >= '0' && c <= '9') {
            digit = (uint16_t)((uint8_t)c - 0x30);
        } else if (c >= 'A' && c <= 'Z') {
            digit = (uint16_t)((uint8_t)c - 0x37);
        } else {
            return false;
        }

        if (digit < 0x10) {
            *value = digit;
            return true;
        }

        return false;
    }

    void __cdecl Set_Value() {
        int16_t rel_x = line_p->x - para_p->x1;
        char cmd = para_p->str[1];

        switch (cmd) {
            case 'I':
                para_p->first_line_indent = rel_x - para_p->left_indent;
                break;

            case 'L':
                para_p->left_indent = rel_x;
                break;

            case 'R':
                para_p->right_indent = para_p->width - rel_x;
                break;

            case 'T':
                if (tabs[0] < 10) {
                    tabs[tabs[0] + 1] = line_p->x;
                    tabs[0]++;
                    Sort_Tabs();
                }
                break;

            case 'i':
                para_p->first_line_indent = 0;
                break;

            case 'l':
                para_p->left_indent = 0;
                break;

            case 'm':
                para_p->left_indent = 0;
                // Fallthrough
            case 'r':
                para_p->right_indent = 0;
                break;

            case 't':
                tabs[0] = 0;
                break;

            default:
                break;
        }
    }

    void __cdecl Insert_Item() {
        para_p->str++;
        uint8_t c = (uint8_t)*para_p->str;
        uint16_t arg_idx;
        bool has_arg = true;

        if (k_insert_item_kind[c - 'A'] != '0') {
            has_arg = Get_Insert_Argument_Next(&arg_idx);
        }

        if (!has_arg) {
            return;
        }

        switch (c) {
            case 'A':
                Skip_Arg_Insert_A_String(arg_idx, true);
                break;

            case 'B': {
                int val = Get_Item_Value_Set_Plural(arg_idx);
                snprintf(tmp_str, sizeof(tmp_str), "%i", val);
                Insert_String(tmp_str);
                break;
            }

            case 'C': {
                int val = Get_Item_Value_Set_Plural(arg_idx);
                snprintf(tmp_str, sizeof(tmp_str), "%i", val);
                int len = (int)strlen(tmp_str);
                int digit = tmp_str[len - 1] - '0';
                /* \a^2. renders the ordinal suffix in superscript and \a^0. restores the previous style. */
                snprintf(tmp_str2, sizeof(tmp_str2), "%s\a^2.%s\a^0.", tmp_str, ordinal_ending[digit]);
                Insert_String(tmp_str2);
                break;
            }

            case 'D': {
                int val = Get_Item_Value_Set_Plural(arg_idx);
                if (val >= 0 && val <= 10) {
                    Insert_String(ordinal_word[val]);
                } else {
                    snprintf(tmp_str, sizeof(tmp_str), "%i", val);
                    int len = (int)strlen(tmp_str);
                    int digit = tmp_str[len - 1] - '0';
                    snprintf(tmp_str2, sizeof(tmp_str2), "%s\a^2.%s\a^0.", tmp_str, ordinal_ending[digit]);
                    Insert_String(tmp_str2);
                }
                break;
            }

            case 'E':
                if (para_global.use_plural) {
                    Insert_String("s");
                }
                break;

            case 'F':
                if (Get_Item_Value(arg_idx) != 1) {
                    Insert_String("s");
                }
                break;

            case 'G':
                if (para_global.use_plural) {
                    Insert_String("es");
                }
                break;

            case 'H':
                if (Get_Item_Value(arg_idx) != 1) {
                    Insert_String("es");
                }
                break;

            case 'I':
                Skip_Arg_Insert_A_String(arg_idx, para_global.use_plural);
                break;

            case 'J':
                Insert_Next_Argument_String(Get_Item_Value(arg_idx) != 1);
                break;

            case 'K':
                Skip_Arg_Insert_A_String(arg_idx, !para_global.use_plural);
                break;

            case 'L':
                Insert_Next_Argument_String(Get_Item_Value(arg_idx) == 1);
                break;

            case 'M':
                Skip_Arg_Insert_A_String(arg_idx, para_global.cond_flag);
                break;

            case 'N':
                Insert_Next_Argument_String(Get_Item_Value(arg_idx) != 0);
                break;

            case 'O':
                Skip_Arg_Insert_A_String(arg_idx, !para_global.cond_flag);
                break;

            case 'P':
                Insert_Next_Argument_String(Get_Item_Value(arg_idx) == 0);
                break;

            case 'Q':
                insert_case = 1;
                break;

            case 'R':
                insert_case = 2;
                break;

            case 'S':
                insert_case = 3;
                break;

            case 'T':
                insert_case = 0;
                break;

            default:
                break;
        }
    }

    bool __cdecl Quit_Context() {
        char current_char = *para_p->str;
        para_p->str++;

        if (current_char == 'L' /* 0x4c */) {
            if (para_global.cond_flag) {
                Previous_Nesting_Level();
            }
            // Logic falls through to return false
        } else if (current_char == 'P' /* 0x50 */) {
            Quit_Paragraph();
            return true;
        }

        return false;
    }

    void __cdecl Set_Justification() {
        para_p->str++;
        int8_t new_mode = *para_p->str - '0';

        if (line_p->char_count > 0) {
            int16_t saved_right_limit_x = line_p->right_limit_x;
            line_p->right_limit_x = line_p->x;

            Complete_Line();

            line_p->right_limit_x = saved_right_limit_x;
            para_p->line_start_x = line_p->x;

            Initialize_Line_Start();

            char_p = first_char_p;
            line_p->line_done = false;
        }

        para_p->justify_mode = new_mode;
    }

    void __cdecl Complete_Line_Data() {
        line_p->line_done = true;

        if (line_p->char_count > 0) {
            if (line_p->line_y_offset != 0) {
                char_p = first_char_p;
                end_char_p = &first_char_p[line_p->char_count];

                do {
                    char_p->y += line_p->line_y_offset;

                    if (char_p->y < 0) {
                        char_p->y = 0;
                    }

                    char_p++;
                } while (char_p != end_char_p);

                para_p->para_y += line_p->line_y_offset;
            }

            int diff = (int)line_p->x_candidate - (int)line_p->left_limit_x;
            line_p->left_offset_abs = (int16_t)abs(diff);
        } else {
            line_p->left_offset_abs = 0;
        }
    }

    void __cdecl Complete_Line() {
        Complete_Line_Data();

        s_fmtpara_para* para = para_p;
        int8_t mode = para->justify_mode;

        // If justifying fully, check if this is the end of a block/paragraph.
        // Special character codes: 13 (CR), 10 (LF), 12 (FF).
        if (mode == 3 /* JUSTIFY_FULL */) {
            char current_char = *para->str;
            if (current_char == '\r' || current_char == '\n' || current_char == '\f' ||
                (para->nest_depth == 0 && current_char == '\0')) {
                // Do not fully justify the last line of a paragraph.
                Justify_Line(0 /* JUSTIFY_LEFT */);
                Print_Line();
                return;
            }
        }

        Justify_Line(mode);
        Print_Line();
    }

    void __cdecl Test_Item() {
        para_global.cond_flag = false;
        para_p->str++;
        const char* current_str = para_p->str;
        char c = *current_str;
        uint32_t val1 = Get_Next_Item_Value();

        switch (c) {
            case '0':
                para_global.cond_flag = (val1 != 0);
                break;

            case '1':
                para_global.cond_flag = para_global.use_plural;
                para_p->str = current_str;
                break;

            case '2':
                para_global.cond_flag = !para_global.cond_flag;
                para_p->str = current_str;
                break;

            case '3': {
                para_p->str = current_str;
                uint32_t cond_val = (uint32_t)para_global.cond_flag;
                uint16_t arg;
                if (Get_Insert_Argument_Next(&arg)) {
                    para_global.cond_flag = Set_Item_Value(cond_val, arg);
                }
                break;
            }

            case '4': {
                uint16_t arg;
                if (Get_Insert_Argument_Next(&arg)) {
                    para_global.cond_flag = Set_Item_Value(val1, arg);
                }
                break;
            }

            case '5': {
                uint32_t val2 = Get_Next_Item_Value();
                para_global.cond_flag = (val2 == val1);
                break;
            }

            case '6': {
                uint32_t val2 = Get_Next_Item_Value();
                para_global.cond_flag = (val2 != val1);
                break;
            }

            case '7': {
                uint32_t val2 = Get_Next_Item_Value();
                para_global.cond_flag = ((int32_t)val2 > (int32_t)val1);
                break;
            }

            case '8': {
                uint32_t val2 = Get_Next_Item_Value();
                para_global.cond_flag = ((int32_t)val2 < (int32_t)val1);
                break;
            }

            case '9': {
                uint32_t val2 = Get_Next_Item_Value();
                para_global.cond_flag = ((int32_t)val2 >= (int32_t)val1);
                break;
            }

            case 'A': {
                uint32_t val2 = Get_Next_Item_Value();
                para_global.cond_flag = ((int32_t)val2 <= (int32_t)val1);
                break;
            }

            default:
                break;
        }
    }

    void __cdecl Set_Current_Colors() {
        para_p->str++;
        uint8_t color_code = (uint8_t)*para_p->str;
        style_p->color_attr = (uint8_t)(color_code << 5);
    }

    uint32_t __cdecl Get_Item_Value(uint16_t index) {
        if (index < (sizeof(para_global.item_ptr) / sizeof(para_global.item_ptr[0]))) {
            int8_t type = FMTPARA::para_global.item_type[index];
            void* ptr = FMTPARA::para_global.item_ptr[index];

            switch (type) {
                case 2:
                    // Return as 32-bit dword
                    return *(uint32_t*)ptr;

                case 3:
                    // Return as unsigned 16-bit word
                    return (uint32_t) * (uint16_t*)ptr;

                case 4:
                    // Return as signed 16-bit short (sign-extended)
                    return (int32_t) * (int16_t*)ptr;

                case 6:
                    // Return as signed 8-bit char (sign-extended)
                    return (int32_t) * (int8_t*)ptr;

                case 5:
                default:
                    // Default case handles type 5 and any undefined types as unsigned 8-bit byte
                    return (uint32_t) * (uint8_t*)ptr;
            }
        }

        return 0;
    }

    uint32_t __cdecl Get_Next_Item_Value() {
        uint16_t item_id;

        if (FMTPARA::Get_Insert_Argument_Next(&item_id)) {
            return (uint32_t)FMTPARA::Get_Item_Value(item_id);
        }

        return 0;
    }

    bool __cdecl Set_Item_Value(uint16_t index, uint32_t value) {
        if (index >= (sizeof(para_global.item_ptr) / sizeof(para_global.item_ptr[0]))) {
            return false;
        }

        int8_t type = para_global.item_type[index];
        void* ptr = para_global.item_ptr[index];

        switch (type) {
            case 2:
                *(uint32_t*)ptr = value;
                return true;

            case 3:
                *(uint16_t*)ptr = (uint16_t)value;
                return true;

            case 4:
                *(int16_t*)ptr = (int16_t)value;
                return true;

            case 5:
                *(uint8_t*)ptr = (uint8_t)value;
                return true;

            case 6:
                *(int8_t*)ptr = (int8_t)value;
                return true;

            default:
                return false;
        }
    }

    void __cdecl Set_Current_Style() {
        uint16_t arg;
        para_p->str++;
        uint8_t ch = (uint8_t)*para_p->str;

        if (ch == '6' /* 0x36 */) {
            if (Get_Insert_Argument_Next(&arg)) {
                if (arg <= 7) {
                    style_p->bg_color = (uint8_t)arg;
                }
            }
        } else {
            if ((uint16_t)(ch - '0' /* 0x30 */) <= 5) {
                style_p->fg_color = ch;
            }
        }
    }

    bool __cdecl Get_Function_Argument(int32_t* val, bool* flag) {
        int8_t is_blank_arg = 1;

        para_p->str = Skip_Spaces(para_p->str);

        if (*para_p->str == '+' || *para_p->str == '-') {
            *flag = true;
        } else {
            *flag = false;
        }

        *val = atoi(para_p->str);

        // Advance current pointer past all characters that are legal in an argument
        while (strchr(legal_fn_arg_chars, *para_p->str) != nullptr) {
            if (*para_p->str != ' ') {
                is_blank_arg = 0;
            }
            para_p->str++;
        }

        return (int16_t)is_blank_arg;
    }

    char __cdecl FN_Set_Variable(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag) {
        if (arg_index == 1 /* Case 1: Initialize index */) {
            if (arg_ret == 0) {
                uint32_t value = (uint32_t)val;
                uint32_t sign = 0u - (value >> 31);

                var_index = (uint16_t)((value ^ sign) - sign);
            }
            return func_char;
        }

        if (arg_index == 2 /* Case 2: Update item value */) {
            if (var_index < 0x10) {
                uint32_t value = (uint32_t)val;

                if (flag) {
                    value += Get_Item_Value(var_index);
                }

                Set_Item_Value(var_index, value);
            }
        }

        return 0;
    }

    char __cdecl FN_Set_Font(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag) {
        if (flag) {
            val += style_p->font_id;
        }

        if (val >= 0 && val < 6) {
            uint8_t old_font_base = font_base_offsets[style_p->font_id];

            style_p->font_id = static_cast<int8_t>(val);

            if (func_char == 'F' /* 0x46 */) {
                style_p->highlight = 0;
            } else {
                style_p->highlight = 1;
            }

            Reset_Font_Spacing();
            Reset_Font_LF();
            Set_Para_Y2();

            int diff = static_cast<int>(font_base_offsets[style_p->font_id]) - static_cast<int>(old_font_base);

            if (line_p->line_y_offset < static_cast<int16_t>(diff)) {
                line_p->line_y_offset = static_cast<int16_t>(diff);
            }

            int adjusted_para_y = static_cast<int>(para_p->para_y) - diff;
            if (adjusted_para_y >= 0) {
                para_p->para_y = static_cast<int16_t>(adjusted_para_y);
                return 0;
            } else {
                line_p->line_y_offset = para_p->para_y;
                para_p->para_y = 0;
            }
        }

        return 0;
    }
    char __cdecl FN_Set_H_Spacing(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag) {
        if (arg_ret == 0) {
            int32_t spacing = val;

            if (flag) {
                spacing += (int32_t)line_p->font_spacing;
            }

            // Check if horizontal spacing is within valid bounds [0, 64)
            if (spacing >= 0 && spacing < 0x40) {
                line_p->font_spacing = (int16_t)spacing;
            }
        }

        return 'S';
    }

    char __cdecl FN_Set_Indent(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag) {
        int32_t new_indent;

        if (flag) {
            // Relative indentation adjustment
            new_indent = val + para_p->first_line_indent;
        } else {
            // Absolute indentation adjustment
            new_indent = val;
        }

        // Calculate total left position (left indent + the new indentation value)
        int32_t total_left_pos = (int32_t)para_p->left_indent + new_indent;

        // Ensure the paragraph block still fits within the defined width
        if ((int32_t)para_p->right_indent + total_left_pos < (int32_t)para_p->width) {
            if (total_left_pos >= 0) {
                // If indentation results in a valid positive coordinate, apply it
                para_p->first_line_indent = (int16_t)new_indent;
            } else {
                // If indentation would move text outside of bounds, clamp to left_indent
                para_p->first_line_indent = para_p->left_indent;
            }
        }

        return 0;
    }

    char __cdecl FN_Set_Margins(char status, int margin_val, int16_t side, bool skip_logic, bool apply_delta) {
        if (skip_logic == 0) {
            if (side == 1 /* Left side */) {
                if (apply_delta != 0) {
                    margin_val += para_p->left_indent;
                }

                if (margin_val < (int)para_p->width) {
                    if (margin_val < 0) {
                        para_p->left_indent = 0;
                    } else {
                        para_p->left_indent = (int16_t)margin_val;
                    }

                    // Ensure that the left boundary (indent + x1) does not overlap the right boundary (x2 + right_indent)
                    if ((int)para_p->left_indent + (int)para_p->x1 >= (int)para_p->x2 + (int)para_p->right_indent) {
                        para_p->right_indent = 0;
                    }
                }
            } else { // Right side
                if (apply_delta != 0) {
                    margin_val += para_p->right_indent;
                }

                if (margin_val >= 0) {
                    // Only apply if the right indent doesn't exceed the available width based on current left indent
                    if (margin_val < (int)para_p->width - (int)para_p->left_indent) {
                        para_p->right_indent = (int16_t)margin_val;
                    }
                }
            }
        }

        if (side == 1) {
            return status;
        }

        return 0x49 /* 'I' */;
    }

    char __cdecl FN_Set_FF_Size(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag) {
        if (arg_ret != 0) {
            para_p->para_advance_y = para_p->line_advance_y;
        } else {
            int32_t value = flag ? val + para_p->para_advance_y : val;
            uint32_t sign = 0u - ((uint32_t)value >> 31);
            uint32_t absolute_value = ((uint32_t)value ^ sign) - sign;

            if ((absolute_value ^ 0x80000000u) < (0x40u ^ 0x80000000u)) {
                para_p->para_advance_y = (int16_t)value;
            }
        }

        return 0;
    }

    char __cdecl FN_Set_Space_Size(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag) {
        if (arg_ret != 0) {
            style_p->space_width = line_p->font_spacing;
        } else {
            int32_t new_width = val;

            if (flag) {
                new_width += style_p->space_width;
            }

            if (new_width >= 0 && new_width < 0x40 /* 64 */) {
                style_p->space_width = (int16_t)new_width;
            }
        }

        return 0;
    }

    char __cdecl FN_Set_Tabs(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag) {
        if (tabs[0] < 10) {
            short base_x;
            if (flag) {
                base_x = line_p->x;
            } else {
                base_x = para_p->x1;
            }

            int32_t calculated_x = val + base_x;

            if (para_p->x1 <= calculated_x && calculated_x <= para_p->x2 && arg_ret == 0) {
                short current_count = tabs[0];
                tabs[current_count + 1] = static_cast<short>(calculated_x);
                tabs[0]++;
            }

            return func_char;
        }

        return 0;
    }

    char __cdecl FN_Set_Origin(char func_char, int32_t value, int16_t origin_axis, bool parse_failed, bool relative) {
        if (parse_failed == 0 /* false */) {
            if (origin_axis == 1) {
                if (relative != 0 /* true */) {
                    value += (int32_t)para_p->x1;
                }
                // Bounds check: 0 <= x < 640
                if (value >= 0 && value < 640) {
                    para_p->x1 = (int16_t)value;
                    return func_char;
                }
            } else if (origin_axis == 2) {
                if (relative != 0 /* true */) {
                    value += (int32_t)para_p->y;
                }
                // Bounds check: 0 <= y < 480
                if (value >= 0 && value < 480) {
                    para_p->y = (int16_t)value;
                    return func_char;
                }
            } else if (origin_axis == 3) {
                if (relative != 0 /* true */) {
                    value += (int32_t)para_p->width;
                }
                // Bounds check: 0 < width <= 640
                if (value > 0 && value <= 640) {
                    para_p->width = (int16_t)value;
                }
            }
        }
        return func_char;
    }

    char __cdecl FN_Set_V_Spacing(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag) {
        if (arg_ret == 0) {
            int32_t v_spacing = val;

            if (flag) {
                v_spacing += para_p->line_advance_y;
            }

            int64_t abs_v_spacing = v_spacing;
            if (abs_v_spacing < 0) {
                abs_v_spacing = -abs_v_spacing;
            }

            if (abs_v_spacing < 0x40) {
                para_p->line_advance_y = (int16_t)v_spacing;
            }
        }

        return 'P';
    }

    char __cdecl FN_Set_X_Pos(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag) {
        short base_x;

        if (flag) {
            base_x = line_p->x;
        } else {
            base_x = para_p->x1;
        }

        int new_x = val + base_x;

        if (new_x >= para_p->x1 && new_x <= para_p->x2) {
            line_p->x = static_cast<short>(new_x);
        }

        return 'Y';
    }

    char __cdecl FN_Set_Y_Pos(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag) {
        short start_y;

        // Determine if we are calculating relative to current paragraph Y or the absolute Y limit
        if (flag) {
            start_y = para_p->para_y;
        } else {
            start_y = para_p->y;
        }

        int final_y = val + (int)start_y;

        // Clamp the new paragraph Y position between 0 and the vertical origin/limit
        if (final_y > (int)para_p->y_origin) {
            final_y = (int)para_p->y_origin;
        } else if (final_y < 0) {
            final_y = 0;
        }

        para_p->para_y = (short)final_y;

        // Calculate discrepancy between the base y and the clamped paragraph y
        int diff = (int)para_p->y - (int)para_p->para_y;

        if (diff > 0) {
            // Update line vertical offset if current offset is smaller than the discrepancy
            if ((int)line_p->line_y_offset < diff) {
                line_p->line_y_offset = (short)diff;
            }
        }

        return 0;
    }

    const char* __cdecl Skip_Spaces(const char* str) {
        const char* current = str;
        char c = *str;

        while (true) {
            if (c != ' ') {
                if (*current != '\0') {
                    return current;
                }
                return str;
            }

            if (*current == '\0') {
                return str;
            }

            c = current[1];
            current++;
        }
    }

    void __cdecl Sort_Tabs() {
        if (tabs[0] > 1) {
            qsort(&tabs[1], (size_t)tabs[0], sizeof(int16_t), _Comp_Tabs);
        }
    }

    void __cdecl _Justify_Line_Fully(int16_t extra_space) {
        short space_count = line_p->space_count;
        short char_count = line_p->char_count;
        uint32_t spread_index;

        if (space_count == 0 || (char_count / 4) < space_count) {
            spread_index = 0;
        } else {
            // Check if space_count <= char_count / 8
            if (space_count <= (char_count / 8)) {
                spread_index = 2;
            } else {
                spread_index = 1;
            }
        }

        uint16_t non_space_justify_count = 0;
        end_char_p = &first_char_p[char_count];

        s_fmtpara_char* first_non_space = first_char_p;
        while (first_non_space < end_char_p && first_non_space->ch == ' ') {
            first_non_space++;
        }

        if (first_non_space != end_char_p) {
            // Start distributing extra space from the character following the first non-space
            s_fmtpara_char* distribution_start = first_non_space + 1;
            uint16_t flag_offset = 0;

            while (extra_space > 0) {
                uint8_t flags = justify_spread_flags[spread_index * 8 + flag_offset];

                if (flags & 2) {
                    non_space_justify_count++;
                    if (non_space_justify_count > 3) {
                        // Cap non-space justification to 3 passes
                        flags = 1; /* Force space-only justification */
                        if (line_p->space_count == 0) {
                            return;
                        }
                    }
                }

                s_fmtpara_char* current = distribution_start;
                while (current < end_char_p) {
                    bool applied = false;
                    if (current->ch == ' ') {
                        if (flags & 1) {
                            current->x_advance++;
                            extra_space--;
                            applied = true;
                        }
                    } else {
                        if (flags & 2) {
                            current->x_advance++;
                            extra_space--;
                            applied = true;
                        }
                    }

                    if (applied && extra_space <= 0) {
                        return;
                    }
                    current++;
                }

                flag_offset++;
                // Check for null terminator in flag pattern and reset
                if (justify_spread_flags[spread_index * 8 + flag_offset] == 0) {
                    flag_offset = 0;
                }
            }
        }
    }

    void __cdecl Justify_Line(int8_t mode) {
        s_fmtpara_line* line = line_p;
        int16_t remaining_width = (line->right_limit_x - line->left_limit_x - line->left_offset_abs) + 1;

        if (mode != 0 && line->char_count > 0 && remaining_width > 0) {
            if (mode == 1) {
                first_char_p->x_advance += remaining_width;
            } else if (mode == 2) {
                first_char_p->x_advance += (remaining_width / 2);
            } else if (mode == 3) {
                _Justify_Line_Fully(remaining_width);
            }
        }
    }

    bool __cdecl Return_To_Last_Break() {
        if (!break_p->break_valid) {
            return false;
        }

        para_p->str = break_p->break_str_next;
        line_p->x = break_p->break_x;
        line_p->x_candidate = break_p->break_x_candidate;
        insert_case = break_p->insert_case;
        para_p->nest_depth = break_p->nest_depth;
        line_p->line_y_offset = break_p->line_y_offset;
        line_p->font_spacing = break_p->font_spacing;
        line_p->char_count = break_p->char_count;
        line_p->space_count = break_p->space_count;

        memcpy(style_p, &break_p->style, sizeof(s_fmtpara_style));

        if (break_p->break_is_backspace && &first_char_p[line_p->char_count - 1] == char_p) {
            Add_Character_To_Line('-');
            char_p--;
            line_p->char_count--;
            return true;
        }

        char_p = &first_char_p[line_p->char_count - 1];
        return true;
    }

    void __cdecl Print_Line() {
        int16_t x = 0;

        if (line_p->char_count > 0) {
            char_p = first_char_p;
            end_char_p = first_char_p + line_p->char_count;

            if (para_p->paragraph_break != 0) {
                fonts::font_header.font_style = first_char_p->fg_and_attr & 0xF;
                Pick_Font_Color(first_char_p->fg_and_attr);
                last_fg_and_attr = char_p->fg_and_attr;
                Pick_Font_Number(char_p->font_id);
                last_font_id = char_p->font_id;
                para_p->paragraph_break = 0;
            }

            do {
                if ((int32_t)(int8_t)char_p->fg_and_attr != (int32_t)last_fg_and_attr) {
                    fonts::font_header.font_style = char_p->fg_and_attr & 0xF;
                    if (((char_p->fg_and_attr ^ last_fg_and_attr) & 0xE0) != 0) {
                        Pick_Font_Color(char_p->fg_and_attr);
                    }
                    last_fg_and_attr = char_p->fg_and_attr;
                }

                if ((int32_t)last_font_id != (int32_t)(int8_t)char_p->font_id) {
                    Pick_Font_Number(char_p->font_id);
                    last_font_id = char_p->font_id;
                }

                x += char_p->x_advance;
                Set_Max_Min_X(x, char_p);
                Set_Max_Min_Y(char_p);

                if (para_p->do_render) {
                    if (fonts::font_header.font_style != 0) {
                        uint8_t outline_byte = (uint8_t)fonts::outline_color;

                        for (int i = 0; i < 8; i++) {
                            fonts::font_header.selected_colors.colors[i] = outline_byte;
                        }

                        if (para_p->seg == nullptr) {
                            if (fonts::font_header.font_style != 2) {
                                fonts::Print_Character(x + 1, char_p->y + 1, char_p->ch);
                                fonts::Print_Character(x, char_p->y + 1, char_p->ch);
                                fonts::Print_Character(x + 1, char_p->y, char_p->ch);
                            }
                            if (fonts::font_header.font_style != 1 && fonts::font_header.font_style != 3) {
                                fonts::Print_Character(x - 1, char_p->y, char_p->ch);
                                fonts::Print_Character(x - 1, char_p->y - 1, char_p->ch);
                                fonts::Print_Character(x, char_p->y - 1, char_p->ch);
                            }
                            if (fonts::font_header.font_style == 3 || fonts::font_header.font_style == 5) {
                                fonts::Print_Character(x + 2, char_p->y + 2, char_p->ch);
                                fonts::Print_Character(x + 1, char_p->y + 2, char_p->ch);
                                fonts::Print_Character(x + 2, char_p->y + 1, char_p->ch);
                            }
                            if (fonts::font_header.font_style > 3) {
                                fonts::Print_Character(x + 1, char_p->y - 1, char_p->ch);
                                fonts::Print_Character(x - 1, char_p->y + 1, char_p->ch);
                            }
                            if (fonts::font_header.font_style == 5) {
                                fonts::Print_Character(x + 2, char_p->y, char_p->ch);
                                fonts::Print_Character(x, char_p->y + 2, char_p->ch);
                            }
                        } else {
                            if (fonts::font_header.font_style != 2) {
                                fonts::Print_Character_To_Bitmap(x + 1, char_p->y + 1, char_p->ch, para_p->seg);
                                fonts::Print_Character_To_Bitmap(x, char_p->y + 1, char_p->ch, para_p->seg);
                                fonts::Print_Character_To_Bitmap(x + 1, char_p->y, char_p->ch, para_p->seg);
                            }
                            if (fonts::font_header.font_style != 1 && fonts::font_header.font_style != 3) {
                                fonts::Print_Character_To_Bitmap(x - 1, char_p->y, char_p->ch, para_p->seg);
                                fonts::Print_Character_To_Bitmap(x - 1, char_p->y - 1, char_p->ch, para_p->seg);
                                fonts::Print_Character_To_Bitmap(x, char_p->y - 1, char_p->ch, para_p->seg);
                            }
                            if (fonts::font_header.font_style == 3 || fonts::font_header.font_style == 5) {
                                fonts::Print_Character_To_Bitmap(x + 2, char_p->y + 2, char_p->ch, para_p->seg);
                                fonts::Print_Character_To_Bitmap(x + 1, char_p->y + 2, char_p->ch, para_p->seg);
                                fonts::Print_Character_To_Bitmap(x + 2, char_p->y + 1, char_p->ch, para_p->seg);
                            }
                            if (fonts::font_header.font_style > 3) {
                                fonts::Print_Character_To_Bitmap(x + 1, char_p->y - 1, char_p->ch, para_p->seg);
                                fonts::Print_Character_To_Bitmap(x - 1, char_p->y + 1, char_p->ch, para_p->seg);
                            }
                            if (fonts::font_header.font_style == 5) {
                                fonts::Print_Character_To_Bitmap(x + 2, char_p->y, char_p->ch, para_p->seg);
                                fonts::Print_Character_To_Bitmap(x, char_p->y + 2, char_p->ch, para_p->seg);
                            }
                        }

                        fonts::font_header.selected_colors = fonts::font_header.normal_colors;
                    }

                    if (para_p->seg == nullptr) {
                        fonts::Print_Character(x, char_p->y, char_p->ch);
                    } else {
                        fonts::Print_Character_To_Bitmap(x, char_p->y, char_p->ch, para_p->seg);
                    }
                }
                char_p++;
            } while (char_p != end_char_p);
        }
    }

    /**
     * @brief Manages superscript font styling.
     * If val is 0, restores the previously saved font style.
     * Otherwise, saves the current style and shifts the font ID based on val.
     */
    char __cdecl FN_Superscript(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag) {
        if (val == 0) {
            // Restore style from saved buffer
            memcpy(style_p, &saved_superscript, sizeof(saved_superscript));
            return 0;
        }

        // Backup current style to saved buffer
        memcpy(&saved_superscript, style_p, sizeof(saved_superscript));

        // Calculate new font ID based on superscript depth (val)
        // font_id is at offset 0 of s_fmtpara_style
        int32_t new_font_id = (int32_t)style_p->font_id - val;

        if (new_font_id < 0) {
            style_p->font_id = 0;
        } else if (new_font_id < 6) {
            style_p->font_id = (int8_t)new_font_id;
        } else {
            style_p->font_id = 5;
        }

        Reset_Font_Spacing();
        return 0;
    }

    int8_t __cdecl FN_User_Call(int8_t func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag) {
        void* user_fn_ptr = para_global.user_fn;

        if (user_fn_ptr == nullptr) {
            return 0;
        }

        if (arg_index == 1) {
            if (arg_ret) {
                user_fn_arg = 0;
            } else {
                user_fn_arg = (uint32_t)val;
            }
            return func_char;
        }

        // Logical path for either arg_index == 2 or arg_ret being true
        if (arg_index == 2 || arg_ret) {
            Update_Max_Min_Values();

            user_fn fn = (user_fn)user_fn_ptr;
            fn(para_p, user_fn_arg, val);

            return 0;
        }

        return func_char;
    }

    int __cdecl _Comp_Tabs(const void* a, const void* b) {
        return (int)*(const int16_t*)a - (int)*(const int16_t*)b;
    }

    void __cdecl Pick_Font_Color(uint8_t fg_and_attr) {
        uint32_t masked_val = static_cast<uint32_t>(fg_and_attr) & 0xE0;

        if (masked_val == 0x00) {
            fonts::Set_Normal_Colors_On();
        } else if (masked_val == 0x20) {
            fonts::Set_Highlight_Colors_On();
        } else if (masked_val == 0x40) {
            fonts::Set_Special_Colors_On();
        }
    }

    void __cdecl Pick_Font_Number(uint8_t font_id) {
        s_colors saved_selected_colors;
        s_colors saved_current_colors;

        // Backup original font color states
        saved_selected_colors = fonts::font_header.selected_colors;
        saved_current_colors = fonts::current_font_color_array;

        if ((font_id & 0x80) != 0) {
            fonts::Set_Remap_Font_Style(font_id & 0x7F, &fonts::font_header.normal_colors);
        }

        // Restore the original font color states that might have been modified by Set_Remap_Font_Style_
        fonts::font_header.selected_colors = saved_selected_colors;
        fonts::current_font_color_array = saved_current_colors;
    }

    void __cdecl Update_Max_Min_Values() {
        short current_x = 0;

        if (FMTPARA::line_p->char_count > 0) {
            FMTPARA::char_p = FMTPARA::first_char_p;
            FMTPARA::end_char_p = &FMTPARA::first_char_p[FMTPARA::line_p->char_count];

            if (FMTPARA::char_p != FMTPARA::end_char_p) {
                do {
                    current_x += FMTPARA::char_p->x_advance;
                    FMTPARA::Set_Max_Min_X(current_x, FMTPARA::char_p);
                    FMTPARA::Set_Max_Min_Y(FMTPARA::char_p);
                    FMTPARA::char_p++;
                } while (FMTPARA::char_p != FMTPARA::end_char_p);
            }

            int diff = (int)FMTPARA::line_p->x_candidate - (int)FMTPARA::first_char_p->x_advance;
            FMTPARA::line_p->left_offset_abs = (short)abs(diff);
        }
    }

    void __cdecl Set_Max_Min_X(short current_x, s_fmtpara_char* char_info) {
        if (FMTPARA::first_char_p->x_advance < current_x) {
            int16_t glyph_offset = (int16_t)(((char_info->font_id & 0x7F) << 8) + char_info->ch);
            current_x += (&fonts::font_header.font_glyph_widths[0][0])[glyph_offset];
        }

        if (current_x < FMTPARA::para_global.min_x) {
            FMTPARA::para_global.min_x = current_x;
            return;
        }

        if (current_x > FMTPARA::para_global.max_x) {
            FMTPARA::para_global.max_x = current_x;
        }
    }

    void __cdecl Set_Max_Min_Y(s_fmtpara_char* char_p) {
        int16_t y = char_p->y;

        if (y < para_global.min_y) {
            para_global.min_y = y;
        }

        int16_t font_height = fonts::font_header.font_heights[char_p->font_id & 0x7f];
        int16_t total_y = y + font_height;

        if (total_y > para_global.max_y) {
            para_global.max_y = total_y;
        }
    }

    uint32_t __cdecl Get_Item_Value_Set_Plural(int16_t item_index) {
        uint32_t value = FMTPARA::Get_Item_Value(item_index);

        if (value != 1) {
            FMTPARA::para_global.use_plural = true;
        } else {
            FMTPARA::para_global.use_plural = false;
        }

        return value;
    }

    void __cdecl Insert_Next_Argument_String(bool flag) {
        uint16_t arg_index;

        if (FMTPARA::Get_Insert_Argument_Next(&arg_index)) {
            FMTPARA::Skip_Arg_Insert_A_String(arg_index, flag);
        }
    }

    void __cdecl Skip_Arg_Insert_A_String(uint16_t arg, bool flag) {
        if (!flag) {
            para_p->str++;
        } else {
            char* string_to_insert;
            if (arg < (sizeof(para_global.item_ptr) / sizeof(para_global.item_ptr[0]))) {
                string_to_insert = (char*)para_global.item_ptr[arg];
            } else {
                // If arg >= 16, string_to_insert is used uninitialized from the local stack slot
            }
            Insert_String(string_to_insert);
        }
    }

    void __cdecl Insert_A_String(const char* new_str) {
        if (para_p->nest_depth < 10) {
            para_p->nest_stack[para_p->nest_depth] = para_p->str;
            para_p->nest_depth++;
            para_p->str = new_str;
        }
    }

    void __cdecl Insert_String(const char* str) {
        FMTPARA::para_p->str++;
        FMTPARA::Insert_A_String(str);
    }

    void __cdecl Set_Paragraph_Integer_Pointer(uint16_t index, void* value_ptr) {
        Set_Paragraph_Variable_Pointer(index, value_ptr, 2);
    }

    void __cdecl Set_Paragraph_Sint_Pointer(uint16_t index, void* value_ptr) {
        Set_Paragraph_Variable_Pointer(index, value_ptr, 4);
    }

    void __cdecl Print_Formatted_Paragraph_To_Bitmap(int16_t x, int16_t y, int16_t width, const char* str, int16_t justify, s_animation_header* seg) {
        _Print_Formatted_Paragraph(x, y, width, str, justify, seg, true);
    }

    void __cdecl Set_Paragraph_Variable_Pointer(uint16_t item_index, void* value_ptr, int8_t value_type) {
        if (item_index < (sizeof(para_global.item_ptr) / sizeof(para_global.item_ptr[0]))) {
            para_global.item_ptr[item_index] = value_ptr;
            para_global.item_type[item_index] = value_type;
        }
    }
} // namespace FMTPARA