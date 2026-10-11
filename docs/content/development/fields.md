# Fields

The `fields` module is the game's generic UI and input layer. It provides a shared runtime for clickable regions, buttons, list rows, scroll bars, text entry, grid and circular hit regions, help overlays, and hotkey-only actions.

The code is old-style and heavily stateful. It is best understood as a small UI runtime driven by global arrays and flags rather than as a widget tree.

## Source layout

- `src/game/fields.cpp` implements field construction, rendering, and input interpretation.
- `src/game/fields.h` declares the public functions and typed payload helpers.
- `src/game/orion2.h` contains the core field and payload structures.

## Runtime model

All active interactive objects live in `fields::fields`. The number of used entries is stored in `fields::fields_count`; index `0` is effectively reserved, and normal iteration begins at index `1`.

Each `s_field` contains:

- the rectangular hit box in `x`, `y`, `x_end`, and `y_end`
- a `field_type` tag
- text style and color information
- the type-specific `detail` payload
- an optional hotkey
- a sound to play on activation

The detail member is a tagged union. Use helpers such as `Field_Data_Button()`, `Field_Data_Radio()`, `Field_Data_Scroll()`, and `Field_Data_String_List()` rather than selecting union members directly. The helpers validate `field_type` and terminate through `Field_Type_Mismatch()` when the requested payload does not match.

## Lifecycle and ownership

A screen normally uses the field system in this order:

1. Call `Clear_Fields()`.
2. Add entries with the appropriate `Add_*` functions.
3. Configure help boxes or an auto function when needed.
4. Draw with `Draw_Visible_Fields()`, `Draw_Visible_Fields_Back()`, or `Draw_All_Fields()`.
5. Poll with `Get_Input()` and interpret the returned field or action value.
6. Clear or rebuild the list when the screen changes.

There is no ownership abstraction for payload memory. Text, selection values, scroll positions, list state, and animation headers are commonly stored as pointers into caller-owned data. Those objects must remain valid for the entire lifetime of the field.

`Mark_Fields()`, `Release_Fields()`, and `Clear_Fields_Above()` provide limited save and restore support for screens that temporarily add a nested set of fields.

## Field types

The active field types fall into a few groups.

| Type | Purpose | Common constructor |
| --- | --- | --- |
| `FIELD_TYPE_BUTTON` | Rectangular sprite and text button | `Add_Button_Field()` |
| `FIELD_TYPE_IRREGULAR_BUTTON` | Button with separate hit and draw positions | `Add_Irregular_Button_Field()` |
| `FIELD_TYPE_RADIO` | Two-state toggle | `Add_Radio_Button_Field()` |
| `FIELD_TYPE_LOCKED` | One-way latch or confirmation | `Add_Locked_Button_Field()` |
| `FIELD_TYPE_MULTI_BUTTON` | One member of a shared selection group | `Add_Multi_Button_Field()` |
| `FIELD_TYPE_SCROLL` | Drag-adjustable numeric value | `Add_Scroll_Field()` |
| `FIELD_TYPE_SCROLL_RETURN` | Scroll payload that returns an action value | Converted from an existing scroll field |
| `FIELD_TYPE_HIDDEN` | Invisible hit region or keyboard-only action | `Add_Hidden_Field()` or `Add_Hot_Key()` |
| `FIELD_TYPE_MULTI_HOTKEY` | Multi-key sequence | `Add_Multi_Hot_Key_Field()` |
| `FIELD_TYPE_STRING_LIST` | Selectable or informational list row | `Add_String_List_Field()` |
| `FIELD_TYPE_CONTINUOUS_INPUT` | Continuously active text input | `Add_Continuous_String_Input_Field()` |
| `FIELD_TYPE_GRID` | Region that reports grid coordinates | `Add_Grid_Field()` |
| `FIELD_TYPE_CIRCLE` | Circular hit region within clipped bounds | `Add_Clipped_Circle_Field()` |
| `FIELD_TYPE_LEGACY_INPUT` | Older modal text input | No clear active constructor path |

Most constructors append one entry, fill the common header and typed payload, normalize a one-character uppercase ASCII hotkey to lowercase, and return the new field index.

## Rendering

`Draw_Field(field_idx, action)` handles one field. An action of `0` normally selects the resting appearance; `1` normally selects the pressed appearance or performs the field-specific interaction update. Some field types use this function as a small behavior dispatcher as well as a renderer. Scroll fields update their bar position, grid fields update coordinates, and legacy input fields can enter their edit loop.

`Draw_Visible_Fields()` draws the active list to the Off page. It saves and restores the graphics window, handles pressed-field state, draws type-specific content, and can add field or help outlines. `Draw_Visible_Fields_Back()` performs the corresponding pass for the Back page. The two functions intentionally duplicate behavior, so a rendering change commonly needs to be applied to both.

`Draw_All_Fields()` forces a full field pass by temporarily enabling the draw override.

## Input and return values

`Get_Input()` is the usual polling entry point. It applies the configured input delay, delegates to `Interpret_Mouse_Input()`, and plays the selected field sound when an action is returned. Despite its name, `Interpret_Mouse_Input()` is the main router for both mouse and keyboard activity.

The router handles:

- field hotkeys and command keys
- multi-key sequence progress
- buffered clicks and held-button behavior
- continuous text editing
- right-click help
- circular hit testing
- scroll and grid semantics

Return values are part of the interface:

- a positive value is an activated field id or action value
- a negative value is commonly a cancel or right-click variant
- `0` means that no action is ready

Do not treat every negative value as an internal error.

`Interpret_Keyboard_Input()` maps a raw key to a matching field, command code, completed multi-hotkey, or the original key value. `Scan_Field()` performs rectangular hit testing. `Scan_Input()` adds type-specific behavior such as rejecting informational list rows, updating grid coordinates, or returning the configured scroll action.

SDL events do not call this layer directly. The platform thread queues translated input, and the game thread drains that queue before the legacy mouse and keyboard state is queried. See [Threading and SDL](threading.md#input-handoff).

## Text input

Two text systems remain in the module.

Legacy input uses `Display_Input_Field()` and the modal `Input_String()` edit loop. Continuous input keeps the active field and edit state in globals, mirrors the text into `continuous_string`, adds or removes the cursor marker during drawing, and commits through `Copy_Continuous_String()`.

Before adding text behavior, determine which model the calling screen already uses. Mixing the state transitions of the two systems can leave the wrong field active or commit into the wrong caller-owned buffer.

## Lists, help, and auto functions

`Get_List_Field()` constructs a temporary set of `FIELD_TYPE_STRING_LIST` rows, enters a `Get_Input()` polling loop, and clears the temporary fields before returning. It returns `-1` for cancellation or the accepted zero-based row index.

Help boxes are maintained separately from the field list. `Set_Help_List()`, `Deactivate_Help_List()`, and `Check_Help_List()` manage rectangular help regions whose ids are passed to `TEXTBOX::Draw_Help_Entry()`.

The auto-function mechanism supplies screen-specific work while field code waits, drags, or redraws. Use `Assign_Auto_Function()`, `Save_Auto_Function()`, `Restore_Auto_Function()`, and the invocation helpers rather than replacing `fields::function` without preserving the previous value.

## Debugging guidance

For an input bug, follow this path:

1. `Get_Input()`
2. `Interpret_Mouse_Input()`
3. `Interpret_Keyboard_Input()`, `Scan_Field()`, or `Scan_Input()`
4. the type-specific handler

For a rendering bug, inspect:

1. the constructor that populated `s_field`
2. `Draw_Field()`
3. both visible-field drawing passes

When changing the module:

- use typed payload helpers
- preserve the reserved entry at index `0`
- keep caller-owned payload objects alive
- preserve hotkey normalization
- check negative return-value semantics
- update both Off-page and Back-page drawing where required