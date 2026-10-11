#pragma once
#include <cstdint>

struct s_help_box;

namespace BILLHELP {
    // Clears the active help list and installs `help_list` with `count` entries.
    void __cdecl My_Set_Help_List(s_help_box* help_list, int16_t count);

    extern s_help_box info_help_list[3];

    extern s_help_box reference_help_list[1];

    extern s_help_box ref_category_help_list[1];

    extern s_help_box ref_how_to_help_list[1];

    extern s_help_box turn_summary_help_list[1];

    extern s_help_box turn_summary_popup_help_list[1];

    extern s_help_box race_statistics_help_list[1];

    extern s_help_box tech_review_help_list[1];

    extern s_help_box history_graph_help_list[1];

    extern s_help_box technology_select_help_list[3];

    extern s_help_box technology_change_help_list[3];

    // Activates the help list for the history graph screen.
    void __cdecl History_Graph_Help();
    // Activates the help list for the technology review screen.
    void __cdecl Tech_Review_Help();
    // Activates the help list for the race statistics screen.
    void __cdecl Race_Statistics_Help();
    // Activates the help list for the turn summary popup.
    void __cdecl Turn_Summary_Popup_Help();
    // Activates the help list for the turn summary screen.
    void __cdecl Turn_Summary_Help();

    // Activates the help list for the races screen.
    void __cdecl Races_Help();
    // Activates the help list for the technology change screen.
    void __cdecl Technology_Change_Help();
    // Activates the help list for the technology selection screen.
    void __cdecl Technology_Select_Help();
    // Activates the help list for the report screen.
    void __cdecl Report_Help();
    // Activates the help list for the reference screen.
    void __cdecl Reference_Help();
    // Activates the help list for the reference how-to screen.
    void __cdecl Ref_How_To_Help();
    // Activates the help list for the reference category screen.
    void __cdecl Ref_Category_Help();
    // Activates the help list for the right-side technology change list.
    void __cdecl Tech_Change_List_Right_Help();
    // Activates the help list for the right-side technology selection list.
    void __cdecl Tech_Select_List_Right_Help();
    // Activates the help list for the left-side technology change list.
    void __cdecl Tech_Change_List_Left_Help();
    // Activates the help list for the left-side technology selection list.
    void __cdecl Tech_Select_List_Left_Help();

    extern s_help_box technology_list1_help_list[2];

    extern s_help_box technology_list2_help_list[2];

    extern s_help_box technology_list3_help_list[2];

    extern s_help_box technology_list4_help_list[2];

    extern s_help_box races_help_list[MAX_PLAYERS];

    extern s_help_box report_help_list[1];

} // namespace BILLHELP