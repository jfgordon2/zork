# Visual consistency review — November 19, 2026

A full static sweep of the production backgrounds against `qa/screen-checklist.md` and
`assets/SCENERY_RULES.md`. It follows up recommendation 5 of
`asset-story-consistency-audit.md` (sweep the remaining crops).

## Latest art replacements (Nov 19)

These replacements address open geometry findings from this review and the follow-up
asset sweep. Each production plate was reduced to 240×102 and checked at the 240×76
display crop. Both ROMs have been rebuilt; current-ROM mGBA screenshots, overlay contact
and UI coverage remain unchecked.

- **Z1 Stone Barrow ending:** the bridge now crosses the left-to-right stream front to
  back, joins both banks, and continues to the far-bank path and tunnel.
- **Z3 Hallway 169:** added a closed state with the north-wall mirror and an open state
  that reveals the actual Inside Mirror. The panel is still attached at the east jamb;
  room selection and the focus box follow the panel's open state.
- **Z3 South Corridor 190:** the front-on wall was replaced with a perspective marble
  junction; its floor is continuous into the east/west arms, which turn north at their
  far ends.
- **Z3 Technology Museum:** the south door opens onto the connected Museum Entrance
  with a flush floor threshold; the Royal Puzzle steps remain at the far end of that hall.

## Fix log (Nov 19, after the review)

- **Updated: Zork II Top of Well and Circular Room.** Replaced the wellhead with the approved raised masonry shaft and timber hoist, anchored at the curb's far left and right edges. The scene carries its centered pulley rope; in room 119 a thicker braided tether reaches from the top scene edge to a raised iron bail on the bucket, so the connection reads clearly at GBA scale. Rebuilt the trilogy ROM and refreshed five native mGBA screenshots; all captured the expected room IDs.

- **Added: Zork I river path trio.** Rooms 40 Shore, 67 Stream View and 86 Stream now have
  dedicated backgrounds instead of the generic lake grotto. The three plates were composed
  as connected views against the Sandy Beach, Aragain Falls, Canyon Bottom and Reservoir
  South art; each has a separate floor-contact anchor. Shore 40 shows the east-bank path
  bending west toward the falls, Stream View 67 shows the smaller side stream reaching the
  reservoir, and Stream 86 narrows into twisting walls with a small beach for landing. The
  240×76 crops were inspected; an mGBA capture and interaction/persistence pass remain open.
- **Added: Zork I distinct scenes.** Room 25 Damp Cave now has damp side passages and a
  floor-level south crack; room 162 Deep Canyon has the canyon edge, side passages and one
  descending stair; room 220 Studio has the paint-splattered room, open Gallery door and
  fireplace chimney; room 238 Forest Path has a low branch matching the Tree Canopy view.
  Each scene is wired by room ID with its own floor anchor. Crops were reviewed; mGBA
  captures and interaction/persistence checks remain open.
- **Added: Zork II Dragon Room bridge.** Room 248 now has a broad supported stone bridge
  along the paved route from the west passage to the south, a separate smoky north tunnel
  for the dragon encounter, and a narrow east crack. The bridge and tunnel do not merge.
  The crop was reviewed; in-game placement and the two dragon poses still need mGBA review.
- **Added: Zork III Royal Hall seal.** Room 116 now has the carved Royal Seal on the broad
  east wall beside the north arch, leaving the central tiled passage clear. The native crop
  was reviewed.
- **Revised: Zork III Technology Museum south door.** Rooms 5 and 176 now face the open
  south doorway into the Museum Entrance. The matching oak leaves, side doors, entrance
  floor, and descending south steps are visible through it; the previous extra archway was
  removed. The two exhibits and central floor remain clear. The crop was reviewed.
- **Fixed: Zork I Studio manual placement.** Object 41 (the ZORK owner's manual, described
  in room 220 as a small piece of paper attached to the wall) is now anchored to the painted
  wall instead of being auto-placed on the floor. It remains a separate removable sprite.

- **Fixed: P1 test link.** Stubs for the four redesigned scenes were added to `tests/scene_stubs.c` and `tests/z3_scene_stubs.c`. When the transcript-replay QA ran, it found four target boxes overshooting the art band by 2 px: Z3 169 object 107 and Z3 160 objects 52, 196 and 212. Those were trimmed in `src/room_visuals.c`. `make test` now passes, with 0 boxes outside the band and all three full routes decoding byte-for-byte. ROM rebuilt (SHA-256 `d2311f82…7ffa`).
- **Fixed: P2 #2 Damp Cave.** The water keyword is now an exact `"Dam"` match. Every real Dam room already had an ID mapping, so Damp Cave now gets the generic cave art.
- **Fixed: P2 #1 maze.** All 15 maze rooms, including the skeleton room 218, now use `z1_maze`. The four Dead Ends (141, 143, 147, 153) use the new `z1_maze_dead_end.png`. It has the same camera, materials and floor as `z1_maze`, with the passage openings closed using the image's own rock at native scale, and its floor anchors match. In room 218, the skeleton, knife, key and bag were composited at their existing contact points and sit on the `z1_maze` floor. The off-style `z1_maze_skeleton.png` was moved to `docs/art-history/encounter-options/retired-backgrounds/`. Its scene, `FLOOR` row and gallery mapping were updated. The build and `make test` pass. Not yet captured in mGBA.

- **Fixed: generic-hall Zork II rooms (P3 table; originally Z2-S-01/02).**
  - Oddly-angled rooms 95, 142, 185 and 226 now use `z2_angled_window` (dark) or `z2_angled_window_lit` (glowing). Both are derived from `z2_angled_northeast` and add the story's small diamond window in the floor.
  - The story routine at 0xe01c prints "glowing serenely" when global 13 is set and "dark" otherwise, and `src/chapter.c` selects the image the same way. Global 13 is the flag that also opens the room 19 secret passage. Each room has a focus box for the window (object 216).
  - Murky Room 237 now uses `z2_murky_room`. It is built from `aquarium_broken`'s own tank slabs and sand at exactly 2× so it matches the tank seen from outside, plus the blur and murk the story describes.
  - In the story interpreter, all four rooms read "dark" with global 13 = 0 and "glowing serenely" with global 13 = 1.
  - No winning route visits these rooms, so `tests/transcript_replay_test.c` now checks them directly after the Zork II route. It sets the story's current room and window flag, asserts that `story_scene` picks `z2_angled_window`/`_lit` and `z2_murky_room`, and runs the focus-box audit. The audit grew from 970 to 978 states, with 0 missing boxes and 0 out of band.
  - The Murky Room has no selectable targets in that state, so only its art selection is checked. There is no mGBA capture yet.

## Method

- Every production root background in `assets/*.png` plus the four Makefile mirror scenes was
  cropped to the in-game band (source rows 10–85) and viewed at 2× on labeled contact
  sheets. Suspect scenes were re-checked at 3×.
- A room→art table was derived from `src/chapter.c`, `src/zork2_scenes.def` and
  `src/chapter.h` for all 278 story rooms. Rooms that reach the generic keyword
  fallbacks (`forest`, `water`, `cavern`, `hall`) were listed.
- Story text and exits come from `build/story-dump/zork{1,2,3}-ldescs.json` and
  `-rooms.json`.

Checklist rows covered: **Story match**, **Route continuity**, **Architectural access**,
**Human clearance**, **Entity identity** (background structures only), and asset wiring.
The following rows are **Unchecked** because they need emulator captures: Placement with
sprites, Drop/take/move, Valid and rejected actions, Visibility and focus, UI access,
Text and controls, Persistence, and Timing. This background-focused pass did not cover item
and encounter sprites; those are reviewed below.

## Findings

### P1 — `make test` does not link

`tests/scene_stubs.c` and `tests/z3_scene_stubs.c` have no stubs for `art_mirror_room_v2`,
`art_z3_guardian_hallway_v3`, `art_z3_mirror_hall_v3` or `art_z3_water_slide_v2`. The four
redesigned rooms were wired in `src/chapter.c` without stubs, so `build/story-test`
fails to link. The transcript-replay QA cannot run until the stubs are added. The ROM
itself is fine: all four symbols are in `src/scenes.S`.

### P2 — Story, access and continuity issues

| # | Room / art | Rule | Problem |
| --- | --- | --- | --- |
| 1 | Zork I maze and dead ends | Reuse only when the story makes rooms alike | **Resolved:** all listed twisty passages/dead ends now share `z1_maze` or its matching dead-end variant; room 218 uses the same maze background with separate skeleton/item sprites. |
| 2 | Z1 #25 Damp Cave | Story match | **Resolved:** room ID 25 selects `z1_damp_cave`, avoiding the `dam` keyword collision. |
| 3 | Z2 #46 `z2_tiny_open` / #59 `z2_dreary_open` | Door swing must agree in both views | **Resolved Nov 20:** the shared door opens into Dreary Room. The Tiny view shows it swinging away; the Dreary view shows it swinging toward the viewer. This direction is an inference because the story does not specify the hinge or swing. |
| 4 | Z2 #15 `z2_lava_room` | State and route continuity | **Resolved Nov 20:** the east passage no longer shows the glacier after the route has melted it. |
| 5 | Z1 #31 `z1_slide_room` | Access and clearance | **Resolved Nov 20:** the low north opening remains walkable, and the south wall now carries the "Granite Wall" inscription. |
| 6 | Z1 #127 `troll_room` | Human clearance | **Partially resolved:** the west opening is adult-sized and at floor level. Confirm the south passage is visible in the current crop. |
| 7 | Z1 #244 `z1_barrow_inside` | Bridges meet their banks | **Resolved Nov 19:** the replacement bridge crosses the stream front-to-back, touches both banks, and meets the far-bank path. Emulator composition check remains open. |
| 8 | Z3 #169 `z3_mirror_hall_closed` / `z3_mirror_hall_open` | Story match | **Resolved Nov 19:** closed state shows the north-wall mirror; when object 29 opens outward, the attached panel folds to the east wall and reveals Inside Mirror 160. Emulator composition check remains open. |
| 9 | Z3 #190 `z3_south_corridor` | Shared materials and perspective | **Resolved Nov 19:** replaced with a converging lavender-marble junction whose east/west arms turn north. Emulator composition check remains open. |

### P3 — Smaller story mismatches

- **Resolved in later art passes:** Z1 #93 staircase, Z1 #18 white-marble pedestal, Z3 #5/#176 open south door, and the dry Z3 #163 → #93 channel match are corrected in the current assets.
- **Resolved Nov 20:** Z1 #202 `z1_rainbow_top` now uses a softer, mist-lit arc that fits the waterfall palette.
- **Refreshed Nov 20:** Z1 #16/#72 ladder views now use a matching timber color and rail/rung construction; the different views show the ladder descending into and rising out of its opening.

### P3 — Fallback art status

| Room | Fallback | Why it doesn't fit |
| --- | --- | --- |
| Z1 #102, #145 Twisting/Winding Passage | `cavern` | Plain passages; acceptable shared art, though not individually distinctive |
| Z1 #32, #87, #221 Forest/Path | `forest` | Shared forest art remains acceptable for these descriptions; #167 has its own mountain-reveal scene and #238 its own low-branch scene |
| Z3 #155/#177 Dark Place, #228 Drafty Room | `hall` | N/A in play: the story engine blacks out the art band while these rooms are dark |

The previously listed fallbacks for Z1 Studio, Deep Canyon, Shore/Stream View/Stream,
Forest Path 238, and the Z2 angled/Murky rooms are resolved by dedicated scene wiring.

### P3 — Wiring hygiene

- **Resolved in the sprite/wiring follow-up.** The unused generic mirror and parapet scenes,
  old Z3 hallway/mirror scenes, and oversized `gazebo.png` were moved to
  `docs/art-history/encounter-options/retired-backgrounds/`; their scene declarations, floor rows,
  test stubs, and build entries were removed. The active Z3 list is now explicit in the
  Makefile, so new comparison drafts are not included automatically. Room 16 uses
  `z2_gazebo.png`.
- **Resolved.** Dead name fallbacks for Gazebo, Formal Garden, North End of Garden,
  Topiary, Deep Ford and Dragon's Lair were removed. Their room IDs already select the
  corresponding Z2 scene entries. The separate Dragon Room fallback remains for its
  distinct room.

## Passed on inspection

- The kitchen/behind-house window pair, living-room rug and trapdoor states, and cellar hatch states.
- The dam and reservoir state sets.
- The Guarded Room/Workshop door: right-jamb hinge, swings into the Workshop.
- The Marble Hall/Prison Cell door: same leaf, consistent swing.
- Crypt anteroom closed and open.
- Ice Room/melted: camera and architecture preserved.
- The Wizard's Quarters variants share one camera and exit.
- The narrow-mirror set and its derived mirrors.
- The cliff top/ledge/base chain and the lake surface/underwater/shore set.
- Mist rooms, volcano set, angled-room set (#19 etc.), and bank rooms.

## Suggested order

1. Finish the remaining paired-room art review for the Barrow footbridge, Hallway 169 and South Corridor, comparing each view against its connected room.
2. Complete the interaction, item-drop, leave-and-return, and save/restore checks in the screen QA checklist.
3. Rebuild the full mGBA gallery when the next complete route audit is run; the Nov 20 refresh covers the seven approved art changes and the three shared-door states.

## Fixes after the full verification

- **Fixed: Oddly-angled rooms 75, 121, 202, 242.** All eight window rooms now show the diamond floor window and follow the story's dark/glowing state (global 13). The replay test checks all eight in both states. The target-box audit is now 986 states, with 0 missing and 0 out of band.
- **Fixed: Z3 163 Water Slide.** It is back on the dry `z3_water_slide`, matching Aqueduct 203 and Damp Passage 93. The wet `_v2` image and its scene, floor row and stub were removed from the build.

- **Fixed: Z3 93 Damp Passage.** Rubble (from the matching Water Slide rock) now blocks the channel's northern opening.
- **Fixed: Z3 204 Prison Cell.** It uses the new `z3_prison_cell_bronze`: the far (south) wall's studded door is now bronze. Cells 45 and 225 keep the wooden version.
- **Fixed: Z3 170 Land of Shadow.** The clifftop now has a visible switchback cut into its west face, descending to a sandy beach and surf far below. The north ruined wall still ends at the cliff edge; the cool shadow palette and clifftop floor remain intact.
- **Revised: Z3 169 Hallway after route check.** Going north from Hallway 169 enters Inside Mirror 160. The open panel's plain wooden back now leans against the east wall, with the red/black and blue wooden room visible through the opening. The floor channel and compass remain clear, and the target boxes now select the board. This replaces the prior direct reflection; the gallery screenshot still shows an older build and is not evidence for this view.
- **Fixed: Z1 93 Smelly Room** (the ragged hole is now a stairwell descending away from the viewer), **Z1 18 Torch Room** (white marble pedestal), **Z1 31 Slide Room** (the north opening now reaches the floor as a low walkable passage), and **Z1 127 Troll Room** (the west hole is now an adult-sized rough opening at floor level).
- **Reclassified as N/A: Z3 155/177 Dark Place and 228 Drafty Room.** These rooms are dark in play: the route log shows `dark=1`, and `src/trilogy.c` fills the art band with black when dark. The fallback hall image is never shown.
- **Artwork gaps listed in this review are filled.** New Z1, Z2, and Z3 backgrounds are
  wired, and the Studio manual has a wall anchor. The Nov 20 current-ROM mGBA refresh now
  covers the seven approved art updates and three shared-door states (11 captures total).
  Still needed: a paired in-game recheck of the Barrow footbridge and the
  interaction/drop/leave-return/save-restore rows in the screen QA checklist.

## Sprite and overlay audit (Nov 19)

**Method.** A frame-dumping copy of `tests/transcript_replay_test.c` (kept in the session scratchpad, not the repo) captured the code-drawn overlay layer for every scene state reached by the three winning routes and the off-route checks. That gave 446 unique states, 325 of them with visible overlays. Each overlay was composited onto its real background, and up to three states per room and background were reviewed at the in-game crop (239 frames). The checks covered: items on supports, character scale and contact, the same entity across rooms (dragon, princess, robot, old man, hooded figure, wizard, Cerberus, serpent), and duplicated or clashing visuals.

**Findings and fixes**

- **Fixed: duplicate floor window in the Oddly-angled rooms (introduced earlier in this review).** `src/room_visuals.c` already draws the diamond window in code, and its brightness follows story global 58. My baked-in window images therefore drew a second window over it in rooms 75/121/202/242.
  - The story actually has six window states: dark, flickering dimly, dimly glowing, glowing and glowing brightly (global 58 = 0–4), plus "glowing serenely" once the passage opens (global 13).
  - The code overlay is the right mechanism, so it now covers all eight window rooms, and global 13 drives its brightest state.
  - Backgrounds are back to the original window-less art: 75/121/202 are byte-identical to their pre-edit crops, and 95/142/185/226 share 242's `z2_angled_northeast`. The baked `z2_angled_window*` and `*_lit` images were retired, and their focus boxes removed in favour of the code-drawn one.
  - The off-route test now walks all six states in all eight rooms. The target-box audit is 1018 states, with 0 missing and 0 out of band.
- **Fixed: white matte fringes on three item sprites.** Semi-transparent light edge pixels above the converter's alpha cutoff (112) showed as white speckles: the open ritual book on the Altar (Z1 129), the corroded tool chests in the Maintenance Room (Z1 224), and the broken egg. The edges were cleaned in `assets/items/`, and the originals are kept in `docs/art-history/encounter-options/retired-sprites/`. Light-coloured items (pearl necklace, skull, white book, sphere, trident) were checked and are not matte.
- **Resolved, not an issue: Z3 192 / Z2 207 Room 8.** The east "blur" exit is drawn by code at the right edge, so the room shows its only exit on the correct side. The crawl at left is the one you arrive through.

**Passed:** item contact on tables, floors, shelves, pedestals and containers in every reviewed room; dropped-item placement (Living Room, Guarded Room, Pentagram Room, Library, Pool Room multi-drops); boat, balloon and basket states; consistent character sprites across rooms and poses (troll standing/fallen, dragon awake/asleep, princess standing/seated, hooded figure standing/kneeling/defeated); Cerberus collared and uncollared; and the aquarium serpent alive and dead.

**Not covered:** states that no route or off-route check reaches. Item focus highlights were checked only by the existing box audit.

## Item sprite and asset inventory audit (Nov 19)

**Wiring.** (Since this review, the unused grate sprite has been retired to `docs/art-history/encounter-options/retired-sprites/`; the grate is drawn by the room backgrounds.) All 152 compiled item poses from `assets/items/frames.json` and 32 encounter poses from `imagegen-sprites/frames.json` are generated and referenced by `src/room_visuals.c`. Every sheet listed in the manifests exists, and the only unlisted PNG is `new_props_source.png`, which is the documented generation source. Eight more sprites are hand-built in code (lunch, coal, broken timber, broken staff, hood, burned torch, oil lamp, and the Z3 pedestal).

**Story match.** The initial comparison of all 131 `ART(chapter, object, pose)` bindings
flagged the following mismatches:

| Object | Sprite | Problem |
| --- | --- | --- |
| Z3 #174 "jewelled knife" (one of the Crown Jewels) | `KnifeOld` | The same plain, worn knife as Zork I's rusty and nasty knives; no jewels |
| Z2 #138 "blue label" | `LeafletFolded` | A tan folded leaflet; should be blue |
| Z1 #91 "burned-out lantern" | `BrokenLanternFloor` | Intact but spent, yet depicted with cracked glass |
| Z1 #165 "nasty knife" | `KnifeOld` | Shares the rusty knife's worn look; the attic knife isn't described as rusty |
| Z1 #96 "skeleton key", Z2 #127 "delicate gold key", Z3 #13 "strange key" | `KeyGold` | One gold key for three different keys |

**Resolved in the follow-up:** Z3 #174 now uses the jeweled knife, Z2 #138 the blue label,
Z1 #91 the intact unlit lantern, Z1 #165 a dark-steel nasty knife, and Z1 #96 the iron
skeleton key. Z2 #127 remains the gold key; Z3 #13 now has a distinct aged, irregular key.
Z1 #232 continues to use the rusty-looking old knife. The four new sprites are documented
in `assets/items/sprite-audit-prompts.md` and wired in `assets/items/frames.json`.

**Background inventory, after follow-up.** Unused generic mirror/parapet/gazebo sources and
the old Z3 hallway/mirror variants were moved under
`docs/art-history/encounter-options/retired-backgrounds/`. Their five scene symbols, floor entries
and host-test stubs were removed. The Makefile now lists production Z3 scenes explicitly,
so comparison drafts are not picked up by a wildcard. `z2_gazebo.png` remains the room 16
production view; the unreachable name fallbacks for Gazebo, Formal Garden, North End of
Garden, Topiary, Deep Ford and Dragon's Lair were removed. Production encounter sprites remain in the documented
build source folder `assets/encounter-options/imagegen-sprites/`; `assets/SCENERY_RULES.md`
now calls this active production subfolder out explicitly.

## Encounter sprites and edited-route continuity (Nov 19)

**Aspect ratio.** `tools/encounters.py` resizes five encounter cells: banker, volcano gnome, Cerberus, sailor and the unicorn's garden pose. Measured on the alpha-trimmed silhouettes, as the scenery rules require, each keeps its shape within 7% (the sheet cells are padded). Pass.

**Entity identity.** All 32 encounter poses were reviewed per character. Troll (armed, unarmed, down), cyclops (awake, asleep), bat (flying, hanging), dragon (three poses), princess (standing, seated), old man (awake, asleep), hooded figure (standing, kneeling) and cliff man share head silhouette, build and materials. The friendly Cerberus palette was restyled to match the guarding pose's muted russet and clustered shading while keeping its three heads, relaxed pose and chest chevrons. The previous palette is preserved in `docs/art-history/encounter-options/retired-sprites/`.

**Continuity of rooms edited in this review.**

- **Slide Room → Mine Entrance:** the north opening is a low passage at floor level, matching the mine's timbered entry. Pass.
- **Troll Room west hole → Cellar north passage:** a rough adult-sized opening in both. Pass.
- **Damp Passage:** earlier audits read this camera as facing north, so the rubble sits on the correct (north) opening, and the slide's channel arrives from behind the viewer. Pass.
- **Fixed (my edits):**
  - The Smelly Room stairs were flat grey, unlike the tan stone stairs of the Gas Room below. They are now textured from the room's own floor, with lit edges on each step.
  - The Torch Room pedestal recolour had missed its shading pixels, leaving brown speckles. The original art was restored exactly from `build/story-dump/crops/` and the pedestal recoloured inside its traced outline, giving a clean white marble block. Build and `make test` pass.

## Full story-text verification (Nov 19)

Every story room with a name was paired with each background the game can select for it (316 room/state pairs across 278 rooms). Each pair was viewed at the in-game 240×76 crop beside the room's own `look` text from `build/story-dump/zork{1,2,3}-ldescs.json`. Rooms that resolve by code paths (dam, reservoir, crypt, quarters, prison, mirror variants) were checked in every state.

**Result:** about 290 pairs match their text. The rest are listed below.

### Mismatches found in this pass

| Room | Art | Story says | Art shows |
| --- | --- | --- | --- |
| Z2 75, 121, 202, 242 Oddly-angled | `z2_angled_northwest/southeast/west/northeast` | "On the floor is a very small diamond shaped window" (all eight non-stair angled rooms) | No window. Only the new `z2_angled_window` rooms (95/142/185/226) show it, and they also follow the dark/glowing state. |
| Z3 163 Water Slide | `z3_water_slide_v2` | Aqueduct 203: the channel "has long since dried up"; Damp Passage 93 receives a mossy dry channel | Running water into the hole. The dry `z3_water_slide` was the consistent version. This settles the open question from the earlier audit. |
| Z3 170 Land of Shadow | `land_of_shadow` | "Atop a steep cliff, looking west over a vast ocean… surf pounds at a sandy beach… a tall stone wall… ends at the cliff edge" | The shared rolling-hills image with no cliff, sea or wall |
| Z3 93 Damp Passage | `z3_damp_passage` | The channel's northern continuation "is now blocked by rubble"; junction to the west, passages east and northeast | The channel runs into an open dark tunnel; no rubble and no side passages |
| Z3 204 Prison Cell | `z3_prison_cell_closed` | "On the south wall is a bronze door which seems to be closed" | The same cell as 45/225 with no bronze door |
| Z3 169 Hallway | `z3_mirror_hall_v3` | Still open from the first pass | — |
| Z3 155, 177 Dark Place | `hall` (fallback) | Dark rooms (the story prints no description) | A lit arched stone hall |
| Z3 228 Drafty Room | `hall` (fallback) | No description returned with forced light; reached between Timber Room and Machine Room | Generic hall |
| Z1 25 Damp Cave | `cavern` (fallback, after the keyword fix) | West and east exits, "narrows to a crack toward the south" | Generic mossy cave with two openings; acceptable but no crack |
| Z1 40 Shore, 67 Stream View, 86 Stream | `water` (fallback) | A river shore path; a path beside a gently flowing stream; on the stream between twisting walls | A still underground lake in a stalactite grotto |
| Z1 162 Deep Canyon, 220 Studio | `cavern`, `hall` (fallback) | Canyon edge with a stairway down; paint-splattered studio with a fireplace | Generic cave; generic arched hall |

Still open from the first pass (re-confirmed): Z1 31 Slide Room opening, 127 Troll Room west hole, 244 barrow footbridge, 93 Smelly Room staircase, 18 Torch Room pedestal, and the Technology Museum's missing open south door (Z3 5/176).

### Minor omissions (P3)

- Z2 248 Dragon Room: the "huge stone bridge to the south" is not shown.
- Z3 116 Royal Hall: the carved Royal Seal of Lord Dimwit Flathead is not visible.
- Z3 192 Room 8: the only exit is east, but the crawl is drawn on the left.
- Z1 238 Forest Path: the "particularly large tree with some low branches" is missing from the shared forest image.

### Verified as matching

All other rooms, including every state variant of the house, dam and reservoir, coal mine, maze and dead ends, river and cliffs, rainbow and falls, Zork II gardens, bank, volcano, crypt, quarters, mist spheres and aquarium, and Zork III lake, aqueduct, cliff chain, museum, jewel rooms, guardians and narrow-mirror rooms, puzzle, prison and parapet.

Limits: this checks backgrounds against room text only. Item and character sprites, which carry many of the objects the text mentions, were not composited. The Narrow Room sheets showed the base mirror art; the code's variant choice (guardians/mirrored) was checked against the text separately.

## Exact duplicate state screenshots (Nov 19)

I checked all captured gallery PNGs by exact file hash, grouped by game and room. There are 61 duplicate groups (68 extra rows), but most are alternate checkpoints for the same story moment. The gallery now labels exact duplicate frames so a state name does not imply a visible change when the screenshot is identical.

**Visual gaps addressed and recaptured in mGBA:**

- **Zork I, Dam Base:** entering the magic boat now switches from the shore-side boat sprite to a shallow first-person raft rim, with the river still visible. The boat-on-land frame differs; the inside-boat and pre-launch captures are the same state after entering.
- **Zork II, Menhir Room:** the short-lived enchantment after waving the wand now adds a faint green glow and a green carved `F`. The blocking and enchanted frames differ; `menhir-enchanted` and `menhir-before-float` are duplicate saves from the same turn.
- **Zork II, Cage:** the floor-level poison haze is now visible behind the cage bars. The game starts the gas on the same turn the cage falls, so `cage-entered`, `cage-poison` and `cage-trapped` are expected to remain identical.

All nine refreshed screenshots reached their expected chapter and room in mGBA. The capture report is `qa/mgba-room-gallery/state-cue-refresh-results.csv`; the gallery's exact-duplicate labels are produced by `qa/mgba-room-gallery/finalize_gallery.py`.

## QA gallery refresh (Nov 19)

A full mGBA capture pass ran the new `build/mgba-room-captures/run-gallery.lua`. That wrapper clears range and report overrides left by earlier refresh runs, loads the regenerated plan, and starts the queue. All 499 entries were captured. `build_plan.py` scene mappings were corrected for rooms the code now maps directly:
- Zork I: 25, 40, 67, 86, 110, 162, 220 and 238.
- Zork III: 66, 170, 179, 204 and 218.
- Zork III 169: the route save is the closed-mirror state.

Of 486 lit frames, 481 match their manifest scene, and the 5 exceptions are expected state or UI views. The edited regions of this review's art (Torch Room pedestal, Smelly Room stairs, Slide Room opening, Damp Passage rubble, bronze cell door, clifftop) match the current assets at 99–100%.

## Double target boxes (Nov 19)

**Cause.** `draw_room_target_anchor` drew every `anchors[]` row matching the focused object, and sprite or special-case code could also frame the object. As a result, 27 focus states drew two or three visibly different boxes. The Crypt's three targets each had a stale second row tuned for older art (sarcophagus in front, doorway at right). The replay QA only checked for at least one box, so none of this was caught.

**Fix.**
- **One box per target:** `corners()` in `src/room_visuals.c` now draws at most one box per frame (redrawing the identical box is allowed), reset by `draw_room_details`/`draw_puzzle_map`. Sprite and special-case boxes come before the table, so they win.
- **Duplicate rows:** all 22 duplicated `anchors[]` keys were reviewed against the room's mGBA screenshot. The row on the object the story names was kept, and 23 stale rows were removed. The Crypt's poled heads and the Stone Barrow got new boxes covering the real object.
- **Test:** `tests/transcript_replay_test.c` now fails if a focused target gets two distinct boxes. With the one-box rule disabled, it fails on the Troll Room axe.
- **Related outside regressions, also fixed:** a missing `art_cliff_ledge_upper_shelf` stub; the Prison Cell Dungeon Master drawing the name-row frame a second time; the in-boat box running past row 90; and the Cliff Ledge man's box 1 px above the art band.

`make test` passes: 1018 states, 0 boxes outside the band, at most one box per target. Gallery frames whose first focused target had a duplicate (Crypt, Aragain Falls, Reservoir South, Chasm, Machine Room, Attic, Parapet, the Zork III museum plaques, Narrow Corridor, lake rooms) still show the old double boxes until the next capture pass.

## Gallery coverage and recapture (Nov 19, 23:58)

- **New rooms:** `qa/mgba-room-gallery/route_saves.c` builds save checkpoints for rooms no traced route stopped in. It replays the deterministic winning-route fixture, walks the story's static exits (or a short recipe, such as carrying the boat to Stream View), and keeps a save only when the live story reports the room lit at a prompt. 29 rooms were added: 25 in Zork I (all mazes and dead ends, forests, river and stream rooms, Damp Cave, Deep Canyon, both White Cliffs Beaches, Studio) and Zork II angled rooms 95/142/185/226. All restore through `save_read` to the correct lit room.
- **Murky Room (Zork II, 237):** it is fatal to enter in both tank states (serpent / broken glass), so it is documented as a gap with a probe log.
- **Recapture:** a single uninterrupted 527-entry mGBA pass on the 23:28 ROM had 0 failures. Zork I now covers all 110 named rooms. No frame shows more than one in-picture target box, and every lit frame matches its scene or the code's state variant for that save.
- **Earlier interruption:** the first rerun was interrupted after 53 entries when another process restarted mGBA for its own refresh. The capture wait now aborts if mGBA's process changes or the ROM is rebuilt mid-run.

## Story-match re-check from mGBA frames (Nov 20)

This pass used the 23:58 mGBA captures, which show exactly what the game displays. It covered all 57 rooms whose art changed since Nov 19, that were newly captured, or that earlier passes flagged. Rooms with unchanged art keep their earlier "matches" verdict.

**Now matching (previously flagged):** Damp Cave (crack south), Shore, Stream View, Stream, Deep Canyon, Studio, Forest Path (large low-branched tree), Torch Room (white marble), Smelly Room (stairs), Troll Room (west hole), Slide Room opening, barrow ending (bridge joins both banks), Dragon Room (stone bridge), Royal Hall (Royal Seal), Technology Museum (open door), Damp Passage (rubble), Water Slide (dry), Hallway 169 (mirror fills the north end; open state separate), Land of Shadow 170, Prison Cell 204 (bronze door), South Corridor, Cliff Ledge, all maze rooms and dead ends, and all eight Oddly-angled rooms (floor window).

**Earlier findings and Nov 20 disposition:**

| Room | Problem |
| --- | --- |
| Z2 15 Lava Room | **Resolved:** the east passage shows the post-melted route. |
| Z2 46/59 Tiny/Dreary door | **Resolved:** paired views show the door swinging into Dreary Room. The story does not specify the swing direction. |
| Z1 167 Forest | **Resolved:** new art shows the thinning forest and impassable mountains. |
| Z1 16/72 Ladder Top/Bottom | **Refreshed:** matching wood and ladder construction are visible in both views; perspective changes which section is in frame. |
| Z1 202 On the Rainbow | **Resolved:** the rainbow now has a softer arc and waterfall mist palette. |
| Z1 31 Slide Room | **Resolved:** the south wall now shows the Granite Wall inscription. |

**Overlay bug (not art):** Z1 86 Stream drew a second boat on the beach while the player was aboard. Fixed in the generic loose-item renderer by suppressing the shore-side boat sprite whenever the player is inside it. The Nov 20 mGBA screenshot shows the interior raft rim and no second boat.

**Approved art installed and captured Nov 20:** Forest 167 mountain reveal, post-melt Lava Room passage, paired Tiny/Dreary door swing, refreshed ladder views, softer rainbow, and Slide Room inscription. The rebuilt ROM was captured in mGBA; all 11 targeted captures reached their expected chapter and room. See [art-followup-refresh-results.csv](../../qa/mgba-room-gallery/art-followup-refresh-results.csv) and the [updated gallery](../../qa/mgba-room-gallery/index.html). The ladder pair is refreshed, though the ladder is seen from opposite ends and only the visible portion can be compared.

Acceptable as is: Twisting and Winding Passages (102/145) use the generic cave, which fits "a winding passage". The Murky Room cannot be entered, and the dark rooms 155/177/228 show black.

## Full re-review against story text and neighbours (Nov 20, 07:30)

All 275 named rooms were reviewed. Each room's text was shown with every captured state from the 23:58 mGBA pass, plus the art of each room its exits lead to. For the seven backgrounds that changed after that pass, the current asset was used instead: Ladder Top/Bottom, Lava Room, Tiny Room open, Forest 167, Slide Room, and On the Rainbow.

**Previously open, now fixed in the current art:** Lava Room (no glacier east), Tiny/Dreary door (the Tiny Room's open view now swings away into the Dreary Room), Forest 167 (mountains), Ladder Top/Bottom (matching ladder), Slide Room ("Granite Wall" etching), On the Rainbow (softer style), and the Stream's duplicate boat (re-captured at 00:44). These seven still need an mGBA capture to confirm in game.

**Still not matching:**

| Room | Problem | Severity |
| --- | --- | --- |
| Z3 54 Hallway | "A large mirror fills the south side of the hallway." It shares `guardians_approach` with Hallway 224, which shows an open corridor and no mirror | P2 |
| Z3 136 Great Door | It draws a wooden door at right and a rock cleft at left. The text has only the north exit and the rusted iron door east | P3 |
| Z3 44/54/224 guardian hallways | The text's shallow channel and compass-rose hole are not visible (the matching Hallways 113/169 show them) | P3 |
| Z3 69 Land of Shadow | "Bordered to the north by a massive stone wall … crumbled at one point". It uses the generic hills art | P3 |
| Z3 216 Southern Shore | The "dark passage sloping steeply upward" to the south is not visible | P3 |
| Z2 238 Riddle Room | The "exit down in the northwest corner" is not shown | P3 |

**Consistent across neighbours and states:** the house exterior ring and window states; forest/clearing/canyon/rainbow; the mine, ladder and coal-mine chain; dam, reservoir, river and boat views in order; maze and dead ends; temple/Egyptian/altar; Zork II gardens, bank wing, volcano and balloon ledges, Wizard's rooms and quarters variants, crypt states, mist spheres and aquarium states; and Zork III lake/shore/underwater, aqueduct chain, cliff chain, Land of Shadow → clifftop → ocean, museum and puzzle entrances, marble corridors, prison/parapet, and all narrow-mirror rooms (mirror on the side the text names). Generic Land of Shadow art for the hill rooms fits their shared description.

**Not reviewable:** the Murky Room (fatal to enter), the dark rooms 155/177/228 (black by design), and the documented transient or trap rooms in Zork III.

## Room-screen focus-box coverage and correspondence (November 22, 2026)

The replay audit now proves the gold focus box is around the right thing, not merely that
one exists. Every draw path that can render the focused object reports its actual sprite
rect through `visual_qa_focused_sprite`; the audit requires:

- exactly one scene box and one name-row box per focused target;
- every gold box to report itself with the focused object (presence and identity);
- where a sprite draw reports its actual rect, the box to contain it within the border;
- the box to stay inside the visible art band (rows 15–90);
- zero targets without a box report unless explicitly allowlisted.

Current result: 1,017 states, 4,410 focused targets, 4,412 box reports, 1,925
independent sprite rects and 2,487 scene-feature rects, 0 missing, 0 out-of-band,
0 mismatches. The room-target visibility audit now treats “isn’t here” / “is not here”
as absent, with a documented exception for the Master outside a shut cell door. The
allowlisted targets are carried or contained items with no separate scene footprint.

Phantom-target corrections: the North Corridor/Parapet Master proxy is filtered when
the actor (object 1) is absent, and the Kennel’s room-local mention of the Menhir Room
stone is filtered there because the Kennel text and art show only the collar and bones.
The actor Master still draws and boxes when he is following the player.

Fixes that came out of it: encounter sprites and boxes are clamped into the art band
(Zork II's garden unicorn framed above row 15) and the floor shadow now follows the
clamped sprite; the Zork III prison cell door anchor was moved right and up into the
band; a `loose_extras` table draws the shared coal/diamond where their object-tree parent
is another room. Carried and contained items (thief's loot, nest egg, basket contents,
machine coal) are not drawn on the floor; they stay name-row only. The attic knife and
stair railing already had placement rows, but those rows used the wrong parents; with
the parents corrected they draw on the table and at the left of the attic.

Name-row-only allowlist (carried, contained, or no scene art): Zork I Treasure Room bag
and egg, Machine Room coal, Strange Passage bag and stiletto, Shaft/Drafty coal, Cellar
chute; Zork II Aquarium sphere; Zork III Jewel Room stone door; and the Dreary Room lid
while the door is open. These keep the L/R affordance and are boxed in the name row.

In the North Corridor and Parapet the stale Master proxy is filtered from the target
list entirely when object 1 is absent, so the name row never offers a Master who is not
in the room. When the actor is present, his sprite is drawn and boxed.

When the Master is outside a shut prison-cell door, he is now boxed on the barred
window in that door: the cell shows the corridor only through that opening, and the
story still lets the player address him from inside the cell (`Dungeon Master
(outside)`). With the door open in room 45 the box follows his distant corridor sprite
instead. The three cell views use different door objects (183, 153, 72), so the check is
per-room rather than hard-coded to room 45.

### Scene-anchor placement pass

The automated audit can prove presence and sprite correspondence, but scene-feature
anchors frame painted scenery and therefore need visual review. Rendering representative
room frames with the real art exposed four anchors that pointed at the wrong part of the
picture; all were corrected:

- Living Room carpet: the box now follows the rug's two arrangements — spread across
  the floor while the trap door is hidden, and the rolled bundle at the left after the
  rug is moved.
- Kitchen and Attic tables: the boxes covered only the left part of the tabletop; both
  now span the full table edge.
- Zork III Parapet sundial: the box stopped short of the dial's right limb and top.
- Zork III Hallway (113/169) compass rose and stone channel: the boxes were on the
  mirror frame or bare wall above the floor carving.
- Compass roses in Hallways 113/169 and the Inside Mirror room 160 now frame the whole
  floor medallion (the story's "compass rose engraved around the hole"), not just its
  central star, and the channel boxes sit on the visible groove.
- The other guardian hallways (44, 54, 224) now box their compass rose and stone
  channel using the same floor rectangles as the matching art. Room 54's mirror/panel
  use the mirror-hall box; its south wooden wall remains name-row only because it is
  behind the camera.
- **Guardians of Zork (152)** are now addressable as “guardians” in hallways 44/54/224
  and are boxed as one frame around the pair of statues. The narrow-mirror rooms and
  the Inside Mirror list the same local name but do not draw the statues, so the target
  is filtered there rather than boxed on empty scenery.

The attic and coal-room views still allowlist scenery the art does not draw, and the
Cellar chute remains name-row only because the cellar view shows an archway rather than
the ramp. `make test` reports zero missing boxes and zero mismatches after the pass; the
ARM ROM builds and contains no `visual_qa` symbols.

## Follow-up focus-box alignment (November 22)

Adjusted the remaining scene anchors called out in the latest screenshot review:
Zork I Machine Room switch raised; Dam control-panel box widened over the full panel;
Studio manual moved left onto the painted wall clear of the doorway; Zork II Tiny Room
keyhole lowered; Pentagram target moved down onto the floor marking; Dusty Room hole
snapped to the small aperture and the explosion debris target lowered to the rubble;
Crypt Anteroom door narrowed and extended to the floor; Aquarium box tightened around
the tank and raised to its top; Riddle Room stone-door box moved down and widened to the
door face. `make test` reports zero target-box mismatches, and the ARM ROM builds.
