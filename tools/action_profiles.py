#!/usr/bin/env python3
"""Compile route-audited menus into object-wide and state-specific verb masks."""

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
    object_masks = {}
    state_masks = {}
    state_row_masks = {}
    conflicting_states = set()
    vertical_route_masks = {}
    with open(source, newline="", encoding="utf-8") as stream:
        for row in csv.reader(stream, delimiter="\t"):
            if len(row) < 7:
                raise ValueError("profile row is missing key fields")
            game, room, obj = int(row[0]), int(row[1]), int(row[2])
            parent, carried, attributes = int(row[3]), int(row[4]), int(row[5], 16)
            count = int(row[6])
            if (game >= 3 or room > 65535 or obj > 65535 or parent > 255 or
                    carried not in (0, 1) or attributes > 0xFFFFFFFF):
                raise ValueError(f"profile key is outside mask range: {row[:6]}")
            action_end = 7 + count * 4
            if len(row) not in (action_end, action_end + 2):
                raise ValueError(f"profile for game {game}, object {obj} has a bad action count")
            route_mask = None
            if len(row) == action_end + 2:
                if row[action_end] != "VROUTE":
                    raise ValueError(f"profile for game {game}, object {obj} has unknown metadata")
                route_mask = int(row[action_end + 1], 0)
                if route_mask & ~0x03:
                    raise ValueError(f"profile for game {game}, object {obj} has an invalid vertical route mask")
            verb_mask = 0
            for at in range(7, action_end, 4):
                verb, kind = row[at + 1], int(row[at + 3])
                if kind != 0:  # DO_OBJECT actions only; authored literal forms stay in the base rules.
                    continue
                if verb in probe_verbs:
                    verb_mask |= 1 << probe_verbs.index(verb)
            object_key = (game, obj)
            object_masks[object_key] = object_masks.get(object_key, 0) | verb_mask
            state_key = (game, room, obj, parent, carried, attributes)
            state_masks[state_key] = state_masks.get(state_key, 0) | verb_mask
            previous_mask = state_row_masks.get(state_key)
            if previous_mask is not None and previous_mask != verb_mask:
                conflicting_states.add(state_key)
            else:
                state_row_masks[state_key] = verb_mask
            if route_mask is not None:
                # Several audited routes may reach the same structural key
                # through different hidden story flags. Only retain directions
                # observed to work in every occurrence of that key.
                vertical_route_masks[state_key] = (
                    vertical_route_masks[state_key] & route_mask
                    if state_key in vertical_route_masks else route_mask
                )

    lines = [
        "/* Generated from tests/fixtures/action-profiles.tsv by tools/action_profiles.py. */",
        "typedef struct { uint16_t game, object; uint32_t verbs; } ActionCandidateMask;",
        "typedef struct {",
        "    uint16_t game, room, object;",
        "    uint8_t parent, carried;",
        "    uint32_t attributes, verbs;",
        "} ActionCandidateStateMask;",
        "typedef struct {",
        "    uint16_t game, room, object;",
        "    uint8_t parent, carried, directions;",
        "    uint32_t attributes;",
        "} ActionVerticalRouteMask;",
        "static const ActionCandidateMask action_candidate_masks[] = {",
    ]
    for (game, obj), verbs in sorted(object_masks.items()):
        lines.append(f"    {{{game}, {obj}, 0x{verbs:08x}u}},")
    lines.extend(["};", "static const ActionCandidateStateMask action_candidate_state_masks[] = {"])
    for (key, verbs) in sorted(state_masks.items()):
        game, room, obj, parent, carried, attributes = key
        # A repeated structural key with different observed candidate menus
        # implies hidden story state. Widen that exact key to the object's
        # observed superset; runtime still probes every candidate verb.
        if key in conflicting_states:
            verbs = object_masks[(game, obj)]
        lines.append(
            f"    {{{game}, {room}, {obj}, {parent}, {carried}, "
            f"0x{attributes:08x}u, 0x{verbs:08x}u}},"
        )
    lines.extend(["};", "static const ActionVerticalRouteMask action_vertical_route_masks[] = {"])
    for key, directions in sorted(vertical_route_masks.items()):
        game, room, obj, parent, carried, attributes = key
        lines.append(
            f"    {{{game}, {room}, {obj}, {parent}, {carried}, {directions}, "
            f"0x{attributes:08x}u}},"
        )
    if not vertical_route_masks:
        lines.append("    {0, 0, 0, 0, 0, 0, 0},")
    lines.extend([
        "};",
        f"#define ACTION_VERTICAL_ROUTE_MASK_COUNT {len(vertical_route_masks)}u",
        "",
    ])
    pathlib.Path(destination).write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        raise SystemExit("usage: action_profiles.py INPUT.tsv OUTPUT.inc VERBS.def")
    main(sys.argv[1], sys.argv[2], sys.argv[3])
