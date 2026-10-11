#ifndef ORION2RE_GAME_SHA256_H
#define ORION2RE_GAME_SHA256_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace orion2re::crypto {

    using Sha256Digest = std::array<std::uint8_t, 32>;

    Sha256Digest sha256(std::string_view source);
    bool sha256(const void* source,
        std::size_t source_size,
        std::uint8_t* digest,
        std::size_t digest_capacity);
    std::string sha256_hex(std::string_view source);
    std::string sha256_hex(const Sha256Digest& digest);

} // namespace orion2re::crypto

#endif