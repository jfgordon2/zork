# Zork I repeat audit

The rebuilt cartridge reached the Stone Barrow with **350/350 points in 507 moves**. The winning route visited **83 room IDs**. The controller replay resolved 334 exact and 106 equivalent actions. Five redundant failed inputs replayed as Look to preserve turn timing. The host test suite passed, and the other chapters' controller replays still reached their 400-point and 7-potential endings.

**Follow-up art comparison, November 18, 2026:** the same-room dam water/sluice variants still change the far masonry design. This was not identified by the earlier route replay; see [the paired entrance/exit audit](paired-entrance-exit-art-audit.md).

The table records the targeted checks applied to the rebuild. mGBA checks used the cartridge controls for `SWIM`, the trident, the trap door, the coffin, and the new boat repair path. The larger route and branch replays used the same story interpreter, menu command builders, and room renderer. [Screen checklist](../../qa/screen-checklist.md) · [Repeat-audit captures](../images/zork-i-followup-preview.png) · [ImageGen prompts](../../assets/zork-i-followup-art.md)

## Mine scenery, UI access, and persistence pass

Ladder Top, Ladder Bottom, and the Gas Room now have dedicated scenery. The two ladder views share the same worn wooden rails, mine materials, and shaft. The Gas Room shows its stairs and eastern tunnel with lantern-compatible light, plus the sapphire bracelet on its floor. [Latest emulator captures](../images/zork-i-mine-preview.png) · [Mine art prompts](../../assets/zork-i-mine-art.md)

The access pass found that the generic Climb action at Ladder Top tried to go upward. The ladder now offers **Climb down** at the top and **Climb up** at the bottom in both primary and Movement menus. The separate top staircase and Gas Room stairs offer **Climb up**. The native emulator check traveled down and up the ladder, up the separate staircase, and back down through Paths.

The 83-room winning replay again reached **350/350 in 507 moves**. Across its first-entry room states, button-handler checks visited 184 in-room target entries, 358 inventory entries, 4,252 argument pickers, and 43,684 scrollable menu rows. There were 5,227 room-crop comparisons across focus, action menus, categories, tool or target pickers, Paths, and inventory. Every comparison matched the underlying room view. Browsing left the story turn, score, and room unchanged.

These counts include repeated objects and controls across different rooms. They establish access and visibility for the tested room states. The original valid and rejected action replays provide the outcome checks.

| Changed state | Leave and re-enter | Save, change, and restore | Additional evidence |
| --- | --- | --- | --- |
| Gas Room bracelet taken | Exited up the stairs and returned. The bracelet stayed in inventory and the floor stayed empty. | Saved through the pause menu, dropped it, and restored. The bracelet returned to the saved inventory state. | mGBA captured the saved state, return, drop, Restore, and launcher Continue. |
| Gas Room bracelet dropped | Returned through the stairs with the bracelet still on the floor. | Saved the dropped state, took it, and restored. It returned to the ground. | The host compared focused room-art pixels before and after the return and Restore. |
| Living Room hatch open | Visited the Kitchen and returned. The hatch stayed open. | Saved while open, closed it, and restored. The open state and artwork returned. | mGBA used the real Save and Restore menus. Descending still triggered the story's separate hatch-closing event. |
| Maintenance Room flood | The flood remained tied to its story timer. | Saved after pressing the blue button, waited for the water to rise, and restored. The earlier water level, turn, and artwork returned. | Host button-handler and pixel checks. |
| Lantern left in the beached boat | Returning to shore exposed the lantern in the boat. | Saved from Sandy Cave, returned and took the lantern, then restored. The lantern was again in the boat. | Host checked the original container ownership and retrieved it again from shore. |

The host test suite now includes the mine climb directions and bracelet take, re-entry, drop, and restore regression. Zork II and III controller replays also completed with 400 points and 7 potential.

## Follow-up verification

| Screen and states | Story and placement checks | Observed interaction results |
| --- | --- | --- |
| Dam Base (111), folded boat | The folded hull rests on the landing. The story refusal stays in the room. | Entering `SWIM` through the GBA keyboard returned “Swimming isn't usually allowed in the dungeon.” The player could continue. |
| Atlantis (4), focused, menu open, trident taken | The trident rests on the near shore. The direction indicators are in the header and the selection label and action menu are below the art. | Opening the action menu left every room-art pixel identical to the focused view. Taking the trident removed it from the room. |
| Behind House (85), Kitchen (27), Living Room (75), Cellar (33) | The vertical sliding window, floor hatch, underside of the hatch, and broken west doorway show their story states. | The closed window refused entry. The opened window allowed entry. The hatch changed when opened through the GBA menu and looked closed again after the story barred it behind the player. The Cyclops shortcut worked in both directions. |
| Dam (178), Lobby (120), Maintenance (224), Reservoir (95, 123, 191) | Lobby and maintenance are indoor rooms. The correct bolts, colored buttons, indicator, sluices, water level, and exposed trunk are visible at the relevant points in the route. | A knife and a disabled wrench attempt failed. The enabled wrench changed the sluice view. The water drained and exposed the trunk. The blue button caused a visible flood and waiting led to the story's drowning outcome. |
| Frigid River (37, 174, 45, 182, 53), Sandy Beach (30) | The banks, downstream landmarks, and sand distinguish the reaches. The boat shows folded, inflated, punctured, afloat, and beached art. Buoy, shovel, and supported cargo appear in the proper places. | The pump inflated the boat. Carrying a sword aboard punctured it. The original menu's repair alias had emitted an unknown word. Selecting **Viscous material** in mGBA repaired it, and the host replay inflated it again and relaunched. Floating past the safe landing still led over the falls. |
| Boat cargo, Frigid River to Sandy Cave | A lit lantern dropped aboard appeared in the hull and traveled with the boat. | Taking it while afloat succeeded. Landing, leaving for Sandy Cave, and checking inventory left it in the boat. Returning to Sandy Beach and waiting still allowed retrieval. A longer wait let the lit lantern run out of power while it stayed in the boat. |
| Up a Tree (5), Attic (187), maze skeleton (218) | The nest and egg sit on a branch, rope and knife rest on the attic table, and the cave remains mark the key and coin bag. | The egg left the nest when taken. Opening it by hand failed, and dropping it from the tree damaged it. The maze key and coins could be taken. |
| Mirror Rooms (52, 110), Dome (11), Shaft (245, 249), forest grate (160, 216) | The two mirrors have distinct views. The rope hangs from the railing. The chain and basket track the upper and lower shaft. The grate reads as a floor opening above and overhead from below. | Rubbing the mirror changed rooms. Tying the rope and lowering or raising the basket changed what was visible. The skeleton key opened the shortcut, and both crossings worked. The locked grate blocked passage. |
| Egyptian Room (241), coffin closed, open, empty, carried, and dropped | The coffin's long edge follows the plinth, the lid hinges toward the wall, and the sceptre sits in the trough. | The GBA Open action displayed the open pose. Taking the sceptre emptied the trough. Picking up the coffin cleared the plinth. Dropping it put it on the floor with its contents intact. Restoring that save reproduced the room-art pixels exactly. |
| Torch Room (18), Altar (129), Hades (38) | The torch rests on its pedestal. The open book and candles sit on the altar. The dropped candles, hot bell, and book appear on the floor when present. | Taking and dropping the torch moved it from pedestal to floor. The spirits blocked early passage. Bell, candles, and prayer cleared them. The lit torch still triggered the Gas Room explosion. |
| Stone Barrow (244), approach and ending | The forest tomb exterior changes to the lit cavern, stream, and wooden bridge on entering. | The full treasure route opened the Barrow. The final score and story pages displayed with the interior view. |

## Additional placement checks

- In the Kitchen, the bottle disappeared from the table when carried and returned on the floor when dropped.
- In the Living Room, the lit lantern and sword occupied separate ground positions after both were dropped.
- At the Bat Room, taking garlic in kept the bat overhead. Leaving the garlic outside led to the original carryaway to Ladder Bottom.

## Coverage

The tables record the room states, action outcomes, and persistence paths exercised. Use the checklist to record additional combinations as Unchecked until they are tested.


<details>
<summary>All 83 first-entry screens on the winning route</summary>

“State checks” marks rooms with targeted branch or visual checks in the tables above or the repeat audit's hazard cases. “Route entry” records navigation and the native first-entry render. Use the checklist to extend coverage within each room.

| Room ID | Screen | Backdrop | Evidence |
| --- | --- | --- | --- |
| 64 | West of House | `west` | State checks |
| 209 | South of House | `south` | Route entry |
| 85 | Behind House | `rear` | State checks |
| 27 | Kitchen | `kitchen` | State checks |
| 75 | Living Room | `living` | State checks |
| 137 | North of House | `north` | Route entry |
| 238 | Forest Path | `forest` | Route entry |
| 5 | Up a Tree | `z1_tree_canopy` | State checks |
| 187 | Attic | `z1_attic` | State checks |
| 33 | Cellar | `z1_cellar_hatch` | State checks |
| 127 | The Troll Room | `troll_room` | State checks |
| 104 | Maze | `cavern` | Route entry |
| 193 | Maze | `cavern` | Route entry |
| 54 | Maze | `cavern` | Route entry |
| 218 | Maze | `z1_maze_skeleton` | State checks |
| 134 | Maze | `cavern` | Route entry |
| 158 | Maze | `cavern` | Route entry |
| 170 | Maze | `cavern` | Route entry |
| 78 | Cyclops Room | `cyclops_room` | State checks |
| 82 | Treasure Room | `treasure_room` | State checks |
| 117 | Strange Passage | `z1_strange_passage` | State checks |
| 130 | East-West Passage | `cavern` | Route entry |
| 15 | Round Room | `hall` | Route entry |
| 175 | North-South Passage | `cavern` | Route entry |
| 106 | Chasm | `cavern` | Route entry |
| 123 | Reservoir South | `z1_reservoir_south` | State checks |
| 178 | Dam | `dam` | State checks |
| 120 | Dam Lobby | `z1_dam_lobby` | State checks |
| 224 | Maintenance Room | `z1_maintenance` | State checks |
| 95 | Reservoir | `z1_reservoir_drained` | State checks |
| 191 | Reservoir North | `z1_reservoir_drained` | State checks |
| 4 | Atlantis Room | `atlantis` | State checks |
| 69 | Engravings Cave | `cavern` | Route entry |
| 11 | Dome Room | `z1_dome` | State checks |
| 18 | Torch Room | `z1_torch_room` | State checks |
| 2 | Temple | `hall` | Route entry |
| 241 | Egyptian Room | `egyptian` | State checks |
| 129 | Altar | `z1_altar` | State checks |
| 176 | Cave | `cavern` | Route entry |
| 38 | Entrance to Hades | `hades` | State checks |
| 13 | Land of the Dead | `hades_inside` | State checks |
| 52 | Mirror Room | `z1_mirror_north` | State checks |
| 228 | Narrow Passage | `cavern` | Route entry |
| 247 | East of Chasm | `cavern` | Route entry |
| 122 | Gallery | `hall` | Route entry |
| 205 | Loud Room | `hall` | Route entry |
| 128 | Clearing | `forest` | Route entry |
| 22 | Canyon View | `canyon` | Route entry |
| 21 | Rocky Ledge | `cavern` | Route entry |
| 74 | Canyon Bottom | `cavern` | Route entry |
| 125 | End of Rainbow | `end_of_rainbow` | State checks |
| 47 | Aragain Falls | `aragain_falls` | State checks |
| 87 | Forest | `forest` | Route entry |
| 62 | Cave | `cavern` | Route entry |
| 110 | Mirror Room | `z1_mirror_south` | State checks |
| 139 | Cold Passage | `cavern` | Route entry |
| 31 | Slide Room | `hall` | Route entry |
| 188 | Mine Entrance | `cavern` | Route entry |
| 171 | Squeaky Room | `hall` | Route entry |
| 235 | Bat Room | `cavern` | State checks |
| 245 | Shaft Room | `z1_shaft` | State checks |
| 93 | Smelly Room | `hall` | Route entry |
| 46 | Gas Room | `z1_gas_room` | State, UI access, persistence |
| 173 | Coal Mine | `cavern` | Route entry |
| 90 | Coal Mine | `cavern` | Route entry |
| 112 | Coal Mine | `cavern` | Route entry |
| 3 | Coal Mine | `cavern` | Route entry |
| 16 | Ladder Top | `z1_ladder_top` | State and UI access |
| 72 | Ladder Bottom | `z1_ladder_bottom` | State and UI access |
| 156 | Dead End | `hall` | Route entry |
| 8 | Timber Room | `hall` | Route entry |
| 249 | Drafty Room | `z1_shaft_bottom` | State checks |
| 89 | Machine Room | `hall` | Route entry |
| 111 | Dam Base | `z1_dam_base` | State checks |
| 37 | Frigid River | `z1_dam_base` | State checks |
| 174 | Frigid River | `z1_river_bend` | State checks |
| 45 | Frigid River | `z1_river_valley` | State checks |
| 182 | Frigid River | `z1_river_beach` | State checks |
| 30 | Sandy Beach | `z1_sandy_beach` | State checks |
| 155 | Sandy Cave | `cavern` | Route entry |
| 40 | Shore | `water` | Route entry |
| 202 | On the Rainbow | `hall` | Route entry |
| 244 | Stone Barrow | `z1_barrow_outside` | State checks |

</details>
