#include "pch.h"

#include "game/tech.h"

namespace TECH {

    s_list_item_research* __cdecl Get_Selected_Entry(
        s_list_item_research* entries) {
        s_list_item_research* current = entries;
        s_list_item_research* end = &entries[8];

        do {
            if (current->current_app_index != 0) {
                return current;
            }
            current++;
        } while (current != end);

        return nullptr;
    }

    bool __cdecl Commit_Selected_Entry(
        s_player* player, const s_list_item_research* selected_entry) {
        if (player == nullptr || selected_entry == nullptr ||
            selected_entry->selected_app_index < 0 ||
            selected_entry->selected_app_index >= 4) {
            return false;
        }
        player->current_research_field =
            static_cast<int8_t>(selected_entry->tech_field_id);
        player->current_research_application = static_cast<int8_t>(
            selected_entry->app_ids[selected_entry->selected_app_index]);
        return true;
    }

} // namespace TECH