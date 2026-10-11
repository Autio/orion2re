#include "pch.h"

namespace {
    s_current_design design;
}

namespace HAROLD {
    const char* __cdecl H_Message(int16_t id, int, const s_save_game_description*) {
        return id == HESTR_065_SPECIAL ? "special" : "";
    }
}

namespace TECHDATA {
    s_tech_weapon_data weapons[WEAPON_COUNT];
}

namespace MOX {
    s_current_design* design = &::design;
    int16_t PLAYER_NUM;
    int16_t weapon_mod_field_status[WEAPON_MOD_COUNT];
}

namespace INITSHIP {
    int16_t __cdecl Best_Bomb_Weapon(int16_t) {
        return 0;
    }

    int16_t __cdecl Best_Point_Defense_Weapon(int16_t) {
        return 0;
    }
}