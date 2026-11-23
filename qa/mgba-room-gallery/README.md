# Zork Trilogy mGBA Room Gallery

A route-based index of actual mGBA screenshots from `zork-trilogy.gba`. Each PNG is the emulator's native **240 × 160** frame, including the GBA UI and story text. Most screenshots are taken after restoring a traced save and confirming the current chapter and room; route-only states are captured by replaying their traced input path and confirming the live room. Explicit Bag captures wait for the inventory screen before saving. Targeted visual refreshes take precedence over older frames for the entries they recapture.

ROM SHA-256: `0166ea49d99a16e64abb8e281130018988f05c27701b839b1c7250a77af296b6`

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
- [Zork III alternate route captures](z3-routes-refresh-results.csv)
- [Zork III Restore and ending UI captures](z3-ui-refresh-results.csv)
- [Zork III route coverage notes](../../docs/audits/zork-iii-route-coverage.md)
- [Rooms without stable screenshots and route evidence](route-gaps.md)

| Chapter | Room IDs with verified frame | Capture records | Verified PNGs |
| --- | ---: | ---: | ---: |
| Zork I | 110 | 208 | 208 |
| Zork II | 84 | 216 | 216 |
| Zork III | 81 | 112 | 112 |
| **Total** | **275** | **536** | **536** |

The manifest identifies each room/state, its source checkpoint or route replay, the trace log or branch script that establishes the path, and the PNG path. Named state captures preserve variants such as doors, lights, puzzle geometry, water levels, vehicle placement, character states, and dropped/carried items. The shared boat interior was recaptured in all five saved aboard states after the current ROM build. Exact duplicate PNGs within a room are labeled in the gallery; several are aliases for one story moment rather than separate visual states.

Zork III's route notes include stable mirror-room branches, the Royal Puzzle entrance, and transient or unsafe outcomes such as Ladder Bottom 57 and the Guardian-side mirror rooms. The native route refresh records reachable branches in mGBA, and a separate UI replay captures in-game Restore plus the successful ending and its controller-driven Restart / Restore / Quit menu. The last completed full mGBA pass confirmed all Zork III checkpoints then present in their expected rooms; that pass predates the current ROM build. The three Zork II sphere rooms (224, 227, 228) were generated from the death-recovery route in `checkpoints/zork-ii-mist-rooms.txt`.

Hallway 169's post-button route and state frames use the open-panel artwork. The story's global 107 records this panel state; it is not represented by the Z-machine `OPEN` attribute.

## Inputs and scripts

Every save and trace log the plan uses is committed under [route-data/](route-data/) (the original audit harness sources are in each folder's `generators/`), alongside the [checkpoints/](checkpoints/) saves built by `route_saves.c`. To refresh: `python3 qa/mgba-room-gallery/refresh_screenshots.py prepare`, run the printed command in mGBA's Scripting window, then `... finalize`. For a named subset, set `galleryRefreshSet` and load `refresh-sets.lua`.

To review focus boxes around artwork embedded in backgrounds, set
`ZORK_FOCUS_REVIEW_DIR` to an existing directory when running
`build/transcript-replay-test`. The optional export captures distinct rendered
room art frames with their focused feature box. Convert them into a browsable
review page with `python3 qa/mgba-room-gallery/build_focus_review.py
/path/to/frames.tsv /path/to/review-gallery`.

## Capture result

527 of 527 save-restoration screenshots were verified in the last completed full mGBA gallery pass. That pass predates the current ROM build; rerun the full gallery to verify those captures against the current build. 9 route-only frames were verified by route replay. 0 targeted entries were captured after the current ROM build; the gallery uses those current frames. Each capture is accepted only after the live chapter and room ID match the manifest. The frozen Ice Room frame has no destination save, so its plan record restores the pre-dragon room 206 checkpoint and `capture-gallery.lua` replays North → Down through the Paths menu before capturing (run it alone with `galleryRefreshSet='ice-frozen'` and `refresh-sets.lua`). [route-gaps.md](route-gaps.md) lists transient/fatal rooms and reports checkpoint restore results.
