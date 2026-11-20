# Instructions for game artwork

Before generating or editing scenery, read [SCENERY_RULES.md](SCENERY_RULES.md) and the [screen QA checklist](../qa-screen-checklist.md). Apply them to prompts, native-size artwork, and in-game placement.

The story program's current room description and state are authoritative. Check every visible entrance for continuous access, human clearance, and correct elevation. A high narrow crack must not become an apparent doorway.

For encounter state changes, identify the story entity and its canonical approved sprite before generating. Preserve its anatomy, head silhouette, proportions, crest, markings, and materials across poses. Matching the palette alone is insufficient. In Zork II, the swimming baby sea serpent and the dead sea serpent are the same animal; use `encounter-options/imagegen-sprites/serpent.png` as their identity reference.

Review a small connected set before expanding generation. Do not launch a large unreviewed scenery batch. Correct failed architecture before integration. Inspect the actual 240 × 76 game crop with sprites and UI, rather than accepting a large generated image alone.

Keep collectible items and changing encounter objects separate from the background. Verify original placement, take/drop, leave/return, and Save/Restore where applicable. Document prompts, references, room/state mappings, and review results. These are agent implementation and review requirements; they do not require a new user approval step.
