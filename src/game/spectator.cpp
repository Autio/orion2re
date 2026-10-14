#include "pch.h"
#include "spectator.h"
#include "spectator/journal.h"

#include <chrono>
#include <string>

namespace spectator {
    static spectator_journal::s_turn turn;
    static std::string journal_path;
    static std::string control_path;
    static bool recording = false;
    static bool capturing = false;
    static bool finished = false;
    static int previous_treaties[MAX_PLAYERS][MAX_PLAYERS];
    static int previous_trade[MAX_PLAYERS][MAX_PLAYERS];
    static int previous_research[MAX_PLAYERS][MAX_PLAYERS];
    static std::chrono::steady_clock::time_point last_advance;
    static std::chrono::steady_clock::time_point last_poll;
    static int interval_ms = 0;

    static bool Eligible_() {
        if (!recording || MOX::game_type != 0 || RUSS::all_ai_game == 0) return false;
        // The legacy all_ai_game flag can survive a load or a new-game transition.
        for (int i = 0; i < MOX::NUM_PLAYERS; ++i) {
            if (!MOX::player[i].eliminated && MOX::player[i].objectives == PLAYER_OBJECTIVE_HUMAN) return false;
        }
        return true;
    }

    bool Owns_Playback() {
        return !journal_path.empty() && MOX::game_type == 0 && RUSS::all_ai_game != 0;
    }

    static std::string Name_(int player) {
        const auto& name = MOX::player[player].race_name;
        size_t length = 0;
        while (length < sizeof(name) && name[length] != 0) ++length;
        return std::string(name, length);
    }

    static void Snapshot_() {
        turn.empires.clear();
        int living = 0;
        bool victory = false;
        for (int i = 0; i < MOX::NUM_PLAYERS; ++i) {
            const auto& p = MOX::player[i];
            turn.empires.push_back({i, Name_(i), p.personality, p.total_colonies,
                p.total_pop, p.total_ships, p.eliminated != 0});
            if (!p.eliminated) ++living;
            if (p.captured_antares) victory = true;
        }
        finished = living < 2 || victory || diplodef::council_winner != -1;
        turn.finished = finished;
    }

    static void Publish_() {
        Snapshot_();
        if (!spectator_journal::Append_Turn(journal_path.c_str(), turn)) {
            std::fprintf(stderr, "Spectator journal write failed: %s; autoplay stopped.\n", journal_path.c_str());
            recording = false;
        }
        last_advance = std::chrono::steady_clock::now();
    }

    void Start_Session() {
        recording = false;
        capturing = false;
        journal_path.clear();
        const char* path = std::getenv("ORION2RE_JOURNAL");
        if (path == nullptr || *path == 0 || MOX::game_type != 0) return;
        journal_path = path;
        const char* control = std::getenv("ORION2RE_STREAM_CONTROL");
        control_path = control == nullptr ? "" : control;
        turn = {};
        turn.session = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
        turn.stardate = MOX::stardate;
        recording = true;
        interval_ms = 0;
        Publish_();
    }

    void Begin_Turn() {
        capturing = Eligible_();
        if (!capturing) return;
        // A loaded earlier save starts a new broadcast segment rather than merging timelines.
        if (MOX::stardate < turn.stardate) Start_Session();
        capturing = Eligible_();
        turn.stardate = MOX::stardate;
        turn.events.clear();
        turn.dropped_events = 0;
        for (int i = 0; i < MOX::NUM_PLAYERS; ++i) {
            for (int j = 0; j < MOX::NUM_PLAYERS; ++j) {
                previous_treaties[i][j] = MOX::player[i].treaty[j];
                previous_trade[i][j] = MOX::player[i].trade_treaty[j];
                previous_research[i][j] = MOX::player[i].research_treaty[j];
            }
        }
    }

    void Proposal_Evaluated(int player, int target, int category, int score) {
        if (!capturing) return;
        const char* stance = score >= 0 ? "The proposal passes my initial evaluation." :
            score < -75 ? "The proposal fails my initial evaluation." :
            "I may need concessions before agreeing.";
        spectator_journal::Add_Event(&turn, {player, target, "evaluation",
            "Considering an agreement with " + Name_(target) + ". " + stance,
            "Diplomacy_Test: score=" + std::to_string(score) + "; modifier bucket=" +
                std::to_string(category) + "; relations=" +
                std::to_string(MOX::player[player].relations[target]) +
                ". This is an evaluation, not a completed agreement."});
    }

    void War_Selected(int player, int target, int reason) {
        if (!capturing) return;
        const char* motivation = "The strategic war checks selected this target.";
        switch (reason) {
            case 20: motivation = "My personality-driven aggression check selected this target."; break;
            case 22: motivation = "I am responding to colony losses."; break;
            case 68: motivation = "Poor relations and a favorable fleet comparison present an opportunity."; break;
            case 113: motivation = "Expansion or food pressure has pushed me toward war."; break;
        }
        spectator_journal::Add_Event(&turn, {player, target, "decision",
            "I have selected " + Name_(target) + " for war. " + motivation,
            "NPC_Declarations_Of_War: reason=" + std::to_string(reason) +
                "; relations=" + std::to_string(MOX::player[player].relations[target])});
    }

    void End_Turn() {
        if (!capturing) return;
        for (int i = 0; i < MOX::NUM_PLAYERS; ++i) {
            for (int j = 0; j < MOX::NUM_PLAYERS; ++j) {
                if (i == j) continue;
                const auto& p = MOX::player[i];
                if (previous_treaties[i][j] != p.treaty[j] ||
                    previous_trade[i][j] != p.trade_treaty[j] ||
                    previous_research[i][j] != p.research_treaty[j]) {
                    std::string outcome = "Our diplomatic arrangements with " + Name_(j) + " have changed.";
                    if (previous_treaties[i][j] < 4 && p.treaty[j] >= 4) {
                        outcome = "We are now at war with " + Name_(j) + ".";
                    } else if (previous_treaties[i][j] >= 4 && p.treaty[j] < 4) {
                        outcome = "Our war with " + Name_(j) + " has ended.";
                    } else if (previous_trade[i][j] == 0 && p.trade_treaty[j] != 0) {
                        outcome = "We now have a trade agreement with " + Name_(j) + ".";
                    } else if (previous_research[i][j] == 0 && p.research_treaty[j] != 0) {
                        outcome = "We now have a research agreement with " + Name_(j) + ".";
                    }
                    spectator_journal::Add_Event(&turn, {i, j, "outcome",
                        outcome,
                        "Treaty " + std::to_string(previous_treaties[i][j]) + " -> " + std::to_string(p.treaty[j]) +
                            "; trade " + std::to_string(previous_trade[i][j]) + " -> " + std::to_string(p.trade_treaty[j]) +
                            "; research " + std::to_string(previous_research[i][j]) + " -> " + std::to_string(p.research_treaty[j])});
                }
            }
        }
        Publish_();
        capturing = false;
    }

    bool Should_Advance() {
        if (!Eligible_() || finished || control_path.empty()) return false;
        const auto now = std::chrono::steady_clock::now();
        if (now - last_poll >= std::chrono::milliseconds(250)) {
            last_poll = now;
            interval_ms = 0;
            FILE* file = std::fopen(control_path.c_str(), "r");
            if (file != nullptr) {
                int requested = 0;
                if (std::fscanf(file, "%d", &requested) == 1 && requested >= 1000 && requested <= 60000) {
                    interval_ms = requested;
                }
                std::fclose(file);
            }
        }
        if (interval_ms == 0 || now - last_advance < std::chrono::milliseconds(interval_ms)) return false;
        last_advance = now;
        return true;
    }
}
