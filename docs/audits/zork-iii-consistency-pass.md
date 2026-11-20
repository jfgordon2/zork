# Zork III connected-scene consistency pass

Reviewed November 17, 2026 against the bundled Zork III Release 25 story. This pass compares connected room descriptions, background geometry, shared materials, interactive overlays, and state variants.

**Follow-up visual review, November 18, 2026:** the earlier connected-scene results remain useful for the routes listed below. The underwater masonry arch and overhead rock ceiling, prison-door state/view mismatches, and missing Museum through-view have since been corrected; see the [paired entrance/exit audit](paired-entrance-exit-art-audit.md) for per-asset changes and validation. The previously uncaptured route entries have since been dispositioned in [route coverage](zork-iii-route-coverage.md).

## Connected sets checked

| Connected rooms | Story relationship | Result |
| --- | --- | --- |
| Lake 184 → Underwater 12 → Western/Southern Shores 140/216 | A shallow, cold lake with sandy underwater floor, algae, a sparkling amulet, and separate shore views. | The four views distinguish surface, underwater, and shoreline perspectives. Underwater now has a continuous water surface above natural side banks and the sandy bottom; the amulet sits on sand, and the vegetation and turquoise water remain visually connected to the shore views. |
| Aqueduct 203 → High Arch 182 → Water Slide 163 → Damp Passage 93; Aqueduct View 75 | A dry stone channel rises over a deep chasm, then drops through a slippery mossy slide into a damp room with rubble blocking the old north channel. A balcony overlooks the aqueduct. | The route reads as one supported stone system. The slide drops toward the damp passage, rubble blocks the north continuation, and the high walkway stays above the gorge. Cooler shadows at height read as lighting; the walkable stone stays tan and warm. |
| Button Room 98 ↔ Beam Room 6 | The long corridor has a low red beam inches above the floor; the southern room has stairs and a red wall button. | Both rooms use the same warm block-stone palette. The beam overlay is placed at each room’s floor perspective, so it crosses at the same low relative height. Its endpoints align with the corridor opening. |
| Mirror Hallway 169 ↔ Inside Mirror 160; narrow rooms 8/48/51/58/100/119/151/161/178/221 | The halls have a stone channel and compass rose; the inner wooden box has colored walls and movable panels. The Narrow Room descriptions specify an east- or west-wall mirror, plain solid rock opposite it, and Guardian statues only in some positions. | The Narrow Room set now uses the mirror on the described side, plain rock opposite, and no unmentioned floor compass. Rooms 48/151 show one Guardian and its reflection; 58/161 show the described distant paired statues; 51/178 and 119/221 have no statues. West-mirror art is horizontally flipped from its matching east view. The earlier forced-catalog screenshot captured stale text from the preceding room; the diagnostic now performs `look` before rendering. |
| Scenic Vista 124 → Timber Room 15 / Room 8 192 / Damp Passage 93 | A wall-mounted viewing table and changing I–IV indicator select three destinations; IV is the fatal altar outcome. | The table chamber and all three destination backgrounds are distinct. The indicator is drawn from state, and the wall torch uses a mounted pose until taken. Existing route checks include all four outcomes. |
| Technology Museum 5 → Museum Entrance 173 → Royal Puzzle Entrance 84 → Side Room 130; alternate entrance 143 → Side Room 188; museums 66/176 and 179/103 | The museum door opens south, the Royal Puzzle Entrance has a floor hole and a west exit, and the Side Room has north steps and an east steel door. | Real in-story routes now capture IDs 5, 173, 84, 130, and 188. The Side Room has its own background shared by IDs 130/188, and the Royal Puzzle Entrance now shows a floor-connected west passage into it. Unfinished and present museum states remain distinct. |
| North Corridor 19 → Parapet 36 → Prison Cell 45/204 → Treasury 46 | The corridor opens north to the fiery parapet and has a closed barred door. The parapet has a numbered sundial and button. The cell door, bronze exit, fire view, and final treasure room have distinct roles. | Room 19 now maps to its dedicated corridor art, matching the parapet’s sandstone and fire. The dial face is redrawn as one perspective-aligned pixel-art object with eight numbered sectors and a central button; its shaded pointer follows the saved setting. In the open-cell state, the right-side wooden cell door opens to the corridor and fire view while the central bolted bronze door stays shut. The closed state keeps both doors closed. The Treasury has its own scene. |
| Royal Puzzle Entrance 143 → puzzle diagram 194 | The entrance is a small chamber with a floor hole, west arch, north stairs, and warning note; the puzzle itself is represented by a separate grid. | The hole, paths, and note have room in the entrance scene. The puzzle remains an interface view, with a floor-item strip below the grid so dropped objects stay visible above the story panel. |

## Corrections made

- Redrew the whole sundial face as a cohesive pixel-art ellipse with recessed numerals, divider grooves, a raised button, and a stateful pointer aligned to all eight positions.
- Matched the Beam Room and Button Room stone palette and aligned the red beam to each corridor’s floor perspective.
- Added the missing `North Corridor` room-to-background mapping and its focus anchors for the cell door, parapet, and fiery pit.
- Corrected the prison-cell open state to open the wooden cell door on the right wall. The central bronze door stays shut, and the opening shows the connected fire-lit corridor.
- Added all referenced Z3 scene symbols to the cartridge scene table and stopped linking two unused duplicate/obsolete scenes.
- Added a compact Royal Puzzle floor-item row. It uses each item's scene sprite, keeps the focused item in view when the row pages, and stays clear of the diagram and story text.

## Evidence and limits

- [Connected-scene montage](../images/zork-iii-connected-scenes.png); the current source has 35 active Z3 background assets, including the additions noted below.
- [18 connected room states in the production renderer](../images/zork-iii-connected-scene-states.png)
- [Parapet sundial state](../images/zork-iii-sundial-preview.png)
- [Updated 72-room renderer review](../images/zork-iii-scene-review.png)
- [Per-state checklist](zork-iii-audit-screens.csv)
- [Route coverage and reachable-room checkpoints](zork-iii-route-coverage.md)
- [Royal Puzzle floor-item and UI preview](../images/zork-iii-puzzle-floor-preview.png)

The host renderer refreshed **38 connected-room states**, regenerated the wider inventory at **72 room IDs / 76 room-light panels**, and captured all eight sundial settings to confirm the pointer tracks each engraved mark. New real route captures cover museum rooms 5/173/84/130 and alternate Side Room 188; room 54 reaches the east- and west-mirror pair 119/221. Thirteen catalog entries still lack stable route-driven captures and are classified in the [screen matrix](zork-iii-audit-screens.csv). Their diagnostic renders now pair each room's own `look` text with its artwork. The **27 existing route and branch scripts** completed without harness failures. For the floor-item update, a dropped-book scenario confirms the item appears below the grid and above the story panel. `make test` passes all three stories. A full cartridge build could not be repeated in this environment because `arm-none-eabi-gcc` is unavailable.

The new state captures use the production C renderer at 240×160. They do not replace a fresh mGBA replay of the latest artwork. The earlier native controller and persistence results remain in the initial [Zork III QA audit](zork-iii-audit.md).
