#pragma once

#include <string>
#include <vector>

namespace spectator_journal {
    struct s_event {
        int player;
        int target;
        std::string kind;
        std::string text;
        std::string evidence;
    };

    struct s_empire {
        int id;
        std::string name;
        int personality;
        int colonies;
        int population;
        int ships;
        bool eliminated;
    };

    struct s_turn {
        std::string session;
        int stardate = 0;
        bool finished = false;
        int dropped_events = 0;
        std::vector<s_empire> empires;
        std::vector<s_event> events;
    };

    // Produces one complete ASCII JSON record; game strings are legacy byte strings.
    std::string Serialize_Turn(const s_turn& turn);
    bool Append_Turn(const char* path, const s_turn& turn);
    void Add_Event(s_turn* turn, s_event event);
}
