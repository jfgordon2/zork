#!/usr/bin/env python3
"""Static room/exits dumper for the bundled Z-machine v3 stories.

Reads stories/zork{1,2,3}.z3 the same way the game's interpreter does
(9-byte v3 object entries, property tables in descending property order)
and emits, for every room object, its short name, long description, and
direction exits with destination room names. Output: JSON + Markdown.

Review aid for the asset/story consistency audit, and the source of the arrival room
names qa/mgba-room-gallery/build_plan.py uses; not part of the ROM build.
Run from anywhere: python3 tools/dump_rooms.py (writes build/story-dump/).
"""
import json
import struct
from pathlib import Path

A0 = "\0\0\0\0\0\0abcdefghijklmnopqrstuvwxyz"
A1 = "\0\0\0\0\0\0ABCDEFGHIJKLMNOPQRSTUVWXYZ"
A2 = "\0\0\0\0\0\0@\n0123456789.,!?_#'\"/\\-:()"  # 6=escape, 7=newline, 8..31 punct
TABLES = (A0, A1, A2)
DIRS = [(31, "north"), (30, "east"), (29, "south"), (28, "west"), (27, "northeast"),
        (26, "northwest"), (25, "southeast"), (24, "southwest"), (23, "up"),
        (22, "down"), (21, "in"), (20, "out")]


def word(data, addr):
    return struct.unpack_from(">H", data, addr)[0]


def words_from_bytes(raw):
    """Z-chars packed three per word; stop after a word with the pad bit."""
    out = []
    for i in range(0, len(raw) - 1, 2):
        z = (raw[i] << 8) | raw[i + 1]
        out.append(z)
        if z & 0x8000:
            break
    return out


def zchars(words):
    for z in words:
        for shift in (10, 5, 0):
            yield (z >> shift) & 0x1F


class Story:
    def __init__(self, path):
        self.data = Path(path).read_bytes()
        d = self.data
        assert d[0] == 3, "expected v3 story"
        # Header layout per vendor/mojozork initStory (authoritative).
        self.objtab = word(d, 0x0A)
        self.static = word(d, 0x0E)
        self.abbrev = word(d, 0x18)
        self.objects = self._count_objects()

    def _count_objects(self):
        d, first = self.data, self.static
        row, count = self.objtab + 62, 0
        while row + 9 <= first and count < 255:
            prop = word(d, row + 7)
            if prop < row + 9:
                break
            if prop < first:
                first = prop
            count += 1
            row += 9
        return count

    def obj_row(self, oid):
        return self.objtab + 62 + 9 * (oid - 1)

    def attrs(self, oid):
        return self.data[self.obj_row(oid):self.obj_row(oid) + 4]

    def prop_table(self, oid):
        return word(self.data, self.obj_row(oid) + 7)

    def decode(self, words, allow_abbrev=True):
        """Decode a list of packed words to text (v3 rules)."""
        d = self.data
        out = []
        chars = list(zchars(words))
        i = 0
        alphabet = 0
        while i < len(chars):
            c = chars[i]
            i += 1
            if c == 0:
                out.append(" ")
                alphabet = 0
            elif c in (1, 2, 3):
                if not allow_abbrev or i >= len(chars):
                    out.append("?")
                    alphabet = 0
                    continue
                entry = 32 * (c - 1) + chars[i]
                i += 1
                abbrev_addr = 2 * word(d, self.abbrev + 2 * entry)
                # Abbreviation strings have no length byte; read pad-terminated words.
                out.append(self.decode(words_from_bytes(d[abbrev_addr:abbrev_addr + 64]),
                                       allow_abbrev=False))
                alphabet = 0
            elif c in (4, 5):
                alphabet = 1 if c == 4 else 2
            elif c == 6 and alphabet == 2:
                if i + 1 < len(chars):
                    zscii = (chars[i] << 5) | chars[i + 1]
                    i += 2
                    out.append(chr(zscii) if 32 <= zscii < 127 else
                               {13: "\n", 10: "\n"}.get(zscii, "?"))
                alphabet = 0
            else:
                table = TABLES[alphabet]
                if c < len(table) and table[c]:
                    out.append(table[c])
                alphabet = 0
        return "".join(out)

    def name(self, oid):
        addr = self.prop_table(oid)
        length = self.data[addr]
        raw = self.data[addr + 1:addr + 1 + 2 * length]
        return self.decode([struct.unpack(">H", raw[i:i + 2])[0]
                            for i in range(0, len(raw) - 1, 2)])

    def properties(self, oid):
        """Yield (number, size, raw bytes) in table order (descending)."""
        addr = self.prop_table(oid)
        length = self.data[addr]
        addr += 1 + 2 * length
        d = self.data
        while d[addr]:
            info = d[addr]
            num, size = info & 0x1F, ((info >> 5) & 0x7) + 1
            yield num, size, d[addr + 1:addr + 1 + size]
            addr += 1 + size


def main(story_path, out_json, out_md):
    story = Story(story_path)

    def room_name(rid):
        if not (1 <= rid <= story.objects):
            return f"<{rid}>"
        try:
            return story.name(rid)
        except Exception:
            return f"<obj {rid}>"

    rooms = []
    for oid in range(1, story.objects + 1):
        props = dict((num, (size, raw)) for num, size, raw in story.properties(oid))
        exits = {}
        for prop, dirname in DIRS:
            if prop in props:
                size, raw = props[prop]
                if size == 1:
                    exits[dirname] = {
                        "kind": "static",
                        "dest": raw[0],
                        "dest_name": room_name(raw[0]),
                        "raw": raw.hex(),
                    }
                else:
                    # 2-byte v3 exits in the bundled releases always have the
                    # high bit set: the destination is chosen by story code
                    # (door state, currents, random relocation), not static data.
                    exits[dirname] = {
                        "kind": "routine",
                        "dest": None,
                        "dest_name": None,
                        "raw": raw.hex(),
                    }
        if not exits:
            continue
        rooms.append({
            "id": oid,
            "name": story.name(oid),
            "attrs": story.attrs(oid).hex(),
            "exits": exits,
        })

    Path(out_json).write_text(json.dumps(rooms, indent=1))
    lines = [f"# Rooms: {Path(story_path).name}", "",
             f"{len(rooms)} room objects (any direction property 31-20 present).", ""]
    for room in rooms:
        lines.append(f"## {room['id']} · {room['name']}")
        lines.append("")
        exits = []
        for d, e in room["exits"].items():
            if e["kind"] == "routine":
                exits.append(f"{d} → (story routine; conditional)")
            else:
                exits.append(f"{d} → {e['dest']} ({e['dest_name']})")
        lines.append("- Exits: " + ("; ".join(exits) if exits else "none"))
        lines.append("")
    Path(out_md).write_text("\n".join(lines))
    print(f"{story_path}: {len(rooms)} rooms -> {out_md}")


if __name__ == "__main__":
    root = Path(__file__).resolve().parents[1]
    (root / "build/story-dump").mkdir(parents=True, exist_ok=True)
    for n in (1, 2, 3):
        main(root / f"stories/zork{n}.z3",
             root / f"build/story-dump/zork{n}-rooms.json",
             root / f"build/story-dump/zork{n}-rooms.md")
