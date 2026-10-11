#pragma once

#include <cstddef>
#include <string>

namespace config {

    bool Encode_Utf8_To_Cp850(const char* text, std::size_t text_size,
        std::string* encoded);
    void Decode_Cp850_To_Utf8(const char* text, std::size_t text_size,
        std::string* decoded);

} // namespace config