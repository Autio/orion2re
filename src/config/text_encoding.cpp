#include "config/text_encoding.h"

#include <cstdint>

namespace config {
    namespace {

        constexpr std::uint16_t CP850_EXTENDED[] = {
            0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
            0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
            0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
            0x00FF, 0x00D6, 0x00DC, 0x00F8, 0x00A3, 0x00D8, 0x00D7, 0x0192,
            0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
            0x00BF, 0x00AE, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
            0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x00C1, 0x00C2, 0x00C0,
            0x00A9, 0x2563, 0x2551, 0x2557, 0x255D, 0x00A2, 0x00A5, 0x2510,
            0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x00E3, 0x00C3,
            0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x00A4,
            0x00F0, 0x00D0, 0x00CA, 0x00CB, 0x00C8, 0x0131, 0x00CD, 0x00CE,
            0x00CF, 0x2518, 0x250C, 0x2588, 0x2584, 0x00A6, 0x00CC, 0x2580,
            0x00D3, 0x00DF, 0x00D4, 0x00D2, 0x00F5, 0x00D5, 0x00B5, 0x00FE,
            0x00DE, 0x00DA, 0x00DB, 0x00D9, 0x00FD, 0x00DD, 0x00AF, 0x00B4,
            0x00AD, 0x00B1, 0x2017, 0x00BE, 0x00B6, 0x00A7, 0x00F7, 0x00B8,
            0x00B0, 0x00A8, 0x00B7, 0x00B9, 0x00B3, 0x00B2, 0x25A0, 0x00A0,
        };

        bool Decode_Utf8(const char* text, std::size_t text_size,
            std::size_t* offset, std::uint32_t* codepoint) {
            const std::uint8_t first = static_cast<std::uint8_t>(text[*offset]);
            if (first < 0x80) {
                *codepoint = first;
                ++*offset;
                return true;
            }
            std::size_t count = 0;
            std::uint32_t value = 0;
            std::uint32_t minimum = 0;
            if (first >= 0xC2 && first <= 0xDF) {
                count = 2;
                value = first & 0x1F;
                minimum = 0x80;
            } else if (first >= 0xE0 && first <= 0xEF) {
                count = 3;
                value = first & 0x0F;
                minimum = 0x800;
            } else if (first >= 0xF0 && first <= 0xF4) {
                count = 4;
                value = first & 0x07;
                minimum = 0x10000;
            } else {
                return false;
            }
            if (*offset + count > text_size) {
                return false;
            }
            for (std::size_t index = 1; index < count; ++index) {
                const std::uint8_t next =
                    static_cast<std::uint8_t>(text[*offset + index]);
                if ((next & 0xC0) != 0x80) {
                    return false;
                }
                value = value << 6 | (next & 0x3F);
            }
            if (value < minimum || value > 0x10FFFF ||
                (value >= 0xD800 && value <= 0xDFFF)) {
                return false;
            }
            *offset += count;
            *codepoint = value;
            return true;
        }

        bool Append_Cp850(std::uint32_t codepoint, std::string* output) {
            if (codepoint == 0) {
                return false;
            }
            if (codepoint < 0x80) {
                output->push_back(static_cast<char>(codepoint));
                return true;
            }
            for (std::size_t index = 0;
                 index < sizeof(CP850_EXTENDED) / sizeof(CP850_EXTENDED[0]);
                 ++index) {
                if (CP850_EXTENDED[index] == codepoint) {
                    output->push_back(static_cast<char>(0x80 + index));
                    return true;
                }
            }
            return false;
        }

        void Append_Utf8(std::uint32_t codepoint, std::string* output) {
            if (codepoint < 0x80) {
                output->push_back(static_cast<char>(codepoint));
            } else if (codepoint < 0x800) {
                output->push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
                output->push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            } else {
                output->push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
                output->push_back(
                    static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                output->push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            }
        }

    } // namespace

    bool Encode_Utf8_To_Cp850(const char* text, std::size_t text_size,
        std::string* encoded) {
        if (text == nullptr || encoded == nullptr) {
            return false;
        }
        encoded->clear();
        std::size_t offset = 0;
        while (offset < text_size) {
            std::uint32_t codepoint = 0;
            if (!Decode_Utf8(text, text_size, &offset, &codepoint) ||
                !Append_Cp850(codepoint, encoded)) {
                encoded->clear();
                return false;
            }
        }
        return true;
    }

    void Decode_Cp850_To_Utf8(const char* text, std::size_t text_size,
        std::string* decoded) {
        decoded->clear();
        if (text == nullptr) {
            return;
        }
        for (std::size_t index = 0; index < text_size; ++index) {
            const std::uint8_t value = static_cast<std::uint8_t>(text[index]);
            Append_Utf8(value < 0x80 ? value : CP850_EXTENDED[value - 0x80],
                decoded);
        }
    }

} // namespace config