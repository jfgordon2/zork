#!/usr/bin/env python3
"""Compile route-audited menus into per-object candidate-verb masks."""

import csv
import pathlib
import re
import sys


def read_probe_verbs(path):
    verbs = []
    pattern = re.compile(r'PROBE_VERB\("([^"\\]*)", "([^"\\]*)"\)')
    for line in pathlib.Path(path).read_text(encoding="utf-8").splitlines():
        match = pattern.fullmatch(line.strip())
        if not match:
            raise ValueError(f"invalid probe verb declaration: {line}")
        verbs.append(match.group(2))
    if len(verbs) != 32 or len(set(verbs)) != len(verbs):
        raise ValueError("the candidate verb mask must contain 32 unique verbs")
    return verbs


def main(source, destination, verb_def):
    probe_verbs = read_probe_verbs(verb_def)
    masks = {}
    with open(source, newline="", encoding="utf-8") as stream:
        for row in csv.reader(stream, delimiter="\t"):
            if len(row) < 7:
                raise ValueError("profile row is missing key fields")
            game, obj = int(row[0]), int(row[2])
            count = int(row[6])
            if game >= 3 or obj > 65535:
                raise ValueError(f"profile game/object is outside mask range: {game}/{obj}")
            if len(row) != 7 + count * 4:
                raise ValueError(f"profile for game {game}, object {obj} has a bad action count")
            verb_mask = masks.get((game, obj), 0)
            for at in range(7, len(row), 4):
                verb, kind = row[at + 1], int(row[at + 3])
                if kind != 0:  # DO_OBJECT actions only; authored literal forms stay in the base rules.
                    continue
                if verb in probe_verbs:
                    verb_mask |= 1 << probe_verbs.index(verb)
            masks[(game, obj)] = verb_mask

    lines = [
        "/* Generated from tests/fixtures/action-profiles.tsv by tools/action_profiles.py. */",
        "typedef struct { uint16_t game, object; uint32_t verbs; } ActionCandidateMask;",
        "static const ActionCandidateMask action_candidate_masks[] = {",
    ]
    for (game, obj), verbs in sorted(masks.items()):
        lines.append(f"    {{{game}, {obj}, 0x{verbs:08x}u}},")
    lines.extend(["};", ""])
    pathlib.Path(destination).write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise SystemExit("usage: action_profiles.py INPUT.tsv OUTPUT.inc VERBS.def")
    main(sys.argv[1], sys.argv[2], sys.argv[3])
