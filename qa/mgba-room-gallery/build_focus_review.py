#!/usr/bin/env python3
"""Build a browsable contact gallery from VISUAL_QA focused-feature frames."""

import argparse
import csv
import html
import textwrap
from collections import defaultdict
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("frames", type=Path, help="frames.tsv emitted by transcript-replay-test")
    parser.add_argument("output", type=Path, help="gallery output directory")
    args = parser.parse_args()
    source_dir = args.frames.parent
    args.output.mkdir(parents=True, exist_ok=True)
    image_dir = args.output / "frames"
    image_dir.mkdir(exist_ok=True)
    sheet_dir = args.output / "sheets"
    sheet_dir.mkdir(exist_ok=True)

    rooms = defaultdict(list)
    with args.frames.open(newline="", encoding="utf-8") as stream:
        for row in csv.DictReader(stream, delimiter="\t"):
            source = source_dir / row["file"]
            output = image_dir / f"{source.stem}.png"
            if not output.exists():
                with Image.open(source) as image:
                    image.resize((image.width * 2, image.height * 2), Image.Resampling.NEAREST).save(output)
            row["png"] = output.relative_to(args.output).as_posix()
            rooms[(int(row["chapter"]), int(row["room"]))].append(row)

    sections = []
    for (chapter, room), rows in sorted(rooms.items()):
        rows.sort(key=lambda row: (int(row["object"]), row["image_hash"]))
        room_name = html.escape(rows[0]["room_name"])
        cards = []
        for row in rows:
            target_name = html.escape(row["target_name"])
            filename = html.escape(row["png"], quote=True)
            card_text = (f"Z{chapter} room {room} · {room_name} · "
                         f"target {row['object']}: {target_name} · "
                         f"box {row['box_x']},{row['box_y']} {row['box_w']}×{row['box_h']} · "
                         f"state {row['image_hash'][:8]}")
            cards.append(
                f'<figure><img loading="lazy" src="{filename}" alt="{html.escape(card_text, quote=True)}">'
                f"<figcaption>{html.escape(card_text)}</figcaption></figure>"
            )
        search_text = html.escape(f"z{chapter} {room} {rows[0]['room_name']}", quote=True)
        sections.append(
            f'<details class="room" data-search="{search_text}">'
            f"<summary>Z{chapter} · Room {room} · {room_name} ({len(rows)} frame(s))</summary>"
            f'<div class="frames">{"".join(cards)}</div></details>'
        )

        for page_number, start in enumerate(range(0, len(rows), 6), 1):
            page_rows = rows[start:start + 6]
            sheet = Image.new("RGB", (960, 570), (32, 37, 42))
            draw = ImageDraw.Draw(sheet)
            font = ImageFont.load_default()
            for index, row in enumerate(page_rows):
                x, y = (index % 2) * 480, (index // 2) * 190
                preview = Image.open(args.output / row["png"]).convert("RGB")
                sheet.paste(preview, (x, y))
                target = row["target_name"]
                label = (f"O{row['object']} {target} · box {row['box_x']},{row['box_y']} "
                         f"{row['box_w']}×{row['box_h']} · {row['image_hash'][:8]}")
                lines = textwrap.wrap(label, width=64)[:2]
                draw.multiline_text((x + 3, y + 154), "\n".join(lines), fill=(239, 230, 202), font=font, spacing=2)
            sheet_name = f"z{chapter}-r{room:03}-{page_number:02}.png"
            sheet.save(sheet_dir / sheet_name)

    page = f"""<!doctype html>
<html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width">
<title>Zork embedded-art focus review</title>
<style>
body{{font:16px system-ui,sans-serif;background:#20252a;color:#eee;margin:1rem}}
header{{position:sticky;top:0;background:#20252a;padding:.5rem 0;z-index:2}}
input{{font:inherit;padding:.5rem;width:min(32rem,90vw)}}
details.room{{border-top:1px solid #667;padding:.6rem 0}}
summary{{cursor:pointer;font-weight:700}}
.frames{{display:grid;grid-template-columns:repeat(auto-fill,minmax(490px,1fr));gap:1rem;margin:.8rem 0}}
figure{{margin:0;background:#30373d;padding:.5rem;max-width:480px}}
img{{display:block;width:100%;height:auto;image-rendering:pixelated}}
figcaption{{font-size:.82rem;line-height:1.35;margin-top:.4rem;overflow-wrap:anywhere}}
</style>
<header><h1>Embedded-art focus review</h1>
<p>{sum(map(len, rooms.values()))} unique focused feature frames across {len(rooms)} room/state groups. Gold corner boxes are from the current renderer.</p>
<input id="filter" placeholder="Filter by game, room number, or room name" aria-label="Filter rooms"></header>
{"".join(sections)}
<script>document.getElementById('filter').addEventListener('input',e=>{{const q=e.target.value.toLowerCase();document.querySelectorAll('.room').forEach(r=>r.hidden=!r.dataset.search.toLowerCase().includes(q));}});</script>
</html>"""
    (args.output / "index.html").write_text(page, encoding="utf-8")
    print(f"Wrote {sum(map(len, rooms.values()))} frame previews across {len(rooms)} rooms to {args.output}")


if __name__ == "__main__":
    main()
