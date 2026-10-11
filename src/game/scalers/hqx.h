#pragma once

#include <cstdint>

#include "game/scalers/scaler.h"

namespace hqx {
    bool Initialize();

    bool Scale(
        const uint32_t* source,
        int32_t source_width,
        int32_t source_height,
        int32_t source_stride,
        uint32_t* destination,
        int32_t destination_stride,
        int32_t factor);

    bool Scale_Region(
        const uint32_t* source,
        int32_t source_width,
        int32_t source_height,
        int32_t source_stride,
        uint32_t* destination,
        int32_t destination_stride,
        int32_t factor,
        const scalers::s_region* region);
} // namespace hqx