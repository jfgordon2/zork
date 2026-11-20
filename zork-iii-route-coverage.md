# Zork III route coverage for previously uncaptured rooms

Route traces use the bundled Zork III Release 25 story and saved checkpoints from the real game state. Forced catalog renders (`^ID name`) are useful for checking a room's text-to-scene mapping, but they do not count as route evidence. Replay the checked-in command files from the repository root with `build/zork-iii-audit/audit 1 build/zork-iii-audit/<route-file>.txt`.

## Stable, repeatable routes

| Destination | Checkpoint and repeatable route | Evidence |
| --- | --- | --- |
| Technology Museum 5 → Museum Entrance 173 → Royal Puzzle Entrance 84 → Side Room 130 | Restore `route-176-lit.sav` (Technology Museum, year 948); enter the gold machine, set the dial to 947, press the button, exit, open the wooden door, then go south, south, west. | `museum-connected-route-real.log`; captures `route-5-lit.ppm`, `route-173-lit.ppm`, `route-84-lit.ppm`, `route-130-lit.ppm` |
| Side Room 188 | Restore `route-143-lit.sav` at the alternate Royal Puzzle Entrance and go west. | `side-room-alt-route.log`; `route-188-lit.ppm` |
| Narrow Rooms 119 and 221 | Restore `route-54-lit.sav`; go southwest for 119 or southeast for 221. | `narrow-route-matrix.log`; `route-119-lit.ppm`, `route-221-lit.ppm` |
| Narrow Rooms 58 and 161 | Restore `route-160-lit.sav` inside the mirror box. Lift the short pole, push the red panel twice, lower the pole, push the mahogany panel, push the pine panel, then go out. Northwest reaches 58; northeast reaches 161. | `route-narrow-58.txt/.log/.ppm`, `route-narrow-161.txt/.log/.ppm` |
| Narrow Rooms 178 and 51 | Restore `route-160-lit.sav`. Push the white panel twice, lower the short pole, push the mahogany panel, lift the short pole, push the red panel four times, lower the pole, push the pine panel, then go out. Northwest reaches 178; northeast reaches 51. | `route-narrow-178.txt/.log/.ppm`, `route-narrow-51.txt/.log/.ppm` |
| Drafty Room 228 | Restore `route-124-lit.sav` at Scenic Vista. The checked-in route file obtains repellent from View II, returns, drops the torch and amulet, waits until the table indicates View I, sprays the repellent on the player, drops the can, and touches the table. From Timber Room 15, west reaches and saves stable room 228. | `route-drafty-228.txt/.log/.ppm`; saved state `route-drafty-228.sav` |

The mirror routes have a real save at room 160 as their common starting point. The two branches differ in panel sequence and in when the short pole is lifted, so follow the corresponding script verbatim. Together with the previously verified 119/221 pair, all six narrow-room destinations have stable prompts; the captures are route-driven, not forced catalog renders.

## Transient and unsafe outcomes

- **Ladder Bottom 57:** From `route-15-lit.sav`, east narrates the Ladder Bottom, then immediately returns to Scenic Vista 124 before another command prompt. The repeatable trace is `route-ladder-57-transient.txt/.log`. The declared exits from 57 to Ladder Top 20 and Dead End 138 do not become player-actionable on this timed visit. Neither 20 nor 138 has a stable route capture.
- **Machine Room 70:** Restore the saved, stable Drafty Room checkpoint `route-drafty-228.sav` and go south. The game narrates the gurgling passage and returns to Vista 124 during that action, before a separate Machine Room screen or prompt. See `route-machine-70-transient.txt/.log`. Thus 228 is reachable and stable; 70 is not independently screenable on this route.
- **Guardian-side Narrow Rooms 48 and 151:** From `route-160-lit.sav`, the recorded approach moves the box north to the Guardian position. `out` reports that the box is closed; opening the pine panel exposes the player to the Guardians and kills them. The death narration visits Prison Cell 225, then revives at Endless Stair 211. See `route-narrow-guardians-trap.txt/.log` and `route-narrow-guardians-death.txt/.log`. This confirms the attempted branch is a trap/death path, not a stable arrival at 48 or 151. No safe, stable route to those two catalog positions has been reproduced.
- **Prison Cell 225:** It appears in the Guardian death narration, followed by the old man's speech and revival at Endless Stair 211. The player receives no prompt in the cell. It is a story sequence, not an interactive room state.
- **Sacrificial Altar 210:** Scenic Vista View IV triggers the fatal altar narration and revival at Endless Stair. It is likewise not a stable room. See `optional-eras.log` and the Vista route records.

This closes the evidence classification for the 13 catalog entries that were previously uncaptured: five new stable routes (51, 58, 161, 178, 228), and eight entries with transient, fatal, or trap outcomes (20, 48, 57, 70, 138, 151, 210, 225). The previously verified 119/221 pair remains stable. The caveat is that 20/138 and 48/151 remain without a safe stable gameplay route; the evidence now explains why the sampled paths do not produce one rather than treating a forced render as proof of reachability.

## Paired-room continuity rule

When a route shares a physical feature across rooms, audit the views as one pair. Compare construction, width, materials, supports, orientation, and where the feature meets each floor or ceiling. Apparent position can shift with the camera, but the geometry must still read as one connected object. For Ladder Top 20 and Ladder Bottom 57, the same timber ladder descends through the upper floor and rises through the lower ceiling. See [the mine-art acceptance notes](assets/zork-iii-mine-art.md) and [the mirror-art mapping](assets/zork-iii-mirror-art.md).

The November 18, 2026 visual pass moved both ladder openings to a matching center-right position and aligned their rail width, rung rhythm, timber value, and rectangular openings. This is a source-art comparison only: Ladder Top 20 has no stable player-route capture, and Ladder Bottom 57 remains a transient visit from Scenic Vista. The updated views are visually paired, but a player-controlled up/down round-trip has not been observed.
