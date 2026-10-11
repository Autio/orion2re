#include "pch.h"

#include "game/scalers/dirty_regions.h"

namespace scalers {
    namespace {
        bool Block_Changed(const uint32_t* current, int32_t current_stride, const uint32_t* previous, int32_t previous_stride, int32_t x, int32_t y, int32_t width, int32_t height) {
            const size_t row_size = (size_t)width * sizeof(uint32_t);
            for (int32_t row = 0; row < height; ++row) {
                if (memcmp(
                        current + (y + row) * current_stride + x,
                        previous + (y + row) * previous_stride + x,
                        row_size) != 0) {
                    return true;
                }
            }
            return false;
        }
    } // namespace

    bool Find_Dirty_Regions(const uint32_t* current, int32_t current_stride, const uint32_t* previous, int32_t previous_stride, int32_t width, int32_t height, int32_t dependency_radius, s_dirty_workspace* workspace, s_region* regions, int32_t region_capacity, int32_t* region_count) {
        if (current == nullptr || previous == nullptr || workspace == nullptr ||
            regions == nullptr || region_count == nullptr || region_capacity <= 0 ||
            width <= 0 || height <= 0 || width > MAX_SOURCE_WIDTH ||
            height > MAX_SOURCE_HEIGHT || current_stride < width || previous_stride < width ||
            dependency_radius < 0) {
            return false;
        }

        *region_count = 0;
        const int32_t block_columns = (width + DIRTY_BLOCK_SIZE - 1) / DIRTY_BLOCK_SIZE;
        const int32_t block_rows = (height + DIRTY_BLOCK_SIZE - 1) / DIRTY_BLOCK_SIZE;
        const int32_t block_count = block_columns * block_rows;
        memset(workspace->changed, 0, (size_t)block_count);
        int32_t changed_count = 0;

        for (int32_t block_y = 0; block_y < block_rows; ++block_y) {
            const int32_t y = block_y * DIRTY_BLOCK_SIZE;
            const int32_t block_height =
                height - y < DIRTY_BLOCK_SIZE ? height - y : DIRTY_BLOCK_SIZE;
            for (int32_t block_x = 0; block_x < block_columns; ++block_x) {
                const int32_t x = block_x * DIRTY_BLOCK_SIZE;
                const int32_t block_width =
                    width - x < DIRTY_BLOCK_SIZE ? width - x : DIRTY_BLOCK_SIZE;
                if (Block_Changed(
                        current,
                        current_stride,
                        previous,
                        previous_stride,
                        x,
                        y,
                        block_width,
                        block_height)) {
                    workspace->changed[block_y * block_columns + block_x] = 1;
                    ++changed_count;
                }
            }
        }

        if (changed_count == 0) {
            return true;
        }

        if (changed_count * 10 >= block_count * 7) {
            regions[0] = {0, 0, width, height};
            *region_count = 1;
            return true;
        }

        for (int32_t block_y = 0; block_y < block_rows; ++block_y) {
            int32_t block_x = 0;
            while (block_x < block_columns) {
                while (block_x < block_columns &&
                    workspace->changed[block_y * block_columns + block_x] == 0) {
                    ++block_x;
                }
                if (block_x == block_columns) {
                    break;
                }

                const int32_t first_block_x = block_x;
                while (block_x < block_columns &&
                    workspace->changed[block_y * block_columns + block_x] != 0) {
                    ++block_x;
                }

                const int32_t x = first_block_x * DIRTY_BLOCK_SIZE;
                const int32_t region_width =
                    (width < block_x * DIRTY_BLOCK_SIZE ? width : block_x * DIRTY_BLOCK_SIZE) - x;
                const int32_t y = block_y * DIRTY_BLOCK_SIZE;
                const int32_t region_height =
                    height - y < DIRTY_BLOCK_SIZE ? height - y : DIRTY_BLOCK_SIZE;

                if (*region_count > 0) {
                    s_region* previous_region = &regions[*region_count - 1];
                    if (previous_region->x == x && previous_region->width == region_width &&
                        previous_region->y + previous_region->height == y) {
                        previous_region->height += region_height;
                        continue;
                    }
                }
                if (*region_count >= region_capacity) {
                    return false;
                }
                regions[*region_count] = {x, y, region_width, region_height};
                ++*region_count;
            }
        }

        for (int32_t region_index = 0; region_index < *region_count; ++region_index) {
            s_region& region = regions[region_index];
            const int32_t last_x = region.x + region.width;
            const int32_t last_y = region.y + region.height;
            const int32_t expanded_x = region.x > dependency_radius
                ? region.x - dependency_radius
                : 0;
            const int32_t expanded_y = region.y > dependency_radius
                ? region.y - dependency_radius
                : 0;
            const int32_t expanded_last_x = last_x < width - dependency_radius
                ? last_x + dependency_radius
                : width;
            const int32_t expanded_last_y = last_y < height - dependency_radius
                ? last_y + dependency_radius
                : height;
            region = {
                expanded_x,
                expanded_y,
                expanded_last_x - expanded_x,
                expanded_last_y - expanded_y,
            };
        }
        return true;
    }
} // namespace scalers