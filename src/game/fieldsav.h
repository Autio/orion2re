#pragma once

struct s_field;

typedef struct s_fieldsav_data s_fieldsav_data, *Ps_fieldsav_data;

struct s_fieldsav_data {
    int16_t field_count;
    int16_t input_delay;
    int16_t down_mouse_button;
    int16_t auto_input_variable;
    int16_t input_field_active;
    int16_t active_input_field_number;
    int16_t max_fields_count;
    struct s_field* fields;
};

namespace FIELDSAV {
    /** Saves the current field state into the provided save buffer.
     * @param data Destination structure that receives the current field state.
     */
    void __cdecl Save_Field_Stats(s_fieldsav_data* data);
    /** Restores the current field state from the provided save buffer.
     * @param data Source structure that provides the field state to restore.
     */
    void __cdecl Restore_Field_Stats(s_fieldsav_data* data);
} // namespace FIELDSAV