#ifndef ORION2RE_LUA_STORAGE_H
#define ORION2RE_LUA_STORAGE_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct lua_State;

namespace orion2re::lua {

    inline constexpr uint16_t LUA_STORAGE_FORMAT_VERSION = 2;

    struct s_lua_storage_reference {
        uint8_t kind = 0;
        int64_t id = 0;
        uint64_t generation = 0;
    };

    enum e_lua_storage_reference_result {
        LUA_STORAGE_REFERENCE_UNSUPPORTED,
        LUA_STORAGE_REFERENCE_OK,
        LUA_STORAGE_REFERENCE_ERROR,
    };

    using t_lua_storage_reference_reader =
        e_lua_storage_reference_result (*)(lua_State* lua_state,
            int index,
            s_lua_storage_reference* reference,
            std::string* diagnostic);
    using t_lua_storage_reference_writer = bool (*)(lua_State* lua_state,
        const s_lua_storage_reference* reference,
        std::string* diagnostic);

    struct s_lua_storage_reference_adapter {
        t_lua_storage_reference_reader read = nullptr;
        t_lua_storage_reference_writer write = nullptr;
    };

    struct s_lua_storage_limits {
        size_t bytes = 4 * 1024 * 1024;
        size_t nodes = 65'536;
        size_t entries = 262'144;
        size_t string_bytes = 1024 * 1024;
        size_t depth = 256;
    };

    bool Serialize_Storage(lua_State* lua_state, int index, const s_lua_storage_limits* limits, std::vector<uint8_t>* bytes, std::string* diagnostic);
    bool Deserialize_Storage(lua_State* lua_state, const uint8_t* bytes, size_t size, const s_lua_storage_limits* limits, std::string* diagnostic);
    bool Serialize_Storage_With_References(lua_State* lua_state, int index, const s_lua_storage_limits* limits, const s_lua_storage_reference_adapter* adapter, std::vector<uint8_t>* bytes, std::string* diagnostic);
    bool Deserialize_Storage_With_References(lua_State* lua_state, const uint8_t* bytes, size_t size, const s_lua_storage_limits* limits, const s_lua_storage_reference_adapter* adapter, std::string* diagnostic);

} // namespace orion2re::lua

#endif