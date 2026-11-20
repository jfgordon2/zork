# Zork II scenery update and re-audit

**Historical pass:** the [completion audit](zork-ii-completion-audit.md) addresses the remaining work listed below and supersedes this pass's temporary Workshop-door preference. Use its ROM hash and previews for the current build.

The updated cartridge reaches **400/400 points in 419 moves** on the same 73-room winning route. This pass adds **72 background/state images and 35 item or vehicle poses**, replaces the audited generic scenery, and fixes support and contact placements. The Workshop's earlier open-door image is restored; the Guarded Room correction opens away from the viewer. The aquarium's live and dead serpent now share one canonical design.

[Updated game screens](docs/images/zork-ii-updated-preview.png) · [73 updated route views](docs/images/zork-ii-updated-scenes.png) · [Remaining optional-room gaps](docs/images/zork-ii-remaining-scenes.png) · [Art sources and prompts](assets/zork-ii-art.md) · [Screen checklist](qa-screen-checklist.md)

Reviewed November 17, 2026. The bundled Zork II Release 63, serial 860811 remains authoritative and unchanged. Production ROM and the isolated mGBA review copy have SHA-256 `36580dc2ba96113043011330b34ac33e77c901a364f65d69b9fac01d59306143`. The [earlier audit](zork-ii-audit.md) documents the pre-update build; its screenshots remain historical evidence.

## Changes against the earlier findings

| Finding | Implemented and reviewed | Status |
| --- | --- | --- |
| Z2-01 Crypt | Twelve heads on poles; actual available light selects lamplit or near-black art; faint F and secret-door states; ending reachable. | Implemented; native lamp-off and door/ending sequence passed. |
| Z2-02 Well | Connected brick shaft/upper landing, wooden bucket and water cue, correct shaft position at the top. | Improved; a separate viewpoint from inside the bucket remains. |
| Z2-03 Shrinking/pool | Huge table legs and floor cracks in Posts Room; ceiling leak and depressed basin; full/dry pool and exposed item state. | Implemented; shrink, growth, evaporation and fatal cake/flask outcomes replayed. |
| Z2-04 Cage | Bars in the same closet, faint gas, robot beyond bars, mangled cage after rescue. | Implemented; native Take and robot order passed. |
| Z2-05 Connected routes | Wooden bridge, fords, ravine ledges, accessible paths, stone bridge, cold junction, blocking/melted glacier and marble shortcut. | Implemented; main and alternate dragon routes replayed. |
| Z2-06 Balloon | Dormant shaft at successive elevations, two landings, POV basket/cargo/burner, full vehicle poses, mounted tether endpoints and gnome escape chimney. | Implemented for sampled states; crowded cargo and every possible item combination remain unchecked. |
| Z2-07 Door puzzle | Oak door with barred window, table/sphere, covers/keyholes, mat and inserted opener; open views from both sides. | Implemented; key recovery, unlocking and passage replayed. |
| Z2-08 Safe | Rusty wall box, charge in its front-door hole, blast debris and removed door, small crown/card inside the opened safe. | Improved; a distinct burning-fuse visual cue remains. Original explosion timing is preserved. |
| Z2-09 Garden/gazebo | Formal underground garden, hedges, gravel paths, roses and topiary; small tea table leaves visible plank floor for drops. | Implemented; native Take/Drop, return, Restore and Continue passed. |
| Z2-10 Encounters | Matched swimming/dead serpent; lizard head embedded in door with awake/asleep and foreshortened open-door views; attached Cerberus collar. | Improved; a separate relaxed Cerberus pose remains. |
| Z2-11 Bank | Dedicated entrance, teller, chairman and east-viewing rooms; movable wall portrait; original vault/depository/bare magical room preserved. | Implemented for earlier audit's rooms; optional West Viewing Room remains below. |
| Z2-12 Angled maze | Marble stair approach, distinct glassy angular rooms, club and diamond-window brightness, revealed secret passage. | Implemented; winning travel rules and secret opening replayed. |
| Z2-13 Ending/death | Descending landing toward red hall and distant statues; red/blue/white crystalline mist interiors. | Implemented as static states; transient green-line flare is not animated. |
| Z2-14 Other rooms/items | Necklace closet, robot low room, cobweb corridor, guarded door, workshop, library books, fresco, Room 8 and kennel; new portable poses. | Implemented for sampled rooms and states. |

The [scenery rules](assets/SCENERY_RULES.md) now require continuous access through openings, adult clearance, supported elevations, connected camera views, separate movable props, native-crop review and entity identity. Explicit user approvals take precedence: retain the restored Workshop hinge/composition.

## Story, interaction, UI and persistence evidence

The host harness used the current production interpreter, button handlers, renderer, packed RGB555 backgrounds and sprite headers. Its winning replay visited 73 room IDs and matched the earlier story transcript exactly, apart from diagnostic screenshot/coverage lines. The controller-command replay again resolved **308 exact and 74 equivalent actions with zero missing commands**, reaching 400 points. It still omits ten redundant failed inputs and substitutes four waits; this is not evidence that every failure was entered through buttons.

Across the 73 first-entry states, the button audit traversed **188 room targets, 416 bag entries, 4,357 argument pickers and 56,675 menu rows**, with **5,372 matching art-crop comparisons** while opening, scrolling and cancelling menus. These are repeated entries, not unique puzzles. Four additional optional-room views also received access checks. No UI access blocker was found in these samples. The normal story/menu drawer remains below the 240 × 76 art band.

All 15 fixture/branch input sets reran. Their story transcripts match the earlier audit after removing screenshot/coverage diagnostics. They include wrong riddle answers; well rise/descent and rejected climbs; cakes/flask deaths; robot buttons, cage timeout and rescue; keyhole failures; both dragon-lure routes and fatal provocation; sphere placement/fusion; balloon boarding, fuel, mooring, abandonment and airborne refusal; safe escape and blast death; bank routes; demon/menhir/Cerberus outcomes; crypt light/door states; death recovery; and the paid/refused volcano-gnome route.

Targeted teapot, riddle, crystal-stand, aquarium and cage persistence checks passed, including nine exact room-art comparisons after returning or restoring. Native mGBA repeated the teapot's ground drop, travel, Restore and launcher Continue; aquarium break via the real Throw target picker, leave/return and Restore; robot rescue; Crypt lamp switching and secret passage through the ending. Additional native captures verified the corrected Guarded Room, restored Workshop, blasted safe, top-of-well bucket and airborne basket. The preview contains native 240 × 160 mGBA frames enlarged without smoothing; the complete route board uses the host renderer.

`make all test` passed for the cartridge and existing three-story suite. Scratch inputs, logs, saves, capture scripts and review sheets are in `build/zork-ii-art/re-audit/`.

## Remaining work

1. **Optional-room scenery:** Trophy Room **63**, Wizard's Quarters **69**, Volcano View **117**, and West Viewing Room **204** still use fallback or reused scenery. They were naturally entered and checked separately from the 73-room winning route. The Wizard's Quarters changes its description between a cave, Louis XIV room, 1950s bunk bedroom, jungle hammock, austere room and Arabian furnishings; a single fixed bedroom would contradict some LOOK responses. Its art needs a reliable link to that changing description.
2. Add a distinct inside-bucket viewpoint, a relaxed Cerberus pose and an active-fuse cue. The current bucket, collar and installed charge already follow story state.
3. Stress-test arbitrary multi-item drops and crowded vehicle/container contents across more rooms. The sampled teapot, puzzle objects, container contents and selected-item priority passed; this does not certify every portable item at every floor position.
4. Optional animation polish: finale power-line flare and other events that currently resolve into a static final scene.

This pass establishes the tested route, sampled branches, state transitions and UI access. It does not claim exhaustive coverage of every story command, optional path, random event or possible item arrangement.
