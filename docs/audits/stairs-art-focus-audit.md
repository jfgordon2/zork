# Stairway Art and Focus Audit

**Pass:** 2026-11-23, inventory and follow-up review

## Method

Compared the room descriptions and traced route target lists with the current
mGBA room gallery and the renderer's focused-feature frames. Checked the
`RoomAnchor` entries in `src/room_visuals.c` where the gallery did not include a
selected-Stairs screenshot. The follow-up verifies room orientation and story
states, corrects focus-only mismatches, and prepares new scenery for review
before wiring it into the game.

## Zork I

| Room | Finding | Follow-up |
|---|---|---|
| 2 Temple | The descending stairs are visible at right and the focus box lands there. | Pass |
| 4 Atlantis Room | The ascending steps at right match the text and focus box. | Pass |
| 16 Ladder Top | Separate stairs at left are visible beside the ladder and focused correctly. | Pass |
| 18 Torch Room | Text says the left doorway leads to a descending staircase. | Added the descending treads inside the left doorway and moved focus there. |
| 27 Kitchen | The two steps at the far right are visible and focused. | Pass |
| 33 Cellar | The central stairway is visible and focused. | Pass |
| 46 Gas Room | The short stairs at left are visible and focused. | Pass |
| 62 Cave | The descending stairs at right are visible and focused. | Pass |
| 75 Living Room | Stairs are only revealed after opening the trapdoor. The route screenshot predates the current target filter. | No art change, per user direction. The existing filter keeps Stairs unavailable until the hatch is open. |
| 78 Cyclops Room | The staircase is visible at right, behind the Cyclops, and the focus box is on it. | Pass |
| 82 Treasure Room | The stairs at left are visible and focused. | Pass |
| 93 Smelly Room | The small descending stair opening is visible in the foreground and focused. | Pass |
| 106 Chasm | The room description mentions the chasm, path, and crack—not stairs. | Suppress the false Stairs focus target. The diagonal routes remain available in Paths. |
| 117 Strange Passage | A stale stairs anchor exists, but the live target list contains only the Thief, stiletto, and bag. | Remove the unused anchor; no art change is needed. |
| 130 East-West Passage | The narrow stairway down is visible at the north end and focused. | Pass |
| 176 Cave | The dark descending stairway is visible at right and focused. | Pass |
| 187 Attic | Story says the only exit is a stairway down. No steps are visible; the approved railing-only sprite was not yet wired. | The approved railing-only sprite is now wired at lower left and supplies its own focus box. Do not depict steps. |
| 191 Reservoir North | Story says a slimy stairway leaves north. The background faces south across the drained basin, so the stairs are behind the camera; the current focus box sits over water/rock. | Suppress the misleading scene target. North remains available in Paths; do not invent stairs in the south-facing view. |
| 205 Loud Room | The stairway at right is visible and focused. | Pass |
| 241 Egyptian Room | The west-side stairs are visible and focused. | Pass |

## Zork II

| Room | Finding | Follow-up |
|---|---|---|
| 19 Oddly-angled Room | The stairway at left is visible and focused. | Pass |
| 162 Cerberus Room | The marble stairs at left are visible and focused. | Pass |
| 240 Stairway | The descending marble stairs are visible and focused. | Pass |

## Zork III

| Room | Finding | Follow-up |
|---|---|---|
| 75 Aqueduct View | Text says stone steps lead up northwest. | Added connected steps climbing from the far-left walkway and a matching focus anchor. |
| 84 Royal Puzzle Entrance | A stair target is present and the shared room art depicts steps, but there was no room-84-specific stairs anchor. | Added a room-specific focus anchor matching the steps used in room 143. |
| 98 Button Room | Text places stairs south, ascending into darkness. The image has foreground steps, but the current focus box points back toward the far corridor. | The focus anchor now covers the visible foreground treads. |
| 102 Lake Shore | The polished steps at right match the description and focus box. | Pass |
| 130 Side Room | Northbound steps are visible and the room-specific anchor covers them. | Pass |
| 143 Royal Puzzle Entrance | The steps north of the hole match the description and focus box. | Pass |
| 172 Engravings Room | With the secret door closed, the text describes passages and runes, not stairs. When opened, the renderer reveals descending stairs in the wall. | Hide the stale focus target while closed; when open, focus the rendered stair opening rather than the carvings. |
| 173 Museum Entrance | Text says the Royal Puzzle entrance is a few steps south. | Added matching south-entry steps to both door-state images and a focus anchor. |
| 188 Side Room | Northbound steps are visible and the room-specific anchor covers them. | Pass |
| 211 Endless Stair | The long stairway and rail are visible and focused. | Pass |
| 227 Museum Entrance | Text also puts the Royal Puzzle entrance a few steps south. The old focus box was on empty floor at lower right. | Added matching south-entry steps to both door-state images and moved focus to the steps. |

## Proposal order

1. Refresh mGBA route evidence for Torch Room 18, Aqueduct View 75, and the
   Museum Entrance states, then inspect their focus boxes and paired views.
2. Confirm the current Living Room filter and the open/closed Engravings Room
   target behavior in the refreshed gallery.

## Evidence

- Zork I route text and target list: `qa/mgba-room-gallery/route-data/zork-i-mine-audit/route-replay.log`.
- Zork II route text and target list: `qa/mgba-room-gallery/route-data/zork-ii-finish/re-audit/route.log`.
- Zork III route text and target list: `qa/mgba-room-gallery/route-data/zork-iii-audit/route.log` and `museum-connected-route-real.log`.
- Current full-screen captures: `qa/mgba-room-gallery/zork-i/room-*.png`,
  `qa/mgba-room-gallery/zork-ii/room-*.png`, and
  `qa/mgba-room-gallery/zork-iii/room-*.png`.

## Art previews

The approved Torch Room, Aqueduct View, and paired Museum Entrance generations
are installed. The Living Room preview was declined and was not installed.
