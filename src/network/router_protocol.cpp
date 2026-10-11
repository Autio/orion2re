#include "network/router_protocol.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <utility>

#include <lz4.h>

namespace network::router {

    constexpr uint32_t ROUTER_LZ4_MAX_DICTIONARY_SIZE = 64U * 1024U;
    constexpr uint16_t ROUTER_KNOWN_PACKET_FLAGS = ROUTER_PACKET_FLAG_PAYLOAD_LZ4_COMPRESSED;

    [[noreturn]] void Crash_Write_Failed() {
        std::abort();
    }

    void Set_Error(std::string* error_message, const char* message) {
        if (error_message != nullptr) {
            *error_message = message;
        }
    }

    bool Read_Header_Uint16(const uint8_t* data, uint32_t offset, uint16_t* value) {
        *value = (uint16_t)data[offset] | ((uint16_t)data[offset + 1] << 8);
        return true;
    }

    bool Read_Header_Uint32(const uint8_t* data, uint32_t offset, uint32_t* value) {
        *value =
            (uint32_t)data[offset] |
            ((uint32_t)data[offset + 1] << 8) |
            ((uint32_t)data[offset + 2] << 16) |
            ((uint32_t)data[offset + 3] << 24);
        return true;
    }

    bool Write_Session_Info(Bit_Stream* output, const s_router_session_info* session_info) {
        if (output == nullptr || session_info == nullptr) {
            return false;
        }

        output->Write_Uint32(session_info->session_id);
        output->Write_String(session_info->router_id);
        output->Write_String(session_info->session_name);
        output->Write_String(session_info->host_name);
        output->Write_Uint16(session_info->current_players);
        output->Write_Uint16(session_info->max_players);
        output->Write_String(session_info->build_string);
        output->Write_Uint8(session_info->open ? 1 : 0);
        return true;
    }

    bool Read_Session_Info(Bit_Stream* reader, s_router_session_info* session_info) {
        uint8_t open = 0;
        if (!reader->Read_Uint32(&session_info->session_id) ||
            !reader->Read_String(nullptr, sizeof(session_info->router_id), session_info->router_id) ||
            !reader->Read_String(nullptr, sizeof(session_info->session_name), session_info->session_name) ||
            !reader->Read_String(nullptr, sizeof(session_info->host_name), session_info->host_name) ||
            !reader->Read_Uint16(&session_info->current_players) ||
            !reader->Read_Uint16(&session_info->max_players) ||
            !reader->Read_String(nullptr, sizeof(session_info->build_string), session_info->build_string) ||
            !reader->Read_Uint8(&open)) {
            return false;
        }
        session_info->open = open != 0;
        return true;
    }

    bool Write_Member_Info(Bit_Stream* output, const s_router_member_info* member) {
        if (output == nullptr || member == nullptr) {
            return false;
        }

        output->Write_Uint32(member->network_id);
        output->Write_String(member->player_name);
        output->Write_String(member->build_string);
        output->Write_Uint8(member->owner ? 1 : 0);
        return true;
    }

    bool Read_Member_Info(Bit_Stream* reader, s_router_member_info* member) {
        uint8_t owner = 0;
        if (!reader->Read_Uint32(&member->network_id) ||
            !reader->Read_String(nullptr, sizeof(member->player_name), member->player_name) ||
            !reader->Read_String(nullptr, sizeof(member->build_string), member->build_string) ||
            !reader->Read_Uint8(&owner)) {
            return false;
        }
        member->owner = owner != 0;
        return true;
    }

    uint32_t Max_Wire_Frame_Size(uint32_t max_frame_size) {
        uint64_t max_wire_size = max_frame_size;
        if (max_frame_size >= ROUTER_MESSAGE_HEADER_SIZE) {
            const uint32_t max_payload_size = max_frame_size - ROUTER_MESSAGE_HEADER_SIZE;
            if (max_payload_size <= (uint32_t)std::numeric_limits<int>::max()) {
                const int compressed_bound = LZ4_compressBound((int)max_payload_size);
                if (compressed_bound > 0) {
                    const uint64_t compressed_wire_size = (uint64_t)ROUTER_MESSAGE_HEADER_SIZE + (uint32_t)compressed_bound;
                    if (compressed_wire_size > max_wire_size) {
                        max_wire_size = compressed_wire_size;
                    }
                }
            }
        }

        max_wire_size += ROUTER_PACKET_HEADER_SIZE;
        if (max_wire_size > std::numeric_limits<uint32_t>::max()) {
            return std::numeric_limits<uint32_t>::max();
        }
        return (uint32_t)max_wire_size;
    }

    void Save_Lz4_Dictionary(Bit_Stream* dictionary, const uint8_t* data, uint32_t size) {
        if (dictionary == nullptr || size == 0) {
            return;
        }

        Bit_Stream combined;
        combined.Reserve(dictionary->Size() + size);
        combined.Write_Bytes(dictionary->Data(), dictionary->Size());
        combined.Write_Bytes(data, size);

        const uint32_t dictionary_size = combined.Size() > ROUTER_LZ4_MAX_DICTIONARY_SIZE ? ROUTER_LZ4_MAX_DICTIONARY_SIZE : combined.Size();
        dictionary->Set_Buffer(combined.Data() + combined.Size() - dictionary_size, dictionary_size);
    }

    bool Decode_Message_Header(const uint8_t* data, uint32_t size, s_router_message* message, uint32_t* payload_size, std::string* error_message) {
        if (message == nullptr || payload_size == nullptr) {
            Set_Error(error_message, "message output is null");
            return false;
        }
        if (data == nullptr || size < ROUTER_MESSAGE_HEADER_SIZE) {
            Set_Error(error_message, "message body is shorter than the fixed header");
            return false;
        }

        uint32_t magic = 0;
        uint16_t version = 0;
        uint16_t kind = 0;

        Read_Header_Uint32(data, 0, &magic);
        Read_Header_Uint16(data, 4, &version);
        Read_Header_Uint16(data, 6, &kind);
        if (magic != ROUTER_PROTOCOL_MAGIC) {
            Set_Error(error_message, "invalid router protocol magic");
            return false;
        }
        if (version != ROUTER_PROTOCOL_VERSION) {
            Set_Error(error_message, "unsupported router protocol version");
            return false;
        }
        if (!Is_Known_Message_Kind(kind)) {
            Set_Error(error_message, "unknown router message kind");
            return false;
        }

        s_router_message decoded;
        decoded.header.kind = (e_router_message_kind)kind;
        Read_Header_Uint32(data, 8, &decoded.header.flags);
        Read_Header_Uint32(data, 12, &decoded.header.session_id);
        Read_Header_Uint32(data, 16, &decoded.header.sender_network_id);
        Read_Header_Uint32(data, 20, &decoded.header.target_network_id);
        Read_Header_Uint32(data, 24, &decoded.header.request_id);
        Read_Header_Uint32(data, 28, payload_size);

        *message = std::move(decoded);
        return true;
    }

    struct s_router_frame_codec_state {
        LZ4_stream_t* encode_stream = nullptr;
        LZ4_streamDecode_t* decode_stream = nullptr;
        Bit_Stream encode_dictionary;
        Bit_Stream decode_dictionary;
        s_router_frame_codec_stats stats;
    };

    s_router_frame_codec::s_router_frame_codec()
        : state(std::make_unique<s_router_frame_codec_state>()) {
        state->encode_stream = LZ4_createStream();
        state->decode_stream = LZ4_createStreamDecode();
        if (state->encode_stream == nullptr || state->decode_stream == nullptr) {
            Crash_Write_Failed();
        }
    }

    s_router_frame_codec::~s_router_frame_codec() {
        if (state != nullptr) {
            if (state->encode_stream != nullptr) {
                LZ4_freeStream(state->encode_stream);
                state->encode_stream = nullptr;
            }
            if (state->decode_stream != nullptr) {
                LZ4_freeStreamDecode(state->decode_stream);
                state->decode_stream = nullptr;
            }
        }
    }

    void s_router_frame_codec::Reset() {
        if (state == nullptr) {
            state = std::make_unique<s_router_frame_codec_state>();
        }
        if (state->encode_stream == nullptr) {
            state->encode_stream = LZ4_createStream();
        } else {
            LZ4_resetStream_fast(state->encode_stream);
        }
        if (state->decode_stream == nullptr) {
            state->decode_stream = LZ4_createStreamDecode();
        } else {
            LZ4_setStreamDecode(state->decode_stream, nullptr, 0);
        }
        if (state->encode_stream == nullptr || state->decode_stream == nullptr) {
            Crash_Write_Failed();
        }
        state->encode_dictionary.Clear();
        state->decode_dictionary.Clear();
        state->stats = {};
    }

    s_router_frame_codec_stats s_router_frame_codec::Stats() const {
        if (state == nullptr) {
            return {};
        }
        return state->stats;
    }

    void Router_Copy_String(char* destination, uint32_t destination_size, const char* source) {
        if (destination == nullptr || destination_size == 0) {
            return;
        }

        if (source == nullptr) {
            destination[0] = '\0';
            return;
        }

        std::snprintf(destination, destination_size, "%s", source);
    }

    Bit_Stream::Bit_Stream() = default;

    Bit_Stream::Bit_Stream(const uint8_t* data, uint32_t size) {
        Set_Buffer(data, size);
    }

    Bit_Stream::Bit_Stream(const Bit_Stream& other) {
        Set_Buffer(other.Data(), other.Size());
        read_offset_ = other.read_offset_;
    }

    Bit_Stream::Bit_Stream(Bit_Stream&& other) noexcept
        : data_(other.data_), size_(other.size_), capacity_(other.capacity_), read_offset_(other.read_offset_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
        other.read_offset_ = 0;
    }

    Bit_Stream::~Bit_Stream() {
        Clear();
    }

    Bit_Stream& Bit_Stream::operator=(const Bit_Stream& other) {
        if (this != &other) {
            Set_Buffer(other.Data(), other.Size());
            read_offset_ = other.read_offset_;
        }
        return *this;
    }

    Bit_Stream& Bit_Stream::operator=(Bit_Stream&& other) noexcept {
        if (this != &other) {
            Clear();
            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;
            read_offset_ = other.read_offset_;
            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
            other.read_offset_ = 0;
        }
        return *this;
    }

    void Bit_Stream::Clear() {
        if (data_ != nullptr) {
            std::free(data_);
        }
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
        read_offset_ = 0;
    }

    void Bit_Stream::Set_Buffer(const uint8_t* data, uint32_t size) {
        Clear();
        if (size == 0) {
            return;
        }
        if (data == nullptr) {
            Crash_Write_Failed();
        }
        Reserve(size);
        std::memcpy(data_, data, size);
        size_ = size;
    }

    void Bit_Stream::Reserve(uint32_t size) {
        Grow_To(size);
    }

    void Bit_Stream::Resize(uint32_t size) {
        Grow_To(size);
        size_ = size;
        if (read_offset_ > size_) {
            read_offset_ = size_;
        }
    }

    void Bit_Stream::Consume(uint32_t size) {
        if (size >= size_) {
            size_ = 0;
            read_offset_ = 0;
            return;
        }

        std::memmove(data_, data_ + size, size_ - size);
        size_ -= size;
        read_offset_ = read_offset_ > size ? read_offset_ - size : 0;
    }

    void Bit_Stream::Write_Uint8(uint8_t value) {
        Write_Bytes(&value, 1);
    }

    void Bit_Stream::Write_Uint16(uint16_t value) {
        const uint8_t bytes[2] = {
            (uint8_t)(value & 0xffu),
            (uint8_t)((value >> 8) & 0xffu),
        };
        Write_Bytes(bytes, sizeof(bytes));
    }

    void Bit_Stream::Write_Uint32(uint32_t value) {
        const uint8_t bytes[4] = {
            (uint8_t)(value & 0xffu),
            (uint8_t)((value >> 8) & 0xffu),
            (uint8_t)((value >> 16) & 0xffu),
            (uint8_t)((value >> 24) & 0xffu),
        };
        Write_Bytes(bytes, sizeof(bytes));
    }

    void Bit_Stream::Write_Int32(int32_t value) {
        Write_Uint32((uint32_t)value);
    }

    void Bit_Stream::Write_Bytes(const uint8_t* data, uint32_t size) {
        if (size == 0) {
            return;
        }
        if (data == nullptr || size_ > std::numeric_limits<uint32_t>::max() - size) {
            Crash_Write_Failed();
        }
        const uint32_t required_size = size_ + size;
        Grow_To(required_size);
        std::memcpy(data_ + size_, data, size);
        size_ = required_size;
    }

    void Bit_Stream::Write_String(const char* string) {
        const uint32_t length = string != nullptr ? (uint32_t)std::strlen(string) : 0;
        Write_Uint32(length);
        Write_Bytes((const uint8_t*)string, length);
    }

    bool Bit_Stream::Read_Uint8(uint8_t* value) {
        if (value == nullptr || read_offset_ + 1 > size_) {
            return false;
        }
        *value = data_[read_offset_++];
        return true;
    }

    bool Bit_Stream::Read_Uint16(uint16_t* value) {
        if (value == nullptr || read_offset_ + 2 > size_) {
            return false;
        }
        *value = (uint16_t)data_[read_offset_] | ((uint16_t)data_[read_offset_ + 1] << 8);
        read_offset_ += 2;
        return true;
    }

    bool Bit_Stream::Read_Uint32(uint32_t* value) {
        if (value == nullptr || read_offset_ + 4 > size_) {
            return false;
        }
        *value =
            (uint32_t)data_[read_offset_] |
            ((uint32_t)data_[read_offset_ + 1] << 8) |
            ((uint32_t)data_[read_offset_ + 2] << 16) |
            ((uint32_t)data_[read_offset_ + 3] << 24);
        read_offset_ += 4;
        return true;
    }

    bool Bit_Stream::Read_Int32(int32_t* value) {
        uint32_t raw_value = 0;
        if (!Read_Uint32(&raw_value)) {
            return false;
        }
        *value = (int32_t)raw_value;
        return true;
    }

    bool Bit_Stream::Read_Bytes(uint8_t* output, uint32_t output_size) {
        if (output_size == 0) {
            return true;
        }
        if (output == nullptr || read_offset_ + output_size > size_) {
            return false;
        }
        std::memcpy(output, data_ + read_offset_, output_size);
        read_offset_ += output_size;
        return true;
    }

    bool Bit_Stream::Read_String(uint32_t* string_length, uint32_t output_size, char* output) {
        const uint32_t saved_offset = read_offset_;
        uint32_t length = 0;
        if (!Read_Uint32(&length)) {
            return false;
        }
        if (read_offset_ + length > size_) {
            read_offset_ = saved_offset;
            return false;
        }
        if (string_length != nullptr) {
            *string_length = length;
        }
        if (output == nullptr || output_size <= length) {
            read_offset_ = saved_offset;
            return false;
        }
        if (length != 0) {
            std::memcpy(output, data_ + read_offset_, length);
        }
        output[length] = '\0';
        read_offset_ += length;
        return true;
    }

    bool Bit_Stream::Read_ASCIIZ_String(uint32_t* string_length, uint32_t output_size, char* output) {
        const uint32_t saved_offset = read_offset_;
        uint32_t length = 0;
        while (read_offset_ + length < size_ && data_[read_offset_ + length] != 0) {
            ++length;
        }
        if (read_offset_ + length >= size_) {
            return false;
        }
        if (string_length != nullptr) {
            *string_length = length;
        }
        if (output == nullptr || output_size <= length) {
            read_offset_ = saved_offset;
            return false;
        }
        if (length != 0) {
            std::memcpy(output, data_ + read_offset_, length);
        }
        output[length] = '\0';
        read_offset_ += length + 1;
        return true;
    }

    bool Bit_Stream::At_End() const {
        return read_offset_ == size_;
    }

    uint8_t* Bit_Stream::Data() {
        return data_;
    }

    const uint8_t* Bit_Stream::Data() const {
        return data_;
    }

    uint32_t Bit_Stream::Size() const {
        return size_;
    }

    uint32_t Bit_Stream::Tell() const {
        return read_offset_;
    }

    void Bit_Stream::Grow_To(uint32_t size) {
        if (size <= capacity_) {
            return;
        }

        uint32_t new_capacity = capacity_ == 0 ? 64 : capacity_;
        while (new_capacity < size) {
            if (new_capacity > std::numeric_limits<uint32_t>::max() / 2) {
                new_capacity = size;
                break;
            }
            new_capacity *= 2;
        }

        uint8_t* new_data = (uint8_t*)std::realloc(data_, new_capacity);
        if (new_data == nullptr) {
            Crash_Write_Failed();
        }
        data_ = new_data;
        capacity_ = new_capacity;
    }

    bool Is_Known_Message_Kind(uint16_t value) {
        switch ((e_router_message_kind)value) {
            case ROUTER_MESSAGE_HELLO:
            case ROUTER_MESSAGE_WELCOME:
            case ROUTER_MESSAGE_REGISTER_SESSION:
            case ROUTER_MESSAGE_SESSION_REGISTERED:
            case ROUTER_MESSAGE_UNREGISTER_SESSION:
            case ROUTER_MESSAGE_LIST_SESSIONS:
            case ROUTER_MESSAGE_SESSION_LIST:
            case ROUTER_MESSAGE_JOIN_SESSION:
            case ROUTER_MESSAGE_JOINED_SESSION:
            case ROUTER_MESSAGE_LEAVE_SESSION:
            case ROUTER_MESSAGE_LEFT_SESSION:
            case ROUTER_MESSAGE_SEND_TO:
            case ROUTER_MESSAGE_SEND_BROADCAST:
            case ROUTER_MESSAGE_GAME_DATA:
            case ROUTER_MESSAGE_OK:
            case ROUTER_MESSAGE_ERROR:
            case ROUTER_MESSAGE_SET_SESSION_OPEN:
            case ROUTER_MESSAGE_MEMBER_LIST:
            case ROUTER_MESSAGE_MEMBER_JOINED:
            case ROUTER_MESSAGE_MEMBER_LEFT:
            case ROUTER_MESSAGE_SESSION_CLOSED:
            case ROUTER_MESSAGE_REMOVE_MEMBER:
                return true;
            default:
                return false;
        }
    }

    void Encode_Message_Body(const s_router_message* message, Bit_Stream* output) {
        if (message == nullptr || output == nullptr) {
            Crash_Write_Failed();
        }

        output->Clear();
        output->Reserve(ROUTER_MESSAGE_HEADER_SIZE + message->payload.Size());
        output->Write_Uint32(ROUTER_PROTOCOL_MAGIC);
        output->Write_Uint16((uint16_t)ROUTER_PROTOCOL_VERSION);
        output->Write_Uint16((uint16_t)message->header.kind);
        output->Write_Uint32(message->header.flags);
        output->Write_Uint32(message->header.session_id);
        output->Write_Uint32(message->header.sender_network_id);
        output->Write_Uint32(message->header.target_network_id);
        output->Write_Uint32(message->header.request_id);
        output->Write_Uint32(message->payload.Size());
        output->Write_Bytes(message->payload.Data(), message->payload.Size());
    }

    void Encode_Frame(const s_router_message* message, Bit_Stream* output) {
        s_router_frame_codec codec;
        Encode_Frame(message, &codec, output);
    }

    uint32_t Encoded_Frame_Max_Size(const s_router_message* message) {
        if (message == nullptr) {
            return 0;
        }

        uint64_t frame_size = 4U + ROUTER_PACKET_HEADER_SIZE + ROUTER_MESSAGE_HEADER_SIZE;
        if (message->payload.Size() != 0) {
            if (message->payload.Size() > (uint32_t)std::numeric_limits<int>::max()) {
                return std::numeric_limits<uint32_t>::max();
            }

            const int compressed_bound = LZ4_compressBound((int)message->payload.Size());
            if (compressed_bound <= 0) {
                return std::numeric_limits<uint32_t>::max();
            }
            frame_size += (uint32_t)compressed_bound;
        }

        if (frame_size > std::numeric_limits<uint32_t>::max()) {
            return std::numeric_limits<uint32_t>::max();
        }
        return (uint32_t)frame_size;
    }

    void Encode_Frame(const s_router_message* message, s_router_frame_codec* codec, Bit_Stream* output) {
        if (message == nullptr || output == nullptr) {
            Crash_Write_Failed();
        }
        if (codec == nullptr || codec->state == nullptr || codec->state->encode_stream == nullptr) {
            Crash_Write_Failed();
        }

        uint16_t packet_flags = 0;
        Bit_Stream wire_payload;
        if (message->payload.Size() != 0) {
            if (message->payload.Size() > (uint32_t)std::numeric_limits<int>::max()) {
                Crash_Write_Failed();
            }

            const int compressed_bound = LZ4_compressBound((int)message->payload.Size());
            if (compressed_bound <= 0) {
                Crash_Write_Failed();
            }

            wire_payload.Resize((uint32_t)compressed_bound);
            LZ4_loadDict(
                codec->state->encode_stream,
                (const char*)codec->state->encode_dictionary.Data(),
                (int)codec->state->encode_dictionary.Size());
            const int compressed_size = LZ4_compress_fast_continue(
                codec->state->encode_stream,
                (const char*)message->payload.Data(),
                (char*)wire_payload.Data(),
                (int)message->payload.Size(),
                compressed_bound,
                1);
            if (compressed_size <= 0) {
                Crash_Write_Failed();
            }
            wire_payload.Resize((uint32_t)compressed_size);
            Save_Lz4_Dictionary(&codec->state->encode_dictionary, message->payload.Data(), message->payload.Size());
            codec->state->stats.uncompressed_payload_bytes_out += message->payload.Size();
            codec->state->stats.compressed_payload_bytes_out += wire_payload.Size();
            packet_flags |= ROUTER_PACKET_FLAG_PAYLOAD_LZ4_COMPRESSED;
        }

        const uint32_t frame_size = ROUTER_PACKET_HEADER_SIZE + ROUTER_MESSAGE_HEADER_SIZE + wire_payload.Size();
        if (frame_size < wire_payload.Size()) {
            Crash_Write_Failed();
        }

        output->Clear();
        output->Reserve(4 + frame_size);
        output->Write_Uint32(frame_size);
        output->Write_Uint16(packet_flags);
        output->Write_Uint32(ROUTER_PROTOCOL_MAGIC);
        output->Write_Uint16((uint16_t)ROUTER_PROTOCOL_VERSION);
        output->Write_Uint16((uint16_t)message->header.kind);
        output->Write_Uint32(message->header.flags);
        output->Write_Uint32(message->header.session_id);
        output->Write_Uint32(message->header.sender_network_id);
        output->Write_Uint32(message->header.target_network_id);
        output->Write_Uint32(message->header.request_id);
        output->Write_Uint32(message->payload.Size());
        output->Write_Bytes(wire_payload.Data(), wire_payload.Size());
    }

    bool Decode_Message_Body(const uint8_t* data, uint32_t size, s_router_message* message, std::string* error_message) {
        uint32_t payload_size = 0;
        s_router_message decoded;
        if (!Decode_Message_Header(data, size, &decoded, &payload_size, error_message)) {
            return false;
        }

        if (payload_size != size - ROUTER_MESSAGE_HEADER_SIZE) {
            Set_Error(error_message, "payload size does not match frame body size");
            return false;
        }

        decoded.payload.Set_Buffer(data + ROUTER_MESSAGE_HEADER_SIZE, payload_size);
        *message = std::move(decoded);
        return true;
    }

    e_frame_read_status Try_Read_Frame(Bit_Stream* input_buffer, uint32_t max_frame_size, s_router_message* message, std::string* error_message) {
        s_router_frame_codec codec;
        return Try_Read_Frame(input_buffer, max_frame_size, &codec, message, error_message);
    }

    e_frame_read_status Try_Read_Frame(Bit_Stream* input_buffer, uint32_t max_frame_size, s_router_frame_codec* codec, s_router_message* message, std::string* error_message) {
        if (input_buffer == nullptr || codec == nullptr || codec->state == nullptr || message == nullptr) {
            Set_Error(error_message, "frame read argument is null");
            return FRAME_READ_PROTOCOL_ERROR;
        }

        if (input_buffer->Size() < 4) {
            return FRAME_READ_NEED_MORE_DATA;
        }

        uint32_t frame_size = 0;
        Read_Header_Uint32(input_buffer->Data(), 0, &frame_size);
        if (frame_size < ROUTER_PACKET_HEADER_SIZE + ROUTER_MESSAGE_HEADER_SIZE) {
            Set_Error(error_message, "frame size is smaller than the packet header");
            return FRAME_READ_PROTOCOL_ERROR;
        }
        if (frame_size > Max_Wire_Frame_Size(max_frame_size)) {
            Set_Error(error_message, "frame size exceeds configured maximum");
            return FRAME_READ_PROTOCOL_ERROR;
        }
        if (input_buffer->Size() < 4u + frame_size) {
            return FRAME_READ_NEED_MORE_DATA;
        }

        const uint8_t* frame_data = input_buffer->Data() + 4;
        uint16_t packet_flags = 0;
        Read_Header_Uint16(frame_data, 0, &packet_flags);
        if ((packet_flags & ~ROUTER_KNOWN_PACKET_FLAGS) != 0) {
            Set_Error(error_message, "unknown packet flags");
            return FRAME_READ_PROTOCOL_ERROR;
        }

        const uint8_t* body_data = frame_data + ROUTER_PACKET_HEADER_SIZE;
        const uint32_t body_size = frame_size - ROUTER_PACKET_HEADER_SIZE;
        const bool payload_compressed = (packet_flags & ROUTER_PACKET_FLAG_PAYLOAD_LZ4_COMPRESSED) != 0;
        if (payload_compressed) {
            uint32_t payload_size = 0;
            s_router_message decoded;
            if (!Decode_Message_Header(body_data, body_size, &decoded, &payload_size, error_message)) {
                return FRAME_READ_PROTOCOL_ERROR;
            }
            if (payload_size == 0) {
                Set_Error(error_message, "compressed payload has zero uncompressed size");
                return FRAME_READ_PROTOCOL_ERROR;
            }
            if (max_frame_size < ROUTER_MESSAGE_HEADER_SIZE || payload_size > max_frame_size - ROUTER_MESSAGE_HEADER_SIZE) {
                Set_Error(error_message, "uncompressed payload size exceeds configured maximum");
                return FRAME_READ_PROTOCOL_ERROR;
            }
            const uint32_t compressed_payload_size = body_size - ROUTER_MESSAGE_HEADER_SIZE;
            if (payload_size > (uint32_t)std::numeric_limits<int>::max() || compressed_payload_size > (uint32_t)std::numeric_limits<int>::max()) {
                Set_Error(error_message, "compressed payload is too large for lz4");
                return FRAME_READ_PROTOCOL_ERROR;
            }

            Bit_Stream payload;
            payload.Resize(payload_size);
            LZ4_setStreamDecode(
                codec->state->decode_stream,
                (const char*)codec->state->decode_dictionary.Data(),
                (int)codec->state->decode_dictionary.Size());
            const int decoded_size = LZ4_decompress_safe_continue(
                codec->state->decode_stream,
                (const char*)body_data + ROUTER_MESSAGE_HEADER_SIZE,
                (char*)payload.Data(),
                (int)compressed_payload_size,
                (int)payload_size);
            if (decoded_size != (int)payload_size) {
                Set_Error(error_message, "lz4 payload decompression failed");
                return FRAME_READ_PROTOCOL_ERROR;
            }
            Save_Lz4_Dictionary(&codec->state->decode_dictionary, payload.Data(), payload.Size());
            codec->state->stats.uncompressed_payload_bytes_in += payload.Size();
            codec->state->stats.compressed_payload_bytes_in += compressed_payload_size;
            codec->state->stats.last_uncompressed_payload_bytes_in = payload.Size();
            codec->state->stats.last_compressed_payload_bytes_in = compressed_payload_size;

            decoded.payload = std::move(payload);
            *message = std::move(decoded);
        } else {
            if (body_size > max_frame_size) {
                Set_Error(error_message, "frame size exceeds configured maximum");
                return FRAME_READ_PROTOCOL_ERROR;
            }
            if (!Decode_Message_Body(body_data, body_size, message, error_message)) {
                return FRAME_READ_PROTOCOL_ERROR;
            }
            Save_Lz4_Dictionary(&codec->state->decode_dictionary, message->payload.Data(), message->payload.Size());
            codec->state->stats.uncompressed_payload_bytes_in += message->payload.Size();
            codec->state->stats.compressed_payload_bytes_in += message->payload.Size();
            codec->state->stats.last_uncompressed_payload_bytes_in = message->payload.Size();
            codec->state->stats.last_compressed_payload_bytes_in = message->payload.Size();
        }

        input_buffer->Consume(4 + frame_size);
        return FRAME_READ_MESSAGE_READY;
    }

    void Encode_Text_Payload(const char* text, Bit_Stream* payload) {
        if (payload == nullptr) {
            Crash_Write_Failed();
        }
        payload->Clear();
        payload->Write_String(text);
    }

    bool Decode_Text_Payload(const Bit_Stream* payload, uint32_t output_size, char* output, std::string* error_message) {
        if (payload == nullptr || output == nullptr) {
            Set_Error(error_message, "text output is null");
            return false;
        }
        Bit_Stream reader = *payload;
        if (!reader.Read_String(nullptr, output_size, output)) {
            Set_Error(error_message, "invalid text payload");
            return false;
        }
        if (!reader.At_End()) {
            Set_Error(error_message, "payload has trailing bytes");
            return false;
        }
        return true;
    }

    void Encode_Hello_Payload(const char* client_name, uint32_t protocol_version, Bit_Stream* payload) {
        if (payload == nullptr) {
            Crash_Write_Failed();
        }
        payload->Clear();
        payload->Write_String(client_name);
        payload->Write_Uint32(protocol_version);
    }

    bool Decode_Hello_Payload(const Bit_Stream* payload, s_router_hello_payload* hello, std::string* error_message) {
        if (payload == nullptr || hello == nullptr) {
            Set_Error(error_message, "hello output is null");
            return false;
        }
        Bit_Stream reader = *payload;
        if (!reader.Read_String(nullptr, sizeof(hello->client_name), hello->client_name) ||
            !reader.Read_Uint32(&hello->protocol_version)) {
            Set_Error(error_message, "invalid hello payload");
            return false;
        }
        if (!reader.At_End()) {
            Set_Error(error_message, "payload has trailing bytes");
            return false;
        }
        return true;
    }

    void Encode_Join_Session_Payload(const s_router_join_session_payload* join, Bit_Stream* payload) {
        if (join == nullptr || payload == nullptr) {
            Crash_Write_Failed();
        }
        payload->Clear();
        payload->Write_String(join->build_string);
    }

    bool Decode_Join_Session_Payload(const Bit_Stream* payload, s_router_join_session_payload* join, std::string* error_message) {
        if (payload == nullptr || join == nullptr) {
            Set_Error(error_message, "join session output is null");
            return false;
        }
        Bit_Stream reader = *payload;
        if (!reader.Read_String(nullptr, sizeof(join->build_string), join->build_string)) {
            Set_Error(error_message, "invalid join session payload");
            return false;
        }
        if (!reader.At_End()) {
            Set_Error(error_message, "payload has trailing bytes");
            return false;
        }
        return true;
    }

    void Encode_Welcome_Payload(uint32_t network_id, const char* router_id, Bit_Stream* payload) {
        if (payload == nullptr) {
            Crash_Write_Failed();
        }
        payload->Clear();
        payload->Write_Uint32(network_id);
        payload->Write_String(router_id);
    }

    bool Decode_Welcome_Payload(const Bit_Stream* payload, s_router_welcome_payload* welcome, std::string* error_message) {
        if (payload == nullptr || welcome == nullptr) {
            Set_Error(error_message, "welcome output is null");
            return false;
        }
        Bit_Stream reader = *payload;
        if (!reader.Read_Uint32(&welcome->network_id) ||
            !reader.Read_String(nullptr, sizeof(welcome->router_id), welcome->router_id)) {
            Set_Error(error_message, "invalid welcome payload");
            return false;
        }
        if (!reader.At_End()) {
            Set_Error(error_message, "payload has trailing bytes");
            return false;
        }
        return true;
    }

    void Encode_Session_Registration_Payload(const s_router_session_registration_payload* registration, Bit_Stream* payload) {
        if (registration == nullptr || payload == nullptr) {
            Crash_Write_Failed();
        }
        payload->Clear();
        payload->Write_String(registration->session_name);
        payload->Write_String(registration->host_name);
        payload->Write_String(registration->build_string);
        payload->Write_Uint16(registration->max_players);
    }

    bool Decode_Session_Registration_Payload(const Bit_Stream* payload, s_router_session_registration_payload* registration, std::string* error_message) {
        if (payload == nullptr || registration == nullptr) {
            Set_Error(error_message, "session registration output is null");
            return false;
        }
        Bit_Stream reader = *payload;
        if (!reader.Read_String(nullptr, sizeof(registration->session_name), registration->session_name) ||
            !reader.Read_String(nullptr, sizeof(registration->host_name), registration->host_name) ||
            !reader.Read_String(nullptr, sizeof(registration->build_string), registration->build_string) ||
            !reader.Read_Uint16(&registration->max_players)) {
            Set_Error(error_message, "invalid session registration payload");
            return false;
        }
        if (!reader.At_End()) {
            Set_Error(error_message, "payload has trailing bytes");
            return false;
        }
        return true;
    }

    void Encode_Session_Info_Payload(const s_router_session_info* session_info, Bit_Stream* payload) {
        if (session_info == nullptr || payload == nullptr) {
            Crash_Write_Failed();
        }
        payload->Clear();
        Write_Session_Info(payload, session_info);
    }

    bool Decode_Session_Info_Payload(const Bit_Stream* payload, s_router_session_info* session_info, std::string* error_message) {
        if (payload == nullptr || session_info == nullptr) {
            Set_Error(error_message, "session info output is null");
            return false;
        }
        Bit_Stream reader = *payload;
        if (!Read_Session_Info(&reader, session_info)) {
            Set_Error(error_message, "invalid session info payload");
            return false;
        }
        if (!reader.At_End()) {
            Set_Error(error_message, "payload has trailing bytes");
            return false;
        }
        return true;
    }

    void Encode_Session_List_Payload(const std::vector<s_router_session_info>* sessions, Bit_Stream* payload) {
        if (sessions == nullptr || payload == nullptr || sessions->size() > std::numeric_limits<uint32_t>::max()) {
            Crash_Write_Failed();
        }

        payload->Clear();
        payload->Write_Uint32((uint32_t)sessions->size());
        for (const s_router_session_info& session_info : *sessions) {
            Write_Session_Info(payload, &session_info);
        }
    }

    bool Decode_Session_List_Payload(const Bit_Stream* payload, std::vector<s_router_session_info>* sessions, std::string* error_message) {
        if (payload == nullptr || sessions == nullptr) {
            Set_Error(error_message, "session list output is null");
            return false;
        }

        Bit_Stream reader = *payload;
        uint32_t count = 0;
        if (!reader.Read_Uint32(&count)) {
            Set_Error(error_message, "invalid session list payload");
            return false;
        }

        std::vector<s_router_session_info> decoded;
        decoded.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            s_router_session_info session_info;
            if (!Read_Session_Info(&reader, &session_info)) {
                Set_Error(error_message, "invalid session list payload");
                return false;
            }
            decoded.push_back(std::move(session_info));
        }
        if (!reader.At_End()) {
            Set_Error(error_message, "payload has trailing bytes");
            return false;
        }

        *sessions = std::move(decoded);
        return true;
    }

    void Encode_Member_Info_Payload(const s_router_member_info* member, Bit_Stream* payload) {
        if (member == nullptr || payload == nullptr) {
            Crash_Write_Failed();
        }
        payload->Clear();
        Write_Member_Info(payload, member);
    }

    bool Decode_Member_Info_Payload(const Bit_Stream* payload, s_router_member_info* member, std::string* error_message) {
        if (payload == nullptr || member == nullptr) {
            Set_Error(error_message, "member info output is null");
            return false;
        }

        Bit_Stream reader = *payload;
        if (!Read_Member_Info(&reader, member)) {
            Set_Error(error_message, "invalid member info payload");
            return false;
        }
        if (!reader.At_End()) {
            Set_Error(error_message, "payload has trailing bytes");
            return false;
        }
        return true;
    }

    void Encode_Member_List_Payload(const std::vector<s_router_member_info>* members, Bit_Stream* payload) {
        if (members == nullptr || payload == nullptr || members->size() > std::numeric_limits<uint32_t>::max()) {
            Crash_Write_Failed();
        }

        payload->Clear();
        payload->Write_Uint32((uint32_t)members->size());
        for (const s_router_member_info& member : *members) {
            Write_Member_Info(payload, &member);
        }
    }

    bool Decode_Member_List_Payload(const Bit_Stream* payload, std::vector<s_router_member_info>* members, std::string* error_message) {
        if (payload == nullptr || members == nullptr) {
            Set_Error(error_message, "member list output is null");
            return false;
        }

        Bit_Stream reader = *payload;
        uint32_t count = 0;
        if (!reader.Read_Uint32(&count)) {
            Set_Error(error_message, "invalid member list payload");
            return false;
        }

        std::vector<s_router_member_info> decoded;
        decoded.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            s_router_member_info member;
            if (!Read_Member_Info(&reader, &member)) {
                Set_Error(error_message, "invalid member list payload");
                return false;
            }
            decoded.push_back(member);
        }
        if (!reader.At_End()) {
            Set_Error(error_message, "payload has trailing bytes");
            return false;
        }

        *members = std::move(decoded);
        return true;
    }

    void Encode_Member_Event_Payload(uint32_t event, const s_router_member_info* member, Bit_Stream* payload) {
        if (member == nullptr || payload == nullptr) {
            Crash_Write_Failed();
        }
        payload->Clear();
        payload->Write_Uint32(event);
        Write_Member_Info(payload, member);
    }

    bool Decode_Member_Event_Payload(const Bit_Stream* payload, s_router_member_event_payload* event, std::string* error_message) {
        if (payload == nullptr || event == nullptr) {
            Set_Error(error_message, "member event output is null");
            return false;
        }

        Bit_Stream reader = *payload;
        if (!reader.Read_Uint32(&event->event) || !Read_Member_Info(&reader, &event->member)) {
            Set_Error(error_message, "invalid member event payload");
            return false;
        }
        if (!reader.At_End()) {
            Set_Error(error_message, "payload has trailing bytes");
            return false;
        }
        return true;
    }

    void Encode_Error_Payload(e_router_error_code code, const char* message, Bit_Stream* payload) {
        if (payload == nullptr) {
            Crash_Write_Failed();
        }
        payload->Clear();
        payload->Write_Uint32((uint32_t)code);
        payload->Write_String(message);
    }

    bool Decode_Error_Payload(const Bit_Stream* payload, s_router_error_payload* error, std::string* error_message) {
        if (payload == nullptr || error == nullptr) {
            Set_Error(error_message, "error output is null");
            return false;
        }

        Bit_Stream reader = *payload;
        uint32_t code = 0;
        if (!reader.Read_Uint32(&code) ||
            !reader.Read_String(nullptr, sizeof(error->message), error->message)) {
            Set_Error(error_message, "invalid error payload");
            return false;
        }
        if (!reader.At_End()) {
            Set_Error(error_message, "payload has trailing bytes");
            return false;
        }
        error->code = (e_router_error_code)code;
        return true;
    }

    void Encode_Discovery_Probe(Bit_Stream* payload) {
        if (payload == nullptr) {
            Crash_Write_Failed();
        }
        payload->Clear();
        payload->Write_Uint32(ROUTER_DISCOVERY_MAGIC);
        payload->Write_Uint16((uint16_t)ROUTER_DISCOVERY_VERSION);
        payload->Write_Uint16(ROUTER_DISCOVERY_PROBE);
    }

    void Encode_Discovery_Reply(uint16_t tcp_port, const char* router_id, const char* router_name, Bit_Stream* payload) {
        if (payload == nullptr) {
            Crash_Write_Failed();
        }
        payload->Clear();
        payload->Write_Uint32(ROUTER_DISCOVERY_MAGIC);
        payload->Write_Uint16((uint16_t)ROUTER_DISCOVERY_VERSION);
        payload->Write_Uint16(ROUTER_DISCOVERY_REPLY);
        payload->Write_Uint16(tcp_port);
        payload->Write_Uint16((uint16_t)ROUTER_PROTOCOL_VERSION);
        payload->Write_String(router_id);
        payload->Write_String(router_name);
    }

    bool Is_Discovery_Probe(const uint8_t* data, uint32_t size) {
        Bit_Stream reader(data, size);
        uint32_t magic = 0;
        uint16_t version = 0;
        uint16_t kind = 0;
        return reader.Read_Uint32(&magic) &&
            reader.Read_Uint16(&version) &&
            reader.Read_Uint16(&kind) &&
            reader.At_End() &&
            magic == ROUTER_DISCOVERY_MAGIC &&
            version == ROUTER_DISCOVERY_VERSION &&
            kind == ROUTER_DISCOVERY_PROBE;
    }

    bool Decode_Discovery_Reply(const Bit_Stream* payload, s_router_discovery_reply* reply, std::string* error_message) {
        if (payload == nullptr || reply == nullptr) {
            Set_Error(error_message, "discovery reply output is null");
            return false;
        }

        Bit_Stream reader = *payload;
        uint32_t magic = 0;
        uint16_t version = 0;
        uint16_t kind = 0;
        if (!reader.Read_Uint32(&magic) ||
            !reader.Read_Uint16(&version) ||
            !reader.Read_Uint16(&kind) ||
            !reader.Read_Uint16(&reply->tcp_port) ||
            !reader.Read_Uint16(&reply->protocol_version) ||
            !reader.Read_String(nullptr, sizeof(reply->router_id), reply->router_id) ||
            !reader.Read_String(nullptr, sizeof(reply->router_name), reply->router_name) ||
            !reader.At_End()) {
            Set_Error(error_message, "invalid discovery reply payload");
            return false;
        }

        if (magic != ROUTER_DISCOVERY_MAGIC || version != ROUTER_DISCOVERY_VERSION || kind != ROUTER_DISCOVERY_REPLY) {
            Set_Error(error_message, "invalid discovery reply header");
            return false;
        }

        return true;
    }

} // namespace network::router