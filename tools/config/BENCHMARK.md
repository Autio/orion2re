# Configuration startup and save-load benchmark

`config-load-benchmark` links the production game objects. It initializes the
VFS and configuration, allocates native game storage, reads a save into memory,
and calls `SAVEGAME::Read_Game`. No fake configuration or Lua implementation is
linked. It never writes the supplied save.

After loading, it also measures production saving, synchronized configuration
snapshot preparation, network configuration staging/finalization, and host Lua
reconstruction. All save output stays in memory.

## Linux

```sh
cmake --build out/build/Linux/linux-debug-asan-tests --target config-load-benchmark -j 4
ASAN_OPTIONS=detect_leaks=0:abort_on_error=1 \
  out/build/Linux/linux-debug-asan-tests/config-load-benchmark \
  /path/to/game /path/to/game/SAVE10.GAM 3
```

## MSVC Win32 Debug-ASAN

Run from a Windows command prompt:

```bat
tools\config\build_benchmark_win32.cmd C:\projects\moo2 C:\projects\moo2\SAVE10.GAM 3
```

The script discovers Visual Studio, selects the x86 compiler, and builds in
`%TEMP%\orion2re-incremental-win32`. It does not deploy an executable or overwrite
the installed package archives. The third argument is the number of loads in
one process; use fresh processes for startup samples.

Building the full game target from a WSL UNC source path can fail in the existing
archive-packaging commands (`cmd.exe` cannot use a UNC working directory). The
benchmark target does not package assets and builds from that path. Use a normal
Windows drive path for full-game packaging and interactive verification.

Set `ORION2RE_CONFIG_PROFILE=1` to print nested phase timings. Leave it unset for
reported timing samples. Leak detection is disabled in the Linux harness because
native game storage normally lives until process exit.

## Boundaries

- `startup_ms`: `Load_Startup_Config`, including default capture, parsing,
  package setup, Lua setup, identity generation, and native publication.
- `main_to_config_ms`: harness entry through startup configuration completion,
  including VFS initialization but excluding ASAN process initialization.
- `load_ms`: production `Read_Game`, including native rollback serialization,
  native/config/Lua decoding, configuration transition, and Lua restoration.
- `repeat_load_ms`: the same production load with an already loaded native game.

Window creation, later startup, save-file reading, initial native storage
allocation, and post-load UI/resource refresh are not included in `load_ms`.
These numbers are not full time-to-menu or time-to-playable-game measurements.
Require exit status zero, `load_ok=1`, and `remaining=0` for every sample.

## Measured results

Installed v150i and v150 1.50.26, SAVE10.GAM (238,581 bytes), warm filesystem
caches, five fresh processes after a warmup:

| Build | Startup median | First load median | Repeated load median |
| --- | ---: | ---: | ---: |
| MSVC 14.51 x86 Debug-ASAN | 1,800 ms | 446 ms | 409 ms |
| GCC 13 x86-64 Linux Debug-ASAN | 644 ms | 162 ms | Not included in paired run |

The Linux comparison alternated before/after processes using the identical
harness and installed data. Before these incremental changes (commit
854e4ee8, including the first sparse-storage phase), medians were 1,667 ms
startup and 1,741 ms load. The Windows results are current-code measurements,
not a matched measurement of the earlier interactive Windows build.

Normal operation uses version-2 sparse configuration identities. The existing
version-1 canonical identity API is retained for explicit compatibility use.
Old saves remain readable: ordinary Lua save compatibility checks do not require
the old synchronized-configuration hash. Network sessions require matching
configuration identities; mixed identity versions do not compare equal.

The fast path retains mounts only when required package versions/order, source
stamps, manifests, and included config contents match. Optional dependencies
use normal resolution. Lua runtimes are recreated intentionally to reset
transient state; source fingerprints and saved-state restoration still run.
Native save rollback serialization and string-arena lifetime management remain
intact.

## Saving and network-consumer measurements

Five fresh processes after one warmup, Debug-ASAN, the same installed packages
and SAVE10.GAM. Medians in milliseconds:

| Operation | Linux x86-64 | MSVC Win32 |
| --- | ---: | ---: |
| Save, config snapshot/identity cache miss | 26.4 | 72.9 |
| Save, config snapshot/identity cache hit | 4.3 | 6.0 |
| Save after changing a synchronized setting | 22.6 | 71.4 |
| Network config snapshot, cache miss | 13.4 | 65.6 |
| Network config snapshot, cache hit | 0.001 | 0.009 |
| Network configuration staging | 178.6 | 430.1 |
| Network configuration finalization | 57.0 | 195.6 |
| Host Lua reconstruction | 160.4 | 348.7 |

Saving times the complete `SAVEGAME::Write_Game`, including native serialization,
Lua state capture, config snapshot writing, and identity preparation. It excludes
output-buffer allocation, disk writes, compression outside this API, and UI.
The harness compares repeated save bytes, changes `race.picks.starting` through
the active write API, requires the next save to differ, then reloads it and checks
the changed value and random seed. Input files are never modified.

Network timings exercise the production configuration APIs locally, not socket
transport, latency, native game-state transfer, or a complete multiplayer match.
Lua payload preparation is outside the staging/finalization timers. Finalization
includes Lua snapshot decoding/restoration and configuration commit. Host
reconstruction includes Lua capture, package staging, identity verification,
runtime recreation, and restoration.

Network finalization and host reconstruction use authoritative sparse active
configuration instead of full native configuration captures. Native writes must
be recorded through `Record_Active_Native_Config_Write`. Network transactions
still resolve/stage VFS mounts to preserve isolation; they do not use the
immediate save-load mount-reuse path.