# Zork II completion audit

The four remaining optional-room scenery gaps, bucket viewpoint, friendly Cerberus, active fuse, crowded item placement, and finale animation are implemented. The connecting door now matches from both rooms, opens inward at the Workshop's right jamb with concealed hinge hardware, and shows the actual adjoining room through each opening.

**Follow-up visual review, November 18, 2026:** the current source PNG comparison found that the closed Guarded Room face no longer reads as the same oak door shown in the Workshop, and that the Tiny Room open-door view omits the Dreary Room's table and red-lit crack. These current-source findings are recorded in the [paired entrance/exit audit](paired-entrance-exit-art-audit.md); the earlier completion result remains historical evidence for the reviewed ROM and captures.

[Latest game screens](docs/images/zork-ii-completion-preview.png) · [Completed scenery](docs/images/zork-ii-completion-scenes.png) · [Door pair](docs/images/zork-ii-door-pair.png) · [Playable cartridge](zork-trilogy.gba) · [Art sources](assets/zork-ii-art.md) · [Preserved completion prompts](assets/zork-ii-completion-prompts.json)

Reviewed November 17, 2026 against the unchanged bundled Zork II Release 63, serial 860811. Production ROM and isolated native review copy SHA-256: `cf6627c96fe1670eda01ee8d1fe7c9fe9bf7e26293f062d87d5e9b70ecb29591`. This report supersedes the remaining-work list and temporary Workshop preference in the [previous re-audit](zork-ii-re-audit.md).

## Resolved follow-ups

| Follow-up | Result and evidence |
| --- | --- |
| Trophy Room 63 | Dedicated worked-stone chamber, north passage, degree, wands, nicked swords, bottled homunculi, stuffed owl and protected empty cabinet. These are fixed story scenery. The cabinet's refusal remains intact. Selection anchors identify the cabinet and its related scenery. |
| Wizard's Quarters 69 | Six dedicated variants: cave with furs/bones, Louis XIV furnishings, 1950s bunk bedroom, jungle hammock, austere cot/stool and Arabian furnishings. The scene follows the description actually printed. All six variants passed description, Save/Restore, Continue, crop and menu checks. |
| Volcano View 117 | Dedicated view from the eastern ledge across the dormant shaft to the western landings. Floor drops stay on the near eastern ledge, including when crowded; no item is placed over the shaft as if it were floor. Native take, leave and return passed. |
| West Viewing Room 204 | Dedicated bare green bank chamber and north-wall plaque. No invented desk or visible extra exit. The actual magical-bank return route passed. |
| Inside the bucket | Separate first-person wooden rim while aboard, whole bucket while outside. Contents follow the bucket; water is selected inside it rather than drawn as a floating puddle. Native Stand up, Enter and Restore passed. |
| Friendly Cerberus | Separate relaxed, grinning three-headed pose derived from the canonical guarding dog, using the comparable visible scale and matching materials. Connected anatomy, a single tail, and a separate attached collar. Native collar application, return and Restore passed. |
| Burning fuse | A small animated spark follows the live original I-FUSE clock, including an installed charge. Idle rendering spends no turns and draws no story randomness. Native lighting, detonation, and restoring the active charge passed. |
| Crowded floor drops | Focus priority, small overflow thumbnails, and a final foreground redraw keep the selected item visible. Markers stay inside the art band. Checked 466 selected objects over 76 playable states, plus five crowded return/Restore comparisons. |
| Vehicle and movable-container contents | Eight accepted balloon children, four bucket children and three steel-box contents checked. Selection promotes contents beyond ordinary visible capacity. Open/closed visibility, carrying and dropping the box in another room, capacity rejection and Restore passed. |
| Finale | Green power lines undulate, flare on the page containing the story's flare cue, and fade. The stage follows pagination rather than a hard-coded page number. Animation leaves the final story state, move count and random state unchanged. Native narration-page and fade checks passed. |
| Connecting door | Same worn oak planks, rounded top, iron bands and thickness from both sides. Lizard inset on the Guarded face. Away from Guarded means inward into Workshop, at its right jamb; hinge hardware concealed. Guarded looks into warm Workshop arches; Workshop looks into dusty gray-green Guarded masonry and its north passage. Native travel and full-screen crops reviewed. |

Sprite import preserves source aspect after transparent trimming; the guarding and friendly Cerberus silhouettes are 59×55 and 58×56 pixels. This corrects the initial friendly-pose packing stretch.

The completed scenery set has **81 background/state images**, with **37 additional item, vehicle or encounter poses** across the two art passes. Built-in ImageGen produced the new bitmap assets. The checked-in PNGs are production sources; the JSON records preserve final prompts and references. Resampling, transparent packing and QA contact sheets are deterministic build steps.

## Story and controller regression

The winning replay reaches **400/400 in 419 moves** and visits **73 room IDs**. Its story transcript matches the previous pass after removing screenshot and coverage diagnostics. Sixteen fixture/branch input sets also match, including the fixture-generating full route and 15 focused branch sets. They cover rejected tools and orders, wrong riddle answers, well rise/descent, cake/flask deaths, robot/cage timeout and rescue, keyholes, both dragon routes, sphere fusion, balloon fuel/mooring/abandonment, safe blast/death, bank routes, demon/menhir/Cerberus, crypt light/door states, death recovery and volcano-gnome payment/refusal.

Controller replay resolves **308 exact actions, 74 equivalent actions and zero missing commands**, reaching 400 points. As before, it omits ten redundant failed inputs and substitutes four waits. It does not establish that every story failure was entered through the controller.

The existing three-story suite and `make all test` pass. The original story bytes and mechanics are unchanged. Original hazards and container capacity refusals remain effective; the renderer does not prevent a player from abandoning useful items.

## UI, placements and persistence

The 73-route-state button audit checks **188 room targets, 416 bag entries, 4,357 argument pickers and 56,675 menu rows**, with **5,372 matching art-crop comparisons** while opening, scrolling and cancelling menus. The four optional rooms and all six Quarters variants received additional access checks. Story text and menu drawers remain below the art band; selected objects in the crowded-drop and cargo tests have markers entirely inside screen rows 15–90. No access blocker was found in these samples.

The existing teapot, riddle, crystal-stand, aquarium and cage persistence cases pass again, including nine exact art-crop comparisons. Five additional crowded states check leaving, returning and restoring dropped objects: Gazebo, Trophy, Quarters, Volcano View and West Viewing. The moved steel box retains its three children, hides them when closed, and restores the saved parent relationships.

Quarters appearance uses previously reserved save-payload byte 17; payload layout, signature and checksums retain their existing format. Old saves load with an unknown appearance until a description is observed; the fallback is the cave variant, and the Restore message makes no competing decor claim. Reading text and saving/restoring the presentation state consume no extra LOOK, turn or random number. A regression check also verifies legacy zero metadata and character-at-a-time story output.

The native mGBA pass uses the final cartridge and actual GBA button handlers. Lua reads state for assertions and captures frames; actions travel through the controller, with fixtures loaded only to establish starting conditions. Checks cover both open-door views and traversal, Trophy, actual Quarters decor changes plus Restore and launcher Continue, crowded Volcano View return, West Viewing's bank route, bucket boarding/exit/Restore, collar application and friendly Cerberus return/Restore, active fuse/blast/Restore, and the narrated ending flare/fade. Preview frames are native 240 × 160 captures enlarged without smoothing. The scenery board uses production crops and the six actual-description host renders.

Scratch inputs, source-derived host harness, logs, fixture saves, native scripts and captures are in `build/zork-ii-finish/`. `re-audit/regression-results.json` records the transcript comparisons. The one native test-script correction used West to return from the Crypt Anteroom, as specified by the story; the initial Northwest attempt was a harness route error, corrected and replayed.

## Scope

The documented follow-ups are addressed for the states above. This is broad route, branch, presentation and persistence coverage, not an exhaustive enumeration of every command, random event, portable-item combination or possible item arrangement. Future scenery and encounter work must follow the [scenery rules](assets/SCENERY_RULES.md) and [screen checklist](qa-screen-checklist.md), including actual destination views through doors, supported floor drops, entity identity, UI access and saved visual state.
