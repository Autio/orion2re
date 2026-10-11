#ifndef ORION2RE_LUA_CALLBACK_H
#define ORION2RE_LUA_CALLBACK_H

#include <cstdint>

namespace orion2re::lua {

    enum e_lua_callback_kind : std::uint32_t {
        LUA_CALLBACK_EVENT,
        LUA_CALLBACK_RULE,
    };

} // namespace orion2re::lua

#endif