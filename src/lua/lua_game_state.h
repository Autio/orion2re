#ifndef ORION2RE_LUA_GAME_STATE_H
#define ORION2RE_LUA_GAME_STATE_H

struct lua_State;

namespace orion2re::lua::game {

    int Install_Game_State_Api(lua_State* lua_state);

} // namespace orion2re::lua::game

#endif