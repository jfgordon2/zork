# Code review — November 19, 2026

A review of the whole repository: C sources, headers, assembly declarations, build
system, Python tooling, and tests. Findings are ordered by severity. `make test` passes
(all three winning routes; re-confirmed on re-check) and the shipped build is
byte-verified against source art, so this is a review of latent and edge-case issues,
not of a broken tree.

## Scope and method

Read all ~9,000 lines of `src/`, `tools/`, `tests/`, the Makefile, and Dockerfile.
Ran `make test`, extra clang warnings (`-Wshadow -Wconversion -Wsign-conversion`),
and small host probes for suspected edge cases. Note: the tree is being edited by
another session; findings below describe the state at 08:55 on November 19.

## Findings

### C-1 · Fresh-clone sprite import fails — P2 (tooling bug)

`tools/import_zork3_art.py:40-41` opens `build/z3-item-import.lock` for append without
creating `build/` first. `build/` is gitignored, so on a fresh clone the file open
raises `FileNotFoundError` and no Zork III item sprite can be imported. The Zork II
sibling does the right thing (`tools/import_zork2_art.py:37`:
`lock_path.parent.mkdir(parents=True, exist_ok=True)`).

**Fix:** add the same `lock_path.parent.mkdir(parents=True, exist_ok=True)` before
opening the lock. (Also note the two tools keep separate lock files, so concurrent
Z2/Z3 imports can write `assets/items/frames.json` at the same time; a shared lock
would close that race.)

### C-2 · `wrap_line` makes no progress for too-small widths — P3 (latent robustness)

`src/video.c:216-229`: if the first character's advance exceeds `width`, the loop
never executes, `end == text`, and the function returns the same pointer. `paragraph`
(`video.c:232`) is bounded by its row count, so it silently draws nothing. Three
callers in `src/trilogy.c` (lines 256, 381, 758) pass a hardcoded width of 224, so
no live hang today.

`paragraph(12, 4, available_name, 1, name, PAPER)` at `trilogy.c:194` passes a
computed width clamped only to `>= 0`, but a zero width is not reachable with
current data: `draw_header` drops the status when
`name + path + status + 8 > 221` (`trilogy.c:170`), and direction labels are 5px
each, so even all 12 exits (104px) leave at least 111px for the name. With the status
shown and exits present, the remaining width is always at least the name's width + 2.

A narrower real edge case does exist: with the status shown and **no** exits, the
available width can be up to 5px less than the name's width, so a name in that
length window loses its last word (or, if it is a single word, is cut mid-word).
`src/main.c:74` (White House demo build) uses the same computed-width pattern.

I verified with a host probe that `wrap_line(t, 0, &end)` returns `end == t`.

**Fix:** in `wrap_line`, always consume at least one character when `end == text`
(and stop at the newline). Separately, make the no-exits status check in
`draw_header` reserve the same width that `available_name` actually uses.

### C-3 · Re-run the symbol/selection cross-check after the art pass settles — P3 (hygiene)

The mirror-hall rename (`z3_mirror_hall_v3` → `z3_mirror_hall_open`) was accompanied
by the matching source rename: `src/chapter.c` declares both
`art_z3_mirror_hall_closed` and `art_z3_mirror_hall_open` and selects between them on
the panel state (`chapter.c:384`).

**Re-check status:** a cross-check of `art_z3_*` symbols across `src/*.c`, `src/*.S`,
the Makefile, and `tests/*.c` stubs found no stale or dangling Zork III symbols.
`z3_guardian_hallway_v3` (room 113) is still the live selection, not superseded art.
Nothing to clean up at present.

**Fix:** re-run the PNG/symbol/selection three-way cross-check after any further art
pass, and remove stale externs, FLOOR rows, Makefile entries, and test stubs.

### C-4 · No bounds check on `action_count` vs `MAX_ACTIONS` — P3 (defensive)

`story_actions` (`src/interaction.c:93`) has a long chain of `ADD(...)` calls in the
PRIMARY group (lines 125-195). Fixed groups are sized correctly today
(`handle` is exactly 18 = `MAX_ACTIONS`), but the PRIMARY path's many conditional
adds are not independently bounded. Adding one more conditional action for a complex
object could overflow `action_list[18]` in `src/trilogy.c:61`. Tests exercise
specific actions by name, not the maximum count.

**Fix:** add `assert(count <= MAX_ACTIONS)` (or a hard cap) at the end of
`story_actions`, and a test that scans every object in every chapter for the peak
count.

### C-5 · Signed conversion warnings in the Z-machine port — P3 (informational)

`port_signed_divide` (`src/zmachine.c:50-55`) takes absolute values of `int16_t`
operands and passes them to an unsigned routine. With `-Wsign-conversion` (included in
clang's `-Wconversion`) clang flags the `?:` operands on line 51 for changing
signedness. The behaviour is correct for all inputs, including `INT16_MIN`: the
operand is promoted to `int` before negation, so `-(-32768)` is 32768 with no
overflow. This is a clarity issue only. Same for `write_word`'s
narrowing casts — intentional byte packing. No fix required unless the port is
reused for other stories.

## What I checked and found correct

- **Build/test:** `make test` passes end-to-end, including all three 400/400-style
  winning routes, save round trips, transcript compression/decompression byte-for-byte,
  the 1,018-state scene/box audit, and dropped-item placement.
- **Build graph:** the Makefile's scene lists, mirror-flip derivation rules, sprite
  header generation, and host/ARM compiler flags are consistent. The PNG/symbol
  inventory was clean at the time of review (265 PNGs, 269 symbols; the extra symbols
  are the four intentional horizontal mirrors built by
  `tools/scene.py --flip-horizontal`). The count changes as the concurrent session
  works.
- **Z-machine core (`src/zmachine.c`):** object enumeration, property/dictionary
  decoding, string streams, resume validation and reload all bound their indices;
  the custom divide/signed-divide is correct for v3 operands.
- **Persistence (`src/save.c`):** dual-bank selection uses signed sequence
  comparison that handles wraparound; the XOR+RLE payload, trail/transcript hashing,
  and Zork III packed visual state all validate lengths and hashes before use; the
  oversized-transcript tail trim has explicit escape-sequence handling.
- **Transcript codec:** fixed 128-token vocabulary, escape handling for high bytes,
  and capacity checks in both directions; the trained corpus matches the shipped
  table (tool reports the same compression as the tests).
- **UI state machine (`src/trilogy.c`):** menu counts, cursor rotation, pagination,
  page offsets, transcript trimming/discarding, dialog and keyboard input all guard
  their indices. The `wrap_line` issue in C-2 is the only exception; it is not
  reachable with current widths, though the no-exits header case can trim a name.
- **Rendering (`src/video.c`, `src/room_visuals.c`):** clipping in
  `draw_masked_pixels` and `tint_rect`, contact-sprite bounds, item-slot arrays
  (max 6 fixed placements per room vs a 96-entry array), and the 256-entry `drawn`
  guard are all safe for the shipped data.
- **Python tools:** `scene.py`, `encounters.py`, `rom.py`, and the two art importers
  do strict size/bounds checks; C-1 is the only gap. `rom.py` writes a correct GBA
  header and checksum.

## Recommended next steps

1. Fix C-1 (one line) — a fresh clone can't import Zork III sprites without it.
2. Fix C-2 in `wrap_line` to guarantee forward progress; it's a two-line guard that
   removes a whole class of future blank-label/hang bugs.
3. Re-run the stale-symbol cross-check after any further art pass (C-3; clean as of
   the re-check).
4. Add the `story_actions` count assertion and peak-count test (C-4).

No blocking correctness bugs were found in the shipped game, save system, or build.

## Resolution — November 19, 2026

- **C-1 fixed.** `tools/import_zork3_art.py` now creates `build/` before opening the
  lock, and uses the same `build/item-import.lock` as the Zork II importer, so
  concurrent Z2/Z3 imports serialize their `frames.json` writes.
- **C-2 fixed.** `wrap_line` always consumes at least one glyph when nothing fits, so
  callers always progress (`check_wrap_progress` in `tests/story_test.c` covers
  width 0). `draw_header` now reserves the 6px path gap in its status check with or
  without exits, so a shown status no longer trims the room name.
- **C-3 no action.** The Zork III cross-check is clean; re-run after future art passes.
- **C-4 fixed.** `story_actions` caps conditional PRIMARY actions at
  `MAX_ACTIONS - 1` so "More actions..." always fits, and `check_action_capacity`
  scans every object, group, and carried state at each story's start, asserting the
  cap is never reached. Current peaks: 7, 8, and 7 of 18.
- **C-5 no action.** Informational only.

`make test` and `make` pass after these changes.
