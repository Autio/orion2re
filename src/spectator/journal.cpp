#include "journal.h"

#include <cstdio>
#include <utility>

namespace spectator_journal {
    static std::string Quote_(const std::string& value) {
        std::string result = "\"";
        for (unsigned char c : value) {
            if (c == '"' || c == '\\') {
                result += '\\';
                result += static_cast<char>(c);
            } else if (c < 32 || c >= 127) {
                char escape[7];
                std::snprintf(escape, sizeof(escape), "\\u%04x", static_cast<unsigned>(c));
                result += escape;
            } else {
                result += static_cast<char>(c);
            }
        }
        return result + '"';
    }

    void Add_Event(s_turn* turn, s_event event) {
        if (turn->events.size() < 256) {
            turn->events.push_back(std::move(event));
        } else {
            ++turn->dropped_events;
        }
    }

    std::string Serialize_Turn(const s_turn& turn) {
        std::string result = "{\"schema\":1,\"session\":" + Quote_(turn.session) +
            ",\"stardate\":" + std::to_string(turn.stardate) +
            ",\"finished\":" + (turn.finished ? "true" : "false") +
            ",\"dropped_events\":" + std::to_string(turn.dropped_events) + ",\"empires\":[";
        bool first = true;
        for (const auto& empire : turn.empires) {
            if (!first) result += ',';
            first = false;
            result += "{\"id\":" + std::to_string(empire.id) + ",\"name\":" + Quote_(empire.name) +
                ",\"personality\":" + std::to_string(empire.personality) +
                ",\"colonies\":" + std::to_string(empire.colonies) +
                ",\"population\":" + std::to_string(empire.population) +
                ",\"ships\":" + std::to_string(empire.ships) +
                ",\"eliminated\":" + (empire.eliminated ? "true" : "false") +
                ",\"research_project\":" + Quote_(empire.research_project) +
                ",\"research_accumulated\":" + std::to_string(empire.research_accumulated) + '}';
        }
        result += "],\"events\":[";
        first = true;
        for (const auto& event : turn.events) {
            if (!first) result += ',';
            first = false;
            result += "{\"player\":" + std::to_string(event.player) +
                ",\"target\":" + std::to_string(event.target) +
                ",\"kind\":" + Quote_(event.kind) + ",\"text\":" + Quote_(event.text) +
                ",\"evidence\":" + Quote_(event.evidence) + '}';
        }
        return result + "]}\n";
    }

    bool Append_Turn(const char* path, const s_turn& turn) {
        FILE* file = std::fopen(path, "ab");
        if (file == nullptr) return false;
        const std::string record = Serialize_Turn(turn);
        bool success = std::fwrite(record.data(), 1, record.size(), file) == record.size();
        if (std::fclose(file) != 0) success = false;
        return success;
    }
}
