#include "pch.h"

#include "lua/lua_game_internal.h"

#include <cstring>
#include <string>

#include "config/config_loader.h"
#include "config/generated/config_enums.h"
#include "game/techdata.h"
#include "lua/lua_mod_runtime.h"

extern "C" {
#include <lauxlib.h>
#include <lua.h>
}

namespace orion2re::lua::game::internal {
    namespace {

        constexpr char TECHNOLOGY_METATABLE_[] = "orion2re.technology";
        constexpr char TECHNOLOGY_APPLICATION_METATABLE_[] =
            "orion2re.technology_application";
        constexpr char TECHNOLOGY_APPLICATION_COLLECTION_METATABLE_[] =
            "orion2re.technology_application_collection";
        constexpr char TECHNOLOGY_FIELD_METATABLE_[] =
            "orion2re.technology_field";
        constexpr char TECHNOLOGY_FIELD_COLLECTION_METATABLE_[] =
            "orion2re.technology_field_collection";
        constexpr char TECHNOLOGY_FIELD_APPLICATIONS_METATABLE_[] =
            "orion2re.technology_field_applications";
        constexpr char TECHNOLOGY_STARTING_FIELDS_METATABLE_[] =
            "orion2re.technology_starting_fields";

        constexpr char APPLICATION_ENUM_[] = "technology_application";
        constexpr char FIELD_ENUM_[] = "technology_field_index";
        constexpr char STATUS_ENUM_[] = "technology_status";

        struct s_technology_proxy {
            bool valid = true;
        };

        struct s_application_proxy {
            std::int16_t application_id = -1;
        };

        struct s_field_proxy {
            std::int16_t field_id = -1;
        };

        struct s_collection_proxy {
            std::int16_t field_id = -1;
        };

        const config::s_generated_enum* Application_Enum_() {
            return config::Find_Generated_Enum(APPLICATION_ENUM_);
        }

        const config::s_generated_enum* Field_Enum_() {
            return config::Find_Generated_Enum(FIELD_ENUM_);
        }

        const config::s_generated_enum* Status_Enum_() {
            return config::Find_Generated_Enum(STATUS_ENUM_);
        }

        bool Valid_Enum_Extent_(const config::s_generated_enum* type,
            std::size_t expected_count) {
            if (type == nullptr || type->value_count != expected_count) {
                return false;
            }
            for (std::size_t index = 0; index < type->value_count; ++index) {
                if (type->values[index].name == nullptr ||
                    type->values[index].value !=
                        static_cast<std::int64_t>(index) ||
                    config::Find_Generated_Enum_Symbol(
                        type, type->values[index].name) !=
                        &type->values[index]) {
                    return false;
                }
            }
            return true;
        }

        bool Valid_Status_Enum_() {
            const config::s_generated_enum* type = Status_Enum_();
            constexpr const char* EXPECTED_NAMES[] = {
                "unavailable",
                "available",
                "researched",
            };
            if (!Valid_Enum_Extent_(type, std::size(EXPECTED_NAMES))) {
                return false;
            }
            for (std::size_t index = 0;
                 index < std::size(EXPECTED_NAMES);
                 ++index) {
                if (strcmp(type->values[index].name,
                        EXPECTED_NAMES[index]) != 0) {
                    return false;
                }
            }
            return true;
        }

        bool Technology_Available_() {
            return Game_State_Access_Allowed() &&
                Valid_Enum_Extent_(Application_Enum_(), TECH_APP_COUNT) &&
                Valid_Enum_Extent_(Field_Enum_(), TECH_FIELD_COUNT);
        }

        bool Valid_Application_(std::int16_t application_id) {
            return Technology_Available_() && application_id >= 0 &&
                application_id < TECH_APP_COUNT &&
                TECHDATA::technology_applications[application_id]
                    .tech_app_id == application_id;
        }

        bool Valid_Field_(std::int16_t field_id) {
            return Technology_Available_() && field_id >= 0 &&
                field_id < TECH_FIELD_COUNT &&
                TECHDATA::technology_fields[field_id].tech_field_id == field_id;
        }

        const config::s_generated_enum_symbol* Application_Symbol_(
            std::int16_t application_id) {
            return Valid_Application_(application_id)
                ? config::Find_Generated_Enum_Value(
                      Application_Enum_(), application_id)
                : nullptr;
        }

        const config::s_generated_enum_symbol* Field_Symbol_(
            std::int16_t field_id) {
            return Valid_Field_(field_id)
                ? config::Find_Generated_Enum_Value(Field_Enum_(), field_id)
                : nullptr;
        }

        bool Application_Id_(const char* name, std::int16_t* application_id) {
            const config::s_generated_enum_symbol* symbol =
                config::Find_Generated_Enum_Symbol(Application_Enum_(), name);
            if (symbol == nullptr || symbol->value < 0 ||
                symbol->value >= TECH_APP_COUNT || application_id == nullptr) {
                return false;
            }
            *application_id = static_cast<std::int16_t>(symbol->value);
            return Valid_Application_(*application_id);
        }

        bool Field_Id_(const char* name, std::int16_t* field_id) {
            const config::s_generated_enum_symbol* symbol =
                config::Find_Generated_Enum_Symbol(Field_Enum_(), name);
            if (symbol == nullptr || symbol->value < 0 ||
                symbol->value >= TECH_FIELD_COUNT || field_id == nullptr) {
                return false;
            }
            *field_id = static_cast<std::int16_t>(symbol->value);
            return Valid_Field_(*field_id);
        }

        bool Record_Native_Config_Write_(const char* collection,
            std::int16_t index,
            const char* field) {
            const std::string path = "design." + std::string(collection) +
                "." + std::to_string(index) + "." + field;
            config::t_variable_id variable_id = config::CONFIG_NO_VARIABLE;
            return config::Find_Variable(
                       path.data(), path.size(), &variable_id) &&
                config::Record_Active_Native_Config_Write(variable_id);
        }

        bool Push_Application_(lua_State* lua_state,
            std::int16_t application_id) {
            if (Application_Symbol_(application_id) == nullptr) {
                lua_pushnil(lua_state);
                return false;
            }
            auto* proxy = static_cast<s_application_proxy*>(
                lua_newuserdatauv(
                    lua_state, sizeof(s_application_proxy), 0));
            *proxy = {application_id};
            luaL_setmetatable(
                lua_state, TECHNOLOGY_APPLICATION_METATABLE_);
            return true;
        }

        bool Push_Field_(lua_State* lua_state, std::int16_t field_id) {
            if (Field_Symbol_(field_id) == nullptr) {
                lua_pushnil(lua_state);
                return false;
            }
            auto* proxy = static_cast<s_field_proxy*>(
                lua_newuserdatauv(lua_state, sizeof(s_field_proxy), 0));
            *proxy = {field_id};
            luaL_setmetatable(lua_state, TECHNOLOGY_FIELD_METATABLE_);
            return true;
        }

        s_application_proxy* Check_Application_(
            lua_State* lua_state, int index) {
            auto* proxy = static_cast<s_application_proxy*>(luaL_checkudata(
                lua_state, index, TECHNOLOGY_APPLICATION_METATABLE_));
            if (proxy == nullptr || !Valid_Application_(proxy->application_id)) {
                luaL_error(lua_state, "expired technology application reference");
            }
            return proxy;
        }

        s_field_proxy* Check_Field_(lua_State* lua_state, int index) {
            auto* proxy = static_cast<s_field_proxy*>(luaL_checkudata(
                lua_state, index, TECHNOLOGY_FIELD_METATABLE_));
            if (proxy == nullptr || !Valid_Field_(proxy->field_id)) {
                luaL_error(lua_state, "expired technology field reference");
            }
            return proxy;
        }

        int Immutable_New_Index_(lua_State* lua_state) {
            return luaL_error(lua_state, "immutable Lua game object");
        }

        int Application_New_Index_(lua_State* lua_state) {
            const s_application_proxy* proxy = Check_Application_(lua_state, 1);
            const char* key = luaL_checkstring(lua_state, 2);
            if (strcmp(key, "field") != 0) {
                return Immutable_New_Index_(lua_state);
            }
            if (!Technology_State_Mutation_Allowed()) {
                return luaL_error(lua_state,
                    "technology application mutation is not permitted in this callback");
            }
            if (lua_isnil(lua_state, 3)) {
                TECHDATA::technology_applications[proxy->application_id]
                    .tech_field_id = TECH_FIELD_INVALID;
            } else {
                TECHDATA::technology_applications[proxy->application_id]
                    .tech_field_id = Check_Field_(lua_state, 3)->field_id;
            }
            if (!Record_Native_Config_Write_(
                    "tech", proxy->application_id, "tech_field_id")) {
                return luaL_error(lua_state,
                    "unable to record technology application configuration write");
            }
            return 0;
        }

        bool Unlink_Previous_(std::int16_t field_id) {
            s_tech_field_data* fields = TECHDATA::technology_fields;
            const std::int16_t previous = fields[field_id].prev_field_id;
            if (previous >= 0 && previous < TECH_FIELD_COUNT &&
                fields[previous].next_field_id == field_id) {
                fields[previous].next_field_id = TECH_FIELD_STARTING_TECH;
                if (!Record_Native_Config_Write_(
                        "tech_field", previous, "next_field_id")) {
                    return false;
                }
            }
            fields[field_id].prev_field_id = TECH_FIELD_STARTING_TECH;
            return Record_Native_Config_Write_(
                "tech_field", field_id, "prev_field_id");
        }

        bool Unlink_Next_(std::int16_t field_id) {
            s_tech_field_data* fields = TECHDATA::technology_fields;
            const std::int16_t next = fields[field_id].next_field_id;
            if (next > TECH_FIELD_STARTING_TECH && next < TECH_FIELD_COUNT &&
                fields[next].prev_field_id == field_id) {
                fields[next].prev_field_id = TECH_FIELD_STARTING_TECH;
                if (!Record_Native_Config_Write_(
                        "tech_field", next, "prev_field_id")) {
                    return false;
                }
            }
            fields[field_id].next_field_id = TECH_FIELD_STARTING_TECH;
            return Record_Native_Config_Write_(
                "tech_field", field_id, "next_field_id");
        }

        int Field_New_Index_(lua_State* lua_state) {
            const s_field_proxy* proxy = Check_Field_(lua_state, 1);
            const char* key = luaL_checkstring(lua_state, 2);
            if (strcmp(key, "previous") != 0 && strcmp(key, "next") != 0) {
                return Immutable_New_Index_(lua_state);
            }
            if (!Technology_State_Mutation_Allowed()) {
                return luaL_error(lua_state,
                    "technology field mutation is not permitted in this callback");
            }
            const std::int16_t field_id = proxy->field_id;
            const bool set_previous = strcmp(key, "previous") == 0;
            std::int16_t linked_field_id = TECH_FIELD_STARTING_TECH;
            if (!lua_isnil(lua_state, 3)) {
                linked_field_id = Check_Field_(lua_state, 3)->field_id;
                if (field_id == TECH_FIELD_STARTING_TECH ||
                    linked_field_id == field_id ||
                    linked_field_id == TECH_FIELD_STARTING_TECH) {
                    return luaL_error(lua_state,
                        "technology field links require distinct non-sentinel fields");
                }
            }
            s_tech_field_data* fields = TECHDATA::technology_fields;
            if (set_previous) {
                if (!Unlink_Previous_(field_id)) {
                    return luaL_error(lua_state,
                        "unable to record technology field configuration write");
                }
                if (linked_field_id != TECH_FIELD_STARTING_TECH) {
                    if (!Unlink_Next_(linked_field_id)) {
                        return luaL_error(lua_state,
                            "unable to record technology field configuration write");
                    }
                    fields[field_id].prev_field_id = linked_field_id;
                    fields[linked_field_id].next_field_id = field_id;
                    if (!Record_Native_Config_Write_(
                            "tech_field", field_id, "prev_field_id") ||
                        !Record_Native_Config_Write_("tech_field",
                            linked_field_id,
                            "next_field_id")) {
                        return luaL_error(lua_state,
                            "unable to record technology field configuration write");
                    }
                }
            } else {
                if (!Unlink_Next_(field_id)) {
                    return luaL_error(lua_state,
                        "unable to record technology field configuration write");
                }
                if (linked_field_id != TECH_FIELD_STARTING_TECH) {
                    if (!Unlink_Previous_(linked_field_id)) {
                        return luaL_error(lua_state,
                            "unable to record technology field configuration write");
                    }
                    fields[field_id].next_field_id = linked_field_id;
                    fields[linked_field_id].prev_field_id = field_id;
                    if (!Record_Native_Config_Write_(
                            "tech_field", field_id, "next_field_id") ||
                        !Record_Native_Config_Write_("tech_field",
                            linked_field_id,
                            "prev_field_id")) {
                        return luaL_error(lua_state,
                            "unable to record technology field configuration write");
                    }
                }
            }
            return 0;
        }

        int Technology_Index_(lua_State* lua_state) {
            luaL_checkudata(lua_state, 1, TECHNOLOGY_METATABLE_);
            if (!Technology_Available_()) {
                return luaL_error(lua_state,
                    "native technology state is unavailable");
            }
            const char* key = luaL_checkstring(lua_state, 2);
            auto* collection = static_cast<s_collection_proxy*>(
                lua_newuserdatauv(lua_state, sizeof(s_collection_proxy), 0));
            *collection = {};
            if (strcmp(key, "applications") == 0) {
                luaL_setmetatable(
                    lua_state, TECHNOLOGY_APPLICATION_COLLECTION_METATABLE_);
            } else if (strcmp(key, "fields") == 0) {
                luaL_setmetatable(
                    lua_state, TECHNOLOGY_FIELD_COLLECTION_METATABLE_);
            } else if (strcmp(key, "starting_fields") == 0) {
                luaL_setmetatable(
                    lua_state, TECHNOLOGY_STARTING_FIELDS_METATABLE_);
            } else {
                lua_pop(lua_state, 1);
                lua_pushnil(lua_state);
            }
            return 1;
        }

        int Application_Index_(lua_State* lua_state) {
            const s_application_proxy* proxy =
                Check_Application_(lua_state, 1);
            const char* key = luaL_checkstring(lua_state, 2);
            const s_tech_app_data& application =
                TECHDATA::technology_applications[proxy->application_id];
            if (strcmp(key, "id") == 0) {
                lua_pushstring(
                    lua_state, Application_Symbol_(proxy->application_id)->name);
            } else if (strcmp(key, "field") == 0) {
                if (application.tech_field_id == TECH_FIELD_INVALID) {
                    lua_pushnil(lua_state);
                } else if (!Push_Field_(lua_state, application.tech_field_id)) {
                    return luaL_error(lua_state,
                        "technology application '%s' has invalid field %d",
                        Application_Symbol_(proxy->application_id)->name,
                        application.tech_field_id);
                }
            } else {
                lua_pushnil(lua_state);
            }
            return 1;
        }

        int Application_Equal_(lua_State* lua_state) {
            const auto* left = static_cast<s_application_proxy*>(
                luaL_testudata(
                    lua_state, 1, TECHNOLOGY_APPLICATION_METATABLE_));
            const auto* right = static_cast<s_application_proxy*>(
                luaL_testudata(
                    lua_state, 2, TECHNOLOGY_APPLICATION_METATABLE_));
            lua_pushboolean(lua_state,
                left != nullptr && right != nullptr &&
                    left->application_id == right->application_id);
            return 1;
        }

        int Field_Index_(lua_State* lua_state) {
            const s_field_proxy* proxy = Check_Field_(lua_state, 1);
            const char* key = luaL_checkstring(lua_state, 2);
            const s_tech_field_data& field =
                TECHDATA::technology_fields[proxy->field_id];
            if (strcmp(key, "id") == 0) {
                lua_pushstring(lua_state, Field_Symbol_(proxy->field_id)->name);
            } else if (strcmp(key, "cost") == 0) {
                lua_pushinteger(lua_state, field.cost);
            } else if (strcmp(key, "previous") == 0) {
                if (!Push_Field_(lua_state, field.prev_field_id)) {
                    return luaL_error(lua_state,
                        "technology field '%s' has invalid previous field %d",
                        Field_Symbol_(proxy->field_id)->name,
                        field.prev_field_id);
                }
            } else if (strcmp(key, "next") == 0) {
                if (!Push_Field_(lua_state, field.next_field_id)) {
                    return luaL_error(lua_state,
                        "technology field '%s' has invalid next field %d",
                        Field_Symbol_(proxy->field_id)->name,
                        field.next_field_id);
                }
            } else if (strcmp(key, "applications") == 0) {
                auto* collection = static_cast<s_collection_proxy*>(
                    lua_newuserdatauv(
                        lua_state, sizeof(s_collection_proxy), 0));
                *collection = {proxy->field_id};
                luaL_setmetatable(
                    lua_state, TECHNOLOGY_FIELD_APPLICATIONS_METATABLE_);
            } else {
                lua_pushnil(lua_state);
            }
            return 1;
        }

        int Field_Equal_(lua_State* lua_state) {
            const auto* left = static_cast<s_field_proxy*>(
                luaL_testudata(
                    lua_state, 1, TECHNOLOGY_FIELD_METATABLE_));
            const auto* right = static_cast<s_field_proxy*>(
                luaL_testudata(
                    lua_state, 2, TECHNOLOGY_FIELD_METATABLE_));
            lua_pushboolean(lua_state,
                left != nullptr && right != nullptr &&
                    left->field_id == right->field_id);
            return 1;
        }

        int Application_Collection_Get_(lua_State* lua_state) {
            luaL_checkudata(
                lua_state, 1, TECHNOLOGY_APPLICATION_COLLECTION_METATABLE_);
            const char* name = luaL_checkstring(lua_state, 2);
            std::int16_t application_id = -1;
            if (!Application_Id_(name, &application_id)) {
                lua_pushnil(lua_state);
            } else {
                Push_Application_(lua_state, application_id);
            }
            return 1;
        }

        int Application_Collection_All_(lua_State* lua_state) {
            luaL_checkudata(
                lua_state, 1, TECHNOLOGY_APPLICATION_COLLECTION_METATABLE_);
            if (!Technology_Available_()) {
                return luaL_error(lua_state,
                    "native technology application table is unavailable");
            }
            lua_createtable(lua_state, TECH_APP_COUNT, 0);
            for (std::int16_t application_id = 0;
                 application_id < TECH_APP_COUNT;
                 ++application_id) {
                if (!Push_Application_(lua_state, application_id)) {
                    return luaL_error(lua_state,
                        "invalid native technology application %d",
                        application_id);
                }
                lua_rawseti(lua_state, -2, application_id + 1);
            }
            return 1;
        }

        int Field_Collection_Get_(lua_State* lua_state) {
            luaL_checkudata(
                lua_state, 1, TECHNOLOGY_FIELD_COLLECTION_METATABLE_);
            const char* name = luaL_checkstring(lua_state, 2);
            std::int16_t field_id = -1;
            if (!Field_Id_(name, &field_id)) {
                lua_pushnil(lua_state);
            } else {
                Push_Field_(lua_state, field_id);
            }
            return 1;
        }

        int Field_Collection_All_(lua_State* lua_state) {
            luaL_checkudata(
                lua_state, 1, TECHNOLOGY_FIELD_COLLECTION_METATABLE_);
            if (!Technology_Available_()) {
                return luaL_error(lua_state,
                    "native technology field table is unavailable");
            }
            lua_createtable(lua_state, TECH_FIELD_COUNT, 0);
            for (std::int16_t field_id = 0;
                 field_id < TECH_FIELD_COUNT;
                 ++field_id) {
                if (!Push_Field_(lua_state, field_id)) {
                    return luaL_error(lua_state,
                        "invalid native technology field %d",
                        field_id);
                }
                lua_rawseti(lua_state, -2, field_id + 1);
            }
            return 1;
        }

        int Field_Applications_All_(lua_State* lua_state) {
            const auto* collection = static_cast<s_collection_proxy*>(
                luaL_checkudata(
                    lua_state, 1, TECHNOLOGY_FIELD_APPLICATIONS_METATABLE_));
            return Push_Technology_Field_Applications(
                lua_state, collection->field_id);
        }

        int Starting_Fields_Get_(lua_State* lua_state) {
            luaL_checkudata(
                lua_state, 1, TECHNOLOGY_STARTING_FIELDS_METATABLE_);
            if (!Technology_Available_()) {
                return luaL_error(lua_state,
                    "native starting technology fields are unavailable");
            }
            const lua_Integer position = luaL_checkinteger(lua_state, 2);
            if (position < 1 || position > 6) {
                lua_pushnil(lua_state);
                return 1;
            }
            const std::int16_t field_id =
                TECHDATA::starting_tech_field_ids[position - 1];
            if (!Push_Field_(lua_state, field_id)) {
                return luaL_error(lua_state,
                    "starting technology field %d is invalid",
                    static_cast<int>(position));
            }
            return 1;
        }

        int Starting_Fields_All_(lua_State* lua_state) {
            luaL_checkudata(
                lua_state, 1, TECHNOLOGY_STARTING_FIELDS_METATABLE_);
            if (!Technology_Available_()) {
                return luaL_error(lua_state,
                    "native starting technology fields are unavailable");
            }
            lua_createtable(lua_state, 6, 0);
            for (int position = 0; position < 6; ++position) {
                const std::int16_t field_id =
                    TECHDATA::starting_tech_field_ids[position];
                if (!Push_Field_(lua_state, field_id)) {
                    return luaL_error(lua_state,
                        "starting technology field %d is invalid",
                        position + 1);
                }
                lua_rawseti(lua_state, -2, position + 1);
            }
            return 1;
        }

        int Collection_Index_(lua_State* lua_state) {
            const char* key = luaL_checkstring(lua_state, 2);
            if (luaL_testudata(lua_state,
                    1,
                    TECHNOLOGY_APPLICATION_COLLECTION_METATABLE_) != nullptr) {
                if (strcmp(key, "get") == 0) {
                    lua_pushcfunction(lua_state, Application_Collection_Get_);
                    return 1;
                }
                if (strcmp(key, "all") == 0) {
                    lua_pushcfunction(lua_state, Application_Collection_All_);
                    return 1;
                }
            } else if (luaL_testudata(lua_state,
                           1,
                           TECHNOLOGY_FIELD_COLLECTION_METATABLE_) != nullptr) {
                if (strcmp(key, "get") == 0) {
                    lua_pushcfunction(lua_state, Field_Collection_Get_);
                    return 1;
                }
                if (strcmp(key, "all") == 0) {
                    lua_pushcfunction(lua_state, Field_Collection_All_);
                    return 1;
                }
            } else if (luaL_testudata(lua_state,
                           1,
                           TECHNOLOGY_STARTING_FIELDS_METATABLE_) != nullptr) {
                if (strcmp(key, "get") == 0) {
                    lua_pushcfunction(lua_state, Starting_Fields_Get_);
                    return 1;
                }
                if (strcmp(key, "all") == 0) {
                    lua_pushcfunction(lua_state, Starting_Fields_All_);
                    return 1;
                }
            } else {
                luaL_checkudata(
                    lua_state, 1, TECHNOLOGY_FIELD_APPLICATIONS_METATABLE_);
                if (strcmp(key, "all") == 0) {
                    lua_pushcfunction(lua_state, Field_Applications_All_);
                    return 1;
                }
            }
            lua_pushnil(lua_state);
            return 1;
        }

        void Create_Metatable_(lua_State* lua_state,
            const char* name,
            lua_CFunction index,
            lua_CFunction equal = nullptr,
            lua_CFunction new_index = Immutable_New_Index_) {
            if (luaL_newmetatable(lua_state, name) != 0) {
                lua_pushcfunction(lua_state, index);
                lua_setfield(lua_state, -2, "__index");
                lua_pushcfunction(lua_state, new_index);
                lua_setfield(lua_state, -2, "__newindex");
                if (equal != nullptr) {
                    lua_pushcfunction(lua_state, equal);
                    lua_setfield(lua_state, -2, "__eq");
                }
                lua_pushliteral(lua_state, "locked");
                lua_setfield(lua_state, -2, "__metatable");
            }
            lua_pop(lua_state, 1);
        }

    } // namespace

    void Create_Technology_Metatables(lua_State* lua_state) {
        Create_Metatable_(
            lua_state, TECHNOLOGY_METATABLE_, Technology_Index_);
        Create_Metatable_(lua_state,
            TECHNOLOGY_APPLICATION_METATABLE_,
            Application_Index_,
            Application_Equal_,
            Application_New_Index_);
        Create_Metatable_(lua_state,
            TECHNOLOGY_APPLICATION_COLLECTION_METATABLE_,
            Collection_Index_);
        Create_Metatable_(lua_state,
            TECHNOLOGY_FIELD_METATABLE_,
            Field_Index_,
            Field_Equal_,
            Field_New_Index_);
        Create_Metatable_(lua_state,
            TECHNOLOGY_FIELD_COLLECTION_METATABLE_,
            Collection_Index_);
        Create_Metatable_(lua_state,
            TECHNOLOGY_FIELD_APPLICATIONS_METATABLE_,
            Collection_Index_);
        Create_Metatable_(lua_state,
            TECHNOLOGY_STARTING_FIELDS_METATABLE_,
            Collection_Index_);
    }

    void Push_Technology(lua_State* lua_state) {
        auto* technology = static_cast<s_technology_proxy*>(
            lua_newuserdatauv(lua_state, sizeof(s_technology_proxy), 0));
        *technology = {};
        luaL_setmetatable(lua_state, TECHNOLOGY_METATABLE_);
    }

    bool Resolve_Technology_Application_Id(
        const char* name, std::int16_t* application_id) {
        return Application_Id_(name, application_id);
    }

    bool Resolve_Technology_Field_Id(
        const char* name, std::int16_t* field_id) {
        return Field_Id_(name, field_id);
    }

    bool Push_Technology_Field(
        lua_State* lua_state, std::int16_t field_id) {
        return Push_Field_(lua_state, field_id);
    }

    int Push_Technology_Field_Applications(
        lua_State* lua_state, std::int16_t field_id) {
        if (!Valid_Field_(field_id)) {
            return luaL_error(lua_state,
                "expired technology field application collection");
        }
        lua_createtable(lua_state, 4, 0);
        lua_Integer sequence_index = 1;
        for (std::int16_t application_id = 0;
             application_id < TECH_APP_COUNT;
             ++application_id) {
            if (!Valid_Application_(application_id)) {
                return luaL_error(lua_state,
                    "invalid native technology application %d",
                    application_id);
            }
            const std::int16_t application_field_id =
                TECHDATA::technology_applications[application_id]
                    .tech_field_id;
            if (application_field_id != TECH_FIELD_INVALID &&
                !Valid_Field_(application_field_id)) {
                return luaL_error(lua_state,
                    "technology application '%s' has invalid field %d",
                    Application_Symbol_(application_id)->name,
                    application_field_id);
            }
            if (application_field_id != field_id) {
                continue;
            }
            if (sequence_index > 4) {
                return luaL_error(lua_state,
                    "technology field '%s' has more than four applications",
                    Field_Symbol_(field_id)->name);
            }
            Push_Application_(lua_state, application_id);
            lua_rawseti(lua_state, -2, sequence_index++);
        }
        return 1;
    }

    bool Resolve_Technology_Status(
        std::uint8_t native_status, std::int64_t* status) {
        if (status == nullptr || !Valid_Status_Enum_()) {
            return false;
        }
        const char* name = nullptr;
        switch (native_status) {
            case TECH_RESEARCH_STATUS_UNAVAILABLE:
                name = "unavailable";
                break;
            case TECH_RESEARCH_STATUS_AVAILABLE:
                name = "available";
                break;
            case TECH_RESEARCH_STATUS_RESEARCHED:
                name = "researched";
                break;
            default:
                return false;
        }
        const config::s_generated_enum_symbol* symbol =
            config::Find_Generated_Enum_Symbol(Status_Enum_(), name);
        if (symbol == nullptr) {
            return false;
        }
        *status = symbol->value;
        return true;
    }

} // namespace orion2re::lua::game::internal