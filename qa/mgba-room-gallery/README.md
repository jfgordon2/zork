# Zork Trilogy mGBA Room Gallery

> **Updated Nov 20.** The gallery contains 528 route and state screenshots for 275 rooms. Eleven affected screenshots were recaptured from the current ROM in mGBA; every capture reached the expected chapter and room. The remaining gallery screenshots are from the previous full pass. See [the targeted capture report](art-followup-refresh-results.csv). The Murky Room (Zork II, 237) is not reachable in play; see the route gap report.

A route-based index of actual mGBA screenshots from `zork-trilogy.gba`. Each PNG is the emulator's native **240 × 160** frame, including the GBA UI and story text. Most screenshots are taken after restoring a traced save and confirming the current chapter and room; route-only states are captured by replaying their traced input path and confirming the live room. Explicit Bag captures wait for the inventory screen before saving. Targeted visual refreshes take precedence over older frames for the entries they recapture.

ROM SHA-256: `0cf325cbe4f40a2e11309d42d92155eb2ebf3168421c445d645484f60512d65a`

## Browse

- [Interactive room and state gallery](index.html)
- [Complete CSV manifest](manifest.csv)
- [Last full mGBA checkpoint verification results](capture-results.csv)
- [Earlier targeted cavern art refresh report](cavern-refresh-results.csv)
- [Targeted Hallway 169 open-panel state refresh](mirror-state-refresh-results.csv)
- [Boat, menhir and cage state refresh](state-cue-refresh-results.csv)
- [Inventory state refresh](inventory-refresh-results.csv)
- [Well tether state refresh](well-refresh-results.csv)
- [Red Mist story-dependent dark/visible states](red-mist-refresh-results.csv)
- [Defeated Troll disappearance state](troll-refresh-results.csv)
- [Frozen Ice Room pre-dragon route](ice-frozen-refresh-results.csv)
- [Cliff Ledge upper-shelf character refresh](cliff-man-refresh-results.csv)
- [Shared boat interior refresh across five aboard states](boat-interior-refresh-results.csv)
- [Approved scenery and boat-overlay refresh](art-followup-refresh-results.csv)
- [Rooms without stable screenshots and route evidence](route-gaps.md)

| Chapter | Room IDs represented | Capture records | PNG files |
| --- | ---: | ---: | ---: |
| Zork I | 110 | 208 | 208 |
| Zork II | 84 | 216 | 216 |
| Zork III | 81 | 104 | 104 |
| **Total** | **275** | **528** | **528** |

The manifest identifies each room/state, its source checkpoint or route replay, the trace log or branch script that establishes the path, and the PNG path. Named state captures preserve variants such as doors, lights, puzzle geometry, water levels, vehicle placement, character states, and dropped/carried items. The shared boat interior was recaptured in all five saved aboard states after the current ROM build. Exact duplicate PNGs within a room are labeled in the gallery; several are aliases for one story moment rather than separate visual states.

Zork III's route notes include the stable mirror-room branches (51, 58, 161, 178) and Drafty Room (228). The last completed full mGBA pass confirmed all Zork III checkpoints then present in their expected rooms; that pass predates the current ROM build. The three Zork II sphere rooms (224, 227, 228) were generated from the death-recovery route in `checkpoints/zork-ii-mist-rooms.txt`.

Hallway 169's post-button route and state frames use the open-panel artwork. The story's global 107 records this panel state; it is not represented by the Z-machine `OPEN` attribute.

## Inputs and scripts

Every save and trace log the plan uses is committed under [route-data/](route-data/) (the original audit harness sources are in each folder's `generators/`), alongside the [checkpoints/](checkpoints/) saves built by `route_saves.c`. To refresh: `python3 qa/mgba-room-gallery/refresh_screenshots.py prepare`, run the printed command in mGBA's Scripting window, then `... finalize`. For a named subset, set `galleryRefreshSet` and load `refresh-sets.lua`.

## Capture result

The previous full pass captured 527 save-restoration screenshots and one route-only frame. After the current ROM build, 11 affected entries were recaptured in mGBA: Forest 167, Slide Room 31, both ladder views, Stream 86, Rainbow 202, Lava Room 15, the Tiny/Dreary open-door pair, and the Dreary Room entered state. All 11 matched the expected chapter and room ID. The gallery uses these newer frames for the affected entries; unchanged entries retain their prior screenshots. The frozen Ice Room frame has no destination save, so its plan record restores the pre-dragon room 206 checkpoint and `capture-gallery.lua` replays North → Down through the Paths menu before capturing (run it alone with `galleryRefreshSet='ice-frozen'` and `refresh-sets.lua`). [route-gaps.md](route-gaps.md) lists transient/fatal rooms and reports checkpoint restore results.
