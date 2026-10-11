#include "pch.h"

namespace special {

    int16_t __cdecl Range(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
        int16_t dx = x2 - x1;
        int16_t dy = y2 - y1;

        if (dx < 0) {
            dx = -dx;
        }

        if (dy < 0) {
            dy = -dy;
        }

        if (dy < dx) {
            // Path for dx being the major axis
            return (int16_t)(dy / 2 + dx);
        } else {
            // Path for dy being the major axis or equal
            return (int16_t)(dx / 2 + dy);
        }
    }

    void __cdecl Interpolate_Line(int16_t* x_ptr, int16_t* y_ptr, int16_t target_x, int16_t target_y, int16_t step) {
        if (step <= 0) {
            return;
        }

        int16_t current_x = *x_ptr;
        int16_t current_y = *y_ptr;

        // Exit if already at destination
        if (current_x == target_x && current_y == target_y) {
            return;
        }

        int16_t dx16 = (int16_t)(target_x - current_x);
        int16_t dy16 = (int16_t)(target_y - current_y);
        int32_t dx = dx16;
        int32_t dy = dy16;

        int32_t angle_dx = (int32_t)dx16;
        int32_t angle_dy = (int32_t)dy16;

        // Downscale coordinates if delta is too large to fit in Get_Angle precision
        if (dx < -255 || dx > 255 || dy < -255 || dy > 255) {
            angle_dx = (int32_t)(int16_t)(dx16 / 2);
            angle_dy = (int32_t)(int16_t)(dy16 / 2);
        }

        int16_t angle = angle::Get_Angle(angle_dx, angle_dy);

        // Update X coordinate
        int16_t previous_x = *x_ptr;
        int16_t move_x = (int16_t)angle::Cos(angle, step);
        int16_t new_x = (int16_t)(previous_x + move_x);
        *x_ptr = new_x;

        // Clamp to target X
        if (previous_x < target_x) {
            if (new_x > target_x) {
                *x_ptr = target_x;
            }
        } else {
            if (new_x < target_x) {
                *x_ptr = target_x;
            }
        }

        // Update Y coordinate
        int16_t previous_y = *y_ptr;
        int16_t move_y = (int16_t)angle::Sin(angle, step);
        int16_t new_y = (int16_t)(previous_y + move_y);
        *y_ptr = new_y;

        // Clamp to target Y
        if (previous_y < target_y) {
            if (new_y > target_y) {
                *y_ptr = target_y;
            }
        } else {
            if (new_y < target_y) {
                *y_ptr = target_y;
            }
        }
    }

    void __cdecl Absolute_Interpolate_Line(int16_t* x_ptr, int16_t* y_ptr, int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t step) {
        if (step <= 0) {
            return;
        }

        int16_t current_x = *x_ptr;
        if (current_x == x2 && *y_ptr == y2) {
            return;
        }

        int16_t abs_dx = x2 - x1;
        if (abs_dx < 0) {
            abs_dx = -abs_dx;
        }

        int16_t abs_dy = y2 - y1;
        if (abs_dy < 0) {
            abs_dy = -abs_dy;
        }

        if (abs_dy < abs_dx) {
            if (x1 < x2) {
                int16_t new_x = current_x + step;
                *x_ptr = new_x;
                if (new_x > x2) {
                    *x_ptr = x2;
                }
            } else {
                int16_t new_x = current_x - step;
                *x_ptr = new_x;
                if (new_x < x2) {
                    *x_ptr = x2;
                }
            }
            *y_ptr = y1 + (int16_t)(((int32_t)(*x_ptr - x1) * (int32_t)(y2 - y1)) / (int32_t)(x2 - x1));
        } else {
            int16_t curr_y = *y_ptr;
            if (y1 < y2) {
                int16_t new_y = curr_y + step;
                *y_ptr = new_y;
                if (new_y > y2) {
                    *y_ptr = y2;
                }
            } else {
                int16_t new_y = curr_y - step;
                *y_ptr = new_y;
                if (new_y < y2) {
                    *y_ptr = y2;
                }
            }
            *x_ptr = x1 + (int16_t)(((int32_t)(x2 - x1) * (int32_t)(*y_ptr - y1)) / (int32_t)(y2 - y1));
        }
    }

    uint32_t __cdecl isqrt(uint32_t value) {
        if (value >= 0xFFFE0001u) {
            return 0xFFFFu;
        }

        uint32_t low;
        uint32_t high;

        if (value <= 0xFFFFu) {
            low = 0;
            high = 0x100u;
        } else {
            low = 0x100u;
            high = 0xFFFFu;
        }

        while (low + 1 < high) {
            uint32_t mid = (low + high) >> 1;
            uint32_t square = mid * mid;

            if (square <= value) {
                low = mid;
            } else {
                high = mid;
            }
        }

        return low;
    }

} // namespace special