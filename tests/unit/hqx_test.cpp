#include "pch.h"

#include <cstdint>

#include "doctest/doctest.h"
#include "game/scalers/dirty_regions.h"
#include "game/scalers/hqx.h"
#include "game/scalers/mmpx.h"
#include "game/scalers/scaler.h"

namespace {
    uint64_t Hash_Pixels_(const uint32_t* pixels, size_t count) {
        uint64_t hash = UINT64_C(1469598103934665603);
        for (size_t index = 0; index < count; ++index) {
            for (int32_t byte = 0; byte < 4; ++byte) {
                hash ^= (pixels[index] >> (byte * 8)) & 0xff;
                hash *= UINT64_C(1099511628211);
            }
        }
        return hash;
    }
} // namespace

DOCTEST_TEST_SUITE("HQx presentation scaler") {
    DOCTEST_TEST_CASE("HQx output matches FFmpeg reference vectors") {
        DOCTEST_REQUIRE(hqx::Initialize());
        const uint32_t source[20] = {
            0xff102030,
            0xff102030,
            0xffe02010,
            0xffe02010,
            0xff10d040,
            0xff102030,
            0xfff0e010,
            0xffe02010,
            0xff30a0e0,
            0xff10d040,
            0xff8030b0,
            0xfff0e010,
            0xff102030,
            0xff30a0e0,
            0xff30a0e0,
            0xff8030b0,
            0xff8030b0,
            0xfff0e010,
            0xff102030,
            0xff30a0e0,
        };
        const uint64_t reference_hashes[] = {
            UINT64_C(0x02c465351f5ae9e9),
            UINT64_C(0xd14f94a907891833),
            UINT64_C(0x68bfe18255437c7b),
        };

        for (int32_t factor = 2; factor <= 4; ++factor) {
            const int32_t output_width = 5 * factor;
            const int32_t output_height = 4 * factor;
            uint32_t output[5 * 4 * 4 * 4] = {};

            DOCTEST_REQUIRE(hqx::Scale(
                source, 5, 4, 5, output, output_width, factor));
            DOCTEST_CHECK(Hash_Pixels_(
                              output,
                              (size_t)output_width * (size_t)output_height) ==
                reference_hashes[factor - 2]);
        }
    }

    DOCTEST_TEST_CASE("HQx preserves a uniform image and respects padded strides") {
        DOCTEST_REQUIRE(hqx::Initialize());
        constexpr uint32_t color = 0xff3975b1;
        constexpr uint32_t padding = 0x12345678;
        const uint32_t source[] = {
            color,
            color,
            padding,
            color,
            color,
            padding,
        };

        for (int32_t factor = 2; factor <= 4; ++factor) {
            const int32_t output_width = 2 * factor;
            const int32_t output_height = 2 * factor;
            const int32_t output_stride = output_width + 3;
            uint32_t output[(2 * 4 + 3) * (2 * 4)];
            for (size_t index = 0; index < sizeof(output) / sizeof(output[0]); ++index) {
                output[index] = padding;
            }

            DOCTEST_REQUIRE(hqx::Scale(
                source, 2, 2, 3, output, output_stride, factor));
            for (int32_t y = 0; y < output_height; ++y) {
                for (int32_t x = 0; x < output_width; ++x) {
                    DOCTEST_CHECK(output[y * output_stride + x] == color);
                }
                for (int32_t x = output_width; x < output_stride; ++x) {
                    DOCTEST_CHECK(output[y * output_stride + x] == padding);
                }
            }
        }
    }

    DOCTEST_TEST_CASE("HQx rejects invalid buffers, dimensions, strides, and factors") {
        DOCTEST_REQUIRE(hqx::Initialize());
        uint32_t source[4] = {};
        uint32_t destination[64] = {};

        DOCTEST_CHECK_FALSE(hqx::Scale(nullptr, 2, 2, 2, destination, 4, 2));
        DOCTEST_CHECK_FALSE(hqx::Scale(source, 2, 2, 2, nullptr, 4, 2));
        DOCTEST_CHECK_FALSE(hqx::Scale(source, 0, 2, 2, destination, 4, 2));
        DOCTEST_CHECK_FALSE(hqx::Scale(source, 2, 0, 2, destination, 4, 2));
        DOCTEST_CHECK_FALSE(hqx::Scale(source, 2, 2, 1, destination, 4, 2));
        DOCTEST_CHECK_FALSE(hqx::Scale(source, 2, 2, 2, destination, 3, 2));
        DOCTEST_CHECK_FALSE(hqx::Scale(source, 2, 2, 2, destination, 8, 1));
        DOCTEST_CHECK_FALSE(hqx::Scale(source, 2, 2, 2, destination, 8, 5));
        const scalers::s_region empty_region = {0, 0, 0, 1};
        const scalers::s_region outside_region = {1, 1, 2, 2};
        DOCTEST_CHECK_FALSE(hqx::Scale_Region(
            source, 2, 2, 2, destination, 4, 2, nullptr));
        DOCTEST_CHECK_FALSE(hqx::Scale_Region(
            source, 2, 2, 2, destination, 4, 2, &empty_region));
        DOCTEST_CHECK_FALSE(hqx::Scale_Region(
            source, 2, 2, 2, destination, 4, 2, &outside_region));
    }

    DOCTEST_TEST_CASE("dirty regions expand by the exact scaler dependency radius") {
        uint32_t previous[96 * 80] = {};
        uint32_t current[96 * 80] = {};
        current[20 * 96 + 20] = 0xffffffff;
        scalers::s_dirty_workspace workspace = {};
        scalers::s_region regions[scalers::MAX_DIRTY_BLOCKS] = {};
        int32_t region_count = 0;

        DOCTEST_REQUIRE(scalers::Find_Dirty_Regions(
            current, 96, previous, 96, 96, 80, 1, &workspace, regions, scalers::MAX_DIRTY_BLOCKS, &region_count));
        DOCTEST_REQUIRE(region_count == 1);
        DOCTEST_CHECK(regions[0].x == 15);
        DOCTEST_CHECK(regions[0].y == 15);
        DOCTEST_CHECK(regions[0].width == 18);
        DOCTEST_CHECK(regions[0].height == 18);
    }

    DOCTEST_TEST_CASE("dirty regions switch to a full frame at seventy percent") {
        uint32_t previous[160 * 16] = {};
        uint32_t current[160 * 16] = {};
        scalers::s_dirty_workspace workspace = {};
        scalers::s_region regions[scalers::MAX_DIRTY_BLOCKS] = {};
        int32_t region_count = 0;

        for (int32_t block = 0; block < 6; ++block) {
            current[block * 16] = 0xffffffff;
        }
        DOCTEST_REQUIRE(scalers::Find_Dirty_Regions(
            current, 160, previous, 160, 160, 16, 0, &workspace, regions, scalers::MAX_DIRTY_BLOCKS, &region_count));
        DOCTEST_REQUIRE(region_count == 1);
        DOCTEST_CHECK(regions[0].width == 96);

        current[6 * 16] = 0xffffffff;
        DOCTEST_REQUIRE(scalers::Find_Dirty_Regions(
            current, 160, previous, 160, 160, 16, 0, &workspace, regions, scalers::MAX_DIRTY_BLOCKS, &region_count));
        DOCTEST_REQUIRE(region_count == 1);
        DOCTEST_CHECK(regions[0].width == 160);
    }

    DOCTEST_TEST_CASE("dirty region update matches a full rescale") {
        DOCTEST_REQUIRE(hqx::Initialize());
        uint32_t previous[64 * 48] = {};
        uint32_t current[64 * 48] = {};
        uint32_t cached_output[128 * 96] = {};
        uint32_t full_output[128 * 96] = {};
        for (int32_t y = 0; y < 48; ++y) {
            for (int32_t x = 0; x < 64; ++x) {
                previous[y * 64 + x] = ((x / 4 + y / 4) & 1) != 0
                    ? 0xff204080
                    : 0xffc08020;
            }
        }
        memcpy(current, previous, sizeof(current));
        DOCTEST_REQUIRE(hqx::Scale(previous, 64, 48, 64, cached_output, 128, 2));

        current[20 * 64 + 20] = 0xff20d040;
        scalers::s_dirty_workspace workspace = {};
        scalers::s_region regions[scalers::MAX_DIRTY_BLOCKS] = {};
        int32_t region_count = 0;
        DOCTEST_REQUIRE(scalers::Find_Dirty_Regions(
            current, 64, previous, 64, 64, 48, 1, &workspace, regions, scalers::MAX_DIRTY_BLOCKS, &region_count));
        for (int32_t region = 0; region < region_count; ++region) {
            DOCTEST_REQUIRE(hqx::Scale_Region(
                current, 64, 48, 64, cached_output, 128, 2, &regions[region]));
        }
        DOCTEST_REQUIRE(hqx::Scale(current, 64, 48, 64, full_output, 128, 2));
        DOCTEST_CHECK(memcmp(cached_output, full_output, sizeof(full_output)) == 0);
    }

    DOCTEST_TEST_CASE("dirty region scaling changes only the selected output area") {
        DOCTEST_REQUIRE(hqx::Initialize());
        const uint32_t source[16] = {
            0xff000000,
            0xffffffff,
            0xff000000,
            0xffffffff,
            0xffffffff,
            0xff000000,
            0xffffffff,
            0xff000000,
            0xff000000,
            0xffffffff,
            0xff000000,
            0xffffffff,
            0xffffffff,
            0xff000000,
            0xffffffff,
            0xff000000,
        };
        uint32_t full[8 * 8] = {};
        uint32_t partial[8 * 8];
        memset(partial, 0x5a, sizeof(partial));

        DOCTEST_REQUIRE(hqx::Scale(source, 4, 4, 4, full, 8, 2));
        const scalers::s_region region = {1, 1, 2, 2};
        DOCTEST_REQUIRE(hqx::Scale_Region(
            source, 4, 4, 4, partial, 8, 2, &region));

        for (int32_t y = 0; y < 8; ++y) {
            for (int32_t x = 0; x < 8; ++x) {
                if (x >= 2 && x < 6 && y >= 2 && y < 6) {
                    DOCTEST_CHECK(partial[y * 8 + x] == full[y * 8 + x]);
                } else {
                    DOCTEST_CHECK(partial[y * 8 + x] == 0x5a5a5a5a);
                }
            }
        }
    }
}

DOCTEST_TEST_SUITE("Pixel-art scaler families") {
    DOCTEST_TEST_CASE("MMPX2 preserves uniform pixels and supports padded strides") {
        constexpr uint32_t color = 0xff3975b1;
        constexpr uint32_t padding = 0x12345678;
        const uint32_t source[] = {
            color,
            color,
            padding,
            color,
            color,
            padding,
        };
        uint32_t output[7 * 4];
        for (uint32_t& pixel : output) {
            pixel = padding;
        }

        DOCTEST_REQUIRE(mmpx::Scale(source, 2, 2, 3, output, 7));
        for (int32_t y = 0; y < 4; ++y) {
            for (int32_t x = 0; x < 4; ++x) {
                DOCTEST_CHECK(output[y * 7 + x] == color);
            }
            for (int32_t x = 4; x < 7; ++x) {
                DOCTEST_CHECK(output[y * 7 + x] == padding);
            }
        }
    }

    DOCTEST_TEST_CASE("MMPX region updates match full rescaling") {
        uint32_t previous[64 * 48] = {};
        uint32_t current[64 * 48] = {};
        for (int32_t y = 0; y < 48; ++y) {
            for (int32_t x = 0; x < 64; ++x) {
                previous[y * 64 + x] = ((x / 3 + y / 5) & 1) != 0
                    ? 0xff204080
                    : 0xffc08020;
            }
        }

        const scalers::s_region region = {16, 16, 32, 16};
        memcpy(current, previous, sizeof(current));
        current[23 * 64 + 25] = 0xff20d040;

        uint32_t mmpx_cached[128 * 96] = {};
        uint32_t mmpx_full[128 * 96] = {};
        DOCTEST_REQUIRE(mmpx::Scale(previous, 64, 48, 64, mmpx_cached, 128));
        DOCTEST_REQUIRE(mmpx::Scale_Region(current, 64, 48, 64, mmpx_cached, 128, &region));
        DOCTEST_REQUIRE(mmpx::Scale(current, 64, 48, 64, mmpx_full, 128));
        DOCTEST_CHECK(memcmp(mmpx_cached, mmpx_full, sizeof(mmpx_full)) == 0);

        uint32_t mmpx4_intermediate_cached[128 * 96] = {};
        uint32_t mmpx4_intermediate_full[128 * 96] = {};
        uint32_t mmpx4_cached[256 * 192] = {};
        uint32_t mmpx4_full[256 * 192] = {};
        DOCTEST_REQUIRE(mmpx::Scale_4x(
            previous, 64, 48, 64, mmpx4_intermediate_cached, 128, mmpx4_cached, 256));
        DOCTEST_REQUIRE(mmpx::Scale_4x_Region(
            current, 64, 48, 64, mmpx4_intermediate_cached, 128, mmpx4_cached, 256, &region));
        DOCTEST_REQUIRE(mmpx::Scale_4x(
            current, 64, 48, 64, mmpx4_intermediate_full, 128, mmpx4_full, 256));
        DOCTEST_CHECK(memcmp(mmpx4_cached, mmpx4_full, sizeof(mmpx4_full)) == 0);

        const scalers::s_region boundary_region = {24, 16, 16, 16};
        memcpy(current, previous, sizeof(current));
        current[20 * 64 + 19] ^= 0x00ffffff;
        DOCTEST_REQUIRE(mmpx::Scale_4x(
            previous, 64, 48, 64, mmpx4_intermediate_cached, 128, mmpx4_cached, 256));
        DOCTEST_REQUIRE(mmpx::Scale_4x_Region(
            current, 64, 48, 64, mmpx4_intermediate_cached, 128, mmpx4_cached, 256, &boundary_region));
        DOCTEST_REQUIRE(mmpx::Scale_4x(
            current, 64, 48, 64, mmpx4_intermediate_full, 128, mmpx4_full, 256));
        for (int32_t y = boundary_region.y * 4;
             y < (boundary_region.y + boundary_region.height) * 4;
             ++y) {
            DOCTEST_CHECK(memcmp(
                              mmpx4_cached + y * 256 + boundary_region.x * 4,
                              mmpx4_full + y * 256 + boundary_region.x * 4,
                              (size_t)boundary_region.width * 4 * sizeof(uint32_t)) == 0);
        }
    }

    DOCTEST_TEST_CASE("shared pipeline handles scaler changes and partial frames") {
        uint32_t frame[64 * 48] = {};
        for (int32_t y = 0; y < 48; ++y) {
            for (int32_t x = 0; x < 64; ++x) {
                frame[y * 64 + x] = ((x / 4 + y / 4) & 1) != 0
                    ? 0xff102030
                    : 0xffd0b040;
            }
        }

        const scalers::e_scaler scaler_ids[] = {
            scalers::SCALER_PIXELART,
            scalers::SCALER_BILINEAR,
            scalers::SCALER_HQ2X,
            scalers::SCALER_HQ3X,
            scalers::SCALER_HQ4X,
            scalers::SCALER_MMPX2,
            scalers::SCALER_MMPX4,
        };
        scalers::s_pipeline pipeline = {};
        DOCTEST_REQUIRE(scalers::Initialize(&pipeline, 64, 48));

        for (scalers::e_scaler scaler : scaler_ids) {
            DOCTEST_REQUIRE(scalers::Configure(&pipeline, scaler));
            DOCTEST_REQUIRE(scalers::Process(&pipeline, frame, 64));
            DOCTEST_CHECK(scalers::Get_Dirty_Region_Count(&pipeline) == 1);
            DOCTEST_CHECK(scalers::Get_Output_Width(&pipeline) ==
                64 * scalers::Get_Factor(scaler));

            DOCTEST_REQUIRE(scalers::Process(&pipeline, frame, 64));
            DOCTEST_CHECK(scalers::Get_Dirty_Region_Count(&pipeline) == 0);

            frame[20 * 64 + 20] ^= 0x00ffffff;
            DOCTEST_REQUIRE(scalers::Process(&pipeline, frame, 64));
            DOCTEST_CHECK(scalers::Get_Dirty_Region_Count(&pipeline) > 0);

            const int32_t factor = scalers::Get_Factor(scaler);
            const int32_t output_width = 64 * factor;
            const int32_t output_height = 48 * factor;
            uint32_t expected[64 * 48 * 4 * 4] = {};
            if (scaler == scalers::SCALER_HQ2X ||
                scaler == scalers::SCALER_HQ3X ||
                scaler == scalers::SCALER_HQ4X) {
                DOCTEST_REQUIRE(hqx::Scale(frame, 64, 48, 64, expected, output_width, factor));
            } else if (scaler == scalers::SCALER_MMPX2) {
                DOCTEST_REQUIRE(mmpx::Scale(frame, 64, 48, 64, expected, output_width));
            } else if (scaler == scalers::SCALER_MMPX4) {
                uint32_t intermediate[128 * 96] = {};
                DOCTEST_REQUIRE(mmpx::Scale_4x(
                    frame, 64, 48, 64, intermediate, 128, expected, output_width));
            } else {
                memcpy(expected, frame, sizeof(frame));
            }
            DOCTEST_CHECK(memcmp(
                              scalers::Get_Output_Pixels(&pipeline),
                              expected,
                              (size_t)output_width * (size_t)output_height * sizeof(uint32_t)) == 0);
        }

        scalers::Shutdown(&pipeline);
        DOCTEST_CHECK(pipeline.implementation == nullptr);
    }

    DOCTEST_TEST_CASE("scalers reject invalid dimensions, factors, and regions") {
        uint32_t source[4] = {};
        uint32_t destination[144] = {};
        const scalers::s_region invalid_region = {1, 1, 2, 2};

        DOCTEST_CHECK_FALSE(mmpx::Scale(nullptr, 2, 2, 2, destination, 4));
        DOCTEST_CHECK_FALSE(mmpx::Scale(source, 2, 2, 2, nullptr, 4));
        DOCTEST_CHECK_FALSE(mmpx::Scale_Region(
            source, 2, 2, 2, destination, 4, &invalid_region));
        DOCTEST_CHECK_FALSE(mmpx::Scale_4x_Region(
            source, 2, 2, 2, destination, 4, destination, 8, &invalid_region));

        scalers::s_pipeline pipeline = {};
        DOCTEST_CHECK_FALSE(scalers::Initialize(nullptr, 2, 2));
        DOCTEST_CHECK_FALSE(scalers::Initialize(&pipeline, 641, 480));
        DOCTEST_REQUIRE(scalers::Initialize(&pipeline, 2, 2));
        DOCTEST_CHECK_FALSE(scalers::Configure(
            &pipeline, (scalers::e_scaler)UINT8_MAX));
        scalers::Shutdown(&pipeline);
    }
}