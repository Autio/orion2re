#pragma once

#include <cstdint>

namespace config {

    inline unsigned int Count_Trailing_Zero_Bits(uint64_t word) {
        if (word == 0) return 64;

        unsigned int count = 0;
        uint32_t part32 = static_cast<uint32_t>(word);
        if (part32 == 0) {
            count = 32;
            part32 = static_cast<uint32_t>(word >> 32);
        }

        uint16_t part16 = static_cast<uint16_t>(part32);
        if (part16 == 0) {
            count += 16;
            part16 = static_cast<uint16_t>(part32 >> 16);
        }

        while ((part16 & 1) == 0) {
            ++count;
            part16 >>= 1;
        }
        return count;
    }

} // namespace config