#include "pch.h"

#include "game/scalers/scaler.h"

#include "game/scalers/dirty_regions.h"
#include "game/scalers/hqx.h"
#include "game/scalers/mmpx.h"

namespace scalers {
    namespace {
        struct s_pipeline_implementation {
            int32_t source_width;
            int32_t source_height;
            int32_t output_width;
            int32_t output_height;
            e_scaler scaler;
            uint32_t* previous_pixels;
            uint32_t* output_pixels;
            uint32_t* intermediate_pixels;
            s_dirty_workspace dirty_workspace;
            s_region dirty_regions[MAX_DIRTY_BLOCKS];
            int32_t dirty_region_count;
            bool configured;
            bool valid;
        };

        s_pipeline_implementation* Implementation(s_pipeline* pipeline) {
            return pipeline != nullptr
                ? (s_pipeline_implementation*)pipeline->implementation
                : nullptr;
        }

        const s_pipeline_implementation* Implementation(const s_pipeline* pipeline) {
            return pipeline != nullptr
                ? (const s_pipeline_implementation*)pipeline->implementation
                : nullptr;
        }

        bool Pixel_Count(int32_t width, int32_t height, size_t* count) {
            if (count == nullptr || width <= 0 || height <= 0 ||
                (size_t)width > SIZE_MAX / (size_t)height) {
                return false;
            }
            *count = (size_t)width * (size_t)height;
            return *count <= SIZE_MAX / sizeof(uint32_t);
        }

        int32_t Dependency_Radius(e_scaler scaler) {
            switch (scaler) {
                case SCALER_HQ2X:
                case SCALER_HQ3X:
                case SCALER_HQ4X:
                    return 1;
                case SCALER_MMPX2:
                    return 3;
                case SCALER_MMPX4:
                    return 5;
                default:
                    return 0;
            }
        }

        bool Scale_Region(s_pipeline_implementation* implementation, const uint32_t* source, int32_t source_stride, const s_region* region) {
            const int32_t factor = Get_Factor(implementation->scaler);
            switch (implementation->scaler) {
                case SCALER_HQ2X:
                case SCALER_HQ3X:
                case SCALER_HQ4X:
                    return hqx::Scale_Region(
                        source,
                        implementation->source_width,
                        implementation->source_height,
                        source_stride,
                        implementation->output_pixels,
                        implementation->output_width,
                        factor,
                        region);
                case SCALER_MMPX2:
                    return mmpx::Scale_Region(
                        source,
                        implementation->source_width,
                        implementation->source_height,
                        source_stride,
                        implementation->output_pixels,
                        implementation->output_width,
                        region);
                case SCALER_MMPX4:
                    return mmpx::Scale_4x_Region(
                        source,
                        implementation->source_width,
                        implementation->source_height,
                        source_stride,
                        implementation->intermediate_pixels,
                        implementation->source_width * 2,
                        implementation->output_pixels,
                        implementation->output_width,
                        region);
                default:
                    for (int32_t y = region->y; y < region->y + region->height; ++y) {
                        memcpy(
                            implementation->output_pixels + y * implementation->output_width + region->x,
                            source + y * source_stride + region->x,
                            (size_t)region->width * sizeof(uint32_t));
                    }
                    return true;
            }
        }
    } // namespace

    bool Initialize(s_pipeline* pipeline, int32_t source_width, int32_t source_height) {
        if (pipeline == nullptr || pipeline->implementation != nullptr ||
            source_width <= 0 || source_height <= 0 ||
            source_width > MAX_SOURCE_WIDTH || source_height > MAX_SOURCE_HEIGHT) {
            return false;
        }

        size_t source_pixel_count = 0;
        if (!Pixel_Count(source_width, source_height, &source_pixel_count)) {
            return false;
        }

        s_pipeline_implementation* implementation =
            (s_pipeline_implementation*)calloc(1, sizeof(s_pipeline_implementation));
        if (implementation == nullptr) {
            return false;
        }
        implementation->previous_pixels =
            (uint32_t*)malloc(source_pixel_count * sizeof(uint32_t));
        if (implementation->previous_pixels == nullptr) {
            free(implementation);
            return false;
        }

        implementation->source_width = source_width;
        implementation->source_height = source_height;
        pipeline->implementation = implementation;
        return true;
    }

    void Shutdown(s_pipeline* pipeline) {
        s_pipeline_implementation* implementation = Implementation(pipeline);
        if (implementation == nullptr) {
            return;
        }

        free(implementation->output_pixels);
        free(implementation->intermediate_pixels);
        free(implementation->previous_pixels);
        free(implementation);
        pipeline->implementation = nullptr;
    }

    bool Configure(s_pipeline* pipeline, e_scaler scaler) {
        s_pipeline_implementation* implementation = Implementation(pipeline);
        const int32_t factor = Get_Factor(scaler);
        if (implementation == nullptr || factor == 0 ||
            implementation->source_width > INT32_MAX / factor ||
            implementation->source_height > INT32_MAX / factor) {
            return false;
        }
        if (implementation->configured && implementation->scaler == scaler) {
            return true;
        }
        if ((scaler == SCALER_HQ2X || scaler == SCALER_HQ3X ||
                scaler == SCALER_HQ4X) &&
            !hqx::Initialize()) {
            return false;
        }

        const int32_t output_width = implementation->source_width * factor;
        const int32_t output_height = implementation->source_height * factor;
        size_t output_pixel_count = 0;
        if (!Pixel_Count(output_width, output_height, &output_pixel_count)) {
            return false;
        }
        uint32_t* output_pixels =
            (uint32_t*)malloc(output_pixel_count * sizeof(uint32_t));
        if (output_pixels == nullptr) {
            return false;
        }

        uint32_t* intermediate_pixels = nullptr;
        if (scaler == SCALER_MMPX4) {
            size_t intermediate_pixel_count = 0;
            if (!Pixel_Count(
                    implementation->source_width * 2,
                    implementation->source_height * 2,
                    &intermediate_pixel_count)) {
                free(output_pixels);
                return false;
            }
            intermediate_pixels =
                (uint32_t*)malloc(intermediate_pixel_count * sizeof(uint32_t));
            if (intermediate_pixels == nullptr) {
                free(output_pixels);
                return false;
            }
        }

        free(implementation->output_pixels);
        free(implementation->intermediate_pixels);
        implementation->output_pixels = output_pixels;
        implementation->intermediate_pixels = intermediate_pixels;
        implementation->output_width = output_width;
        implementation->output_height = output_height;
        implementation->scaler = scaler;
        implementation->configured = true;
        implementation->valid = false;
        implementation->dirty_region_count = 0;
        return true;
    }

    void Invalidate(s_pipeline* pipeline) {
        s_pipeline_implementation* implementation = Implementation(pipeline);
        if (implementation != nullptr) {
            implementation->valid = false;
        }
    }

    bool Process(s_pipeline* pipeline, const uint32_t* source, int32_t source_stride) {
        s_pipeline_implementation* implementation = Implementation(pipeline);
        if (implementation == nullptr || !implementation->configured ||
            source == nullptr || source_stride < implementation->source_width ||
            source_stride > INT32_MAX / implementation->source_height) {
            return false;
        }

        if (!implementation->valid) {
            implementation->dirty_regions[0] = {
                0,
                0,
                implementation->source_width,
                implementation->source_height,
            };
            implementation->dirty_region_count = 1;
        } else if (!Find_Dirty_Regions(
                       source,
                       source_stride,
                       implementation->previous_pixels,
                       implementation->source_width,
                       implementation->source_width,
                       implementation->source_height,
                       Dependency_Radius(implementation->scaler),
                       &implementation->dirty_workspace,
                       implementation->dirty_regions,
                       MAX_DIRTY_BLOCKS,
                       &implementation->dirty_region_count)) {
            implementation->valid = false;
            return false;
        }

        for (int32_t region_index = 0;
             region_index < implementation->dirty_region_count;
             ++region_index) {
            if (!Scale_Region(
                    implementation,
                    source,
                    source_stride,
                    &implementation->dirty_regions[region_index])) {
                implementation->valid = false;
                return false;
            }
        }

        if (implementation->dirty_region_count > 0 || !implementation->valid) {
            for (int32_t region_index = 0;
                 region_index < implementation->dirty_region_count;
                 ++region_index) {
                const s_region& region = implementation->dirty_regions[region_index];
                for (int32_t y = region.y; y < region.y + region.height; ++y) {
                    memcpy(
                        implementation->previous_pixels + y * implementation->source_width + region.x,
                        source + y * source_stride + region.x,
                        (size_t)region.width * sizeof(uint32_t));
                }
            }
            implementation->valid = true;
        }

        const int32_t factor = Get_Factor(implementation->scaler);
        for (int32_t region_index = 0;
             region_index < implementation->dirty_region_count;
             ++region_index) {
            s_region& region = implementation->dirty_regions[region_index];
            region.x *= factor;
            region.y *= factor;
            region.width *= factor;
            region.height *= factor;
        }
        return true;
    }

    bool Is_Configured(const s_pipeline* pipeline, e_scaler scaler) {
        const s_pipeline_implementation* implementation = Implementation(pipeline);
        return implementation != nullptr && implementation->configured &&
            implementation->scaler == scaler;
    }

    bool Uses_Linear_Filter(e_scaler scaler) {
        return scaler == SCALER_BILINEAR;
    }

    int32_t Get_Factor(e_scaler scaler) {
        switch (scaler) {
            case SCALER_PIXELART:
            case SCALER_BILINEAR:
                return 1;
            case SCALER_HQ2X:
            case SCALER_MMPX2:
                return 2;
            case SCALER_HQ3X:
                return 3;
            case SCALER_HQ4X:
            case SCALER_MMPX4:
                return 4;
            default:
                return 0;
        }
    }

    const char* Get_Name(e_scaler scaler) {
        switch (scaler) {
            case SCALER_PIXELART: return "pixelart";
            case SCALER_BILINEAR: return "bilinear";
            case SCALER_HQ2X: return "hq2x";
            case SCALER_HQ3X: return "hq3x";
            case SCALER_HQ4X: return "hq4x";
            case SCALER_MMPX2: return "mmpx2";
            case SCALER_MMPX4: return "mmpx4";
            default: return "unknown";
        }
    }

    const uint32_t* Get_Output_Pixels(const s_pipeline* pipeline) {
        const s_pipeline_implementation* implementation = Implementation(pipeline);
        return implementation != nullptr ? implementation->output_pixels : nullptr;
    }

    int32_t Get_Output_Width(const s_pipeline* pipeline) {
        const s_pipeline_implementation* implementation = Implementation(pipeline);
        return implementation != nullptr ? implementation->output_width : 0;
    }

    int32_t Get_Output_Height(const s_pipeline* pipeline) {
        const s_pipeline_implementation* implementation = Implementation(pipeline);
        return implementation != nullptr ? implementation->output_height : 0;
    }

    int32_t Get_Output_Stride(const s_pipeline* pipeline) {
        return Get_Output_Width(pipeline);
    }

    const s_region* Get_Dirty_Regions(const s_pipeline* pipeline) {
        const s_pipeline_implementation* implementation = Implementation(pipeline);
        return implementation != nullptr ? implementation->dirty_regions : nullptr;
    }

    int32_t Get_Dirty_Region_Count(const s_pipeline* pipeline) {
        const s_pipeline_implementation* implementation = Implementation(pipeline);
        return implementation != nullptr ? implementation->dirty_region_count : 0;
    }
} // namespace scalers