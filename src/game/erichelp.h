#pragma once

#include <cstdint>

struct s_help_box;

namespace ERICHELP {

    extern s_help_box colony_xport_pup_help_list[7];

    extern s_help_box occupation_policy_pup_help_list[1];

    extern s_help_box player_attacked_popup_help_list[1];

    extern s_help_box new_colony_confirmation_box_help_list[1];

    extern s_help_box new_game_screen_help_list[11];

    extern s_help_box tax_rate_popup_help_list[1];

    extern s_help_box dragon_new_star_popup_help_list[1];

    extern s_help_box colony_building_queue_screen_help_list[13];

    extern s_help_box colony_summary_screen_help_list[22];

    extern s_help_box colony_screen_help_list[17];

    extern s_help_box planet_selection_popup_help_list[1];

    extern s_help_box officer_information_popup_help_list[1];

    extern s_help_box refit_ship_popup_help_list[7];

    /**
     * Replaces the active help-box list with the supplied entries.
     * @param help_list Help-box array to activate.
     * @param help_box_count Number of entries in help_list.
     */
    void __cdecl Eric_Set_Help_List(s_help_box* help_list, int16_t help_box_count);

    /** Installs the transport popup help regions. */
    void __cdecl Set_Transport_Popup_Help_List();
    /** Installs the tax-rate popup help regions. */
    void __cdecl Set_Tax_Rate_Popup_Help_List();
    /** Installs the colony building queue screen help regions. */
    void __cdecl Set_Building_Queue_Screen_Help_List();
    /** Installs the colony summary screen help regions. */
    void __cdecl Set_Colony_Summary_Screen_Help_List();
    /** Installs the colony screen help regions. */
    void __cdecl Set_Colony_Screen_Help_List();
    /** Installs the new-game screen help regions. */
    void __cdecl Set_New_Game_Screen_Help_List();
    /** Installs the occupation-policy popup help regions. */
    void __cdecl Set_Occupation_Policy_Popup_Help_List();
    /** Installs the dragon new-star popup help regions. */
    void __cdecl Set_Dragon_New_Star_Popup_Help_List();
    /** Installs the player-attacked popup help regions. */
    void __cdecl Set_Player_Attacked_Popup_Help_List();
    /** Installs the new-colony confirmation popup help regions. */
    void __cdecl Set_New_Colony_Confirmation_Box_Help_List();
    /** Installs the planet-selection popup help regions. */
    void __cdecl Set_Planet_Selection_Popup_Help_List();
    /** Currently does not install a dedicated help list for the new-system-discovery popup. */
    void __cdecl Set_New_System_Discovery_Popup_Help_List();
    /** Installs the officer-information popup help regions. */
    void __cdecl Set_Officer_Information_Popup_Help_List();
    /** Installs the refit-ship popup help regions. */
    void __cdecl Set_Refit_Ship_Popup_Help_List();
} // namespace ERICHELP