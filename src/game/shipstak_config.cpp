#include "pch.h"
#include "game/shipstak.h"

namespace SHIPSTAK {
    s_scan_bonus scan_bonus = {
        2,
        0,
        2,
        4,
        6,
        2,
        2,
        4,
        6,
        9,
        32000,
        32000,
    };
    s_stealth_bonus stealth_bonus = {
        500,
        500,
        500,
        500,
    };
    e_stealth_size_penalty stealth_size_penalty =
        e_stealth_size_penalty::race_tech;
} // namespace SHIPSTAK