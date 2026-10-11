#include "pch.h"

namespace LOADER {
    void __cdecl Load_Main_Screen_Pictures() {
        allocate::Get_Remaining_Block_Space(MOX::global_screen_seg);

        MOX::main_game_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 1, MOX::global_screen_seg);
        MOX::main_next_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 2, MOX::global_screen_seg);
        MOX::main_colonies_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 3, MOX::global_screen_seg);
        MOX::main_planets_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 4, MOX::global_screen_seg);
        MOX::main_fleets_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 5, MOX::global_screen_seg);
        MOX::main_zoom_in_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 6, MOX::global_screen_seg);
        MOX::main_zoom_out_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 7, MOX::global_screen_seg);
        MOX::main_leaders_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 8, MOX::global_screen_seg);
        MOX::main_races_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 9, MOX::global_screen_seg);
        MOX::main_info_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 10, MOX::global_screen_seg);
        MOX::fleet_box_selection_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 11, MOX::global_screen_seg);
        MOX::darkened_turn_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 12, MOX::global_screen_seg);

        allocate::Get_Remaining_Block_Space(MOX::global_screen_seg);

        MOX::moveable_box_pict[4] = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 0x52, MOX::global_screen_seg);

        RUSS::Mox_Update();

        allocate::Get_Remaining_Block_Space(MOX::global_screen_seg);

        MOX::moveable_box_pict[14] = nullptr;
        MOX::fleet_box_scroll_up_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 0x3A, MOX::global_screen_seg);
        MOX::fleet_box_scroll_dn_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 0x3B, MOX::global_screen_seg);
        MOX::fleet_box_all_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 0x3C, MOX::global_screen_seg);

        RUSS::Mox_Update();

        MOX::fleet_box_att_xenons_lg_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 0x3D, MOX::global_screen_seg);
        MOX::fleet_box_colonize_lg_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 0x3E, MOX::global_screen_seg);
        MOX::fleet_box_engage_lg_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 0x3F, MOX::global_screen_seg);
        MOX::fleet_box_transport_lg_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 0x40, MOX::global_screen_seg);
        MOX::fleet_box_outpost_lg_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 0x41, MOX::global_screen_seg);
        MOX::fleet_box_close_lg_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 0x42, MOX::global_screen_seg);
        MOX::fleet_box_att_xenons_sm_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 0x43, MOX::global_screen_seg);
        MOX::fleet_box_engage_sm_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 0x44, MOX::global_screen_seg);
        MOX::fleet_box_colonize_sm_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 0x45, MOX::global_screen_seg);
        MOX::fleet_box_transport_sm_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 0x46, MOX::global_screen_seg);
        MOX::fleet_box_outpost_sm_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 0x47, MOX::global_screen_seg);
        MOX::fleet_box_close_sm_button_seg = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", 0x48, MOX::global_screen_seg);

        int32_t picture_idx = 0xF1;
        allocate::Get_Remaining_Block_Space(MOX::global_screen_seg);

        for (int32_t i = 0; i < 6; ++i) {
            for (int32_t j = 0; j < 4; ++j) {
                int32_t index = j + i * 4;
                MOX::hl_antaran_ship_icon_seg[index] = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", picture_idx, MOX::global_screen_seg);
                MOX::antaran_ship_icons[index] = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", picture_idx, MOX::global_screen_seg);
                picture_idx++;
            }
        }

        allocate::Get_Remaining_Block_Space(MOX::global_screen_seg);

        MOX::star_width = 0x21;
        MOX::star_height = 0x21;
        RUSS::Mox_Update();

        int32_t star_picture_idx = 1;
        for (int32_t i = 0; i < 11; ++i) {
            RUSS::Mox_Update();
            MOX::star_colors_seg[i] = (s_animation_header*)farload::Far_Reload_Next("gstar.lbx", star_picture_idx, MOX::global_screen_seg);
            star_picture_idx++;
        }

        SYS::sys_data_buffer = allocate::Allocate_Next_Block(MOX::global_screen_seg, 0xDC);

        int32_t player_light_idx = 0x109;
        for (int32_t i = 0; i < MAX_PLAYERS; ++i) {
            MAINSCR::player_lights_seg[i] = (s_animation_header*)farload::Far_Reload_Next("BUFFER0.LBX", player_light_idx, MOX::global_screen_seg);
            player_light_idx++;
        }

        allocate::Get_Remaining_Block_Space(MOX::global_screen_seg);
    }

    void __cdecl Load_Pictures() {
        Load_Main_Screen_Pictures();
        RUSS::Mox_Update();
        COLONY::Load_Colony_Anims();
        RUSS::Mox_Update();
        TECHINIT::Init_Tech();
        RUSS::Mox_Update();

        void* screen_block = MOX::global_screen_seg;
        int32_t remaining_space = allocate::Get_Remaining_Block_Space(screen_block);

        // Allocate block and leave space for the animation header
        s_animation_header* screen_anim = (s_animation_header*)allocate::Allocate_Next_Block(
            screen_block,
            remaining_space - (int32_t)sizeof(s_animation_header));
        MOX::screen_seg = screen_anim;

        // Initialize the start of the frame offset data area
        memset(Animation_Frame_Offsets_(screen_anim), 0, sizeof(uint32_t) * 5);
    }

    void __cdecl Load_Player_Special_Names() {
        char* const loaded_names = (char*)farload::Far_Reload(
            "RACESTUF.lbx",
            (int32_t)(uint8_t)MOX::settings.language + 8,
            MOX::special_names_seg);

        int16_t name_offset = 0;
        int16_t name_index = 1;

        MOX::special_ability_names[0] = loaded_names;

        do {
            name_offset = (int16_t)(name_offset + (int16_t)strlen(loaded_names + name_offset));

            while (loaded_names[name_offset] == '\0') {
                ++name_offset;
            }

            MOX::special_ability_names[name_index] = loaded_names + name_offset;
            ++name_index;
        } while (name_index < 0x20);
    }
} // namespace LOADER