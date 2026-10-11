#ifndef ORION2RE_GAME_SAVE_SECTIONS_H
#define ORION2RE_GAME_SAVE_SECTIONS_H

#include <cstdint>
#include <string>

namespace SERIAL {
    class Reader_;
    class Writer_;
} // namespace SERIAL

namespace SAVEGAME {

    inline constexpr uint32_t EXTENSION_MAGIC = 0x5845324F;
    inline constexpr uint16_t EXTENSION_FORMAT_VERSION = 1;
    inline constexpr uint16_t EXTENSION_HEADER_SIZE = 16;
    inline constexpr uint16_t SECTION_HEADER_SIZE = 12;
    inline constexpr uint32_t MAX_EXTENSION_SECTIONS = 64;

    inline constexpr uint32_t SECTION_TYPE_CONFIG = 0x464E4F43;
    inline constexpr uint32_t SECTION_TYPE_LUA = 0x2041554C;
    inline constexpr uint16_t SECTION_FORMAT_VERSION = 1;
    inline constexpr uint16_t SECTION_FLAG_REQUIRED = 1;

    struct s_section_view {
        uint32_t type = 0;
        uint16_t version = 0;
        uint16_t flags = 0;
        const uint8_t* data = nullptr;
        uint32_t size = 0;
    };

    uint32_t Extension_Size(uint32_t payload_size, uint32_t section_count);
    void Write_Extension_Header(SERIAL::Writer_* writer,
        uint32_t payload_size,
        uint32_t section_count);
    void Write_Section_Header(SERIAL::Writer_* writer,
        uint32_t type,
        uint16_t version,
        uint16_t flags,
        uint32_t payload_size);
    bool Read_Extension(SERIAL::Reader_* reader,
        s_section_view* sections,
        uint32_t section_capacity,
        uint32_t* section_count,
        std::string* diagnostic);

} // namespace SAVEGAME

#endif