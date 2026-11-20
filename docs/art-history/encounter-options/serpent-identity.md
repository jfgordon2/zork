# Zork II sea-serpent identity

The aquarium's **baby sea serpent** (story object 32) and the **dead sea serpent** (object 17) are the same animal. In the bundled Release 63 story, breaking the aquarium spills the serpent out with salt water and sand; it bites at the sword and player, struggles to breathe, slithers toward the player, and dies. No transformation or growth is described. Examining either state adds no appearance details.

The attack and death resolve in one command. The resulting playable art needs the swimming state and the dead floor state, rather than a separately designed land creature.

The canonical approved design is [the swimming sprite](../../../assets/encounter-options/imagegen-sprites/serpent.png). Both sprite manifests carry the shared entity ID `zork2.aquarium_serpent` and reference that identity source. The corpse is [the floor pose](../../../assets/items/dead_serpent.png).

[Side-by-side pose comparison](serpent-states.png) uses the same 6× nearest-neighbor display scale for both native sprites.

Preserve these features in every additional pose:

- Small, short-headed eel-like profile, with the same muzzle and head-to-body ratio.
- Slim continuous teal neck, body, and tapering tail; no thickened torso or enlarged crocodile jaw.
- Restrained olive/amber crest at the head, cool pale underside, and the same teal scale planes.
- Consistent colored edges and pixel clusters. Pose, expression, lighting, and scene depth may change; anatomy and distinctive markings stay recognizable.
- The living pose swims in an open S curve. The dead pose lies flat with its eye closed. Neither requires legs, wings, new horns, or a different dorsal fin.

Compare poses at equivalent creature scale before acceptance, then check the swimmer inside the intact tank and the corpse on the wet floor. Perspective can explain apparent scene size; it cannot explain changed anatomy.

The Topiary's great serpent is a separately described **hedge sculpture**. It remains visibly foliage and can echo the silhouette; it is not another living aquarium serpent.

Older coiled concept studies are historical alternatives. They are superseded by the approved swimming sprite for production identity and aquatic posture.
