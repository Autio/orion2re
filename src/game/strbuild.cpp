#include "pch.h"

#include "strbuild.h"

namespace STRBUILD {
    static uint32_t Remaining(const s_builder* builder) {
        if (builder == nullptr || builder->cur == nullptr || builder->end == nullptr || builder->cur >= builder->end) {
            return 0;
        }

        return (uint32_t)(builder->end - builder->cur);
    }

    static void Cat_N(s_builder* builder, const char* source, uint32_t source_len) {
        if (builder == nullptr || source == nullptr) {
            return;
        }

        uint32_t remaining = Remaining(builder);
        if (remaining == 0) {
            return;
        }

        uint32_t copy_len = source_len;
        if (copy_len >= remaining) {
            copy_len = remaining - 1;
        }

        memcpy(builder->cur, source, copy_len);
        builder->cur += copy_len;
        builder->cur[0] = '\0';
    }

    static const s_named_argument* Find_Argument(
        const char* name,
        uint32_t name_len,
        const s_named_argument* arguments,
        uint32_t argument_count) {
        if (arguments == nullptr) {
            return nullptr;
        }

        for (uint32_t i = 0; i < argument_count; ++i) {
            const char* argument_name = arguments[i].name;
            if (argument_name != nullptr &&
                strlen(argument_name) == name_len &&
                memcmp(argument_name, name, name_len) == 0) {
                return &arguments[i];
            }
        }

        return nullptr;
    }

    s_builder Init(char* buffer, uint32_t buffer_size) {
        s_builder builder = {buffer, buffer, buffer};

        if (buffer != nullptr && buffer_size != 0) {
            builder.end = buffer + buffer_size;
            buffer[0] = '\0';
        }

        return builder;
    }

    void Copy(s_builder* builder, const char* source) {
        if (builder == nullptr) {
            return;
        }

        builder->cur = builder->start;
        if (builder->cur != nullptr && builder->cur < builder->end) {
            builder->cur[0] = '\0';
        }

        Cat(builder, source);
    }

    void Cat(s_builder* builder, const char* source) {
        if (builder == nullptr || source == nullptr) {
            return;
        }

        uint32_t remaining = Remaining(builder);
        if (remaining == 0) {
            return;
        }

        uint32_t source_len = (uint32_t)strlen(source);
        if (source_len >= remaining) {
            memcpy(builder->cur, source, remaining - 1);
            builder->cur += remaining - 1;
            builder->cur[0] = '\0';
            return;
        }

        memcpy(builder->cur, source, source_len + 1);
        builder->cur += source_len;
    }

    void Sprintf(s_builder* builder, const char* format, ...) {
        if (builder == nullptr || format == nullptr) {
            return;
        }

        uint32_t remaining = Remaining(builder);
        if (remaining == 0) {
            return;
        }

        va_list args;
        va_start(args, format);
        int written = vsnprintf(builder->cur, remaining, format, args);
        va_end(args);

        if (written < 0) {
            if (builder->end != nullptr && builder->end > builder->start) {
                builder->cur = builder->end - 1;
            }
            return;
        }

        if ((uint32_t)written >= remaining) {
            builder->cur = builder->end - 1;
            return;
        }

        builder->cur += written;
    }

    s_named_argument Named_Argument(const char* name, const char* value) {
        return {name, NAMED_ARGUMENT_STRING, value, 0, 0};
    }

    s_named_argument Named_Argument(const char* name, int16_t value) {
        return Named_Argument(name, (int64_t)value);
    }

    s_named_argument Named_Argument(const char* name, uint16_t value) {
        return Named_Argument(name, (uint64_t)value);
    }

    s_named_argument Named_Argument(const char* name, int32_t value) {
        return Named_Argument(name, (int64_t)value);
    }

    s_named_argument Named_Argument(const char* name, uint32_t value) {
        return Named_Argument(name, (uint64_t)value);
    }

    s_named_argument Named_Argument(const char* name, int64_t value) {
        return {name, NAMED_ARGUMENT_SIGNED, nullptr, value, 0};
    }

    s_named_argument Named_Argument(const char* name, uint64_t value) {
        return {name, NAMED_ARGUMENT_UNSIGNED, nullptr, 0, value};
    }

    void Named_Snprintf(
        char* output,
        uint32_t output_size,
        const char* format,
        const s_named_argument* arguments,
        uint32_t argument_count) {
        s_builder builder = Init(output, output_size);
        if (format == nullptr || output == nullptr || output_size == 0) {
            return;
        }

        const char* cursor = format;
        while (*cursor != '\0') {
            const char* marker = strchr(cursor, '$');
            if (marker == nullptr) {
                Cat(&builder, cursor);
                break;
            }

            Cat_N(&builder, cursor, (uint32_t)(marker - cursor));
            if (marker[1] == '$') {
                Cat_N(&builder, "$", 1);
                cursor = marker + 2;
                continue;
            }

            const char* end = strchr(marker + 1, '$');
            if (end == nullptr) {
                Cat(&builder, marker);
                break;
            }

            const uint32_t name_len = (uint32_t)(end - marker - 1);
            const s_named_argument* argument = Find_Argument(
                marker + 1, name_len, arguments, argument_count);
            if (argument == nullptr) {
                Cat_N(&builder, marker, (uint32_t)(end - marker + 1));
            } else if (argument->type == NAMED_ARGUMENT_STRING) {
                Cat(&builder, argument->string_value != nullptr ? argument->string_value : "");
            } else if (argument->type == NAMED_ARGUMENT_SIGNED) {
                Sprintf(&builder, "%lld", (long long)argument->signed_value);
            } else {
                Sprintf(&builder, "%llu", (unsigned long long)argument->unsigned_value);
            }

            cursor = end + 1;
        }
    }
} // namespace STRBUILD