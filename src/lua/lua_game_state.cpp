#include "pch.h"

#include "lua/lua_game_state.h"

#include <cstring>
#include <limits>

#include "game/random.h"
#include "lua/lua_game_internal.h"
#include "lua/lua_mod_runtime.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

namespace orion2re::lua::game {
    namespace {

        constexpr char GAME_METATABLE_[] = "orion2re.game";
        constexpr char RNG_METATABLE_[] = "orion2re.game_rng";

        struct s_game_proxy {
            bool valid = true;
        };

        struct s_rng_proxy {
            bool valid = true;
        };

        int Rng_Integer(lua_State* lua_state) {
            luaL_checkudata(lua_state, 1, RNG_METATABLE_);
            const lua_Integer minimum = luaL_checkinteger(lua_state, 2);
            const lua_Integer maximum = luaL_checkinteger(lua_state, 3);
            if (minimum >= maximum) {
                return luaL_error(lua_state,
                    "random integer minimum must be less than maximum");
            }
            const lua_Unsigned range =
                static_cast<lua_Unsigned>(maximum) -
                static_cast<lua_Unsigned>(minimum);
            if (range >= std::numeric_limits<uint32_t>::max()) {
                return luaL_error(lua_state,
                    "random integer range exceeds native RNG bounds");
            }
            lua_pushinteger(lua_state,
                minimum + static_cast<lua_Integer>(game_random::Random(static_cast<uint32_t>(range))));
            return 1;
        }

        int Rng_Get_Seed(lua_State* lua_state) {
            luaL_checkudata(lua_state, 1, RNG_METATABLE_);
            lua_pushinteger(lua_state, game_random::Get_Random_Seed());
            return 1;
        }

        int Rng_Set_Seed(lua_State* lua_state) {
            luaL_checkudata(lua_state, 1, RNG_METATABLE_);
            const lua_Integer seed = luaL_checkinteger(lua_state, 2);
            if (seed < 0 ||
                static_cast<lua_Unsigned>(seed) >
                    std::numeric_limits<uint32_t>::max()) {
                return luaL_error(lua_state,
                    "random seed is outside the unsigned 32-bit domain");
            }
            game_random::Set_Random_Seed(static_cast<uint32_t>(seed));
            return 0;
        }

        int Rng_Index(lua_State* lua_state) {
            luaL_checkudata(lua_state, 1, RNG_METATABLE_);
            const char* key = luaL_checkstring(lua_state, 2);
            if (strcmp(key, "integer") == 0) {
                lua_pushcfunction(lua_state, Rng_Integer);
                return 1;
            }
            if (strcmp(key, "get_seed") == 0) {
                lua_pushcfunction(lua_state, Rng_Get_Seed);
                return 1;
            }
            if (strcmp(key, "set_seed") == 0) {
                lua_pushcfunction(lua_state, Rng_Set_Seed);
                return 1;
            }
            lua_pushnil(lua_state);
            return 1;
        }

        int Rng_New_Index(lua_State* lua_state) {
            return luaL_error(lua_state, "Lua game RNG is immutable");
        }

        void Push_Rng(lua_State* lua_state) {
            auto* rng = static_cast<s_rng_proxy*>(
                lua_newuserdatauv(lua_state, sizeof(s_rng_proxy), 0));
            *rng = {};
            luaL_setmetatable(lua_state, RNG_METATABLE_);
        }

        int Game_Index(lua_State* lua_state) {
            luaL_checkudata(lua_state, 1, GAME_METATABLE_);
            const char* key = luaL_checkstring(lua_state, 2);
            if (strcmp(key, "rng") == 0) {
                lua_getiuservalue(lua_state, 1, 1);
                return 1;
            }
            if (!Game_State_Access_Allowed()) {
                lua_pushnil(lua_state);
                return 1;
            }
            if (strcmp(key, "players") == 0) {
                internal::Push_Player_Collection(lua_state);
                return 1;
            }
            if (strcmp(key, "ships") == 0) {
                internal::Push_Ship_Collection(lua_state);
                return 1;
            }
            if (strcmp(key, "planets") == 0) {
                internal::Push_Planet_Collection(lua_state);
                return 1;
            }
            if (strcmp(key, "systems") == 0) {
                internal::Push_System_Collection(lua_state);
                return 1;
            }
            if (strcmp(key, "technology") == 0) {
                internal::Push_Technology(lua_state);
                return 1;
            }
            if (strcmp(key, "leaders") == 0) {
                internal::Push_Leader_Collection(lua_state);
                return 1;
            }
            if (strcmp(key, "nebulas") == 0) {
                internal::Push_Nebula_Collection(lua_state);
                return 1;
            }
            if (strcmp(key, "map_width") == 0) {
                lua_pushinteger(lua_state, MOX::MAP_MAX_X);
                return 1;
            }
            if (strcmp(key, "map_height") == 0) {
                lua_pushinteger(lua_state, MOX::MAP_MAX_Y);
                return 1;
            }
            lua_pushnil(lua_state);
            return 1;
        }

        int Game_New_Index(lua_State* lua_state) {
            return luaL_error(lua_state, "Lua game state root is immutable");
        }

    } // namespace

    int Install_Game_State_Api(lua_State* lua_state) {
        internal::Create_Player_Metatables(lua_state);
        internal::Create_Ship_Metatables(lua_state);
        internal::Create_Planet_System_Metatables(lua_state);
        internal::Create_Technology_Metatables(lua_state);
        internal::Create_Leader_Nebula_Metatables(lua_state);
        if (luaL_newmetatable(lua_state, RNG_METATABLE_) != 0) {
            lua_pushcfunction(lua_state, Rng_Index);
            lua_setfield(lua_state, -2, "__index");
            lua_pushcfunction(lua_state, Rng_New_Index);
            lua_setfield(lua_state, -2, "__newindex");
            lua_pushliteral(lua_state, "locked");
            lua_setfield(lua_state, -2, "__metatable");
        }
        lua_pop(lua_state, 1);
        if (luaL_newmetatable(lua_state, GAME_METATABLE_) != 0) {
            lua_pushcfunction(lua_state, Game_Index);
            lua_setfield(lua_state, -2, "__index");
            lua_pushcfunction(lua_state, Game_New_Index);
            lua_setfield(lua_state, -2, "__newindex");
            lua_pushliteral(lua_state, "locked");
            lua_setfield(lua_state, -2, "__metatable");
        }
        lua_pop(lua_state, 1);

        auto* game = static_cast<s_game_proxy*>(
            lua_newuserdatauv(lua_state, sizeof(s_game_proxy), 1));
        *game = {};
        luaL_setmetatable(lua_state, GAME_METATABLE_);
        Push_Rng(lua_state);
        lua_setiuservalue(lua_state, -2, 1);
        lua_setglobal(lua_state, "game");
        return 0;
    }

} // namespace orion2re::lua::game