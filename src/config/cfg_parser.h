#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace config {

    struct s_source_location {
        std::string source;
        size_t offset = 0;
        uint32_t line = 1;
        uint32_t column = 1;
    };

    enum e_cfg_token_kind {
        CFG_TOKEN_IDENTIFIER,
        CFG_TOKEN_INTEGER,
        CFG_TOKEN_RATIO,
        CFG_TOKEN_MASK,
        CFG_TOKEN_STRING,
        CFG_TOKEN_PATH,
        CFG_TOKEN_SKIP,
    };

    struct s_cfg_token {
        e_cfg_token_kind kind = CFG_TOKEN_IDENTIFIER;
        s_source_location location;
        std::string raw;
        std::string value;
    };

    struct s_cfg_assignment_group {
        s_cfg_token selector;
        std::vector<s_cfg_token> values;
    };

    enum e_cfg_statement_kind {
        CFG_STATEMENT_ASSIGNMENT,
        CFG_STATEMENT_INCLUDE,
        CFG_STATEMENT_ENABLE,
        CFG_STATEMENT_STOP,
    };

    struct s_cfg_statement {
        e_cfg_statement_kind kind = CFG_STATEMENT_ASSIGNMENT;
        s_source_location location;
        uint32_t statement_number = 0;
        std::vector<s_cfg_token> path;
        std::vector<s_cfg_token> values;
        std::vector<s_cfg_assignment_group> groups;
        s_cfg_token argument;
    };

    struct s_cfg_document {
        std::vector<s_cfg_statement> statements;
        // Nonzero only for loader-owned immutable source revisions.
        uint64_t source_revision = 0;
    };

    struct s_cfg_diagnostic {
        s_source_location location;
        std::string message;
    };

    bool Parse_Cfg(const char* source_name, const char* source, size_t source_size, s_cfg_document* document, s_cfg_diagnostic* diagnostic);

} // namespace config