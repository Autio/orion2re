#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace config::tool {

enum e_json_kind {
    JSON_NULL,
    JSON_BOOL,
    JSON_SIGNED,
    JSON_UNSIGNED,
    JSON_REAL,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT,
};

struct s_json_value {
    e_json_kind kind = JSON_NULL;
    bool bool_value = false;
    int64_t signed_value = 0;
    uint64_t unsigned_value = 0;
    double real_value = 0.0;
    std::string string_value;
    std::vector<std::string> member_names;
    std::vector<s_json_value> values;
};

bool Read_Json5_File(const char* path, s_json_value* value, std::string* diagnostic);

}