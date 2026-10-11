#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#define ROUTER_PROTOCOL_MAGIC 0x5452324fU
#define ROUTER_PROTOCOL_VERSION 2U
#define ROUTER_VERSION 2U
#define ROUTER_MESSAGE_HEADER_SIZE 32U
#define ROUTER_PACKET_HEADER_SIZE 2U
#define ROUTER_PACKET_FLAG_PAYLOAD_LZ4_COMPRESSED (1U << 0)
#define ROUTER_DEFAULT_MAX_FRAME_SIZE (16U * 1024U * 1024U)

#define ROUTER_DISCOVERY_MAGIC 0x4452324fU
#define ROUTER_DISCOVERY_VERSION 1U

#define ROUTER_MIN_ASSIGNED_NETWORK_ID 1U
#define ROUTER_MAX_ASSIGNED_NETWORK_ID 0x7FFDU
#define ROUTER_RESERVED_BROADCAST_EXCLUDING_SELF_ID 0x7FFEU
#define ROUTER_RESERVED_BROADCAST_INCLUDING_SELF_ID 0x7FFFU
#define ROUTER_ASSIGNED_NETWORK_ID_COUNT (ROUTER_MAX_ASSIGNED_NETWORK_ID - ROUTER_MIN_ASSIGNED_NETWORK_ID + 1U)

#define ROUTER_ADDRESS_SIZE 64U
#define ROUTER_ID_SIZE 64U
#define ROUTER_NAME_SIZE 64U
#define ROUTER_PLAYER_NAME_SIZE 64U
#define ROUTER_SESSION_NAME_SIZE 128U
#define ROUTER_BUILD_STRING_SIZE 64U
#define ROUTER_ERROR_MESSAGE_SIZE 256U
#define ROUTER_MEMBER_REMOVAL_REASON_SIZE 1025U

#define ROUTER_FLAG_BROADCAST_INCLUDE_SENDER (1U << 0)
#define ROUTER_FLAG_SESSION_CLOSED (1U << 1)
#define ROUTER_FLAG_SESSION_OPEN (1U << 2)

#define ROUTER_MEMBER_JOINED 1U
#define ROUTER_MEMBER_LEFT 2U

namespace network::router {

    enum e_router_message_kind : uint16_t {
        ROUTER_MESSAGE_HELLO = 1,
        ROUTER_MESSAGE_WELCOME = 2,
        ROUTER_MESSAGE_REGISTER_SESSION = 3,
        ROUTER_MESSAGE_SESSION_REGISTERED = 4,
        ROUTER_MESSAGE_UNREGISTER_SESSION = 5,
        ROUTER_MESSAGE_LIST_SESSIONS = 6,
        ROUTER_MESSAGE_SESSION_LIST = 7,
        ROUTER_MESSAGE_JOIN_SESSION = 8,
        ROUTER_MESSAGE_JOINED_SESSION = 9,
        ROUTER_MESSAGE_LEAVE_SESSION = 10,
        ROUTER_MESSAGE_LEFT_SESSION = 11,
        ROUTER_MESSAGE_SEND_TO = 12,
        ROUTER_MESSAGE_SEND_BROADCAST = 13,
        ROUTER_MESSAGE_GAME_DATA = 14,
        ROUTER_MESSAGE_OK = 16,
        ROUTER_MESSAGE_ERROR = 17,
        ROUTER_MESSAGE_SET_SESSION_OPEN = 18,
        ROUTER_MESSAGE_MEMBER_LIST = 19,
        ROUTER_MESSAGE_MEMBER_JOINED = 20,
        ROUTER_MESSAGE_MEMBER_LEFT = 21,
        ROUTER_MESSAGE_SESSION_CLOSED = 22,
        ROUTER_MESSAGE_REMOVE_MEMBER = 23,
    };

    enum e_router_error_code : uint32_t {
        ROUTER_ERROR_INVALID_MESSAGE = 1,
        ROUTER_ERROR_NOT_REGISTERED = 2,
        ROUTER_ERROR_ALREADY_REGISTERED = 3,
        ROUTER_ERROR_ALREADY_IN_SESSION = 4,
        ROUTER_ERROR_NOT_IN_SESSION = 5,
        ROUTER_ERROR_SESSION_NOT_FOUND = 6,
        ROUTER_ERROR_SESSION_FULL = 7,
        ROUTER_ERROR_PERMISSION_DENIED = 8,
        ROUTER_ERROR_TARGET_NOT_FOUND = 9,
        ROUTER_ERROR_BUILD_MISMATCH = 10,
        ROUTER_ERROR_SESSION_CLOSED = 11,
        ROUTER_ERROR_ROUTER_FULL = 12,
    };

    enum e_router_discovery_kind : uint16_t {
        ROUTER_DISCOVERY_PROBE = 1,
        ROUTER_DISCOVERY_REPLY = 2,
    };

    struct s_router_message_header {
        e_router_message_kind kind = ROUTER_MESSAGE_HELLO;
        uint32_t flags = 0;
        uint32_t session_id = 0;
        uint32_t sender_network_id = 0;
        uint32_t target_network_id = 0;
        uint32_t request_id = 0;
    };

    class Bit_Stream {
      public:
        Bit_Stream();
        Bit_Stream(const uint8_t* data, uint32_t size);
        Bit_Stream(const Bit_Stream& other);
        Bit_Stream(Bit_Stream&& other) noexcept;
        ~Bit_Stream();

        Bit_Stream& operator=(const Bit_Stream& other);
        Bit_Stream& operator=(Bit_Stream&& other) noexcept;

        void Clear();
        void Set_Buffer(const uint8_t* data, uint32_t size);
        void Reserve(uint32_t size);
        void Resize(uint32_t size);
        void Consume(uint32_t size);
        void Write_Uint8(uint8_t value);
        void Write_Uint16(uint16_t value);
        void Write_Uint32(uint32_t value);
        void Write_Int32(int32_t value);
        void Write_Bytes(const uint8_t* data, uint32_t size);
        void Write_String(const char* string);

        bool Read_Uint8(uint8_t* value);
        bool Read_Uint16(uint16_t* value);
        bool Read_Uint32(uint32_t* value);
        bool Read_Int32(int32_t* value);
        bool Read_Bytes(uint8_t* output, uint32_t output_size);
        bool Read_String(uint32_t* string_length, uint32_t output_size, char* output);
        bool Read_ASCIIZ_String(uint32_t* string_length, uint32_t output_size, char* output);
        bool At_End() const;
        uint8_t* Data();
        const uint8_t* Data() const;
        uint32_t Size() const;
        uint32_t Tell() const;

      private:
        void Grow_To(uint32_t size);

        uint8_t* data_ = nullptr;
        uint32_t size_ = 0;
        uint32_t capacity_ = 0;
        uint32_t read_offset_ = 0;
    };

    struct s_router_message {
        s_router_message_header header;
        Bit_Stream payload;
    };

    struct s_router_frame_codec_stats {
        uint64_t uncompressed_payload_bytes_in = 0;
        uint64_t compressed_payload_bytes_in = 0;
        uint64_t uncompressed_payload_bytes_out = 0;
        uint64_t compressed_payload_bytes_out = 0;
        uint32_t last_uncompressed_payload_bytes_in = 0;
        uint32_t last_compressed_payload_bytes_in = 0;
    };

    struct s_router_frame_codec_state;

    struct s_router_frame_codec {
        s_router_frame_codec();
        ~s_router_frame_codec();

        s_router_frame_codec(const s_router_frame_codec&) = delete;
        s_router_frame_codec& operator=(const s_router_frame_codec&) = delete;

        void Reset();
        s_router_frame_codec_stats Stats() const;

        std::unique_ptr<s_router_frame_codec_state> state;
    };

    enum e_frame_read_status {
        FRAME_READ_NEED_MORE_DATA,
        FRAME_READ_MESSAGE_READY,
        FRAME_READ_PROTOCOL_ERROR,
    };

    struct s_router_hello_payload {
        char client_name[ROUTER_PLAYER_NAME_SIZE] = {};
        uint32_t protocol_version = 0;
    };

    struct s_router_welcome_payload {
        uint32_t network_id = 0;
        char router_id[ROUTER_ID_SIZE] = {};
    };

    struct s_router_session_registration_payload {
        char session_name[ROUTER_SESSION_NAME_SIZE] = {};
        char host_name[ROUTER_PLAYER_NAME_SIZE] = {};
        char build_string[ROUTER_BUILD_STRING_SIZE] = {};
        uint16_t max_players = 0;
    };

    struct s_router_join_session_payload {
        char build_string[ROUTER_BUILD_STRING_SIZE] = {};
    };

    struct s_router_session_info {
        uint32_t session_id = 0;
        char router_id[ROUTER_ID_SIZE] = {};
        char session_name[ROUTER_SESSION_NAME_SIZE] = {};
        char host_name[ROUTER_PLAYER_NAME_SIZE] = {};
        uint16_t current_players = 0;
        uint16_t max_players = 0;
        char build_string[ROUTER_BUILD_STRING_SIZE] = {};
        bool open = false;
    };

    struct s_router_member_info {
        uint32_t network_id = 0;
        char player_name[ROUTER_PLAYER_NAME_SIZE] = {};
        char build_string[ROUTER_BUILD_STRING_SIZE] = {};
        bool owner = false;
    };

    struct s_router_member_event_payload {
        uint32_t event = 0;
        s_router_member_info member;
    };

    struct s_router_error_payload {
        e_router_error_code code = ROUTER_ERROR_INVALID_MESSAGE;
        char message[ROUTER_ERROR_MESSAGE_SIZE] = {};
    };

    struct s_router_discovery_reply {
        uint16_t tcp_port = 0;
        uint16_t protocol_version = 0;
        char router_id[ROUTER_ID_SIZE] = {};
        char router_name[ROUTER_NAME_SIZE] = {};
    };

    bool Is_Known_Message_Kind(uint16_t value);

    void Encode_Message_Body(const s_router_message* message, Bit_Stream* output);
    uint32_t Encoded_Frame_Max_Size(const s_router_message* message);
    void Encode_Frame(const s_router_message* message, s_router_frame_codec* codec, Bit_Stream* output);
    void Encode_Frame(const s_router_message* message, Bit_Stream* output);

    bool Decode_Message_Body(const uint8_t* data, uint32_t size, s_router_message* message, std::string* error_message);
    e_frame_read_status Try_Read_Frame(Bit_Stream* input_buffer, uint32_t max_frame_size, s_router_frame_codec* codec, s_router_message* message, std::string* error_message);
    e_frame_read_status Try_Read_Frame(Bit_Stream* input_buffer, uint32_t max_frame_size, s_router_message* message, std::string* error_message);

    void Encode_Text_Payload(const char* text, Bit_Stream* payload);
    bool Decode_Text_Payload(const Bit_Stream* payload, uint32_t output_size, char* output, std::string* error_message);

    void Encode_Hello_Payload(const char* client_name, uint32_t protocol_version, Bit_Stream* payload);
    bool Decode_Hello_Payload(const Bit_Stream* payload, s_router_hello_payload* hello, std::string* error_message);

    void Encode_Welcome_Payload(uint32_t network_id, const char* router_id, Bit_Stream* payload);
    bool Decode_Welcome_Payload(const Bit_Stream* payload, s_router_welcome_payload* welcome, std::string* error_message);

    void Encode_Session_Registration_Payload(const s_router_session_registration_payload* registration, Bit_Stream* payload);
    bool Decode_Session_Registration_Payload(const Bit_Stream* payload, s_router_session_registration_payload* registration, std::string* error_message);

    void Encode_Join_Session_Payload(const s_router_join_session_payload* join, Bit_Stream* payload);
    bool Decode_Join_Session_Payload(const Bit_Stream* payload, s_router_join_session_payload* join, std::string* error_message);

    void Encode_Session_Info_Payload(const s_router_session_info* session_info, Bit_Stream* payload);
    bool Decode_Session_Info_Payload(const Bit_Stream* payload, s_router_session_info* session_info, std::string* error_message);

    void Encode_Session_List_Payload(const std::vector<s_router_session_info>* sessions, Bit_Stream* payload);
    bool Decode_Session_List_Payload(const Bit_Stream* payload, std::vector<s_router_session_info>* sessions, std::string* error_message);

    void Encode_Member_Info_Payload(const s_router_member_info* member, Bit_Stream* payload);
    bool Decode_Member_Info_Payload(const Bit_Stream* payload, s_router_member_info* member, std::string* error_message);

    void Encode_Member_List_Payload(const std::vector<s_router_member_info>* members, Bit_Stream* payload);
    bool Decode_Member_List_Payload(const Bit_Stream* payload, std::vector<s_router_member_info>* members, std::string* error_message);

    void Encode_Member_Event_Payload(uint32_t event, const s_router_member_info* member, Bit_Stream* payload);
    bool Decode_Member_Event_Payload(const Bit_Stream* payload, s_router_member_event_payload* event, std::string* error_message);

    void Encode_Error_Payload(e_router_error_code code, const char* message, Bit_Stream* payload);
    bool Decode_Error_Payload(const Bit_Stream* payload, s_router_error_payload* error, std::string* error_message);

    void Encode_Discovery_Probe(Bit_Stream* payload);
    void Encode_Discovery_Reply(uint16_t tcp_port, const char* router_id, const char* router_name, Bit_Stream* payload);
    bool Is_Discovery_Probe(const uint8_t* data, uint32_t size);
    bool Decode_Discovery_Reply(const Bit_Stream* payload, s_router_discovery_reply* reply, std::string* error_message);

    void Router_Copy_String(char* destination, uint32_t destination_size, const char* source);

} // namespace network::router