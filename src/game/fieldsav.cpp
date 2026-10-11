#include "pch.h"

namespace FIELDSAV {
    void __cdecl Save_Field_Stats(s_fieldsav_data* data) {
        int16_t max_fields_count = (int16_t)fields::max_fields_count;

        data->field_count = fields::fields_count;
        data->input_delay = fields::input_delay;
        data->down_mouse_button = fields::down_mouse_button;
        data->auto_input_variable = fields::auto_input_variable;
        data->input_field_active = fields::input_field_active;
        data->active_input_field_number = fields::active_input_field_number;
        data->max_fields_count = max_fields_count;
        data->fields = fields::fields;

        fields::fields += fields::fields_count;
        max_fields_count -= fields::fields_count;
        fields::max_fields_count = max_fields_count;

        fields::Clear_Fields();
    }

    void __cdecl Restore_Field_Stats(s_fieldsav_data* data) {
        fields::fields_count = data->field_count;
        fields::input_delay = data->input_delay;
        fields::down_mouse_button = data->down_mouse_button;
        fields::auto_input_variable = data->auto_input_variable;
        fields::input_field_active = data->input_field_active;
        fields::active_input_field_number = data->active_input_field_number;
        fields::fields = data->fields;
        fields::max_fields_count = data->max_fields_count;
    }
} // namespace FIELDSAV