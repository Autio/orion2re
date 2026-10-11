# Threading and SDL

The main game runtime uses two cooperating threads. This arrangement is inherited from the Windows `orion95.exe` structure, but the SDL port makes the ownership boundary explicit and adds synchronized handoffs for frames, input, cursor state, and shutdown.

The term "two-thread model" refers to the platform and game loops. Optional services such as audio or an embedded multiplayer router may use additional threads without changing this core division.

## Original orion95 arrangement

The restored original Windows entry path split work as follows:

- the Win32 main thread registered and created the window, initialized DirectDraw, started the game thread, and ran the `GetMessage` and `DispatchMessage` loop
- the game thread entered `MOX2::main2()`, initialized the game, ran screen control, and performed the legacy software drawing flow

That structure kept the operating-system window loop responsive while the game used long-running modal screen and input loops. It also meant that game-side code could initiate presentation through the Windows and DirectDraw platform layer.

The SDL port preserves the useful division but does not allow the worker to own SDL window events or renderer presentation. Those operations stay together on the thread running `platform::Run_Main_Loop()`.

## Current thread roles

| Thread | Responsibilities |
| --- | --- |
| Platform and presenter thread | SDL initialization, window and renderer ownership, event polling, coordinate mapping, debug overlay input, frame presentation, and final teardown |
| Game worker thread | `MOX2::main2()`, initialization, screen control, game state, software rendering into game pages, legacy fields and input processing, sound and network cleanup |

`platform::Run_Main_Loop()` initializes SDL, the window, renderer, shared synchronization objects, software video pages, and the debug overlay before creating the worker with `SDL_CreateThread(MOX2::thunk_main2, ...)`.

The platform thread then repeats three operations:

1. poll and translate pending SDL events
2. claim and present a published game frame when one is available
3. delay briefly before polling again

The worker runs the existing game entry point and screen loops. There is no separate "worker ready" handshake; all platform-owned resources needed by normal game initialization exist before the thread is created.

## Frame handoff

The game still renders indexed pixels through its legacy page model, but SDL presentation uses additional ownership states and copies.

The important buffers are:

- `video::g_page_back`: stable software background used by legacy redraw logic
- `video::g_page_off`: working page into which the game draws the next completed frame
- `video::g_page_published`: immutable handoff snapshot while the presenter claims it
- `platform::g_present_surface`: presenter-owned copy used for palette conversion, cursor composition, scaling, and texture upload

The Back page is not exchanged with the presenter. It remains part of the legacy differential-redraw model.

### Publishing from the game thread

`video::Toggle_Pages()` completes field drawing, palette refresh, mouse state, and debug-overlay frame construction. It then calls `video::Publish_Off_Page()` before stencil restoration can modify the Off page for the next incremental frame.

`Publish_Off_Page()`:

1. locks `g_off_page_mutex`
2. waits on `g_off_page_condition` until the state is `OFF_PAGE_WRITABLE`
3. copies the Off page into `g_page_published`
4. snapshots the current palette
5. changes the state to `OFF_PAGE_PUBLISHED`
6. increments `g_off_page_generation`

A completed published frame is not overwritten. If the previous snapshot has not been claimed and copied, the game thread waits instead of dropping it.

### Claiming on the presenter thread

The platform loop calls `video::Try_Begin_Off_Page_Present()`. When a new generation is available, that function changes the state to `OFF_PAGE_PRESENTING` and records the generation being consumed.

`platform::Update_Active_Screen_Frame_()` copies the published indexed pixels and submitted palette into presenter-owned storage, then calls `video::End_Off_Page_Present()`. Ending the claim returns the state to `OFF_PAGE_WRITABLE` and signals a waiting publisher.

The expensive presentation work happens after this copy:

- apply the submitted palette to the indexed surface
- convert to ARGB
- composite the software cursor
- run the selected scaler
- update the SDL texture
- render the debug overlay
- call `SDL_RenderPresent()`

Releasing the published page before those steps lets the worker start constructing and publishing later frames without accessing SDL renderer objects.

Window exposure, cursor movement, scaler changes, and similar presentation-only changes set an atomic refresh request. These requests can be coalesced because they redraw the latest presenter-owned frame; they are separate from the generation-controlled game-frame handoff.

## Input handoff

Only the platform thread polls SDL events. It maps window coordinates into the logical game resolution, tracks left and right mouse buttons, translates SDL key values into the legacy key codes expected by the game, and gives the debug overlay the first opportunity to capture input.

Events intended for the game are written to a mutex-protected ring in `platform.cpp`. The queue has 256 slots, with one slot reserved to distinguish full and empty states.

The producer behavior is:

- adjacent mouse events with the same button state are coalesced by updating the most recent coordinates
- key events are preserved individually
- when the queue is full, the oldest event is discarded
- SDL-generated key repeats are ignored
- losing focus enqueues a mouse event with no buttons held

The game thread calls `platform::Pump_Game_Input_Queue_()` through normal mouse and keyboard query functions. The pump copies pending events into local storage while holding the mutex, unlocks it, and only then dispatches to `mouse::User_Mouse_Handler()` or `key::Push_Key_Code()`.

Dispatching after the unlock is important. Legacy handlers can update cursor state, query other input functions, or enter field logic; none of that work should happen while the producer queue is locked.

After dispatch, the existing field and screen code sees the familiar mouse globals and legacy keyboard ring. The cross-thread queue is therefore a platform adapter, not a replacement for the game's input model.

## Cursor handoff

The software cursor has its own handoff because its image can change without a new game frame.

The game thread selects the cursor shape through the legacy mouse-list logic. `platform::Update_Cursor_Image_()` copies the selected 24 by 24 indexed image and publishes its offset, shape, enabled state, and generation while holding `g_present_cursor_mutex`.

Before composition, the presenter calls `Apply_Pending_Cursor_State_()`. If the generation changed, it copies the pending pixels and metadata into presenter-owned storage. Logical cursor coordinates are shared separately through SDL atomics, and movement requests a redraw of the latest frame.

This keeps cursor selection in the game while ensuring that the presenter never reads an image while the worker is rewriting it.

## Exit and error handoff

Exit state is shared through atomics plus a mutex and condition used to publish the status and message. SDL quit or window-close events call `platform::Request_Exit_()`. Game polling paths call `platform::Check_Exit_()`, which raises `process_exit_requested` on the worker.

`MOX2::thunk_main2()` contains the worker boundary. It catches the normal exit request and unexpected exceptions, converts unexpected failures into a fatal exit request, cleans up sound and networking, and marks the game thread as exited.

The platform loop continues servicing events and frames until that exit marker is visible. It then requests exit if necessary, joins the worker with `SDL_WaitThread()`, presents any exit message, and destroys renderer, window, mutex, video, and SDL resources on the platform thread.

## Rules for changes

When adding platform or rendering behavior:

- keep SDL window, renderer, texture, and event operations on the platform thread
- keep mutable gameplay state and legacy screen logic on the game thread
- transfer snapshots or queued events instead of exposing a buffer to both threads at once
- keep queue locks short and do not invoke legacy callbacks while holding them
- preserve the distinction between the Back, Off, published, and presenter-owned pages
- pair every blocking publication or worker lifecycle change with a wakeup and shutdown path

The useful invariant is ownership rather than timing: at every handoff, one thread owns the mutable object and the other sees either an immutable snapshot or a queued value.