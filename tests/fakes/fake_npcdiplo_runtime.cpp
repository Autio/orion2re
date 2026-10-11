#include "pch.h"

void Legacy_Qsort(void*, size_t, size_t, legacy_qsort_compare) {
}

namespace {
    s_player players[MAX_PLAYERS];
    bool player_may_get_tech;
    int32_t tech_value;
}

namespace TEST_NPCDIPLO {
    void Reset_() {
        memset(players, 0, sizeof(players));
        MOX::NUM_PLAYERS = 2;
        player_may_get_tech = true;
        tech_value = 0;
    }

    void Set_Player_May_Get_Tech_(bool may_get) {
        player_may_get_tech = may_get;
    }

    void Set_Tech_Value_(int32_t value) {
        tech_value = value;
    }
}

namespace MOX {
    s_player* player = players;
    int16_t NUM_PLAYERS = 2;
    int32_t stardate = 35000;
    s_settings settings = {};
}

namespace COLCALC {
    bool __cdecl Player_May_Get_Tech_App(s_player*, int) {
        return player_may_get_tech;
    }

    bool __cdecl May_Surrender_Empire(int16_t, int16_t) {
        return true;
    }
}

namespace game_random {
    uint32_t __cdecl Random(uint32_t) {
        return 1;
    }
}

namespace newtech {
    void __cdecl Set_Competition_Tech_Values(int16_t) {
    }

    int32_t __cdecl Calc_Tech_Value(int16_t, int16_t) {
        return tech_value;
    }
}