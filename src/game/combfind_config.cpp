#include "pch.h"
#include "game/combfind.h"

namespace COMBFIND {
    uint16_t confirm_battle_message_timeout = 32767;
    s_system_discovery_bc system_discovery_bc = {50, 100};
    uint32_t dragon_bribe_bc = 500;
    bool monsters_stay = false;
    uint8_t monsters_stay_chance = 2;
} // namespace COMBFIND