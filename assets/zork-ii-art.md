# Zork II scenery and item pass

The built-in ImageGen tool produced 81 backgrounds, including state variants, and 37 additional item, vehicle or encounter poses across the scenery and completion passes. The final PNGs are production sources. Backgrounds are 240 × 102; the cartridge shows source rows 10–85 as a 240 × 76 band. Sprite manifests retain transparent pose bounds. Packing, resampling and derived small support poses use `tools/import_art.py` and `tools/encounters.py`.

Apply [SCENERY_RULES.md](SCENERY_RULES.md) and the [screen checklist](../qa/screen-checklist.md) before further work. The bundled Release 63 story controls architecture, exits, scale and state. The [completion audit](../docs/audits/zork-ii-completion-audit.md) records the current coverage and resolved follow-ups; the [earlier re-audit](../docs/audits/zork-ii-re-audit.md) is historical evidence.

- [Background mappings, story text and preserved prompts](zork-ii-scenes.json)
- [Additional item prompts](zork-ii-item-prompts.json)
- [Completion prompts and references](zork-ii-completion-prompts.json)
- [Canonical sea-serpent identity](../docs/art-history/encounter-options/serpent-identity.md)

The background manifest replaces inaccurate draft composition notes. Some early generation prompts were not preserved; those entries say so and identify the final PNG as the layout reference. Do not treat a reconstructed recipe as a verbatim original prompt. Later final-edit prompts supersede rejected candidates.

The scene registry in `src/zork2_scenes.def` supplies room IDs and ground contacts. `src/chapter.c` selects puzzle variants from the original story's object parents, attributes and globals. `src/room_visuals.c` places movable items and characters separately. Taking, dropping, opening, destruction and restoring follow story state rather than baked-in inventory.

The swimming and dead sea serpent are one animal, not two designs. Preserve the canonical swimmer's short head, amber/olive crest, slim teal body and pale underside. The Topiary serpent is foliage. Guarding and friendly Cerberus share one identity, scale and connected anatomy; the collar remains a runtime overlay. The November 19 friendly pose palette restyle is recorded in [the Cerberus update](../docs/art-history/encounter-options/cerberus-friendly-palette.md).

The connecting oak door opens away from the Guarded Room and inward into the Workshop. From the Workshop it pivots at the right jamb, with hinge hardware concealed behind the frame and leaf. Both faces share plank construction, iron bands and weathering; the lizard inset is on the Guarded Room face. Each open view depicts its actual destination room. These latest user instructions supersede the temporary preference to preserve the earlier Workshop image.

The six Wizard's Quarters variants follow the actual printed description, without an extra LOOK or random draw. The last observed variant survives Save/Restore and launcher Continue. The Volcano View's ground contacts are restricted to its near eastern ledge; items must not appear to rest over the shaft.
