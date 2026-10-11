#pragma once

struct s_animation_header;

typedef struct s_fmtpara_break s_fmtpara_break, *Ps_fmtpara_break;
typedef struct s_fmtpara_style s_fmtpara_style, *Ps_fmtpara_style;

struct s_fmtpara_style {
    int8_t font_id;
    undefined1 highlight;
    uint8_t fg_color;
    uint8_t bg_color;
    uint8_t color_attr;
    int16_t font_height_px;
    int8_t* char_widths;
    int16_t space_width;
};

struct s_fmtpara_break {
    const char* break_str_next;
    int16_t break_x;
    int16_t break_x_candidate;
    int16_t insert_case;
    int16_t line_y_offset;
    int16_t font_spacing;
    int16_t nest_depth;
    int16_t char_count;
    int16_t space_count;
    bool break_is_backspace;
    bool break_valid;
    s_fmtpara_style style;
};

typedef struct s_fmtpara_global s_fmtpara_global, *Ps_fmtpara_global;

struct s_fmtpara_global {
    int16_t min_x;
    int16_t min_y;
    int16_t max_x;
    int16_t max_y;
    void* item_ptr[16];
    int8_t item_type[16];
    void* user_fn;
    bool cond_flag;
    bool use_plural;
};

typedef struct s_fmtpara_para s_fmtpara_para, *Ps_fmtpara_para;

struct s_fmtpara_para {
    int16_t para_y;
    const char* str;
    int16_t x1;
    int16_t x2;
    int16_t y;
    int16_t y_origin;
    int16_t width;
    struct s_animation_header* seg;
    bool do_render;
    int16_t line_start_x;
    int16_t line_right_limit_x;
    int16_t y2_limit;
    undefined field12_0x1b;
    undefined field13_0x1c;
    int16_t line_advance_y;
    int16_t para_advance_y;
    int16_t first_line_indent;
    int16_t right_indent;
    int16_t left_indent;
    int8_t justify_mode;
    int8_t paragraph_break;
    int16_t nest_depth;
    const char* nest_stack[9];
    struct s_fmtpara_line* line;
};

namespace FMTPARA {
    using user_fn = void (*)(s_fmtpara_para*, uint32_t, int32_t);

    extern const char legal_functions[];
    extern const char legal_fn_arg_chars[];
    extern const uint8_t legal_variable_types[6];
    extern uint8_t font_base_offsets[6];
    extern int8_t reset_parameters_lookup[16];
    extern int16_t tabs[11];
    extern s_fmtpara_style saved_superscript;
    extern s_fmtpara_style* style_p;
    extern s_fmtpara_char* char_p;
    extern s_fmtpara_char* end_char_p;
    extern uint8_t last_fg_and_attr;
    extern s_fmtpara_para* para_p;
    extern int16_t insert_case;
    extern s_fmtpara_line* line_p;
    extern s_fmtpara_break* break_p;
    extern s_fmtpara_char* first_char_p;
    extern char justify_spread_flags[24];
    extern uint16_t var_index;
    extern uint8_t last_font_id;
    extern s_fmtpara_global para_global;
    extern uint32_t variable[16];
    extern char tmp_str[16];
    extern char tmp_str2[32];
    extern const char* const ordinal_ending[10];
    extern const char* const ordinal_word[11];
    extern uint32_t user_fn_arg;

    void __cdecl Init();

    void __cdecl Set_Para_Y2();
    void __cdecl Reset_Max_Min_Values();
    void __cdecl Reset_Font_LF();
    void __cdecl Reset_Font_Spacing_Width();

    uint8_t __cdecl Get_Current_Color_Set();

    void __cdecl Reset_Font_Spacing();

    /** Initializes the paragraph variable table to its default internal storage. */
    void __cdecl Initialize_Formatted_Paragraph_System();

    /** Resets one paragraph variable slot to the internal backing array.
     * @param index Variable slot to reset.
     */
    void __cdecl Reset_A_Paragraph_Variable(uint16_t index);

    void __cdecl Pick_Font_Color(uint8_t fg_and_attr);

    void __cdecl Pick_Font_Number(uint8_t font_id);

    void __cdecl Update_Max_Min_Values();

    void __cdecl Set_Max_Min_X(short current_x, s_fmtpara_char* char_info);

    void __cdecl Set_Max_Min_Y(s_fmtpara_char* char_p);

    /** Measures the rendered height of a formatted paragraph without drawing it.
     * @param width Paragraph width in pixels.
     * @param text Source formatted string.
     */
    int16_t __cdecl Get_Formatted_Paragraph_Max_Height(int16_t width, const char* text);

    /** Measures the rendered width of a formatted paragraph without drawing it.
     * @param width Paragraph width in pixels.
     * @param text Source formatted string.
     * @param justify Justification mode.
     */
    int16_t __cdecl Get_Formatted_Paragraph_Max_Width(int16_t width, const char* text, int16_t justify);

    int16_t __cdecl Get_Last_Formatted_Paragraph_Max_Width();

    /** Renders a formatted paragraph and returns the next unread source position.
     * @param x Left edge in pixels.
     * @param y Top edge in pixels.
     * @param width Paragraph width in pixels.
     * @param str Source formatted string.
     * @param justify Justification mode.
     */
    const char* __cdecl Print_Formatted_Paragraph(int16_t x, int16_t y, int16_t width, const char* str, int16_t justify);

    void __cdecl Initialize_Line_Start();

    bool __cdecl Set_Line_Limits();

    bool __cdecl Legalize_Paragraph_Y();

    void __cdecl Record_Break_Position(char char_code);

    void __cdecl Add_Character_To_Line(char ch);

    bool __cdecl Return_To_Last_Break();

    bool __cdecl Previous_Nesting_Level();

    void __cdecl Complete_Line_Data();

    void __cdecl Justify_Line(int8_t mode);

    void __cdecl _Justify_Line_Fully(int16_t extra_space);

    void __cdecl Print_Line();

    void __cdecl Complete_Line();

    void __cdecl Process_Function();

    void __cdecl Paragraph_HT();

    int8_t __cdecl Vertical_Move(int16_t delta);

    int8_t __cdecl Vertical_Move_Line_Advance();

    void __cdecl Quit_Paragraph();

    bool __cdecl Vertical_Move_Paragraph_Advance();

    void __cdecl Set_Value();

    bool __cdecl Quit_Context();

    void __cdecl Insert_Item();

    void __cdecl Skip_Arg_Insert_A_String(uint16_t arg, bool flag);

    void __cdecl Insert_A_String(const char* new_str);

    uint32_t __cdecl Get_Item_Value_Set_Plural(int16_t item_index);

    void __cdecl Insert_String(const char* str);

    void __cdecl Insert_Next_Argument_String(bool flag);

    void __cdecl Test_Item();

    uint32_t __cdecl Get_Next_Item_Value();

    uint32_t __cdecl Get_Item_Value(uint16_t index);

    bool __cdecl Get_Insert_Argument_Next(uint16_t* value);

    bool __cdecl Set_Item_Value(uint16_t index, uint32_t value);

    void __cdecl Set_Justification();

    void __cdecl Set_Current_Colors();

    void __cdecl Set_Current_Style();

    void __cdecl Reset_Parameters();

    bool __cdecl Get_Function_Argument(int32_t* val, bool* flag);

    char __cdecl FN_Set_Variable(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag);

    int8_t __cdecl FN_User_Call(int8_t func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag);

    char __cdecl FN_Set_Font(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag);

    char __cdecl FN_Set_H_Spacing(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag);

    char __cdecl FN_Set_Indent(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag);

    char __cdecl FN_Set_Margins(char status, int32_t margin_val, int16_t side, bool skip_logic, bool flag);

    char __cdecl FN_Set_Origin(char func_char, int32_t value, int16_t origin_axis, bool parse_failed, bool relative);

    char __cdecl FN_Set_FF_Size(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag);

    char __cdecl FN_Set_Space_Size(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag);

    char __cdecl FN_Set_Tabs(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag);

    char __cdecl FN_Set_V_Spacing(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag);

    char __cdecl FN_Set_X_Pos(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag);

    char __cdecl FN_Set_Y_Pos(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag);

    char __cdecl FN_Superscript(char func_char, int32_t val, int16_t arg_index, bool arg_ret, bool flag);

    const char* __cdecl Skip_Spaces(const char* str);

    int __cdecl _Comp_Tabs(const void* a, const void* b);

    void __cdecl Sort_Tabs();

    bool __cdecl Process_Command(char command_char);

    void __cdecl Process_Paragraph_Text();

    /** Lays out or renders a formatted paragraph and returns the next unread source position.
     * @param x Left edge in pixels.
     * @param y Top edge in pixels.
     * @param width Paragraph width in pixels.
     * @param str Source formatted string.
     * @param justify Justification mode.
     * @param seg Destination bitmap, or nullptr to draw to the active screen.
     * @param do_render False to measure only, true to render glyphs.
     */
    const char* __cdecl _Print_Formatted_Paragraph(int16_t x, int16_t y, int16_t width, const char* str, int16_t justify, s_animation_header* seg, bool do_render);

    /** Binds a signed 16-bit paragraph variable slot to external storage.
     * @param index Variable slot index.
     * @param value_ptr Pointer to the bound value.
     */
    void __cdecl Set_Paragraph_Sint_Pointer(uint16_t index, void* value_ptr);

    /** Binds a 32-bit paragraph variable slot to external storage.
     * @param index Variable slot index.
     * @param value_ptr Pointer to the bound value.
     */
    void __cdecl Set_Paragraph_Integer_Pointer(uint16_t index, void* value_ptr);

    /** Binds a paragraph variable slot to caller-provided storage and a tagged value type.
     * @param item_index Variable slot index.
     * @param value_ptr Pointer to the bound value.
     * @param value_type Tagged storage type used by Get_Item_Value and Set_Item_Value.
     */
    void __cdecl Set_Paragraph_Variable_Pointer(uint16_t item_index, void* value_ptr, int8_t value_type);

    /** Renders a formatted paragraph into a destination bitmap.
     * @param x Left edge in pixels.
     * @param y Top edge in pixels.
     * @param width Paragraph width in pixels.
     * @param str Source formatted string.
     * @param justify Justification mode.
     * @param seg Destination bitmap.
     */
    void __cdecl Print_Formatted_Paragraph_To_Bitmap(int16_t x, int16_t y, int16_t width, const char* str, int16_t justify, s_animation_header* seg);
} // namespace FMTPARA