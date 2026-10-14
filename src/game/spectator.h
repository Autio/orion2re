#pragma once

namespace spectator {
    void Start_Session();
    void Begin_Turn();
    void End_Turn();
    bool Should_Advance();
    bool Owns_Playback();
    void Proposal_Evaluated(int player, int target, int category, int score);
    void War_Selected(int player, int target, int reason);
}
