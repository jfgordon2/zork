# Asset / story consistency review — follow-up sweep (November 19, 2026)

A full visual sweep of the production background set against the bundled stories' room
descriptions, following the focused review in `asset-story-consistency-audit.md`. This
pass viewed **every one of the ~257 production backgrounds** via labeled contact sheets
(build/story-dump/sheets/z1-*.png, z2-*.png, z3-*.png, shared-*.png), each built from the
exact in-game 240×76 crop (build/story-dump/crops/). Findings were checked against the
story sheets (build/story-dump/zork{1,2,3}-sheets.md) and the current room→art selection
in src/chapter.c and the scene registries.

**Important:** another session is actively editing this repo. During this sweep it added
`z2_murky_room.png`, `z3_shadow_cliff.png`, `z3_prison_cell_bronze.png`, `z1_maze_dead_end.png`,
`z1_stream*.png`, `z1_shore.png`, renamed `z3_mirror_hall_v3` to `z3_mirror_hall_open`,
rewired rooms 95/142/185/226, and reverted room 163 to the dry water slide. All findings
below describe the state at 07:50 on November 19.

## Findings

### F-1 · Room 169 conflated closed and open mirror states — P2 (resolved Nov 19)

The sweep's criticism was **partly right**: the closed state needs a large mirror filling
the north wall. But the open art's red/black and blue painted surfaces are not a wrong
mirror design; after the Beam Room button opens the panel, north from room 169 enters
Inside Mirror 160, whose story describes those exact panels and colors. The route text
explicitly says the mirror is mounted on a panel that has been opened outward.

The actual defect was that `src/chapter.c` selected the open art unconditionally. The
game now selects `z3_mirror_hall_closed.png` while object 29 is closed and
`z3_mirror_hall_open.png` after it opens. The closed plate shows the north-wall mirror
and angled stone reflection. The open plate reveals the real Inside Mirror room and
keeps the hinged panel attached at the east jamb, folded against the wall. Its target
box follows the same state. Both native crops were reviewed; mGBA capture and full UI
checks remain open. The previous open plate is retained under
`docs/art-history/encounter-options/retired-backgrounds/`.

### F-2 · Prior findings resolved by the concurrent session (no action needed)

Verified visually or in code during this sweep:

| Prior finding | Current state |
| --- | --- |
| Rooms 113/160/163 art contradicted the story | Redesigned art is wired and in the 23:58 ROM. Room 163 was subsequently reverted to the dry slide, consistent with room 93's dry channel. |
| Four Oddly-angled Rooms + Murky Room used generic hall art | New `z2_murky_room.png` and expanded angled set; rooms 95/142/185/226 now select dedicated art (`src/chapter.c:224`). |
| Room 204 needed a bronze door | New `z3_prison_cell_bronze.png` wired to room 204. |
| Zork I stream/shore rooms had no dedicated art | New `z1_stream.png`, `z1_stream_view.png`, `z1_shore.png` wired to rooms 86/67/40. |
| `parapet.png` orphan | Still declared but unselected; no change observed. |

## Sweep results — no findings

Every other production background was viewed at in-game size and compared with its
room's description and, where applicable, its neighbors and state variants. No further
story/geometry mismatches were found. Specific checks that could have produced findings
but did not:

- **Zork I:** window pair (rear/kitchen sash states), living/cellar hatch pair, barrow
  outside/inside, dam gate and reservoir states, ladder top/bottom, mirror north/south
  (two rooms with identical story text — identical art is correct), the 13 files
  regenerated on Nov 18, the plain marble pedestal in the Torch Room (the flaming torch
  is a separate overlay), the rivershore and beach views (adjacent views of one river).
- **Zork II:** angled-room set as one glassy rotating location, Tiny/Dreary door states
  and through-view, Guarded/Workshop door, Marble Hall garden through-view, Riddle Room
  stone door, Crypt states, ice room, pool states, the six Quarters looks, the volcano
  ledge chain, the well pair, and the bank rooms.
- **Zork III:** guardian hallway (room 113, now correct), prison/marble open+closed,
  underwater/lake/shores, narrow-mirror set and its west flips, the cliff chain, the new
  shadow cliff (room 170), lake shore, aqueduct/balcony views, treasury, foggy room, and
  the beam/button room pair.
- **Shared art:** Hades pair, Jewel Room cage, museum entrance states, Cerberus entrance,
  both guardian scenes, End of Rainbow, Atlantis, the Carousel, Tea Room, and the
  white-house exterior views.

## Coverage and limits

- All production backgrounds were viewed indirectly via contact sheets at the in-game
  crop size; ~15 crops flagged by the sheets or by story comparison were opened
  individually. Contact-sheet viewing can miss a very small detail, so "no finding" means
  nothing conspicuous at game size, not a proof of pixel-perfect correctness.
- Room→art selection was checked against the current source at the time of review;
  the session editing this repo may change it again after this document is written.
- Item and encounter sprites, UI compositing, in-game overlays, and newly added assets
  that appeared after 07:44 were not part of this sweep except where noted.

## Next steps

1. Rebuild the ROM and capture room 169 in mGBA to verify the full UI crop and any target
   box over the moved panel.
2. Re-run the current-ROM gallery for all recently changed backgrounds. This sweep did
   not cover post-07:50 assets or sprite overlays.
3. No other story/geometry replacements were identified by this sweep.
