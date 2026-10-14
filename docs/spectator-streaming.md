# Streaming spectator prototype

This fork adds optional per-empire diplomacy journals and a local browser overlay.
The existing AI still makes the decisions. The journal records evaluations,
selected wars, and end-of-turn treaty/trade/research changes. It does not invent
long-term plans, and it does not yet cover economic or military planning.

## Preview without game data

From the repository root:

```powershell
python tools/stream/server.py --demo
```

Open http://127.0.0.1:8765/ for the dashboard. The sample data is prominently
marked as illustrative. It is not a simulation and playback controls are disabled.

## Live game

Build the engine using the repository instructions. Original MOO2 game data is
required and is not included in this fork. Keep it outside the repository.

Create a broadcast directory and start the companion server:

```powershell
New-Item -ItemType Directory -Force out/stream
python tools/stream/server.py --journal out/stream/journal.jsonl --control out/stream/control.txt
```

In another PowerShell terminal, set absolute paths before launching the engine:

```powershell
$env:ORION2RE_JOURNAL = (Join-Path (Get-Location) 'out/stream/journal.jsonl')
$env:ORION2RE_STREAM_CONTROL = (Join-Path (Get-Location) 'out/stream/control.txt')
# Launch your built engine with its normal original-game-data setup.
```

Start or load a **single-player** game. On the galaxy screen, use the existing
`ALLAI` cheat sequence to hand the human empire to the AI. This starts a new
journal session. The conversion uses the engine's existing AI setup, including
replacing colony queues. Use a dedicated spectator save.

The dashboard starts paused. Choose a turn spacing and click **Run simulation**.
**Pause** stops automatic advancement at the next galaxy-screen boundary; it does
not interrupt a turn already executing. Normal game input has priority. Manual
Next Turn also works. The interval is a minimum gap after a published turn, not a
guaranteed simulation rate. No control path means manual turns only.

Re-enter ALLAI after loading or starting a different game to create a fresh
session. Earlier-date loads are also detected at the next turn. Journals append
to JSONL for archival purposes; use a new path for each broadcast if preferred.

## Stream composition

Capture the game window normally and add a browser source alongside it:

```text
http://127.0.0.1:8765/?overlay=1&empire=0
```

Use empire IDs 0 through 7 for fixed per-player sources. Each transparent source
shows the selected empire's latest five entries; a 600 × 1080 source is a useful
starting size. Crop or resize to fit your scene. The regular dashboard has all
empires, a selector, controls, and an optional engine-evidence view. Multiple
browser sources can follow different empires simultaneously.

The server binds only to loopback. Stream it through your broadcasting software;
do not expose the development server to the internet. No network account, API
key, LLM, or third-party web assets are needed. A missing or malformed control
file pauses advancement. Stop the server normally to write a paused control file;
if it is forcibly killed, the game can retain the last playback setting.

## Data and limitations

- `schema: 1` JSONL contains a session ID, the processed stardate, an empire roster,
  and events tagged `evaluation`, `decision`, or `outcome`.
- Every event has its empire, target, readable text, and engine evidence. Initial
  proposal scores are explicitly not presented as completed agreements.
- Roster statistics are omniscient broadcast information, separate from the AI's
  decision text. Diplomatic text only uses data from the evaluated decision.
- Journal collection makes no random draws and does not change save structures.
  It is enabled only through ALLAI in single-player with `ORION2RE_JOURNAL` set.
- Up to 256 events are collected per turn; overflow is counted. The dashboard
  retains 60 entries per empire while the append-only file preserves all records.
- Legacy names are serialized byte-for-byte as Unicode U+0000–U+00FF. Non-Latin
  localized names need a future game-codepage conversion.
- Existing ALLAI mode skips council meetings. The prototype stops autoplay when
  one empire remains, Antares is captured, or a council winner is already set.
- The existing viewpoint-switching and map-reveal cheats remain available.
  This is not yet a dedicated spectator camera or a complete unattended-game
  implementation. Combat/event interruption behavior requires a live-data test.
- Logging failures print to stderr and disable autoplay. The overlay flags lost
  connection, stale journals, omitted events, and malformed records.

## Next milestones

1. Validate an unattended match with original data; address combat, events,
   elimination of the viewed empire, new-game/load resets, and spectator camera.
2. Record technology trades, surrender, military orders, and economic decisions
   with their actual decision inputs; improve treaty outcome wording.
3. Add a persistent strategic-goal layer and journal explicit goal changes.
4. Add match setup, replay, highlights, and synchronized scene switching.

## Validation

```powershell
python -m unittest discover -s tools/stream -p 'test_*.py'
node --check tools/stream/overlay.js
# With ORION2RE_BUILD_TESTS=ON:
cmake --build <build-directory> --config Debug --target test_spectator_journal test_diplomac test_npcdiplo
ctest --test-dir <build-directory> -C Debug -R 'test_(spectator_journal|diplomac|npcdiplo)' --output-on-failure
```

Before streaming a live match, compare a fixed-save/manual-turn run with and
without journaling, verify repeated browser polls do not duplicate entries, test
pause/resume, and ensure the selected viewpoint survives elimination. Original
game data is needed for that integration pass.

Initial validation on Windows x64 / MSVC (2026-10-14): the complete Debug game
build succeeded; all three journal unit cases (eight assertions), seven Python
tests, and JavaScript syntax validation passed. The browser preview was checked
for rendering, empire selection, evidence display, and the transparent overlay
route. The `test_diplomac` and `test_npcdiplo` targets failed to link unresolved
game-runtime symbols in their fake runtimes, so those suites have not passed.
Trying `test_diplomac` in Release also failed. No live match has been tested yet.
