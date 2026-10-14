#pragma once

namespace spectator {
    void Start_Session();
    void Begin_Turn();
    void End_Turn();
    bool Should_Advance();
    bool Owns_Playback();
    bool Watch_Battles();
    // Attribution exists only while the actual combat/post-combat operation runs.
    class Attack_Context {
        int previous_;
    public:
        explicit Attack_Context(int attacker);
        ~Attack_Context();
        Attack_Context(const Attack_Context&) = delete;
        Attack_Context& operator=(const Attack_Context&) = delete;
    };
    void Colony_Founded(int player, int planet, bool outpost);
    void Colony_Destroyed(int owner, int planet, int reason);
    void Colony_Transferred(int owner, int new_owner, int planet);
    void Battle_Event(int attacker, int defender, const char* location,
                      int attacker_count, int defender_count, bool tactical, int result, bool ended);
    void Proposal_Evaluated(int player, int target, int category, int score);
    void War_Selected(int player, int target, int reason);
}
