#include "lua/lua_save.h"
#include "lua/lua_network.h"
#include "lua/lua_game.h"

#include "game/serialization.h"

namespace TEST_LUA_SAVE {
    int finalize_fingerprint_count;
} // namespace TEST_LUA_SAVE

namespace orion2re::lua {

    bool Queue_Lua_Network_Load_State(
        const s_lua_save_snapshot*, std::string*) {
        return true;
    }

    bool Finalize_Registry_Fingerprint_With_Config_Identity(
        s_lua_registry*,
        const std::vector<std::uint8_t>*,
        std::string* diagnostic) {
        ++TEST_LUA_SAVE::finalize_fingerprint_count;
        diagnostic->clear();
        return true;
    }

    void Clear_Queued_Lua_Network_Load_State() {}

    bool Capture_Lua_Save_Snapshot(s_lua_save_snapshot* snapshot,
        std::string* diagnostic) {
        *snapshot = {};
        diagnostic->clear();
        return true;
    }

    bool Finalize_Registry_Fingerprint(s_lua_registry*,
        const config::s_config_state*,
        std::string* diagnostic) {
        ++TEST_LUA_SAVE::finalize_fingerprint_count;
        diagnostic->clear();
        return true;
    }

    uint32_t Lua_Save_Snapshot_Size(const s_lua_save_snapshot*) {
        return sizeof(uint32_t);
    }

    void Write_Lua_Save_Snapshot(SERIAL::Writer_* writer,
        const s_lua_save_snapshot*) {
        writer->Write_Uint32(0);
    }

    bool Read_Lua_Save_Snapshot(SERIAL::Reader_* reader,
        s_lua_save_snapshot* snapshot,
        std::string* diagnostic) {
        *snapshot = {};
        diagnostic->clear();
        return reader->Remaining() >= sizeof(uint32_t) &&
            reader->Read_Uint32() == 0;
    }

    bool Restore_Lua_Save_Snapshot(const s_lua_save_snapshot*,
        const s_lua_registry*,
        s_lua_mod_runtime_set*,
        std::string*,
        bool,
        bool) {
        return false;
    }

} // namespace orion2re::lua

namespace orion2re::lua::game {

    void Begin_Semantic_State_Replacement(
        s_semantic_state_replacement* replacement) {
        replacement->active = true;
    }

    void Commit_Semantic_State_Replacement(
        s_semantic_state_replacement* replacement) {
        replacement->active = false;
    }

    void Abort_Semantic_State_Replacement(
        s_semantic_state_replacement* replacement) {
        replacement->active = false;
    }

} // namespace orion2re::lua::game