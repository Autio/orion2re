#include "lua/lua_storage.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

namespace orion2re::lua {
    namespace {

        constexpr char MAGIC_[] = {'L', 'S', 'T', 'G'};
        static_assert(sizeof(lua_Integer) == sizeof(int64_t));

        enum e_key_kind : uint8_t {
            KEY_BOOLEAN = 1,
            KEY_INTEGER = 2,
            KEY_STRING = 3,
        };

        enum e_value_kind : uint8_t {
            VALUE_BOOLEAN = 1,
            VALUE_INTEGER = 2,
            VALUE_STRING = 3,
            VALUE_TABLE = 4,
            VALUE_REFERENCE = 5,
        };

        struct s_key {
            e_key_kind kind = KEY_BOOLEAN;
            bool boolean_value = false;
            lua_Integer integer_value = 0;
            std::string string_value;
        };

        struct s_value {
            e_value_kind kind = VALUE_BOOLEAN;
            bool boolean_value = false;
            lua_Integer integer_value = 0;
            std::string string_value;
            uint32_t node = 0;
            s_lua_storage_reference reference;
        };

        struct s_entry {
            s_key key;
            s_value value;
        };

        struct s_node {
            std::vector<s_entry> entries;
        };

        struct s_encode_state {
            lua_State* lua_state = nullptr;
            const s_lua_storage_limits* limits = nullptr;
            const s_lua_storage_reference_adapter* adapter = nullptr;
            std::vector<s_node> nodes;
            std::unordered_map<const void*, uint32_t> identities;
            size_t entries = 0;
            std::string diagnostic;
        };

        struct s_decoder {
            const uint8_t* bytes = nullptr;
            size_t size = 0;
            size_t offset = 0;
            const s_lua_storage_limits* limits = nullptr;
            std::string diagnostic;
        };

        bool Valid_Utf8(std::string_view text) {
            size_t index = 0;
            while (index < text.size()) {
                const unsigned char first =
                    (unsigned char)text[index++];
                if (first <= 0x7F) {
                    continue;
                }
                unsigned int remaining = 0;
                uint32_t value = 0;
                uint32_t minimum = 0;
                if (first >= 0xC2 && first <= 0xDF) {
                    remaining = 1;
                    value = first & 0x1F;
                    minimum = 0x80;
                } else if (first >= 0xE0 && first <= 0xEF) {
                    remaining = 2;
                    value = first & 0x0F;
                    minimum = 0x800;
                } else if (first >= 0xF0 && first <= 0xF4) {
                    remaining = 3;
                    value = first & 0x07;
                    minimum = 0x10000;
                } else {
                    return false;
                }
                if (remaining > text.size() - index) {
                    return false;
                }
                for (unsigned int byte = 0; byte < remaining; ++byte) {
                    const unsigned char continuation =
                        (unsigned char)text[index++];
                    if ((continuation & 0xC0) != 0x80) {
                        return false;
                    }
                    value = (value << 6) | (continuation & 0x3F);
                }
                if (value < minimum || value > 0x10FFFF ||
                    (value >= 0xD800 && value <= 0xDFFF)) {
                    return false;
                }
            }
            return true;
        }

        lua_Integer Decode_Integer(uint64_t value) {
            if (value <= (uint64_t)INT64_MAX) {
                return (lua_Integer)value;
            }
            return (lua_Integer)(-1 - (int64_t)(UINT64_MAX - value));
        }

        bool Key_Less(const s_key& left, const s_key& right) {
            if (left.kind != right.kind) {
                return left.kind < right.kind;
            }
            if (left.kind == KEY_BOOLEAN) {
                return left.boolean_value < right.boolean_value;
            }
            if (left.kind == KEY_INTEGER) {
                return left.integer_value < right.integer_value;
            }
            return std::lexicographical_compare(
                (const unsigned char*)left.string_value.data(),
                (const unsigned char*)left.string_value.data() +
                    left.string_value.size(),
                (const unsigned char*)right.string_value.data(),
                (const unsigned char*)right.string_value.data() +
                    right.string_value.size());
        }

        bool Read_String(lua_State* lua_state, int index, const s_lua_storage_limits* limits, std::string* output, std::string* diagnostic) {
            size_t size = 0;
            const char* value = lua_tolstring(lua_state, index, &size);
            if (size > limits->string_bytes) {
                *diagnostic = "Lua storage string byte limit exceeded";
                return false;
            }
            std::string_view text(value, size);
            if (!Valid_Utf8(text)) {
                *diagnostic = "Lua storage strings must contain valid UTF-8";
                return false;
            }
            output->assign(text);
            return true;
        }

        bool Read_Key(s_encode_state* state, int index, s_key* key) {
            lua_State* lua_state = state->lua_state;
            switch (lua_type(lua_state, index)) {
                case LUA_TBOOLEAN:
                    key->kind = KEY_BOOLEAN;
                    key->boolean_value = lua_toboolean(lua_state, index) != 0;
                    return true;
                case LUA_TNUMBER:
                    if (!lua_isinteger(lua_state, index)) {
                        state->diagnostic =
                            "Lua storage table keys must not be floating-point";
                        return false;
                    }
                    key->kind = KEY_INTEGER;
                    key->integer_value = lua_tointeger(lua_state, index);
                    return true;
                case LUA_TSTRING:
                    key->kind = KEY_STRING;
                    return Read_String(lua_state, index, state->limits, &key->string_value, &state->diagnostic);
                default:
                    state->diagnostic =
                        "Lua storage table keys must be Boolean, integer, or string";
                    return false;
            }
        }

        void Push_Key(lua_State* lua_state, const s_key& key) {
            if (key.kind == KEY_BOOLEAN) {
                lua_pushboolean(lua_state, key.boolean_value);
            } else if (key.kind == KEY_INTEGER) {
                lua_pushinteger(lua_state, key.integer_value);
            } else {
                lua_pushlstring(lua_state, key.string_value.data(), key.string_value.size());
            }
        }

        bool Encode_Table(s_encode_state* state, int index, size_t depth, uint32_t* node_id);

        bool Read_Value(s_encode_state* state, int index, size_t depth, s_value* value) {
            lua_State* lua_state = state->lua_state;
            switch (lua_type(lua_state, index)) {
                case LUA_TBOOLEAN:
                    value->kind = VALUE_BOOLEAN;
                    value->boolean_value = lua_toboolean(lua_state, index) != 0;
                    return true;
                case LUA_TNUMBER:
                    if (!lua_isinteger(lua_state, index)) {
                        state->diagnostic =
                            "Lua storage values must not be floating-point";
                        return false;
                    }
                    value->kind = VALUE_INTEGER;
                    value->integer_value = lua_tointeger(lua_state, index);
                    return true;
                case LUA_TSTRING:
                    value->kind = VALUE_STRING;
                    return Read_String(lua_state, index, state->limits, &value->string_value, &state->diagnostic);
                case LUA_TTABLE:
                    value->kind = VALUE_TABLE;
                    return Encode_Table(state, index, depth, &value->node);
                case LUA_TUSERDATA: {
                    if (state->adapter == nullptr ||
                        state->adapter->read == nullptr) {
                        break;
                    }
                    const e_lua_storage_reference_result result =
                        state->adapter->read(lua_state,
                            index,
                            &value->reference,
                            &state->diagnostic);
                    if (result == LUA_STORAGE_REFERENCE_OK) {
                        value->kind = VALUE_REFERENCE;
                        return true;
                    }
                    if (result == LUA_STORAGE_REFERENCE_ERROR) {
                        return false;
                    }
                    break;
                }
                default:
                    break;
            }
            state->diagnostic =
                "Lua storage values support only Boolean, integer, string, table, and stable engine reference";
            return false;
        }

        bool Encode_Table(s_encode_state* state, int index, size_t depth, uint32_t* node_id) {
            if (depth >= state->limits->depth) {
                state->diagnostic = "Lua storage traversal depth limit exceeded";
                return false;
            }
            index = lua_absindex(state->lua_state, index);
            const void* identity = lua_topointer(state->lua_state, index);
            const auto found = state->identities.find(identity);
            if (found != state->identities.end()) {
                *node_id = found->second;
                return true;
            }
            if (state->nodes.size() >= state->limits->nodes ||
                state->nodes.size() >= std::numeric_limits<uint32_t>::max()) {
                state->diagnostic = "Lua storage graph node limit exceeded";
                return false;
            }
            if (lua_getmetatable(state->lua_state, index) != 0) {
                lua_pop(state->lua_state, 1);
                state->diagnostic = "Lua storage tables must not have metatables";
                return false;
            }

            *node_id = (uint32_t)state->nodes.size();
            state->identities.emplace(identity, *node_id);
            state->nodes.emplace_back();
            std::vector<s_key> keys;
            lua_pushnil(state->lua_state);
            while (lua_next(state->lua_state, index) != 0) {
                lua_pop(state->lua_state, 1);
                if (state->entries >= state->limits->entries) {
                    lua_pop(state->lua_state, 1);
                    state->diagnostic = "Lua storage table entry limit exceeded";
                    return false;
                }
                ++state->entries;
                s_key key;
                if (!Read_Key(state, -1, &key)) {
                    lua_pop(state->lua_state, 1);
                    return false;
                }
                keys.push_back(std::move(key));
            }
            std::sort(keys.begin(), keys.end(), Key_Less);

            std::vector<s_entry> entries;
            entries.reserve(keys.size());
            for (s_key& key : keys) {
                Push_Key(state->lua_state, key);
                lua_rawget(state->lua_state, index);
                s_entry entry;
                entry.key = std::move(key);
                if (!Read_Value(state, -1, depth + 1, &entry.value)) {
                    lua_pop(state->lua_state, 1);
                    return false;
                }
                lua_pop(state->lua_state, 1);
                entries.push_back(std::move(entry));
            }
            state->nodes[*node_id].entries = std::move(entries);
            return true;
        }

        bool Append(std::vector<uint8_t>* bytes,
            const s_lua_storage_limits* limits,
            const void* data,
            size_t size,
            std::string* diagnostic) {
            if (bytes->size() > limits->bytes ||
                size > limits->bytes - bytes->size()) {
                *diagnostic = "Lua storage canonical byte limit exceeded";
                return false;
            }
            const auto* first = (const uint8_t*)data;
            bytes->insert(bytes->end(), first, first + size);
            return true;
        }

        bool Append_U8(std::vector<uint8_t>* bytes,
            const s_lua_storage_limits* limits,
            uint8_t value,
            std::string* diagnostic) {
            return Append(bytes, limits, &value, sizeof(value), diagnostic);
        }

        bool Append_U16(std::vector<uint8_t>* bytes,
            const s_lua_storage_limits* limits,
            uint16_t value,
            std::string* diagnostic) {
            const uint8_t encoded[] = {
                (uint8_t)(value >> 8),
                (uint8_t)value,
            };
            return Append(bytes, limits, encoded, sizeof(encoded), diagnostic);
        }

        bool Append_U32(std::vector<uint8_t>* bytes,
            const s_lua_storage_limits* limits,
            uint32_t value,
            std::string* diagnostic) {
            const uint8_t encoded[] = {
                (uint8_t)(value >> 24),
                (uint8_t)(value >> 16),
                (uint8_t)(value >> 8),
                (uint8_t)value,
            };
            return Append(bytes, limits, encoded, sizeof(encoded), diagnostic);
        }

        bool Append_U64(std::vector<uint8_t>* bytes,
            const s_lua_storage_limits* limits,
            uint64_t value,
            std::string* diagnostic) {
            uint8_t encoded[8];
            for (unsigned int index = 0; index < 8; ++index) {
                encoded[index] = (uint8_t)(value >> ((7 - index) * 8));
            }
            return Append(bytes, limits, encoded, sizeof(encoded), diagnostic);
        }

        bool Append_String(std::vector<uint8_t>* bytes,
            const s_lua_storage_limits* limits,
            const std::string& value,
            std::string* diagnostic) {
            return value.size() <= std::numeric_limits<uint32_t>::max() &&
                Append_U32(bytes, limits, (uint32_t)value.size(), diagnostic) &&
                Append(bytes, limits, value.data(), value.size(), diagnostic);
        }

        bool Append_Key(std::vector<uint8_t>* bytes,
            const s_lua_storage_limits* limits,
            const s_key& key,
            std::string* diagnostic) {
            if (!Append_U8(bytes, limits, key.kind, diagnostic)) {
                return false;
            }
            if (key.kind == KEY_BOOLEAN) {
                return Append_U8(bytes, limits, key.boolean_value ? 1 : 0, diagnostic);
            }
            if (key.kind == KEY_INTEGER) {
                return Append_U64(bytes, limits, (uint64_t)key.integer_value, diagnostic);
            }
            return Append_String(bytes, limits, key.string_value, diagnostic);
        }

        bool Append_Value(std::vector<uint8_t>* bytes,
            const s_lua_storage_limits* limits,
            const s_value& value,
            std::string* diagnostic) {
            if (!Append_U8(bytes, limits, value.kind, diagnostic)) {
                return false;
            }
            if (value.kind == VALUE_BOOLEAN) {
                return Append_U8(bytes, limits, value.boolean_value ? 1 : 0, diagnostic);
            }
            if (value.kind == VALUE_INTEGER) {
                return Append_U64(bytes, limits, (uint64_t)value.integer_value, diagnostic);
            }
            if (value.kind == VALUE_STRING) {
                return Append_String(bytes, limits, value.string_value, diagnostic);
            }
            if (value.kind == VALUE_TABLE) {
                return Append_U32(bytes, limits, value.node, diagnostic);
            }
            return Append_U8(bytes,
                       limits,
                       value.reference.kind,
                       diagnostic) &&
                Append_U64(bytes,
                    limits,
                    (uint64_t)value.reference.id,
                    diagnostic) &&
                Append_U64(bytes,
                    limits,
                    value.reference.generation,
                    diagnostic);
        }

        bool Read_Bytes(s_decoder* decoder, void* output, size_t size) {
            if (size > decoder->size - decoder->offset) {
                decoder->diagnostic = "truncated Lua storage data";
                return false;
            }
            std::memcpy(output, decoder->bytes + decoder->offset, size);
            decoder->offset += size;
            return true;
        }

        bool Read_U8(s_decoder* decoder, uint8_t* value) {
            return Read_Bytes(decoder, value, sizeof(*value));
        }

        bool Read_U16(s_decoder* decoder, uint16_t* value) {
            uint8_t encoded[2];
            if (!Read_Bytes(decoder, encoded, sizeof(encoded))) {
                return false;
            }
            *value = (uint16_t)(encoded[0] << 8 | encoded[1]);
            return true;
        }

        bool Read_U32(s_decoder* decoder, uint32_t* value) {
            uint8_t encoded[4];
            if (!Read_Bytes(decoder, encoded, sizeof(encoded))) {
                return false;
            }
            *value = (uint32_t)encoded[0] << 24 |
                (uint32_t)encoded[1] << 16 |
                (uint32_t)encoded[2] << 8 | encoded[3];
            return true;
        }

        bool Read_U64(s_decoder* decoder, uint64_t* value) {
            uint8_t encoded[8];
            if (!Read_Bytes(decoder, encoded, sizeof(encoded))) {
                return false;
            }
            *value = 0;
            for (const uint8_t byte : encoded) {
                *value = (*value << 8) | byte;
            }
            return true;
        }

        bool Read_String(s_decoder* decoder, std::string* value) {
            uint32_t size = 0;
            if (!Read_U32(decoder, &size)) {
                return false;
            }
            if (size > decoder->limits->string_bytes ||
                size > decoder->size - decoder->offset) {
                decoder->diagnostic = size > decoder->limits->string_bytes
                    ? "Lua storage string byte limit exceeded"
                    : "truncated Lua storage string";
                return false;
            }
            std::string_view text(
                (const char*)(decoder->bytes + decoder->offset), size);
            if (!Valid_Utf8(text)) {
                decoder->diagnostic =
                    "Lua storage strings must contain valid UTF-8";
                return false;
            }
            value->assign(text);
            decoder->offset += size;
            return true;
        }

        bool Read_Key(s_decoder* decoder, s_key* key) {
            uint8_t kind = 0;
            if (!Read_U8(decoder, &kind) || kind < KEY_BOOLEAN ||
                kind > KEY_STRING) {
                decoder->diagnostic = "invalid Lua storage table key tag";
                return false;
            }
            key->kind = (e_key_kind)kind;
            if (key->kind == KEY_BOOLEAN) {
                uint8_t value = 0;
                if (!Read_U8(decoder, &value) || value > 1) {
                    decoder->diagnostic = "invalid Lua storage Boolean key";
                    return false;
                }
                key->boolean_value = value != 0;
            } else if (key->kind == KEY_INTEGER) {
                uint64_t value = 0;
                if (!Read_U64(decoder, &value)) {
                    return false;
                }
                key->integer_value = Decode_Integer(value);
            } else if (!Read_String(decoder, &key->string_value)) {
                return false;
            }
            return true;
        }

        bool Read_Value(s_decoder* decoder,
            uint16_t version,
            uint32_t node_count,
            s_value* value) {
            uint8_t kind = 0;
            if (!Read_U8(decoder, &kind) || kind < VALUE_BOOLEAN ||
                kind > (version >= 2 ? VALUE_REFERENCE : VALUE_TABLE)) {
                decoder->diagnostic = "invalid Lua storage value tag";
                return false;
            }
            value->kind = (e_value_kind)kind;
            if (value->kind == VALUE_BOOLEAN) {
                uint8_t boolean = 0;
                if (!Read_U8(decoder, &boolean) || boolean > 1) {
                    decoder->diagnostic = "invalid Lua storage Boolean value";
                    return false;
                }
                value->boolean_value = boolean != 0;
            } else if (value->kind == VALUE_INTEGER) {
                uint64_t integer = 0;
                if (!Read_U64(decoder, &integer)) {
                    return false;
                }
                value->integer_value = Decode_Integer(integer);
            } else if (value->kind == VALUE_STRING) {
                if (!Read_String(decoder, &value->string_value)) {
                    return false;
                }
            } else if (value->kind == VALUE_TABLE) {
                if (!Read_U32(decoder, &value->node) ||
                    value->node >= node_count) {
                    decoder->diagnostic =
                        "invalid Lua storage table reference";
                    return false;
                }
            } else {
                uint64_t id = 0;
                if (!Read_U8(decoder, &value->reference.kind) ||
                    value->reference.kind == 0 ||
                    !Read_U64(decoder, &id) ||
                    !Read_U64(decoder, &value->reference.generation)) {
                    decoder->diagnostic =
                        "invalid Lua storage engine reference";
                    return false;
                }
                value->reference.id = Decode_Integer(id);
            }
            return true;
        }

        bool Push_Value(lua_State* lua_state,
            const s_value& value,
            int tables,
            const s_lua_storage_reference_adapter* adapter,
            std::string* diagnostic) {
            if (value.kind == VALUE_BOOLEAN) {
                lua_pushboolean(lua_state, value.boolean_value);
            } else if (value.kind == VALUE_INTEGER) {
                lua_pushinteger(lua_state, value.integer_value);
            } else if (value.kind == VALUE_STRING) {
                lua_pushlstring(lua_state, value.string_value.data(), value.string_value.size());
            } else if (value.kind == VALUE_TABLE) {
                lua_rawgeti(lua_state, tables, value.node + 1);
            } else if (adapter == nullptr || adapter->write == nullptr ||
                !adapter->write(lua_state, &value.reference, diagnostic)) {
                if (diagnostic->empty()) {
                    *diagnostic =
                        "Lua storage engine references are unavailable";
                }
                return false;
            }
            return true;
        }

        bool Validate_Node_Order(const std::vector<s_node>& nodes,
            uint32_t node,
            size_t depth,
            std::vector<bool>* visited,
            uint32_t* next_node,
            const s_lua_storage_limits* limits,
            std::string* diagnostic) {
            if (depth >= limits->depth) {
                *diagnostic = "Lua storage traversal depth limit exceeded";
                return false;
            }
            for (const s_entry& entry : nodes[node].entries) {
                if (entry.value.kind != VALUE_TABLE ||
                    (*visited)[entry.value.node]) {
                    continue;
                }
                if (entry.value.node != *next_node) {
                    *diagnostic =
                        "Lua storage table nodes are not in canonical order";
                    return false;
                }
                (*visited)[entry.value.node] = true;
                ++*next_node;
                if (!Validate_Node_Order(nodes, entry.value.node, depth + 1, visited, next_node, limits, diagnostic)) {
                    return false;
                }
            }
            return true;
        }

        bool Parse_Storage(const uint8_t* bytes, size_t size, const s_lua_storage_limits* limits, std::vector<s_node>* nodes, uint32_t* root, std::string* diagnostic) {
            if (bytes == nullptr || limits == nullptr || nodes == nullptr ||
                root == nullptr || diagnostic == nullptr) {
                return false;
            }
            diagnostic->clear();
            if (size > limits->bytes) {
                *diagnostic = "Lua storage canonical byte limit exceeded";
                return false;
            }
            s_decoder decoder{bytes, size, 0, limits, {}};
            char magic[sizeof(MAGIC_)];
            uint16_t version = 0;
            uint16_t reserved = 0;
            uint32_t node_count = 0;
            if (!Read_Bytes(&decoder, magic, sizeof(magic)) ||
                std::memcmp(magic, MAGIC_, sizeof(magic)) != 0 ||
                !Read_U16(&decoder, &version) ||
                (version != 1 && version != LUA_STORAGE_FORMAT_VERSION) ||
                !Read_U16(&decoder, &reserved) || reserved != 0 ||
                !Read_U32(&decoder, root) ||
                !Read_U32(&decoder, &node_count) || node_count == 0 ||
                node_count > limits->nodes || *root != 0) {
                *diagnostic = decoder.diagnostic.empty()
                    ? "invalid Lua storage header"
                    : decoder.diagnostic;
                return false;
            }

            size_t total_entries = 0;
            try {
                nodes->clear();
                nodes->resize(node_count);
                for (s_node& node : *nodes) {
                    uint32_t entry_count = 0;
                    if (!Read_U32(&decoder, &entry_count) ||
                        entry_count > limits->entries - total_entries) {
                        *diagnostic = decoder.diagnostic.empty()
                            ? "Lua storage table entry limit exceeded"
                            : decoder.diagnostic;
                        return false;
                    }
                    total_entries += entry_count;
                    node.entries.resize(entry_count);
                    for (uint32_t index = 0; index < entry_count; ++index) {
                        s_entry& entry = node.entries[index];
                        if (!Read_Key(&decoder, &entry.key) ||
                            !Read_Value(&decoder,
                                version,
                                node_count,
                                &entry.value)) {
                            *diagnostic = decoder.diagnostic;
                            return false;
                        }
                        if (index != 0 && !Key_Less(node.entries[index - 1].key, entry.key)) {
                            *diagnostic =
                                "Lua storage table keys are not in canonical order";
                            return false;
                        }
                    }
                }
                std::vector<bool> visited(node_count, false);
                visited[0] = true;
                uint32_t next_node = 1;
                if (!Validate_Node_Order(*nodes, 0, 0, &visited, &next_node, limits, diagnostic) ||
                    next_node != node_count) {
                    if (diagnostic->empty()) {
                        *diagnostic =
                            "Lua storage graph contains unreachable nodes";
                    }
                    return false;
                }
            } catch (...) {
                *diagnostic =
                    "unable to allocate Lua storage restoration data";
                return false;
            }
            if (decoder.offset != decoder.size) {
                *diagnostic = "Lua storage data has trailing bytes";
                return false;
            }
            return true;
        }

    } // namespace

    bool Serialize_Storage_With_References(lua_State* lua_state, int index, const s_lua_storage_limits* limits, const s_lua_storage_reference_adapter* adapter, std::vector<uint8_t>* bytes, std::string* diagnostic) {
        if (lua_state == nullptr || limits == nullptr || bytes == nullptr ||
            diagnostic == nullptr || !lua_istable(lua_state, index)) {
            return false;
        }
        bytes->clear();
        diagnostic->clear();
        s_encode_state state;
        state.lua_state = lua_state;
        state.limits = limits;
        state.adapter = adapter;
        uint32_t root = 0;
        try {
            if (!Encode_Table(&state, index, 0, &root)) {
                *diagnostic = state.diagnostic;
                return false;
            }
            if (root != 0 || state.nodes.size() > std::numeric_limits<uint32_t>::max() ||
                !Append(bytes, limits, MAGIC_, sizeof(MAGIC_), diagnostic) ||
                !Append_U16(bytes, limits, LUA_STORAGE_FORMAT_VERSION, diagnostic) ||
                !Append_U16(bytes, limits, 0, diagnostic) ||
                !Append_U32(bytes, limits, root, diagnostic) ||
                !Append_U32(bytes, limits, (uint32_t)state.nodes.size(), diagnostic)) {
                return false;
            }
            for (const s_node& node : state.nodes) {
                if (node.entries.size() >
                        std::numeric_limits<uint32_t>::max() ||
                    !Append_U32(bytes, limits, (uint32_t)node.entries.size(), diagnostic)) {
                    return false;
                }
                for (const s_entry& entry : node.entries) {
                    if (!Append_Key(bytes, limits, entry.key, diagnostic) ||
                        !Append_Value(bytes, limits, entry.value, diagnostic)) {
                        return false;
                    }
                }
            }
        } catch (...) {
            *diagnostic = "unable to allocate Lua storage serialization data";
            bytes->clear();
            return false;
        }
        return true;
    }

    bool Serialize_Storage(lua_State* lua_state, int index, const s_lua_storage_limits* limits, std::vector<uint8_t>* bytes, std::string* diagnostic) {
        return Serialize_Storage_With_References(lua_state,
            index,
            limits,
            nullptr,
            bytes,
            diagnostic);
    }

    bool Deserialize_Storage_With_References(lua_State* lua_state, const uint8_t* bytes, size_t size, const s_lua_storage_limits* limits, const s_lua_storage_reference_adapter* adapter, std::string* diagnostic) {
        if (lua_state == nullptr) {
            return false;
        }
        std::vector<s_node> nodes;
        uint32_t root = 0;
        if (!Parse_Storage(bytes, size, limits, &nodes, &root, diagnostic)) {
            return false;
        }

        lua_createtable(lua_state, (int)nodes.size(), 0);
        const int tables = lua_absindex(lua_state, -1);
        for (size_t index = 0; index < nodes.size(); ++index) {
            lua_createtable(lua_state, 0, (int)std::min<size_t>(nodes[index].entries.size(), (size_t)std::numeric_limits<int>::max()));
            lua_rawseti(lua_state, tables, index + 1);
        }
        for (size_t node = 0; node < nodes.size(); ++node) {
            lua_rawgeti(lua_state, tables, node + 1);
            const int table = lua_gettop(lua_state);
            for (const s_entry& entry : nodes[node].entries) {
                Push_Key(lua_state, entry.key);
                if (!Push_Value(lua_state,
                        entry.value,
                        tables,
                        adapter,
                        diagnostic)) {
                    lua_settop(lua_state, tables - 1);
                    return false;
                }
                lua_rawset(lua_state, table);
            }
            lua_pop(lua_state, 1);
        }
        lua_rawgeti(lua_state, tables, root + 1);
        lua_remove(lua_state, tables);
        return true;
    }

    bool Deserialize_Storage(lua_State* lua_state, const uint8_t* bytes, size_t size, const s_lua_storage_limits* limits, std::string* diagnostic) {
        return Deserialize_Storage_With_References(lua_state,
            bytes,
            size,
            limits,
            nullptr,
            diagnostic);
    }

} // namespace orion2re::lua