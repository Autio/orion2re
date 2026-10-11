#include "pch.h"
#include "game/spy.h"

namespace spy {
    s_spy_success success = {80, 90, 70, 90};
    s_spy_count_bonus count_bonus = {5, 10};
    s_spy_random_bonus random_bonus = {100, 100};
    s_spy_guaranteed_roll guaranteed_roll = {100, 100};
    uint8_t telepathic_bonus = 10;
    e_steal_mode steal_mode = STEAL_MODE_EXCEPT_CURRENT_FIELD;
    bool frame_anyone = false;
} // namespace spy