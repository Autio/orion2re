#include "pch.h"

namespace FAKEBUTS {
    uint8_t n_fake_buttons;

    s_fake_button fake_buttons[25];

    uint8_t n_click_thru_fields;

    s_click_through_field click_thru_fields[25];

    void __cdecl Clear_Click_Thru_Fields() {
        memset(click_thru_fields, 0, sizeof(click_thru_fields));
        n_click_thru_fields = 0;
    }

    void __cdecl Clear_Fake_Buttons() {
        memset(fake_buttons, 0, sizeof(fake_buttons));
        n_fake_buttons = 0;
    }

    void __cdecl Evaluate_Click_Thru_Fields(int16_t activated_field_id) {
        if (n_click_thru_fields != 0) {
            for (int16_t i = 0; i < n_click_thru_fields; ++i) {
                if (click_thru_fields[i].field_id1 == activated_field_id ||
                    click_thru_fields[i].field_id2 == activated_field_id) {
                    *click_thru_fields[i].value =
                        (*click_thru_fields[i].value + 1) % click_thru_fields[i].state_count;
                }
            }
        }
    }

    void __cdecl Draw_Click_Thru_Fields() {
        if (n_click_thru_fields != 0) {
            for (int16_t i = 0; i < n_click_thru_fields; ++i) {
                s_click_through_field* field = &click_thru_fields[i];

                if (field->color != 0) {
                    graphics::Box(
                        field->x1,
                        field->y1,
                        field->x2 - field->x1,
                        field->y2 - field->y1,
                        1,
                        field->color);
                }

                graphics::Set_Window(field->x1, field->y1, field->x2, field->y2);
                video::Clip_On();

                BILL::Squeeze_Paragraph_Centered(
                    field->x1,
                    field->y1 + 2,
                    field->x2 - field->x1,
                    field->y2 - field->y1 - 2,
                    field->strings[*field->value],
                    2);

                video::Clip_Off();
            }
        }
    }

    int16_t __cdecl Add_Click_Thru_Field(int16_t x1, int16_t y1, int16_t x2, int16_t y2, char** strings, int16_t state_count, int16_t* value, uint8_t color, const char* help_text, int16_t hidden_field_style, int16_t secondary_field_id) {
        int16_t field_id = fields::Add_Hidden_Field(x1, y1, x2, y2, help_text, (int8_t)hidden_field_style);
        Add_Click_Thru_Field_To_List(x1, y1, x2, y2, field_id, strings, state_count, value, color, secondary_field_id);
        return field_id;
    }

    void __cdecl Add_Click_Thru_Field_To_List(
        int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t primary_field_id, char** strings, int16_t state_count, int16_t* value, uint8_t color, int16_t secondary_field_id) {
        uint8_t idx = n_click_thru_fields;

        click_thru_fields[idx].x1 = x1;
        click_thru_fields[idx].y1 = y1;
        click_thru_fields[idx].x2 = x2;
        click_thru_fields[idx].y2 = y2;
        click_thru_fields[idx].field_id1 = primary_field_id;
        click_thru_fields[idx].strings = strings;

        n_click_thru_fields = idx + 1;

        click_thru_fields[idx].state_count = state_count;
        click_thru_fields[idx].value = value;
        click_thru_fields[idx].color = color;
        click_thru_fields[idx].field_id2 = secondary_field_id;
    }

    void __cdecl Draw_Fake_Buttons() {
        int16_t auto_input = fields::Auto_Input();

        for (int16_t i = 0; i < n_fake_buttons; i++) {
            if (fake_buttons[i].field_id == auto_input) {
                if (animate::Get_Animation_Frame_Count(fake_buttons[i].pic) <= 0) {
                    continue;
                }
                animate::Set_Animation_Frame(fake_buttons[i].pic, 1);
            } else {
                animate::Reset_Animation_Frame(fake_buttons[i].pic);
            }
            animate::Draw(fake_buttons[i].x, fake_buttons[i].y, fake_buttons[i].pic);
        }
    }
} // namespace FAKEBUTS