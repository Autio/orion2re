#pragma once

#include <cstdint>

namespace STRBUILD {
    enum e_named_argument_type : uint8_t {
        NAMED_ARGUMENT_STRING,
        NAMED_ARGUMENT_SIGNED,
        NAMED_ARGUMENT_UNSIGNED,
    };

    struct s_named_argument {
        const char* name;
        e_named_argument_type type;
        const char* string_value;
        int64_t signed_value;
        uint64_t unsigned_value;
    };

    struct s_builder {
        char* start;
        char* cur;
        char* end;
    };

    s_builder Init(char* buffer, uint32_t buffer_size);

    void Copy(s_builder* builder, const char* source);

    void Cat(s_builder* builder, const char* source);

    void Sprintf(s_builder* builder, const char* format, ...);

    s_named_argument Named_Argument(const char* name, const char* value);
    s_named_argument Named_Argument(const char* name, int16_t value);
    s_named_argument Named_Argument(const char* name, uint16_t value);
    s_named_argument Named_Argument(const char* name, int32_t value);
    s_named_argument Named_Argument(const char* name, uint32_t value);
    s_named_argument Named_Argument(const char* name, int64_t value);
    s_named_argument Named_Argument(const char* name, uint64_t value);

    void Named_Snprintf(
        char* output,
        uint32_t output_size,
        const char* format,
        const s_named_argument* arguments,
        uint32_t argument_count);
} // namespace STRBUILD