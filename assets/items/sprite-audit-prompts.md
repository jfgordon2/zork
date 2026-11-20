# Item sprite audit prompts

Generated with the built-in ImageGen tool on November 19, 2026. Each result was cropped
to its visible alpha bounds before resizing into the native frame cell, preserving the
object's proportions. The build then applies the usual alpha trim and palette conversion
in `tools/encounters.py`.

## Jeweled knife · `knife_jewelled.png`

> Use case: illustration-story. Asset type: one isolated in-game item sprite for the Zork trilogy GBA adventure. Create a compact jeweled knife that reads as one of the Crown Jewels: a restrained but unmistakably precious short knife, pale steel blade pointing left, slim gold guard, dark burgundy grip, one clear ruby set in the pommel and tiny jewel accents along the hilt. Slight downward three-quarter view, laid nearly horizontal on its side as if resting on a surface. Match detailed late-16-bit pixel art: deliberate clustered pixels, clear material planes, warm upper-left highlights, cool muted shadows, colored edges, very little black linework. Keep the knife silhouette broad and readable at roughly 24 by 9 native pixels; the subject should fill most of the image. Real transparent alpha. One object only. No blood, hands, floor, cast shadow, backdrop, text, UI, border, extra blades, or decorative aura.

## Blue label · `blue_label.png`

> Use case: illustration-story. Asset type: one isolated small portable item sprite for a late-16-bit pixel-art Zork GBA adventure. Draw one BLUE PAPER LABEL, a small rectangular paper tag that can be picked up from a hot-air balloon basket. Deep muted cobalt-blue paper, one folded pale blue corner, a tiny punched hole near one edge, subtle crease and a few tiny dark ink strokes that are not readable words. Lay it almost flat in a shallow downward three-quarter view, with a slightly uneven cut edge so it reads as a physical label rather than a folded leaflet. Deliberate chunky pixel clusters, warm upper-left highlight, cool slate shadows, colored edge pixels, very restrained black outline. Keep the silhouette compact and readable at about 14 by 8 native pixels; fill most of the image. True transparent alpha. One object only. No tan paper, no full leaflet, no string, no hands, no backdrop, no floor, no cast shadow, no text, no UI, no border.

## Nasty knife · `knife_nasty.png`

> Use case: illustration-story. Asset type: one isolated Zork I portable knife sprite. Draw one short nasty-looking old knife lying almost horizontal, silver-blue steel blade pointing left, dark worn brown wrapped grip to the right, small plain guard, a subtly nicked edge and a blackened blunt spine so it looks menacing and used. It is NOT rusty: no orange rust, no blood, no jewels. Slight downward three-quarter view, flat resting pose. Match detailed late-16-bit adventure pixel art with deliberate chunky clusters, warm upper-left highlights, cool muted shadows, colored metal edges, very little black outline. Readable at about 22 by 8 native pixels; fill most of the wide image. Real transparent alpha. One knife only. No hand, sheath, floor, projected shadow, glow, background, text, UI, or border.

## Strange key · `strange_key.png`

> Use case: illustration-story. Asset type: one isolated small Zork III portable item sprite. Draw one visibly STRANGE old key, distinct from an ordinary delicate gold key: dark aged bronze and pewter metal, a narrow shaft with an asymmetrical forked bit, and a small off-center crescent-shaped bow with one broken-looking notch. It should look functional but uncanny, not jeweled or ornate, laid diagonally almost flat in a shallow downward three-quarter view. Match detailed late-16-bit adventure pixel art: deliberate clusters, warm upper-left glints, cool slate shadows, colored edges, very little black linework. Keep a clear small silhouette readable at roughly 14 by 10 native pixels, filling most of the image. Real transparent alpha. One key only. No ring of keys, no lock, hand, floor, cast shadow, glow, text, UI, border, or backdrop.

## Compiled dimensions

| Sprite | Trimmed in-game size |
| --- | ---: |
| `KnifeJewelledFloor` | 23 × 7 px |
| `BlueLabelFloor` | 16 × 9 px |
| `KnifeNastyFloor` | 23 × 5 px |
| `StrangeKeyOld` | 15 × 10 px |
