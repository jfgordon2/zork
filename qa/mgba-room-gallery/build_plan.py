#!/usr/bin/env python3
"""Build the mGBA capture manifest from route checkpoints and traced state logs."""
from __future__ import annotations
import csv
import os
import pathlib
import re
import subprocess

ROOT = pathlib.Path(__file__).resolve().parents[2]
OUT = ROOT / 'qa' / 'mgba-room-gallery'

chapters = [
    {
        'name': 'Zork I', 'slug': 'zork-i', 'chapter': 0,
        'route_root': 'qa/mgba-room-gallery/route-data/zork-i-mine-audit',
        'trace_roots': ['qa/mgba-room-gallery/route-data/zork-i-mine-audit'],
    },
    {
        'name': 'Zork II', 'slug': 'zork-ii', 'chapter': 1,
        'route_root': 'qa/mgba-room-gallery/route-data/zork-ii-finish/re-audit',
        'trace_roots': ['qa/mgba-room-gallery/route-data/zork-ii-art/re-audit', 'qa/mgba-room-gallery/route-data/zork-ii-finish/re-audit'],
    },
    {
        'name': 'Zork III', 'slug': 'zork-iii', 'chapter': 2,
        'route_root': 'qa/mgba-room-gallery/route-data/zork-iii-audit',
        'trace_roots': ['qa/mgba-room-gallery/route-data/zork-iii-audit'],
    },
]
SHOT = re.compile(r'^SHOT ([\w-]+) room=(\d+) scene=(\S+) title=(.*?) dark=([01])', re.M)

# Older route logs store the room art that was selected when those traces were
# produced. Resolve current mapper-backed scenes from source so a refresh does
# not relabel newly illustrated rooms as the old generic hall/cavern/water art.
Z1_FIXED_SCENES = {
    2: 'z1_temple', 5: 'z1_tree_canopy', 8: 'z3_timber_room', 11: 'z1_dome',
    15: 'z1_round_room', 16: 'z1_ladder_top', 18: 'z1_torch_room',
    30: 'z1_sandy_beach', 31: 'z1_slide_room', 37: 'z1_dam_base',
    45: 'z1_river_valley', 46: 'z1_gas_room', 52: 'z1_mirror_north',
    53: 'z1_river_rapids', 72: 'z1_ladder_bottom', 89: 'z1_machine_room',
    93: 'z1_smelly_room', 111: 'z1_dam_base', 117: 'z1_strange_passage',
    120: 'z1_dam_lobby', 122: 'z1_gallery', 129: 'z1_altar',
    156: 'z1_mine_dead_end', 160: 'z1_grate_clearing_leaves', 166: 'z1_white_cliffs',
    167: 'z1_forest167',
    171: 'z1_squeaky_room', 174: 'z1_river_bend', 182: 'z1_river_beach',
    184: 'z1_white_cliffs', 187: 'z1_attic', 202: 'z1_rainbow_top',
    205: 'z1_loud_room', 216: 'z1_grate_cellar', 218: 'z1_maze',
    224: 'z1_maintenance', 245: 'z1_shaft', 249: 'z1_shaft_bottom',
}
Z1_FIXED_SCENES.update({
    3: 'z1_coal_mine', 21: 'z1_rocky_ledge',
    54: 'z1_maze', 62: 'z1_tiny_cave', 69: 'z1_engraving_cave',
    74: 'z1_canyon_bottom', 90: 'z1_coal_mine', 104: 'z1_maze',
    106: 'z1_chasm_south', 112: 'z1_coal_mine', 130: 'z1_east_west_passage',
    134: 'z1_maze', 139: 'z1_cold_passage', 155: 'z1_sandy_cave',
    158: 'z1_maze', 170: 'z1_maze', 173: 'z1_coal_mine',
    175: 'z1_north_south_passage', 176: 'z1_tiny_cave', 188: 'z1_mine_entrance',
    193: 'z1_maze', 228: 'z1_narrow_passage', 235: 'z1_bat_room',
    19: 'z1_maze', 34: 'z1_maze', 42: 'z1_maze', 50: 'z1_maze', 51: 'z1_maze',
    81: 'z1_maze', 152: 'z1_maze', 161: 'z1_maze',
    141: 'z1_maze_dead_end', 143: 'z1_maze_dead_end', 147: 'z1_maze_dead_end', 153: 'z1_maze_dead_end',
    25: 'z1_damp_cave', 40: 'z1_shore', 67: 'z1_stream_view', 86: 'z1_stream', 110: 'z1_mirror_north',
    162: 'z1_deep_canyon', 220: 'z1_studio', 238: 'z1_forest_path',
    247: 'z1_chasm_east',
})
# The older traces call these dedicated mapper scenes ``hall``. Keep these
# current source-backed art names in the gallery when rebuilding labels.
Z1_FIXED_SCENES.update({
    89: 'z1_machine_room', 93: 'z1_smelly_room', 122: 'z1_gallery',
    156: 'z1_mine_dead_end',
})
Z1_STATEFUL_ROOMS = {27, 33, 75, 85, 95, 123, 178, 191, 244}

Z2_SCENE_BY_ROOM = {}
for match in re.finditer(r'^ZORK2_SCENE\((\w+),\s*(\d+),',
                         (ROOT / 'src' / 'zork2_scenes.def').read_text(), re.M):
    name, room = match.groups()
    if int(room):
        Z2_SCENE_BY_ROOM[int(room)] = f'z2_{name}'
Z2_STATEFUL_ROOMS = {8, 13, 19, 28, 30, 45, 46, 59, 69, 82, 125, 161, 182}

Z3_FIXED_SCENES = {
    5: 'technology_museum', 6: 'z3_beam_room', 8: 'z3_narrow_mirror',
    12: 'z3_underwater', 15: 'z3_timber_room', 16: 'jewel_room',
    19: 'z3_marble_hall', 23: 'land_of_shadow', 28: 'museum_entrance',
    30: 'land_of_shadow', 36: 'z3_parapet', 39: 'z3_key_room',
    40: 'z3_tight_squeeze', 43: 'z3_east_corridor', 44: 'guardians',
    45: 'z3_prison_cell_closed', 46: 'z3_treasury',
    51: 'z3_narrow_mirror', 54: 'guardians_approach', 55: 'z3_creepy_crawl',
    58: 'z3_narrow_mirror_guardians_north', 63: 'land_of_shadow',
    64: 'z3_foggy_room', 66: 'z3_technology_unfinished', 69: 'land_of_shadow',
    73: 'cliff_base', 75: 'z3_aqueduct_view', 78: 'z3_hairpin_loop',
    81: 'z3_crystal_grotto', 84: 'z3_royal_puzzle_entrance', 86: 'z3_dead_end',
    93: 'z3_damp_passage', 98: 'z3_button_room', 100: 'z3_narrow_mirror',
    102: 'lake_shore', 103: 'jewel_room', 105: 'dungeon_entrance',
    109: 'cliff_top', 112: 'z3_barren_area', 113: 'z3_guardian_hallway',
    116: 'z3_royal_hall', 119: 'z3_narrow_mirror', 123: 'land_of_shadow',
    124: 'z3_scenic_vista', 130: 'z3_side_room', 134: 'land_of_shadow',
    136: 'z3_great_door', 140: 'z3_west_shore', 143: 'z3_royal_puzzle_entrance',
    155: 'hall (fallback)', 159: 'z3_narrow_corridor', 160: 'mirror_room',
    161: 'z3_narrow_mirror_guardians_north_west', 162: 'z3_west_corridor', 163: 'z3_water_slide',
    # The stable route save reaches Hallway 169 with the mirror panel closed
    # (global 107 clear; mGBA renders z3_mirror_hall_closed). The open panel
    # has its own story-state capture, mirror-hall-open.png.
    169: 'z3_mirror_hall_closed', 170: 'z3_shadow_cliff', 172: 'engravings',
    173: 'museum_entrance', 176: 'technology_museum', 177: 'hall (fallback)',
    178: 'z3_narrow_mirror_guardians_west', 179: 'z3_jewel_unfinished', 181: 'flathead_ocean',
    182: 'z3_high_arch', 184: 'z3_lake_surface', 186: 'land_of_shadow',
    188: 'z3_side_room', 190: 'z3_south_corridor', 192: 'z2_room8',
    194: 'z3_puzzle_room', 203: 'z3_aqueduct', 204: 'z3_prison_cell_bronze',
    211: 'z3_endless_stair', 216: 'z3_south_shore', 217: 'junction',
    218: 'cliff_ledge_upper_shelf', 221: 'z3_narrow_mirror', 224: 'guardians_approach',
    227: 'museum_entrance', 228: 'hall (fallback)',
}
# story_scene() chooses these directional/statue variants from the physical
# room ID; older traces record only the common room title and omit that detail.
Z3_FIXED_SCENES.update({
    51: 'z3_narrow_mirror_west',
    151: 'z3_narrow_mirror_guardians_west',
    161: 'z3_narrow_mirror_guardians_north_west',
    221: 'z3_narrow_mirror_west',
})
Z3_STATEFUL_SCENES = {
    169: {'z3_mirror_hall_closed', 'z3_mirror_hall_open'},
    19: {'z3_marble_hall', 'z3_marble_hall_prison_open'},
    28: {'museum_entrance', 'museum_entrance_open'},
    45: {'z3_prison_cell_closed', 'z3_prison_cell_open'},
    173: {'museum_entrance', 'museum_entrance_open'},
    181: {'flathead_ocean', 'flathead_ship'},
    204: {'z3_prison_cell_bronze'},
    227: {'museum_entrance', 'museum_entrance_open'},
}

def current_scene(chapter: int, room_id: int, previous: str) -> str:
    if chapter == 0:
        if room_id in Z1_FIXED_SCENES:
            return Z1_FIXED_SCENES[room_id]
        return previous
    if chapter == 1:
        if room_id not in Z2_STATEFUL_ROOMS:
            return Z2_SCENE_BY_ROOM.get(room_id, previous)
        return previous
    if chapter == 2:
        if room_id in Z3_STATEFUL_SCENES:
            if previous in Z3_STATEFUL_SCENES[room_id]:
                return previous
        return Z3_FIXED_SCENES.get(room_id, previous or 'hall (fallback)')
    return previous

def lua_string(value: str) -> str:
    return "'" + value.replace('\\', '\\\\').replace("'", "\\'") + "'"

def collect_shots(roots: list[str]):
    shots = {}
    for root in roots:
        for path in sorted((ROOT / root).glob('*.log')):
            text = path.read_text(errors='replace')
            for m in SHOT.finditer(text):
                name, room, scene, title, dark = m.groups()
                shots[name] = {
                    'room_id': int(room), 'scene': scene, 'title': title,
                    'dark': int(dark), 'trace_log': path.relative_to(ROOT).as_posix(),
                }
    return shots

rows = []
for cfg in chapters:
    route_root = ROOT / cfg['route_root']
    shots = collect_shots(cfg['trace_roots'])
    route_files = sorted(route_root.glob('route-*-lit.sav'), key=lambda p: int(re.match(r'route-(\d+)-lit', p.name).group(1)))
    route_ids = set()
    for save in route_files:
        room_id = int(re.match(r'route-(\d+)-lit', save.name).group(1))
        route_ids.add(room_id)
        meta = shots.get(f'route-{room_id}-lit', {'room_id': room_id, 'scene': '', 'title': f'Room {room_id}', 'trace_log': ''})
        # The legacy room-12 SRAM returns from the frigid lake to Endless Stair
        # on Continue. The audited underwater checkpoint is stable and restores
        # directly into the room, so use it as the gallery's representative.
        capture_save = save
        if cfg['chapter'] == 2 and room_id == 12:
            audited = route_root / 'underwater.sav'
            if audited.exists():
                capture_save = audited
        output = f"{cfg['slug']}/room-{room_id}.png"
        rows.append({
            'game': cfg['name'], 'kind': 'route room', 'chapter': cfg['chapter'],
            'room_id': room_id, 'title': meta['title'],
            'scene': current_scene(cfg['chapter'], room_id, meta['scene']),
            'save_file': capture_save.relative_to(ROOT).as_posix(), 'screenshot': output,
            'trace_log': meta['trace_log'], 'dark': 0, 'capture_status': 'pending',
        })

    if cfg['chapter'] in (0, 1):
        # These named audit saves are observed room states with a recorded native
        # story trace. They show alternate scenery, door, item, and puzzle states.
        roots = cfg['trace_roots']
        variants = {}
        for root in roots:
            folder = ROOT / root
            for save in sorted(folder.glob('*.sav')):
                stem = save.stem
                if stem.startswith(('route-', 'step-', 'drops-')) or stem == 'ice-frozen' or stem not in shots:
                    continue
                meta = shots[stem]
                key = (stem, meta['room_id'])
                variants.setdefault(key, (save, meta))
        for (stem, room_id), (save, meta) in sorted(variants.items(), key=lambda item: (item[0][1], item[0][0])):
            # The old Guarded Room checkpoint was overwritten by a failed
            # locked-door experiment. Prefer the freshly traced open-door save.
            if cfg['chapter'] == 1 and stem in {'guarded-open', 'workshop-open'}:
                save = route_root / f'{stem}.sav'
                room_id, title, scene = (
                    (28, 'Guarded Room', 'z2_guarded_open')
                    if stem == 'guarded-open' else
                    (45, "Wizard's Workshop", 'z2_workshop_open')
                )
                meta = dict(meta, room_id=room_id, title=title, scene=scene,
                            trace_log=(route_root / 'door-check-refresh.log').relative_to(ROOT).as_posix())
            output = f"{cfg['slug']}/states/{stem}.png"
            if cfg['chapter'] == 0 and room_id not in route_ids and not any(r['game'] == cfg['name'] and r['room_id'] == room_id for r in rows):
                output = f"{cfg['slug']}/room-{room_id}.png"
            rows.append({
                'game': cfg['name'], 'kind': 'story state', 'chapter': cfg['chapter'],
                'room_id': room_id, 'title': meta['title'],
                'scene': current_scene(cfg['chapter'], room_id, meta['scene']),
                'save_file': save.relative_to(ROOT).as_posix(), 'screenshot': output,
                'trace_log': meta['trace_log'], 'dark': meta['dark'], 'capture_status': 'pending',
            })
        # Carried objects disappear from the room's nearby-object layer. Keep
        # a paired Bag-screen capture so inventory-only states are visible in
        # the gallery instead of looking identical to the empty-room frame.
        if cfg['chapter'] == 0:
            garlic_save = route_root / 'garlic-carried.sav'
            if garlic_save.exists():
                rows.append({
                    'game': cfg['name'], 'kind': 'story state', 'chapter': cfg['chapter'],
                    'room_id': 27, 'title': 'Kitchen · Carried items',
                    'scene': current_scene(cfg['chapter'], 27, 'kitchen'),
                    'save_file': garlic_save.relative_to(ROOT).as_posix(),
                    'screenshot': f"{cfg['slug']}/states/garlic-in-bag.png",
                    'trace_log': 'qa/mgba-room-gallery/route-data/zork-i-mine-audit/other-food.log',
                    'dark': 0, 'capture_status': 'pending', 'capture_view': 'bag',
                })
    else:
        # Darkness is a real state change in these story checkpoints.
        for save in sorted(route_root.glob('route-*-dark.sav'), key=lambda p: int(re.match(r'route-(\d+)-dark', p.name).group(1))):
            room_id = int(re.match(r'route-(\d+)-dark', save.name).group(1))
            # Dark-only states need their own route transcript where available;
            # the lit checkpoint may have a different route and scene metadata.
            meta = shots.get(
                f'route-{room_id}-dark',
                shots.get(f'route-{room_id}-lit', {
                    'room_id': room_id, 'scene': '', 'title': f'Room {room_id}', 'trace_log': ''
                }),
            )
            rows.append({
                'game': cfg['name'], 'kind': 'dark state', 'chapter': cfg['chapter'],
                'room_id': room_id, 'title': meta['title'],
                'scene': current_scene(cfg['chapter'], room_id, meta['scene']),
                'save_file': save.relative_to(ROOT).as_posix(), 'screenshot': f"{cfg['slug']}/states/dark-{room_id}.png",
                'trace_log': meta['trace_log'], 'dark': 1, 'capture_status': 'pending',
            })
        special = {
            'beam-before': (6, 'Beam Room', 'beam-mirror-failures.log'),
            'beam-after': (6, 'Beam Room', 'visual-state-refresh.log'),
            'junction-sword-taken': (217, 'Junction', 'visual-state-refresh.log'),
            'cliff-man': (218, 'Cliff Ledge', 'cliff-branches.log'),
            'guardians-before-potion': (54, 'Hallway', 'guardians-failures.log'),
            'hood-wounded': (30, 'Land of Shadow', 'hood-branches.log'),
            'guardians-after-potion': (54, 'Hallway', 'visual-state-refresh.log'),
            'lamp-carried': (211, 'Endless Stair', 'lamp-exhaustion.log'),
            'machine-entered': (103, 'Jewel Room', 'museum-enter-year-947.log'),
            'master-orders': (45, 'Prison Cell', 'master-prison-branches.log'),
            'mirror-initial': (160, 'Inside Mirror', 'route.log'),
            'puzzle-initial': (194, 'Royal Puzzle', 'route.log'),
            'reachable-5': (5, 'Technology Museum', 'museum-connected-route-real.log'),
            'sailor-visible': (181, 'Flathead Ocean', 'visual-state-refresh.log'),
            'underwater': (12, 'Underwater', 'route.log'),
            'drops-176': (176, 'Technology Museum', 'drop-stress.log'),
            'route-drafty-228': (228, 'Drafty Room', 'route-drafty-228.log'),
        }
        for stem, (room_id, title, trace_name) in special.items():
            save = route_root / f'{stem}.sav'
            if not save.exists():
                continue
            meta = shots.get(stem, {'dark': 0})
            trace = next((p for p in (route_root / trace_name, route_root / f'{trace_name}.log') if p.exists()), None)
            if trace is None and trace_name.endswith('.log'):
                trace = route_root / trace_name
            screenshot = f"{cfg['slug']}/states/{stem}.png"
            if stem == 'route-drafty-228':
                screenshot = f"{cfg['slug']}/room-{room_id}.png"
            rows.append({
                'game': cfg['name'], 'kind': 'story state', 'chapter': cfg['chapter'],
                'room_id': room_id, 'title': title,
                'scene': current_scene(cfg['chapter'], room_id, ''),
                'save_file': save.relative_to(ROOT).as_posix(),
                'screenshot': screenshot,
                'trace_log': trace.relative_to(ROOT).as_posix() if trace and trace.exists() else '',
                'dark': meta['dark'], 'capture_status': 'pending',
            })

        # This traced checkpoint is after the beam button and reaches Hallway
        # 169 immediately before entering Inside Mirror. Keep a dedicated
        # mGBA frame for the panel-open visual state.
        mirror_states = [
            ('mirror-hall-open', 'z3_mirror_hall_open', 'step-224.sav'),
        ]
        for stem, scene, checkpoint in mirror_states:
            save = ROOT / 'qa' / 'mgba-room-gallery' / 'route-data' / 'zork-iii-audit' / 'main-steps' / checkpoint
            if not save.exists():
                continue
            rows.append({
                'game': cfg['name'], 'kind': 'story state', 'chapter': cfg['chapter'],
                'room_id': 169, 'title': 'Hallway', 'scene': scene,
                'save_file': save.relative_to(ROOT).as_posix(),
                'screenshot': f"{cfg['slug']}/states/{stem}.png",
                'trace_log': 'qa/mgba-room-gallery/route-data/zork-iii-audit/beam-mirror-failures.log',
                'dark': 0, 'capture_status': 'pending',
            })

# Rooms no traced route stops in. qa/mgba-room-gallery/route_saves.c replays the winning-route
# fixture, walks the story's static exits (or a short recipe) to each room, confirms it is lit,
# and writes checkpoints/zork-<n>-room-<id>.sav with the command log beside it.
ROUTE_SAVE_DEFAULTS = {
    (0, 32): 'forest', (0, 167): 'forest', (0, 221): 'forest',
    (0, 102): 'cavern', (0, 145): 'cavern', (0, 166): 'z1_white_cliffs',
    (1, 95): 'z2_angled_northeast', (1, 142): 'z2_angled_northeast',
    (1, 185): 'z2_angled_northeast', (1, 226): 'z2_angled_northeast',
}


STORY_ROOM_NAMES = {}
if not all((ROOT / 'build' / 'story-dump' / f'zork{_n}-rooms.json').exists() for _n in (1, 2, 3)):
    subprocess.run(['python3', str(ROOT / 'tools' / 'dump_rooms.py')], check=True)
for _n in (1, 2, 3):
    _dump = ROOT / 'build' / 'story-dump' / f'zork{_n}-rooms.json'
    if _dump.exists():
        import json as _json
        for _room in _json.loads(_dump.read_text()):
            STORY_ROOM_NAMES[(_n - 1, _room['id'])] = _room['name']


def arrival_title(log: pathlib.Path) -> str:
    """The room name the story printed on arrival (first text line after the marker)."""
    lines = log.read_text().splitlines()
    for i, line in enumerate(lines):
        if line.startswith('# Arrived lit'):
            for text in lines[i + 1:]:
                text = text.lstrip('#').strip()
                if text and not text.startswith('('):
                    return text.split(',')[0]
    return ''
for save in sorted((OUT / 'checkpoints').glob('zork-[12]-room-*.sav')):
    game_no, room_id = (int(part) for part in re.match(r'zork-(\d)-room-(\d+)', save.stem).groups())
    chapter_index = game_no - 1
    cfg = chapters[chapter_index]
    if any(r['chapter'] == chapter_index and r['room_id'] == room_id for r in rows):
        continue
    rows.append({
        'game': cfg['name'], 'kind': 'route room', 'chapter': chapter_index,
        'room_id': room_id,
        'title': STORY_ROOM_NAMES.get((chapter_index, room_id)) or arrival_title(save.with_suffix('.txt')),
        'scene': current_scene(chapter_index, room_id, ROUTE_SAVE_DEFAULTS.get((chapter_index, room_id), 'hall (fallback)')),
        'save_file': save.relative_to(ROOT).as_posix(),
        'screenshot': f"{cfg['slug']}/room-{room_id}.png",
        'trace_log': save.with_suffix('.txt').relative_to(ROOT).as_posix(),
        'dark': 0, 'capture_status': 'pending checkpoint replay',
    })

# Death-only Zork II mist rooms are reachable through the game's sphere route.
for room_id, title, name in [
    (224, 'Room of Red Mist', 'red'),
    (227, 'Room of Blue Mist', 'blue'),
    (228, 'Room of White Mist', 'white'),
]:
    rows.append({
        'game': 'Zork II', 'kind': 'death route', 'chapter': 1,
        'room_id': room_id, 'title': title, 'scene': f'z2_mist_{name}',
        'save_file': f'qa/mgba-room-gallery/checkpoints/zork-ii-mist-{name}.sav',
        'screenshot': f'zork-ii/room-{room_id}.png',
        'trace_log': 'qa/mgba-room-gallery/checkpoints/zork-ii-mist-rooms.txt',
        'dark': 0, 'capture_status': 'pending checkpoint replay',
    })

# Checkpoint additions created from traced stable Zork III mirror and vista routes.
for room_id, title, scene, name, log in [
    (51, 'Narrow Room', 'z3_narrow_mirror_west', 'narrow-51', 'route-narrow-51.log'),
    (58, 'Narrow Room', 'z3_narrow_mirror_guardians_north', 'narrow-58', 'route-narrow-58.log'),
    (161, 'Narrow Room', 'z3_narrow_mirror_guardians_north_west', 'narrow-161', 'route-narrow-161.log'),
    (178, 'Narrow Room', 'z3_narrow_mirror_guardians_west', 'narrow-178', 'route-narrow-178.log'),
]:
    rows.append({
        'game': 'Zork III', 'kind': 'alternate route', 'chapter': 2,
        'room_id': room_id, 'title': title,
        'scene': current_scene(2, room_id, scene),
        'save_file': f"qa/mgba-room-gallery/checkpoints/zork-iii-{name}.sav",
        'screenshot': f"zork-iii/room-{room_id}.png", 'trace_log': f"qa/mgba-room-gallery/route-data/zork-iii-audit/{log}",
        'dark': 0, 'capture_status': 'pending checkpoint replay',
    })

# The frozen Ice Room is reachable before the dragon melts its glacier, but
# there is no SRAM save at that branch. Capture it by replaying the traced
# North -> Down route from room 206 in mGBA: capture-gallery.lua restores the
# room-206 save, then walks the `route` steps (Paths-menu direction 0 = north,
# 9 = down) before capturing.
rows.append({
    'game': 'Zork II', 'kind': 'story state', 'chapter': 1,
    'room_id': 161, 'title': 'Ice Room · glacier intact',
    'scene': 'z2_ice_room',
    'save_file': 'qa/mgba-room-gallery/route-data/zork-ii-finish/re-audit/route-206-lit.sav',
    'screenshot': 'zork-ii/states/ice-frozen.png',
    'trace_log': 'qa/mgba-room-gallery/route-data/zork-ii-finish/re-audit/ice-frozen-route.log',
    'dark': 0, 'capture_status': 'pending route replay', 'capture_view': 'route',
    'start_room': 206, 'route': [(0, 184), (9, 161)],
})

# Current-ROM path replays for Zork III route evidence. These start from traced
# reachable saves and use the same Paths menu as the player. The Ladder Bottom
# and Machine Room visits are transient; both narrate the room and return to
# Scenic Vista before another prompt.
z3_route_root = 'qa/mgba-room-gallery/route-data/zork-iii-audit/'
z3_routes = [
    {
        'room_id': 194, 'title': 'Royal Puzzle Entrance → Room in a Puzzle',
        'scene': 'z3_puzzle_room', 'start_room': 143, 'direction': 9,
        'start_save': 'route-143-lit.sav', 'target_room': 194,
        'screenshot': 'zork-iii/routes/royal-entrance-down.png',
        'trace': 'route-royal-puzzle-entry.txt',
    },
    {
        'room_id': 119, 'title': 'Mirror Hallway → east-mirror Narrow Room (southwest)',
        'scene': 'z3_narrow_mirror', 'start_room': 54, 'direction': 6,
        'start_save': 'route-54-lit.sav', 'target_room': 119,
        'screenshot': 'zork-iii/routes/mirror-southwest-119.png',
        'trace': 'route-mirror-exits.txt',
    },
    {
        'room_id': 221, 'title': 'Mirror Hallway → west-mirror Narrow Room (southeast)',
        'scene': 'z3_narrow_mirror_west', 'start_room': 54, 'direction': 5,
        'start_save': 'route-54-lit.sav', 'target_room': 221,
        'screenshot': 'zork-iii/routes/mirror-southeast-221.png',
        'trace': 'route-mirror-exits.txt',
    },
    {
        'room_id': 124, 'title': 'Scenic Vista after transient Ladder Bottom visit',
        'scene': 'z3_scenic_vista', 'start_room': 15, 'direction': 1,
        'start_save': 'route-15-lit.sav', 'target_room': 124,
        'screenshot': 'zork-iii/routes/ladder-transient-return.png',
        'trace': 'route-ladder-57-transient.txt',
    },
    {
        'room_id': 124, 'title': 'Scenic Vista after transient Machine Room passage',
        'scene': 'z3_scenic_vista', 'start_room': 228, 'direction': 2,
        'start_save': 'route-drafty-228.sav', 'target_room': 124,
        'screenshot': 'zork-iii/routes/machine-room-transient-return.png',
        'trace': 'route-machine-70-transient.txt',
    },
]
for route in z3_routes:
    rows.append({
        'game': 'Zork III', 'kind': 'alternate route', 'chapter': 2,
        'room_id': route['room_id'], 'title': route['title'],
        'scene': route['scene'],
        'save_file': z3_route_root + route['start_save'],
        'screenshot': route['screenshot'],
        'trace_log': z3_route_root + route['trace'],
        'dark': 0, 'capture_status': 'pending route replay', 'capture_view': 'route',
        'start_room': route['start_room'],
        'route': [(route['direction'], route['target_room'])],
    })

# Native UI checks after route restoration: restore the Royal Puzzle through
# the Pause menu, then complete Zork III from the traced prison checkpoint and
# capture its actual GBA ending screen and controller-driven ending choices.
z3_ui_script = 'qa/mgba-room-gallery/capture-z3-ui-refresh.lua'
rows.extend([
    {
        'game': 'Zork III', 'kind': 'story state', 'chapter': 2,
        'room_id': 194, 'title': 'Royal Puzzle · after in-game Restore',
        'scene': 'z3_puzzle_room',
        'save_file': z3_route_root + 'puzzle-initial.sav',
        'screenshot': 'zork-iii/states/puzzle-restored.png',
        'trace_log': z3_ui_script, 'dark': 0,
        'capture_status': 'pending UI replay', 'capture_view': 'route',
    },
    {
        'game': 'Zork III', 'kind': 'story state', 'chapter': 2,
        'room_id': 46, 'title': 'Treasury · successful ending',
        'scene': 'z3_treasury',
        'save_file': z3_route_root + 'route-204-lit.sav',
        'screenshot': 'zork-iii/states/ending-story.png',
        'trace_log': z3_route_root + 'route-z3-ending.txt', 'dark': 0,
        'capture_status': 'pending UI replay', 'capture_view': 'route',
    },
    {
        'game': 'Zork III', 'kind': 'story state', 'chapter': 2,
        'room_id': 46, 'title': 'Treasury · Restart / Restore / Quit menu',
        'scene': 'z3_treasury',
        'save_file': z3_route_root + 'route-204-lit.sav',
        'screenshot': 'zork-iii/states/ending-options.png',
        'trace_log': z3_ui_script, 'dark': 0,
        'capture_status': 'pending UI replay', 'capture_view': 'route',
    },
])

# Keep one physical-room screenshot per room; encode alternate states separately.
STATE_TITLE_OVERRIDES = {
    'cellar': 'Cellar · trap door barred from above',
    'bat-carryaway': 'Ladder Bottom · arrival after bat transport',
    'troll-defeated': 'Troll Room · troll disappears in black fog',
    'garlic-on-table': 'Kitchen · garlic visible before pickup',
    'garlic-carried': 'Kitchen · after garlic pickup',
    'garlic-in-bag': 'Kitchen · Bag open with garlic carried',
    'banker-appeared': 'Small Room · Wizard vanishes with the Black Crystal',
    'banker-timeout': 'Small Room · Gnome of Zurich arrives',
    'demon-before': 'Pentagram Room · before summoning',
    'demon-moved-menhir': 'Pentagram Room · Wizard moves the menhir',
    'cerberus-return': 'Crypt Anteroom · empty after leaving Cerberus Room',
    'princess-alert': 'Dragon’s Lair · princess leaves after awakening',
    'dead-dragon-ford': 'Deep Ford · ordinary route view',
    'master-orders': 'Prison Cell · Dungeon Master remains outside',
    'sailor-visible': 'Flathead Ocean · ship and sailor passing',
    'guardians-after-potion': 'Hallway · guardians unchanged after the vial',
    'beam-after': 'Beam Room · sword breaks the beam',
    'junction-sword-taken': 'Junction · sword is gone from the rock',
}
for row in rows:
    stem = pathlib.Path(row['screenshot']).stem
    if stem in STATE_TITLE_OVERRIDES:
        row['title'] = STATE_TITLE_OVERRIDES[stem]

# Keep one physical-room screenshot per room; encode alternate states separately.
for row in rows:
    row.setdefault('capture_view', 'room')
rows.sort(key=lambda r: (r['chapter'], {'route room': 0, 'death route': 1, 'alternate route': 1, 'dark state': 2, 'story state': 3}[r['kind']], r['room_id'], r['screenshot']))
for row in rows:
    (OUT / row['screenshot']).parent.mkdir(parents=True, exist_ok=True)
fields = ['game','kind','chapter','room_id','title','scene','save_file','screenshot','trace_log','dark','capture_status','capture_view']
with (OUT / 'manifest.csv').open('w', newline='') as f:
    w = csv.DictWriter(f, fieldnames=fields, extrasaction='ignore', lineterminator='\n'); w.writeheader(); w.writerows(rows)

lua = ['roomCapturePlan = {']
for row in rows:
    save = str(ROOT / row['save_file'])
    output = str(OUT / row['screenshot'])
    label = f"{row['game'].lower().replace(' ','-')}-{row['kind'].replace(' ','-')}-{row['room_id']}-{pathlib.Path(row['screenshot']).stem}"
    route = ''
    if row.get('route'):
        steps = ', '.join('{dir=%d, room=%d}' % step for step in row['route'])
        route = ', start_room=%d, route={%s}' % (row['start_room'], steps)
    lua.append('  {chapter=%d, room=%d, save=%s, out=%s, label=%s, capture_view=%s%s},' % (
        row['chapter'], row['room_id'], lua_string(save), lua_string(output), lua_string(label),
        lua_string('room' if row.get('capture_view') == 'route' else row.get('capture_view', 'room')), route))
lua.append('}')
(ROOT / 'build' / 'mgba-room-captures' / 'plan.lua').write_text('\n'.join(lua) + '\n')

# Generate the mGBA RAM reader from the current linker's symbols. Hard-coded
# addresses drift as the C globals change and can falsely verify screenshots
# from a previous room while displaying the expected label.
elf = ROOT / 'build' / 'trilogy.elf'
nm = os.environ.get('NM', os.environ.get('CROSS', 'arm-none-eabi-') + 'nm')
symbol_output = subprocess.check_output([nm, '-n', str(elf)], text=True)
symbols = {}
for line in symbol_output.splitlines():
    fields = line.split()
    if len(fields) >= 3 and fields[-1] in {'busy', 'chapter', 'room_object', 'screen', 'selected',
                                           'path_count', 'path_options'}:
        symbols[fields[-1]] = int(fields[0], 16)
required = {'busy', 'chapter', 'room_object', 'screen', 'selected', 'path_count', 'path_options'}
missing_symbols = required - symbols.keys()
if missing_symbols:
    raise RuntimeError(f'missing live-state symbols in {elf}: {sorted(missing_symbols)}')
read_state = [
    '-- Generated from build/trilogy.elf by build_plan.py; keep addresses in sync with the ROM.',
    'return {',
    f'  screen=function() return emu:read8(0x{symbols["screen"]:08x}) end,',
    f'  chapter=function() return emu:read32(0x{symbols["chapter"]:08x}) end,',
    f'  selected=function() return emu:read32(0x{symbols["selected"]:08x}) end,',
    f'  busy=function() return emu:read8(0x{symbols["busy"]:08x}) end,',
    f'  room=function() return emu:read32(0x{symbols["room_object"]:08x}) end,',
    f'  path_count=function() return emu:read32(0x{symbols["path_count"]:08x}) end,',
    f'  path=function(i) return emu:read8(0x{symbols["path_options"]:08x} + i) end,',
    '}',
]
(ROOT / 'build' / 'mgba-room-captures' / 'read-state.lua').write_text('\n'.join(read_state) + '\n')

print(f'Generated {len(rows)} capture records:')
for cfg in chapters:
    selected = [r for r in rows if r['game'] == cfg['name']]
    print(cfg['name'], len(selected), 'records,', len({r['room_id'] for r in selected}), 'unique saved room IDs')
