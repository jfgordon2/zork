# SDD ledger — plan: /Users/jeff/GitHub/zork/docs/superpowers/plans/2026-11-22-room-focus-boxes.md
Setup: Ruling: no git history — execute in place, manual ledger, no commits; cost if wrong: none.
Pre-flight: Task 1 produces visual_qa_focused_sprite/report_focused consumed by Task 2's audit; signatures match.
Task 1: Ruling: assert geometry only when a sprite report exists, and count/report missing reports instead of asserting, so the suite stays green between tasks; Task 2 flips missing to a hard failure — cost if wrong: a missing report could hide until Task 2 (which is the task that fixes it).
Task 1: complete (make test → pass; ARM build pending after Task 2). Hook + reporting at item/encounter/contents/stand/menhir/grating/boat paths; 1240 reports, 0 mismatches, 0 out-of-band. Also clamped encounter sprites and boxes into rows 15–90 (pre-existing outside-band cases).
Task 2: complete (make test → pass; ARM builds, 260,256 B .bss, no visual_qa symbols). Added loose_extras drawing for art-bearing story-local targets; clamped encounter sprites/boxes into rows 15–90; moved the cell-door anchor; allowlisted 8 targets that have no scene art (name-row box only). Audit now asserts missing == 0.
Task 2: Ruling: targets with no drawable art are allowlisted (name-row box) rather than framed around empty scenery — cost if wrong: those eight keep only the name-row box.
Task 2: Ruling: loose_extras is an explicit per-room table instead of relaxing the parent check — cost if wrong: a future story-local item needs a table entry.
Final review fixes (one pass):
- Critical: every gold box now reports itself with the focused object (visual_qa_focused_box from corners with current_focused, set by draw_room_details and draw_puzzle_map); audit requires a report for every non-allowlisted target and checks object identity. Coverage: 4,435/4,442 (7 allowlisted), 1,250 independent sprite rects, 0 mismatches.
- Critical: sprite identity compared with an explicit alias for the sword-in-rock (202 reports 146).
- Important: encounter shadow follows the clamped sprite row.
- Important: loose_extras trimmed to the shared diamonds only; carried/contained items are name-row only (bag, egg, stiletto, coal, knife).
- Important: allowlist is state-aware for the Dungeon Master (door open) and Dreary lid (door shut); stale entries removed.
- Important: dropped-item audit now requires a box report with matching object and checks the sprite rect.
- Minor: sprite tolerance tightened to sprite+6; docs corrected (anchor moved right/up, box vs sprite counts).
Final: Ruling: concept boxes (anchors/specials) are validated for presence and identity, not geometry, because the box is defined by the same rect — cost if wrong: a concept box on the wrong scenery spot is not caught geometrically.
Final: Ruling: corners() clamps boxes into rows 15–90, so the raw out-of-band metric is informational — cost if wrong: a box that would spill off-band is trimmed rather than flagged.
Final: minor (deferred): the "two scene boxes" assertion is vacuous because corners() suppresses the second box by design.
Final: minor (deferred): anchor data changes are only presence-checked.
Scene-anchor placement pass:
- Added the missing anchors for the Zork III Hallway compass rose and stone channel (room 113) and allowed the attic stairs and the Dungeon Master in the North Corridor/Parapet views, where the art does not draw them.
- Visual review of representative rendered frames found four concept boxes on the wrong scenery: the living-room carpet (now state-aware: rug spread vs rolled aside), the kitchen and attic tables (widened to the full tabletop), the Parapet sundial (widened to the full dial), and the Hallway 113/169 floor features (moved onto the medallion/groove).
- Audit after the pass: 1,018 states, 4,412 targets, 4,402 box reports, 1,921 sprite + 2,481 scene-feature rects, 0 missing, 0 out-of-band, 0 mismatches. ARM ROM builds; no visual_qa symbols.
Dungeon Master + medallion pass:
- Verified against route saves that the follower Master is object 1 (parent = current room) and is already drawn and boxed in the North Corridor, Parapet, Narrow Corridor and South Corridor. Object 11 is the room-local proxy/duplicate; it is correctly name-row-only when the actor is absent.
- With the prison-cell door shut, the Master target (object 11) now boxes the barred window in the cell door (203,15 32x17); with the door open in room 45 it keeps the distant corridor sprite box. The per-room door objects are 183/153/72; the DM 45/204/225 allowlist entries were removed.
- Compass-rose anchors in Hallways 113/169 and Inside Mirror 160 now cover the full floor medallion.
- Audit: 4,412 targets, 4,408 box reports (1,921 sprite + 2,487 scene-feature rects), 0 missing, 0 out-of-band, 0 mismatches.
Master proxy target filter:
- The North Corridor/Parapet local property lists object 11, but the real presence is actor object 1 (parent = current room). When object 1 is absent, add_object now drops the object-11 proxy in rooms 19/36, so the name row no longer offers "Dungeon Master" in a room he isn't in. The actor still draws and boxes when present (route saves: room 19 box 52,33 28x53; room 36 box 40,38 28x53). The DM 19/36 allowlist entries were removed.
- Cell views 45/204/225 keep the proxy because the Master is addressable from inside through the shut door; the barred-window box remains.
Visual re-audit (targets vs presence):
- Strengthened the room-target visibility audit: "isn't here" / "is not here" now count as absent, with a documented exception for the Master outside a shut prison-cell door (he is addressed through the window).
- Filtered two phantom local mentions: the Master proxy in North Corridor/Parapet when actor object 1 is absent, and the Kennel's citation of the Menhir Room stone (room 128 shows only the collar/bones).
- Fixed two dead item placements: the attic nasty knife (parent should be the table, 250) and the attic stair railing (global parent 48). Both now draw and are validated as sprite targets; removed from the allowlist.
- Remaining allowlist is carried/contained items plus the Dreary lid's open state and the Jewel Room stone door (a functional exit behind the player).
- Final audit: 1,017 states, 4,410 targets, 4,412 box reports, 1,925 sprites + 2,487 scene features, 0 missing, 0 out-of-band, 0 mismatches; visibility audit 4,218 appearances, 0 rejected.
Guardian-hallway focus anchors:
- Added compass-rose and stone-channel anchors for rooms 44/54/224, using the guardian-hallway rects for 44/224 and the mirror-hall rects for 54. Room 54's mirror/panel now use the mirror-hall box; its south wooden wall (object 80) is allowlisted because it is behind the camera.
- Verified in the route saves: room 54 compass 81,72 87x19 / channel 119,66 12x25; room 224 compass 68,70 108x21 / channel 116,46 10x26, all on the drawn medallion/groove.
- make test still 0 missing/0 mismatches; ROM rebuilt.
Guardian pair target:
- Added the "Guardians of Zork" -> "guardians" parser alias so object 152 is offered in hallways 44/54/224, where the statues are drawn.
- draw_z3_guardians now frames the pair with one combined sprite rect (44: 25,23 190x59; 54: 50,27 141x39; 224: 46,28 149x44) and reports it for the focus box.
- Filtered object 152 outside 44/54/224 (narrow-mirror rooms and Inside Mirror 160), where the statues are not in the view.
- make test passes 0 missing/0 mismatches; ROM rebuilt.
