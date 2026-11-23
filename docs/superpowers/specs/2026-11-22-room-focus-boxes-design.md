# Room-screen focus boxes: coverage and correspondence

Date: 2026-11-22
Status: awaiting approval
Scope: room screens only (scene art + selected name row). Menus, map, speech, dial and
keyboard keep their current focus treatment.

## Intent

Every object the player can focus with L/R on a room screen should have exactly one
gold corner box around the part of the scene that represents it, and the audit should
prove the box is on the right thing — not merely that some box was drawn.

## Current state

- `corners()` draws one gold box per frame (first box wins) and records it for
  `VISUAL_QA`.
- The selected name row always gets a gold box.
- The replay audit checks: one scene box, at most one distinct box, inside the art
  band, one name-row focus. It does not check that the box matches the drawn object.
- 22 focused targets currently draw no scene box: mostly loose items in rooms with no
  floor contact (coal, diamond, bag, stiletto, egg, knife), plus encounters drawn
  outside the target room (garden unicorn), the menhir, metal lid, crystal sphere,
  stone door, chute, and the dungeon-master proxy.

## Approaches considered

1. Audit-only, expected rect (chosen): draw paths report the focused object's actual
   sprite rect; the harness compares the recorded box with it.
2. Runtime hard assertion only: assert on hardware that a box was drawn; proves
   presence but not correspondence, and is invisible in tests.
3. Add boxes to phantom targets without drawing them: a box around empty scenery.

## Design

1. **Sprite reporting.** In `VISUAL_QA`, every draw path that can draw the focused
   object reports its exact drawn rect through a new hook
   `visual_qa_focused_sprite(unsigned object, int x, int y, int width, int height)`:
   `draw_room_item`, `draw_masked` encounter drawing, boat cargo, contents sprites,
   stand spheres, the menhir, the trophy case, the grating, and the scaled scene props.
   Reporting is independent of whether a box is drawn.
2. **Correspondence check.** In `audit_scene_target_boxes`, for each focused object:
   - A focused-sprite report must exist unless the object is in an explicit
     name-row-only allowlist (target: empty).
   - Exactly one distinct marker must exist when a sprite was reported.
   - The marker must equal the reported rect expanded by the draw border (tolerance
     ±3 px), so a box on the wrong item or a scaled/offset mismatch fails.
   - The name-row box must still appear exactly once.
3. **Fix the 22.** For each missing target, either give the item a floor contact or
   placement so it draws and is reported, draw the special prop (menhir, lid, sphere,
   door, unicorn where targeted, dungeon-master proxy in the cell), or remove the
   object from `story_targets` if the story cannot show it in that room. The audit
   becomes an assertion (0 missing) rather than a warning.
4. **No gameplay change outside `VISUAL_QA`** apart from the data fixes in (3), which
   only add missing scenery/placement for objects already listed as present.

## Tests

- `make test` fails if any room-screen target has no sprite report and is not
  allowlisted, if a target has zero or two scene boxes, if the box does not match the
  reported sprite rect within tolerance, or if the name-row box is missing.
- Keep the existing dropped-item check, tightened to require a matching box.
- Report the final counts (states, targets, mismatches) in the audit line.

## Risks

- Some anchors intentionally frame a scene *concept* rather than a sprite (e.g., "the
  house"); those report the anchor rect, not a sprite, through the same hook.
- The one-box-per-frame rule can hide a second box; the audit compares against the
  reported rect, so a wrong first box now fails instead of passing silently.
