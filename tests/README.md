# Host game tests

Run from the repository root with an existing host GCC and Python (standard
library only):

```text
C:/Espressif/tools/python/python.exe tests/run_game_tests.py --cc C:/msys64/mingw64/bin/gcc.exe
```

The runner prepends the compiler directory to its child processes' PATH, prints
exact compile/run commands, uses temporary executables, and removes them even on
failure. It runs `-O0` and `-O2` with C11 and
`-Wall -Wextra -Wpedantic -Werror`. It never edits production sources.

- `snake_test.c`: independent logic tests using real `App/app/snake.c`.
- `game_session_test.c`: real `App/app/game.c` + `piano.c` + `snake.c`, with VOX on/off.
  Covers entry rejection without side effects, PA/RX/audio shutdown, MENU
  non-fallthrough, three consecutive physical-release samples, reset on PTT/
  side/matrix keys, debounced 2/4/6/8 input, paused immobility, terminal restart,
  EXIT game -> list -> menu, forced GUI departure, idempotent cleanup, held-PTT
  exit TX blocking, serial-injection rejection as release evidence, and exit API
  behavior for low battery/reduced service/power save/configuration. Deferred
  VOX cases verify that three physical-release samples unblock keys/TX without
  restoring RF in INCOMING/TRANSMIT/RECEIVE, then the next FOREGROUND call
  restores once, clears VOX state, and never repeats the restoration. Even when
  already FOREGROUND, restoration waits until the call after barrier completion.
  Piano coverage adds UP/DOWN two-game list wrap, held MENU entry isolation,
  all twelve key mappings and exact integer Hz, invalid notes/keys, single-note
  holds without replay, first-sample release/change mute, stable-sample debounce
  including bouncing transitions, and MENU/UP/DOWN/side/PTT mute without TX.
  Checks include PA/RX disabled at every tone start, audio/tone shutdown, REG50/71
  snapshot restoration, repeated Open preserving the snapshot, reopened sessions
  taking fresh snapshots, idle Close and closed-session API idempotence, EXIT
  to list, every compiled GUI destination, battery/configuration GAME_Leave
  cleanup, and unchanged snake state throughout a fresh piano session.
- `game_ui_test.c`: real `App/ui/game.c` + `game.c` + `piano.c` + `snake.c`, with Chinese
  enabled/disabled. Its independently runnable `language`, `list`, `board`, and
  `piano` cases cover language/font fallback, battery status, both list labels
  and exact selection marker positions, all twelve piano note/frequency displays
  and idle/help text in both languages, three-cell
  snake, bottom-right food, full 240-cell ring, paused/lost/won overlays, every
  DrawPixel x/y bound, and framebuffer/status canaries. Text stubs use the real
  helper's 6-column glyph + 1-column spacing and leading-column offset; invalid
  bounds fail rather than silently clipping. Expected corrected UI positions
  are `M:Start E:Back` and `M:Pause` at x=76.

`game_stubs/common.h` shares minimal hardware declarations and includes the real
keyboard/display/function enums. One-line wrappers shadow firmware dependencies;
`runtime.h` shares test-only hardware call recording, a register bank, per-register
read/write counters, tone frequency history, stop counts and audio/mute state.
Real game state transitions, piano mapping/debounce and snake logic are never
duplicated in stubs.

## Scope limits

These are host API/geometry tests, not MCU/RF/display validation. The GUI stub
models the dispatcher's call to `GAME_OnLeave`; it does not compile `ui/ui.c`.
The menu stub verifies the restoration callback and saved-index contract, not
`menu.c`'s internal selection-history implementation. Battery/config detection
belongs to `app.c` and is not compiled here; tests simulate those callers using
`GAME_Leave`. The radio stub models VOX suppression while TX-blocked, not actual
full register restoration, RF safety, or timing. Piano driver stubs record the
requested Hz and model tone/mute register effects; they do not compile the real
BK driver, encode the chip frequency value, or prove audible pitch/latency.
REG50/71 restoration checks exercise real piano snapshot/cleanup code against
that register model. Font stubs validate geometry and
language fallback, not glyph artwork, flash contents, or Chinese font decoding.
No firmware Flash/RAM deltas or hardware stack limits are measured.
