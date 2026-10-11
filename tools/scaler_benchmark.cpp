#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "game/scalers/scaler.h"

namespace {
    using t_clock = std::chrono::steady_clock;

    double Measure_(
        scalers::s_pipeline* pipeline,
        const uint32_t* first_frame,
        const uint32_t* second_frame,
        int32_t iterations,
        bool use_dirty_regions) {
        scalers::Invalidate(pipeline);
        if (!scalers::Process(pipeline, first_frame, 640)) {
            return -1.0;
        }
        const auto start = t_clock::now();
        for (int32_t iteration = 0; iteration < iterations; ++iteration) {
            if (!use_dirty_regions) {
                scalers::Invalidate(pipeline);
            }
            const uint32_t* frame = (iteration & 1) != 0
                ? first_frame
                : second_frame;
            if (!scalers::Process(pipeline, frame, 640)) {
                return -1.0;
            }
        }
        const auto elapsed = std::chrono::duration<double, std::milli>(
            t_clock::now() - start);
        return elapsed.count() / iterations;
    }

    double Measure_Best_(
        scalers::s_pipeline* pipeline,
        const uint32_t* first_frame,
        const uint32_t* second_frame,
        int32_t iterations,
        bool use_dirty_regions) {
        double best = -1.0;
        for (int32_t repetition = 0; repetition < 3; ++repetition) {
            const double measured = Measure_(
                pipeline,
                first_frame,
                second_frame,
                iterations,
                use_dirty_regions);
            if (measured < 0.0) {
                return measured;
            }
            if (best < 0.0 || measured < best) {
                best = measured;
            }
        }
        return best;
    }
} // namespace

int main() {
    std::vector<uint32_t> full_first(640 * 480);
    std::vector<uint32_t> full_second(640 * 480);
    for (int32_t y = 0; y < 480; ++y) {
        for (int32_t x = 0; x < 640; ++x) {
            const uint32_t red = (uint32_t)((x * 13 + y * 3) & 0xff);
            const uint32_t green = (uint32_t)((x * 5 + y * 11) & 0xff);
            const uint32_t blue = (uint32_t)((x * 7 + y * 17) & 0xff);
            full_first[y * 640 + x] =
                0xff000000 | (red << 16) | (green << 8) | blue;
            full_second[y * 640 + x] =
                full_first[y * 640 + x] ^ 0x00ffffff;
        }
    }
    std::vector<uint32_t> sparse_first = full_first;
    std::vector<uint32_t> sparse_second = full_first;
    sparse_second[240 * 640 + 320] ^= 0x00010101;

    const scalers::e_scaler scaler_ids[] = {
        scalers::SCALER_PIXELART,
        scalers::SCALER_BILINEAR,
        scalers::SCALER_HQ2X,
        scalers::SCALER_HQ3X,
        scalers::SCALER_HQ4X,
        scalers::SCALER_MMPX2,
        scalers::SCALER_MMPX4,
    };

    std::puts("scaler,full_no_dirty_ms,full_dirty_ms,sparse_no_dirty_ms,sparse_dirty_ms,idle_no_dirty_ms,idle_dirty_ms,output_mib");
    for (scalers::e_scaler scaler : scaler_ids) {
        scalers::s_pipeline pipeline = {};
        if (!scalers::Initialize(&pipeline, 640, 480) ||
            !scalers::Configure(&pipeline, scaler) ||
            !scalers::Process(&pipeline, full_first.data(), 640)) {
            std::fprintf(stderr, "failed to initialize %s\n", scalers::Get_Name(scaler));
            scalers::Shutdown(&pipeline);
            return 1;
        }

        const int32_t factor = scalers::Get_Factor(scaler);
        const int32_t full_iterations = std::max(8, 32 / factor);
        const double full_no_dirty_ms = Measure_Best_(
            &pipeline, full_first.data(), full_second.data(), full_iterations, false);
        const double full_dirty_ms = Measure_Best_(
            &pipeline, full_first.data(), full_second.data(), full_iterations, true);
        const double sparse_no_dirty_ms = Measure_Best_(
            &pipeline, sparse_first.data(), sparse_second.data(), full_iterations, false);
        const double sparse_dirty_ms = Measure_Best_(
            &pipeline, sparse_first.data(), sparse_second.data(), 200, true);
        const double idle_no_dirty_ms = Measure_Best_(
            &pipeline, full_first.data(), full_first.data(), full_iterations, false);
        const double idle_dirty_ms = Measure_Best_(
            &pipeline, full_first.data(), full_first.data(), 200, true);
        const double output_mib =
            (double)scalers::Get_Output_Width(&pipeline) *
            (double)scalers::Get_Output_Height(&pipeline) * sizeof(uint32_t) /
            (1024.0 * 1024.0);
        std::printf(
            "%s,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.2f\n",
            scalers::Get_Name(scaler),
            full_no_dirty_ms,
            full_dirty_ms,
            sparse_no_dirty_ms,
            sparse_dirty_ms,
            idle_no_dirty_ms,
            idle_dirty_ms,
            output_mib);
        scalers::Shutdown(&pipeline);
    }
    return 0;
}