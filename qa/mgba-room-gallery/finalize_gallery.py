#!/usr/bin/env python3
"""Validate mGBA screenshots and produce the browsable gallery/index."""
import csv
import hashlib
import html
import pathlib
import struct
from collections import defaultdict, Counter

ROOT = pathlib.Path(__file__).resolve().parents[2]
GALLERY = ROOT / 'qa' / 'mgba-room-gallery'
ROM = ROOT / 'zork-trilogy.gba'
rom_sha256 = hashlib.sha256(ROM.read_bytes()).hexdigest()
manifest_path = GALLERY / 'manifest.csv'
results_path = GALLERY / 'capture-results.csv'
with manifest_path.open(newline='') as f:
    rows = list(csv.DictReader(f))

full_capture_results = {}
if results_path.is_file():
    with results_path.open(newline='') as f:
        for result in csv.DictReader(f):
            full_capture_results[result['screenshot']] = result

# Targeted refreshes overlay a full pass only when their report is newer. A
# later complete pass supersedes any targeted captures it covered.
targeted_results = {}
targeted_result_mtimes = {}
refresh_paths = [
    GALLERY / 'cavern-refresh-results.csv',
    GALLERY / 'mirror-state-refresh-results.csv',
    GALLERY / 'state-cue-refresh-results.csv',
    GALLERY / 'inventory-refresh-results.csv',
    GALLERY / 'garlic-refresh-results.csv',
    GALLERY / 'followup-refresh-results.csv',
    GALLERY / 'well-refresh-results.csv',
    GALLERY / 'red-mist-refresh-results.csv',
    GALLERY / 'troll-refresh-results.csv',
    GALLERY / 'guarded-open-refresh-results.csv',
    GALLERY / 'door-pair-refresh-results.csv',
    GALLERY / 'ice-frozen-refresh-results.csv',
    GALLERY / 'cliff-man-refresh-results.csv',
    GALLERY / 'boat-interior-refresh-results.csv',
    GALLERY / 'art-followup-refresh-results.csv',
    GALLERY / 'z3-routes-refresh-results.csv',
    GALLERY / 'z3-ui-refresh-results.csv',
    GALLERY / 'story-art-refresh-results.csv',
    GALLERY / 'connected-art-refresh-results.csv',
    GALLERY / 'great-door-cleft-refresh-results.csv',
]
existing_refresh_paths = [path for path in refresh_paths if path.is_file()]
for refresh_path in existing_refresh_paths:
    report_mtime = refresh_path.stat().st_mtime
    with refresh_path.open(newline='') as f:
        for result in csv.DictReader(f):
            screenshot = result['screenshot']
            if report_mtime >= targeted_result_mtimes.get(screenshot, 0):
                targeted_results[screenshot] = result
                targeted_result_mtimes[screenshot] = report_mtime
full_mtime = results_path.stat().st_mtime if results_path.is_file() else 0
full_pass_matches_current_rom = results_path.is_file() and results_path.stat().st_mtime_ns >= ROM.stat().st_mtime_ns
newer_targeted_results = {
    screenshot: result for screenshot, result in targeted_results.items()
    if targeted_result_mtimes[screenshot] > full_mtime
}
capture_results = dict(full_capture_results)
for screenshot, result in targeted_results.items():
    # If the full pass includes this frame, only a later targeted run may
    # replace it. If the full pass did not include it, keep the last known
    # targeted result even when that report is older.
    if screenshot not in full_capture_results or screenshot in newer_targeted_results:
        capture_results[screenshot] = result

missing = []
bad = []
seen = Counter(r['screenshot'] for r in rows)
for r in rows:
    image_path = GALLERY / r['screenshot']
    result = capture_results.get(r['screenshot'])
    if not result:
        r['capture_status'] = 'not verified in last full mGBA run'
        continue
    if result['status'] != 'captured':
        actual = result.get('actual_room', '')
        detail = f" (emulator reached room {actual})" if actual else ''
        r['capture_status'] = result['status'] + detail
        continue
    if not image_path.is_file():
        missing.append(r['screenshot'])
        r['capture_status'] = 'missing'
        continue
    data = image_path.read_bytes()
    if data[:8] != b'\x89PNG\r\n\x1a\n' or len(data) < 24:
        bad.append(r['screenshot'])
        r['capture_status'] = 'invalid PNG'
        continue
    width, height = struct.unpack('>II', data[16:24])
    if (width, height) != (240, 160):
        bad.append(f"{r['screenshot']} ({width}x{height})")
        r['capture_status'] = f'invalid dimensions {width}x{height}'
    else:
        r['capture_status'] = 'captured'

with manifest_path.open('w', newline='') as f:
    writer = csv.DictWriter(f, fieldnames=list(rows[0]), lineterminator='\n')
    writer.writeheader()
    writer.writerows(rows)

by_game = defaultdict(list)
for row in rows:
    by_game[row['game']].append(row)
room_groups = {}
for game, game_rows in by_game.items():
    groups = defaultdict(list)
    for row in game_rows:
        groups[int(row['room_id'])].append(row)
    room_groups[game] = groups

# Call out exact repeated PNGs inside one story room. Some records are useful
# aliases, but a named state that resolves to the same rendered frame should
# be visible as such in the gallery instead of implying a visual change.
duplicate_rows = defaultdict(list)
frame_groups = defaultdict(list)
for game, game_rows in by_game.items():
    for row in game_rows:
        if row['capture_status'] != 'captured':
            continue
        image_path = GALLERY / row['screenshot']
        if not image_path.is_file():
            continue
        frame_hash = hashlib.sha256(image_path.read_bytes()).hexdigest()
        frame_groups[(game, int(row['room_id']), frame_hash)].append(row)
for same_frame in frame_groups.values():
    if len(same_frame) < 2:
        continue
    for row in same_frame:
        duplicate_rows[row['screenshot']] = [
            pathlib.Path(other['screenshot']).stem.replace('-', ' ')
            for other in same_frame if other is not row
        ]

# Build a compact but fully browsable static page, grouped by story room.
def href(path):
    return html.escape(path, quote=True)
def trace_href(path):
    if not path:
        return ''
    return html.escape('../../' + path, quote=True)

def card(game, room_id, group):
    # Prefer a verified route frame. For branch-only rooms use the verified
    # room-ID frame, then fall back to another verified state.
    main = next((r for r in group if r['kind'] == 'route room' and r['capture_status'] == 'captured'), None)
    if main is None:
        main = next((r for r in group if pathlib.Path(r['screenshot']).name == f'room-{room_id}.png' and r['capture_status'] == 'captured'), None)
    if main is None:
        main = next((r for r in group if r['capture_status'] == 'captured'), group[0])
    variants = [r for r in group if r is not main]
    title = html.escape(main['title'])
    scene = html.escape(main['scene'])
    link = trace_href(main['trace_log'])
    trace = f'<a class="trace" href="{link}">trace</a>' if link else ''
    out = [f'<article class="room" data-search="{html.escape((game + " " + str(room_id) + " " + main["title"] + " " + main["scene"]).lower(), quote=True)}">']
    out.append(f'<h3>#{room_id} · {title}</h3>')
    if scene:
        out.append(f'<p class="scene">Scene art: <code>{scene}</code> {trace}</p>')
    if main['capture_status'] == 'captured':
        out.append(f'<a class="screen" href="{href(main["screenshot"])}"><img loading="lazy" src="{href(main["screenshot"])}" alt="{html.escape(game)} · {title} · room {room_id}"></a>')
    else:
        out.append(f'<div class="empty">No verified screenshot in the last full capture run. Status: {html.escape(main["capture_status"])}</div>')
    dark_tag = ' · dark' if main['dark'] == '1' and main['kind'] != 'dark state' else ''
    main_duplicates = duplicate_rows.get(main['screenshot'], [])
    repeated_main = (' · exact same frame as ' + ', '.join(main_duplicates)) if main_duplicates else ''
    out.append(f'<p class="caption">{html.escape(main["kind"])}{dark_tag} · {main["capture_status"]}{html.escape(repeated_main)}</p>')
    if variants:
        out.append('<details><summary>%d additional state%s</summary><div class="states">' % (len(variants), '' if len(variants) == 1 else 's'))
        for row in variants:
            stem = pathlib.Path(row['screenshot']).stem
            state_labels = {
                'garlic-on-table': 'before pickup · garlic visible on the table',
                'garlic-carried': 'after pickup · garlic absent from the room',
                'garlic-in-bag': 'Bag open · carried clove listed',
            }
            label = state_labels.get(stem, stem.replace('-', ' '))
            caption = label if stem in state_labels else f'{row["title"]} · {label} state'
            state_trace = trace_href(row['trace_log'])
            state_link = f'<a href="{state_trace}">route</a>' if state_trace else ''
            state_duplicates = duplicate_rows.get(row['screenshot'], [])
            repeated_state = (' · exact same frame as ' + ', '.join(state_duplicates)) if state_duplicates else ''
            if row['capture_status'] == 'captured':
                out.append(f'<figure><a href="{href(row["screenshot"])}"><img loading="lazy" src="{href(row["screenshot"])}" alt="{html.escape(game)} · {html.escape(caption)}"></a><figcaption>{html.escape(caption)} · {html.escape(row["kind"])}{html.escape(repeated_state)} {state_link}</figcaption></figure>')
            else:
                out.append(f'<figure class="empty">{html.escape(caption)} — {html.escape(row["capture_status"])} {state_link}</figure>')
        out.append('</div></details>')
    out.append('</article>')
    return '\n'.join(out)

sections = []
for game in ('Zork I', 'Zork II', 'Zork III'):
    groups = room_groups[game]
    verified_ids = {int(r['room_id']) for r in by_game[game] if r['capture_status'] == 'captured'}
    sections.append(f'<section id="{game.lower().replace(" ", "-")}"><h2>{game} <small>{len(verified_ids)} verified room IDs · {len(by_game[game])} records</small></h2><div class="rooms">')
    for room_id in sorted(groups):
        sections.append(card(game, room_id, groups[room_id]))
    sections.append('</div></section>')

index = '''<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Zork Trilogy mGBA Room Gallery</title>
<style>
:root{color-scheme:dark;--bg:#111714;--panel:#1c2520;--line:#39473d;--gold:#f4c779;--text:#eee4cf;--dim:#aebba9}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:15px/1.45 system-ui,sans-serif}
header{position:sticky;top:0;background:#111714ee;border-bottom:1px solid var(--line);padding:14px clamp(16px,4vw,48px);z-index:2;backdrop-filter:blur(6px)}
header h1{margin:0 0 8px;font-size:21px;color:var(--gold)}header p{margin:0;color:var(--dim)}nav{display:flex;gap:14px;flex-wrap:wrap;margin-top:8px}a{color:var(--gold)}input{background:var(--panel);border:1px solid var(--line);color:var(--text);padding:9px 12px;border-radius:5px;width:min(420px,90vw)}main{padding:18px clamp(12px,3vw,36px)}section{margin:28px 0 52px}h2{margin:0 0 14px;color:var(--gold)}h2 small{font-size:13px;color:var(--dim);font-weight:400;margin-left:8px}.rooms{display:grid;grid-template-columns:repeat(auto-fill,minmax(300px,1fr));gap:12px}.room{background:var(--panel);border:1px solid var(--line);border-radius:7px;padding:12px;min-width:0}.room h3{font-size:16px;margin:0 0 2px}.scene,.caption{font-size:12px;color:var(--dim);margin:2px 0 8px}.trace{margin-left:8px;font-size:11px}.screen img{display:block;width:100%;height:auto;image-rendering:pixelated;background:#000;border:1px solid #58634d}.caption{margin:6px 0 0}details{margin-top:8px;border-top:1px solid var(--line);padding-top:8px}summary{cursor:pointer;color:var(--gold);font-size:13px}.states{display:grid;grid-template-columns:repeat(auto-fill,minmax(240px,1fr));gap:8px;margin-top:8px}figure{margin:0;min-width:0}figure img{width:100%;height:auto;image-rendering:pixelated;border:1px solid #58634d;display:block}figcaption{font-size:11px;color:var(--dim);margin:4px 0 8px}footer{color:var(--dim);padding:32px;text-align:center}code{color:#d4dfc8} .empty{padding:12px;color:var(--dim)}
</style></head><body><header><h1>Zork Trilogy · mGBA room and state gallery</h1>
<p>{len(rows)} native 240×160 screenshot records from the current cartridge, grouped by traced story room. Only frames verified against the emulator's live chapter and room ID are shown.</p>
<input id="filter" type="search" placeholder="Filter rooms…"><nav><a href="#zork-i">Zork I</a><a href="#zork-ii">Zork II</a><a href="#zork-iii">Zork III</a><a href="manifest.csv">CSV manifest</a><a href="capture-results.csv">mGBA capture report</a><a href="route-gaps.md">Route gap report</a></nav></header><main>
'''
index += '\n'.join(sections)
index += '''</main><footer>Each card links to the original mGBA PNG. The manifest records the save checkpoint and trace log used for it.</footer>
<script>const filter=document.querySelector('#filter');filter.addEventListener('input',()=>{const q=filter.value.trim().toLowerCase();document.querySelectorAll('.room').forEach(card=>{card.hidden=!!q&&!card.dataset.search.includes(q);});});</script></body></html>'''
(GALLERY / 'index.html').write_text(index)

# Route evidence disposition for rooms without a stable screenshot.
gaps = '''# Traced room gaps and capture verification

The gallery displays a screenshot only when the current ROM reached the manifest's expected room ID in mGBA. The first table lists rooms whose traced route is transient, fatal, or unsafe. The checkpoint summary reports whether route saves restored to their expected rooms; any failed restores are listed separately.

| Room | Finding | Route evidence |
| --- | --- | --- |
| Ladder Top (20) | No safe, stable route found. The declared ladder-top exit is not actionable on the transient visit to Ladder Bottom. | [Zork III route coverage](../../docs/audits/zork-iii-route-coverage.md); [ladder route probe](route-data/zork-iii-audit/route-ladder-57-transient.txt) |
| Narrow Room (48) | Guardian-side mirror branch is a trap; opening the relevant panel exposes the player to the Guardians and leads to death, not a stable arrival. | [Zork III route coverage](../../docs/audits/zork-iii-route-coverage.md); [guardian trap route](route-data/zork-iii-audit/route-narrow-guardians-trap.txt) |
| Ladder Bottom (57) | Reached only as narration during travel from Scenic Vista, then returns to the Vista before a player prompt. | [transient ladder route](route-data/zork-iii-audit/route-ladder-57-transient.txt) |
| Machine Room (70) | The southward passage from Drafty Room narrates the Machine Room and returns to Scenic Vista before a separate prompt. | [transient machine route](route-data/zork-iii-audit/route-machine-70-transient.txt) |
| Murky Room (Zork II, 237) | Fatal in both tank states: entering the intact aquarium feeds you to the serpent, and entering the broken one cuts you fatally. No stable arrival exists. | [aquarium entry probe](checkpoints/zork-2-room-237-unreachable.txt) |
| Mine Dead End (138) | No safe, stable route found in the traced paths; the transient ladder visit does not expose the exit as a separate move. | [Zork III route coverage](../../docs/audits/zork-iii-route-coverage.md); [ladder route probe](route-data/zork-iii-audit/route-ladder-57-transient.txt) |
| Narrow Room (151) | Guardian-side mirror branch is a trap; the recorded route reaches the Guardians and death instead of a stable room prompt. | [Zork III route coverage](../../docs/audits/zork-iii-route-coverage.md); [guardian trap route](route-data/zork-iii-audit/route-narrow-guardians-trap.txt) |
| Sacrificial Altar (210) | View IV at Scenic Vista triggers fatal altar narration and revival at Endless Stair; no stable altar prompt. | [Zork III route coverage](../../docs/audits/zork-iii-route-coverage.md); [Vista route traces](route-data/zork-iii-audit/vista-destinations.log) |
| Prison Cell (225) | Appears during Guardian-death narration; the player receives no prompt before revival. | [Zork III route coverage](../../docs/audits/zork-iii-route-coverage.md); [guardian death route](route-data/zork-iii-audit/route-narrow-guardians-death.txt) |

The Zork III route notes classify these as transient, fatal, or trap outcomes; the Zork II Murky Room was probed directly. They also document stable paths to side rooms 51, 58, 161, 178 and Drafty Room 228. Zork II's Red, Blue, and White Mist rooms are death-recovery destinations; the gallery includes them using the traced Riddle Room → darkness/grue recovery → westward sphere path.

No stable room route was missing from the Zork I or Zork II screenshot set. The Zork I alternate grating route contributes Clearing (160) and Grating Room (216) screenshots. The Zork II route set adds its three death-only sphere rooms to the 77 regular route checkpoints.
'''
unverified_z3 = []
for row in rows:
    result = capture_results.get(row['screenshot'])
    if row['game'] == 'Zork III' and result and result['status'] != 'captured':
        unverified_z3.append((row, result))

def md_cell(value):
    return str(value or '').replace('|', '\\|').replace('\n', ' ')

if capture_results:
    verified_z3 = sum(
        r['capture_status'] == 'captured'
        for r in rows if r['game'] == 'Zork III'
    )
    full_pass_rows_for_gaps = [r for r in rows if r.get('capture_view') != 'route']
    full_pass_screenshots_for_gaps = {r['screenshot'] for r in full_pass_rows_for_gaps}
    full_pass_verified_for_gaps = sum(
        result['status'] == 'captured'
        for screenshot, result in full_capture_results.items()
        if screenshot in full_pass_screenshots_for_gaps
    )
    route_only_verified = sum(
        r.get('capture_view') == 'route' and r['capture_status'] == 'captured'
        for r in rows
    )
    pass_summary = (
        f"The last full gallery pass verified {full_pass_verified_for_gaps} of "
        f"{len(full_pass_rows_for_gaps)} save-restoration captures"
    )
    if route_only_verified:
        pass_summary += f", plus {route_only_verified} route-only frame{' was' if route_only_verified == 1 else 's were'} verified by route replay"
    pass_summary += '.'
    if not full_pass_matches_current_rom:
        pass_summary += ' The last completed full pass predates the current ROM build, so the full gallery still needs a refresh against this build.'
    if unverified_z3:
        audit_summary = (
            f"{pass_summary} {len(unverified_z3)} Zork III entries did not reach "
            "their expected room in mGBA. These are **checkpoint restore failures**, not evidence "
            "that each destination is unreachable. The route traces remain evidence of story paths, "
            "and the gallery hides unverified PNGs until an mGBA replay captures them in the correct room."
        )
    else:
        audit_summary = (
            f"{pass_summary} All {verified_z3} Zork III entries reached their expected "
            "room ID in mGBA."
        )
    z3_capture_audit = [
        '',
        '## Last full mGBA checkpoint verification',
        '',
        audit_summary,
    ]
    if unverified_z3:
        z3_capture_audit.extend([
            '',
            '| Expected room | Story title | Capture kind | Checkpoint file | mGBA room | Route trace |',
            '| ---: | --- | --- | --- | ---: | --- |',
        ])
    for row, result in unverified_z3:
        trace = row['trace_log']
        trace_cell = f"[{trace}](../../{trace})" if trace else '—'
        z3_capture_audit.append(
            f"| {row['room_id']} | {md_cell(row['title'])} | {md_cell(row['kind'])} | `{md_cell(row['save_file'])}` | {md_cell(result.get('actual_room')) or '—'} | {trace_cell} |"
        )
    gaps += '\n' + '\n'.join(z3_capture_audit) + '\n'
(GALLERY / 'route-gaps.md').write_text(gaps)

counts = {
    game: (
        len({int(r['room_id']) for r in by_game[game] if r['capture_status'] == 'captured'}),
        len(by_game[game]),
        sum(r['capture_status'] == 'captured' for r in by_game[game]),
    )
    for game in ('Zork I','Zork II','Zork III')
}
verified = sum(r['capture_status'] == 'captured' for r in rows)
unverified = len(rows) - verified
targeted_verified = sum(result['status'] == 'captured' for result in newer_targeted_results.values())
full_pass_rows = [r for r in rows if r.get('capture_view') != 'route']
full_pass_screenshots = {r['screenshot'] for r in full_pass_rows}
full_pass_verified = sum(
    result['status'] == 'captured'
    for screenshot, result in full_capture_results.items()
    if screenshot in full_pass_screenshots
)
capture_summary = f"{full_pass_verified} of {len(full_pass_rows)} save-restoration screenshots were verified in the last completed full mGBA gallery pass."
if not full_pass_matches_current_rom:
    capture_summary += " That pass predates the current ROM build; rerun the full gallery to verify those captures against the current build."
route_replayed = sum(
    r.get('capture_view') == 'route' and r['capture_status'] == 'captured'
    for r in rows
)
if route_replayed:
    capture_summary += f" {route_replayed} route-only frame{' was' if route_replayed == 1 else 's were'} verified by route replay."
if newer_targeted_results:
    current_targeted_verified = sum(
        result['status'] == 'captured'
        and targeted_result_mtimes.get(screenshot, 0) >= ROM.stat().st_mtime
        for screenshot, result in newer_targeted_results.items()
    )
    noun = 'entry' if current_targeted_verified == 1 else 'entries'
    verb = 'was' if current_targeted_verified == 1 else 'were'
    capture_summary += (f" {current_targeted_verified} targeted {noun} {verb} captured after the current ROM build; "
                        "the gallery uses those current frames.")
elif targeted_results:
    capture_summary += " The full pass supersedes the older targeted refresh captures."
if unverified:
    noun = 'entry' if unverified == 1 else 'entries'
    verb = 'is' if unverified == 1 else 'are'
    possessive = 'its card shows the restore result.' if unverified == 1 else 'their cards show the restore result.'
    capture_summary += f" {unverified} {noun} {verb} hidden because the emulator did not reach the expected room; {possessive}"
readme = f'''# Zork Trilogy mGBA Room Gallery

A route-based index of actual mGBA screenshots from `zork-trilogy.gba`. Each PNG is the emulator's native **240 × 160** frame, including the GBA UI and story text. Most screenshots are taken after restoring a traced save and confirming the current chapter and room; route-only states are captured by replaying their traced input path and confirming the live room. Explicit Bag captures wait for the inventory screen before saving. Targeted visual refreshes take precedence over older frames for the entries they recapture.

ROM SHA-256: `{rom_sha256}`

## Browse

- [Interactive room and state gallery](index.html)
- [Complete CSV manifest](manifest.csv)
- [Last full mGBA checkpoint verification results](capture-results.csv)
- [Earlier targeted cavern art refresh report](cavern-refresh-results.csv)
- [Targeted Hallway 169 open-panel state refresh](mirror-state-refresh-results.csv)
- [Boat, menhir and cage state refresh](state-cue-refresh-results.csv)
- [Inventory state refresh](inventory-refresh-results.csv)
- [Well tether state refresh](well-refresh-results.csv)
- [Red Mist story-dependent dark/visible states](red-mist-refresh-results.csv)
- [Defeated Troll disappearance state](troll-refresh-results.csv)
- [Frozen Ice Room pre-dragon route](ice-frozen-refresh-results.csv)
- [Cliff Ledge upper-shelf character refresh](cliff-man-refresh-results.csv)
- [Shared boat interior refresh across five aboard states](boat-interior-refresh-results.csv)
- [Zork III alternate route captures](z3-routes-refresh-results.csv)
- [Zork III Restore and ending UI captures](z3-ui-refresh-results.csv)
- [Attic, fallen dragon, and Water Slide refresh](story-art-refresh-results.csv)
- [Connected art and story-state refresh](connected-art-refresh-results.csv)
- [Great Door earthquake states](great-door-cleft-refresh-results.csv)
- [Zork III route coverage notes](../../docs/audits/zork-iii-route-coverage.md)
- [Rooms without stable screenshots and route evidence](route-gaps.md)

| Chapter | Room IDs with verified frame | Capture records | Verified PNGs |
| --- | ---: | ---: | ---: |
| Zork I | {counts['Zork I'][0]} | {counts['Zork I'][1]} | {counts['Zork I'][2]} |
| Zork II | {counts['Zork II'][0]} | {counts['Zork II'][1]} | {counts['Zork II'][2]} |
| Zork III | {counts['Zork III'][0]} | {counts['Zork III'][1]} | {counts['Zork III'][2]} |
| **Total** | **{sum(c[0] for c in counts.values())}** | **{len(rows)}** | **{verified}** |

The manifest identifies each room/state, its source checkpoint or route replay, the trace log or branch script that establishes the path, and the PNG path. Named state captures preserve variants such as doors, lights, puzzle geometry, water levels, vehicle placement, character states, and dropped/carried items. The shared boat interior was recaptured in all five saved aboard states after the current ROM build. Exact duplicate PNGs within a room are labeled in the gallery; several are aliases for one story moment rather than separate visual states.

Zork III's route notes include stable mirror-room branches, the Royal Puzzle entrance, and transient or unsafe outcomes such as Ladder Bottom 57 and the Guardian-side mirror rooms. The native route refresh records reachable branches in mGBA, and a separate UI replay captures in-game Restore plus the successful ending and its controller-driven Restart / Restore / Quit menu. {('The latest full mGBA pass confirmed all Zork III room and state checkpoints in their expected rooms.' if not unverified_z3 and full_pass_matches_current_rom else 'The last completed full mGBA pass confirmed all Zork III checkpoints then present in their expected rooms; that pass predates the current ROM build.' if not unverified_z3 else 'The latest mGBA report identifies Zork III checkpoints that restored to a different room.')} The three Zork II sphere rooms (224, 227, 228) were generated from the death-recovery route in `checkpoints/zork-ii-mist-rooms.txt`.

Hallway 169's post-button route and state frames use the open-panel artwork. The story's global 107 records this panel state; it is not represented by the Z-machine `OPEN` attribute.

## Inputs and scripts

Every save and trace log the plan uses is committed under [route-data/](route-data/) (the original audit harness sources are in each folder's `generators/`), alongside the [checkpoints/](checkpoints/) saves built by `route_saves.c`. To refresh: `python3 qa/mgba-room-gallery/refresh_screenshots.py prepare`, run the printed command in mGBA's Scripting window, then `... finalize`. For a named subset, set `galleryRefreshSet` and load `refresh-sets.lua`.

To review focus boxes around artwork embedded in backgrounds, set
`ZORK_FOCUS_REVIEW_DIR` to an existing directory when running
`build/transcript-replay-test`. The optional export captures distinct rendered
room art frames with their focused feature box. Convert them into a browsable
review page with `python3 qa/mgba-room-gallery/build_focus_review.py
/path/to/frames.tsv /path/to/review-gallery`.

## Capture result

{capture_summary} Each capture is accepted only after the live chapter and room ID match the manifest. The frozen Ice Room frame has no destination save, so its plan record restores the pre-dragon room 206 checkpoint and `capture-gallery.lua` replays North → Down through the Paths menu before capturing (run it alone with `galleryRefreshSet='ice-frozen'` and `refresh-sets.lua`). [route-gaps.md](route-gaps.md) lists transient/fatal rooms and reports checkpoint restore results.
'''
(GALLERY / 'README.md').write_text(readme)

print('entries', len(rows), 'missing', len(missing), 'bad', len(bad), 'duplicate outputs', sum(n-1 for n in seen.values() if n>1))
print('unique rooms', {k: v[0] for k,v in counts.items()})
print('last-full-run capture status', Counter(r['capture_status'] for r in rows))
if missing or bad:
    print('missing:', missing[:20])
    print('bad:', bad[:20])
