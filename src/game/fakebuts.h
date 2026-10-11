#pragma once
#include <cstdint>

struct s_animation_header;

typedef struct s_click_through_field s_click_through_field, *Ps_click_through_field;

struct s_click_through_field {
    int16_t x1;
    int16_t y1;
    int16_t x2;
    int16_t y2;
    int16_t field_id1;
    int16_t field_id2;
    char** strings;
    int16_t state_count;
    int16_t* value;
    uint8_t color;
};

typedef struct s_fake_button s_fake_button, *Ps_fake_button;

struct s_fake_button {
    int16_t x;
    int16_t y;
    int16_t field_id;
    struct s_animation_header* pic;
};

namespace FAKEBUTS {
    extern uint8_t n_fake_buttons;

    extern s_fake_button fake_buttons[25];

    // Resets the registered fake-button list.
    void __cdecl Clear_Fake_Buttons();
    // Draws all registered fake buttons, highlighting the current auto-input field.
    void __cdecl Draw_Fake_Buttons();

    extern uint8_t n_click_thru_fields;

    extern s_click_through_field click_thru_fields[25];

    // Clears all registered click-through fields.
    void __cdecl Clear_Click_Thru_Fields();
    // Advances the selected state for click-through fields triggered by the given field ID.
    void __cdecl Evaluate_Click_Thru_Fields(int16_t activated_field_id);

    // Draws registered click-through fields using their current string state.
    void __cdecl Draw_Click_Thru_Fields();

    // Adds a click-through field entry to the internal registry.
    void __cdecl Add_Click_Thru_Field_To_List(
        int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t primary_field_id, char** strings, int16_t state_count, int16_t* value, uint8_t color, int16_t secondary_field_id);

    // Creates a hidden field and registers a matching click-through field entry.
    int16_t __cdecl Add_Click_Thru_Field(int16_t x1, int16_t y1, int16_t x2, int16_t y2, char** strings, int16_t state_count, int16_t* value, uint8_t color, const char* help_text, int16_t hidden_field_style, int16_t secondary_field_id);
} // namespace FAKEBUTS