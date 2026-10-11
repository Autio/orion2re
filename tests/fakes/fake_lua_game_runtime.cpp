#include "pch.h"

#include "lua/lua_game.h"

namespace {
    int event_count;
    orion2re::lua::game::s_ship_destroyed_event last_event;
} // namespace

namespace TEST_LUA_EVENT {

    void Reset_() {
        event_count = 0;
        last_event = {};
    }

    int Count_() {
        return event_count;
    }

    const orion2re::lua::game::s_ship_destroyed_event& Last_Event_() {
        return last_event;
    }

} // namespace TEST_LUA_EVENT

namespace orion2re::lua::game {

    void Ship_Destroyed(const s_ship_destroyed_event* event) {
        ++event_count;
        last_event = *event;
    }

} // namespace orion2re::lua::game