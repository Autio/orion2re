#pragma once

namespace exitm {
    /** Clears game state, shows a fatal-exit message, and terminates the process.
     *  @param message Message text shown to the user before exit.
     */
    [[noreturn]] void __cdecl Exit_With_Message(const char* message);

    /** Requests a successful process exit after showing an informational message.
     *  @param message Message text shown to the user before exit.
     */
    [[noreturn]] void __cdecl Exit_Normally(const char* message);

    /** Presents a stored nonempty exit message with styling appropriate to its status.
     *  @param window_handle Main-thread window used as the dialog parent.
     */
    void __cdecl Present_Exit_Message(void* window_handle);

    /** Forwards to Exit_With_Message.
     *  @param message Message text shown to the user before exit.
     */
    [[noreturn]] void __cdecl Exit_With_message2(const char* message);
} // namespace exitm