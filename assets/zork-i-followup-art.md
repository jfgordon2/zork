# Zork I follow-up artwork

The production PNGs use the built-in ImageGen tool. Room plates are cropped and scaled to 240 × 102, and the GBA shows rows 10 through 85. The item atlas uses native-sized transparent silhouettes with floor, tabletop, and container placements tied to the story.

## Room sheet prompt

You are making production background plates for first-person Zork I on a 240×160 GBA. Large readable deliberately clustered 16-bit pixels, restrained highlights, grounded perspective, and worn materials. Camera at player eye height looking slightly down. Leave the ground support clear and within the middle and lower parts of each view. Match supplied reference art in palette and rendering. Return exactly one 2x2 image composed of four separate edge-to-edge rectangular game backgrounds, locked to equal-size quadrants with exact center seams, no margin or gutters. Each quadrant is a wide 2.35:1 panorama of an entirely different view. No captions, text, figures, icons, borders, interface, or stickers. Important geometry and floor contact should read when each separate quadrant is downsampled to 240x102 (only rows 10 through 85 will be visible).

### dam rooms

Style reference set `dam.png`, `technology_museum.png`

1. TOP LEFT, a clearly indoor abandoned dam visitor lobby. Cut gray-green masonry and worn floor, a simple wooden reception desk across the center-left with empty flat tabletop around 60% height, blue-gray filtered light from open exit south, doors to north and east, no books or loose props.
2. TOP RIGHT, the dam maintenance room. Squat damp green-gray masonry chamber, copper water pipe on east/right wall, two doorways left/back and shallow water channels in the flagstones up front. A central box with FOUR separate blank matching round metal button sockets in one horizontal line at x=30, 42, 54, 66 percent, y around 44 percent. Room for colored controls added separately. Space on floor under sockets for tools. No water over floor yet.
3. BOTTOM LEFT, reservoir north facing south in a vast cavern, muddy slate ledge with a detritus waterline, tall chalky slate-white cliff at right. Full steel-blue water covers the middle and foreground so a narrow near ledge remains. In the far south left, recognizable straight masonry dam wall.
4. BOTTOM RIGHT, exactly the SAME reservoir immediately after draining, same camera, left masonry dam and white chalk cliff to the right, same ledges. Replace high water with broad muddy exposed basin, old waterline, moist gray shingle and only a narrow silver stream far away. Flat accessible basin in center foreground for treasure added separately.

### river

Style reference set `dam.png`, `aragain_falls.png`, `cliff_base.png`

1. TOP LEFT, the base of the huge masonry dam as seen from a low west landing looking downstream to the southeast. Huge regular mossy dam abutment left behind our shoulder, towering pale gray-white limestone cliffs on EAST (screen left while looking south from this position use them visibly across the opposite right bank), dark blue-gray flowing water, low accessible stone landing at lower left and center. No boat.
2. TOP RIGHT, first clear reach ON the Frigid River looking DOWNSTREAM SOUTH. Dam is behind the viewer. Tall pale chalky limestone east bank on left if looking south, a continuous massive chalk bluff on screen left, dark gray rock shoals on the right west bank leave no landing. Broad calm central slate-blue downstream channel fills foreground so an inflatable boat sprite can sit on the water. Natural cavern lighting.
3. BOTTOM LEFT, farther along the same blue-gray river looking south, visibly faster current bends between the pale east-side cliffs and a low narrow strip of gray shingle on west/right. White water ripples kept to the sides with open central water in foreground for a boat. Distant hazy valley and strong geological continuity with the prior view, dark slate and pale chalk, no boat.
4. BOTTOM RIGHT, next downstream stretch of the same Frigid River. Crouching pale cliffs along left east bank draw away to reveal an accessible warm sandy bay on the LEFT, small slate landing under towering broken gray cliffs on west/right. Increasingly strong current towards a distant misty waterfall downstream center, water foreground for boat, no boat or buoy.

### river ends

Style reference set `aragain_falls.png`, `cliff_base.png`, `water.png`

1. TOP LEFT, Sandy Beach at water level on the EAST shore of Frigid River. First-person view standing on broad level pale sand across lower half of frame, dark damp sand touching the steel blue-gray river on left, water flows past toward distant spray and same pale chalk cliffs. Low partly buried tunnel mouth on upper right. Empty open sand for a beached boat and shovel.
2. TOP RIGHT, west shore below White Cliffs. Tall dominant rugged pale-gray-white chalk cliff rises behind a narrow rocky landing that occupies the foreground right; continuing blue-grey river with haze on left. A few chalk boulders and no objects. Looks connected to the previous paintings and distinct from sandy east shore.
3. BOTTOM LEFT, final very fast rapids ABOVE Aragain Falls from water level, the same pale blue-gray cliff walls part at the edge of a mighty drop, heavy mist ahead, strong flowing froth recedes; open foreground water for boat; no boat. Show where the current is going.
4. BOTTOM RIGHT, reservoir's south bank at the top of the dam. Shallow slightly elevated stone landing foreground, reservoir and atmospheric blue slate cliffs ahead, distinctive low built masonry on the right and quiet water in center. Near foreground stone patch largely dry and visibly walkable.

## Added river path scenes (Nov 19)

All three are independent 240×102 production plates; the game shows source rows 10–85.
Their style references are `z1_river_valley.png`, `z1_river_beach.png`, `aragain_falls.png`,
`z1_sandy_beach.png`, `z1_canyon_bottom.png`, `cavern.png`, and `z1_reservoir_south.png`.

- **Z1 room 40, Shore (`z1_shore.png`):** east-bank path with the river on the left,
  running north-south and bending west toward Aragain Falls. Preserve a continuous walking
  surface, chalk cliffs, treacherous water, and clear foreground for item overlays.
- **Z1 room 67, Stream View (`z1_stream_view.png`):** a modest damp-rock stream flowing
  west to east, with the path following it and bending toward Reservoir South. Keep a low
  access point to the water for the conditional south route; avoid the broad Frigid River.
- **Z1 room 86, Stream (`z1_stream.png`):** low water-level view in a tighter twisting
  limestone channel, upstream visibly too narrow for navigation, downstream hidden around
  a bend, and a narrow beach connected directly to the water for landing.

These were checked as 240×76 game crops against their connected scenes. Their mGBA overlays,
drop states, and persistence are still unchecked.

### treasure

Style reference set `forest.png`, `kitchen.png`, `cavern.png`

1. TOP LEFT, tree canopy high above the White House forest. Camera on a thick broad horizontal pine branch spanning foreground about 68–82 percent height, needles and faded evening blue around it, ground visibly many feet below through gaps, branch fork slightly left of center where a separate tiny bird nest overlay can rest. No bird, nest, or egg.
2. TOP RIGHT, the attic of the very small wood house. Low angled roof rafters sloping down left and right, rough old tan pine beams, worn boards, dusty wall, faint late dusk from a tiny high vent. A small low empty rectangular wooden table at center-right with the top clearly visible at about 58–67 percent height. floor space near foreground. No clutter or loose items.
3. BOTTOM LEFT, maze dead end containing remains of a long-dead adventurer. Same twisty rough cave style but clearly a low dead-end alcove, warm amber glancing across empty ground in foreground, bone-colored crumbling limestone at edges. Blank patch near center for a separate skeleton and several props. No bones or collectible things baked in.
4. BOTTOM RIGHT, the underground Torch Room overlooking the Dome Room. Warm brown rock and old worked stone, a single low wide hewn stone pedestal near center with visible small empty top at 58–62 percent height, quiet tomb atmosphere, distinct passages at rear and left, no torch, no flame.

### routes

Style reference set `cavern.png`, `engravings.png`, `hall.png`

1. TOP LEFT, a square subterranean Mirror Room. Huge single aged dark glass West Empire mirror fills one far wall, rough rectangular carved stone frame, glass has muted teal slate reflections with broad clean unbroken surface, other walls and side exits visible, floor uncluttered. Camera aimed at the North wall with side passages visible either side.
2. TOP RIGHT, the companion Mirror Room on the other side of the mirror. Huge far-wall mirror in a closely matching frame, slightly warmer brown and blue stones, irregular bands and changed side columns to tell the two ends apart. Camera aimed at the SOUTH wall, exits visible both sides. Low neutral amber lighting, empty floor, no characters.
3. BOTTOM LEFT, Dome Room, atop a rounded underground dome looking out over the drop. The floor abruptly ends into a central near-black abyss; a simple horizontal old wooden railing, two sturdy round posts, across near edge at 55 to 68 percent of height, creates readable foreground ledge with space left of middle to tie a separate rope down the drop. Curved worked-stone dome in shadow across gap, no hanging rope or accessories.
4. BOTTOM RIGHT, top of a deep hand-worked vertical mine shaft. One wide black shaft opening toward center-left in a rough flagstone floor, stone rim with small rusty winch mechanism, chain will be drawn by game from overhead, leave its path clear at center. No drawn chain, rope, people or basket. Cool moss-grey mine masonry and faint bronze reflected light.

### other routes

Style reference set `forest.png`, `cavern.png`, `barrow.png`, `water.png`

1. TOP LEFT, small quiet pine clearing in the same evening forest with a footpath receding south. Level earthy patch across center foreground for a grated opening to be composited after leaves are moved, flat dark soil and a few yellow leaf accents outside that patch. No baked hatch, grating, leaf pile or props.
2. TOP RIGHT, Grating Room underground just below a clearing. Low fractured stone ceiling with an iron-reinforced but dark rectangular aperture at center-top, silhouette shaped to admit a separate metal grate and relatively small, nearby textured dirt ceiling/roots. Warm lamp light on charcoal walls, a chamber below with irregular stone walls and crawlway at far right. No grate baked into art. Preserve clear distinction between overhead aperture and ground. Forest greenery hinted from its edges only.
3. BOTTOM LEFT, sunlight-dappled forest exterior approach to the Stone Barrow, a massive ancient earthen tomb retaining wall with gray boulders and massive open doorway on its east face directly ahead in the right half, dark interior visible. Packed-earth path leads in right across foreground, tall cool firs on both sides, match the original house's forest dusk and live outside atmosphere. No people.
4. BOTTOM RIGHT, finale looking FROM the dark barrow into its enormous brightly lit cavern. Warm amber-gray stone threshold surrounds composition, blue-white light from the broad open cavern, central wide stream visibly running horizontally across foreground and a plain little wooden footbridge across it near center connecting to a trail towards a small dark tunnel deep in background. Strong near-to-far, no readable sign, no people.

### shaft lower

Style reference set `cavern.png`, `hall.png`

1. TOP LEFT, Drafty Room at the FOOT of the same old vertical mine shaft, from ground level looking toward a shaft recess and rough stone tunnel. Ceiling is hidden high only within the vertical chimney, with center overhead stone lip. Leave an open flat round patch of grey-brown stone below the opening in the middle foreground for the lowered wicker basket overlay. Old close rock, hints of amber and cold green, no chains already painted.
2. TOP RIGHT, inside Zork I's Strange Passage, a tight long rough rock tunnel with a low wide opening, there is a massive broken old first floor plank door beyond, splintered panels around a very large broken Cyclops-sized hole through which the tiny cottage interior has warm low daylight, with old cave rock near the camera. No visible people.
3. BOTTOM LEFT, viewed inside the cellar facing up toward a small trapdoor aperture at the top of a short crude wooden staircase. Dark grey-green stone basement with chute in deep shadows, first person plane wide as references. The overhead hatch is visible and closed, not part of any central floor, no props.
4. BOTTOM RIGHT, camp for miscellaneous lost adventurer coin and key art reference only, an empty quiet underground slate wall tunnel, no objects.

## Transparent item sheet prompt

Create one horizontal strip of FIVE separate small transparent sprite cutouts for Zork I on Game Boy Advance. Late 16-bit first-person adventure style, warm gray shadows, muted slate, upper-left warm light, grouped deliberate pixels and precise readable silhouettes at tiny screen scale of about 7 to 48 px across. Camera looks slightly down from the front in three-quarter perspective, every sprite can be staged as a native overlay. Equal-width cells in one left-to-right line with large fully transparent gutters. Authentic transparent alpha. No floor, characters, background, screen UI, panels, lettering, typography or cast shadows outside the objects.

### marine

Style reference set `dam.png`, `aragain_falls.png`

1. Small folded pile of tan-gray rubberized cloth, origami-like creases and tiny bronze valve sticking out of one side, visibly a compact deflated boat, low lying silhouette.
2. The same boat inflated into a low oval two-ended old flexible tan-gray rubberized boat, beige rounded tube and inside wooden slats, diagonal runs left back to right front. Slightly worn muted tan, simple opaque black water-dark interior, no flag or oars, clearly not a rigid modern powerboat.
3. Same boat deflated and collapsed from a puncture, same warm tan worn material and floor angle and a little hole in one sagging edge.
4. One small red fabric-coated navigational buoy, oval flared shape and dark attachment near the top, red with faded pale ring, no lettering, upright half-submerged profile so the lower part of the silhouette is softly level.
5. One plain broad iron-bladed shovel, weathered brown wood grip, laid low horizontally on a solid surface with stone-gray blade at left.

### upstairs

Style reference set `living.png`, `forest.png`, `cavern.png`

1. One small bird's nest only, angled slightly down into a coil of dark warm twig and straw, bowl empty and deep dark amber, chunky readable natural texture.
2. One loosely coiled brown hemp rope with two readable thicker overlapping loops and a loose end, laid horizontally on old wood, unadorned.
3. One short nasty old steel knife, visibly worn triangular blade and dark leathery grip. Angle lies horizontally on a table, blade light slate steel not reflective polished chrome.
4. One complete ancient human skeleton gently slumped at the foot of a cave wall, all connected anatomically, mostly pelvis, dangling legs and ribcage, tarnished pale bone and dark sienna occlusion, seated low and compact, no weapon, props or gore.
5. One small dark brown battered leather drawstring bag of old coins. One highlight on a visible bronze coin just inside its mouth, low soft silhouette, no lettering.

### ritual

Style reference set `egyptian.png`, `hades.png`, `engravings.png`

1. One ceremonial Egyptian sceptre with low horizontal tapered gold shaft and tiny colored enamel at flared end, readable metal sheen, blade-like point at one end. Lies almost horizontal and low to fit inside the coffin sprite.
2. One joined pair of short worn beige ritual candles with dark wicks, small melted pool inside a dark stubby holder, UNLIT, front angle.
3. EXACT same pair of beige candles in same holder, same width and size, LIT with one small amber flame over each wick and no stray glow. Render whole pair.
4. One small red-hot brass bell tipped on its side, readable small curved dark bronze brim, hot reddish copper body, selective warm bright near-white edge but no smoke.
5. One thick old black prayer book laid shut, worn slate-black leather binding, faint dark-red spine and muted parchment pages exposed at right edge, no symbol or lettering.

### controls

Style reference set `dam.png`, `hall.png`

1. One tiny straight-sided hand held bicycle-style air pump, worn golden brown and brass tube with a dark plunger T handle, horizontal on surface.
2. One heavy short tarnished steel open-end wrench with one hole near the grip, lying diagonal, no other tools.
3. One small old brass and wood screwdriver with compact handle and bright gleam on slender shaft, placed horizontal.
4. One old flattened metal toothpaste-style tube with a folded base and dark cap, lies on its side, off-white and tarnished blue, no writing.
5. One closed small visitor guidebook with dusty pale teal cover, thin ochre page edge and two faded broad unreadable stripes on it. Lies flat, antique paper.

### shaft

Style reference set `cavern.png`, `forest.png`

1. One sturdy round wicker mining basket with dark empty open top, low wall and two little metal hanger tabs, suspended by the scene later, view slightly into the basket and wide, warm brass highlight on edges. No chains.
2. One one-piece rectangular antique iron bar grate in a weathered frame, read as a horizontal cover viewed slightly down obliquely, square grid with open black spaces between. Chunky discrete struts not a fine texture. No surrounds or soil.
3. Cover for the same iron grate seen upward and swung open vertically, tilted slightly backwards. Same rectangular size and iron, holes open transparent, pale green daylight glances on top edge.
4. One small low pile of dead dark umber, ochre and weathered yellow leaves, looks naturally collected on the clearing floor. Only leaves, contained silhouette.
5. One forgotten rectangular treasure trunk, dark ochre-brown riveted wood and clear cameo of a few jewels near a separated lid gap. Long mud-smeared sides. Simple closed-looking horizontal floor profile.

### Ritual book, broken egg, hanging rope, boat label, and tool chests

Create one horizontal strip of FIVE isolated small transparent game sprites in a row for Zork I, Game Boy Advance. Adjacent cells with very broad gutters, same approximate scale across. Style is tight warm shaded late 16-bit fantasy adventure pixel art matching the supplied reference, clear readable chunky 8–28-pixel silhouettes, real transparent alpha. No floors, cast shadows, writing or text. Left to right: (1) one very large open black leather ritual book, viewed at a shallow angle almost horizontal such that both yellow ivory pages are visible, has understated short dark illegible abstract strokes that read as writing but no lettering, rectangular book exterior; (2) one broken jewel-encrusted golden egg in two touching shell halves, studded burgundy red and small sapphire enamel, lying open and cracked, show dark interior with tiny bent-gold gear; (3) one long slender vertical hanging hemp rope with two tightly tied little loops around an unseen narrow rail at its top, slightly tapered shape, parallel fibers in few muted ochre pixels, no rail; (4) one small folded rectangular tan yellowed boat instruction card, lying low, narrow brown edge, no writing; (5) low open pile of two squat rusted dark steel tool chests with hinges and aged brown metal latches, simple group in museum maintenance room, no tools visible.

## State-specific artwork

### rear open

Reference `rear.png`

Keep the exterior composition, camera, clapboard house, forest, ground, palette, and framing unchanged. The intact four-pane kitchen window is open for entry: the lower sash slides straight upward, leaving the lower half of the frame as a dark rectangular aperture while the upper two panes remain in place. Do not show the interior floor or hearth through the opening. No hinge, broken glass, added objects, person, or text.

### kitchen shut

Reference `kitchen.png`

Keep the kitchen composition, table, hearth, shelves, shadows, doorway, and perspective unchanged. Show the same intact four-pane window from inside with the lower sash down in its vertical tracks and the dusk forest visible through all four panes. No hinge or broken glass. No figures, captions, UI, or new props.

### kitchen open

Reference `kitchen.png`

Keep the kitchen composition and furnishings unchanged. Show the same window with its lower sash slid straight upward; retain the upper two panes and leave the lower half as an open view to the dusk forest. Keep the opening aligned with the exterior view. No hinged pane, added props, captions, or UI.

### hatch open

Reference `living_moved.png`

Edit this exact 16-bit living room artwork. Preserve camera, scale, all objects, walls, lighting, foreground, palette and framing exactly. On the floor in the center immediately right of the rolled-up rug, open the very same wide closed rectangular trap door on its back hinge. Replace the area inside the frame with a readable dark rectangular hole in the floor revealing the first two treads of rough rickety wooden stairs descending into black. Put the plank hatch upright behind the hole, along its farther edge at floor level. Its lower rectangular opening remains in precisely the existing position, similar projected width, preserves perspective of parallel floorboards. Keep existing left and right doors, furniture and width. No highlights or outlines from UI, no words or people.

### shortcut open

Reference `living_moved.png`

Edit this exact first-person 16-bit pixel art Living Room frame. Keep the old cottage, rug rolled up, the closed central hatch, floorboard direction, cabinet, fully unchanged camera and lighting. ONLY change the old very left-edge closed wooden door against the wall behind the chair. The Cyclops has burst through it. Turn the central part of that door into a large ragged dark walkable opening with splintered old plank edges and a little glimpse of a grey rocky passage. Keep its frame, right trim, floor, chair, rug and surroundings intact. Work within the leftmost 20 percent. No cyclops, captions, UI, flames or props.

### dam open

Reference `dam.png`

Edit this exact wide faded green-blue pixel art background. Preserve camera, platform, antique console, mounting plate, stone parapet, reservoir lake height, colors, cliff geometry. Only modify the monumental stone dam wall at upper right and the water immediately below it. Draw open broad black rectangular sluice gates with pale fast foaming blue-white water pouring prominently from their low gates into the downstream channel, clearly readable at 240x76. Confine the edit to the dam on screen right. No lighting changes on its console or foreground, no text, people or game UI.

### dam low

Reference `dam.png`

Edit this exact wide 16-bit GBA Dam background. Preserve camera, placed foreground with blank metallic console and mossy parapet, dark slate large cavern and regular huge stone dam structure at upper right. The reservoir behind this dam HAS DRAINED. Remove the broad high-standing water behind the parapet, revealing darker muddy stone slopes, a receded flat slate-brown exposed basin and a narrow ribbon of water down at the bottom. Show high damp waterline. Sluice openings on the dam visibly open and only a limited volume flowing through the old dam. Do not alter the console, left foreground or overall lighting. No text, people or props.

### coffin

Reference `egyptian.png`

TWO transparent game sprite cutouts side by side, wide clear alpha gutter. Draw a solid-gold Egyptian sarcophagus for the exact raised flat wide stone plinth in the attached 16-bit Zork I Egyptian Room. Treat the plinth as the perspective guide only, keep any floor and plinth OUT of the output. Camera square to the carved wall, very shallow downward look. Crucial axis: both the long front edge of the coffin and its long top run perfectly HORIZONTALLY left-to-right, parallel to the long front edge of the tan plinth behind it in the reference. No diagonal tilt of the coffin axis, no foreshortened one-point three-quarter view, no feet under it. A slim low, solid ancient gold anthropoid coffin, head relief at LEFT, feet at RIGHT; burnished dull gold with selective blue enamel and detailed hand-clustered warm brown pixel shading. LEFT sprite is it closed, the face is on top in shallow perspective. RIGHT sprite is the same identical lower box and footprint, lid opened on its far long hinge toward the back wall with the opening dark and visibly empty, lid rim and base stay level and completely horizontal. The empty trough mouth is wide enough for a separate very small horizontal sceptre in the game. Texture must read within 65 render pixels wide. Authentic transparent RGBA around and between both cutouts. No background, floors, writing, shadow outside silhouettes, stands, character, or UI.

### dam refill

Reference `z1_dam_low.png`

Edit this exact GBA pixel art backdrop of the drained Flood Control Dam. Same perspective, framing, stone console, setting, fully drained near basin and low damp waterline exactly. Only the metal sluice gates on the right are now SHUT and water has just begun to collect. Insert shut dark weathered metal gate doors into the visible open dam apertures, remove the small trickles from those apertures, add a few faint shallow blue-gray pools low in the basin floor without hiding the muddy foreground. Preserve image scale, colors and sharp 16-bit pixel style. No extra features, text or objects.

### reservoir south dry

Reference `z1_reservoir_south.png`

Edit this exact GBA first-person pixel art Reservoir South backdrop. Preserve reference camera, cliff geometry, temperature, masonry abutment on right, foreground stone landing and pixel rendering. The reservoir is now DRAINED. Remove the high still-water surface behind the near shore across the middle of the view. Reveal the newly exposed beige-brown slate sediment floor below a clearly visible damp waterline on same far cliffs. One small distant narrow channel of dark blue water still drains away. Natural deep underground ambience, dry-accessible near ground. No treasure chest, sprites, objects, UI, lettering or figures.

### altar

Reference `egyptian.png`

A first-person wide 2.35:1 GBA pixel-art location backdrop for the Altar in Zork I, compatible with the game view of the adjacent Egypt tomb reference and its warm earth hues. A small quiet underground temple chamber, mitered warm dark limestone blocks, facing a single ancient modest freestanding carved stone altar placed near center, with its level bare slab top visually clear at around 55–63 percent height of the image. Leave top EMPTY for a black prayer book and candle sprite placed separately. Stairs descending into the shadow at bottom right. Give the altar clear horizontal tabletop plane, distinct stone rim, no other furniture or mobile objects. Selective warm lantern bounce, slate shadows, no wallpaper, no text, people, light source, torches, UI or border. Detailed 16-bit pixel clusters and decisive large silhouettes.

### cellar hatch open

Reference `z1_cellar_hatch.png`

Edit this exact first-person Zork I cellar scene for the open trap door state. Preserve camera and all decoration or stone. At the top of the stairs change only the small overhead dark wooden hatch slab, now open, leaving the overhead opening visibly empty. Let faint warm house light fall over its upper treads, preserve very dark green slate cellar tone. No items, person or text, same wide framing, same pixel art.

### Walkable ledge at Reservoir North

Edit this exact wide 16-bit Zork I Reservoir North location. Preserve pixel palette and texture, far southern dam left, white chalk cliff right, and wide full high-water surface. In the NEAR foreground add a low narrow visibly walkable stone ledge on the lower right, rendered from exactly the current camera and below the same slate-white cliff. It should have an accessible level area from about x 55–88% of frame and fill at most bottom quarter, connect to the land at right, and show a dark waterline as the blue-gray lake laps the left edge. Keep the reservoir body in the center and left, do not turn the view into a beach. Actual game crop shows rows 10 to 85 of a 240x102 plate, so the dry shelf should be visible before row 85. Flat patch intentionally empty for a little portable air pump. No boat, prop, people, writing, borders or interface.

## Added distinct room backgrounds (Nov 19)

- **Z1 room 25, Damp Cave (`z1_damp_cave.png`):** distinct damp limestone chamber with
  east and west walking tunnels and an irregular, floor-level south crack. References:
  `z1_sandy_cave.png`, `z1_tiny_cave.png`, `z1_white_cliffs.png`, `z1_loud_room.png`.
- **Z1 room 162, Deep Canyon (`z1_deep_canyon.png`):** south canyon ledge above running
  water, level passage approaches and one descending stair toward the Loud Room. References:
  `z1_canyon_bottom.png`, `z1_chasm_south.png`, `z1_chasm_east.png`, `z1_loud_room.png`.
- **Z1 room 220, Studio (`z1_studio.png`):** paint-splattered room, open painted door
  toward the Gallery, and a separate fireplace with a dark narrow chimney. Object 41, the
  ZORK owner's manual described as a small piece of paper on the wall, is a separate sprite
  anchored to the painted wall at (68,58); it is not baked into the background. The room's
  two floor-drop anchors remain at (75,86) and (168,86). References: `z1_gallery.png`, `z1_torch_room.png`,
  `z1_attic.png`, `z1_maze.png`.
- **Z1 room 238, Forest Path (`z1_forest_path.png`):** a clear north-south earth path beside
  a large low-branched tree that matches `z1_tree_canopy.png`; the branch does not block the
  path. References: `forest.png`, `z1_tree_canopy.png`, `north.png`.

All four are 240×102 plates with the important geometry inside visible rows 10–85. Their
room-ID mappings and floor anchors are in `src/chapter.c` and `src/room_visuals.c`. They have
not yet been checked in mGBA.
