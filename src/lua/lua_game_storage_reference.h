#ifndef ORION2RE_LUA_GAME_STORAGE_REFERENCE_H
#define ORION2RE_LUA_GAME_STORAGE_REFERENCE_H

namespace orion2re::lua {

    struct s_lua_storage_reference_adapter;

    const s_lua_storage_reference_adapter*
    Get_Game_Storage_Reference_Adapter();

} // namespace orion2re::lua

#endif