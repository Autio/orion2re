#include "network/router_core.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>

namespace network::router {

    s_router_message Make_Message(e_router_message_kind kind, uint32_t request_id) {
        s_router_message message;
        message.header.kind = kind;
        message.header.request_id = request_id;
        return message;
    }

    s_router_outbound_message Make_Error(t_router_connection_id connection_id, uint32_t request_id, e_router_error_code code, const char* message) {
        s_router_message response = Make_Message(ROUTER_MESSAGE_ERROR, request_id);
        Encode_Error_Payload(code, message, &response.payload);
        return {connection_id, std::move(response)};
    }

    uint32_t Next_Network_Id(uint32_t network_id) {
        if (network_id >= ROUTER_MAX_ASSIGNED_NETWORK_ID || network_id < ROUTER_MIN_ASSIGNED_NETWORK_ID) {
            return ROUTER_MIN_ASSIGNED_NETWORK_ID;
        }
        return network_id + 1;
    }

    uint32_t Allocate_Network_Id(s_router_core_state* state) {
        uint32_t candidate = state->next_network_id;
        for (uint32_t attempts = 0; attempts < ROUTER_ASSIGNED_NETWORK_ID_COUNT; ++attempts) {
            if (candidate < ROUTER_MIN_ASSIGNED_NETWORK_ID || candidate > ROUTER_MAX_ASSIGNED_NETWORK_ID) {
                candidate = ROUTER_MIN_ASSIGNED_NETWORK_ID;
            }

            if (state->network_to_connection.find(candidate) == state->network_to_connection.end()) {
                state->next_network_id = Next_Network_Id(candidate);
                return candidate;
            }

            candidate = Next_Network_Id(candidate);
        }

        return 0;
    }

    uint32_t Next_Session_Id(uint32_t session_id) {
        if (session_id == std::numeric_limits<uint32_t>::max() || session_id == 0) {
            return 1;
        }
        return session_id + 1;
    }

    uint32_t Allocate_Session_Id(s_router_core_state* state) {
        uint32_t candidate = state->next_session_id != 0 ? state->next_session_id : 1;
        const uint32_t first_candidate = candidate;

        do {
            if (candidate != 0 && state->sessions.find(candidate) == state->sessions.end()) {
                state->next_session_id = Next_Session_Id(candidate);
                return candidate;
            }
            candidate = Next_Session_Id(candidate);
        } while (candidate != first_candidate);

        return 0;
    }

    s_router_message Make_Routed_Game_Data(const s_router_client_state* sender, uint32_t target_network_id, const s_router_message* message) {
        s_router_message routed = Make_Message(ROUTER_MESSAGE_GAME_DATA, message->header.request_id);
        routed.header.flags = message->header.flags;
        routed.header.session_id = sender->session_id;
        routed.header.sender_network_id = sender->network_id;
        routed.header.target_network_id = target_network_id;
        routed.payload = message->payload;
        return routed;
    }

    s_router_outbound_message Make_Text_Response(t_router_connection_id connection_id, e_router_message_kind kind, uint32_t request_id, const char* text) {
        s_router_message response = Make_Message(kind, request_id);
        Encode_Text_Payload(text, &response.payload);
        return {connection_id, std::move(response)};
    }

    s_router_member_info Make_Member_Info(const s_router_session_state* session, const s_router_client_state* client) {
        s_router_member_info member;
        member.network_id = client->network_id;
        Router_Copy_String(member.player_name, sizeof(member.player_name), client->client_name);
        Router_Copy_String(member.build_string, sizeof(member.build_string), client->build_string);
        member.owner = session != nullptr && session->owner_network_id == client->network_id;
        return member;
    }

    void Collect_Members(const s_router_core_state* state, const s_router_session_state* session, std::vector<s_router_member_info>* members) {
        members->clear();
        members->reserve(session->member_network_ids.size());
        for (uint32_t member_network_id : session->member_network_ids) {
            auto connection_it = state->network_to_connection.find(member_network_id);
            if (connection_it == state->network_to_connection.end()) {
                continue;
            }
            auto client_it = state->clients.find(connection_it->second);
            if (client_it == state->clients.end()) {
                continue;
            }
            members->push_back(Make_Member_Info(session, &client_it->second));
        }
    }

    void Send_Member_Event_To_Session(
        s_router_core_state* state,
        const s_router_session_state* session,
        uint32_t event,
        const s_router_member_info* member,
        uint32_t request_id,
        uint32_t skip_network_id,
        std::vector<s_router_outbound_message>* output) {
        e_router_message_kind kind = ROUTER_MESSAGE_MEMBER_LEFT;
        if (event == ROUTER_MEMBER_JOINED) {
            kind = ROUTER_MESSAGE_MEMBER_JOINED;
        }

        for (uint32_t member_network_id : session->member_network_ids) {
            if (member_network_id == skip_network_id) {
                continue;
            }
            auto target_connection_it = state->network_to_connection.find(member_network_id);
            if (target_connection_it == state->network_to_connection.end()) {
                continue;
            }

            s_router_message notice = Make_Message(kind, request_id);
            notice.header.session_id = session->info.session_id;
            notice.header.sender_network_id = member->network_id;
            notice.header.target_network_id = member_network_id;
            Encode_Member_Event_Payload(event, member, &notice.payload);
            output->push_back({target_connection_it->second, std::move(notice)});
        }
    }

    void Send_Member_List(
        const s_router_core_state* state,
        const s_router_session_state* session,
        t_router_connection_id connection_id,
        uint32_t target_network_id,
        uint32_t request_id,
        std::vector<s_router_outbound_message>* output) {
        std::vector<s_router_member_info> members;
        Collect_Members(state, session, &members);

        s_router_message member_list = Make_Message(ROUTER_MESSAGE_MEMBER_LIST, request_id);
        member_list.header.session_id = session->info.session_id;
        member_list.header.target_network_id = target_network_id;
        Encode_Member_List_Payload(&members, &member_list.payload);
        output->push_back({connection_id, std::move(member_list)});
    }

    void Send_Session_Closed(
        s_router_core_state* state,
        const std::vector<uint32_t>* members,
        uint32_t owner_network_id,
        uint32_t session_id,
        uint32_t request_id,
        uint32_t skip_network_id,
        std::vector<s_router_outbound_message>* output) {
        for (uint32_t member_network_id : *members) {
            if (member_network_id == skip_network_id) {
                continue;
            }
            auto target_connection_it = state->network_to_connection.find(member_network_id);
            if (target_connection_it == state->network_to_connection.end()) {
                continue;
            }

            s_router_message notice = Make_Message(ROUTER_MESSAGE_SESSION_CLOSED, request_id);
            notice.header.flags = ROUTER_FLAG_SESSION_CLOSED;
            notice.header.session_id = session_id;
            notice.header.sender_network_id = owner_network_id;
            notice.header.target_network_id = member_network_id;
            Encode_Text_Payload("session closed", &notice.payload);
            output->push_back({target_connection_it->second, std::move(notice)});
        }
    }

    void Refresh_Session_Open_State(s_router_session_state* session) {
        session->info.open = session->new_players_enabled && session->info.current_players < session->info.max_players;
    }

    void Leave_Client_Session(s_router_core_state* state, s_router_client_state* client, const char* reason, uint32_t notice_flags, std::vector<s_router_outbound_message>* output) {
        (void)reason;
        (void)notice_flags;

        if (client->session_id == 0) {
            return;
        }

        auto session_it = state->sessions.find(client->session_id);
        if (session_it == state->sessions.end()) {
            client->session_id = 0;
            return;
        }

        s_router_session_state& session = session_it->second;
        const bool leaving_owner = session.owner_network_id == client->network_id;
        if (session.owner_network_id == client->network_id) {
            session.owner_network_id = 0;
        }

        const s_router_member_info leaving_member = Make_Member_Info(&session, client);
        session.member_network_ids.erase(
            std::remove(session.member_network_ids.begin(), session.member_network_ids.end(), client->network_id),
            session.member_network_ids.end());

        Send_Member_Event_To_Session(state, &session, ROUTER_MEMBER_LEFT, &leaving_member, 0, 0, output);

        client->session_id = 0;

        if (session.member_network_ids.empty()) {
            state->sessions.erase(session_it);
            return;
        }

        session.info.current_players = (uint16_t)session.member_network_ids.size();
        if (leaving_owner) {
            session.new_players_enabled = false;
        }
        Refresh_Session_Open_State(&session);
    }

    void Router_Core_Init(s_router_core_state* state, const char* router_id) {
        if (state == nullptr) {
            return;
        }

        Router_Copy_String(state->router_id, sizeof(state->router_id), (router_id != nullptr && router_id[0] != '\0') ? router_id : "router");
        state->next_network_id = ROUTER_MIN_ASSIGNED_NETWORK_ID;
        state->next_session_id = 1;
        state->clients.clear();
        state->network_to_connection.clear();
        state->sessions.clear();
    }

    void Router_Core_Open_Connection(s_router_core_state* state, t_router_connection_id connection_id) {
        if (state == nullptr) {
            return;
        }
        state->clients.emplace(connection_id, s_router_client_state{connection_id});
    }

    void Router_Core_Close_Connection(s_router_core_state* state, t_router_connection_id connection_id, std::vector<s_router_outbound_message>* output) {
        if (state == nullptr || output == nullptr) {
            return;
        }

        output->clear();
        auto client_it = state->clients.find(connection_id);
        if (client_it == state->clients.end()) {
            return;
        }

        s_router_client_state& client = client_it->second;
        if (client.greeted) {
            if (client.session_id != 0) {
                Leave_Client_Session(state, &client, "disconnect", 0, output);
            }
            state->network_to_connection.erase(client.network_id);
        }

        state->clients.erase(client_it);
    }

    void Router_Core_Handle_Message(s_router_core_state* state, t_router_connection_id connection_id, const s_router_message* message, std::vector<s_router_outbound_message>* output) {
        if (state == nullptr || message == nullptr || output == nullptr) {
            return;
        }

        output->clear();
        auto client_it = state->clients.find(connection_id);
        if (client_it == state->clients.end()) {
            return;
        }

        s_router_client_state& client = client_it->second;
        const uint32_t request_id = message->header.request_id;

        if (message->header.kind == ROUTER_MESSAGE_HELLO) {
            if (client.greeted) {
                output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_ALREADY_REGISTERED, "connection already sent HELLO"));
                return;
            }

            s_router_hello_payload hello;
            std::string parse_error;
            if (!Decode_Hello_Payload(&message->payload, &hello, &parse_error)) {
                output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_INVALID_MESSAGE, parse_error.c_str()));
                return;
            }
            if (hello.protocol_version != ROUTER_VERSION) {
                output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_INVALID_MESSAGE, "unsupported router version"));
                return;
            }

            const uint32_t network_id = Allocate_Network_Id(state);
            if (network_id == 0) {
                output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_ROUTER_FULL, "router has no available network ids"));
                return;
            }

            client.greeted = true;
            client.network_id = network_id;
            client.protocol_version = hello.protocol_version;
            Router_Copy_String(client.client_name, sizeof(client.client_name), hello.client_name[0] != '\0' ? hello.client_name : "Player");
            client.build_string[0] = '\0';
            state->network_to_connection[client.network_id] = connection_id;

            s_router_message welcome = Make_Message(ROUTER_MESSAGE_WELCOME, request_id);
            welcome.header.target_network_id = client.network_id;
            Encode_Welcome_Payload(client.network_id, state->router_id, &welcome.payload);
            output->push_back({connection_id, std::move(welcome)});
            return;
        }

        if (!client.greeted) {
            output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_NOT_REGISTERED, "HELLO is required before other router messages"));
            return;
        }

        switch (message->header.kind) {
            case ROUTER_MESSAGE_REGISTER_SESSION: {
                if (client.session_id != 0) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_ALREADY_IN_SESSION, "client is already in a session"));
                    return;
                }

                s_router_session_registration_payload registration;
                std::string parse_error;
                if (!Decode_Session_Registration_Payload(&message->payload, &registration, &parse_error)) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_INVALID_MESSAGE, parse_error.c_str()));
                    return;
                }
                if (registration.session_name[0] == '\0') {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_INVALID_MESSAGE, "session name is required"));
                    return;
                }
                if (registration.max_players == 0) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_INVALID_MESSAGE, "max_players must be greater than zero"));
                    return;
                }
                if (registration.build_string[0] == '\0') {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_BUILD_MISMATCH, "session build string is required"));
                    return;
                }

                s_router_session_state session;
                session.info.session_id = Allocate_Session_Id(state);
                if (session.info.session_id == 0) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_ROUTER_FULL, "router has no available session ids"));
                    return;
                }
                Router_Copy_String(session.info.router_id, sizeof(session.info.router_id), state->router_id);
                Router_Copy_String(session.info.session_name, sizeof(session.info.session_name), registration.session_name);
                Router_Copy_String(session.info.host_name, sizeof(session.info.host_name), registration.host_name[0] != '\0' ? registration.host_name : client.client_name);
                session.info.current_players = 1;
                session.info.max_players = registration.max_players;
                Router_Copy_String(session.info.build_string, sizeof(session.info.build_string), registration.build_string);
                Router_Copy_String(client.build_string, sizeof(client.build_string), registration.build_string);
                session.owner_network_id = client.network_id;
                session.member_network_ids.push_back(client.network_id);
                session.new_players_enabled = true;
                Refresh_Session_Open_State(&session);

                const uint32_t session_id = session.info.session_id;
                client.session_id = session_id;

                s_router_message response = Make_Message(ROUTER_MESSAGE_SESSION_REGISTERED, request_id);
                response.header.session_id = session_id;
                response.header.target_network_id = client.network_id;
                Encode_Session_Info_Payload(&session.info, &response.payload);
                state->sessions.emplace(session_id, std::move(session));
                output->push_back({connection_id, std::move(response)});

                auto stored_session_it = state->sessions.find(session_id);
                if (stored_session_it != state->sessions.end()) {
                    Send_Member_List(state, &stored_session_it->second, connection_id, client.network_id, request_id, output);
                }
                return;
            }
            case ROUTER_MESSAGE_UNREGISTER_SESSION: {
                const uint32_t session_id = message->header.session_id != 0 ? message->header.session_id : client.session_id;
                auto session_it = state->sessions.find(session_id);
                if (session_it == state->sessions.end()) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_SESSION_NOT_FOUND, "session does not exist"));
                    return;
                }
                if (session_it->second.owner_network_id != client.network_id) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_PERMISSION_DENIED, "only the session owner can unregister the session"));
                    return;
                }

                output->push_back(Make_Text_Response(connection_id, ROUTER_MESSAGE_OK, request_id, "session unregistered"));

                const std::vector<uint32_t> members = session_it->second.member_network_ids;
                for (uint32_t member_network_id : members) {
                    auto member_connection_it = state->network_to_connection.find(member_network_id);
                    if (member_connection_it != state->network_to_connection.end()) {
                        auto member_client_it = state->clients.find(member_connection_it->second);
                        if (member_client_it != state->clients.end()) {
                            member_client_it->second.session_id = 0;
                        }
                    }
                }

                Send_Session_Closed(state, &members, client.network_id, session_id, request_id, client.network_id, output);

                state->sessions.erase(session_it);
                return;
            }
            case ROUTER_MESSAGE_SET_SESSION_OPEN: {
                const uint32_t session_id = message->header.session_id != 0 ? message->header.session_id : client.session_id;
                auto session_it = state->sessions.find(session_id);
                if (session_it == state->sessions.end()) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_SESSION_NOT_FOUND, "session does not exist"));
                    return;
                }
                if (session_it->second.owner_network_id != client.network_id) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_PERMISSION_DENIED, "only the session owner can change join state"));
                    return;
                }

                session_it->second.new_players_enabled = (message->header.flags & ROUTER_FLAG_SESSION_OPEN) != 0;
                Refresh_Session_Open_State(&session_it->second);
                output->push_back(Make_Text_Response(connection_id, ROUTER_MESSAGE_OK, request_id, session_it->second.info.open ? "session open" : "session closed"));
                return;
            }
            case ROUTER_MESSAGE_LIST_SESSIONS: {
                s_router_message response = Make_Message(ROUTER_MESSAGE_SESSION_LIST, request_id);
                response.header.target_network_id = client.network_id;
                std::vector<s_router_session_info> sessions;
                Router_Core_List_Open_Sessions(state, &sessions);
                Encode_Session_List_Payload(&sessions, &response.payload);
                output->push_back({connection_id, std::move(response)});
                return;
            }
            case ROUTER_MESSAGE_JOIN_SESSION: {
                if (client.session_id != 0) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_ALREADY_IN_SESSION, "client is already in a session"));
                    return;
                }

                auto session_it = state->sessions.find(message->header.session_id);
                if (session_it == state->sessions.end()) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_SESSION_NOT_FOUND, "session does not exist"));
                    return;
                }

                s_router_join_session_payload join;
                std::string parse_error;
                if (!Decode_Join_Session_Payload(&message->payload, &join, &parse_error)) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_INVALID_MESSAGE, parse_error.c_str()));
                    return;
                }

                s_router_session_state& session = session_it->second;
                if (join.build_string[0] == '\0' || session.info.build_string[0] == '\0' || std::strcmp(session.info.build_string, join.build_string) != 0) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_BUILD_MISMATCH, "client build does not match session build"));
                    return;
                }
                if (!session.new_players_enabled) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_SESSION_CLOSED, "session is closed to new players"));
                    return;
                }
                if (session.info.current_players >= session.info.max_players) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_SESSION_FULL, "session is full"));
                    return;
                }

                Router_Copy_String(client.build_string, sizeof(client.build_string), join.build_string);
                const s_router_member_info joining_member = Make_Member_Info(&session, &client);
                session.member_network_ids.push_back(client.network_id);
                session.info.current_players = (uint16_t)session.member_network_ids.size();
                Refresh_Session_Open_State(&session);
                client.session_id = session.info.session_id;

                s_router_message response = Make_Message(ROUTER_MESSAGE_JOINED_SESSION, request_id);
                response.header.session_id = session.info.session_id;
                response.header.target_network_id = client.network_id;
                Encode_Session_Info_Payload(&session.info, &response.payload);
                output->push_back({connection_id, std::move(response)});

                Send_Member_List(state, &session, connection_id, client.network_id, request_id, output);
                Send_Member_Event_To_Session(state, &session, ROUTER_MEMBER_JOINED, &joining_member, request_id, client.network_id, output);
                return;
            }
            case ROUTER_MESSAGE_LEAVE_SESSION: {
                if (client.session_id == 0) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_NOT_IN_SESSION, "client is not in a session"));
                    return;
                }

                output->push_back(Make_Text_Response(connection_id, ROUTER_MESSAGE_LEFT_SESSION, request_id, "left session"));
                Leave_Client_Session(state, &client, "left session", 0, output);
                return;
            }
            case ROUTER_MESSAGE_REMOVE_MEMBER: {
                char reason[ROUTER_MEMBER_REMOVAL_REASON_SIZE] = {};
                std::string parse_error;
                if (!Decode_Text_Payload(&message->payload,
                        sizeof(reason),
                        reason,
                        &parse_error)) {
                    output->push_back(Make_Error(connection_id,
                        request_id,
                        ROUTER_ERROR_INVALID_MESSAGE,
                        parse_error.c_str()));
                    return;
                }
                auto session_it = state->sessions.find(client.session_id);
                if (client.session_id == 0 || session_it == state->sessions.end()) {
                    output->push_back(Make_Error(connection_id,
                        request_id,
                        ROUTER_ERROR_NOT_IN_SESSION,
                        "client is not in a session"));
                    return;
                }
                if (session_it->second.owner_network_id != client.network_id) {
                    output->push_back(Make_Error(connection_id,
                        request_id,
                        ROUTER_ERROR_PERMISSION_DENIED,
                        "only the session owner can remove a member"));
                    return;
                }
                if (message->header.target_network_id == client.network_id) {
                    output->push_back(Make_Error(connection_id,
                        request_id,
                        ROUTER_ERROR_PERMISSION_DENIED,
                        "the session owner cannot remove itself"));
                    return;
                }
                auto target_connection_it = state->network_to_connection.find(
                    message->header.target_network_id);
                if (target_connection_it == state->network_to_connection.end()) {
                    output->push_back(Make_Error(connection_id,
                        request_id,
                        ROUTER_ERROR_TARGET_NOT_FOUND,
                        "target network id is not connected"));
                    return;
                }
                auto target_client_it = state->clients.find(
                    target_connection_it->second);
                if (target_client_it == state->clients.end() ||
                    target_client_it->second.session_id != client.session_id) {
                    output->push_back(Make_Error(connection_id,
                        request_id,
                        ROUTER_ERROR_TARGET_NOT_FOUND,
                        "target network id is not in this session"));
                    return;
                }

                output->push_back(Make_Text_Response(connection_id,
                    ROUTER_MESSAGE_OK,
                    request_id,
                    "member removed"));
                output->push_back(Make_Text_Response(
                    target_connection_it->second,
                    ROUTER_MESSAGE_LEFT_SESSION,
                    0,
                    reason[0] != '\0' ? reason : "removed from session"));
                Leave_Client_Session(state,
                    &target_client_it->second,
                    "removed from session",
                    0,
                    output);
                return;
            }
            case ROUTER_MESSAGE_SEND_TO: {
                if (client.session_id == 0) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_NOT_IN_SESSION, "client is not in a session"));
                    return;
                }

                auto session_it = state->sessions.find(client.session_id);
                if (session_it == state->sessions.end()) {
                    client.session_id = 0;
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_SESSION_NOT_FOUND, "session does not exist"));
                    return;
                }

                auto target_connection_it = state->network_to_connection.find(message->header.target_network_id);
                if (target_connection_it == state->network_to_connection.end()) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_TARGET_NOT_FOUND, "target network id is not connected"));
                    return;
                }
                auto target_client_it = state->clients.find(target_connection_it->second);
                if (target_client_it == state->clients.end() || target_client_it->second.session_id != client.session_id) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_TARGET_NOT_FOUND, "target network id is not in this session"));
                    return;
                }

                s_router_message routed = Make_Routed_Game_Data(&client, message->header.target_network_id, message);
                output->push_back({target_connection_it->second, std::move(routed)});
                return;
            }
            case ROUTER_MESSAGE_SEND_BROADCAST: {
                if (client.session_id == 0) {
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_NOT_IN_SESSION, "client is not in a session"));
                    return;
                }

                auto session_it = state->sessions.find(client.session_id);
                if (session_it == state->sessions.end()) {
                    client.session_id = 0;
                    output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_SESSION_NOT_FOUND, "session does not exist"));
                    return;
                }

                for (uint32_t member_network_id : session_it->second.member_network_ids) {
                    if (member_network_id == client.network_id && (message->header.flags & ROUTER_FLAG_BROADCAST_INCLUDE_SENDER) == 0) {
                        continue;
                    }
                    auto target_connection_it = state->network_to_connection.find(member_network_id);
                    if (target_connection_it == state->network_to_connection.end()) {
                        continue;
                    }

                    s_router_message routed = Make_Routed_Game_Data(&client, member_network_id, message);
                    output->push_back({target_connection_it->second, std::move(routed)});
                }
                return;
            }
            default:
                output->push_back(Make_Error(connection_id, request_id, ROUTER_ERROR_INVALID_MESSAGE, "message kind is not accepted from clients"));
                return;
        }
    }

    uint32_t Router_Core_Client_Network_Id(const s_router_core_state* state, t_router_connection_id connection_id) {
        if (state == nullptr) {
            return 0;
        }
        auto client_it = state->clients.find(connection_id);
        if (client_it == state->clients.end()) {
            return 0;
        }
        return client_it->second.network_id;
    }

    uint32_t Router_Core_Client_Session_Id(const s_router_core_state* state, t_router_connection_id connection_id) {
        if (state == nullptr) {
            return 0;
        }
        auto client_it = state->clients.find(connection_id);
        if (client_it == state->clients.end()) {
            return 0;
        }
        return client_it->second.session_id;
    }

    void Router_Core_List_Sessions(const s_router_core_state* state, std::vector<s_router_session_info>* sessions) {
        if (state == nullptr || sessions == nullptr) {
            return;
        }

        sessions->clear();
        sessions->reserve(state->sessions.size());
        for (const auto& entry : state->sessions) {
            sessions->push_back(entry.second.info);
        }
        std::sort(sessions->begin(), sessions->end(), [](const s_router_session_info& lhs, const s_router_session_info& rhs) {
            return lhs.session_id < rhs.session_id;
        });
    }

    void Router_Core_List_Open_Sessions(const s_router_core_state* state, std::vector<s_router_session_info>* sessions) {
        if (state == nullptr || sessions == nullptr) {
            return;
        }

        sessions->clear();
        sessions->reserve(state->sessions.size());
        for (const auto& entry : state->sessions) {
            if (entry.second.info.open) {
                sessions->push_back(entry.second.info);
            }
        }
        std::sort(sessions->begin(), sessions->end(), [](const s_router_session_info& lhs, const s_router_session_info& rhs) {
            return lhs.session_id < rhs.session_id;
        });
    }

} // namespace network::router