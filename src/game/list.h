#pragma once

struct s_animation_header;
struct s_colors;

typedef struct s_list_item s_list_item, *Ps_list_item;

struct s_list_item {
    int16_t x;
    int16_t y;
    uint8_t stop_flag;
    uint8_t is_selectable;
    uint8_t current_app_index;
    char group_name[64];
    uint8_t group_id;
    uint8_t group_color_index;
    uint8_t app_count;
    int16_t app_base_x;
    int16_t app_x_max;
    int16_t app_y_offsets[16];
    uint8_t app_ids[16];
    uint8_t tech_app_ids[16];
    uint8_t tech_app_status[16];
};

typedef struct s_list_state s_list_state, *Ps_list_state;

struct s_list_state {
    int16_t x;
    int16_t y;
    int16_t item_width;
    int16_t y_max;
    int16_t* app_y_table;
    int16_t* app_ymin_offsets;
    int16_t* app_ymax_offsets;
    uint8_t item_spacing;
    uint8_t num_items;
    struct s_list_item* items;
    struct s_list_item* selected_item;
    uint8_t max_page;
    uint8_t cur_page;
    uint8_t group_font;
    uint8_t item_font;
    void* draw_app_cb;
    uint8_t app_x_offset;
    uint8_t marker_color;
    struct s_colors* active_color;
    struct s_colors* group_colors[4];
    struct s_colors* disabled_color;
};

namespace LIST {
    using draw_app_fn = void (*)(int16_t, int16_t, uint8_t);

    /**
     * Draws every item on the current page.
     * @param state List pagination and drawing state.
     */
    void __cdecl Display_List_Page(s_list_state* state);

    /**
     * Positions selected_item at the first entry on the current page.
     * @param state List pagination and selection state.
     */
    void __cdecl Set_List_Page_Pointer(s_list_state* state);

    /**
     * Draws one list item using its group and application colors.
     * @param state List drawing state.
     * @param item Item to render.
     */
    void __cdecl Display_List_Text(s_list_state* state, s_list_item* item);

    /**
     * Creates hidden fields for the items on the current page.
     * @param state List state whose visible items should receive fields.
     */
    int16_t __cdecl Add_Fields_To_List_Page(s_list_state* state);

    /**
     * Updates the up/down page field visibility for the current page.
     * @param state List pagination state.
     * @param up_field_id Field ID for the up-page control.
     * @param down_field_id Field ID for the down-page control.
     */
    void __cdecl Set_List_Up_Down_Field_Drawing(s_list_state* state, int16_t up_field_id, int16_t down_field_id);

    /**
     * Advances to the next page when one exists.
     * @param state List pagination state.
     */
    int8_t __cdecl Next_List_Page(s_list_state* state);

    /**
     * Moves back to the previous page when one exists.
     * @param state List pagination state.
     */
    int8_t __cdecl Previous_List_Page(s_list_state* state);

    /**
     * Marks the entry whose generated field ID matches the requested field.
     * @param state List pagination and selection state.
     * @param target_field_id Hidden-field identifier to select.
     */
    s_list_item* __cdecl Set_Selected_List_Entry(s_list_state* state, int16_t target_field_id);

    /**
     * Returns the currently marked selectable entry on the active page.
     * @param state List pagination and selection state.
     */
    s_list_item* __cdecl Get_Selected_List_Entry(s_list_state* state);

    /**
     * Computes item positions and page boundaries for the list.
     * @param state List layout state and item array.
     */
    void __cdecl Init_List_Data(s_list_state* state);

    /**
     * Draws the selected entry with the active highlight color.
     * @param state List drawing state.
     * @param item Selected item to render.
     */
    void __cdecl Display_Selected_List_Text(s_list_state* state, s_list_item* item);

    /**
     * Moves the current page so the target item becomes visible.
     * @param state List pagination state.
     * @param target_item Item that should become visible.
     */
    int8_t __cdecl Goto_Page_With_Entry(s_list_state* state, s_list_item* target_item);

    /**
     * Selects the first entry with the requested group ID and pages to it.
     * @param state List pagination and selection state.
     * @param head_id Group identifier to search for.
     */
    s_list_item* __cdecl Select_And_Page_By_Head_ID(s_list_state* state, uint8_t head_id);

    /**
     * Draws the selected tech-list entry with the active color.
     * @param list_state Tech-list drawing state.
     * @param item Selected item to render.
     */
    void __cdecl Display_Selected_List_Text_For_Tech_List(s_list_state* list_state, s_list_item* item);
} // namespace LIST