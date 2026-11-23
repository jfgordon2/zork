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
    # The host test links on both macOS (Mach-O) and Linux (ELF). Keep the
    # generated assembly portable: Mach-O needs __DATA,__const and a leading
    # underscore; ELF needs .rodata and no underscore.
    lines = [
        "#ifdef __APPLE__",
        ".section __DATA,__const",
        "#define ART_SYM(x) _##x",
        "#else",
        ".section .rodata",
        "#define ART_SYM(x) x",
        "#endif",
    ]
    for name in scene_names():
        asset = ROOT / "build" / f"{name}.rgb555"
        if not asset.is_file():
            raise SystemExit(f"missing compiled scene art: {asset}")
        relative = asset.relative_to(ROOT).as_posix()
        lines += [
            ".p2align 2",
            f".globl ART_SYM(art_{name})",
            f"ART_SYM(art_{name}):",
            f'.incbin "{relative}"',
        ]
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote host scene assembly for {len(scene_names())} backgrounds to {output}")


if __name__ == "__main__":
    main()
