#include "network/router_session_browser.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>

#include <uv.h>

namespace network::router {

    enum e_browser_phase {
        BROWSER_IDLE,
        BROWSER_DISCOVERING,
        BROWSER_QUERYING,
    };

    struct s_discovery_send_request {
        uv_udp_send_t request{};
        uv_buf_t buffer{};
        Bit_Stream data;
        s_router_session_browser_state* state = nullptr;
    };

    struct s_router_query {
        s_router_endpoint endpoint;
        s_router_client client;
        std::chrono::steady_clock::time_point deadline;
        bool started = false;
        bool complete = false;
    };

    const std::vector<s_router_session_record>& Empty_Session_Records() {
        static const std::vector<s_router_session_record> empty;
        return empty;
    }

    bool Endpoints_Match(const s_router_endpoint& left, const s_router_endpoint& right) {
        return left.tcp_port == right.tcp_port && std::strcmp(left.host, right.host) == 0;
    }

    void Merge_Endpoint(s_router_endpoint* existing, const s_router_endpoint& endpoint) {
        if (existing == nullptr) {
            return;
        }
        if (existing->router_id[0] == '\0') {
            Router_Copy_String(existing->router_id, sizeof(existing->router_id), endpoint.router_id);
        }
        if (existing->router_name[0] == '\0') {
            Router_Copy_String(existing->router_name, sizeof(existing->router_name), endpoint.router_name);
        }
    }

    void Add_Unique_Endpoint(std::vector<s_router_endpoint>* endpoints, const s_router_endpoint& endpoint) {
        for (s_router_endpoint& existing : *endpoints) {
            if (Endpoints_Match(existing, endpoint)) {
                Merge_Endpoint(&existing, endpoint);
                return;
            }
        }
        endpoints->push_back(endpoint);
    }

    bool Session_Records_Match(const s_router_session_record& left, const s_router_session_record& right) {
        return left.session.session_id == right.session.session_id &&
            Endpoints_Match(left.router, right.router);
    }

    void Add_Unique_Session_Record(std::vector<s_router_session_record>* sessions, const s_router_session_record& record) {
        for (s_router_session_record& existing : *sessions) {
            if (Session_Records_Match(existing, record)) {
                Merge_Endpoint(&existing.router, record.router);
                return;
            }
        }
        sessions->push_back(record);
    }

    void Close_Uv_Handle(uv_handle_t* handle) {
        if (handle != nullptr && !uv_is_closing(handle)) {
            uv_close(handle, nullptr);
        }
    }

    void Discovery_Alloc_Callback(uv_handle_t*, size_t suggested_size, uv_buf_t* buffer) {
        const size_t allocation_size = suggested_size != 0 ? suggested_size : 512;
        buffer->base = static_cast<char*>(std::malloc(allocation_size));
        buffer->len = static_cast<unsigned int>(allocation_size);
    }

    struct s_router_session_browser_state {
        s_router_session_browser_config config;
        e_browser_phase phase = BROWSER_IDLE;
        uv_loop_t discovery_loop{};
        uv_udp_t discovery_udp{};
        uv_timer_t discovery_timer{};
        bool discovery_loop_initialized = false;
        bool discovery_udp_initialized = false;
        bool discovery_timer_initialized = false;
        bool discovery_done = false;
        std::vector<s_router_endpoint> endpoints;
        std::vector<std::unique_ptr<s_router_query>> queries;
        std::vector<s_router_session_record> sessions;
        uint32_t revision = 0;
        uint32_t next_request_id = 1;
        int last_error = 0;
    };

    void Discovery_Send_Callback(uv_udp_send_t* request, int status) {
        s_discovery_send_request* send_request = static_cast<s_discovery_send_request*>(request->data);
        s_router_session_browser_state* state = send_request != nullptr ? send_request->state : nullptr;
        if (state != nullptr && status != 0 && state->last_error == 0) {
            state->last_error = status;
        }
        delete send_request;
    }

    void Discovery_Timer_Callback(uv_timer_t* timer) {
        s_router_session_browser_state* state = static_cast<s_router_session_browser_state*>(timer->data);
        if (state != nullptr) {
            state->discovery_done = true;
        }
        uv_stop(timer->loop);
    }

    void Discovery_Recv_Callback(uv_udp_t* handle, ssize_t bytes_read, const uv_buf_t* buffer, const sockaddr* address, unsigned flags) {
        (void)flags;
        s_router_session_browser_state* state = static_cast<s_router_session_browser_state*>(handle->data);

        if (bytes_read > 0 && state != nullptr && address != nullptr && address->sa_family == AF_INET) {
            Bit_Stream payload(reinterpret_cast<const uint8_t*>(buffer->base), static_cast<uint32_t>(bytes_read));
            s_router_discovery_reply reply;
            if (Decode_Discovery_Reply(&payload, &reply, nullptr) &&
                reply.protocol_version == ROUTER_PROTOCOL_VERSION &&
                reply.tcp_port != 0) {
                s_router_endpoint endpoint;
                if (uv_ip4_name(reinterpret_cast<const sockaddr_in*>(address), endpoint.host, sizeof(endpoint.host)) == 0) {
                    endpoint.tcp_port = reply.tcp_port;
                    Router_Copy_String(endpoint.router_id, sizeof(endpoint.router_id), reply.router_id);
                    Router_Copy_String(endpoint.router_name, sizeof(endpoint.router_name), reply.router_name);
                    Add_Unique_Endpoint(&state->endpoints, endpoint);
                }
            }
        } else if (bytes_read < 0 && state != nullptr && state->last_error == 0) {
            state->last_error = static_cast<int>(bytes_read);
        }

        if (buffer != nullptr && buffer->base != nullptr) {
            std::free(buffer->base);
        }
    }

    bool Send_Discovery_Probe(s_router_session_browser_state* state, const char* host) {
        sockaddr_in address{};
        int result = uv_ip4_addr(host, state->config.discovery_port, &address);
        if (result != 0) {
            state->last_error = result;
            return false;
        }

        s_discovery_send_request* request = new s_discovery_send_request{};
        Encode_Discovery_Probe(&request->data);
        request->buffer = uv_buf_init(reinterpret_cast<char*>(const_cast<uint8_t*>(request->data.Data())), request->data.Size());
        request->state = state;
        request->request.data = request;
        result = uv_udp_send(&request->request, &state->discovery_udp, &request->buffer, 1, reinterpret_cast<const sockaddr*>(&address), Discovery_Send_Callback);
        if (result != 0) {
            state->last_error = result;
            delete request;
            return false;
        }
        return true;
    }

    void Finish_Discovery(s_router_session_browser_state* state) {
        if (state->discovery_udp_initialized) {
            uv_udp_recv_stop(&state->discovery_udp);
            Close_Uv_Handle(reinterpret_cast<uv_handle_t*>(&state->discovery_udp));
        }
        if (state->discovery_timer_initialized) {
            Close_Uv_Handle(reinterpret_cast<uv_handle_t*>(&state->discovery_timer));
        }
        while (state->discovery_loop_initialized && uv_loop_close(&state->discovery_loop) == UV_EBUSY) {
            uv_run(&state->discovery_loop, UV_RUN_NOWAIT);
        }
        state->discovery_loop_initialized = false;
        state->discovery_udp_initialized = false;
        state->discovery_timer_initialized = false;
    }

    void Disconnect_Queries(s_router_session_browser_state* state) {
        for (std::unique_ptr<s_router_query>& query : state->queries) {
            if (query != nullptr) {
                Router_Client_Disconnect(&query->client);
            }
        }
        state->queries.clear();
    }

    void Complete_Refresh(s_router_session_browser_state* state) {
        if (state->config.mode != ROUTER_SESSION_BROWSER_DIRECT_ENDPOINT) {
            Disconnect_Queries(state);
        }
        state->phase = BROWSER_IDLE;
        state->revision++;
    }

    bool Start_Direct_Query(s_router_session_browser_state* state, const s_router_endpoint& endpoint) {
        s_router_query* query = nullptr;
        if (state->queries.size() == 1 && state->queries[0] != nullptr && Endpoints_Match(state->queries[0]->endpoint, endpoint)) {
            query = state->queries[0].get();
        } else {
            Disconnect_Queries(state);
            std::unique_ptr<s_router_query> direct_query = std::make_unique<s_router_query>();
            direct_query->endpoint = endpoint;
            state->queries.push_back(std::move(direct_query));
            query = state->queries[0].get();
        }

        query->endpoint = endpoint;
        query->deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(state->config.request_timeout_ms);
        query->started = false;
        query->complete = false;

        Router_Client_Pump(&query->client);
        if (!Router_Client_Is_Connected(&query->client)) {
            Router_Client_Disconnect(&query->client);

            s_router_client_config client_config;
            Router_Copy_String(client_config.host, sizeof(client_config.host), endpoint.host);
            client_config.tcp_port = endpoint.tcp_port;
            client_config.connect_timeout_ms = state->config.connect_timeout_ms;
            Router_Client_Init(&query->client, &client_config);

            if (!Router_Client_Connect(&query->client)) {
                if (state->last_error == 0) {
                    state->last_error = Router_Client_Last_Error(&query->client);
                }
                query->complete = true;
                state->phase = BROWSER_QUERYING;
                return false;
            }

            if (!Router_Client_Send_Hello(&query->client, state->config.client_name, ROUTER_VERSION, state->next_request_id++)) {
                if (state->last_error == 0) {
                    state->last_error = Router_Client_Last_Error(&query->client);
                }
                Router_Client_Disconnect(&query->client);
                query->complete = true;
                state->phase = BROWSER_QUERYING;
                return false;
            }
        }

        if (!Router_Client_Request_Session_List(&query->client, state->next_request_id++)) {
            if (state->last_error == 0) {
                state->last_error = Router_Client_Last_Error(&query->client);
            }
            Router_Client_Disconnect(&query->client);
            query->complete = true;
            state->phase = BROWSER_QUERYING;
            return false;
        }

        query->started = true;
        state->phase = BROWSER_QUERYING;
        return true;
    }

    bool Start_Query(s_router_session_browser_state* state, const s_router_endpoint& endpoint) {
        std::unique_ptr<s_router_query> query = std::make_unique<s_router_query>();
        query->endpoint = endpoint;
        query->deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(state->config.request_timeout_ms);

        s_router_client_config client_config;
        Router_Copy_String(client_config.host, sizeof(client_config.host), endpoint.host);
        client_config.tcp_port = endpoint.tcp_port;
        client_config.connect_timeout_ms = state->config.connect_timeout_ms;
        Router_Client_Init(&query->client, &client_config);

        if (!Router_Client_Connect(&query->client)) {
            if (state->last_error == 0) {
                state->last_error = Router_Client_Last_Error(&query->client);
            }
            query->complete = true;
            state->queries.push_back(std::move(query));
            return false;
        }

        query->started = true;
        Router_Client_Send_Hello(&query->client, state->config.client_name, ROUTER_VERSION, state->next_request_id++);
        Router_Client_Request_Session_List(&query->client, state->next_request_id++);
        state->queries.push_back(std::move(query));
        return true;
    }

    void Start_Queries(s_router_session_browser_state* state) {
        state->queries.clear();
        for (const s_router_endpoint& endpoint : state->endpoints) {
            Start_Query(state, endpoint);
        }
        state->phase = BROWSER_QUERYING;
        if (state->queries.empty()) {
            Complete_Refresh(state);
        }
    }

    bool Start_Discovery(s_router_session_browser_state* state) {
        int result = uv_loop_init(&state->discovery_loop);
        if (result != 0) {
            state->last_error = result;
            return false;
        }
        state->discovery_loop.data = state;
        state->discovery_loop_initialized = true;

        result = uv_udp_init(&state->discovery_loop, &state->discovery_udp);
        if (result != 0) {
            state->last_error = result;
            return false;
        }
        state->discovery_udp.data = state;
        state->discovery_udp_initialized = true;

        sockaddr_in bind_address{};
        result = uv_ip4_addr("0.0.0.0", 0, &bind_address);
        if (result == 0) {
            result = uv_udp_bind(&state->discovery_udp, reinterpret_cast<const sockaddr*>(&bind_address), 0);
        }
        if (result != 0) {
            state->last_error = result;
            return false;
        }

        result = uv_udp_recv_start(&state->discovery_udp, Discovery_Alloc_Callback, Discovery_Recv_Callback);
        if (result != 0) {
            state->last_error = result;
            return false;
        }

        uv_udp_set_broadcast(&state->discovery_udp, 1);
        Send_Discovery_Probe(state, "255.255.255.255");

        result = uv_timer_init(&state->discovery_loop, &state->discovery_timer);
        if (result != 0) {
            state->last_error = result;
            return false;
        }
        state->discovery_timer.data = state;
        state->discovery_timer_initialized = true;
        uv_timer_start(&state->discovery_timer, Discovery_Timer_Callback, state->config.discovery_timeout_ms, 0);
        state->phase = BROWSER_DISCOVERING;
        return true;
    }

    void Pump_Query(s_router_session_browser_state* state, s_router_query* query) {
        if (query == nullptr || query->complete) {
            return;
        }

        Router_Client_Pump(&query->client);
        s_router_client_event event;
        while (Router_Client_Pop_Event(&query->client, &event)) {
            if (event.kind == ROUTER_CLIENT_EVENT_WELCOME) {
                if (query->endpoint.router_id[0] == '\0') {
                    Router_Copy_String(query->endpoint.router_id, sizeof(query->endpoint.router_id), Router_Client_Router_Id(&query->client));
                }
            } else if (event.kind == ROUTER_CLIENT_EVENT_SESSION_LIST) {
                for (const s_router_session_info& session : event.sessions) {
                    s_router_session_record record;
                    record.router = query->endpoint;
                    record.session = session;
                    Add_Unique_Session_Record(&state->sessions, record);
                }
                query->complete = true;
            } else if (event.kind == ROUTER_CLIENT_EVENT_ERROR ||
                event.kind == ROUTER_CLIENT_EVENT_PROTOCOL_ERROR ||
                event.kind == ROUTER_CLIENT_EVENT_DISCONNECTED) {
                query->complete = true;
            }
        }

        if (!query->complete && std::chrono::steady_clock::now() >= query->deadline) {
            query->complete = true;
        }
    }

    s_router_session_browser::s_router_session_browser()
        : state(std::make_unique<s_router_session_browser_state>()) {
    }

    s_router_session_browser::~s_router_session_browser() {
        Router_Session_Browser_Shutdown(this);
    }

    void Router_Session_Browser_Init(s_router_session_browser* browser, const s_router_session_browser_config* config) {
        if (browser == nullptr) {
            return;
        }
        if (browser->state == nullptr) {
            browser->state = std::make_unique<s_router_session_browser_state>();
        }
        Router_Session_Browser_Shutdown(browser);
        browser->state->config = config != nullptr ? *config : s_router_session_browser_config{};
        browser->state->phase = BROWSER_IDLE;
        browser->state->revision = 0;
        browser->state->next_request_id = 1;
        browser->state->last_error = 0;
        browser->state->sessions.clear();
        browser->state->endpoints.clear();
    }

    void Router_Session_Browser_Shutdown(s_router_session_browser* browser) {
        if (browser == nullptr || browser->state == nullptr) {
            return;
        }
        Finish_Discovery(browser->state.get());
        Disconnect_Queries(browser->state.get());
        browser->state->phase = BROWSER_IDLE;
    }

    bool Router_Session_Browser_Refresh(s_router_session_browser* browser) {
        if (browser == nullptr || browser->state == nullptr) {
            return false;
        }

        s_router_session_browser_state* state = browser->state.get();
        const bool preserve_direct_query =
            state->config.mode == ROUTER_SESSION_BROWSER_DIRECT_ENDPOINT &&
            state->phase == BROWSER_IDLE &&
            state->queries.size() == 1 &&
            state->queries[0] != nullptr;
        if (!preserve_direct_query) {
            Router_Session_Browser_Shutdown(browser);
        }
        state->sessions.clear();
        state->endpoints.clear();
        state->last_error = 0;
        state->discovery_done = false;

        const uint32_t initial_endpoint_count = state->config.initial_endpoint_count < ROUTER_SESSION_BROWSER_MAX_INITIAL_ENDPOINTS ? state->config.initial_endpoint_count : ROUTER_SESSION_BROWSER_MAX_INITIAL_ENDPOINTS;
        for (uint32_t i = 0; i < initial_endpoint_count; ++i) {
            if (state->config.initial_endpoints[i].tcp_port != 0) {
                Add_Unique_Endpoint(&state->endpoints, state->config.initial_endpoints[i]);
            }
        }

        if (state->config.mode == ROUTER_SESSION_BROWSER_DIRECT_ENDPOINT) {
            s_router_endpoint endpoint;
            Router_Copy_String(endpoint.host, sizeof(endpoint.host), state->config.host);
            endpoint.tcp_port = state->config.tcp_port;
            state->endpoints.push_back(endpoint);
            Start_Direct_Query(state, endpoint);
            return true;
        }

        if (!state->endpoints.empty()) {
            Start_Queries(state);
            return true;
        }

        if (!Start_Discovery(state)) {
            Finish_Discovery(state);
            Complete_Refresh(state);
        }
        return true;
    }

    void Router_Session_Browser_Pump(s_router_session_browser* browser) {
        if (browser == nullptr || browser->state == nullptr) {
            return;
        }

        s_router_session_browser_state* state = browser->state.get();
        if (state->phase == BROWSER_DISCOVERING) {
            uv_run(&state->discovery_loop, UV_RUN_NOWAIT);
            if (state->discovery_done) {
                Finish_Discovery(state);
                Start_Queries(state);
            }
        }

        if (state->phase == BROWSER_QUERYING) {
            bool all_complete = true;
            for (std::unique_ptr<s_router_query>& query : state->queries) {
                Pump_Query(state, query.get());
                if (query != nullptr && !query->complete) {
                    all_complete = false;
                }
            }
            if (all_complete) {
                Complete_Refresh(state);
            }
        }

        if (state->phase == BROWSER_IDLE &&
            state->config.mode == ROUTER_SESSION_BROWSER_DIRECT_ENDPOINT &&
            state->queries.size() == 1 &&
            state->queries[0] != nullptr) {
            Router_Client_Pump(&state->queries[0]->client);
        }
    }

    bool Router_Session_Browser_Is_Refreshing(const s_router_session_browser* browser) {
        return browser != nullptr && browser->state != nullptr && browser->state->phase != BROWSER_IDLE;
    }

    uint32_t Router_Session_Browser_Revision(const s_router_session_browser* browser) {
        if (browser == nullptr || browser->state == nullptr) {
            return 0;
        }
        return browser->state->revision;
    }

    int Router_Session_Browser_Last_Error(const s_router_session_browser* browser) {
        if (browser == nullptr || browser->state == nullptr) {
            return 0;
        }
        return browser->state->last_error;
    }

    const char* Router_Session_Browser_Last_Error_Message(const s_router_session_browser* browser) {
        const int error = Router_Session_Browser_Last_Error(browser);
        return error == 0 ? "no error" : uv_strerror(error);
    }

    const std::vector<s_router_session_record>& Router_Session_Browser_Sessions(const s_router_session_browser* browser) {
        if (browser == nullptr || browser->state == nullptr) {
            return Empty_Session_Records();
        }
        return browser->state->sessions;
    }

} // namespace network::router