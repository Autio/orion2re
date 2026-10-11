#pragma once

#include <cstdint>

#include "game/scalers/scaler.h"

namespace mmpx {
    bool Scale(
        const uint32_t* source,
        int32_t source_width,
        int32_t source_height,
        int32_t source_stride,
        uint32_t* destination,
        int32_t destination_stride);

    bool Scale_Region(
        const uint32_t* source,
        int32_t source_width,
        int32_t source_height,
        int32_t source_stride,
        uint32_t* destination,
        int32_t destination_stride,
        const scalers::s_region* region);

    bool Scale_4x(
        const uint32_t* source,
        int32_t source_width,
        int32_t source_height,
        int32_t source_stride,
        uint32_t* intermediate,
        int32_t intermediate_stride,
        uint32_t* destination,
        int32_t destination_stride);

    bool Scale_4x_Region(
        const uint32_t* source,
        int32_t source_width,
        int32_t source_height,
        int32_t source_stride,
        uint32_t* intermediate,
        int32_t intermediate_stride,
        uint32_t* destination,
        int32_t destination_stride,
        const scalers::s_region* region);
} // namespace mmpx