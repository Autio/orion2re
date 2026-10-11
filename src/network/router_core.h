#pragma once

#include "network/router_protocol.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace network::router {

    using t_router_connection_id = uint32_t;

    struct s_router_outbound_message {
        t_router_connection_id connection_id = 0;
        s_router_message message;
    };

    struct s_router_client_state {
        t_router_connection_id connection_id = 0;
        uint32_t network_id = 0;
        uint32_t session_id = 0;
        uint32_t protocol_version = 0;
        char client_name[ROUTER_PLAYER_NAME_SIZE] = {};
        char build_string[ROUTER_BUILD_STRING_SIZE] = {};
        bool greeted = false;
    };

    struct s_router_session_state {
        s_router_session_info info;
        uint32_t owner_network_id = 0;
        std::vector<uint32_t> member_network_ids;
        bool new_players_enabled = true;
    };

    struct s_router_core_state {
        char router_id[ROUTER_ID_SIZE] = {};
        uint32_t next_network_id = 1;
        uint32_t next_session_id = 1;
        std::unordered_map<t_router_connection_id, s_router_client_state> clients;
        std::unordered_map<uint32_t, t_router_connection_id> network_to_connection;
        std::unordered_map<uint32_t, s_router_session_state> sessions;
    };

    void Router_Core_Init(s_router_core_state* state, const char* router_id);
    void Router_Core_Open_Connection(s_router_core_state* state, t_router_connection_id connection_id);
    void Router_Core_Close_Connection(s_router_core_state* state, t_router_connection_id connection_id, std::vector<s_router_outbound_message>* output);
    void Router_Core_Handle_Message(s_router_core_state* state, t_router_connection_id connection_id, const s_router_message* message, std::vector<s_router_outbound_message>* output);
    uint32_t Router_Core_Client_Network_Id(const s_router_core_state* state, t_router_connection_id connection_id);
    uint32_t Router_Core_Client_Session_Id(const s_router_core_state* state, t_router_connection_id connection_id);
    void Router_Core_List_Sessions(const s_router_core_state* state, std::vector<s_router_session_info>* sessions);
    void Router_Core_List_Open_Sessions(const s_router_core_state* state, std::vector<s_router_session_info>* sessions);

} // namespace network::router