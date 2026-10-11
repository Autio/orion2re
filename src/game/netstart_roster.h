#pragma once

#include "game/deps.h"

namespace NETSTART {
    inline int16_t Assign_New_Game_Network_Roster_(s_player* players,
        s_net_pack_start_game* message,
        const int16_t* users,
        int connected_players,
        int16_t local_net_id) {
        int16_t local_player = -1;
        for (int index = 0; index < MAX_PLAYERS; ++index) {
            // Lua negotiation and broadcasts use -1, not zero, for AI slots.
            players[index].network_player_id = -1;
            message->net_ids[index] = 0;
            if (index < connected_players) {
                players[index].network_player_id = users[index];
                message->net_ids[index] = users[index];
                if (users[index] == local_net_id) {
                    local_player = static_cast<int16_t>(index);
                }
            }
        }
        if (local_player == -1) {
            players[0].network_player_id = local_net_id;
            message->net_ids[0] = local_net_id;
            local_player = 0;
        }
        return local_player;
    }
} // namespace NETSTART