# Faster primary action menus: render first, probe less, cache the result

Date: 2026-11-23
Status: approved for implementation
Scope: the primary action list opened by **Act** on a room-screen target
(`VERBS` / `PRIMARY`), plus the return path from `More actions…` and category menus.
Category bodies, structured actions, story execution and the focus-box audit keep their
current behavior.

## Intent

Pressing **Act** currently blocks the frame while the game probes the story for every
candidate action. This spec makes the menu appear immediately, keeps it responsive
while availability is checked, avoids repeating work for the same object/state, and
cuts the cost of each probe without changing which actions the story can offer.

## Current state

`explore_pressed` calls `pick_object`, which calls
`choose_action_list(VERBS, PRIMARY)` (`src/trilogy.c`). That function:

1. Starts a probe batch and sets the menu probe budget to 5,000 Z-machine instructions.
2. Builds the static candidate list in `story_actions` from attributes and names
   (Examine, vertical routes, Take/Drop, Open/Close, Read, Light, Eat, actor/weapon
   verbs, climb, `More actions…`). Climbables may run up to three direction probes.
3. Runs `story_filter_defaults_masked` (`src/interaction.c`):
   - probes each static candidate to keep the actions the story accepts,
   - walks the 32-entry `probe_verbs` table filtered by the precomputed per-object
     candidate mask, probing each absent verb to discover story-specific actions,
   - collapses duplicate replies, replaying candidates with up to 24 effect probes to
     compare persistent story state,
   - trims the list to the menu capacity.
4. Ends the probe batch, and only then does `render()` draw the menu.

Every `z_probe` runs the story up to the budget and then runs an `inventory`
normalization pass to settle parser tables. Effect probes run the action again and
compare state.

Measured by the replay audit over 1,003 states:

- masked availability probes: 6,847 total, average ~6.8, maximum 17 per menu;
- duplicate verification: up to 16 effect probes in one menu (24-probe cap);
- the full unmasked sweep would have used 31,024 probes, so the per-object mask is
  already saving roughly 4.5×.

The remaining cost is synchronous, uncached, and repeated on every Act press and every
return from `More actions…`/Orders. The previous frame stays on screen, so the block
reads as a freeze.

## Goals

- Show the primary action menu in the same frame as the Act press.
- Never block the frame for longer than a bounded probe slice while filtering.
- Avoid re-probing when the same object and story state are selected again.
- Reduce the average and worst-case number of story executions per Act.
- Preserve the probe-driven action set and the existing `More actions…` path.
- Stay within the freestanding ARM/EWRAM constraints (~2 KB free).

## Non-goals

- Replacing the category menus, structured actions, or `story_filter_defaults`.
- Probing two-object/tool verbs in the primary list.
- Changing how an action is executed or how the story responds.
- Removing the availability filter entirely.

## Approaches considered

1. **Cache only.** Reuses the filtered list for repeated Act/back-outs, but the first
   Act still blocks. Cheap and safe; does not fix the perceived freeze.
2. **Render first, filter later.** Draw the static candidates immediately and refine
   over subsequent frames. Fixes perceived latency; the final list may shrink while
   the player is looking at it.
3. **Reply-only probes.** Skip the inventory normalization for availability checks;
   keep it for effect probes. Nearly halves the common path.
4. **Cap/remove effect probes in the primary menu.** Removes a large tail (up to 16
   story runs) at the cost of occasionally leaving two actions with the same visible
   reply.
5. **Lazy discovery.** Show static candidates only; run the 32-verb discovery when
   `More actions…` opens or after idle. Fastest, but changes when obscure verbs appear.
6. **Compiled per-state menus.** Use the QA action profiles to build a lookup table.
   Zero probes for covered states, but adds ROM data and a state-signature problem.

Chosen: **1 + 2 + 3 + 4**, with 5 and 6 as measured fallbacks. This keeps the current
correctness model while removing both the freeze and most of its cost.

## Design

### Phase 0 — Instrumentation

- Add a cycle/frame counter around `choose_action_list` and report
  `story_menu_probes_used()` / `story_menu_effect_probes_used()` in a QA build.
- Record, per Act: total frames/cycles, probe count, effect-probe count, and whether
  the menu came from cache.
- Use the existing replay audit numbers as the baseline; no user-facing change.

### Phase 0 implementation notes

- Build the profiling cartridge with `make -B PROFILE=1 zork-trilogy.gba`. The
  Makefile adds `-DPROFILE` to the ARM object flags; normal and `VISUAL_QA` builds are
  unchanged.
- `src/video.c` starts TM0 at prescaler 1024 (~16.384 kHz) around the primary menu
  build and returns the 16-bit tick count. The timer is a no-op outside `PROFILE`.
- `src/trilogy.c` records the last primary build in `MenuProfile`: ticks, availability
  probes (`story_menu_probes_used`), effect probes
  (`story_menu_effect_probes_used`), offered action count, and a cache-hit flag
  (always 0 until Phase 2).
- A PROFILE-only strip at the top of the art band shows
  `menu <probes>p <effects>e <milliseconds>ms <actions>a` while the Verbs/Group drawer
  is open. It is absent from normal builds.
- The generated `zork-trilogy.gba` is the profiling ROM; copy it aside before rebuilding
  the normal cartridge.

### Phase 0 results (PROFILE ROM, mGBA, 2026-11-23)

The profiling run measured complete primary-menu builds on the real cartridge:

| Room | Target | Probes | Effects | Build time |
| --- | --- | ---: | ---: | ---: |
| Z2 Riddle Room | Stone door | 8 | 0 | **3,670 ms** |
| Z3 Museum Entrance | Stone door | 8 | 0 | **3,144 ms** |
| Z3 Prison Cell | Dungeon master | 13 | 13 | 2,243 ms |
| Z1 Living Room | Sword | 5 | 0 | 1,878 ms |
| Z1 Treasure Room | Stiletto | 3 | 0 | 1,652 ms |
| Z2 Gazebo | China teapot | 10 | 2 | 997 ms |
| Z1 Living Room | Brass lantern | 9 | 2 | 529 ms |

The probe count is **not** a good predictor of latency. The lantern spends 11
probes in ~0.5 s, while the sword spends 5 probes in ~1.9 s and the Riddle Room
door spends 8 probes in ~3.7 s. The dominant cost is individual slow probes that
run long story routines (door/knock/open paths, item verbs) up to the 5,000-
instruction budget and then run the inventory normalization pass.

Consequences for the plan:

- A one-frame Act target is not reachable by tuning probe counts alone. Phase 1
  (show the menu immediately, filter in slices) is the primary fix.
- Phase 3 (reply-only availability probes) targets the real cost; it should be
  prioritized with Phase 1 rather than treated as optional.
- Phase 4 (effect cap) helps the Dungeon Master menu (13 effect probes) but does
  nothing for the door menus, which have no effect probes.
- Add per-probe timing before Phase 1 tuning so the slow verbs can be identified
  by name; the total overlay does not show which probe dominates.
- The cache still matters for repeat Act and back-outs, but it does not help the
  first open of a slow menu.

### Implementation status (2026-11-23)

- **Phase 0** complete: `PROFILE=1` build, TM0 timing, probe/effect/build counters,
  and the `menu ...` strip. The profiling script is
  `qa/mgba-room-gallery/profile-primary-actions.lua`.
- **Phase 2 cache** implemented: two MRU entries keyed by chapter/room/object/carried
  plus a generation counter retired by any story command, restore or new game. A
  repeated Act/back-out now shows `menu 0p 0e 0ms ... cache`; alternating between two
  targets in the same room can reuse both entries without re-probing.
- **Phase 3 reply-only probes** implemented: availability probes use
  `z_probe_menu` and skip the inventory normalization; effect probes keep the
  normalized path. Measured first-open improvement: Living Room sword
  **1,878 → 1,172 ms**, Riddle Room stone door **3,670 → 2,247 ms**.
- **Phase 4 effect cap** not enabled: a cap of 4 changed the masked menu count in the
  QA sweep (duplicate actions remained), so it needs a test-policy decision first.
- **Phase 1 sliced filtering** implemented: the static list is drawn on the Act frame,
  then one candidate is probed per frame and one effect probe is replayed per frame
  during the duplicate collapse. The slice is deliberately one candidate rather than
  the 2–4 in the sketch: the measured worst case is a single slow probe, so stacking
  probes would stack those peaks into one frame. Navigation and selection stay live,
  choosing a row cancels the filter and sends the command, and an incomplete filter is
  never cached. While pending, the title carries a trailing ellipsis (for example
  `Brass lantern ...`) and the PROFILE strip adds `pk <ms>` for the longest filter
  frame.

### Phase 1 results (PROFILE ROM, mGBA, 2026-11-23)

The menu is on screen by frame 2 of the Act phase with 0 probes reported, while the
filter continues behind it. "Total" is the summed per-step TM0 time (the same probe
work the synchronous build used to do in one frame); "peak" is the longest single
filter frame. The Effects column is the number of effect replays left after the
availability probe captures the duplicate effect.

| Room | Target | Probes | Effects | First menu | Total filter | Peak frame |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Z1 Living Room | Sword | 5 | 0 | frame 2 | 1,026 ms | 52 ms |
| Z1 Living Room | Brass lantern | 9 | 1 | frame 2 | 2,574 ms | 51 ms |
| Z2 Gazebo | China teapot | 10 | 1 | frame 2 | 2,753 ms | 44 ms |
| Z2 Riddle Room | Stone door | 8 | 0 | frame 2 | 2,459 ms | 72 ms |
| Any of the above | reopened (cache) | 0 | 0 | frame 1 | 0 ms | 0 ms |

Effect replay reuse: when an availability reply duplicates an accepted action, the
probe runs the normalization pass in the same session and returns the effect hash, so
only the first action of each duplicate group is replayed. The audit's worst-case
effect count fell from 16 to 3 and the lantern/teapot dropped from 2 to 1. The
normalization pass, not the action replay, still dominates the effect path; the peak
frame is an atomic Z-machine instruction inside a probe.

### Instruction-level slicing (2026-11-23)

`z_probe_menu_begin` / `continue` / `normalize` / `complete` now run at most
`STORY_MENU_PROBE_CHUNK` (200) Z-machine instructions per filter step. The
candidate noun-resolution sequence and the effect replays are explicit state
machines, so the main loop renders and accepts input between chunks.

Chunk tuning on the PROFILE ROM (worst frame):

| Menu | Before slicing | chunk 400 | chunk 200 | chunk 100 |
| --- | ---: | ---: | ---: | ---: |
| Z1 sword | 257 ms | 96 ms | 76 ms | 65 ms |
| Z1 lantern | 532 ms | 194 ms | 185 ms | 185 ms |
| Z2 teapot | 509 ms | 196 ms | 196 ms | 186 ms |
| Z2 stone door | 309 ms | 211 ms | 190 ms | 179 ms |

These rows predate the fingerprint fast path, the hash/restore split and the restore
fast path below; after them, the chunk-200 peaks are 52 / 51 / 44 / 72 ms for the
sword, lantern, teapot and stone door.

The audit still matches all 1,003 states (max 17 probes, max 3 effect replays).
Slicing moves work rather than removing it: summed probe time is essentially
unchanged for the sword, lantern and teapot, and the Riddle door's total rose about
29%. The menu is usable throughout; only the settle time grows.

Per-instruction tracing showed that the slowest single story instruction is only
about 1–1.6 ms, so the remaining peak was not an instruction. It was the
persistent-effect fingerprint: `probe_effect_after_run` walked every dynamic byte and
called the transient/parser-address checks before the unchanged-byte early-out.

### Effect fingerprint fast path (2026-11-23)

The fingerprint now compares four bytes at a time and only performs the
transient/parser checks for bytes that actually changed. The changed-byte hash order
and values are unchanged.

- Fingerprint time: **122 ms → 40 ms** (663 ticks).
- The fingerprint and the batch restore now run in separate frames: hash 40 ms,
  restore 34 ms.
- Worst frame: **185–190 ms → 70–97 ms**.
- Summed probe time: about 150 ms lower per menu.

Tokenization is only ~8.6 ms (141 ticks) and is not the peak. After the fingerprint
and restore were separated, the next fixed cost was the per-probe restore itself.

### Restore fast path (2026-11-23)

`z_load` restores the full dynamic-memory image after every probe with a byte-wise
`memcpy`. The freestanding copy was taking ~33 ms per probe (8–10 restores per slow
menu). `memcpy`/`memset` now copy aligned 32-bit words with a byte fallback:

- Restore: **33 ms → 9 ms** per probe.
- Total filter: another **8–11%** lower (sword 1,043 ms, lantern 2,607 ms, teapot
  2,795 ms, Riddle door 2,502 ms).
- Worst frames: 52 / 51 / 44 / 72 ms.

The audit is unchanged. The Riddle door's remaining 72 ms peak is its inventory
normalization; the other menus are near the 40 ms fingerprint frame.

### Budget and fixed-cost trims (2026-11-24)

- Probe budget kept at **5,000**. A 4,000 trial passed all 1,003 states but produced
  no measurable gain on the measured menus: their probes finish below 4,000, so the
  extra budget is unused headroom. Keeping 5,000 preserves the safety margin.
- Inventory normalization output is suppressed; only the parser-settling execution
  runs.
- The dictionary tokenizer has a 16-entry encoded-word cache.

Measured at 5,000: about **1.5%** lower total time than the pre-trim memcpy
baseline, and no peak improvement. Tokenization fell 24–46% on three menus, but the
total is dominated by story execution. The inventory routine's instruction
execution, not its output, is the remaining normalization cost.

The next real lever is reducing the *number* of probes, not their fixed costs:

1. **State-aware probe masks.** Use the QA action profiles to skip verbs that cannot
   produce actions for an object's current room, parent, attribute and carried state.
   This removes whole slow probes (each 0.2–0.5 s) rather than shaving fixed costs.
2. **Cheaper canonical normalization command.** "inventory" walks the carried list;
   a no-effect command with shorter output or less story work (for example "score")
   would cut each normalization step, but needs the 1,003-state audit to prove that
   duplicate detection is unchanged.
3. **Faster story execution.** The interpreter dominates the remaining seconds;
   compiled per-state menus or interpreter hot-path work are the large options.

The Phase 0 timings for the lantern (529 ms) and teapot (997 ms) were not
reproduced: the same probe/effect counts now sum to 3.0–3.3 s. The Phase 1 captures
show the named targets and use a per-step timer, so they are the values to trust for
those two menus.

Every audited state still settles on the reference list:

- `make test` runs the new filter over the same 1,003 states and compares it with
  `story_filter_defaults_masked`: 1,003 matches, 0 mismatches, 6,766 probes total,
  maximum 17 probes and 3 effect replays in one menu. (The replay-based reference
  still uses up to 16.)
- The worst frame is now 0.04–0.07 s. The fingerprint (40 ms) and restore (9 ms)
  each run in their own frame; the Riddle door's remaining peak is its inventory
  normalization. The ~1 ms slowest instruction is no longer the bottleneck.
- The filter state (about 3.6 KB) lives in IWRAM below the stack. The two-entry
  cache is the only new EWRAM consumer, leaving about 1.2 KB free. `src/gba.ld`
  defines the IWRAM section and `src/start.s` clears it at boot.

Remaining narrow case: a climbable target still runs the `story_actions` climb
direction probes before the first draw. Moving that choice into the sliced filter
would make climbables render in one frame too; the action-set parity audit currently
depends on the same `story_actions` list.

### State-aware candidate masks (implementation in progress)

Generated profiles now include an exact observed structural-state key consisting
of game, room, object, parent, carried status and the object's 32 attribute bits.
Duplicate profile rows for the same key are ORed together, preserving every
observed action. An exact key uses that narrower route-evidence superset. An
unknown key falls back to the object's broader all-room superset; an object absent
from the audit data falls back to all candidate verbs. The runtime still probes
each retained candidate against the live story, so replies and effects remain
authoritative for candidates the profile retains.

This key does not encode every Z-machine global. A story branch that changes
available actions while leaving room, parent, carried status and attributes
unchanged must be represented in the route profile union before its narrower mask
is safe. The full route/action parity audit is therefore required before relying
on measured savings.

Savings and route parity still need a fresh replay audit. After any story-data or
action-rule update, refresh `tests/fixtures/action-profiles.tsv` from the route audit
before relying on a narrower profile.

### Phase 1 — Immediate menu, incremental filter

Split `choose_action_list(VERBS, PRIMARY)` into two stages:

1. **Build and show** the static candidate list from `story_actions` immediately.
   Render the menu in the same frame. Set a `menu_filter_pending` flag and a small
   “Checking actions…” hint in the menu title/footer.
2. **Filter in slices** in the main loop:
   - keep the probe batch open across frames (`story_menu_probe_begin` once, then
     `story_menu_probe_end` when done or canceled);
   - run a bounded number of probes per frame (start with 2–4; tune from Phase 0);
   - after each slice, remove newly rejected actions and redraw;
   - preserve the selected action when it survives; otherwise clamp the cursor to the
     nearest remaining row.

While filtering is pending:

- the player may navigate the menu and may choose a candidate; choosing an action
  cancels the filter, closes the probe batch, and sends the command;
- a completed filter caches its result (Phase 2);
- a canceled filter is not cached.

The story must not advance while the batch is open except through the menu’s own
probe calls. Any non-menu input that can send a command cancels the filter first.

### Phase 2 — Cache the filtered list

Add a small cache of finished primary menus, keyed by:

```c
typedef struct {
    uint8_t  chapter;
    uint8_t  carried;
    uint16_t room;
    uint16_t object;
    uint32_t generation;   // bumped after every completed story command
    uint8_t  count;
    Action   actions[MAX_ACTIONS];
} PrimaryMenuCache;
```

- `generation` is a single counter bumped whenever a command completes (or any story
  state mutation is committed). A menu can only become stale after a command, so this
  is conservative and cheap.
- Start with **two entries**, most-recently-used replacement. `Action` is small; two
  entries fit comfortably in the ~2 KB EWRAM headroom (measure before finalizing).
- On Act: if the key matches a complete entry, copy it and skip probing entirely.
- On return from `More actions…`/Orders: use the cached primary list instead of calling
  `choose_action_list` again.
- On a canceled/incomplete filter: do not store.
- If the cache is full, replace the oldest entry.

The cache stores the *filtered* list, not probe replies, so no story-state payload is
retained.

### Phase 3 — Reply-only availability probes

Add a probe mode that returns the reply without the post-reply `inventory`
normalization:

- use it for `probe_action_form` / availability checks while building a menu;
- keep the existing normalized/effect path for `action_effect_signature` and any caller
  that fingerprints persistent state;
- if a reply-only probe crashes or exceeds the budget, treat the action as rejected, as
  today.

This roughly halves the work per availability probe. Add a test that the reply-only
result matches the normalized result for the existing action corpus.

### Phase 4 — Bound duplicate verification in the primary menu

- Keep normalized-reply de-duplication for all actions.
- Run effect probes only when two kept actions share a reply signature, and cap them at
  **4 per menu** for the primary list (down from 24).
- If the cap is reached, keep the earlier action and leave the later one in place
  rather than making an unverified merge.
- The category/`More actions…` lists may keep the existing higher cap if needed.

### Fallbacks (only if Phase 0 measurements miss the target)

5. **Lazy discovery.** For the primary list, skip the `probe_verbs` walk until the
   menu has been idle for N frames or the player opens `More actions…`. Static
   candidates appear instantly; story-specific verbs arrive shortly after or in More.
6. **Attribute fast paths.** Trust attributes for actions the story cannot plausibly
   refuse (Take/portable, Open-Close/door, Read/readable, Light/light, Eat/edible,
   Drop/carried), and probe only ambiguous/custom verbs. This changes menu filtering
   slightly: a locked door’s “Open” would remain visible.
7. **Compiled per-state menus.** Use the QA action profiles (1,003 audited states) to
   build a hash-keyed table of final menus, with a probe fallback for misses.

The implementation starts with Phases 0–4 and only adopts a fallback if the measured
Act latency or probe count still misses the targets below.

## Interaction details

- **Menu title/footer:** add a compact pending indicator (for example, a dim
  `Checking…` label) and remove it when the filter completes.
- **Selection:** if the selected row disappears, keep the same index clamped to the new
  count; do not jump to the top. If the selected row survives, keep it selected.
- **Cancel:** any input that sends a story command cancels the pending filter and ends
  the probe batch before sending.
- **No story drift:** probing must not change the visible room, story log, or pending
  menu; `z_probe` already rewinds state. Keep the batch scoped to the menu.
- **Back-outs:** returning from `More actions…`/Orders must not re-probe when the cache
  key still matches.

## Tests

- **Menu equivalence:** for the existing 1,003 audited states, the final filtered
  primary list must match the current synchronous result, except where the Phase 4
  effect cap intentionally leaves a duplicate. Record and review any differences.
- **Cache:** Act twice → second time performs zero probes; change state (take/drop,
  open/close, light) → the menu rebuilds and reflects the new state; back out of More →
  zero probes.
- **Slicing:** simulate one probe per frame and verify the menu redraws, the selection
  is preserved/clamped, and a selection made mid-filter cancels the batch and executes
  correctly.
- **Reply-only probe:** compare against the normalized probe on the existing action
  corpus; no differences in accepted/rejected classification.
- **Latency:** in mGBA, Act draws the menu within one frame; the pending filter never
  blocks more than the configured slice. Record the frame/cycle count in the QA log.
- **Existing audits:** `make test` action-profile, de-duplication, room-target and
  parser audits remain green; `make all` builds both ROMs with no `visual_qa` symbols.

## Risks

- **Sliced probes and input:** the story must not run a command while the probe batch is
  open. Cancel the batch on any command-sending input; test this explicitly.
- **Cache staleness:** `generation` must bump on every completed command (including
  deaths/restores). If a new state mutation path bypasses it, invalidate the cache on
  room changes and menu-object changes as a second guard.
- **Menu churn:** actions disappearing while the player navigates is the main UX risk;
  preserve/clamp selection and keep the pending indicator visible.
- **Effect cap:** two actions with the same reply and different effects can both remain;
  this is a deliberate trade-off and should be rare.
- **EWRAM:** two cache entries plus the filter state must fit; measure `.bss` before and
  after. If tight, start with one entry.
- **Probe budget:** reply-only probes must not change parser state used by effect
  probes; keep the two modes separate and test the effect path.

## Rollout and metrics

1. Land Phase 0 instrumentation only; record the baseline on mGBA.
2. Land Phase 1 (render-first/slicing) behind no flag; the menu is usable immediately.
3. Land Phase 2 (cache), then Phase 3 (reply-only), then Phase 4 (effect cap), measuring
   after each.
4. Success criteria: Act-to-menu ≤ 1 frame; no synchronous stall longer than one probe
   slice; repeat Act/back-out = 0 probes; average availability probes no higher than
   today; existing action lists preserved except the documented effect-cap cases.

## Open questions

- The exact probe slice size (2–4 per frame) and the pending indicator wording should be
  tuned on hardware.
- One cache entry may be enough if only the current object is ever re-opened; start with
  two and measure the `.bss` cost.
- Whether the primary menu should keep the effect cap at 4 or skip effect verification
  entirely; Phase 0/1 measurements will show the tail cost.
