# Streaming spectator prototype

This fork adds optional per-empire diplomacy journals and a local browser overlay.
The existing AI still makes the decisions. The journal records diplomacy,
named colony events, space engagements, and acquired technologies. It does not
invent long-term plans, and it does not yet explain economic or military planning.

## Watching AI battles and following research

Launch with `-WatchBattles` to let single-player ALLAI matches display AI-versus-AI
engagements using the existing tactical engine and AI ship commands:

```powershell
.\tools\stream\start.ps1 -GameDataDirectory 'C:\Games\Master of Orion 2' -WatchBattles
```

The match must use tactical combat in its game settings. Strategic-mode games,
multiplayer, and games with living human players keep their existing routing.
Without the option, AI-versus-AI engagements retain strategic resolution.
Equivalent environment setting: `ORION2RE_WATCH_BATTLES=1` alongside the journal
environment variables. This changes which combat simulator resolves the battle;
it is not a visual replay of a strategic result. Both sides remain AI-controlled.
The existing combat animation settings control speed. Dashboard Pause stops
future galaxy turns; it does not interrupt an ongoing tactical battle.

New engine builds record colonization/outposts, ownership transfers, colony
destruction versus planet destruction, and battle starts/results with named
locations and participants. Destruction attribution comes from a scoped combat
or post-combat attacker, including Antarans and monsters. Outside that scope the
cause is explicitly unrecorded. Ownership changes are not automatically called
conquests because they may also result from other transfers. Events are currently
published at the end of the galaxy turn, not during individual ship actions.

Each empire card shows its current research application and accumulated research
points. Newly acquired technology applications enter the journal; their source
is not inferred to be research, since trading, capture, and espionage also grant
technologies. Repeated hyper-advanced levels and full interactive research-screen
viewing are not yet exposed. Older journals remain readable, but cannot recover
these newly instrumented events retroactively.

Rebuild and relaunch the game to activate engine changes. Updating the dashboard
alone cannot add hooks to an already-running executable.

## Journal styles

Choose **Empire chronicle** in the dashboard for longer first-person entries,
personality-based flavor, and a running account of expansion, losses, and recorded
wars. **Engine log** retains the original concise events. The dashboard remembers
the selected style; an OBS source can select it explicitly with
`/?overlay=1&empire=0&style=chronicle`.

Chronicles replay the existing journal, so enabling them does not restart the
match. Prose is deterministic and generated locally without an LLM or API key.
It is labeled dramatization, not canonical lore or access to hidden AI thoughts.
History references only earlier records. Snapshot changes supply additional
milestones, but cannot identify an attacker, planet, or battle cause. Enable
evidence to inspect the recorded inputs; raw engine journals remain unchanged.

When restarting only the companion server during a match, pass
`--preserve-control` to keep the current playback interval. Normal startup and
graceful shutdown still pause autoplay.

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

On Windows, the launcher prepares a separate runtime beside the repository,
copies the original LBX/SMK assets and built engine, supplies `ORIONCD.INI`, and
starts the game plus the live dashboard on port 8766:

```powershell
.\tools\stream\start.ps1 -GameDataDirectory 'C:\Games\Master of Orion 2'
```

Select the folder containing `FONTS.LBX` and the other LBX files. The launcher
expects the build under `out/build/stream`, preserves an existing runtime config,
and keeps test saves separate from the original installation. Close the test
game before running the launcher again. `-PrepareOnly` prepares files without
starting either process; `-RuntimeDirectory`, `-GameExecutable`, and `-Port`
override their defaults. The server process ID is printed for later shutdown.

For manual setup or other platforms:

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
`ALLAI` cheat sequence (hold Alt while typing ALLAI) to hand the human empire to the AI. This starts a new
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
  and events tagged `evaluation`, `decision`, `outcome`, `colony`, `destruction`,
  `battle`, or `research`. The roster optionally includes `research_project` and
  `research_accumulated`; older records without them remain supported.
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
Trying `test_diplomac` in Release also failed.

The first original-data integration test successfully reached the main menu,
initialized a five-empire single-player ALLAI game, and delivered live snapshots
to the dashboard. Autoplay advanced turns; a pause check held stardate 35013
across repeated reads and resume continued the game. The launcher passed parsing,
runtime-preparation, and existing-config preservation checks. A complete match,
combat interruption behavior, and elimination handling remain unverified.

The live run subsequently reached stardate 35137 with no journal parse errors.
At 35111, the Meklar's war-selection decision (`reason=23`) and the resulting
Meklar/Klackon treaty changes (`0 -> 4`) appeared in the actual browser journal.
The simulation continued afterward. The dashboard's Pause button stopped it at
35137, verified through repeated journal reads. This validates the real
engine-to-browser event path, not just sample data.

Chronicle validation: all 16 Python tests and JavaScript syntax validation pass.
Tests cover deterministic replay, past-only historical context, war duration,
session resets, preservation of raw events, and elimination after a population
of one. The live dashboard was checked with the existing match and playback
remained at its five-second interval during the companion-server update.

Battle/event extension validation: the complete Windows x64 Debug game builds;
four journal unit cases (12 assertions), 18 Python tests, JavaScript syntax, and
PowerShell launcher parsing pass. Named events and research cards were verified
in the browser with explicitly illustrative data. A complete watched tactical
battle has not yet been live-tested. The previously running game was left intact
and still needs a relaunch to use this engine build.
