# Asset / story consistency review — November 18, updated November 19, 2026

## Follow-up status (Nov 19)

The original scenery and wiring findings below have since been addressed in the current
source. Room 163 remains dry, matching the connected room 182 → 163 → 93 channel. Hallway
169's mirror panel is moved aside and the actual colored Inside Mirror room 160 is visible
through the opening. The unused generic mirror, parapet, gazebo, and old Z3 hallway/mirror
scenes were moved to the retired-backgrounds folder and removed from ROM scene declarations;
Z3 production backgrounds are now listed explicitly in the Makefile. Rooms 95, 142, 185,
226 and 237 have dedicated Zork II art/state handling, and the background wiring audit was
refreshed. The later item-sprite mismatches are covered in `visual-consistency-review.md`
and `assets/items/ITEM_COVERAGE.md`.

A cross-cutting review of the production background set against the bundled stories'
own room text, and against neighboring rooms and state variants. It complements the
earlier route-driven audits (which followed reachable paths); this pass starts from the
story data and the asset inventory instead.

**Status:** focused review, not exhaustive. The three chapter reviewers were interrupted
by provider usage limits and expired API keys; the main session completed the high-value
connected sets visually and verified the cross-cutting wiring findings. Every verdict
below is backed by what was actually inspected; areas not inspected are listed under
Coverage and limits.

## Method and ground truth

- `build/story-dump/dump_rooms.py` decodes the v3 object/property tables the same way the
  game's MojoZork-derived interpreter does: 111/80/87 room objects for Zork I/II/III,
  their names, and their direction exits. Size-1 exits are static rooms; every size-2
  exit in these releases has the high bit set and is a story-routine conditional.
- `build/story-dump/dump_ldesc.py` uses the oracle dylib to teleport `HERE`, force `LIT`,
  and submit `look`, capturing the story's own description for each room — 278 rooms,
  zero capture failures.
- `build/story-dump/zork{1,2,3}-sheets.md` combine description + exits per room; these
  were the authoritative reference for all findings below.
- `build/story-dump/crops/` holds the in-game 240×76 band (source rows 10–85) at 2× for
  every root background. Visual checks were made on these crops.
- Room→art selection was verified in `src/chapter.c`, `src/zork1_scenes.def`,
  `src/zork2_scenes.def`, `src/scenes.S`, and the Makefile.

## Findings

### 1. Four shipped backgrounds contradict their story descriptions (P2)

The current source (`src/chapter.c`, verified 23:57) selects redesigned art for rooms
113, 160, 163, and 169; the concurrent session rebuilt `zork-trilogy.gba` at 23:58 with
the new symbols. The art these replaced, which was wired and shipped earlier today, does
not match the stories:

| Room | Story (from the sheets) | Previously wired art | Redesign |
| --- | --- | --- | --- |
| Z3 113 · Hallway | "dressed stone … shallow stone channel … widens into a large hole around which is engraved a compass rose. The corridor continues north and south." | Plank door and two statues in alcoves — both invented; corridor blocked | `z3_guardian_hallway_v3.png` removes them and opens the corridor; channel and compass rose kept |
| Z3 169 · Hallway | Same channel and compass rose; "the hallway continues to the south. A large mirror fills the north side." | Mirror on the right wall with a carved relief opposite it | `z3_mirror_hall_v3.png` shows the mirror panel moved aside and leaning against the east wall, revealing Inside Mirror 160 |
| Z3 160 · Inside Mirror | Open floor, channel, compass rose, mahogany/pine panels, red-black and white-yellow walls, blue ceiling, carved bar, two poles, west-pointing arrow | Closed carved-stone floor, gear mechanism, no colored panels | `mirror_room_v2.png` matches each described detail |
| Z3 163 · Water Slide | "the water channel drops precipitously and enters a rocky hole" | Dry channel | Final `z3_water_slide.png` keeps the channel dry to match the same route in rooms 182 and 93 |

Follow-up: the wet version was rejected after checking 182 → 163 → 93 as one connected
route. The Water Slide is dry, matching the Aqueduct above and Damp Passage below.

Follow-up: the mirror panel is moved aside and leans against the east wall; its plain wooden
back is visible, and the opening reveals the colored room entered by going north to 160.

### 2. `parapet.png` was embedded but never displayed (P3, wiring; resolved)

`src/scenes.S` includes it in the ROM and `tests/scene_stubs.c` stubs it, but no code path
selects it: Zork III room 36 uses `art_z3_parapet` in both the ID branch and the name
branch, and the Zork II story has no Parapet room. The `FLOOR(parapet, …)` row in
`room_visuals.c` is keyed by that unused pointer and is dead. It is a superseded single-
state scene left behind by the eight-state `z3_parapet` work, of the same class as the
three orphans the Nov 18 wiring audit did remove.

Resolved: `assets/parapet.png` and its scene symbol, Makefile entry, externs, floor row,
test stub and obsolete README description were retired. The active Zork III Parapet is
`z3_parapet.png`.

### 3. Four reachable Zork II rooms with distinctive descriptions fell to generic hall art (P2; resolved)

From `build/story-dump/review-zork2.md` (programmatic/mapping analysis), verified against
`src/zork2_scenes.def` and visually confirmed by viewing `hall.png` and
`z2_angled_west.png`:

- Rooms 95, 142, 185, 226 — "Oddly-angled Room": "walls are made of some glassy
  substance. On the floor is a very small diamond shaped window which is dark."
  Five other Oddly-angled Rooms have dedicated `z2_angled_*` glassy art; these four were
  not in the scene registry and previously received the generic arched stone `hall.png`.
- Room 237 — Murky Room: "sandy floor here, and your vision seems murky and blurred.
  The wall you are looking at is nicely dressed stone." Also falls to `hall.png`.

Resolved: rooms 95, 142, 185 and 226 now share the dedicated glassy Oddly-angled Room art
with the story-driven floor-window overlay; room 237 uses `z2_murky_room.png`.

### 4. The Nov 18 wiring audit's completeness claim was stale (P3; resolved)

Resolved: `background-asset-wiring-audit.md` was refreshed, retired sources moved out of
the production directory, dead scene symbols removed, and the Z3 list made explicit.

### 5. Positive: shipped ROM parity and connected sets checked

- The 23:58 ROM observation was before the wiring cleanup. The current trilogy was rebuilt
  after the scene list changes; the ELF symbol table contains the active mirror, Guardians
  hallway and Z2 Gazebo symbols, and no exact symbols for the retired generic scenes.
  This verifies wiring, not a visual emulator capture. An earlier 22:33 build matched a
  fresh RGB555 conversion of every then-wired PNG byte-for-byte.
- Visually checked and consistent with story text / prior audit: kitchen and behind-house
  window pair, living-room rug/trapdoor and cellar hatch states, dam gate states and
  reservoir views, barrow outside/inside, living room ladder, Z3 prison/marble hall open
  and closed views, Z3 lake surface vs underwater, Z3 narrow-mirror corridor, and the
  Z3 cliff-top / cliff-ledge / cliff-base chain as three elevations of one wall.
- Zork I's `z1_east_west_passage.png` stairway, which first looked invented, is in fact
  required: room 130's text says "a narrow stairway leading down at the north end."

### 6. Recent Zork I art batch is undocumented (P3)

Thirteen `z1_` backgrounds plus `z3_creepy_crawl.png` were regenerated 17:33–17:37 today,
after the newest written Zork I audit (08:21) and the paired/wiring audits (14:31). The
current ROM embeds them, but no audit document describes the current versions. I viewed
four of them (`z1_east_west_passage`, `z1_engraving_cave`, `z1_chasm_east`,
`z1_rocky_ledge`, plus `z1_tiny_cave`) against their room text and found no mismatch;
the rest remain unchecked against a written review.

## Remaining follow-up

The original recommendations to wire-check rooms 113/160/163/169, settle the dry Water
Slide, revise the mirror reveal, add Zork II art for rooms 95/142/185/226/237, and remove
the unused Parapet scene are complete in current source. A full visual sweep of every
remaining room crop is still open; the original room sheets and crops remain in
`build/story-dump/` for that work.

## Coverage and limits

- Visually inspected in this session: the four redesigned-room pairs; Zork I window,
  hatch, barrow, dam, ladder-adjacent and cave/ledge sets; Zork III prison/marble,
  lake/underwater, mirror corridor, cliff chain; Zork II `hall.png` vs an angled room.
- Mapping/wiring: all room→art selections in `src/chapter.c` and both scene registries
  were cross-checked at the code level.
- Zork I: ~6 of 52 `z1_` assets viewed. Zork II: 2 of ~101 assets viewed directly; the
  rest of that chapter's verdicts come from the programmatic report plus the repo's
  written audits. Zork III: ~18 of ~80 assets viewed.
- The original Nov 18 scenery pass did not review item/encounter sprite art, in-game
  compositing, menus, or ROM behavior in mGBA. The later item inventory pass reviewed the
  story-to-sprite bindings and corrected its five flagged mismatches, but did not capture
  these latest sprites in mGBA. The three chapter reports
  in `build/story-dump/` are retained as evidence: `review-zork2.md` is a complete
  mapping/programmatic report; `review-zork1.md` and `review-zork3.md` were not
  produced because their agents hit the provider limit.

## Reviewed sources

`build/story-dump/zork{1,2,3}-sheets.md`, `build/story-dump/crops/`, `src/chapter.c`,
`src/chapter.h`, `src/zork1_scenes.def`, `src/zork2_scenes.def`, `src/scenes.S`,
`src/room_visuals.c`, `src/trilogy.c`, the Makefile, `assets/SCENERY_RULES.md`,
`assets/AGENTS.md`, `qa-screen-checklist.md`, and the existing root-level audits
(especially `paired-entrance-exit-art-audit.md`, `background-asset-wiring-audit.md`,
`zork-ii-completion-audit.md`, `zork-iii-consistency-pass.md`).
