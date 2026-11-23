# Room-screen focus boxes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Every focusable object on a room screen has exactly one gold box around its drawn footprint, and the replay audit proves the box matches that footprint.

**Architecture:** Draw paths report the focused object's actual sprite rect through a `VISUAL_QA` hook; `corners()` keeps recording the drawn box. The replay audit compares the two (containment within a 3 px border), requires a box for every visible target unless explicitly allowlisted, and fails on zero or two boxes. Data fixes give the currently unboxed items a drawable footprint.

**Tech Stack:** C11, freestanding ARM (devkitARM), host replay audit via `make test`, `VISUAL_QA` hooks.

**Spec:** `docs/superpowers/specs/2026-11-22-room-focus-boxes-design.md`

## Global Constraints

- Room screens only; menus, map, speech, dial, keyboard unchanged.
- `VISUAL_QA` hooks must not exist in the shipped ARM build (guard with `#ifdef VISUAL_QA`).
- Backgrounds are 240×102; the scene band is rows 15–90 (`y >= 15`, `y + h <= 91`).
- `make test` and `make zork-trilogy.gba` must both pass.
- No git history exists; confirm `git status --short` lists only touched files instead of committing.

## Review Focus

- A target that is focusable but not drawn in the scene must be an explicit, documented allowlist entry (name-row box only), never silent.
- A box drawn around the wrong sprite must fail via the independent sprite report, not pass because one box exists.
- Dark rooms and the Royal Puzzle map have no scene sprite; they must not be forced into the sprite assertion.
- Contents children (e.g., a coin in the trophy case) are focusable; their box must match the small contents sprite, not the container.
- Adding floor rows or placements must not move existing scenery or break the `room_floors`/`placements` tables.

---

### Task 1: Sprite reports and correspondence audit

**Files:**
- Modify: `src/room_visuals.c` (add `report_focused`, call it from sprite draw paths)
- Test: `tests/transcript_replay_test.c` (add `visual_qa_focused_sprite`, assertions, allowlist)

**Interfaces:**
- Produces (test side): `void visual_qa_focused_sprite(unsigned object, int x, int y, int width, int height);`
- Produces (production side): `static void report_focused(unsigned object, int x, int y, int width, int height)` that calls the hook under `#ifdef VISUAL_QA`.

- [ ] **Step 1: Write the failing test**

In `tests/transcript_replay_test.c` add globals and the hook next to `visual_qa_marker`:

```c
static unsigned qa_focused_object;
static int qa_focused_rect[4];
static bool qa_focused_seen;

void visual_qa_focused_sprite(unsigned object, int x, int y, int width, int height) {
    if (qa_focused_seen && object != qa_focused_object) return;
    qa_focused_seen = true;
    qa_focused_object = object;
    qa_focused_rect[0] = x; qa_focused_rect[1] = y;
    qa_focused_rect[2] = width; qa_focused_rect[3] = height;
}
```

Add an allowlist helper above `audit_scene_target_boxes`:

```c
// Visible in the name row only: the story keeps these off the room view.
static bool focus_allowlisted(unsigned chapter_id, unsigned room, unsigned object) {
    return chapter_id == 2 && object == 11 &&
           (room == 45 || room == 204 || room == 225); // Dungeon Master behind a shut cell door
}
```

In `audit_scene_target_boxes`, before drawing each target, reset:

```c
qa_focused_seen = false;
qa_focused_object = 0;
```

After `draw_target_focus_box(true);` and the `scene_is_visible` computation, insert:

```c
if (scene_is_visible && !focus_allowlisted(chapter, room_id, object)) {
    if (!qa_focused_seen || qa_marker_count == 0) {
        fprintf(stderr, "Missing focus box: Z%u room %u object %u\n",
                chapter + 1, room_id, object);
        assert(false);
    }
    int mx = qa_first_marker[0], my = qa_first_marker[1];
    int mw = qa_first_marker[2], mh = qa_first_marker[3];
    assert(mx <= qa_focused_rect[0] + 3 && my <= qa_focused_rect[1] + 3);
    assert(mx + mw >= qa_focused_rect[0] + qa_focused_rect[2] - 3);
    assert(my + mh >= qa_focused_rect[1] + qa_focused_rect[3] - 3);
    assert(mw <= qa_focused_rect[2] + 8 && mh <= qa_focused_rect[3] + 8);
}
```

- [ ] **Step 2: Run the test to verify it fails**

Run: `make test`
Expected: assertion failures or "Missing focus box" lines; no `visual_qa_focused_sprite` calls exist yet.

- [ ] **Step 3: Report the focused sprite at each draw site**

In `src/room_visuals.c` add:

```c
static void report_focused(unsigned object, int x, int y, int width, int height) {
#ifdef VISUAL_QA
    extern void visual_qa_focused_sprite(unsigned object, int x, int y, int width, int height);
    visual_qa_focused_sprite(object, x, y, width, height);
#else
    (void)object; (void)x; (void)y; (void)width; (void)height;
#endif
}
```

Call `report_focused(object, bounds.x, bounds.y, bounds.width, bounds.height)` immediately after drawing the focused sprite in:

- `draw_room_items`: fixed placements (`selected`), loose floor slots (`object == focused`), the focused redraw pass, and overflow compact draws (`focused_compact`).
- `draw_encounter`: when `focused == figure->object`.
- `draw_menhir`: when the menhir is the focused object.
- `draw_grating`: when the grating is focused.
- `draw_stand_spheres`: when the sphere is focused.
- `draw_boat_cargo`: when `selected`.
- `draw_zork1_item_contents` / `draw_zork2_item_contents` / `draw_zork3_item_contents`: when a child is focused, report the child's small bounds.
- `draw_zork1_boat_pov`: when the boat (208) is focused, report the interior rect.
- Scaled prop branch (Zork I room 122 object 92) and the guardians' scaled sprites when focused.

For the first report of a frame only (the focused entity), the hook's first-wins rule matches `corners()`.

- [ ] **Step 4: Run the test to verify it passes for boxed targets**

Run: `make test`
Expected: the correspondence assertions pass for targets that draw a sprite; the remaining failures are the known unboxed/unreported targets listed in Task 2.

- [ ] **Step 5: Verify the ARM build**

Run: `make zork-trilogy.gba`
Expected: builds; `visual_qa_focused_sprite` is absent from the binary (`nm build/trilogy.elf | grep visual_qa` is empty).

---

### Task 2: Give every focusable room target a box

**Files:**
- Modify: `src/room_visuals.c` (floor rows, placements, encounter drawing, special props)
- Test: `tests/transcript_replay_test.c` (allowlist only where the story hides the object)

**Interfaces:**
- Consumes: `report_focused` and the audit from Task 1.
- Produces: audit reports zero missing room-screen boxes.

- [ ] **Step 1: Run the audit and capture the remaining list**

Run: `make test 2>&1 | grep "Missing focus box"`
Expected: the 22 known cases, grouped as loose items without floor contact (coal, diamond, bag, stiletto, egg, knife, chute), props without reports (menhir, metal lid, crystal sphere, stone door), garden unicorns, and the dungeon master.

- [ ] **Step 2: Add floor rows for rooms that cannot draw loose items**

In `room_floors[]`, add `FLOOR(<scene>, left_x, 85, right_x, 85)` entries for the backgrounds used by Treasure Room, Machine Room, Attic, Strange Passage, Drafty Room, Shaft Room, and the Cellar chute, using coordinates that keep items inside the 15–90 band and clear of fixed props. Verify each with the audit rather than guessing twice.

- [ ] **Step 3: Draw and report the special props**

- Menhir: report its drawn bounds when focused (`draw_menhir`).
- Metal lid: report the lid sprite bounds for the focused lid object.
- Crystal sphere (Aquarium): report the sphere's contents bounds.
- Stone door (Jewel Room): ensure the door placement's parent matches the room target and report its bounds.
- Chute (Cellar): add a placement or contents draw and report it.

- [ ] **Step 4: Handle the garden unicorn**

The unicorn encounter is targeted in the three garden rooms but drawn at its home room. Either draw it at a garden position when the room has a garden background, or exclude it from `story_targets` in those rooms. Prefer drawing it if the story text places it in view; otherwise exclude.

- [ ] **Step 5: Re-run until the audit is clean**

Run: `make test 2>&1 | grep -E "Missing focus box|Scene target-box audit"`
Expected: no "Missing focus box" lines; the audit line reports `0 without a scene-specific box`.

- [ ] **Step 6: Verify both builds**

Run: `make test && make zork-trilogy.gba`
Expected: both pass; `git status --short` lists only `src/room_visuals.c` and the two test/documentation files touched.

- [ ] **Step 7: Record the result**

Update `docs/audits/visual-consistency-review.md` (or the file that owns focus-box notes) with the new assertion, the allowlist, and the final audit counts.

---

## Self-Review

- **Spec coverage:** sprite reporting (Task 1), correspondence check (Task 1), fixing the 22 (Task 2), allowlist for hidden entities (Tasks 1–2), name-row box retained (existing, asserted by `qa_ui_focus_count`).
- **Type consistency:** `visual_qa_focused_sprite(unsigned, int, int, int, int)` and `report_focused` use the same signature in both files.
- **Review Focus coverage:** hidden target allowlist (Task 1), wrong-sprite detection (Task 1 assertions), dark/puzzle exclusions (`scene_is_visible` guard), contents children (Task 1 reporting), data-table safety (Task 2 steps verify with the audit).
