#include "pch.h"

namespace ERICHELP {

    s_help_box colony_xport_pup_help_list[7] = {
        {514, 449, 53, 460, 72},
        {522, 448, 244, 460, 263},
        {537, 353, 286, 454, 309},
        {521, 446, 75, 458, 242},
        {515, 15, 55, 144, 266},
        {538, 148, 54, 441, 266},
        {539, 1, 1, 471, 323},
    };

    s_help_box occupation_policy_pup_help_list[1] = {
        {543, 0, 0, 639, 479},
    };

    s_help_box player_attacked_popup_help_list[1] = {
        {558, 0, 0, 639, 479},
    };

    s_help_box new_colony_confirmation_box_help_list[1] = {
        {559, 0, 0, 639, 479},
    };

    s_help_box new_game_screen_help_list[11] = {
        {550, 249, 227, 375, 371},
        {544, 409, 83, 524, 225},
        {546, 99, 84, 212, 226},
        {547, 249, 85, 376, 225},
        {552, 99, 228, 212, 371},
        {553, 380, 293, 524, 326},
        {554, 380, 258, 524, 291},
        {555, 380, 328, 524, 362},
        {557, 99, 383, 196, 409},
        {556, 417, 385, 513, 410},
        {545, 0, 0, 639, 479},
    };

    s_help_box tax_rate_popup_help_list[1] = {
        {560, 0, 0, 639, 479},
    };

    s_help_box dragon_new_star_popup_help_list[1] = {
        {561, 0, 0, 639, 479},
    };

    s_help_box colony_building_queue_screen_help_list[13] = {
        {501, 13, 9, 184, 469},
        {502, 203, 9, 285, 103},
        {503, 302, 10, 462, 103},
        {504, 482, 10, 626, 308},
        {505, 204, 114, 461, 302},
        {506, 207, 332, 458, 465},
        {507, 490, 342, 623, 363},
        {508, 492, 379, 552, 397},
        {509, 561, 379, 621, 397},
        {510, 503, 411, 615, 431},
        {537, 493, 447, 554, 465},
        {511, 560, 447, 623, 465},
        {512, 1, 1, 638, 478},
    };

    s_help_box colony_summary_screen_help_list[22] = {
        {514, 619, 15, 627, 34},
        {515, 8, 1, 95, 343},
        {516, 97, 1, 235, 343},
        {517, 237, 1, 375, 343},
        {518, 377, 1, 514, 343},
        {519, 516, 1, 594, 343},
        {520, 596, 1, 616, 343},
        {521, 620, 39, 627, 312},
        {522, 620, 317, 629, 337},
        {523, 9, 349, 91, 439},
        {524, 101, 349, 371, 439},
        {525, 381, 349, 510, 439},
        {526, 520, 349, 631, 439},
        {500, 537, 447, 612, 469},
        {527, 92, 450, 138, 466},
        {528, 140, 450, 217, 466},
        {529, 219, 450, 260, 466},
        {530, 262, 450, 324, 466},
        {531, 326, 450, 392, 466},
        {532, 394, 450, 479, 466},
        {533, 481, 450, 512, 466},
        {513, 1, 1, 638, 478},
    };

    s_help_box colony_screen_help_list[17] = {
        {484, 1, 1, 120, 14},
        {485, 122, 1, 506, 15},
        {486, 508, 1, 638, 15},
        {487, 7, 24, 115, 151},
        {488, 526, 27, 632, 112},
        {489, 124, 29, 304, 57},
        {490, 307, 29, 512, 56},
        {492, 125, 59, 305, 88},
        {493, 125, 88, 305, 118},
        {494, 125, 118, 305, 148},
        {495, 307, 59, 512, 148},
        {496, 1, 443, 176, 478},
        {497, 520, 124, 578, 143},
        {498, 591, 124, 625, 143},
        {499, 552, 425, 638, 448},
        {500, 551, 455, 638, 478},
        {483, 0, 0, 639, 50},
    };

    s_help_box planet_selection_popup_help_list[1] = {
        {563, 0, 0, 639, 479},
    };

    s_help_box officer_information_popup_help_list[1] = {
        {564, 0, 0, 639, 479},
    };

    s_help_box refit_ship_popup_help_list[7] = {
        {514, 468, 46, 478, 66},
        {535, 159, 47, 460, 230},
        {521, 467, 68, 479, 404},
        {536, 159, 239, 459, 424},
        {522, 468, 406, 478, 424},
        {537, 272, 436, 363, 465},
        {534, 1, 1, 638, 478},
    };

    void __cdecl Eric_Set_Help_List(s_help_box* help_list, int16_t help_box_count) {
        fields::Deactivate_Help_List();
        fields::Set_Help_List(help_list, help_box_count);
    }

    void __cdecl Set_Transport_Popup_Help_List() {
        Eric_Set_Help_List(colony_xport_pup_help_list, 7);
    }

    void __cdecl Set_Tax_Rate_Popup_Help_List() {
        Eric_Set_Help_List(tax_rate_popup_help_list, 1);
    }

    void __cdecl Set_Building_Queue_Screen_Help_List() {
        Eric_Set_Help_List(colony_building_queue_screen_help_list, 13);
    }

    void __cdecl Set_Colony_Summary_Screen_Help_List() {
        Eric_Set_Help_List(colony_summary_screen_help_list, 22);
    }

    void __cdecl Set_Colony_Screen_Help_List() {
        Eric_Set_Help_List(colony_screen_help_list, 17);
    }

    void __cdecl Set_New_Game_Screen_Help_List() {
        Eric_Set_Help_List(new_game_screen_help_list, 11);
    }

    void __cdecl Set_Occupation_Policy_Popup_Help_List() {
        Eric_Set_Help_List(occupation_policy_pup_help_list, 1);
    }

    void __cdecl Set_Dragon_New_Star_Popup_Help_List() {
        Eric_Set_Help_List(dragon_new_star_popup_help_list, 1);
    }

    void __cdecl Set_Player_Attacked_Popup_Help_List() {
        Eric_Set_Help_List(player_attacked_popup_help_list, 1);
    }

    void __cdecl Set_New_Colony_Confirmation_Box_Help_List() {
        Eric_Set_Help_List(new_colony_confirmation_box_help_list, 1);
    }

    void __cdecl Set_Planet_Selection_Popup_Help_List() {
        Eric_Set_Help_List(planet_selection_popup_help_list, 1);
    }

    void __cdecl Set_New_System_Discovery_Popup_Help_List() {
    }

    void __cdecl Set_Officer_Information_Popup_Help_List() {
        Eric_Set_Help_List(officer_information_popup_help_list, 1);
    }

    void __cdecl Set_Refit_Ship_Popup_Help_List() {
        Eric_Set_Help_List(refit_ship_popup_help_list, 7);
    }

} // namespace ERICHELP