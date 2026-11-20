# Background asset wiring audit

Reviewed November 19, 2026. This static wiring pass checks production background PNGs against the GBA scene declarations, room/state selection code, and floor-contact entries. It does not replay every room in mGBA.

## Removed orphan drafts

| Asset | Why it had no useful production home |
| --- | --- |
| `assets/z3_prison_cell.png` | Superseded by the paired `z3_prison_cell_closed.png` and `z3_prison_cell_open.png` views. It did not represent a separate room or state. |
| `assets/z3_room8.png` | Zork II room 207 and Zork III room 192 are the same Room 8. Both now use canonical `assets/z2_room8.png`, so a second copy could drift from the shared design. |
| `assets/z3_underwater_variant.png` | No distinct underwater state exists in the story. The updated `assets/z3_underwater.png` is the canonical room view and matches the connected lake and shore scenes. |
| `assets/mirror_room.png` | Superseded by `mirror_room_v2.png`, the active view into Inside Mirror. |
| `assets/parapet.png` | Generic unused draft; the Zork III Parapet uses its separate `z3_parapet.png` artwork. |
| `assets/gazebo.png` | Old oversized-table draft; Zork II room 16 uses `z2_gazebo.png`. |
| `assets/z3_mirror_hall.png`, `assets/z3_mirror_hall_v2.png` | Superseded by `z3_mirror_hall_v3.png`, which shows the moved mirror panel and the room beyond. |
| `assets/z3_guardian_hallway.png`, `assets/z3_guardian_hallway_v2.png` | Superseded by the active `z3_guardian_hallway_v3.png`. |

These seven sources are preserved under `docs/art-history/encounter-options/retired-backgrounds/` and are not build inputs.

## Wiring checked

- Every remaining root-level background has a current scene, story-state, or fallback selection path.
- Zork I backgrounds are registered in `src/zork1_scenes.def` and selected by the Zork I room mapper.
- Zork II backgrounds are registered in `src/zork2_scenes.def`. Entries with room ID zero are real visual states selected by explicit room and puzzle-state logic, not unreachable rooms.
- Zork III backgrounds are listed explicitly in `Makefile` and selected by room IDs or state branches in `src/chapter.c`; unlisted comparison drafts are not compiled.
- Unused `SCENE` declarations, floor-contact rows, and host-test stubs for the old mirror/parapet/Gazebo and Z3 hallway/mirror variants have been removed. `z3_water_slide` remains active for room 163.
- Dead name-based fallbacks for Zork II Gazebo, Formal Garden, North End of Garden, Topiary, Deep Ford, and Dragon's Lair were removed because their numeric scene entries always select first. The distinct Dragon Room fallback remains.
- `hall.png` and other shared non-prefixed scenes remain in use as mapped scenes or the final general-scene fallback; they were not treated as orphans just because their names are generic.
- Four assembled scene symbols intentionally have no standalone PNG: west-facing views are built by horizontally mirroring their paired source artwork. `z3_west_corridor` derives from `z3_east_corridor`; the three west-oriented narrow-mirror states derive from their east-facing counterparts.

## Shared locations retained

| Story location | Shared mapping |
| --- | --- |
| Zork I room 8 and Zork III room 15, Timber Room | Both select `art_z3_timber_room`. |
| Zork II room 207 and Zork III room 192, Room 8 | Both select `art_z2_room8`. |

The different room numbers and chapter names are intentional story mappings, not duplicate background assets. Conversely, similarly named but distinct rooms such as the Zork I and Zork II Machine Rooms keep separate artwork.

## Maintenance rule

Before adding a background, wire it to a room or actual story state and add any needed floor-contact data. Add Zork III production sources to the explicit Makefile list; do not place comparison drafts in the root background directory. Before retiring one, check scene declarations, room selection, state branches, floor placements, tests, and audit notes. Keep retired comparisons in `docs/art-history/encounter-options/retired-backgrounds/`.
