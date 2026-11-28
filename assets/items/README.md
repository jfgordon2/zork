# Room item sprites

The cartridge loads transparent, native-pixel poses from `frames.json`. Use the chapter’s room and examination text as reference. Keep the encounter art’s warm light, colored shadows, and restrained outlines. Give each pose a clear silhouette at 240 × 160.

The current portable-item coverage and placement rules are recorded in [ITEM_COVERAGE.md](ITEM_COVERAGE.md). Every portable object in the three bundled story releases has a room-art pose; related forms share a silhouette only where that matches the story.

Room placements use contact points on the visible floor, tabletop, or support. Closed containers conceal their contents through the story’s visibility rules. Portable props can reappear on the ground after they are dropped. The source PNGs are sprite strips with transparent gutters.

The [Zork II pass](../zork-ii-art.md) adds 35 item and vehicle poses, including the bucket, books, floor serpent, lizard head, balloon, fuse, brick, crown, collar and damaged cage. [Its prompt records](../zork-ii-item-prompts.json) and the [serpent identity reference](../../docs/art-history/encounter-options/serpent-identity.md) take precedence over older studies for these production assets.

The sheets below were generated with the built-in ImageGen tool. The packed sprites were trimmed, scaled, and checked at cartridge resolution. The cake icing, stand accents, and crystal colors use palette variants of shared silhouettes.

The November 19 sprite review added distinct sprites for Zork I's nasty knife, Zork II's blue label, Zork III's jewelled knife, and Zork III's strange key. The existing intact unlit lantern and iron key are reused for the burned-out lantern and skeleton key. See [the review prompts](sprite-audit-prompts.md) and [coverage notes](ITEM_COVERAGE.md) for the story-to-sprite bindings.

`pile_bodies.png` is a reusable, non-graphic sprite for the same remains described at Entrance to Hades and in the Land of the Dead. The renderer uses a smaller derived pose through the gate and the full pose in the closer room.

`new_props_source.png` is the source atlas for the small Zork I treasure and household props. `new_props.png` is its 32-pixel-high transparent production strip, indexed by the `new_props` cells in `frames.json`.

## Shared prompt

Make exactly one horizontal sprite sheet for the first-person GBA Zork trilogy. Treat the attached art as the pixel style reference. Sharp 16-bit adventure-game pixels, deliberately grouped planes, warm light from upper left and muted slate/plum shadows, colored material edges, minimal small highlights. Eye level camera with a slight downward look at the objects, 3/4 perspective. Bottoms of the objects sit on a flat surface when composited later. Fully isolate the objects, no projected shadows, floor, hands, lettering, frames, labels or UI, real transparent RGBA. Render exactly the objects listed, once each, in LEFT-TO-RIGHT order in a single horizontal row. Use clearly separated equal-width cells with broad fully transparent gutters. Use a 20–40 pixel logical scale per piece with crisp deliberate pixels even if output is delivered enlarged.

### Lantern and tools

Style reference `docs/art-history/encounter-options/imagegen-human-lineup.png`

1. unlit compact battery-powered brass lantern, thick handle loop over glass cylinder, dark glass, base plated metal
2. the exact same lantern in the same stance and size, glowing warm amber inside its chamber, glow held inside glass
3. one slender elvish steel sword with decorative leaf-shaped crossguard, an old grip, laid almost sideways diagonally across a supporting plane, blade points right from a horizontal-ish silhouette
4. one rough old axe with bloody steel crescent head, worn brown haft, resting diagonally low on a surface
5. one short red-brown folded paper matchbook with worn cream top stripe and a thin striking strip, flat, compact

### Treasures

Style reference `assets/treasure_room.png`

1. the solid gold Egyptian royal coffin of Ramses II, lifesize sarcophagus, laid flat at shallow three-quarter perspective so it reads as low and horizontal, dignified old gold pharaoh-shaped lid and indigo stripes
2. the identical low gold sarcophagus exact same orientation and scale with the lid open off to rear side, visible dark inner cavity
3. one ancient SILVER chalice with fine engraved rings around the silver bowl, short bright stem, flat foot, readable highlights
4. one near-horizontal Poseidon crystal trident with three separate blue-white clear crystalline prongs and long tapered shaft
5. one low dark old heavy pot overflowing with distinct gold coins, flat base, no glitter outside its shape

### Tabletop items

Style reference `assets/gazebo.png`

1. one white glazed china teapot with a muted blue floral band, low round belly, lid, curved handle and short spout
2. one folded discolored broadsheet newspaper lying flat, gray-beige folded corner and charcoal ink columns, illegible
3. one single off-cream leaflet lying flat, uneven fold across its middle and several restrained charcoal ink lines
4. one whole tiny round tea cake sitting flat, cream frosting and icing marks on top in green tint, one visible layer of golden cake with no plate, icing marks miniature and simple; this base gets recolored into four puzzle variants
5. one small irregular piece of tan waybread with a lightly browned crust and a pale flour crumb, resting flat

### Workshop implements

Style reference `assets/workroom.png`

1. one round clear glass-crystal sphere sitting on a surface, dark silver transparent volume, minimal compact highlights; base to make red blue clear and black variants
2. one short heavy bare receptacle stand in dark antiqued metal and brass, simple circular cup at top to nest a ball and squat foot
3. one differently shaped black glassy obsidian stand with angular cup, squat and faceted, no contents
4. one crooked old slender wooden magic wand with brass and pale tip, resting almost horizontal on stone, no aura
5. one single delicate old gold key with small filigree bow, thin shaft and single clearly shaped bit, diagonal and resting flat

### Cliff and museum

Style reference `assets/cliff_ledge.png`

1. one large worn dark plank chest with a low arched lid, iron corner bands and large central metal lock. closed
2. the exact same chest in matching three-quarter view and same size, lid raised revealing the dark hollow, same corner bands and clasp
3. one blue slate hooded cloak discarded in a low heap of thick fabric, softly folded, hood opening visible in the pile, natural gravity
4. one tiny upright glass vial with narrow neck and small stopper, containing a few colorless drops and using blue-white glass edge highlights
5. one large shiny golden antique time machine made for a human to sit in, visibly an open single seat and low console in front with single large button and one dial and a small three-aperture display, clean old brass and gold panels, readable three-quarter profile; no cube casing, no wheels, no jets

### Mailbox

Style reference `assets/west.png`

1. one small rural grey-green metal mailbox mounted on a single weathered wooden post, door SHUT, simple matching top curve and red latch; keep the whole post visible down to the foot
2. the exact same posted mailbox with front door HINGED OPEN dropping forward, the inside visible and empty, same box size same post and same viewpoint. The inside must look dark and open

### Curios

Style reference `assets/treasure_room.png`

1. a small jewel-encrusted egg in delicate gold filigree with jewel facets visible in the shell, upright
2. one clear icy carved crystal skull with readable eye sockets and grin
3. one small wrought gold dragon statuette, elegant curling tail and folded wings, sits on own low gold base
4. one sturdy tarnished brass handbell with a small wooden handle at the top
5. one flat carefully stacked pile of old dull-valued pale brown paper currency notes with subtle successive edges and decorative printed borders, no readable numbers

## Fixtures and long items

Produce exactly one row of FIVE separate transparent game sprites in the left-to-right order below. Use attached weathered masonry as a style and light reference for chunky 16-bit GBA pixel art. One item each, broad empty gutters, crisp colored edges, ambient warm upper-left highlights and cool shadows, no projected shadows, no scenery, page fill, text or game UI.
1. Large dull bronze-gray HEXAGONAL steel bolt head with a washer under it, seen at a slight angle, short physically mounted profile for a dam's control panel. Use a clean metallic bevel and dark middle.
2. A tall massive upright menhir, ancient gray-green roughly hewn upright granite slab, roughly rectangular broad face, pores in rock in hand-clustered pixels, a shallow blank indent in the middle, darker side bevel and stable bottom, no lettering.
3. A separate low horizontally fallen rendition of that exact same megalith, same stone, a long slate face and angular edges, resting on its long side.
4. One ordinary slender worn hardwood walking STAFF, clean dark wood grain with two primitive color planes, blunt rough ends, laid nearly sideways low on a surface.
5. One handheld antique wooden torch with dark wrapped head and a compact warm orange flame directly at the head, no glow outside the flame, a diagonal shaft suitable for lying flat.
Actual transparent background.

## Household items and the embedded sword

Standalone transparent sprite sheet for a handheld adventure in late 16-bit pixel art. Horizontal strip of five isolated cutouts on one floor plane, with wide transparent gaps. Native GBA view reference: detailed stone, warm lantern lighting, dark copper, soft colored shadows, clear pixel clusters, restrained edges, front three-quarter fixed camera at a slight downward tilt, consistent with a classic 1990s pixel-art adventure. The small art must read at 12-34 actual game pixels. No text, captions, panels, UI, background, subdivisions, characters, and no cast shadows outside the silhouettes. Draw from left to right: (1) at the Junction in Zork III, a wide rough squat slab-like gray-brown boulder, with one old silver Elvish sword embedded almost vertically in the top with its compact brass crossguard and leather grip above the rock; the entire rock and embedded weapon form ONE cutout, sword visible in silhouette, no pedestal; (2) a round dry glass bottle with short neck and small cork, with faint blue water visible through its bottom half; (3) the same glass bottle empty, identical silhouette and cork; (4) a tied, elongated brown lunch sack of worn paper with folded creases, long and low on a table; (5) one tiny closed antique bronze oil lamp, shaped like an elongated teardrop with a pointed spout to its left and curved handle on the right, grounded in three-quarter view. Every cutout should have visible deliberate chunky pixels rather than high-res random texture. Keep all five art silhouettes fully separated and genuinely transparent.

## Rock after the sword is removed

Create one isolated transparent PNG sprite based on the FIRST cutout (the chunky gray-brown boulder with an Elvish sword) in the supplied sheet. Preserve the rock's same flattened irregular silhouette, footprint, broad warm gray top, sunken dark left facets, mottled gray-brown clefts, late 16-bit pixel clusters, and lighting from the upper left. Remove the sword completely, leaving an unobtrusive narrow dark indentation where it was embedded. Draw very nearly the same boulder in the same angle and proportions, as a before/after state on the GBA. The rock alone, centered, nothing else from the source sheet. No floor, text, drop shadow, UI, or backdrop. Real transparent alpha.


## Revised time machine

Style references `assets/jewel_room.png` and `docs/art-history/encounter-options/imagegen-human-lineup.png`. This pose is used through the museum route.

Standalone transparent sprite of Zork III's movable golden time machine for the supplied museum room in a 16-bit fantasy adventure. Follow the described object literally: relatively large surprisingly shiny gold machine, a low open mechanical seat with a simple dark plum cushion, and a distinct squat instrument console directly in FRONT of the seat with one red button, one dark round dial and one recessed small three-digit amber display. This is museum technology built to be pushed as one unit. The seat and console sit together on a compact broad low chassis with two dark-bronze feet or short rollers on the same ground plane. No circular hoop, canopy, throne back, scrolling filigree, central arch, hanging parts or tall outline. Show the front and side, with the console at front-left, the viewer looking slightly down matching the foreground stone floor perspective in the supplied image. The seat has a believable room-facing entry on its nearest side and is visibly accessible. Main forms in warm bronze, tarnished shaded gold and selective bright warm pale gold, colored edges and no hard black outline. Few deliberate large pixel clusters that will survive downscaling to around 45x32 game pixels. No prop except the golden machine, no shine outside the silhouette, no visible readable letters, no floor or cast shadow. Real transparent background.

## Cellar chute

[cellar_chute.png](cellar_chute.png) is a 48×34 transparent sprite generated with built-in ImageGen. Its original references were [Slide Room](../z1_slide_room.png) for the chute material and [Cellar](../z1_cellar_hatch.png) for lighting and perspective.

> Create ONE isolated transparent-background GBA scenery sprite for the bottom end of the steep metal chute. Reference 1 establishes the SAME chute: dark worn smooth gunmetal with a continuous shallow trough and narrow curved raised edges, no rungs. Reference 2 is ONLY the target room palette and perspective: damp dim green-brown cellar. In the cellar west alcove, the chute disappears upward into deep shadow toward screen upper left, sweeps down toward camera/right, and its broad lower lip lands flush on the stone floor. End view from below, rounded curved trough with believable thickness and strong recognisable shape. Tight near-square sprite silhouette, late 16-bit clustered pixel art with muted green-gray highlights and dark bronze-grey underside, readable after resizing to around 48 by 42 actual pixels. Make entire object visible on real transparent background, no stone, no room, no stairs, no ground or cast shadow outside the silhouette, no text. It should feel like a worn metal slide integrated into an old dungeon rather than a clean geometric ramp.

Controller replay verified **Down** from Zork I Slide Room (31) to Cellar (33). The Cellar description puts the foot of the steep, unclimbable metal ramp on the west side. Both lit hatch states use the same sprite and anchor. The darkness checkpoint hides it.

The second built-in ImageGen pass used the first chute sprite and the same Cellar backdrop.

> Revise the first reference, the isolated rusty gunmetal GBA chute sprite. Keep its lower exit lip, shallow trough, narrow rolled rails, worn green gray metal, and curved diagonal profile. Change the upper-left third decisively: this is only the visible foot of a very long twisting, steep, unclimbable chute descending from an upstairs cave. The upper track curves back and up away from the viewer into pitch-dark recess of the west alcove. Progressively fade BOTH rails and interior to full transparency in the upper third, like something passing into darkness, with absolutely no visible cut-off top, upper edge, or capped end. It must read as a chute coming out of a shadowy passage, not a freestanding slide. Preserve a strong contrast gradient with the lower exit clearly readable at 47 x 44 native pixels, but upper end barely perceptible. Actual transparent background; no walls, rocks, text, floor, or surrounding effect. Second reference supplies lighting context only. Late 16-bit restrained clustered pixel art, proportion near square.

The third built-in ImageGen pass used the second chute sprite as its reference.

> Extend this dark green-gray GBA metal chute UPWARD, preserving its original lower curved exit lip, patina and shallow worn trough. Make a taller narrow sprite about 47:77 width:height, same original lower portion floor-level; add a steep, dark continuous upper segment along the LEFT of the image. Crucial: both upper rails and the dark metal between them physically cross and are cut by the TOP CANVAS EDGE, with a continuous track cross-section roughly a quarter of the sprite's width at the top. The cut is straight against the canvas edge and there is ZERO transparent padding above it; it continues off screen. No visible top cap or upper endpoint. Fade the upper metal nearly to black but keep subtly readable rails. Transparent everywhere outside the chute, no room, walls, shadows, text, or floor. Late 16-bit GBA pixel art.

The final built-in ImageGen pass used the third chute sprite as its reference. The native artwork preserves the lower curve at the larger scale and crops the upper run at the left edge.

> Redesign the placement and silhouette of this dark worn gunmetal dungeon chute sprite. It MUST enter the image THROUGH THE LEFT VERTICAL FRAME EDGE, between 22% and 42% down the canvas. At that boundary it is cropped with the chute trough and both rail edges visibly continuing leftward beyond the frame, NEVER exiting at the top. Leave the ENTIRE top 18% of the canvas transparent. Then the chute sweeps steeply down and diagonally right, rounding into a shallow lower lip that rests at the bottom-right, with the same restrained green-gray metal, bronze undertones and curled narrow rails as the reference. Side perspective, enough width for a person to slide, dark where it enters at the far left and readable near the floor. Late 16-bit GBA pixel art. A rectangle roughly 1.3:1 width to height. Transparent everywhere else, no walls, ground, extra objects or text. The left edge intersection is non-negotiable.
