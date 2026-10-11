#include "pch.h"
#include "game/boarding.h"

namespace BOARDING {
    uint8_t telepathic_multiple_capture = 0;
    uint8_t kill_excess_marines_after_capture = 0;
    uint8_t board_in_black_hole = 1;
    s_boarding boarding = {3, 0, 50, 2};
    s_board_range board_range = {3, 12};
} // namespace BOARDING