#include "pch.h"

namespace {
    s_player players[1];
    s_ship_data ships[1];
}

namespace TEST_BILL_HISTORY {
    void Reset_() {
        memset(players, 0, sizeof(players));
        memset(ships, 0, sizeof(ships));
        memset(MOX::bill_savegame, 0, sizeof(MOX::bill_savegame));

        MOX::NUM_PLAYERS = 1;
        MOX::NUM_SHIPS = 0;
        MOX::bill_savegame[0] = 2;
        MOX::bill_savegame[1] = 7;
        MOX::bill_savegame[2] = 1;
        MOX::bill_savegame[3] = 1;
        MOX::bill_savegame[4] = 1;
        MOX::bill_savegame[5] = 1;
    }
}

namespace MOX {
    int16_t NUM_PLAYERS;
    int16_t NUM_SHIPS;
    s_player* player = players;
    s_ship_data* ship = ships;
    int16_t bill_savegame[6];
}

namespace COLCALC {
    void __cdecl Compute_Total_Building_Costs(int32_t* building_costs) {
        memset(building_costs, 0, sizeof(int32_t) * MAX_PLAYERS);
    }
}