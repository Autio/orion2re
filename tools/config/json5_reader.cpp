#include "json5_reader.h"

#include <cmath>
#include <utility>

#include "file_util.h"
#include "yyjson.h"

namespace config::tool {
namespace {
    constexpr size_t CONFIG_JSON_MAX_DEPTH = 64;

    bool Convert_Value_(yyjson_val* input, size_t depth, const char* path,
                        s_json_value* output, std::string* diagnostic) {
        if (depth > CONFIG_JSON_MAX_DEPTH) {
            *diagnostic = std::string(path) + ": JSON nesting exceeds 64 levels";
            return false;
        }

        if (yyjson_is_null(input)) {
            output->kind = JSON_NULL;
        } else if (yyjson_is_bool(input)) {
            output->kind = JSON_BOOL;
            output->bool_value = yyjson_get_bool(input);
        } else if (yyjson_is_sint(input)) {
            output->kind = JSON_SIGNED;
            output->signed_value = yyjson_get_sint(input);
        } else if (yyjson_is_uint(input)) {
            output->kind = JSON_UNSIGNED;
            output->unsigned_value = yyjson_get_uint(input);
        } else if (yyjson_is_real(input)) {
            output->kind = JSON_REAL;
            output->real_value = yyjson_get_real(input);
            if (!std::isfinite(output->real_value)) {
                *diagnostic = std::string(path) + ": NaN and Infinity are not supported";
                return false;
            }
        } else if (yyjson_is_str(input)) {
            output->kind = JSON_STRING;
            output->string_value.assign(yyjson_get_str(input), yyjson_get_len(input));
        } else if (yyjson_is_arr(input)) {
            output->kind = JSON_ARRAY;
            size_t index = 0;
            size_t count = 0;
            yyjson_val* item = nullptr;
            yyjson_arr_foreach(input, index, count, item) {
                output->values.emplace_back();
                if (!Convert_Value_(item, depth + 1, path, &output->values.back(), diagnostic)) {
                    return false;
                }
            }
        } else if (yyjson_is_obj(input)) {
            output->kind = JSON_OBJECT;
            size_t index = 0;
            size_t count = 0;
            yyjson_val* key = nullptr;
            yyjson_val* item = nullptr;
            yyjson_obj_foreach(input, index, count, key, item) {
                const std::string name(yyjson_get_str(key), yyjson_get_len(key));
                for (const std::string& existing : output->member_names) {
                    if (existing == name) {
                        *diagnostic = std::string(path) + ": duplicate object key '" + name + "'";
                        return false;
                    }
                }
                output->member_names.push_back(name);
                output->values.emplace_back();
                if (!Convert_Value_(item, depth + 1, path, &output->values.back(), diagnostic)) {
                    return false;
                }
            }
        } else {
            *diagnostic = std::string(path) + ": unsupported JSON value";
            return false;
        }

        return true;
    }
}

bool Read_Json5_File(const char* path, s_json_value* value, std::string* diagnostic) {
    if (path == nullptr || value == nullptr || diagnostic == nullptr) {
        return false;
    }

    std::string source;
    if (!Read_File(path, &source)) {
        *diagnostic = std::string(path) + ": unable to read file";
        return false;
    }

    const yyjson_read_flag flags = YYJSON_READ_ALLOW_TRAILING_COMMAS |
                                   YYJSON_READ_ALLOW_COMMENTS |
                                   YYJSON_READ_ALLOW_EXT_NUMBER |
                                   YYJSON_READ_ALLOW_SINGLE_QUOTED_STR |
                                   YYJSON_READ_ALLOW_UNQUOTED_KEY |
                                   YYJSON_READ_ALLOW_INF_AND_NAN;
    yyjson_read_err error{};
    yyjson_doc* document = yyjson_read_opts(source.data(), source.size(), flags, nullptr, &error);
    if (document == nullptr) {
        size_t line = 0;
        size_t column = 0;
        size_t character = 0;
        yyjson_locate_pos(source.data(), source.size(), error.pos, &line, &column, &character);
        *diagnostic = std::string(path) + ":" + std::to_string(line) + ":" +
                      std::to_string(column) + ": " + error.msg;
        return false;
    }

    s_json_value converted;
    const bool result = Convert_Value_(yyjson_doc_get_root(document), 0, path,
                                       &converted, diagnostic);
    yyjson_doc_free(document);
    if (result) {
        *value = std::move(converted);
    }
    return result;
}

}