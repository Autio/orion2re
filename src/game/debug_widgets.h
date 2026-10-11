#pragma once

namespace debug_widgets {
    using bit_label_provider = const char* (*)(uint16_t bit_index, void* user_data);

    bool Bit_Field_Checkboxes(
        const char* id,
        void* bit_field,
        uint16_t byte_count,
        uint16_t bit_count,
        bit_label_provider label_provider,
        void* user_data = nullptr,
        uint16_t column_count = 1);
} // namespace debug_widgets