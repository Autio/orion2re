#pragma once

#include <cstdint>

namespace scalers {
    enum {
        DIRTY_BLOCK_SIZE = 16,
        MAX_SOURCE_WIDTH = 640,
        MAX_SOURCE_HEIGHT = 480,
        MAX_DIRTY_BLOCK_COLUMNS =
            (MAX_SOURCE_WIDTH + DIRTY_BLOCK_SIZE - 1) / DIRTY_BLOCK_SIZE,
        MAX_DIRTY_BLOCK_ROWS =
            (MAX_SOURCE_HEIGHT + DIRTY_BLOCK_SIZE - 1) / DIRTY_BLOCK_SIZE,
        MAX_DIRTY_BLOCKS = MAX_DIRTY_BLOCK_COLUMNS * MAX_DIRTY_BLOCK_ROWS,
    };

    enum e_scaler : uint8_t {
        SCALER_PIXELART = 0,
        SCALER_BILINEAR = 1,
        SCALER_HQ2X = 2,
        SCALER_HQ3X = 3,
        SCALER_HQ4X = 4,
        SCALER_MMPX2 = 5,
        SCALER_MMPX4 = 11,
    };

    struct s_region {
        int32_t x;
        int32_t y;
        int32_t width;
        int32_t height;
    };

    struct s_pipeline {
        void* implementation;
    };

    bool Initialize(s_pipeline* pipeline, int32_t source_width, int32_t source_height);
    void Shutdown(s_pipeline* pipeline);
    bool Configure(s_pipeline* pipeline, e_scaler scaler);
    void Invalidate(s_pipeline* pipeline);
    bool Process(s_pipeline* pipeline, const uint32_t* source, int32_t source_stride);

    bool Is_Configured(const s_pipeline* pipeline, e_scaler scaler);
    bool Uses_Linear_Filter(e_scaler scaler);
    int32_t Get_Factor(e_scaler scaler);
    const char* Get_Name(e_scaler scaler);

    const uint32_t* Get_Output_Pixels(const s_pipeline* pipeline);
    int32_t Get_Output_Width(const s_pipeline* pipeline);
    int32_t Get_Output_Height(const s_pipeline* pipeline);
    int32_t Get_Output_Stride(const s_pipeline* pipeline);
    const s_region* Get_Dirty_Regions(const s_pipeline* pipeline);
    int32_t Get_Dirty_Region_Count(const s_pipeline* pipeline);
} // namespace scalers