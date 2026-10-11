#include "pch.h"

namespace invasion {
    uint8_t ground_commando_defender_x5 = 0;
    uint8_t ground_commando_attacker_x2 = 0;
    s_settings settings = {
        {
            125,
            115,
            65,
            55,
        },
    };
    s_troop_rules troop = {
        {10, -10, -20},
        {-10, 10, 20, 10, 20, 10, 10, 1, 1, 1},
    };
} // namespace invasion