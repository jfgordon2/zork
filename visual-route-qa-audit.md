# Route-driven visual consistency and QA audit

Reviewed November 17, 2026 against the checked-in route captures, route transcripts, and the current source scene artwork. This pass treats connected rooms as pairs or sequences: the route text and state determine what should be visible, then the images are checked for matching architecture, materials, props, character placement, and usable paths. The source art review does not prove that every update is present in the current ROM; see the ROM integration status in the [paired audit](paired-entrance-exit-art-audit.md).

**Current follow-up, November 18, 2026:** [The paired entrance/exit audit](paired-entrance-exit-art-audit.md) adds same-room open/closed comparisons and records the completed Zork I dam, Zork II door, and Zork III prison, Museum, ladder, and underwater-scene art corrections. The route-coverage section below is historical; see the [current Zork III route register](zork-iii-route-coverage.md) for the resolved route classifications and the ladder's remaining route limitation.

## Result

The November 17 sampled routes and their documented interaction outcomes were coherent across the reviewed Zork I and II scenes and the stable Zork III routes. The underwater art/story mismatch and the additional same-room and connected-door findings from November 18 have since been corrected; see the [current paired audit](paired-entrance-exit-art-audit.md). Zork III's former uncaptured-room list has since been classified in the [route register](zork-iii-route-coverage.md). The latest Royal Puzzle recovery behavior still needs a fresh native capture.

This November 17 route review uses existing route-driven captures and logs rather than a new cartridge replay. Evidence includes the Zork I 83-room repeat route and follow-up states, Zork II's 73-room completion route plus optional branch captures, and Zork III's connected-scene route logs and named path checkpoints. A November 18 container build has since rebuilt both ROMs and embedded the current source backgrounds; it did not replay the routes in an emulator.

## Connected-route findings

| Route or scene pair | Result | QA notes |
| --- | --- | --- |
| **Zork I: Behind House → Kitchen; Living Room → Cellar** | Pass, updated | The four window backgrounds now show the same vertical slider from both sides; room art still switches on object 243's open flag. The open hatch shows descending steps; the story closes it after descent. Returning and restoring reproduce the saved hatch state. |
| **Zork I: Dam Base → Dam → Lobby/Maintenance → Reservoir; Frigid River → Sandy Beach/Boat** | Pass, sampled | Indoor control rooms remain distinct from the exterior dam. Sluice and water states update with the story. River reaches and the boat/beached state agree with travel; the lantern remains retrievable in the boat and can run out of power as the story allows. |
| **Zork I: mine paths and ladder** | Pass, sampled | Ladder Top and Bottom use the same timber, rails, rungs, and mine palette from opposite viewpoints. The top-room staircase is a separate route. Native climb and return checks are recorded. |
| **Zork I: Egyptian Room → Hades → Stone Barrow** | Pass, sampled | The coffin changes from plinth to open and dropped states; Hades has its own gate/ritual states; the Barrow transition moves from stone exterior to lit cavern and bridge. |
| **Zork II: bridge/ford/ice routes and balloon/volcano branches** | Pass, sampled | Connected routes show continuous walkable surfaces and supported ledges. Balloon height, mooring, cargo, and return states are represented. Optional route states were checked in addition to the winning path. |
| **Zork II: Guarded Room ↔ Workshop** | Pass, sampled | The same oak door and opening are represented from both sides. The Guarded Room view opens to the actual Workshop; the Workshop view opens to the Guarded Room. The hinge is hidden on the Workshop side and the door state follows travel. |
| **Zork II: Oddly-angled Rooms, Carousel, Quarters, Aquarium** | Pass, sampled | The five angular rooms share a recognizable green crystal/slate identity; Carousel In/Out routes remain story-randomized. Quarters scenery changes with the printed description and persists through Restore/Continue. The aquarium serpent is drawn swimming, and its broken state follows the throw route. |
| **Zork III: Lake Shore → On the Lake → Underwater → Western/Southern Shore** | Pass, updated | The underwater scene now has an uninterrupted rippling water surface across the top and natural rock only along the side banks. The sandy bottom, plants, algae, and open center for the amulet cue agree with the room description; the water and rock forms connect to the surface and shore views. |
| **Zork III: Aqueduct → High Arch → Water Slide → Damp Passage** | Pass, sampled | The dry channel, supported elevated arches, slippery descent, moss, and blocking rubble read as one route. Tan walkable stone stays aligned across the views; darker chasm shadows are localized to the elevated overlook. |
| **Zork III: Button Room ↔ Beam Room; mirror halls and mechanism** | Pass, sampled | The beam is low in both connected views and meets the doorway. Mirror/panel states and the matching narrow-room variants follow the route descriptions; the same orientation and plain opposite wall are retained where appropriate. |
| **Zork III: museum route and alternate Side Room** | Pass, sampled | Real routes capture Technology Museum 5 → Museum Entrance 173 → Puzzle Entrance 84 → Side Room 130, plus alternate entrance 143 → Side Room 188. The doorway and floor-connected passage lead into the actual destination view. |
| **Zork III: North Corridor → Parapet → Prison Cell/Treasury** | Pass, sampled | The fiery pit, sundial positions, open cell door, closed bronze door, and Treasury have distinct roles. Door views and the stated route align. The final transformation art is implemented, but still needs a fresh native ending-screen capture. |
| **Zork III: Timber Room → Ladder Bottom and adjacent mine rooms** | Incomplete route evidence | Entering Ladder Bottom 57 from the sampled Timber Room state narrates the room and immediately returns to Scenic Vista. Rooms 20 and 138 are catalog-linked from 57 but are not proven playable from that transient state. Do not count this as a completed connected ladder route yet. |

## Interaction, UI, and persistence

- Across the documented samples, closed doors, gates, creatures, vehicles, and puzzle conditions return story-owned refusals; opening or satisfying the relevant condition makes the route succeed. No route candidate should be treated as guaranteed passable merely because it appears in Paths.
- The Zork I and II native audits found no UI overlay blocking sampled art or selected objects. Their audits include thousands of room-crop comparisons while opening and scrolling menus. Zork III's sampled renderer sweep also found no crop or focus-marker obstruction; the most recent terminal-screen behavior still lacks a fresh native replay.
- Zork I and II route tests cover take, drop, leave, return, Save/Restore, and Continue for representative floor props and containers. The Zork III samples cover museum drops, hidden/recovered ring, doors, machine state, and puzzle walls. After the Royal Puzzle save-state change, visually confirm Restore and Continue rebuild the diagram without a story turn; the earlier “Press B to survey” capture predates or misrepresents the current production restore path.
- Keep intentional hazards intact. In particular, the lantern may be left behind in the boat; the story lets its light expire over time. The audit confirms retrievability and persistence in the tested states rather than adding protection against that choice.

## Historical route gaps from the November 17 pass

This report originally listed 13 Zork III catalog entries without stable route captures. Subsequent tracing found stable routes to Narrow Rooms 51/58/161/178 and Drafty Room 228, and documented the transient, fatal, or trapped outcomes for the remaining entries. See [the current route coverage notes](zork-iii-route-coverage.md) for exact commands and evidence.

The older [Zork III screen matrix](zork-iii-audit-screens.csv) retains issue labels from its initial audit. Use the later [Zork III re-audit](zork-iii-re-audit.md) and the route classifications above for current dispositions.

## Evidence reviewed

- [Zork I repeat audit](zork-i-re-audit.md) and [route follow-up captures](docs/images/zork-i-followup-preview.png)
- [Zork II completion audit](zork-ii-completion-audit.md), [connected scene board](docs/images/zork-ii-completion-scenes.png), and [paired door views](docs/images/zork-ii-door-pair.png)
- [Zork III connected-scene comparison](zork-iii-consistency-pass.md), [scene-state captures](docs/images/zork-iii-connected-scene-states.png), [route coverage](zork-iii-route-coverage.md), and the lake route transcript in `build/zork-iii-audit/alternate-input.log`
- Current underwater, lake, and shore art: `assets/z3_underwater.png`, `assets/z3_lake_surface.png`, `assets/z3_west_shore.png`, and `assets/z3_south_shore.png`
