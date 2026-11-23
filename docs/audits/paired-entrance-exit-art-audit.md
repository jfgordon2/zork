# Paired entrance, exit, and door-state art audit

Reviewed November 18, 2026. This pass compares current background PNGs, room-to-art state selection, story descriptions, and existing route captures. It covers both closed/open views within one room and views from the connected room. At the time, the review used existing route captures and the later full container build did not include a fresh mGBA replay. That evidence gap was closed in the November 22 current-ROM refresh: 528 full-gallery frames and eight targeted Zork III route/UI frames were captured in mGBA. See the [route-driven QA update](visual-route-qa-audit.md) and [Zork III route register](zork-iii-route-coverage.md).

## Summary

The reviewed Zork I house hatch/window pairs and several Zork II/III route sets read as connected. The initial pass found seven follow-ups; all seven have been addressed in the source artwork. The Zork I window pair has since been updated to a vertical slider, alongside the Zork I dam, both Zork II door pairs, and the Zork III prison, Museum Entrance, ladder, and underwater lake views.

**ROM integration status, November 18:** `make all` completed inside the new ARM64 Podman build image. An exact RGB555 comparison confirmed the production scenes in that build were embedded in `zork-trilogy.gba`; it also included the Dam states, underwater update, and newer scene additions. A later asset-wiring review found three obsolete source drafts with no room/state mapping; those files have now been removed. `zork-white-house.gba` also rebuilt successfully with its eight base scenes.

## Open/closed comparisons within a room

| Chapter and state pair | Result | Notes |
| --- | --- | --- |
| **Zork I: Behind House window / Kitchen window** (`rear.png`, `z1_rear_open.png`, `z1_kitchen_shut.png`, `kitchen.png`) | Pass, updated | All four states now use the same intact four-pane vertical slider. The lower sash travels upward in both views; open views retain the top panes and show the lower aperture. Room selectors still share object 243's open flag. |
| **Zork I: Living Room trapdoor / Cellar hatch** (`living_moved.png`, `z1_hatch_open.png`, `z1_cellar_hatch.png`, `z1_cellar_hatch_open.png`) | Pass, sampled | Opening replaces the floor panel with descending stairs; from below the same opening reads as an overhead hatch with stairs rising to it. The open/closed changes preserve the room framing. |
| **Zork I: Dam and reservoir water states** (`dam.png`, `z1_dam_open.png`, `z1_dam_low.png`, `z1_dam_refill.png`, `z1_reservoir_*`) | Pass, updated | All four Dam states now share the same four arch openings, piers, parapet, perspective, and control platform. Open/high shows flow through the arches; closed/full retains the gates and shows overflow across the crest to match the story; low/open exposes the basin; low/closed shows a shallow refilling pool. Reservoir north/south retaining-wall layouts remain unchanged. |
| **Zork II: Guarded Room / Workshop door** (`z2_guarded_room.png`, `z2_guarded_open.png`, `z2_workshop.png`, `z2_workshop_open.png`) | Pass, updated | The Guarded Room closed view now uses the same vertical oak boards and plain horizontal iron straps as the reverse Workshop face. Its clear center remains available for the separately rendered lizard-head item; open views and swing remain unchanged. |
| **Zork II: Tiny Room / Dreary Room door** (`z2_tiny_room.png`, `z2_tiny_open.png`, `z2_dreary_room.png`, `z2_dreary_open.png`) | Pass, updated | The open view from Tiny Room now shows a foreshortened Dreary Room table and the red-lit crack beyond the same barred oak door. The reverse view into the bare Tiny Room remains appropriate. |
| **Zork II: Marble Hall / Formal Garden passage** (`z2_marble_hall.png`, `z2_marble_open.png`) | Pass, sampled | The open state reveals the green hedge route and keeps the stone landing and floor connected to the arch. |
| **Zork II: Riddle Room stone door** (`riddle_room.png`, `riddle_room_open.png`) | Pass, sampled | The closed stone face becomes a same-width opening with descending steps. The lintel, wall, and floor remain aligned. |
| **Zork III: Prison Cell wooden door** (`z3_prison_cell_closed.png`, `z3_prison_cell_open.png`) | Pass, updated | The closed cell door has hinges on its left edge and its ring latch on the right. The open pose keeps the leaf seated against that jamb, hides the hardware, and places the latch on the left side of the swung leaf. Its through-view shows the east-west marble corridor, parapet, and fiery pit; the closed window shows distant flames. |
| **Zork III: Museum Entrance stone door** (`museum_entrance.png`, `museum_entrance_open.png`) | Pass, updated | The open central doorway now reveals the Jewel Collection's large barred cage and blue-green hall at a plausible distance; the entrance threshold and side arches remain fixed. |
| **Zork III: Engravings Room secret door** (`engravings.png` plus runtime stair overlay) | Pass, sampled | The opened state adds a recessed stairwell at the carved wall without shifting the rest of the room. |

## Connected-room comparisons

| Connection | Result | Notes |
| --- | --- | --- |
| **Zork I: House ↔ Kitchen; Living Room ↔ Cellar** | Pass, sampled | Window and hatch state changes agree from both sides. Existing route and Save/Restore captures cover opening, entry, return, and the story closing the hatch after descent. |
| **Zork I: Forest Barrow ↔ Inside the Barrow** | Pass, sampled | The exterior forest entrance transitions to the illuminated underground cavern; the interior's rock jambs, stream, and bridge make the change of space legible. |
| **Zork I: Dam ↔ Reservoir North/South** | Pass, updated | The four Dam state images now retain a single architectural design while gate flow and basin level change. Connected reservoir scenes remain unchanged. |
| **Zork II: Guarded Room ↔ Workshop** | Pass, updated | Both closed views now read as the two faces of one oak door; the existing connected-room views and door swing are unchanged. |
| **Zork II: Tiny Room ↔ Dreary Room** | Pass, updated | The open view from Tiny Room now shows the connected Dreary Room's table and red glow; the reverse view remains the bare Tiny Room. |
| **Zork III: North Corridor 19 ↔ Prison Cell 45** | Pass, updated | The prison entrance now sits left of the balcony in the corridor view. Room 19 switches between closed `z3_marble_hall` and `z3_marble_hall_prison_open` using object 183's open flag. From the corridor, the leaf swings into the cell and reveals its plain back wall; the bronze door on the perpendicular wall appears only from the cell-side camera. The open cell-side view shows the marble passage, parapet, and fire. |
| **Zork III: Museum Entrance ↔ Jewel Collection** | Pass, updated | The open entrance view now shows the caged collection room beyond the continuous stone threshold. |
| **Zork III: Ladder Top 20 ↔ Ladder Bottom 57** | Pass, updated; route-limited | Both openings now sit center-right and use matching rail width, rung rhythm, timber tone, and rectangular construction. Existing route evidence still cannot provide a stable player-controlled comparison: room 20 has no stable route capture and room 57 is a transient Scenic Vista visit. |
| **Zork III: Lake 184 ↔ Underwater 12 ↔ Western/Southern Shores 140/216** | Pass, updated | The underwater scene now has a continuous water surface across the top and natural rock only along the side banks. The turquoise water, sand, plants, and algae read consistently with the connected lake and shore views; the central sand remains open for the separate amulet cue. |
| **Zork III: Aqueduct 203 → High Arch 182 → Water Slide 163 → Damp Passage 93** | Pass, sampled | The connected channel, supported height, sloping slide, rubble, and landing are legible as one route. The route report and current connected-scene board agree on stone materials and floor continuity. |
| **Zork III: Button Room 98 ↔ Beam Room 6** | Pass, sampled | The corridor opening and low beam line up at floor height in both views. |
| **Zork III: Royal Puzzle Entrance 84 → Side Room 130 / 188** | Pass, sampled | Route captures show a floor-connected west passage into the dedicated Side Room. Its north steps and east steel door remain separate exits. |

## Follow-up resolutions

### Resolved · Z1-PE-01 · Dam state family

Updated `assets/dam.png`, `assets/z1_dam_open.png`, `assets/z1_dam_low.png`, and `assets/z1_dam_refill.png` from a shared four-arch structure and camera reference. A native 240×102 comparison confirms that the arches, piers, parapet, and foreground control platform register as one dam while the gate and water states remain distinct. The closed/full view also shows overflow across the crest in keeping with the Zork I text. The room-state selector in `src/chapter.c` still maps the four combinations to the intended art.

Validation: all four PNGs are 240×102; their converted RGB555 buffers are each 48,960 bytes; `make test` passed. The normal PNG conversion target could not run because the host Python lacks Pillow, and a full ROM build could not run because `arm-none-eabi-gcc` is unavailable. The changed art was downsampled and visually reviewed at native background size; route-state selection was checked against the existing dam action log.

### Resolved · Z2-PE-01 · Guarded Room / Workshop door

Replaced the round stone door in `assets/z2_guarded_room.png` with the shared vertical oak-board design and two plain iron straps. The Workshop face remains plain as before. The lizard is rendered as its own room item, so the new background leaves its center clear; the existing door-state selector and open-door views are unchanged.

Validation: reviewed the updated 240×102 image against the Workshop closed view; confirmed the Guarded Room still maps to closed/open art using the same story door flag and that the lizard remains a separate item at its existing door location. RGB555 output is 48,960 bytes. Full ROM rendering remains unavailable in this environment because `arm-none-eabi-gcc` is missing.

### Resolved · Z2-PE-02 · Tiny Room / Dreary Room through-view

Updated `assets/z2_tiny_open.png` so the open doorway reveals a small, distant oval table and the red-lit crack that identify the Dreary Room. The barred oak door, hinge, threshold, Tiny Room foreground, and reverse view remain unchanged.

Validation: reviewed at 240×102; verified the Tiny Room open-state selector still chooses this asset and the Dreary Room reverse-open view remains mapped separately. RGB555 output is 48,960 bytes. Full ROM rendering remains unavailable in this environment because `arm-none-eabi-gcc` is missing.

### Resolved · Z3-PE-01 · Prison Cell door states and reverse view

Updated the Prison Cell's closed and open backgrounds, repositioned the North Corridor's closed doorway to the left of the balcony, and added `assets/z3_marble_hall_prison_open.png`. A visual follow-up removed the bronze door that had been visible on the cell's far wall from the corridor camera; that view now shows a continuous masonry back wall. The bronze door on the perpendicular wall remains visible only from the cell-side camera. The cell-side closed leaf has left-side hinges and a right-side ring latch. The open pose keeps the leaf seated at that jamb, conceals the hinges, and puts the latch on the left side of the swung leaf. The cell-side open view shows the east-west marble corridor, parapet, and fiery pit. Room 19 selects the open corridor art from object 183's open flag, and its door, parapet, and pit interaction bounds follow the revised composition.

Validation: compared closed/open views from both rooms at 240×102; confirmed all four PNGs are 240×102 and each produces a 48,960-byte RGB555 buffer. The revised corridor open-state PNG keeps the entrance, door leaf, balcony, and fire composition while replacing the cell's far-wall door with masonry. `make test` passed, `room_visuals.c` compiled with host `cc` after suppressing an existing signed-comparison warning, and the Makefile resolves the new art asset. Existing route evidence confirms `open cell door` in room 19 followed by entry into room 45. Full GBA rebuilding remains unavailable because `arm-none-eabi-gcc` is missing.

### Resolved · Z3-PE-03 · Museum Entrance through-view

Updated `assets/museum_entrance_open.png` so the open central doorway reveals the barred cage and blue-green hall from the Jewel Collection. The cage remains beyond the entrance threshold, and the surrounding arches and approach floor remain in place. The closed entrance image is unchanged.

Validation: reviewed against `assets/jewel_room.png` at 240×102; the new PNG is 240×102 and converts to a 48,960-byte RGB555 buffer. `src/chapter.c` continues to select the open image when east stone door object 205 is open. Full GBA rendering remains unavailable because `arm-none-eabi-gcc` is missing.

### Resolved · Z3-PE-04 · Mine ladder pair

Revised `assets/z3_ladder_top.png` and `assets/z3_ladder_bottom.png` together. The ladder now occupies a matching center-right position, with the same narrow timber rails, rung rhythm, value, and rectangular floor/ceiling openings. The upper room retains its clear stair path; the lower room keeps both west and south passages unobstructed.

Validation: compared both 240×102 images side by side; each produces a 48,960-byte RGB555 buffer. The route records remain authoritative for reachability: Ladder Top 20 has no stable player-route capture, while Ladder Bottom 57 is reached only transiently from Scenic Vista. The art pair passes visual comparison, but a player-controlled up/down round-trip remains unverified.

### Resolved · Z3-PE-05 · Underwater lake architecture

Replaced the block-built arch and overhead rock ceiling in `assets/z3_underwater.png`. The view now has an uninterrupted rippling water surface across the top, with natural eroded rock confined to the side banks. The edit preserves the turquoise water, plants, algae, and clear central sand for the separate amulet cue. The surface and shore backgrounds remain unchanged; their natural cavern formations now match the underwater side banks. Room 12 continues to select `art_z3_underwater`. The unused `z3_underwater_variant.png` was subsequently removed because Zork III has no matching alternate underwater state.

Validation: reviewed the updated image at native 240×102 and in the game's 240×76 crop against `z3_lake_surface.png`, `z3_west_shore.png`, and `z3_south_shore.png`; confirmed the continuous water surface, open sand area, and aquatic plants remain readable. Checked that room 12 still maps to the updated scene. This was an image-only correction; no new ROM build or mGBA replay was performed at that time. The later wiring cleanup is recorded in [the background asset wiring audit](background-asset-wiring-audit.md).

## Reviewed sources and limits

- Current source art: the linked `assets/*.png` files and state mappings in `src/chapter.c`.
- Route/state transcripts: `build/zork-i-audit/dam-actions.log`, `build/zork-ii-art/re-audit/door-check.log`, `build/zork-ii-art/re-audit/keyhole-failures.log`, and `build/zork-iii-audit/master-prison-branches.log`.
- Existing route and state evidence: [Zork I repeat audit](zork-i-re-audit.md), [Zork II completion audit](zork-ii-completion-audit.md), and [Zork III route coverage](zork-iii-route-coverage.md).
- Shared acceptance rules: [Scenery rules](../../assets/SCENERY_RULES.md), including same-object door identity, fixed architecture across state changes, and actual destination views through open doors.
- This section describes the November 18 review, which checked source images and existing route captures without a new mGBA replay. The subsequent November 22 current-ROM pass verified the full gallery and the Zork III route frames linked above.
