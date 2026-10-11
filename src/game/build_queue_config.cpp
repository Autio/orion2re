#include "pch.h"
#include "game/build_queue.h"

namespace BUILD_QUEUE {
    s_autobuild_settings autobuild_settings = {};
    uint8_t presets[PRESET_COUNT][INSTRUCTION_CAPACITY] = {};
} // namespace BUILD_QUEUE