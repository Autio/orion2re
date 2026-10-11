#include "pch.h"

#if defined(ORION2RE_ENABLE_DEBUG_OVERLAY)
#include <imgui.h>
#endif

namespace debug_widgets {
    bool Bit_Field_Checkboxes(
        const char* id,
        void* bit_field,
        uint16_t byte_count,
        uint16_t bit_count,
        bit_label_provider label_provider,
        void* user_data,
        uint16_t column_count) {
#if defined(ORION2RE_ENABLE_DEBUG_OVERLAY)
        if (id == nullptr || bit_field == nullptr || bit_count == 0 ||
            bit_count > (uint32_t)byte_count * 8u) {
            return false;
        }

        if (column_count == 0) {
            column_count = 1;
        }

        bool changed = false;
        uint8_t* bytes = (uint8_t*)bit_field;
        ImGui::PushID(id);
        if (ImGui::BeginTable("bits", column_count)) {
            for (uint16_t bit_index = 0; bit_index < bit_count; ++bit_index) {
                const uint16_t byte_index = (uint16_t)(bit_index >> 3);
                const uint8_t mask = (uint8_t)(1u << (bit_index & 7u));
                bool selected = (bytes[byte_index] & mask) != 0;
                const char* label = label_provider != nullptr
                    ? label_provider(bit_index, user_data)
                    : nullptr;
                char fallback_label[24];
                if (label == nullptr || label[0] == '\0') {
                    snprintf(fallback_label, sizeof(fallback_label), "Bit %u", (unsigned)bit_index);
                    label = fallback_label;
                }

                ImGui::TableNextColumn();
                ImGui::PushID((int32_t)bit_index);
                if (ImGui::Checkbox(label, &selected)) {
                    if (selected) {
                        bytes[byte_index] |= mask;
                    } else {
                        bytes[byte_index] &= (uint8_t)~mask;
                    }
                    changed = true;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Bit %u", (unsigned)bit_index);
                }
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::PopID();
        return changed;
#else
        (void)id;
        (void)bit_field;
        (void)byte_count;
        (void)bit_count;
        (void)label_provider;
        (void)user_data;
        (void)column_count;
        return false;
#endif
    }
} // namespace debug_widgets