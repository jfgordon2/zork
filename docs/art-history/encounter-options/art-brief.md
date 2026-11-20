# Zork GBA art brief

For new scenery, apply the [scenery generation rules](../../../assets/SCENERY_RULES.md). For state poses, start from the current approved sprite in [the production manifest](../../../assets/encounter-options/imagegen-sprites/frames.json). Historical option prompts below are not alternate production identities. The [sea-serpent identity note](serpent-identity.md) documents why its swimming and dead poses must preserve one animal's anatomy and features.

## Room backgrounds

- **Frame the room at eye level.** Build the composition around a 240 × 76 visible band. The source backdrop is 240 × 102, and the game samples rows 10 through 85. Keep doors, stairs, and key landmarks in that band so they survive the crop.
- **Use large shapes for space and smaller pixel clusters for materials.** A doorway or riverbank should read before the chips in the stone. Group the details into patches of dampness, worn wood, or reflected light. Use calmer textures where a sprite or a menu will sit.
- **Pick a light direction and stick with it.** For an amber-lit dungeon, warm light can hit the upper and left-facing planes while blue or green carries the shadows. In the Land of Shadow, use cooler, more even light. Relative values should keep exits and silhouettes readable.
- **Use material color at the edges.** Let dark red separate a dragon’s scales or slate blue separate folds of a cloak. Save the darkest color for contact shadows and tight crevices. Use sharp pixel edges and clusters of a few related tones.

## Character sheets

Generate a transparent sheet for each encounter. Put all supported poses in separate cells in a single row, with empty space between silhouettes. Keep proportions, face, clothing, materials, lighting, and viewing angle consistent. Use the same scale for every pose when cropping the sheet to the sprite boxes. Line up the feet or the underside of a resting body on the same baseline.

For a pose change, start with the same anatomy and move the joints. The dragon's head looks toward the player in both poses. Its far legs can be partly hidden while it stands and tucked under the body while it lies down. Cerberus's three heads face the player and share one chest. The unicorn needs a horse's deep barrel and broad chest. The robot has a broad, straight-sided steel chassis, sturdy articulated limbs, and flat feet.

## Encounter rendering

Use the thief, princess, and sailor as the reference for pixel scale, edge treatment, and shading. Block in the silhouette and important joints first. Build each material out of a midtone, an upper-left light, and a cool far-side shadow. Add a fourth value only to show a specific feature, such as the robot’s visor or the unicorn’s horn. Shade edges with darker local color, like wine red on the dragon and blue slate on the robot.

| Subject | Shapes and details to keep at game size |
| --- | --- |
| Fur and feathers | Use a few grouped masses along the head, chest, and joints. Make an individual head readable from its brow, muzzle, and jaw. |
| Scales and wing membranes | Use continuous planes along the belly and wing joints. Place a few scale clusters where the body turns toward the light. |
| Steel and stone | Let broad planar bevels describe the material. Place highlights on facing edges and keep dark recesses small. |
| Humanoids | Use the human model for faces and hands. Keep the head and joint scale as the character changes pose. Preserve distinctive proportions for gnomes, trolls, and giants. |
| Spirits | Give each figure a distinct silhouette, limited blue-green values, and small dark face recesses. Use the ragged edge to suggest movement. |

Treat the creature’s anatomy, costume, props, and poses as design references. Apply the same light direction and scale of pixel clusters to the materials throughout the cast. Review the silhouettes first on one neutral background, then in their rooms.

## Human character model

Give every human the same drawing rules. Age, build, complexion, hairstyle, posture, and costume distinguish individuals. Keep the head and hand sizes, camera angle, light direction, and number of shading values consistent.

Use this shared prompt with the character's directions below.

> Draw 2D 16-bit adventure-game pixel sprites for the human cast of a first-person GBA dark-fantasy game. Follow one construction and rendering model for every person. The attached game room is a reference for amber key light from the upper left, cool slate shadows, and muted material colors. Export isolated full-body figures on genuine transparent RGBA.
>
> Use a 48 × 50 logical pixel cell for a typical standing adult. Allow around 44–47 pixels from sole to crown, about 6 heads tall with a roughly 7–8-pixel chin-to-crown head, ignoring hats and hair volume. Hands are about 3–4 pixels across. Use a front three-quarter view at eye level, body turned slightly toward screen right, face looking back toward the player. Keep feet on one baseline, fully visible, and avoid foreshortening. Characters may be slight or sturdy or have different features and complexions, while the head and hand scale and drawing technique stay consistent.
>
> Draw the face as a clear head silhouette with just three connected skin-tone groups: midtone, upper-left forehead and nose light, cool shadow along the far cheek and under the chin. Place a small, dark eye cluster on the near side and a single shaded far eye if visible. Put the nose in the boundary between those value planes and use a single short connected seam for a mouth only when it reads. No white eyeballs, stray cheek dots, crosshatch, or marks floating on the face. Group hair and beards into solid readable masses with a shade plane.
>
> Use a limited palette per material, typically three flat values with a fourth only for a small highlight. Long uninterrupted light and dark clusters should define folds and planes. Give hems and outer edges a darker version of the material's own color. Keep the same shadow depth, colored edge treatment, pixel grid, and level of detail for skin, wool and leather across figures. Use crisp pixel steps, no airbrush, no black contours, no gradients, no dithering, and no fine surface speckling. Age, clothing, posture, features, and one prominent prop can identify the individual. Retain visible hands when possible. Keep all boots on the same baseline.

When generating more characters, attach the human lineup as the reference for figure construction and pixel handling. Use room art to cue the palette and lighting. Keep the head, hands, and costume landmarks at the standing scale when drawing a shorter seated or kneeling frame. On a concealed face, let the hood carry the value shapes. Check every face and hand at native size, alongside the others, before reviewing them in their rooms. [Human style reference](imagegen-human-lineup.png) · [Study in the game rooms](imagegen-humans.png)

## References and review

The previews crop the generated alpha and fit the sprites to native game pixels. All poses for an encounter keep a common scale. The dragon uses 64 × 46 cells for both poses. The wider robot design was worked out in a separate proportion study. [Robot proportion study](imagegen-robot.png)

Review the fitted sprites against the room art. Check the eye line, count the limbs, verify that they join the shoulders and hips, and compare landmarks such as horns and the fall of a cloak across poses. Check the darkest rooms for enough contrast at game size.

## ImageGen prompts

The built-in ImageGen tool used the human lineup as the rendering reference and the earlier encounter sheets as design references. Apply the common prompt and the appropriate character direction. The transparent sprite sheets are fitted to their GBA cells after generation.

### Common prompt for all encounters

> Draw a new encounter sprite sheet for a first-person GBA adaptation of Zork. The FIRST attached image is the approved visual style: look at the large grouped planes, thin colored edges, and clean decisive features on those three figures. Use EXACTLY that pixel-art technique. The SECOND image is the subject design, which supplies the creature or person, props, palette family, and existing poses. Draw only the subject of the second image, completely isolated, applying exactly the pixel scale and rendering conventions of the first. Maintain the second image's identity, pose scale, number of poses, and recognizable geometry.
>
> One art direction across the cast: hand-clustered 16-bit pixel art. Build the silhouette from a few readable shapes and each material from three broad flat value groups, with a small fourth highlight only when useful. Warm muted amber on top and left; compact cool plum or slate shadow below and on the far side. Edges are darker local colors, with darkest pixels reserved for crevices and contact. One shared crisp square pixel pitch, with meaningful orthogonal and diagonal pixel steps even if delivered enlarged. No black outline, airbrushed gradients, noise, individual fur strands, dithering, or photorealistic detailing. A detail should still read when fitted to its logical sprite box. Keep all anatomy attached at real joints. Render a grounded eye-level view, with the playable character in front of the subject. Use genuine transparent alpha with ample outer padding and wide clear gutters. No labels, frames, cast shadows, floor art, terrain, scenery, or props beyond those specified. Keep every whole sprite inside its cell. In multi-pose sheets, maintain one body scale; each frame is separate, ordered left to right.

### Human face and proportions

> Match the human model precisely: roughly 6 heads tall when standing, a 7–8 pixel head at standard standing height and 3–4 pixel hands. Use three connected skin-tone planes for forehead/nose light, midtone, and far cheek/chin shadow. Use one compact dark near-eye cluster and just one far-eye shape if visible. No cheek speckles, heavy black faces, giant eyes, or skin outlines; model hair and beards in solid masses. Variation comes from age, build and costume.

### Gnome face

> Use the human lineup's face construction (three connected skin planes, compact dark eye cluster, solid shapes for hair and beard) and shared colors, with the subject's shorter adult proportions and distinct clothes.

### Zork I

#### Troll

> Exactly three frames, in 48×46, 48×46, and 56×26 cells: armed with a chipped iron axe, unarmed with open hands, then down on his side. Same muscular grey-olive troll, angular jaw and leather wrap. Use slightly warmer olive skin and readable midtone planes for contrast with the cool stone. Keep nose and brow consistent as his eyes close.

#### Thief

Drawn in the human lineup below.

#### Bat

> One 66×32 flying cave bat, dark smoky plum. Two broad folded-membrane shapes at the shoulders and one small attached body. Aim for two or three large warm and cool planes per wing, with no thin vein clutter or detached claws.

#### Cyclops

> Two frames. Left 48×52 awake standing broadly with a single unambiguous central amber eye; right 56×30 the same giant asleep on his side, eye closed and knees bent. Same head size, heavy brow, skin and tan leather wrap. Large flesh clusters and muted shapes with human-lineup face construction adapted to one eye.

#### Ghosts

> One 76×48 encounter cell with two distinct ragged dungeon spirits on a shared line, separated by a visible gap. Each is a clean cloak-like silhouette with two or three pale smoky blue-green value planes and ragged hems. Use a dark hood recess and tiny intentional eye shadows. Bodies have fully readable silhouettes, with no smoky aura, extra wisps, or unrelated floating fragments.

### Zork II

#### Wizard of Frobozz

> One 48×50 human wizard, silver beard, muted indigo pointed hat and blue-mauve robe, one empty hand raised and the other at rest. No staff or wand. Keep the hat and robe compact so the face and visible hands stay large enough to read.

Append the human face and proportions paragraph.

#### Unicorn

> One 68×50 ivory unicorn. Keep the full deep horse ribcage, muscular shoulder and hip, one horn and a plain gold neck ribbon. Four legs with near legs clearly grown from body and far legs tucked subtly behind. Three-quarter body, head turned to the player. Use broad cream, cool warm gray and muted slate value planes, with coherent clusters for mane and tail.

#### Baby sea serpent

> One 50×50 young teal sea serpent coiled, lifting an S-shaped neck and turning its head toward the player. Same eyes and restrained amber-green crest; continuous coil, no feet, water or splash. Broad teal planes and three decorative scale clusters maximum.

#### Red dragon

> Two 64×46 frames side by side, standing and fallen. Keep the attached dragon's two ivory horns, low rust-red quadrupedal body, attached leather wings, inward-curling tail, forehead and muzzle. Head faces the player in both poses. Standing has four naturally attached legs; fallen on the belly has the same near legs tucked and far legs naturally concealed by the body. Keep the same shape, scale and horn spacing; use a large copper highlight plane, muted crimson midtone and deep burgundy shadow. Group scales and membrane folds into a handful of large shapes.

#### Cerberus

> One 78×48 front view of Cerberus. Exactly three mastiff heads all looking toward the player. The heads descend through three rooted necks into one broad, low chest. One coherent dog body, four obviously attached or partly occluded legs, one tail. Broad chest fur and tawny muzzle clusters, clean eyes, and compact highlight groups which distinguish all three muzzles. Do not add face accessories.

#### Robot

> One 40×48 robot with a wide solid rectangular chest, short low collar, chunky square steel limbs on copper pivots, broad feet, flat head, narrow cyan visor, and two steel clamp hands. Keep the attached robot's broad shoulders and arm span, roughly 31 out of the cell's 40 pixels, and 47 pixels tall. Intact straight edges and angular bevels. Box-section legs and arms, blue gunmetal, warm copper. Use three matte steel planes and a tiny copper or cyan light. Treat the prior design's proportions and sturdy silhouette as locked.

#### Gnome of Zurich

> One 48×50 short adult gnome banker in a muted green-gray three-piece suit, obvious small spectacles and one closed compact steel deposit box. Bring the robe shadows and spectacles into the human lineup's consistent shapes, with clean face planes and expressive compact eyes.

Append the gnome face paragraph.

#### Princess

Drawn in the human lineup below.

#### Demon

> One 48×50 broad-shouldered humanoid demon with plum-gray skin, two stubby backward horns, one narrow curling tail, empty hands, no wings. Use the same proportions and face planes as the sturdier human in the reference, with unmistakably demonic head shape and muted spark-lit eyes.

Use the resting pose with one arm descending from each shoulder and the hands visible on opposite sides of the torso.

#### Volcano gnome

> One 48×50 short adult gnome in a dusty dark leather hood or cap, goggles, umber apron, and short boots. Readable goggle lenses and broad planes suggest warm reflected forge light over cool cave shadow.

Append the gnome face paragraph.

### Zork III

#### Dungeon Master

> One 48×50 elderly human Dungeon Master, ochre-brown hooded robe, one wooden staff, one plain book tucked under the arm, and a clean white beard. Maintain figure scale and eye-level viewpoint.

Append the human face and proportions paragraph.

#### Old man

> Two 48×36 seated human frames. Left awake, right asleep, wearing the same patched olive cloak. Same frail elderly man, clear head silhouette, same scale and length of gray beard. Keep the same large cloak and lap clusters and preserve the pose scale. On the right let the head slump and close eyes. Keep the 7–8 pixel head and 3–4 pixel visible hands of the standing cast even while seated, with compact connected skin-tone planes.

#### Hooded figure

> Two isolated human poses, 44×46 standing and 46×36 kneeling. The same figure, same muted slate cloak and face hidden by hood, same one narrow sword held down in both. Keep a 7–8 pixel implied head size, consistent hand size and costume folds between states; use navy recesses and blue-gray rim-light planes. The hood conceals all facial detail, no glowing eye marks.

#### Man at the cliff

> One 48×50 weathered human cliff climber in muted chestnut work clothes and a short leather cap, with a single rope coil slung over a shoulder. Rugged boots, slight asymmetry in stance and compact facial hair.

Append the human face and proportions paragraph.

#### Viking sailor

Drawn in the human lineup below.

#### Guardians of Zork

> Two matching carved stone guardians in 40×60 cells, mirrored inward. Each has a small attached plinth, worn angular ancient armor and one raised club. Keep silhouettes, human joint placement, plinth height, club scale and camera angle matched. Style stone with three gray-blue colored planes, sparse chips, no gradients and no metallic sheen.

The Zork III room contains the approved pair, baked into the two alcoves. The review board shows the unscaled source sprites beside the finished room. [Guardians background](../../../assets/guardians.png) · [Zork III comparison](imagegen-zork-iii.png)

### Human lineup

Use the human character model text above, followed by the directions below. The treasure room supplied the light and palette reference.

> Return exactly three adult figures in one horizontal lineup of equal-width cells, with generous empty gutters between silhouettes, matching a common body scale so one could swap them between rooms. Keep the figures familiar and fairly restrained. LEFT: the Thief, wiry with a deep forest-green cowl, dark green worn laced coat, one compact back satchel, and one narrow steel blade aimed down. Use a visible face under the front of the cowl. CENTER: the Princess, an adult with auburn hair, muted dusty-rose floor-length dress with simple layered sleeves, thin brass circlet, and shoes peeking out. No other jewelry or props. RIGHT: the Viking Sailor, a sturdy older man, worn muted blue wool with broad pale blue stripes, simple low blue cap, compact low gray beard and plain boots. No props. The three faces must actually look drawn at the same pixel scale by the same artist, with similarly sized features and identical shading language. One full body per cell, no text or names, no background, no ground or shadows outside the figures, no checkerboard, no guides. Render the chunky base pixels at a larger sharp scale if needed.

## Story-based encounter placements

Anchor the lowest visible contact point to its surface in the actual room crop. Roaming encounters use a floor point on the active background and face inward for a readable first-person view. The story takes priority for characters that look at another figure, stay on an upper ledge, hang from the ceiling, or move in water. The Gnome of Zurich and the volcano gnome use a shorter game-size sprite, and the sailor is scaled to the boat's deck.

### Additional ImageGen poses

### Ceiling-hanging bat

> Edit this approved Zork cave bat design into exactly one upside-down full-body pixel sprite. In the game the vampire bat hangs by its two hind claws from the ceiling in a corner, with mostly folded smoky-plum wings, and comically holds its nose at the smell of garlic. Body and head hang down from the claws, ears toward the floor, tucked wings still recognizable and joined to the shoulders. One small wing thumb near the snout is enough to convey the gesture, keep it anatomically coherent. Preserve the tan fur, smoky plum membrane colors, big ears, simple light/shadow clusters and same crisp 16-bit adventure game style. The final body will be about 20–26 pixels high at GBA resolution. Real transparent alpha, no ceiling or rock pixels, no text, one bat only.

### Dragon guarding the tunnel

> Edit the approved Zork GBA pixel art dragon sheet. Output exactly ONE new isolated full-body pose of that exact same rust-red dragon, alive and lying belly-down low on the floor with its folded legs close to its body, guarding a tunnel. Use the fallen pose's overall size and crouch as the structure. The eyes are OPEN, head held up a little at the end of the same thick neck and facing the player, watchful. Preserve the same two closely spaced ivory horns, folded copper bat wings, red scales, curl of tail, body design and proportions, warm upper-left lighting, dark berry shadows and chunky 16-bit pixel shading. The visible face at native size is important. Keep all anatomy connected and naturally foreshortened. Logical cell 64×46, body a low broad silhouette, roughly 30–33 px tall. Genuine transparent background, no ground, smoke, labels, or other poses.

### Princess on the rock

> Edit the CENTER character in the approved GBA human lineup into exactly one isolated seated sprite. This is Zork II's first view of the princess in the Dragon's Lair. She is a young adult woman who SITS UPRIGHT on a low rough angular rock in the corner, almost in a trance, oblivious to the player. Her body is seated in a modest natural pose with knees bent and boots or gown hem touching the floor, hands at rest on her knees, slightly bowed head with unfocused or lidded eyes. Same auburn hair, somewhat unkempt, thin brass circlet, and dusty rose floor-length gown with a dirt-darkened hem. Use the same restrained human face planes, chunky connected 16-bit detail, warm upper-left highlights, and the second reference's muted basalt colors for the distinct small dark rock she sits on. The small seat rock is part of the single sprite, flush down to the implied ground. Shallow three-quarter view toward screen left, front of her face a bit visible. Envision about 36×34 GBA native pixels in the complete figure including rock. Keep the final image transparent, no terrain beyond that attached rock seat, no floor, props, glow, wall, border, text, or second person.

### Man looking down from the cliff

> Draw one new isolated sprite of the rope-bearing brown-capped man in the first reference. Keep his costume, beard, satchel, shoulder rope, and chunky 16-bit Zork GBA art as the references. For this scene the player is on a shelf well BELOW him and he peers DOWN at them from a higher rocky ledge. Turn the man slightly to screen left toward the lower screen center. Show a clear bent neck and lowered nose, sloped cap angle, upper body pitched a little forward so the eye line visibly descends, with feet braced at uneven legs against the edge. Whole figure. The second image is the lighting and context reference only. Arrange a very distinct simple silhouette, as this becomes 15–18 pixels high in the actual game. Single full-body figure, genuine alpha, no scenery, ledge, text, or duplicated persons.

### Swimming serpent

> Edit the baby sea-serpent in the first reference into ONE clear full-body MID-SWIM pixel sprite for the second image's Zork II aquarium. Preserve the approved teal scales, olive crest, wary amber eye, chunky 16-bit shaded pixel technique and recognizable face. Change its pose completely: the entire slim body is suspended in the water in a long open S curve at a shallow diagonal, head high at the left and looking sideways toward the player, body rises and curves down, with a visible narrow tail streaming back to the right and slightly up. The curves are loose and open with transparent gaps, like an eel in motion. No stacked loops, grounded coil, floor contact or upright resting stance. Wide silhouette approximately 42×24 logical GBA pixels. Keep a connected slender body and tail, a simple face and dark teal plus cool upper-left reflected light. Transparent RGBA. No bubbles, water pixels, shadows, labels, scenery or extra figures.

### Friendly Cerberus

The guarding and collared poses depict the same `zork2.cerberus` entity. Derive the relaxed expression from `imagegen-sprites/cerberus.png`; keep the three heads, reddish fur, body, connected four legs and single tail. The collar is a separate state-driven overlay. See the [completion prompt records](../../../assets/zork-ii-completion-prompts.json) for the final edit and reference.

Fit the transparent sprite while preserving its source aspect ratio. Compare alpha-trimmed production sizes: guarding 59×55, friendly 58×56. Do not stretch an entire drawing to the guarding sheet's 91×56 pre-trim dimensions, which include transparent side padding.
