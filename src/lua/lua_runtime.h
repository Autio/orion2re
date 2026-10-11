#ifndef ORION2RE_LUA_RUNTIME_H
#define ORION2RE_LUA_RUNTIME_H

#include <cstddef>
#include <cstdint>
#include <string>

struct lua_State;

namespace orion2re::lua {

    struct s_lua_runtime_state;

    inline constexpr char LUA_UPSTREAM_VERSION[] = "5.5.1";
    inline constexpr std::uint32_t LUA_RUNTIME_PROFILE_VERSION = 4;

    enum e_lua_error {
        LUA_ERROR_NONE,
        LUA_ERROR_INVALID_SOURCE,
        LUA_ERROR_SOURCE_LIMIT,
        LUA_ERROR_LANGUAGE_PROFILE,
        LUA_ERROR_SYNTAX,
        LUA_ERROR_MEMORY_LIMIT,
        LUA_ERROR_INSTRUCTION_LIMIT,
        LUA_ERROR_STACK_LIMIT,
        LUA_ERROR_RUNTIME,
    };

    enum e_lua_value_kind {
        LUA_VALUE_NONE,
        LUA_VALUE_NIL,
        LUA_VALUE_BOOLEAN,
        LUA_VALUE_INTEGER,
        LUA_VALUE_STRING,
        LUA_VALUE_OTHER,
    };

    using t_lua_argument_builder = int (*)(lua_State* lua_state, void* user_data);
    using t_lua_state_operation = int (*)(lua_State* lua_state, void* user_data);
    using t_lua_result_consumer = int (*)(lua_State* lua_state,
        int result_count,
        void* user_data);

    struct s_lua_limits {
        std::size_t source_bytes = 256 * 1024;
        std::size_t allocator_bytes = 4 * 1024 * 1024;
        std::uint64_t instructions = 1'000'000;
        unsigned int lua_call_depth = 200;
        std::size_t diagnostic_bytes = 4096;
    };

    struct s_lua_result {
        e_lua_error error = LUA_ERROR_NONE;
        e_lua_value_kind value_kind = LUA_VALUE_NONE;
        std::string diagnostic;
        std::int64_t integer_value = 0;
        bool boolean_value = false;
        std::string string_value;
        std::size_t peak_allocator_bytes = 0;
        std::size_t remaining_allocator_bytes = 0;
        std::uint64_t instructions_consumed = 0;
    };

    bool Evaluate_Module(const char* source, std::size_t source_size, const char* source_name, const s_lua_limits* limits, s_lua_result* result);
    bool Evaluate_Module_Exports(const char* source, std::size_t source_size, const char* source_name, const std::string* required_exports, std::size_t required_export_count, const s_lua_limits* limits, s_lua_result* result);
    bool Invoke_Module_Export(const char* source, std::size_t source_size, const char* source_name, const char* export_name, t_lua_argument_builder build_arguments, void* user_data, const s_lua_limits* limits, s_lua_result* result);
    bool Create_Runtime_State(const s_lua_limits* limits, s_lua_runtime_state** runtime, s_lua_result* result);
    void Destroy_Runtime_State(s_lua_runtime_state* runtime);
    lua_State* Get_Runtime_State_Handle(s_lua_runtime_state* runtime);
    bool Load_Runtime_Source_Chunk(lua_State* lua_state, const char* source, std::size_t source_size, const char* source_name);
    void Mark_Runtime_Invocation_Error(lua_State* lua_state,
        e_lua_error error);
    bool Execute_Runtime_Operation(s_lua_runtime_state* runtime, t_lua_state_operation operation, void* user_data, s_lua_result* result);
    bool Execute_Runtime_Source(s_lua_runtime_state* runtime, const char* source, std::size_t source_size, const char* source_name, s_lua_result* result);
    bool Invoke_Runtime_Function(s_lua_runtime_state* runtime, int function_reference, t_lua_argument_builder build_arguments, void* user_data, s_lua_result* result);
    bool Invoke_Runtime_Function_Results(s_lua_runtime_state* runtime,
        int function_reference,
        t_lua_argument_builder build_arguments,
        void* argument_user_data,
        t_lua_result_consumer consume_results,
        void* result_user_data,
        s_lua_result* result);

} // namespace orion2re::lua

#endif