# Portable item art and placement coverage

**Audit date:** November 19, 2026  
**Story source:** the three bundled `stories/zork*.z3` releases.

The portable-object inventory was read from each live Z-machine object table using the story's `PORTABLE` attribute, then compared by object ID with the `RoomItem` art table in `src/room_visuals.c`.

| Game | Portable objects | Objects with art poses | Unmatched |
| --- | ---: | ---: | ---: |
| Zork I | 60 | 60 | 0 |
| Zork II | 51 | 51 | 0 |
| Zork III | 24 | 24 | 0 |

The same check found no duplicate chapter/object mappings. Some objects share appropriate physical art, including water quantities, books, worn lanterns, and related wood pieces; stateful objects use the story's open, lit, or broken state.

## New or newly bound art

- The Zork I item set now includes the jade figurine, lunch parcel, ancient map, owner's manual, coal, clockwork canaries in both conditions, gallery painting, emerald, scarab, brass bauble, viscous material, vitreous slag, water, diamond, platinum bar, garlic, stiletto, and the boat label.
- Zork II's plain and salty water quantities use the same low water silhouette; the story text distinguishes them. Existing matching poses cover all other portable objects.
- Zork III now has poses for the valuables pile, broken staff, broken lantern, water, burned torch, and bronze oil lamp.
- The November 19 sprite review added dedicated poses for the Zork I nasty knife, Zork II blue label, Zork III jewelled knife, and Zork III strange key. The burned-out Zork I lantern now uses the intact unlit pose; its skeleton key uses the existing iron key pose.
- Newly generated props are in `new_props_source.png` and the native-size `new_props.png` strip. Coal, lunch, broken staff, burned torch, and oil lamp use small hand-built pixel sprites that keep the same warm-light palette.

## Scene placement rules and special cases

Loose room items use per-scene floor contact points and avoid occupied sprites where a free spot exists. If the floor is crowded, the renderer draws a compact, focus-prioritized pile; it does not silently omit the selected item. Automatic placements stay within the visible scene bounds (`x=2..237`, `y=17..89`), below the top overlays and above the bottom command/UI area.

Initial items that belong on a support, wall, in a container, or on an actor have explicit handling:

- **Zork I:** the map is anchored inside the trophy case when the story exposes it; the painting is on the Gallery wall; the opened nest egg reveals its canary; the opened sack reveals lunch and garlic; opening the buoy or tube reveals its contents; the bottle shows its water as a filled state; the coffin, boat, and mailbox retain their existing contents views. The troll's axe and the thief's stiletto are already held in their encounter art.
- **Zork II:** the rusty key sits in the Dreary Room keyhole when the cover exposes it; the gold key sits at the unicorn's mane; the Wizard's wand is shown at his hand; the banker-held safety deposit box is placed beside the banker. Existing book, chest, steel-box, stand, table, and vehicle content views remain attached to their supports.
- **Zork III:** jewels stay inside the cage at the pedestal; the sword and rock keep their embedded/removal state; the valuables sit beside the man on the Cliff; and the warning note is mounted on the Royal Puzzle entrance wall. The hood and cloak remain part of the hooded figure's silhouette.

When an item is carried or dropped, its room pose is selected by chapter and object ID. The normal story rules still determine whether a container is open, an object is visible, or an item has moved into another object.

## Resolved visual distinctions (November 19)

| Story object | Current pose | Visual rationale |
| --- | --- | --- |
| Zork I #91 burned-out lantern | `LanternOff` | Intact lantern with no flame; distinct from cracked broken lanterns. |
| Zork I #96 skeleton key | `RustyKeyIron` | A dark iron key rather than the delicate gold key. |
| Zork I #165 nasty knife | `KnifeNastyFloor` | Dark steel with a nicked edge and worn grip, without orange rust. |
| Zork I #232 rusty knife | `KnifeOld` | Retains the existing worn, rusty-looking blade. |
| Zork II #127 delicate gold key | `KeyGold` | Preserves the gold key's fine bow and warm metal. |
| Zork II #138 blue label | `BlueLabelFloor` | Blue paper tag rather than a tan folded leaflet. |
| Zork III #13 strange key | `StrangeKeyOld` | Aged, asymmetrical key distinct from the gold and iron keys. |
| Zork III #174 jewelled knife | `KnifeJewelledFloor` | Gold fittings and visible ruby accents identify it as a Crown Jewel. |

## Review boundary

This audit covers every portable object in the bundled releases and the support/container/actor placements represented in the renderer. Representative current-code captures checked the nest, kitchen, bank room, Cliff, keyhole room, and Royal Puzzle entrance. It does not enumerate every possible player-created combination of dropped items; crowded drops use the same floor-placement and compact-pile path.
