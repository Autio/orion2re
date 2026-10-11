#include "pch.h"

namespace key {
    namespace {
        int32_t g_last_key_pressed[10];
        uint8_t g_key_pressed;
        uint8_t g_key_write;
        int8_t g_key_read;
    } // namespace

    constexpr int32_t kLastKeyPressedCount = sizeof(g_last_key_pressed) / sizeof(g_last_key_pressed[0]);

    uint8_t Keyboard_Status() {
        platform::Pump_Game_Input_Queue();
        if (g_key_pressed == 1) {
            g_key_pressed = 0;
            return 1;
        }
        return 0;
    }

    int32_t Read_Key() {
        platform::Pump_Game_Input_Queue();

        if (g_key_pressed == 0 && g_key_read == g_key_write) {
            return 0;
        }

        int8_t current_index = g_key_read;
        int32_t key_code = g_last_key_pressed[current_index];
        g_key_read = (current_index + 1) % kLastKeyPressedCount;

        if (g_key_read == g_key_write) {
            g_key_pressed = 0;
        } else {
            g_key_pressed = 1;
        }

        return key_code;
    }

    int32_t Wait_For_Key() {
        int32_t key_code;

        do {
            key_code = Read_Key();
            if (key_code == 0) {
                SDL_Delay(1);
            }
        } while (key_code == 0);

        return key_code;
    }

    int16_t Scan_Key() {
        platform::Pump_Game_Input_Queue();
        if (Keyboard_Status() == 1) {
            int16_t key = (int16_t)Read_Key();
            if (key == 0x0D) {
                return 0x0D;
            }
            return key;
        }
        return 0;
    }

    void Push_Key_Code(uint32_t key_code, uint32_t scan_code) {
        g_key_pressed = 1;
        g_last_key_pressed[g_key_write] = (int32_t)key_code;
        timer::Handle_Animation_Speed_Key(key_code);
        g_key_write = (uint8_t)((g_key_write + 1) % kLastKeyPressedCount);
        g_last_scan_code = scan_code;
    }
} // namespace key