# Zork III mirror artwork and connected rooms

## Hallway 169 and the Inside Mirror entrance

Hallway 169 shares the dressed-stone walls, floor channel, and compass rose of plain
Hallway 113. Artwork follows Zork III global 107, which tracks the panel state. The
panel is not a Z-machine OPEN object, so its `ATTR_OPEN` flag is not reliable here:

- **Closed:** `z3_mirror_hall_closed.png` shows one large reflective mirror filling the
  north end wall. Its angled glass reflection suggests the side wall rather than a
  second corridor or the painted Inside Mirror room.
- **Open outward:** `z3_mirror_hall_open.png` shows the actual Inside Mirror 160 through
  the opening, not a reflection or invented hallway. The single hinged panel remains
  attached at the east jamb and folds flat against the east wall with its wooden back
  visible. It rests on the floor, leaves a clear route, and has no visible hinge hardware.

The opened state is verified in the route transcript: after the Beam Room button is
pressed, global 107 is set, room 169 says the mirror is mounted on a panel which has
been opened outward, and north then enters Inside Mirror 160. In that opening preserve
the blue ceiling, mahogany
left side, pine right side, and red/black far wall from `mirror_room_v2.png`. The corridor
threshold and stone channel continue into the room.

## Narrow Room variants

The story uses several room IDs with the same title but different mirror orientation and Guardian descriptions. Keep one camera and architecture across them; vary only the mirror side and the statues the text describes.

| Room ID | Mirror side | Guardian text | Scene |
| --- | --- | --- | --- |
| 48 | East | One statue and its reflection appear in perfect symmetry | `z3_narrow_mirror_guardians.png` |
| 51 | West | No statues; north hallway | Horizontally flipped `z3_narrow_mirror.png` |
| 58 | East | Two matching statues on pedestals, somewhat to the north | `z3_narrow_mirror_guardians_north.png` |
| 151 | West | One statue and its reflection appear in perfect symmetry | Horizontally flipped `z3_narrow_mirror_guardians.png` |
| 161 | West | Two matching statues on pedestals, somewhat to the north | Horizontally flipped `z3_narrow_mirror_guardians_north.png` |
| 178 | East | No statues; north hallway | `z3_narrow_mirror.png` |
| 119 | East | Opposite solid-rock wall; north and south hallways | `z3_narrow_mirror.png` |
| 221 | West | Opposite solid-rock wall; north and south hallways | Horizontally flipped `z3_narrow_mirror.png` |

## Acceptance rules

- The large framed mirror is on the side named by the room text. West-side scenes are exact horizontal flips of the corresponding east-side art.
- The wall opposite the mirror is continuous plain rock. It has no carved relief, mural, floor compass, opening, or invented symbol.
- In rooms 48 and 151, show one physical Guardian and its mirror image; do not draw two independent statues. In rooms 58 and 161, show the two described statues at matching depth on opposite sides of the northern corridor.
- Keep the corridor open and the statues on their described supports. The repeated Guardian design follows the approved statue sprite/background model in `guardians.png`.
- Focus anchors for the mirror, panel, and described Guardian group remain inside the scene crop and do not cover story text or the walkable center floor.

This set was revised after comparing the exact Release 25 descriptions with the native GBA crop. The earlier background had carved circles opposite the mirror and an unmentioned floor compass; both were removed. Catalog-only previews must issue `look` before taking the screenshot so the text and room artwork belong to the same room. These previews show scene matching; they do not establish that a room is reachable.
