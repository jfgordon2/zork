# Scenery generation and acceptance rules

Apply these rules to new backgrounds, edits, connected-room sets, and state variants in all three chapters. They capture the established style and the architecture failures found during the Zork II scenery pass. Use them before prompting and before accepting an asset.

## Story and route come first

- Record chapter, room ID, original description, entry route, exits, and relevant puzzle state. The bundled story is authoritative.
- Distinguish normal passages, human crawlways, barred windows, tiny puzzle openings, high cracks, and blocked exits. Their silhouettes must communicate their different uses.
- Show the entrances and obstructions described by the story. Do not invent extra doors, platforms, stairs, bridges, furnishings, or collectible objects as decoration.
- Keep underground places underground. A cavern garden retains its hedges and moss-lit roof; a stream beyond a dungeon arch must not become an outdoor forest.
- Follow the current state: cooled rock in a dormant volcano, ice blocking a passage until melted, and only the faint cues specified by the story in a dark crypt.

## Continuous access and structural support

- Trace an uninterrupted surface from the viewer's standing position to and through every visible traversable opening. Show the floor continuing a short distance inside it.
- A threshold meets the floor plane **at that doorway's depth**. Perspective can put a distant threshold higher on screen; it cannot leave a strip of vertical wall between its approach and its opening.
- Reject floating doors, paths ending against a wall beneath a doorway, detached landings, unexplained drops, and bridge decks disconnected from their banks.
- Ordinary thresholds are flush or visibly easy to step over. Where the story requires elevation, show its actual steps, slope, ladder, or other access. Do not invent stairs to conceal an incorrectly placed door.
- Bridges need supported, continuous decks joining both landings. Ledges need believable width, thickness, and contact with rock. Water beneath a bridge reads as beneath its deck.
- Keep a path's connection readable. Rocks or vegetation may obscure it only when the story describes that obstruction.

## Human scale and passage clearance

- Compare openings, stairs, rails, furniture, and characters at the same perspective depth. Normal entrances accommodate an adult's shoulders and head with plausible clearance.
- A human crawlway fits an adult in the posture required by the story. It may be low and tight; it must not read as a mouse hole, drain, or pet entrance.
- Do not arbitrarily shrink distant doors relative to neighboring masonry. Perspective must explain the scale consistently.
- Preserve intentional puzzle exceptions. The Tea Room's four-inch hole is inaccessible at normal size; the Posts Room shows the same table at the shrunken player's scale. Do not enlarge those openings into ordinary entrances.
- A high crack stays a narrow irregular crack. Do not frame it as a raised human-sized arch that falsely promises a walking route.

## Perspective, travel, and state variants

- Choose the camera's eye level and floor plane before decorative detail. Floor seams, thresholds, supports, and wall bases agree with them.
- Adjacent views share materials, landmarks, water depth, light, and elevation. The reverse side of a door is the same door; both ends of a shaft belong to the same structure.
- Treat a connected feature as one designed object across every room view. Before accepting the images, compare the same ladder, stair, bridge, tunnel, or doorway from both ends: material, construction, width, rung/step spacing, supports, orientation, and contact with each room's floor or ceiling must agree. Apparent position and scale may change with the camera, but the geometry must still map to the same physical opening. Record the paired room IDs and travel direction with the assets.
- Treat connected windows as one physical opening in both rooms and all states. A sliding sash moves vertically in both views; keep its pane count, frame, and opening height consistent. Do not turn a slider into a hinged leaf or mirror its motion horizontally. From outside, show only the interior details visible through the aperture at that height; do not expose an unsupported floor plane or unrelated hearth.
- For ladders between rooms, show the same rail thickness, rung count/spacing rhythm, wood tone, and opening shape from above and below. The upper view must make the ladder descend through a floor opening; the lower view must make it rise through the corresponding ceiling opening. The ladder should meet the opening cleanly without floating or passing through a wall.
- Different rooms have their described geometry. Sharing materials is appropriate; replacing unrelated puzzles with one generic hall is insufficient.
- Do not assign the generic `cavern` image to distinct caves, mines, ledges, chasms, or passages when their descriptions distinguish the geometry or material. Reuse is appropriate only when the story establishes the rooms as alike (for example, the twisty maze passages), or when connected views are deliberately designed as one consistent scene set. Give every new background a room mapping and floor-contact anchors.
- State variants retain the camera and unchanged architecture. Opening a door, melting ice, or breaking a safe must not move unrelated entrances or furniture.
- Choose each door's hinge side and swing direction before making its open variant. Show the leaf attached to that hinge, with consistent foreshortening and floor clearance. The reverse room view must show the same physical swing: away from one room means into the other. Keep the walking opening clear, and move or foreshorten embedded details with the panel. The Guarded Room's door opens away into the Workshop.
- The Workshop door pivots at the RIGHT jamb and swings inward into the Workshop. Its hinge hardware is concealed behind the frame and leaf. Match the same battered oak planks, proportions, iron bands and weathering from both sides; the lizard is on the Guarded Room face. This supersedes the earlier temporary left-hinge preference.
- Through an open doorway, depict the actual destination room from the reverse approach: its materials, lighting, floor and recognizable architecture. Do not substitute a generic repeated corridor. The Workshop looks into the dusty cobwebbed Guarded Room; the Guarded Room looks into the warmer arched Workshop hall.
- Vehicle views explain the player's position. Airborne balloon views show suspension over a shaft and the basket appropriately, rather than an ordinary walkable foreground cave floor.

## Item supports and character contact

- Identify the actual floor, table, shelf, ledge, water volume, or container supporting each overlay. Its contact point and perspective must match that surface.
- Floor anchors belong on the floor. A foreground table must not consume every ground-placement position, as happened with the dropped gazebo teapot.
- Keep removable items separate from the background. Taking removes them; dropping uses their new parent and support rather than their original display position.
- Keep moving characters and changing puzzle objects separate where practical. Feet, scale, gaze, and activity follow the current story. Swimming creatures belong in the water.
- Preserve entity identity across states. First check the story to determine whether two appearances are the same entity, then derive new poses from its canonical approved sprite. Keep head shape, body proportions, anatomy, crest, markings, and materials consistent; a similar color scheme is insufficient. Compare poses at equivalent scale before placing them at different scene depths.
- Preserve the source aspect ratio when fitting a sprite to its native bounds. Compare the visible alpha-trimmed production silhouettes, not only the sheet dimensions: transparent padding must not make a state change stretch or shrink the character.
- Reserve space for multiple drops and the largest expected encounter. Check overlaps and selection corners at native size.
- Represent meaningful changes such as attached collars, emptied supports, open containers, damaged structures, and rescued traps, including after returning or restoring.

## Pixel style and actual display

- Match approved room and encounter references: detailed late 16-bit clustered pixels, restrained colored edges, limited black outlines, readable materials, and subdued atmospheric color.
- Favor structural silhouettes over noisy texture, blur, elaborate ornament, or ambiguous black recesses.
- Backgrounds are **240 × 102 pixels**. The game displays source rows **10–85 inclusive** as a **240 × 76** crop at screen rows **15–90 inclusive**. Within that crop, source `(x, y)` appears at screen `(x, y + 5)`.
- Keep important entrances, thresholds, approaches, obstacles, supports, and character contacts inside the visible band. Inspect the reduced asset and in-game crop, not only the large generated source.
- Check the full 240 × 160 screen with focus, menus, target pickers, and story pagination. Required visual cues and controls must remain visible and accessible.

## Asset wiring and shared locations

- Every production background must have a route to the screen: a scene symbol in `src/scenes.S`, a room or state mapping in `src/chapter.c` or the Zork I/II scene registries, and any needed floor-contact entry in `src/room_visuals.c`.
- Before keeping a new PNG, confirm the game selects it for a reachable room or a real story state. Build inclusion alone does not make an image usable.
- Do not retain alternate drafts as production art when the story has no corresponding state. Keep comparison material under a clearly non-production location, rather than beside bundled backgrounds. `assets/encounter-options/imagegen-sprites/` is the intentional exception: it is the active, Makefile-wired character sprite atlas. Store alternate character studies outside that production subfolder, such as in `docs/art-history/encounter-options/` itself or `retired-sprites/`; reserve `retired-backgrounds/` for scene drafts.
- When a room is intentionally identical across chapters, point both room mappings at one canonical background. Record the shared room IDs and do not keep stale duplicate copies.
- Generated mirror views should be declared as derived scenes and name their source image and transform in the Makefile. Do not add a redundant PNG for a pure horizontal mirror.
- When removing an image, remove stale symbol declarations and test stubs, update audit notes, and check that no production mapping or document still points at it.

## Prompt requirements

Describe concrete geometry instead of relying on “coherent.” Include the applicable fields:

```text
Asset: chapter, room ID, state; 240×102 with source rows 10–85 visible
Story: original description, exits, and obstructions
References: identify style, connected-room reference, and edit target
Camera: eye level, viewing direction, floor plane
Access: trace the foreground surface to and through each traversable opening
Elevation: same-level thresholds; only story-supported height changes
Clearance: adult doorway / human crawlway / intentional tiny puzzle opening
Supports: real floor and furniture reserved for separate props and characters
State: changes to show; architecture and camera to preserve
Avoid: extra entrances, floating thresholds, animal-sized human routes,
       unsupported platforms, baked-in collectibles, UI, captions, borders
```

## Acceptance checks before expanding generation

Review one scene or a small connected set, correct problems, then expand. These are agent review gates, not requests for user permission.

| Check | Required evidence |
| --- | --- |
| Story and exit inventory | Match each entrance, crack, obstacle, and major structure to the description. |
| Floor trace | Trace the full approach into each traversable opening; no wall strip or unexplained gap below its sill. |
| Human clearance | Compare at the same depth and confirm the required standing or crawling posture fits. |
| Elevation and support | Explain every height change and structural support through story and visible geometry. |
| Connected views | Compare neighboring rooms, reverse approaches, and state variants. |
| Entity identity | Compare each pose to the canonical approved entity, including head/body proportions and distinctive features. Confirm the story supports any actual transformation. |
| Native crop | Inspect the actual 240×76 view, rather than only the larger source. |
| Overlay contact | Render actual items and characters on supports, including floor drops. |
| UI and actions | Exercise focus, menus, targets, and relevant successful and rejected actions. |
| Persistence | Check leave/return and Save/Restore for changing content; Continue where relevant. |

Record **Pass**, **Issue**, **Unchecked**, or **N/A** with evidence in the scene's QA record. Missing evidence is not Pass. Do not integrate known architecture failures. Use contact sheets for comparison and individual scene views whenever access or scale is ambiguous.

## Specific corrections to preserve

- **Cobwebby Corridor:** walking passages meet the dusty floor; the high opening is a narrow crack, not a raised arched doorway.
- **Path Near Stream:** the tunnel and adult crawlway each connect to the ledge path. The crawlway is human-sized; the stream stays far below.
- **Marble Hall:** the marble approach continues through the northern arch to a small underground ford, without an elevated cave lip or forest beyond it.
- **Gazebo:** a modest tea table leaves visible plank floor for an item dropped onto the ground.
