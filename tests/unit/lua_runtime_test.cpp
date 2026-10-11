#include "lua/lua_runtime.h"

#include <doctest/doctest.h>

#include <cstring>

namespace {

bool Evaluate_(const char *source, const char *source_name,
               orion2re::lua::s_lua_result *result,
               const orion2re::lua::s_lua_limits *limits = nullptr)
{
    const orion2re::lua::s_lua_limits default_limits;
    return orion2re::lua::Evaluate_Module(
        source, std::strlen(source), source_name,
        limits != nullptr ? limits : &default_limits, result);
}

}

TEST_CASE("Lua runtime evaluates signed 64-bit integer modules")
{
    orion2re::lua::s_lua_result result;
    REQUIRE(Evaluate_(
        "local function midpoint(left, right)\n"
        "    return (left + right) // 2\n"
        "end\n"
        "return midpoint(-9, 4)\n",
        "mods/example/scripts/math.lua", &result));

    CHECK(result.value_kind == orion2re::lua::LUA_VALUE_INTEGER);
    CHECK(result.integer_value == -3);
    CHECK(result.remaining_allocator_bytes == 0);
}

TEST_CASE("Lua runtime rejects floating language features in nested functions")
{
    struct s_rejected_source {
        const char *source;
        const char *message;
    };
    const s_rejected_source rejected[] = {
        {"return function() return 1.0 end", "floating-point values"},
        {"return function() return 1e2 end", "floating-point values"},
        {"return function() return 0x1p2 end", "floating-point values"},
        {"return function(left, right) return left / right end", "operator '/'"},
        {"return function(base, exponent) return base ^ exponent end", "operator '^'"},
        {"return function() return 9223372036854775808 end", "floating-point values"},
    };

    for (const auto &entry : rejected) {
        CAPTURE(entry.source);
        orion2re::lua::s_lua_result result;
        CHECK_FALSE(Evaluate_(entry.source, "profile.lua", &result));
        CAPTURE(result.diagnostic);
        CHECK(result.error == orion2re::lua::LUA_ERROR_LANGUAGE_PROFILE);
        CHECK(result.diagnostic.find(entry.message) != std::string::npos);
    }
}

TEST_CASE("Lua compiled profile ignores operators and numbers in text and comments")
{
    orion2re::lua::s_lua_result result;
    REQUIRE(Evaluate_(
        "-- 1.5 / 2 ^ 3\n"
        "local short = '1.5 / 2 ^ 3'\n"
        "local long = [=[1.5 / 2 ^ 3]=]\n"
        "return short == long\n",
        "text.lua", &result));

    CHECK(result.value_kind == orion2re::lua::LUA_VALUE_BOOLEAN);
    CHECK(result.boolean_value);
}

TEST_CASE("Lua compiled profile reports nested source lines before execution")
{
    orion2re::lua::s_lua_result result;
    CHECK_FALSE(Evaluate_(
        "error('module must not execute')\n"
        "local function hidden()\n"
        "    local ratio = 3 / 2\n"
        "    return ratio\n"
        "end\n"
        "return hidden\n",
        "mods/example/scripts/nested.lua", &result));

    CHECK(result.error == orion2re::lua::LUA_ERROR_LANGUAGE_PROFILE);
    CHECK(result.diagnostic.find("mods/example/scripts/nested.lua:3") !=
          std::string::npos);
    CHECK(result.diagnostic.find("floating-point values") != std::string::npos);
}

TEST_CASE("Lua compiled profile rejects float immediates in ordered comparisons")
{
    orion2re::lua::s_lua_result result;
    CHECK_FALSE(Evaluate_(
        "local saved\n"
        "local value = setmetatable({}, {\n"
        "    __lt = function(_, right) saved = right; return false end,\n"
        "})\n"
        "local ignored = value < 1.0\n"
        "return saved\n",
        "comparison.lua", &result));

    CHECK(result.error == orion2re::lua::LUA_ERROR_LANGUAGE_PROFILE);
    CHECK(result.diagnostic.find("comparison.lua:5") != std::string::npos);
    CHECK(result.diagnostic.find("floating-point values") != std::string::npos);
}

TEST_CASE("Lua compiled profile rejects float-tagged equality immediates")
{
    orion2re::lua::s_lua_result result;
    CHECK_FALSE(Evaluate_("local value = {}; return value == 1.0",
                          "equality.lua", &result));

    CHECK(result.error == orion2re::lua::LUA_ERROR_LANGUAGE_PROFILE);
    CHECK(result.diagnostic.find("equality.lua:1") != std::string::npos);
}

TEST_CASE("Lua sequences retain standard one-based behavior")
{
    orion2re::lua::s_lua_result result;
    REQUIRE(Evaluate_(
        "local values = { 10, 20 }\n"
        "table.insert(values, 30)\n"
        "return values[1] + values[2] + values[3] + #values\n",
        "sequences.lua", &result));

    CHECK(result.value_kind == orion2re::lua::LUA_VALUE_INTEGER);
    CHECK(result.integer_value == 63);
}

TEST_CASE("Lua runtime exposes only curated libraries")
{
    orion2re::lua::s_lua_result result;
    REQUIRE(Evaluate_(
        "return debug == nil and io == nil and os == nil and package == nil and "
        "coroutine == nil and load == nil and loadfile == nil and "
        "rawset == nil and setmetatable == nil and "
        "string.dump == nil and string.format == nil and math.abs(-7) == 7 and "
        "tonumber('42') == 42 and tonumber('1.5') == nil",
        "sandbox.lua", &result));

    CHECK(result.value_kind == orion2re::lua::LUA_VALUE_BOOLEAN);
    CHECK(result.boolean_value);
}

TEST_CASE("Lua runtime cannot produce floats through string coercion")
{
    orion2re::lua::s_lua_result result;
    CHECK_FALSE(Evaluate_("return '1.5' + 1", "coercion.lua", &result));

    CHECK(result.error == orion2re::lua::LUA_ERROR_RUNTIME);
    CHECK(result.diagnostic.find("coercion.lua:1") != std::string::npos);
}

TEST_CASE("Lua runtime rejects binary chunks")
{
    const std::string bytecode("\x1bLua", 4);
    const orion2re::lua::s_lua_limits limits;
    orion2re::lua::s_lua_result result;
    CHECK_FALSE(orion2re::lua::Evaluate_Module(
        bytecode.data(), bytecode.size(), "binary.lua", &limits, &result));

    CHECK(result.error == orion2re::lua::LUA_ERROR_INVALID_SOURCE);
}

TEST_CASE("Lua runtime enforces source memory and instruction limits")
{
    SUBCASE("source")
    {
        orion2re::lua::s_lua_limits limits;
        limits.source_bytes = 4;
        orion2re::lua::s_lua_result result;
        CHECK_FALSE(Evaluate_("return 1", "large.lua", &result, &limits));
        CHECK(result.error == orion2re::lua::LUA_ERROR_SOURCE_LIMIT);
    }

    SUBCASE("memory")
    {
        orion2re::lua::s_lua_limits limits;
        limits.allocator_bytes = 32 * 1024;
        orion2re::lua::s_lua_result result;
        CHECK_FALSE(Evaluate_(
            "local values = {}\n"
            "for i = 1, 100000 do values[i] = i end\n"
            "return #values\n",
            "memory.lua", &result, &limits));
        CHECK(result.error == orion2re::lua::LUA_ERROR_MEMORY_LIMIT);
        CHECK(result.remaining_allocator_bytes == 0);
    }

    SUBCASE("non-round instruction limit")
    {
        orion2re::lua::s_lua_limits limits;
        limits.instructions = 1501;
        orion2re::lua::s_lua_result result;
        CHECK_FALSE(Evaluate_("while true do end", "loop.lua", &result, &limits));
        CHECK(result.error == orion2re::lua::LUA_ERROR_INSTRUCTION_LIMIT);
        CHECK(result.instructions_consumed == limits.instructions);
    }

    SUBCASE("zero instruction limit")
    {
        orion2re::lua::s_lua_limits limits;
        limits.instructions = 0;
        orion2re::lua::s_lua_result result;
        CHECK_FALSE(Evaluate_("return 1", "zero.lua", &result, &limits));
        CHECK(result.error == orion2re::lua::LUA_ERROR_INSTRUCTION_LIMIT);
    }

    SUBCASE("instructions")
    {
        orion2re::lua::s_lua_limits limits;
        limits.instructions = 2000;
        orion2re::lua::s_lua_result result;
        CHECK_FALSE(Evaluate_("while true do end", "loop.lua", &result, &limits));
        CHECK(result.error == orion2re::lua::LUA_ERROR_INSTRUCTION_LIMIT);
        CHECK(result.instructions_consumed == limits.instructions);
        CHECK(result.diagnostic.find("loop.lua") != std::string::npos);
        CHECK(result.remaining_allocator_bytes == 0);
    }

    SUBCASE("call depth")
    {
        orion2re::lua::s_lua_limits limits;
        limits.lua_call_depth = 20;
        orion2re::lua::s_lua_result result;
        CHECK_FALSE(Evaluate_(
            "local function recurse() return 1 + recurse() end\n"
            "return recurse()\n",
            "recursion.lua", &result, &limits));
        CHECK(result.error == orion2re::lua::LUA_ERROR_STACK_LIMIT);
        CHECK(result.diagnostic.find("recursion.lua") != std::string::npos);
        CHECK(result.remaining_allocator_bytes == 0);
    }
}

TEST_CASE("Lua runtime reports source locations for syntax and runtime errors")
{
    orion2re::lua::s_lua_result syntax;
    CHECK_FALSE(Evaluate_("local value =\n", "broken.lua", &syntax));
    CHECK(syntax.error == orion2re::lua::LUA_ERROR_SYNTAX);
    CHECK(syntax.diagnostic.find("broken.lua:2") != std::string::npos);

    orion2re::lua::s_lua_result runtime;
    CHECK_FALSE(Evaluate_("\nerror('failed')", "failure.lua", &runtime));
    CHECK(runtime.error == orion2re::lua::LUA_ERROR_RUNTIME);
    CHECK(runtime.diagnostic.find("failure.lua:2") != std::string::npos);
}

TEST_CASE("Lua activation requires a table containing callable exports")
{
    const std::vector<std::string> exports = {"run", "report"};
    const orion2re::lua::s_lua_limits limits;
    orion2re::lua::s_lua_result valid;
    const char valid_source[] =
        "return { run = function() end, report = function() end }";
    REQUIRE(orion2re::lua::Evaluate_Module_Exports(
        valid_source, sizeof(valid_source) - 1, "module.lua", exports.data(),
        exports.size(), &limits, &valid));
    CHECK(valid.remaining_allocator_bytes == 0);

    orion2re::lua::s_lua_result not_table;
    const char value_source[] = "return 1";
    CHECK_FALSE(orion2re::lua::Evaluate_Module_Exports(
        value_source, sizeof(value_source) - 1, "value.lua", exports.data(),
        exports.size(), &limits, &not_table));
    CHECK(not_table.error == orion2re::lua::LUA_ERROR_RUNTIME);
    CHECK(not_table.diagnostic.find("must return a table") != std::string::npos);

    orion2re::lua::s_lua_result missing;
    const char missing_source[] = "return { run = function() end }";
    CHECK_FALSE(orion2re::lua::Evaluate_Module_Exports(
        missing_source, sizeof(missing_source) - 1, "missing.lua",
        exports.data(), exports.size(), &limits, &missing));
    CHECK(missing.error == orion2re::lua::LUA_ERROR_RUNTIME);
    CHECK(missing.diagnostic.find("export 'report' must be a function") !=
          std::string::npos);
}