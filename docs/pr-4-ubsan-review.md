# PR #4 UBSan review

## Scope

Reviewed the 27 commits and 35 changed files in `origin/main...9e2e5b8`.
The PR is checked out locally as `pr-4`. It fixes UBSan findings; it does
not enable UBSan in CMake or add a UBSan preset.

## Findings and disposition

### Unaligned access: keep the safety, centralize the implementation

The PR correctly distinguishes many packed-member accesses from accesses
through ordinary scalar pointers. A compiler-visible packed member can be
read or assigned directly using the project's packing extension. Passing its
address to code that dereferences an ordinary pointer, or binding a scalar
reference to it, loses that protection. These are not equivalent operations.

Added `src/unaligned.h` with `unaligned::Load<T>` and `Store<T>`. They accept
untyped addresses, copy trivially copyable values, and require sufficient
readable/writable storage. They use native byte order, not serialized byte
order. Fixed-size `memcpy` belongs inside these helpers, not repeated in
gameplay or rendering code. They do not provide bounds checking.

Applied this implementation to POP/SETTLER Load/Store, animation frame offsets,
palette entries, packed list lookup, config values, and savegame array access.
Savegame readers/writers still handle their existing serialization format;
the new helpers only access the in-memory values.

Config integer access was fixed by the PR, but string-pointer targets still
used typed pointer dereferences. Those now use the same safe Load/Store path.

### Rendering: no new per-pixel copy storm

The PR's rendering copies are palette-entry operations and frame-offset
lookups. It does not introduce `memcpy` into sprite pixel loops. Replacing
these with dereferenced `uint32_t*` casts would reintroduce alignment and
aliasing problems.

An isolated GCC 13 x86-64 `-O2` code-generation check compiles each new
`uint32_t` Load/Store helper to one `movl`, with no `memcpy` call. This is
evidence for the helper implementation on that target, not a frame-time
benchmark or a guarantee for every compiler/debug configuration.

Existing screen-background dword accesses in
`animate::Draw_Screen_Background_Off` remain a separate audit candidate.
Any future span-copy optimization must preserve clipping, run boundaries,
transparent zero pixels, and glass-remapping semantics. No pixel-loop
optimization or behavioral change was made here.

### Population and settlers: separate memory access from field semantics

Replaced raw population race, profession, original-owner, assignment, and
conquest masks in the PR-touched AI, colony-calculation, and colony-movement
files with POP helpers. This includes homeworld job assignment, growth,
assimilation, surrender, android creation, sorting, and population UI text.
Multi-field construction uses local values before storing the completed word.

SETTLER already encapsulates its field masks. Its Load/Store implementation
now shares the unaligned primitive. Legacy-format masks remain intentionally
inside `From_Legacy`; current-format getters cannot decode that different
layout. No packed structure layout or saved representation was changed.

The remaining typed population pointers passed to existing APIs are not
dereferenced by these changes; their consumers use POP::Load/Store. Avoid
adding ordinary pointer dereferences or scalar references in future edits.

### Other PR changes

- Aligned local outputs for ship coordinates and randomized leader skills
  avoid passing misaligned scalar output pointers. Their control flow is
  preserved.
- Returning the rotating-planet segment removes a packed pointer-output
  parameter without changing the loaded object.
- Direct star-slot/production indexing preserves compiler knowledge of
  packing and is preferable to unnecessary byte copies.
- The build-queue refund change retains its low-32-bit arithmetic without
  binding a reference to packed player money.
- The static research-entry array fixes screen-pool alignment, but assumes
  the research screen is non-reentrant. A future aligned arena allocation
  API would address the allocator limitation more generally; no arena
  redesign is included here.
- `Player_Tech_Field_Status_` preserves the documented legacy field `-1`
  read from the preceding player-record byte. It is compatibility behavior,
  not a general bounds-validation solution. Deciding a new fieldless-tech
  policy should be separate from this sanitizer refactor and have explicit
  regression tests.
- The six changed test files retain their predicates and expected values.
  Parenthesized comparisons stop doctest from binding references to packed
  members, at the cost of less detailed operand diagnostics. Heap-allocated
  save states and byte-filled sentinel arrays are reasonable fixes.

## Validation

Configured `out/build/linux-ubsan` with Debug, tests enabled, and
`-fsanitize=undefined -fno-sanitize-recover=all` in C/C++ flags plus
`-fsanitize=undefined` in executable linker flags. The full build passed.

New tests cover native-byte scalar and pointer access, adjacent storage,
packed POP/SETTLER setters at every natural alignment offset, reserved-bit
preservation, the settler transit sentinel, and unaligned animation frame
tables including their terminal offset. They use existing test targets.

All seven focused UBSan suites passed: `test_pop_helpers`,
`test_animation_frame`, `test_settler`, `test_build_queue`, `test_colonize`,
`test_savegame`, and `test_config_loader`.

The complete UBSan run passed 71 of 99 suites. The 28 failures stop at:

- 26 packed-reference errors in tests not updated by this PR, including
  AI/colony tests, config bindings, networking, and combat tests.
- `tests/fakes/fake_events_runtime.cpp:27`, an unchanged packed-reference
  error in a test fake.
- `vendors/lua/lstring.c:294`, a misaligned userdata access. The underlying
  project allocator in `src/lua/lua_runtime.cpp` returns `new_header + 1`
  after a single-`size_t` allocation header. On this target the 8-byte
  displacement breaks Lua's required 16-byte alignment. That allocator is
  unchanged by the PR; the vendor access is the detection site, not the
  root cause.

The four affected AI/colony suites (`test_aidudes`,
`test_colony_production_helpers`, `test_colony_pop_growth`, and
`test_colony_replicators`) also passed in the normal Debug build. Full UBSan
success is not claimed. Logs are in `/tmp/orion2re-pr4-ctest.log` and
`/tmp/orion2re-pr4-full-build.log` for this session.

`git diff --check`, changed-production-line clang-format checks, and the new
helper's clang-format check passed. Added test blocks follow their existing
suite indentation rather than reformatting the surrounding files.

## Recommended follow-ups

1. Correct the Lua allocator header/payload alignment, including realloc
   and accounting paths, and add an alignment regression test.
2. Finish the packed-reference cleanup in the remaining tests and fake;
   rerun UBSan to expose any failures currently hidden behind the first one.
3. Add a reproducible UBSan preset/CI job once those failures are triaged.
4. Add direct misaligned config-target and palette-runtime tests. Existing
   integration tests plus primitive tests do not fully cover those paths.
5. Audit remaining rendering dword casts and screen-pool alignment
   separately; measure optimized builds before changing hot loops.

These follow-ups were not implemented as unrelated fixes. A concurrent
working-tree change to `src/main.cpp` was observed during validation and
left untouched.