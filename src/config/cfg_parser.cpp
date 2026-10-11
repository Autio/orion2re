#include "cfg_parser.h"

#include <cctype>
#include <cstring>
#include <utility>

namespace config {
    namespace {
        struct s_parser {
            const char* source_name = nullptr;
            const char* source = nullptr;
            size_t source_size = 0;
            size_t offset = 0;
            uint32_t line = 1;
            uint32_t column = 1;
            s_cfg_diagnostic* diagnostic = nullptr;
        };

        s_source_location Location(const s_parser* parser) {
            s_source_location location;
            location.source = parser->source_name;
            location.offset = parser->offset;
            location.line = parser->line;
            location.column = parser->column;
            return location;
        }

        bool Fail(s_parser* parser, const s_source_location& location, const std::string& message) {
            parser->diagnostic->location = location;
            parser->diagnostic->message = message;
            return false;
        }

        bool At_End(const s_parser* parser) {
            return parser->offset >= parser->source_size;
        }

        char Current(const s_parser* parser) {
            return At_End(parser) ? '\0' : parser->source[parser->offset];
        }

        void Advance(s_parser* parser) {
            if (At_End(parser)) {
                return;
            }
            const uint8_t byte = static_cast<uint8_t>(parser->source[parser->offset]);
            if (byte == '\r') {
                ++parser->offset;
                if (!At_End(parser) && parser->source[parser->offset] == '\n') {
                    ++parser->offset;
                }
                ++parser->line;
                parser->column = 1;
            } else if (byte == '\n') {
                ++parser->offset;
                ++parser->line;
                parser->column = 1;
            } else {
                ++parser->offset;
                if ((byte & 0xC0) != 0x80) {
                    ++parser->column;
                }
            }
        }

        bool Decode_Utf8(const char* source, size_t size, size_t* offset, uint32_t* codepoint) {
            const uint8_t first = static_cast<uint8_t>(source[*offset]);
            if (first < 0x80) {
                *codepoint = first;
                ++*offset;
                return true;
            }

            size_t count = 0;
            uint32_t value = 0;
            uint32_t minimum = 0;
            if ((first & 0xE0) == 0xC0) {
                count = 2;
                value = first & 0x1F;
                minimum = 0x80;
            } else if ((first & 0xF0) == 0xE0) {
                count = 3;
                value = first & 0x0F;
                minimum = 0x800;
            } else if ((first & 0xF8) == 0xF0) {
                count = 4;
                value = first & 0x07;
                minimum = 0x10000;
            } else {
                return false;
            }
            if (*offset + count > size) {
                return false;
            }
            for (size_t index = 1; index < count; ++index) {
                const uint8_t next = static_cast<uint8_t>(source[*offset + index]);
                if ((next & 0xC0) != 0x80) {
                    return false;
                }
                value = value << 6 | (next & 0x3F);
            }
            if (value < minimum || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)) {
                return false;
            }
            *offset += count;
            *codepoint = value;
            return true;
        }

        bool Validate_Utf8(s_parser* parser) {
            size_t offset = 0;
            uint32_t line = 1;
            uint32_t column = 1;
            while (offset < parser->source_size) {
                const uint8_t byte = static_cast<uint8_t>(parser->source[offset]);
                if (byte == '\r') {
                    ++offset;
                    if (offset < parser->source_size && parser->source[offset] == '\n') {
                        ++offset;
                    }
                    ++line;
                    column = 1;
                    continue;
                }
                if (byte == '\n') {
                    ++offset;
                    ++line;
                    column = 1;
                    continue;
                }
                const size_t invalid_offset = offset;
                uint32_t codepoint = 0;
                if (!Decode_Utf8(parser->source, parser->source_size, &offset, &codepoint)) {
                    s_source_location location;
                    location.source = parser->source_name;
                    location.offset = invalid_offset;
                    location.line = line;
                    location.column = column;
                    return Fail(parser, location, "source is not valid UTF-8");
                }
                ++column;
            }
            return true;
        }

        void Skip_Whitespace_And_Comments(s_parser* parser) {
            while (!At_End(parser)) {
                if (std::isspace(static_cast<uint8_t>(Current(parser))) != 0) {
                    Advance(parser);
                } else if (Current(parser) == '#') {
                    while (!At_End(parser) && Current(parser) != '\r' && Current(parser) != '\n') {
                        Advance(parser);
                    }
                } else {
                    break;
                }
            }
        }

        bool Is_Identifier_Start(char character) {
            return (character >= 'a' && character <= 'z') ||
                (character >= 'A' && character <= 'Z') || character == '_';
        }

        bool Is_Identifier_Continue(char character) {
            return Is_Identifier_Start(character) ||
                (character >= '0' && character <= '9') || character == '-' || character == '.';
        }

        bool Is_Identifier(const std::string& value) {
            if (value.empty() || !Is_Identifier_Start(value.front())) {
                return false;
            }
            for (char character : value) {
                if (!Is_Identifier_Continue(character)) {
                    return false;
                }
            }
            return true;
        }

        bool Is_Decimal(const std::string& value, bool allow_negative) {
            size_t offset = 0;
            if (allow_negative && !value.empty() && value[0] == '-') {
                offset = 1;
            }
            if (offset == value.size()) {
                return false;
            }
            for (; offset < value.size(); ++offset) {
                if (value[offset] < '0' || value[offset] > '9') {
                    return false;
                }
            }
            return true;
        }

        bool Is_Base_Integer(const std::string& value, char prefix, const char* digits) {
            size_t offset = 0;
            if (!value.empty() && value[0] == '-') {
                offset = 1;
            }
            if (offset + 2 >= value.size() || value[offset] != '0' ||
                (value[offset + 1] != prefix && value[offset + 1] != static_cast<char>(prefix - 'a' + 'A'))) {
                return false;
            }
            offset += 2;
            for (; offset < value.size(); ++offset) {
                if (std::strchr(digits, value[offset]) == nullptr) {
                    return false;
                }
            }
            return true;
        }

        bool Is_Integer(const std::string& value) {
            return Is_Decimal(value, true) || Is_Base_Integer(value, 'x', "0123456789abcdefABCDEF") ||
                Is_Base_Integer(value, 'b', "01");
        }

        bool Is_Ratio(const std::string& value) {
            const size_t slash = value.find('/');
            return slash != std::string::npos && slash == value.rfind('/') &&
                Is_Decimal(value.substr(0, slash), true) &&
                Is_Decimal(value.substr(slash + 1), true);
        }

        bool Is_Mask(const std::string& value) {
            size_t start = 0;
            bool has_comma = false;
            while (start <= value.size()) {
                const size_t comma = value.find(',', start);
                const size_t end = comma == std::string::npos ? value.size() : comma;
                if (!Is_Identifier(value.substr(start, end - start))) {
                    return false;
                }
                if (comma == std::string::npos) {
                    return has_comma;
                }
                has_comma = true;
                start = comma + 1;
            }
            return false;
        }

        bool Looks_Numeric(const std::string& value) {
            return !value.empty() && ((value[0] >= '0' && value[0] <= '9') || value[0] == '+' || (value[0] == '-' && value.size() > 1));
        }

        bool Append_Utf8(uint32_t codepoint, std::string* value) {
            if (codepoint > 0x10FFFF || (codepoint >= 0xD800 && codepoint <= 0xDFFF)) {
                return false;
            }
            if (codepoint <= 0x7F) {
                value->push_back(static_cast<char>(codepoint));
            } else if (codepoint <= 0x7FF) {
                value->push_back(static_cast<char>(0xC0 | codepoint >> 6));
                value->push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            } else if (codepoint <= 0xFFFF) {
                value->push_back(static_cast<char>(0xE0 | codepoint >> 12));
                value->push_back(static_cast<char>(0x80 | (codepoint >> 6 & 0x3F)));
                value->push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            } else {
                value->push_back(static_cast<char>(0xF0 | codepoint >> 18));
                value->push_back(static_cast<char>(0x80 | (codepoint >> 12 & 0x3F)));
                value->push_back(static_cast<char>(0x80 | (codepoint >> 6 & 0x3F)));
                value->push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
            }
            return true;
        }

        int Hex_Value(char character) {
            if (character >= '0' && character <= '9') {
                return character - '0';
            }
            if (character >= 'a' && character <= 'f') {
                return character - 'a' + 10;
            }
            if (character >= 'A' && character <= 'F') {
                return character - 'A' + 10;
            }
            return -1;
        }

        bool Read_Unicode_Escape(s_parser* parser, uint32_t* codepoint, const s_source_location& string_location) {
            uint32_t value = 0;
            for (size_t index = 0; index < 4; ++index) {
                if (At_End(parser)) {
                    return Fail(parser, string_location, "unterminated Unicode escape");
                }
                const int digit = Hex_Value(Current(parser));
                if (digit < 0) {
                    return Fail(parser, Location(parser), "invalid Unicode escape");
                }
                value = value << 4 | static_cast<uint32_t>(digit);
                Advance(parser);
            }

            if (value >= 0xD800 && value <= 0xDBFF) {
                if (At_End(parser) || Current(parser) != '\\') {
                    return Fail(parser, Location(parser), "high surrogate requires a low surrogate");
                }
                Advance(parser);
                if (At_End(parser) || Current(parser) != 'u') {
                    return Fail(parser, Location(parser), "high surrogate requires a low surrogate");
                }
                Advance(parser);
                uint32_t low = 0;
                for (size_t index = 0; index < 4; ++index) {
                    if (At_End(parser)) {
                        return Fail(parser, string_location, "unterminated Unicode escape");
                    }
                    const int digit = Hex_Value(Current(parser));
                    if (digit < 0) {
                        return Fail(parser, Location(parser), "invalid Unicode escape");
                    }
                    low = low << 4 | static_cast<uint32_t>(digit);
                    Advance(parser);
                }
                if (low < 0xDC00 || low > 0xDFFF) {
                    return Fail(parser, Location(parser), "high surrogate requires a low surrogate");
                }
                value = 0x10000 + ((value - 0xD800) << 10) + (low - 0xDC00);
            } else if (value >= 0xDC00 && value <= 0xDFFF) {
                return Fail(parser, Location(parser), "low surrogate requires a high surrogate");
            }
            *codepoint = value;
            return true;
        }

        bool Read_String(s_parser* parser, s_cfg_token* token) {
            token->kind = CFG_TOKEN_STRING;
            token->location = Location(parser);
            const size_t start = parser->offset;
            Advance(parser);
            while (!At_End(parser) && Current(parser) != '"') {
                const uint8_t byte = static_cast<uint8_t>(Current(parser));
                if (byte < 0x20) {
                    return Fail(parser, Location(parser), "unescaped control character in string");
                }
                if (Current(parser) != '\\') {
                    const size_t character_start = parser->offset;
                    Advance(parser);
                    token->value.append(parser->source + character_start,
                        parser->offset - character_start);
                    continue;
                }

                Advance(parser);
                if (At_End(parser)) {
                    return Fail(parser, token->location, "unterminated string");
                }
                const s_source_location escape_location = Location(parser);
                const char escape = Current(parser);
                Advance(parser);
                switch (escape) {
                    case '"': token->value.push_back('"'); break;
                    case '\\': token->value.push_back('\\'); break;
                    case '/': token->value.push_back('/'); break;
                    case 'b': token->value.push_back('\b'); break;
                    case 'f': token->value.push_back('\f'); break;
                    case 'n': token->value.push_back('\n'); break;
                    case 'r': token->value.push_back('\r'); break;
                    case 't': token->value.push_back('\t'); break;
                    case 'u': {
                        uint32_t codepoint = 0;
                        if (!Read_Unicode_Escape(parser, &codepoint, token->location)) {
                            return false;
                        }
                        if (!Append_Utf8(codepoint, &token->value)) {
                            return Fail(parser, token->location, "invalid Unicode escape");
                        }
                        break;
                    }
                    default:
                        return Fail(parser, escape_location, "invalid string escape");
                }
            }
            if (At_End(parser)) {
                return Fail(parser, token->location, "unterminated string");
            }
            Advance(parser);
            token->raw.assign(parser->source + start, parser->offset - start);
            if (!At_End(parser) && std::isspace(static_cast<uint8_t>(Current(parser))) == 0 &&
                Current(parser) != ';' && Current(parser) != '#') {
                return Fail(parser, Location(parser), "expected whitespace after string");
            }
            return true;
        }

        bool Read_Bare(s_parser* parser, s_cfg_token* token, bool allow_escapes) {
            token->location = Location(parser);
            const size_t start = parser->offset;
            bool has_escape = false;
            while (!At_End(parser) && std::isspace(static_cast<uint8_t>(Current(parser))) == 0 &&
                Current(parser) != ';' && Current(parser) != '#' && Current(parser) != '=') {
                if (allow_escapes && Current(parser) == ':') {
                    break;
                }
                if (allow_escapes && Current(parser) == '\\') {
                    const s_source_location escape_location = Location(parser);
                    Advance(parser);
                    if (At_End(parser) ||
                        (Current(parser) != ':' && Current(parser) != '\\')) {
                        return Fail(parser, escape_location, "invalid bare-token escape");
                    }
                    token->value.push_back(Current(parser));
                    Advance(parser);
                    has_escape = true;
                    continue;
                }
                token->value.push_back(Current(parser));
                Advance(parser);
            }
            token->raw.assign(parser->source + start, parser->offset - start);
            if (token->raw.empty()) {
                return Fail(parser, token->location, "expected a token");
            }
            if (has_escape) {
                token->kind = CFG_TOKEN_IDENTIFIER;
            } else if (token->raw == "-") {
                token->kind = CFG_TOKEN_SKIP;
            } else if (Is_Integer(token->raw)) {
                token->kind = CFG_TOKEN_INTEGER;
            } else if (Is_Ratio(token->raw)) {
                token->kind = CFG_TOKEN_RATIO;
            } else if (Is_Mask(token->raw)) {
                token->kind = CFG_TOKEN_MASK;
            } else if (Looks_Numeric(token->raw)) {
                return Fail(parser, token->location, "malformed numeric value '" + token->raw + "'");
            } else if (Is_Identifier(token->raw)) {
                token->kind = CFG_TOKEN_IDENTIFIER;
            } else {
                return Fail(parser, token->location, "invalid token '" + token->raw + "'");
            }
            return true;
        }

        bool Read_Value(s_parser* parser, s_cfg_token* token) {
            if (Current(parser) == '"') {
                return Read_String(parser, token);
            }
            return Read_Bare(parser, token, true);
        }

        bool Read_Label(s_parser* parser, s_cfg_token* selector, bool* matched) {
            *matched = false;
            const size_t start = parser->offset;
            const uint32_t start_line = parser->line;
            const uint32_t start_column = parser->column;
            if (Is_Identifier_Start(Current(parser))) {
                while (!At_End(parser) && Is_Identifier_Continue(Current(parser))) {
                    Advance(parser);
                }
                selector->kind = CFG_TOKEN_IDENTIFIER;
            } else if (Current(parser) >= '0' && Current(parser) <= '9') {
                while (!At_End(parser) && Current(parser) >= '0' &&
                    Current(parser) <= '9') {
                    Advance(parser);
                }
                selector->kind = CFG_TOKEN_INTEGER;
            } else {
                return true;
            }
            if (At_End(parser) || Current(parser) != ':') {
                parser->offset = start;
                parser->line = start_line;
                parser->column = start_column;
                return true;
            }

            selector->location.source = parser->source_name;
            selector->location.offset = start;
            selector->location.line = start_line;
            selector->location.column = start_column;
            selector->raw.assign(parser->source + start, parser->offset - start);
            selector->value = selector->raw;
            Advance(parser);
            if (At_End(parser) ||
                std::isspace(static_cast<uint8_t>(Current(parser))) == 0) {
                return Fail(parser, Location(parser),
                    "expected whitespace after labeled assignment selector");
            }
            *matched = true;
            return true;
        }

        bool Read_Semicolon(s_parser* parser, const s_source_location& statement_location) {
            Skip_Whitespace_And_Comments(parser);
            if (At_End(parser)) {
                return Fail(parser, statement_location, "unterminated statement; expected ';'");
            }
            if (Current(parser) != ';') {
                return Fail(parser, Location(parser), "expected ';'");
            }
            Advance(parser);
            return true;
        }

        bool Read_Stop(s_parser* parser, s_cfg_statement* statement) {
            statement->kind = CFG_STATEMENT_STOP;
            return Read_Semicolon(parser, statement->location);
        }

        bool Read_Include(s_parser* parser, s_cfg_statement* statement) {
            statement->kind = CFG_STATEMENT_INCLUDE;
            Skip_Whitespace_And_Comments(parser);
            if (At_End(parser) || Current(parser) == ';') {
                return Fail(parser, statement->location, "include requires a path");
            }
            if (Current(parser) == '"') {
                if (!Read_String(parser, &statement->argument)) {
                    return false;
                }
            } else {
                statement->argument.kind = CFG_TOKEN_PATH;
                statement->argument.location = Location(parser);
                const size_t start = parser->offset;
                while (!At_End(parser) &&
                    std::isspace(static_cast<uint8_t>(Current(parser))) == 0 &&
                    Current(parser) != ';' && Current(parser) != '#') {
                    if (Current(parser) == '=') {
                        return Fail(parser, Location(parser), "invalid character in include path");
                    }
                    Advance(parser);
                }
                statement->argument.raw.assign(parser->source + start, parser->offset - start);
                statement->argument.value = statement->argument.raw;
                if (statement->argument.raw.empty()) {
                    return Fail(parser, statement->argument.location, "include requires a path");
                }
            }
            return Read_Semicolon(parser, statement->location);
        }

        bool Read_Enable(s_parser* parser, s_cfg_statement* statement) {
            statement->kind = CFG_STATEMENT_ENABLE;
            Skip_Whitespace_And_Comments(parser);
            if (At_End(parser) || Current(parser) == ';') {
                return Fail(parser, statement->location, "enable requires a mod ID");
            }
            if (!Read_Bare(parser, &statement->argument, false)) {
                return false;
            }
            if (statement->argument.kind != CFG_TOKEN_IDENTIFIER) {
                return Fail(parser, statement->argument.location, "enable requires a symbolic package ID");
            }
            return Read_Semicolon(parser, statement->location);
        }

        bool Is_Path_Token(const s_cfg_token& token) {
            return token.kind == CFG_TOKEN_IDENTIFIER ||
                (token.kind == CFG_TOKEN_INTEGER && Is_Decimal(token.raw, false));
        }

        bool Read_Assignment(s_parser* parser, s_cfg_statement* statement, s_cfg_token&& first) {
            statement->kind = CFG_STATEMENT_ASSIGNMENT;
            if (!Is_Path_Token(first)) {
                return Fail(parser, first.location, "path tokens must be names or decimal indexes");
            }
            statement->path.push_back(std::move(first));

            while (true) {
                Skip_Whitespace_And_Comments(parser);
                if (At_End(parser)) {
                    return Fail(parser, statement->location, "unterminated assignment; expected '='");
                }
                if (Current(parser) == '=') {
                    Advance(parser);
                    break;
                }
                if (Current(parser) == ';') {
                    return Fail(parser, Location(parser), "assignment is missing '='");
                }
                s_cfg_token token;
                if (!Read_Bare(parser, &token, false)) {
                    return false;
                }
                if (!Is_Path_Token(token)) {
                    return Fail(parser, token.location, "path tokens must be names or decimal indexes");
                }
                statement->path.push_back(std::move(token));
            }

            while (true) {
                Skip_Whitespace_And_Comments(parser);
                if (At_End(parser)) {
                    return Fail(parser, statement->location, "unterminated assignment; expected ';'");
                }
                if (Current(parser) == ';') {
                    if (!statement->groups.empty() &&
                        statement->groups.back().values.empty()) {
                        return Fail(parser,
                            statement->groups.back().selector.location,
                            "labeled assignment group requires at least one value");
                    }
                    if (statement->values.empty() && statement->groups.empty()) {
                        return Fail(parser, Location(parser), "assignment requires at least one value");
                    }
                    Advance(parser);
                    return true;
                }
                if (Current(parser) == '=') {
                    return Fail(parser, Location(parser), "unexpected '=' in assignment values");
                }
                s_cfg_token selector;
                bool has_label = false;
                if (!Read_Label(parser, &selector, &has_label)) {
                    return false;
                }
                if (has_label) {
                    if (!statement->values.empty()) {
                        return Fail(parser, selector.location,
                            "labeled assignment must begin with a selector");
                    }
                    if (!statement->groups.empty() &&
                        statement->groups.back().values.empty()) {
                        return Fail(parser,
                            statement->groups.back().selector.location,
                            "labeled assignment group requires at least one value");
                    }
                    for (const s_cfg_assignment_group& group : statement->groups) {
                        if (group.selector.kind == selector.kind &&
                            group.selector.value == selector.value) {
                            return Fail(parser, selector.location,
                                "duplicate labeled assignment selector '" +
                                    selector.raw + "'");
                        }
                    }
                    statement->groups.emplace_back();
                    statement->groups.back().selector = std::move(selector);
                    continue;
                }
                std::vector<s_cfg_token>* values = statement->groups.empty() ?
                    &statement->values : &statement->groups.back().values;
                values->emplace_back();
                if (!Read_Value(parser, &values->back())) {
                    return false;
                }
            }
        }

        bool Read_Statement(s_parser* parser, s_cfg_statement* statement) {
            statement->location = Location(parser);
            if (Current(parser) == ';') {
                return Fail(parser, statement->location, "empty statements are not supported");
            }
            if (Current(parser) == '"') {
                return Fail(parser, statement->location, "a statement must begin with a name");
            }

            s_cfg_token first;
            if (!Read_Bare(parser, &first, false)) {
                return false;
            }
            if (first.kind != CFG_TOKEN_IDENTIFIER) {
                return Fail(parser, first.location, "a statement must begin with a name");
            }
            if (first.raw == "stop") {
                return Read_Stop(parser, statement);
            }
            if (first.raw == "enable") {
                return Read_Enable(parser, statement);
            }
            if (first.raw == "include") {
                return Read_Include(parser, statement);
            }
            return Read_Assignment(parser, statement, std::move(first));
        }
    } // namespace

    bool Parse_Cfg(const char* source_name, const char* source, size_t source_size, s_cfg_document* document, s_cfg_diagnostic* diagnostic) {
        if (source_name == nullptr || source == nullptr || document == nullptr || diagnostic == nullptr) {
            return false;
        }

        document->statements.clear();
        *diagnostic = {};
        s_parser parser;
        parser.source_name = source_name;
        parser.source = source;
        parser.source_size = source_size;
        parser.diagnostic = diagnostic;
        if (!Validate_Utf8(&parser)) {
            return false;
        }

        while (true) {
            Skip_Whitespace_And_Comments(&parser);
            if (At_End(&parser)) {
                return true;
            }
            document->statements.emplace_back();
            document->statements.back().statement_number =
                static_cast<uint32_t>(document->statements.size());
            if (!Read_Statement(&parser, &document->statements.back())) {
                document->statements.clear();
                return false;
            }
        }
    }

} // namespace config