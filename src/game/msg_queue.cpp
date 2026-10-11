#include "pch.h"

namespace MSG_QUEUE {
    int16_t Lowest_Priority_Index(const s_MSG* messages, int16_t count) {
        if (messages == nullptr || count <= 0) {
            return -1;
        }

        int16_t lowest_priority_idx = 0;
        for (int16_t idx = 1; idx < count; ++idx) {
            if (messages[idx].priority < messages[lowest_priority_idx].priority) {
                lowest_priority_idx = idx;
            }
        }

        return lowest_priority_idx;
    }

    void Add(s_MSG* messages, int16_t capacity, uint8_t* count, s_MSG message) {
        if (messages == nullptr || count == nullptr || capacity <= 0) {
            return;
        }

        if (*count > capacity) {
            *count = static_cast<uint8_t>(capacity);
        }

        if (*count < capacity) {
            messages[*count] = message;
            ++*count;
            return;
        }

        const int16_t lowest_priority_idx = Lowest_Priority_Index(messages, *count);
        if (lowest_priority_idx >= 0 && message.priority > messages[lowest_priority_idx].priority) {
            messages[lowest_priority_idx] = message;
        }
    }
} // namespace MSG_QUEUE