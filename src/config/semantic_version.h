#pragma once

#include <charconv>
#include <cstdint>
#include <string>

namespace config {

    constexpr int64_t SEMANTIC_VERSION_COMPONENT_MISSING = -1;

    struct s_semantic_version {
        int64_t major = SEMANTIC_VERSION_COMPONENT_MISSING;
        int64_t minor = SEMANTIC_VERSION_COMPONENT_MISSING;
        int64_t patch = SEMANTIC_VERSION_COMPONENT_MISSING;
    };

    inline int64_t Semantic_Version_Component_Value(int64_t component) {
        return component == SEMANTIC_VERSION_COMPONENT_MISSING
            ? INT64_C(0)
            : component;
    }

    inline bool operator==(const s_semantic_version& left,
        const s_semantic_version& right) {
        return Semantic_Version_Component_Value(left.major) ==
                Semantic_Version_Component_Value(right.major) &&
            Semantic_Version_Component_Value(left.minor) ==
                Semantic_Version_Component_Value(right.minor) &&
            Semantic_Version_Component_Value(left.patch) ==
                Semantic_Version_Component_Value(right.patch);
    }

    inline bool Parse_Semantic_Version(const std::string& text,
        s_semantic_version* version) {
        if (version == nullptr || text.empty()) {
            return false;
        }
        uint32_t parts[3] = {};
        size_t start = 0;
        for (size_t part = 0; part < 3; ++part) {
            size_t end = text.find('.', start);
            if (end == std::string::npos) {
                end = text.size();
            }
            if (end == start || (end - start > 1 && text[start] == '0')) {
                return false;
            }
            const char* first = text.data() + start;
            const char* last = text.data() + end;
            const auto result = std::from_chars(first, last, parts[part]);
            if (result.ec != std::errc{} || result.ptr != last) {
                return false;
            }
            if (end == text.size()) {
                version->major = parts[0];
                version->minor = part >= 1
                    ? parts[1]
                    : SEMANTIC_VERSION_COMPONENT_MISSING;
                version->patch = part >= 2
                    ? parts[2]
                    : SEMANTIC_VERSION_COMPONENT_MISSING;
                return true;
            }
            start = end + 1;
            if (start == text.size()) {
                return false;
            }
        }
        return false;
    }

    inline bool Semantic_Versions_Equal(const std::string& left,
        const std::string& right) {
        s_semantic_version parsed_left;
        s_semantic_version parsed_right;
        return Parse_Semantic_Version(left, &parsed_left) &&
            Parse_Semantic_Version(right, &parsed_right) &&
            parsed_left == parsed_right;
    }

} // namespace config