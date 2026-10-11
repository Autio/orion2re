#ifndef ORION2RE_LUA_NETWORK_H
#define ORION2RE_LUA_NETWORK_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace orion2re::lua {

    inline constexpr uint32_t LUA_NETWORK_DIGEST_BYTES = 32;
    inline constexpr uint32_t LUA_NETWORK_MAX_BYTES = 4 * 1024 * 1024;
    inline constexpr uint32_t LUA_NETWORK_MAX_DIAGNOSTIC_BYTES = 1024;

    enum e_lua_network_setup_kind {
        LUA_NETWORK_SETUP_NEW_GAME = 0,
        LUA_NETWORK_SETUP_LOAD_GAME = 1,
    };

    struct s_lua_network_digest {
        uint8_t bytes[LUA_NETWORK_DIGEST_BYTES];
    };

    struct s_lua_network_setup_offer {
        e_lua_network_setup_kind kind = LUA_NETWORK_SETUP_NEW_GAME;
        uint64_t offer_id = 0;
        const uint8_t* configuration = nullptr;
        uint32_t configuration_size = 0;
        const uint8_t* identity = nullptr;
        uint32_t identity_size = 0;
    };

    enum e_lua_network_diagnostic_code {
        LUA_NETWORK_DIAGNOSTIC_NONE = 0,
        LUA_NETWORK_DIAGNOSTIC_INVALID_OFFER = 1,
        LUA_NETWORK_DIAGNOSTIC_WRONG_HOST = 2,
        LUA_NETWORK_DIAGNOSTIC_WRONG_SETUP_KIND = 3,
        LUA_NETWORK_DIAGNOSTIC_CONFIGURATION = 4,
        LUA_NETWORK_DIAGNOSTIC_RUNTIME_IDENTITY = 5,
    };

    struct s_lua_network_result {
        uint64_t offer_id = 0;
        bool accepted = false;
        s_lua_network_digest digest = {};
        e_lua_network_diagnostic_code diagnostic_code =
            LUA_NETWORK_DIAGNOSTIC_NONE;
        uint32_t record_index = UINT32_MAX;
        std::string diagnostic;
    };

    struct s_lua_registry;
    struct s_lua_save_snapshot;
    struct s_lua_network_state_transaction;

    bool Network_Lua_Enabled();
    bool Write_Lua_Network_Setup_Offer(
        void* buffer,
        uint32_t capacity,
        e_lua_network_setup_kind kind,
        uint64_t offer_id,
        const void* configuration,
        uint32_t configuration_size,
        uint32_t* size,
        s_lua_network_digest* digest,
        std::string* diagnostic);
    bool Begin_Lua_Network_Setup_Offer(
        void* buffer,
        uint32_t capacity,
        e_lua_network_setup_kind kind,
        uint32_t* size,
        std::string* diagnostic);
    bool Read_Lua_Network_Setup_Offer(
        const void* buffer,
        uint32_t size,
        s_lua_network_setup_offer* offer,
        std::string* diagnostic);
    bool Validate_Lua_Network_Setup_Offer(
        const s_lua_network_setup_offer* offer,
        const s_lua_registry* registry,
        s_lua_network_digest* local_digest,
        uint32_t* mismatch_record_index,
        std::string* diagnostic);
    bool Write_Lua_Network_Result(
        void* buffer,
        uint32_t capacity,
        const s_lua_network_result* result,
        uint32_t* size);
    bool Read_Lua_Network_Result(
        const void* buffer,
        uint32_t size,
        s_lua_network_result* result);
    bool Validate_Lua_Network_Setup_Result(
        const void* buffer,
        uint32_t size,
        std::string* diagnostic);
    bool Write_Lua_Network_Setup_Decision(
        void* buffer,
        uint32_t capacity,
        bool accepted,
        const std::string& diagnostic,
        uint32_t* size);
    bool Process_Lua_Network_Setup_Offer(
        const void* payload,
        uint32_t payload_size,
        e_lua_network_setup_kind expected_kind,
        void* result_buffer,
        uint32_t result_capacity,
        uint32_t* result_size);
    void Set_Lua_Network_Offer_Context(
        uint64_t offer_id,
        const s_lua_network_digest* compatibility_digest);
    bool Write_Lua_Network_Setup_Commit(
        void* buffer,
        uint32_t capacity,
        uint32_t* size);
    void Reset_Lua_Network_Session();
    bool Prepare_Lua_Gameplay_Host_Authority(
        int32_t elected_network_id,
        int32_t local_network_id,
        std::string* diagnostic);
    bool Commit_Lua_Network_Setup(
        e_lua_network_setup_kind expected_kind,
        const void* payload,
        uint32_t payload_size,
        uint32_t prefix_size,
        std::string* diagnostic);
    bool Has_Lua_Network_Setup_Commit(
        const void* payload,
        uint32_t payload_size,
        uint32_t prefix_size);
    bool Has_Pending_Lua_Network_Setup();
    void Discard_Pending_Lua_Network_Setup();
    bool Finalize_Pending_Lua_Network_Setup(
        const void* lua_snapshot,
        uint32_t lua_snapshot_size,
        std::string* diagnostic);
    bool Queue_Lua_Network_Load_State(
        const s_lua_save_snapshot* snapshot,
        std::string* diagnostic);
    void Clear_Queued_Lua_Network_Load_State();
    bool Has_Queued_Lua_Network_Load_State();
    bool Capture_Lua_Network_Payload(
        void* buffer,
        uint32_t capacity,
        uint32_t* size,
        uint32_t* flags,
        std::string* diagnostic);
    bool Prepare_Lua_Network_Payload(
        const void* buffer,
        uint32_t size,
        uint32_t flags,
        uint32_t native_ship_count,
        s_lua_network_state_transaction** transaction,
        bool* committed,
        std::string* diagnostic);
    bool Capture_Lua_Network_State(
        void* buffer,
        uint32_t capacity,
        uint32_t* size,
        std::string* diagnostic);
    bool Prepare_Lua_Network_State(
        const void* buffer,
        uint32_t size,
        uint32_t native_ship_count,
        s_lua_network_state_transaction** transaction,
        std::string* diagnostic);
    void Commit_Lua_Network_State(
        s_lua_network_state_transaction** transaction);
    void Discard_Lua_Network_State(
        s_lua_network_state_transaction** transaction);

} // namespace orion2re::lua

#endif