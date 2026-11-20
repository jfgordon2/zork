# Zork II visual and QA audit

This is the pre-update audit. See the [scenery changes and re-audit](zork-ii-re-audit.md) for the current cartridge, updated evidence and remaining work.

Audited November 17, 2026. The current cartridge reached the ending with **400/400 points in 419 moves**. The winning route visited **73 room IDs**. All 73 first-entry views received a visual review and a button-access check. The targeted persistence checks passed. **Fourteen visual follow-up groups remain**, including missing puzzle scenery and some misleading placements. No progression blocker was found on the tested winning route.

[Selected issues, with full game screens](docs/images/zork-ii-audit-preview.png) · [All 73 route views](docs/images/zork-ii-scene-review.png) · [Screen checklist](qa-screen-checklist.md)

This was an audit of the existing build; game code, artwork, and the production ROM were unchanged. The story authority was the bundled [Zork II program](stories/zork2.z3), Release 63, serial 860811. The audited `zork-trilogy.gba` SHA-256 was `7dfae342f3e8eeb2c2a5197195be1e849c10c21ed6b0eb337a0d358c0daff885`.

## Follow-up fixes

P1 means a missing visual cue central to solving a puzzle. P2 means a substantial story, placement, or route mismatch. P3 covers the remaining scenery and collectible details. These priorities concern the adaptation's visuals; the story text and menus still provide the tested solutions.

| ID | Priority | Screens and states | Finding and required follow-up |
| --- | --- | --- | --- |
| Z2-01 | P1 | Crypt (125): lamp on/off, secret door shut/open | Switching off the lantern leaves the same visibly illuminated hallway. The text instead describes darkness, a faint doorway outline, and a glowing **F**. Add a dedicated crypt with the twelve poled heads, a dark state that preserves those faint cues, and an opened secret passage. Opening the door and reaching the ending worked in mGBA. Ordinary darkness works elsewhere; this needs a Crypt-specific visual state. |
| Z2-02 | P2 | Circular Room (119), Top of Well (8): outside/inside bucket, ascent/descent | Both ends use the same hallway. The wooden bucket, brick shaft, wall etchings, and upper landing are absent. Draw connected views, distinguish riding from standing outside, and place water inside the bucket. Filling and draining it already move the player correctly. |
| Z2-03 | P2 | Tea Room (190), Posts Room (122), Pool Room (82): normal/shrunken/grown, pool full/evaporated | The normal tea table and colored cakes are readable. Shrinking replaces the table with a hallway, losing the enormous table legs, overhead tabletop, and dangerous floor cracks. The Pool Room lacks the pool of tears and leak, and does not clearly show the exposed contents after evaporation. Add scale-consistent views and item supports through the whole sequence. |
| Z2-04 | P2 | Dingy Closet (54), Cage (106): trap, poison, robot rescue | The robot and red sphere appear, but the steel cage, poison, and mangled cage after rescue do not. The trap reads as another open hallway. Add visible barred confinement and its lifted/damaged state, keeping the robot reachable outside. The actual robot-order menu successfully freed the player. |
| Z2-05 | P2 | Ravine and stream route (168, 21, 53, 212, 30, 231, 124, 171); Stone Bridge (80), Cool Room (172), Ice Room (161) | The wooden and stone bridges are missing. Lake art replaces a stream, ford, or path, so the pursuing dragon appears to stand in water. The Ice Room has no glacier or cleared western passage. Build a connected ravine/bridge/ford sequence and lava-and-ice views before and after the dragon melts the glacier. Match ledge height and the stream's depth between approaches. |
| Z2-06 | P2 | Lava Tube/Room (184, 15), Volcano Bottom/Core (176, 34), flight (113, 86, 177), ledges (13, 143) | Hallway and cave fallbacks omit the balloon's wicker basket, cloth envelope, burner receptacle, wire, hooks, and altitude. Boarding, inflating, flying, landing, and mooring have little visible distinction. Add the vehicle and connected vertical views with separate ground, airborne, landed, and tied states. Also show the western door/chimney after paying the volcano gnome for escape. |
| Z2-07 | P2 | Tiny Room (46), Dreary Room (59): keyhole covers, mat, displaced key, locked/open door | Identical halls omit the oak door, barred window, keyholes, covers, and large table. The blue sphere reads as a floor object although the text places it on the table. Draw both sides of the door and show the mat, opener, key, and sphere on the surfaces dictated by their current parents. |
| Z2-08 | P2 | Dusty Room (182): closed safe, fuse burning, blast aftermath | The safe, fuse hole, damaged door, debris, crown, and card are missing from the room view. Returning after the explosion still shows an undamaged hall. Add before/after scenery and contents, including the damage described as leaving the walls ready to collapse. Preserve the original explosion and timing outcomes. |
| Z2-09 | P2 | Garden north/center/topiary (234, 157, 218); Gazebo (16), original and dropped teapot | Wild pine forest contradicts the formal hedges, white paths, roses, shaped bushes, and moss-lit cavern ceiling. It also clashes with the gazebo's garden. The foreground table occupies the supposed floor anchors: a teapot dropped onto the ground still looks placed on its edge. Add coherent garden views and visible gazebo floor, then assign separate support and ground anchors. Item ownership and persistence already work. |
| Z2-10 | P2 | Aquarium (233), broken tank; Cerberus (162), before/after collar; Guarded Room (28), awake/asleep lizard | The swimming serpent and broken aquarium have appropriate distinct art, but the dead serpent has no visible body after the break. Cerberus keeps the same guarding pose and has no visible collar after becoming friendly. The door's lizard guardian is also undrawn, including after feeding it. Add those encounter states. The collar remains attached after leaving and returning, and passage opens correctly. |
| Z2-11 | P2 | Bank Entrance (92), tellers (152, 243), Chairman's Office (49), East Viewing Room (102) | These rooms revert to the same hall while the depository, bare Small Room, and vault have dedicated views. The teller/viewing signs and the chairman's wall portrait are missing. Extend the existing bank's materials and passage arrangement; place the portrait on the wall and remove it when taken. Avoid adding visible ordinary exits to the magical, exitless Small Room. |
| Z2-12 | P2 | Stairway (240) and Oddly-angled Rooms (19, 202, 121, 242, 75) | Hallway repetition omits the angular baseball-diamond layout, diamond-shaped windows, and directional landmarks. Give these rooms the geometry described by the story, including the stair approach and club's location. Retain the story's unusual travel rules. |
| Z2-13 | P2 | Landing ending (230); death recovery in Red/Blue/White Mist (224, 227, 228) | The ending remains a hall throughout text describing magical runes, green power lines, a descending staircase, and a vast red-lit hall with distant statues. The recovery spheres also use a hall instead of colored mist and crystalline enclosure. Add appropriate ending stages and sphere interiors. The black-mist encounter returns directly to life; do not invent a stationary black-room stop. |
| Z2-14 | P3 | Pearl Room (38), Low Room (187), Cobwebby Corridor (206), Guarded Room (28), Workshop (45), Library (136), Fresco Room (141), Room 8 (207), Kennel (128) | Remaining fallback rooms omit distinctive supports and props: the necklace, green paper, cobwebbed door and lizard, library books, fresco, repellent, and dog collar. Add these in their described setting. Include the princess-opened marble shortcut in Z2-05's Marble Hall states. Make portable objects disappear, relocate, and return according to the story rather than baking them into backgrounds. |

The Riddle Room's closed/open stone door, ruby/diamond/sapphire stands and fused obsidian stand, upright/levitated/fallen menhir, swimming serpent, bank vault bills, and Crypt Anteroom's door already have useful state-dependent art. Preserve those when filling the gaps.

## Interaction and branch checks

The main parser replay used the cartridge's story interpreter and renderer. A separate controller-command replay resolved **308 exact and 74 equivalent actions, with zero missing commands**, and also reached 400 points. That runner omitted ten redundant failed inputs and substituted four waits to preserve timing; it is not proof that every rejection was exercised through buttons. The failure checks below used direct story commands, with selected cases also exercised through the actual button handlers or mGBA.

The existing `make test` suite also passed: all three stories, dictionary decoding, object menus, puzzle maps, route history, and save recovery.

| Sequence | Outcomes exercised | Result |
| --- | --- | --- |
| Riddle and well | Closed door and wrong answer refused; **WELL** opened it. The speech picker also entered the answer successfully. Entering the bucket and adding water raised it; removing its water lowered it again. Climbing the shaft directly was refused. | Pass for tested outcomes; visual gaps Z2-02. |
| Cakes and pool | Green cake shrank the player, blue restored size, westward chasms blocked passage, and throwing the red cake evaporated the pool. Eating red or orange cake killed the player in different ways. Opening the flask killed through its vapors; trying to drink it while closed was refused. | Pass for tested outcomes; Z2-03. |
| Robot, controls, and cage | Robot movement, triangular switch, and **Lift cage** rescue succeeded. The round robot order did nothing; square increased the whirring. Pressing the hazardous switches personally caused death. Taking the red sphere without timely rescue led to poison death. | Pass for tested outcomes; Z2-04. The mGBA capture verifies the rescue using Give orders. |
| Door and key | Locked door and covered keyhole refused early attempts. Mat under door, cover opened, key displaced with opener, mat retrieved, key recovered, and door unlocked/opened all succeeded. | Pass for tested outcomes; Z2-07. |
| Lizard and workshop door | Feeding the pool's candies put the lizard to sleep. Unlocking the door with the gold key and opening it allowed passage to the Wizard's Workshop. | Pass for tested outcomes; missing guardian in Z2-10. |
| Dragon and princess | The main attack-and-retreat route melted the glacier. An alternate **kick-and-retreat** route also worked across the bridge and Cool Room into the ice. Staying and repeatedly provoking the dragon killed the player. Princess rescue and her later route opened the marble shortcut on the winning replay. | Pass for tested outcomes; Z2-05. |
| Aquarium and crystal stands | Intact tank prevented retrieval; breaking it exposed the clear sphere and dead serpent. Wrong stand assignments and multiple spheres on one stand were tried. Correct red/ruby, clear/diamond, blue/sapphire placement fused the black sphere on obsidian. Taking it removed it from the stand. | Pass for tested outcomes; corpse missing in Z2-10. |
| Balloon and safe | Fuel, ignition, inflation, ascent, landing, tying, untying, and departure succeeded. Leaving an untied balloon let it ascend away. The story refused stepping out while airborne. Remaining beside the lit explosive killed the player; leaving and returning revealed the blasted safe and crown. | Pass for tested outcomes; Z2-06 and Z2-08. Original consequences remain intact. |
| Volcano gnome escape | After the untied balloon drifted away, waiting summoned the gnome. Giving him the collectible zorkmid opened the western chimney route back to Volcano Bottom. Offering the lantern instead destroyed it and left the player in darkness; further waiting let him depart. | Pass for tested outcomes; the new escape opening is undrawn in Z2-06. |
| Bank | Curtain travel, vault entry, bills retrieval, banker arrival, and payment with the portrait worked. Giving the banker the lantern made him discard it, leaving the player in darkness; neglecting him let him leave. Ordinary directions in the exitless Small Room failed. | Pass for tested outcomes; Z2-11. |
| Demon and menhir | Unprepared Float was refused. Prepared Float opened the southwest route. Paying the demon and ordering him against the Wizard succeeded on the main route; a separate paid-demon order moved the menhir instead. The latter produced the fallen-stone artwork. | Pass for tested outcomes. |
| Cerberus, Crypt, ending, and death | Cerberus blocked early passage; collaring allowed entry and return. Crypt door opened; lamp off revealed the secret-door text; opening it reached the ending. Ordinary darkness hid room objects, and walking in darkness caused grue death. Red-to-blue-to-white mist travel led through the black-mist encounter back to life. | Pass for tested outcomes; Z2-01, Z2-10, Z2-13. |

## UI access and screen obstruction

Across the 73 first-entry states, the host drove the production button handlers through **188 in-room target entries, 416 inventory entries, 4,357 argument pickers, and 56,675 scrollable menu rows**. These are repeated entries across room states, not counts of unique objects or distinct puzzle outcomes.

The checks covered focus cycling, Room actions, Nearby objects, primary actions, action categories, tools/targets, directions, available character orders, the twelve Paths entries, inventory, L/R list paging, and cancel/back selection. Pending story pages could be dismissed before entering the menus. Browsing left the story's turn, score, and location unchanged.

**All 5,372 room-art crop comparisons matched** while browsing those controls. Menus and labels remained below the art, with compass information in the header. No control became unreachable because of a list's length or a menu covering the scene in these tested states. This establishes UI access; it does not make an undrawn bucket, cage, or other missing prop visually present.

Native mGBA checks used an isolated copy of the unchanged ROM and temporary cartridge saves. They verified Crypt lamp switching and secret-door access through the ending; teapot Take/Drop, travel, Save/Restore, and launcher Continue; and the robot cage rescue through Give orders. Additional native captures confirmed the missing bucket at the top of the well and missing basket while airborne. The preview labels distinguish mGBA captures from exact host-rendered frames.

## Persistent state and dropped items

| Saved or changed state | Leave and return | Save, mutate, Restore / Continue | Result |
| --- | --- | --- | --- |
| Gazebo teapot taken from table and dropped | Left for North End of Garden and returned. The teapot remained a child of the room, rather than returning to the table or inventory. | Saved the ground state, took it, and restored it to the ground. Launcher Continue also restored that state. Native mGBA repeated all of these steps. | State passes; misleading floor placement is Z2-09. |
| Riddle stone door opened through the speech picker | Went east to Pearl Room and west back. The passage stayed open. | Save/Restore preserved the open door and matching room art. The original game refuses closing this many-ton door; that refusal was retained. | Pass. |
| Red and clear spheres placed, blue carried | Restored the partial setup, left east to Workshop, and returned west. Stand contents remained correct. | Saved before fusion, completed fusion, then restored. The partial placement, carried blue sphere, and exact prior artwork returned. | Pass. |
| Aquarium broken | Went east to Workroom and returned west. The tank stayed broken and the live serpent remained absent. | Saved intact, broke it, and restored. The intact tank and live swimming serpent returned with matching artwork. | Pass; dead serpent art is Z2-10. |
| Closet before triggering the cage | Robot orders freed the player after taking the red sphere; the original trap changes were applied. | Saved before the trap, triggered it, ordered rescue, then restored. Robot and red sphere returned to the saved untriggered closet state. | Pass; cage art is Z2-04. |
| Menhir and collared Cerberus | Leaving and returning retained the lifted menhir and attached collar, with the appropriate routes traversable. | Separate Save/Restore combinations were not exercised for these two states. | Re-entry passes; additional persistence combinations unchecked. |

The first five host cases also made nine exact room-art comparisons after restoring or revisiting the relevant setup. These checks establish the sampled ownership, exits, puzzle state, and art; they do not certify every portable object in every room. Multiple spheres sharing a stand were checked, but arbitrary multi-item floor drops throughout Zork II remain unchecked.

## Coverage boundaries and repeat pass

The reviewed route, failure branches, native checks, and persistence cases above are the completed coverage. Additional room entries outside this route, all Wizard spell/random timing combinations, every possible offering, every death variation, every drop location, and Save/Restore at every intermediate timer remain **Unchecked**. After implementing each follow-up, use the screen checklist for its before/after states, reverse approaches, take/drop states, actual button actions, and persistence paths. Repeat the 400-point controller route and obstruction checks after integrating the art.

The audit's scratch harnesses, command inputs, saves, and full response logs are in `build/zork-ii-audit/`. They include `route-replay.log`, `controller.log`, `fixtures.log`, `persistence.log`, and the named branch logs. The durable findings and screenshot boards are linked above.

<details>
<summary>All 73 first-entry route screens</summary>

Every row received the first-entry visual review and UI browsing check. Findings can concern later states in that room, as described above. A route view without an added finding is not a certification of all possible actions or persistence states. Targeted outcome and persistence coverage is recorded in the preceding tables.

| Room ID | Screen | First-entry backdrop | Visual review / related states |
| --- | --- | --- | --- |
| 51 | Inside the Barrow | `barrow` | Route view reviewed; no added finding |
| 168 | Narrow Tunnel | `cavern` | Issue: Z2-05 |
| 21 | Foot Bridge | `water` | Issue: Z2-05 |
| 183 | Great Cavern | `great_cavern` | Route view reviewed; no added finding |
| 53 | Shallow Ford | `water` | Issue: Z2-05 |
| 158 | Dark Tunnel | `cavern` | Route view reviewed; no added finding |
| 234 | North End of Garden | `forest` | Issue: Z2-09 |
| 16 | Gazebo | `gazebo` | Issue: Z2-09 |
| 212 | Path Near Stream | `water` | Issue: Z2-05 |
| 144 | Carousel Room | `carousel` | Route view reviewed; no added finding |
| 238 | Riddle Room | `riddle_room` | Route view reviewed; no added finding |
| 38 | Pearl Room | `hall` | Issue: Z2-14 |
| 119 | Circular Room | `hall` | Issue: Z2-02 |
| 8 | Top of Well | `hall` | Issue: Z2-02 |
| 190 | Tea Room | `tea` | Cakes/table reviewed; related shrink states: Z2-03 |
| 122 | Posts Room | `hall` | Issue: Z2-03 |
| 82 | Pool Room | `hall` | Issue: Z2-03 |
| 187 | Low Room | `hall` | Issue: Z2-14 |
| 87 | Machine Room | `robot_machine` | Route view reviewed; no added finding |
| 54 | Dingy Closet | `hall` | Issue: Z2-04 |
| 106 | Cage | `hall` | Issue: Z2-04 |
| 206 | Cobwebby Corridor | `hall` | Issue: Z2-14 |
| 28 | Guarded Room | `hall` | Issue: Z2-10, Z2-14 |
| 218 | Topiary | `forest` | Issue: Z2-09 |
| 157 | Formal Garden | `forest` | Issue: Z2-09 |
| 30 | Marble Hall | `hall` | Issue: Z2-05 |
| 231 | Deep Ford | `lake_shore` | Issue: Z2-05 |
| 124 | Ledge in Ravine | `cavern` | Issue: Z2-05 |
| 46 | Tiny Room | `hall` | Issue: Z2-07 |
| 59 | Dreary Room | `hall` | Issue: Z2-07 |
| 171 | End of Ledge | `cavern` | Issue: Z2-05 |
| 248 | Dragon Room | `dragon_room` | Route view reviewed; no added finding |
| 80 | Stone Bridge | `water` | Issue: Z2-05 |
| 172 | Cool Room | `hall` | Issue: Z2-05 |
| 161 | Ice Room | `hall` | Issue: Z2-05 |
| 201 | Dragon's Lair | `dragon_room` | Route view reviewed; no added finding |
| 45 | Wizard's Workshop | `hall` | Issue: Z2-14 |
| 94 | Wizard's Workroom | `workroom` | Route view reviewed; no added finding |
| 233 | Aquarium Room | `aquarium` | Issue: Z2-10 |
| 184 | Lava Tube | `hall` | Issue: Z2-06 |
| 15 | Lava Room | `hall` | Issue: Z2-06 |
| 176 | Volcano Bottom | `hall` | Issue: Z2-06 |
| 34 | Volcano Core | `hall` | Issue: Z2-06 |
| 113 | Volcano Near Small Ledge | `cavern` | Issue: Z2-06 |
| 13 | Narrow Ledge | `cavern` | Issue: Z2-06 |
| 136 | Library | `hall` | Issue: Z2-14 |
| 86 | Volcano by Viewing Ledge | `cavern` | Issue: Z2-06 |
| 177 | Volcano Near Wide Ledge | `cavern` | Issue: Z2-06 |
| 143 | Wide Ledge | `cavern` | Issue: Z2-06 |
| 182 | Dusty Room | `hall` | Issue: Z2-08 |
| 141 | Fresco Room | `hall` | Issue: Z2-14 |
| 92 | Bank Entrance | `hall` | Issue: Z2-11 |
| 152 | West Teller's Room | `hall` | Issue: Z2-11 |
| 200 | Safety Depository | `bank_depository` | Route view reviewed; no added finding |
| 49 | Chairman's Office | `hall` | Issue: Z2-11 |
| 64 | Small Room | `bank_small_room` | Route view reviewed; no added finding |
| 2 | Vault | `bank_vault` | Route view reviewed; no added finding |
| 243 | East Teller's Room | `hall` | Issue: Z2-11 |
| 102 | East Viewing Room | `hall` | Issue: Z2-11 |
| 207 | Room 8 | `hall` | Issue: Z2-14 |
| 91 | Pentagram Room | `pentagram_room` | Route view reviewed; no added finding |
| 159 | Menhir Room | `menhir_room` | Route view reviewed; no added finding |
| 128 | Kennel | `hall` | Issue: Z2-14 |
| 240 | Stairway | `hall` | Issue: Z2-14 |
| 19 | Oddly-angled Room | `hall` | Issue: Z2-12 |
| 202 | Oddly-angled Room | `hall` | Issue: Z2-12 |
| 121 | Oddly-angled Room | `hall` | Issue: Z2-12 |
| 242 | Oddly-angled Room | `hall` | Issue: Z2-12 |
| 75 | Oddly-angled Room | `hall` | Issue: Z2-12 |
| 162 | Cerberus Room | `cerberus_room` | Issue: Z2-10 |
| 194 | Crypt Anteroom | `crypt_anteroom` | Route view reviewed; no added finding |
| 125 | Crypt | `hall` | Issue: Z2-01 |
| 230 | Landing | `hall` | Issue: Z2-13 |

</details>
