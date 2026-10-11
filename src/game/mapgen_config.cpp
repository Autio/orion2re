#include "pch.h"

#include "config/config_runtime.h"

namespace MAPGEN {
    namespace {

        bool State_Integer_(const config::s_config_state* state, const char* path, int64_t* value) {
            config::t_variable_id variable_id = config::CONFIG_NO_VARIABLE;
            if (!config::Find_Variable(path, strlen(path), &variable_id) ||
                variable_id >= state->values.size()) {
                return false;
            }
            const config::s_typed_value& state_value = state->values[variable_id];
            *value = state_value.type == config::CONFIG_VALUE_BOOL ? (state_value.bool_value ? 1 : 0) : state_value.integer_value;
            return true;
        }

        bool Positive_Path_Sum_(const config::s_config_state* state, const char* format, int outer_count, int inner_index) {
            int64_t sum = 0;
            char path[128];
            for (int outer = 0; outer < outer_count; ++outer) {
                snprintf(path, sizeof(path), format, outer, inner_index);
                int64_t value = 0;
                if (!State_Integer_(state, path, &value)) return false;
                sum += value;
            }
            return sum > 0;
        }

        bool Has_Home_Climate_Star_Class_(const config::s_config_state* state, int age) {
            char path[128];
            for (int star_class = 0; star_class < STAR_CLASS_BLACK_HOLE; ++star_class) {
                snprintf(path, sizeof(path), "map.star_class_weight.%d.%d", star_class, age);
                int64_t weight = 0;
                if (!State_Integer_(state, path, &weight)) return false;
                if (weight == 0) continue;

                for (int orbit = 0; orbit < 5; ++orbit) {
                    snprintf(path, sizeof(path), "map.environment_group.%d.%d", star_class, orbit);
                    int64_t group = 0;
                    if (!State_Integer_(state, path, &group)) return false;
                    if (group == PLANET_GROUP_LIFE_SUPPORTING) return true;
                }
            }
            return false;
        }

        void Grid_Dimensions_(int size, int64_t star_count, int64_t* grid_x, int64_t* grid_y) {
            static const int64_t fixed_grid[GALAXY_SIZE_COUNT][2] = {
                {5, 4},
                {6, 6},
                {9, 6},
                {9, 8},
                {0, 0},
                {9, 8},
            };
            if (size != GALAXY_SIZE_MAXIMUM) {
                *grid_x = fixed_grid[size][0];
                *grid_y = fixed_grid[size][1];
                return;
            }

            *grid_y = 8;
            while ((((*grid_y * 5) + 3) / 4) * *grid_y < star_count) {
                ++*grid_y;
            }
            *grid_x = ((*grid_y * 5) + 3) / 4;
        }

        bool Valid_Fixed_Layout_(const config::s_config_state* state, int size, int64_t star_count) {
            char path[128];
            int64_t automatic = 0;
            snprintf(path, sizeof(path), "map.layout.%d.automatic", size);
            if (!State_Integer_(state, path, &automatic)) return false;
            if (size == GALAXY_SIZE_MAXIMUM && automatic != 0) return true;

            int64_t width = 0;
            int64_t height = 0;
            int64_t star_spacing = 0;
            snprintf(path, sizeof(path), "map.layout.%d.width", size);
            if (!State_Integer_(state, path, &width)) return false;
            snprintf(path, sizeof(path), "map.layout.%d.height", size);
            if (!State_Integer_(state, path, &height)) return false;
            snprintf(path, sizeof(path), "map.layout.%d.star_spacing", size);
            if (!State_Integer_(state, path, &star_spacing) || star_spacing <= 0) return false;

            const int64_t scaled_width = width * 10 / star_spacing;
            const int64_t scaled_height = height * 10 / star_spacing;
            int64_t vertical_padding = 4;
            if (star_spacing == 10 || star_spacing == 15) {
                vertical_padding = 8;
            } else if (star_spacing == 20) {
                vertical_padding = 6;
            }
            int64_t grid_x = 0;
            int64_t grid_y = 0;
            Grid_Dimensions_(size, star_count, &grid_x, &grid_y);
            return scaled_width >= 42 * grid_x && scaled_width <= INT16_MAX &&
                scaled_height >= (40 + vertical_padding) * grid_y &&
                scaled_height <= INT16_MAX;
        }

        bool Validate_Map_Config_(const config::s_config_state* state) {
            config::t_variable_id map_variable = config::CONFIG_NO_VARIABLE;
            if (!config::Find_Variable("map.star_count.0", strlen("map.star_count.0"), &map_variable)) {
                return true;
            }

            int64_t star_count[GALAXY_SIZE_COUNT];
            char path[128];
            for (int size = 0; size < GALAXY_SIZE_COUNT; ++size) {
                snprintf(path, sizeof(path), "map.star_count.%d", size);
                if (!State_Integer_(state, path, &star_count[size])) return false;
            }
            if (!(star_count[GALAXY_SIZE_SMALL] < star_count[GALAXY_SIZE_MEDIUM] &&
                    star_count[GALAXY_SIZE_MEDIUM] < star_count[GALAXY_SIZE_LARGE] &&
                    star_count[GALAXY_SIZE_LARGE] < star_count[GALAXY_SIZE_HUGE] &&
                    star_count[GALAXY_SIZE_HUGE] < ORIGINAL_MAX_STARS &&
                    star_count[GALAXY_SIZE_MAXIMUM] > star_count[GALAXY_SIZE_HUGE] &&
                    star_count[GALAXY_SIZE_CLUSTER] <= ORIGINAL_MAX_STARS)) {
                return false;
            }

            for (int size = 0; size < GALAXY_SIZE_COUNT; ++size) {
                if (!Valid_Fixed_Layout_(state, size, star_count[size])) return false;

                int64_t maximum_zoom = 0;
                int64_t initial_zoom = 0;
                snprintf(path, sizeof(path), "map.layout.%d.maximum_zoom_level", size);
                if (!State_Integer_(state, path, &maximum_zoom)) return false;
                snprintf(path, sizeof(path), "map.layout.%d.initial_zoom_level", size);
                if (!State_Integer_(state, path, &initial_zoom) || initial_zoom > maximum_zoom) {
                    return false;
                }

                int64_t minimum_nebulas = 0;
                int64_t maximum_nebulas = 0;
                snprintf(path, sizeof(path), "map.nebula_count.%d.minimum", size);
                if (!State_Integer_(state, path, &minimum_nebulas)) return false;
                snprintf(path, sizeof(path), "map.nebula_count.%d.maximum", size);
                if (!State_Integer_(state, path, &maximum_nebulas) ||
                    minimum_nebulas > maximum_nebulas) {
                    return false;
                }
            }

            int64_t previous_threshold = -1;
            for (int planet_size = 0; planet_size < PLANET_SIZE_COUNT; ++planet_size) {
                int64_t threshold = 0;
                snprintf(path, sizeof(path), "map.planet_size_threshold.%d", planet_size);
                if (!State_Integer_(state, path, &threshold) || threshold < previous_threshold) {
                    return false;
                }
                previous_threshold = threshold;
            }
            if (previous_threshold != 10) return false;

            for (int age = 0; age < GALAXY_AGE_COUNT; ++age) {
                if (!Positive_Path_Sum_(state, "map.star_class_weight.%d.%d", STAR_CLASS_COUNT, age) ||
                    !Positive_Path_Sum_(state, "map.star_class_weight.%d.%d", STAR_CLASS_BLACK_HOLE, age) ||
                    !Has_Home_Climate_Star_Class_(state, age) ||
                    !Positive_Path_Sum_(state, "map.orbit_weight.%d.%d", 5, age)) {
                    return false;
                }
            }
            for (int group = 0; group < PLANET_GROUP_COUNT; ++group) {
                if (!Positive_Path_Sum_(state, "map.climate_weight_young_average.%d.%d", PLANET_CLIMATE_COUNT, group) ||
                    !Positive_Path_Sum_(state, "map.climate_weight_old.%d.%d", PLANET_CLIMATE_COUNT, group)) {
                    return false;
                }
            }

            int64_t visual_weight_sum = 0;
            for (int size = 0; size < 3; ++size) {
                int64_t weight = 0;
                snprintf(path, sizeof(path), "map.star_visual_size_weight.%d", size);
                if (!State_Integer_(state, path, &weight)) return false;
                visual_weight_sum += weight;
            }
            return visual_weight_sum > 0;
        }

        const bool map_config_validator_registered = [] {
            config::Register_State_Validator(Validate_Map_Config_);
            return true;
        }();

    } // namespace
} // namespace MAPGEN