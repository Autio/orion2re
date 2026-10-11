#include "pch.h"

namespace BILLHELP {
    s_help_box info_help_list[3] = {
        {242, 0, 40, 205, 197},
        {243, 0, 198, 205, 479},
        {0, 0, 0, 0, 0},
    };

    s_help_box reference_help_list[1] = {
        {244, 206, 0, 639, 479},
    };

    s_help_box ref_category_help_list[1] = {
        {245, 206, 0, 639, 479},
    };

    s_help_box ref_how_to_help_list[1] = {
        {246, 206, 0, 639, 479},
    };

    s_help_box turn_summary_help_list[1] = {
        {247, 206, 0, 639, 479},
    };

    s_help_box turn_summary_popup_help_list[1] = {
        {247, 0, 0, 639, 479},
    };

    s_help_box race_statistics_help_list[1] = {
        {248, 206, 0, 639, 479},
    };

    s_help_box tech_review_help_list[1] = {
        {249, 206, 0, 639, 479},
    };

    s_help_box history_graph_help_list[1] = {
        {250, 206, 0, 639, 479},
    };

    s_help_box technology_select_help_list[3] = {
        {254, 0, 0, 163, 479},
        {254, 163, 0, 639, 25},
        {254, 163, 449, 639, 479},
    };

    s_help_box technology_change_help_list[3] = {
        {255, 0, 0, 80, 479},
        {255, 557, 0, 639, 479},
        {255, 80, 0, 557, 25},
    };

    s_help_box technology_list1_help_list[2] = {
        {256, 0, 0, 366, 479},
        {256, 634, 0, 639, 479},
    };

    s_help_box technology_list2_help_list[2] = {
        {256, 0, 0, 167, 479},
        {256, 435, 0, 639, 479},
    };

    s_help_box technology_list3_help_list[2] = {
        {256, 0, 0, 285, 479},
        {256, 553, 0, 639, 479},
    };

    s_help_box technology_list4_help_list[2] = {
        {256, 0, 0, 86, 479},
        {256, 354, 0, 639, 479},
    };

    s_help_box races_help_list[8] = {
        {259, 320, 360, 639, 422},
        {260, 429, 444, 515, 460},
        {261, 429, 423, 515, 439},
        {262, 334, 423, 421, 439},
        {572, 334, 444, 421, 460},
        {258, 0, 0, 319, 479},
        {258, 320, 0, 639, 479},
        {0, 0, 0, 0, 0},
    };

    s_help_box report_help_list[1] = {
        {263, 0, 0, 639, 479},
    };

    void __cdecl My_Set_Help_List(s_help_box* help_list, int16_t count) {
        fields::Deactivate_Help_List();
        fields::Set_Help_List(help_list, count);
    }

    void __cdecl History_Graph_Help() {
        info_help_list[2] = history_graph_help_list[0];
        My_Set_Help_List(info_help_list, 3);
    }

    void __cdecl Tech_Review_Help() {
        info_help_list[2] = tech_review_help_list[0];
        My_Set_Help_List(info_help_list, 3);
    }

    void __cdecl Race_Statistics_Help() {
        info_help_list[2] = race_statistics_help_list[0];
        My_Set_Help_List(info_help_list, 3);
    }

    void __cdecl Turn_Summary_Popup_Help() {
        My_Set_Help_List(turn_summary_popup_help_list, 1);
    }

    void __cdecl Turn_Summary_Help() {
        info_help_list[2] = turn_summary_help_list[0];
        My_Set_Help_List(info_help_list, 3);
    }

    void __cdecl Races_Help() {
        My_Set_Help_List(races_help_list, 8);
    }

    void __cdecl Technology_Change_Help() {
        My_Set_Help_List(technology_change_help_list, 3);
    }

    void __cdecl Technology_Select_Help() {
        My_Set_Help_List(technology_select_help_list, 3);
    }

    void __cdecl Report_Help() {
        My_Set_Help_List(report_help_list, 1);
    }

    void __cdecl Reference_Help() {
        info_help_list[2] = reference_help_list[0];
        My_Set_Help_List(info_help_list, 3);
    }

    void __cdecl Ref_How_To_Help() {
        info_help_list[2] = ref_category_help_list[0];
        My_Set_Help_List(info_help_list, 3);
    }

    void __cdecl Ref_Category_Help() {
        info_help_list[2] = ref_how_to_help_list[0];
        My_Set_Help_List(info_help_list, 3);
    }

    void __cdecl Tech_Change_List_Right_Help() {
        My_Set_Help_List(technology_list3_help_list, 2);
    }

    void __cdecl Tech_Select_List_Right_Help() {
        My_Set_Help_List(technology_list1_help_list, 2);
    }

    void __cdecl Tech_Change_List_Left_Help() {
        My_Set_Help_List(technology_list4_help_list, 2);
    }

    void __cdecl Tech_Select_List_Left_Help() {
        My_Set_Help_List(technology_list2_help_list, 2);
    }
} // namespace BILLHELP