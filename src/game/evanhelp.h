#pragma once

#include <cstdint>

struct s_help_box;

namespace EVANHELP {
    extern s_help_box main_screen_help_list[25];

    extern s_help_box static_design_screen_help_list[13];

    extern s_help_box design_screen_help_list[13];

    extern s_help_box main_game_screen_help_list[12];

    extern s_help_box main_menu_screen_help_list[6];

    extern s_help_box multi_player_setup_screen_help_list[10];

    extern s_help_box load_game_screen_help_list[7];

    extern s_help_box options_game_screen_help_list[18];

    extern s_help_box save_game_screen_help_list[7];

    extern s_help_box plntsum_screen_help_list[15];

    extern s_help_box input_box_help_list[1];

    extern s_help_box static_diplomacy_request_help_list[4];

    extern s_help_box static_diplomacy_request_help_list2[4];

    /** Activates the predefined help regions for the load-game screen. */
    void __cdecl Set_Load_Game_Screen_Help_List();

    /** Activates the predefined help regions for the options screen. */
    void __cdecl Set_Options_Game_Screen_Help_List();

    /** Activates the predefined help regions for the save-game screen. */
    void __cdecl Set_Save_Game_Screen_Help_List();

    /**
     * Copies and offsets the diplomacy request help regions before activating them.
     * @param x_offset Horizontal offset applied to each help box.
     * @param y_offset Vertical offset applied to each help box.
     */
    void __cdecl Set_Diplomacy_Request_Help_List(int16_t x_offset, int16_t y_offset);

    /** Activates the predefined help regions for the planet summary screen. */
    void __cdecl Set_PlntSum_Screen_Help_List();

    /**
     * Adjusts colony-officer help regions for the displayed skill rows and activates the list.
     * @param displayed_skill_counts Per-officer displayed skill row counts.
     */
    void __cdecl Set_ColOff_Screen_Help_List(int16_t* displayed_skill_counts);

    /**
     * Adjusts ship-officer help regions and appends ship-slot help boxes starting at the given index.
     * @param displayed_skill_counts Per-officer displayed skill row counts.
     * @param start_index First ship-slot index that needs an individual help box.
     */
    void __cdecl Set_ShipOff_Screen_Help_List(int16_t* displayed_skill_counts, int16_t start_index);

    /** Builds and activates the main screen help list for the current game state. */
    void __cdecl Set_Main_Screen_Help_List();

    /**
     * Adjusts design-screen help regions for variable text heights before activating them.
     * @param lines_1 Extra line count affecting the first variable-height region.
     * @param lines_2 Extra line count affecting the second variable-height region.
     */
    void __cdecl Set_Design_Screen_Help_List(int16_t lines_1, int16_t lines_2);

    /** Selects and activates the officer-screen help list for the current officer screen mode. */
    void __cdecl Set_Officer_Screen_Help();

    /**
     * Loads the application help text for the requested item into the caller buffer.
     * @param dest Destination buffer that receives the help text body.
     * @param dest_size Size of the destination buffer in bytes.
     * @param item_index Help record index to load.
     */
    void __cdecl Get_Application_Description(char* dest, int32_t dest_size, int16_t item_index);

    /** Activates the predefined help regions for the main in-game screen. */
    void __cdecl Set_Main_Game_Screen_Help_List();

    /** Activates the predefined help regions for the multiplayer setup screen. */
    void __cdecl Set_Multi_Player_Setup_Screen_Help_List();

    extern s_help_box* prev_help_struct_pointer;
    extern int16_t prev_help_list_active;
    extern int16_t prev_help_list_count;

    /** Saves the currently active help-list state for later restoration. */
    void __cdecl Save_Help_List();

    /** Activates the single full-screen help region used by the input box. */
    void __cdecl Set_Input_Box_Help_List();

    /** Restores the previously saved help-list state. */
    void __cdecl Restore_Help_List();

    extern s_help_box static_fleet_screen_help_list[32];

    extern s_help_box fleet_screen_help_list[32];

    extern s_help_box static_coloff_screen_help_list[16];

    extern s_help_box coloff_screen_help_list[16];

    extern s_help_box static_shipoff_screen_help_list[33];

    extern s_help_box shipoff_screen_help_list[33];

    /**
     * Appends one help-box entry to a caller-provided help list.
     * @param help_list Destination help list.
     * @param list_count In/out count of populated entries.
     * @param unused Legacy unused parameter retained for call compatibility.
     * @param help_id Help-text identifier for the new region.
     * @param x1 Left coordinate of the help region.
     * @param y1 Top coordinate of the help region.
     * @param x2 Right coordinate of the help region.
     * @param y2 Bottom coordinate of the help region.
     */
    void __cdecl Append_Help_List(s_help_box* help_list, int16_t* list_count, int32_t unused, int16_t help_id, int16_t x1, int16_t y1, int16_t x2, int16_t y2);

    /**
     * Builds the fleet-screen help list and appends ship-slot help boxes starting at the given index.
     * @param start_index First fleet slot index that needs an individual help box.
     */
    void __cdecl Set_Fleet_Screen_Help_List(int16_t start_index);

    extern s_help_box scrap_freighters_help_list[1];

    /** Activates the help region for the scrap-freighters dialog. */
    void __cdecl Set_Scrap_Freighters_Help_List();

    /**
     * Builds and activates help regions for the weapon-replacement dialog.
     * @param total_rows Number of weapon rows currently displayed.
     * @param x Dialog X origin.
     * @param y Dialog Y origin.
     * @param needs_racks Selects the rack-specific help text when nonzero.
     */
    void __cdecl Set_Weapon_Replacement_Screen_Help_List(int16_t total_rows, int16_t x, int16_t y, uint8_t needs_racks);

    extern s_help_box weapon_replacement_screen_help_list[12];

    extern s_help_box static_hotseat_setup_screen_help[4];

    extern s_help_box hotseat_setup_screen_help_list[4];

    /**
     * Copies and offsets the hotseat setup help regions before activating them.
     * @param x_offset Horizontal offset applied to each help box.
     * @param y_offset Vertical offset applied to each help box.
     */
    void __cdecl Set_Hotseat_Setup_Screen_Help_List(int16_t x_offset, int16_t y_offset);

    /**
     * Applies the given coordinate offsets to each help box in the array.
     * @param help_boxes Help-box array to modify in place.
     * @param count Number of entries to adjust.
     * @param offset_x Horizontal offset applied to each entry.
     * @param offset_y Vertical offset applied to each entry.
     */
    void __cdecl Offset_Help_Field_XYs(s_help_box* help_boxes, int16_t count, int16_t offset_x, int16_t offset_y);
} // namespace EVANHELP