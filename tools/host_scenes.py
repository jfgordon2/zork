#!/usr/bin/env python3
"""Emit a host-linkable Mach-O assembly object for the compiled room art."""

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def scene_names():
    names = re.findall(r"^SCENE\s+(\w+)\s*$", (ROOT / "src/scenes.S").read_text(), re.M)
    names += re.findall(r"^ZORK1_SCENE\((\w+)\)", (ROOT / "src/zork1_scenes.def").read_text(), re.M)
    names += ["z2_" + name for name in re.findall(
        r"^ZORK2_SCENE\((\w+)\s*,", (ROOT / "src/zork2_scenes.def").read_text(), re.M
    )]
    return list(dict.fromkeys(names))


def main():
    if len(sys.argv) != 2:
        raise SystemExit("usage: host_scenes.py OUTPUT.S")
    output = Path(sys.argv[1])
    lines = [".section __DATA,__const"]
    for name in scene_names():
        asset = ROOT / "build" / f"{name}.rgb555"
        if not asset.is_file():
            raise SystemExit(f"missing compiled scene art: {asset}")
        relative = asset.relative_to(ROOT).as_posix()
        lines += [".p2align 2", f".globl _art_{name}", f"_art_{name}:", f'.incbin "{relative}"']
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote host scene assembly for {len(scene_names())} backgrounds to {output}")


if __name__ == "__main__":
    main()
