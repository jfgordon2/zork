#!/usr/bin/env python3
"""Train and report a shared 128-entry transcript vocabulary from full replay logs.

Generate corpora with:
  mkdir -p /tmp/zork-full-transcripts
  ZORK_TRANSCRIPT_DUMP_DIR=/tmp/zork-full-transcripts make test-transcripts

Then run:
  python3 tools/train_transcript_tokens.py --corpus-dir /tmp/zork-full-transcripts

The output includes a C initializer suitable for src/transcript_codec.c. Review it and
run `make test` after updating the runtime table; the full-route test enforces 40% per
game and verifies every encoded transcript entry round-trips exactly.
"""

import argparse
import ast
from collections import Counter
from pathlib import Path
import re


def load_corpora(directory: Path) -> dict[str, list[bytes]]:
    result = {}
    for game_number in range(1, 4):
        data = (directory / f"zork{game_number}.bin").read_bytes()
        assert data.endswith(b"\0"), f"Zork {game_number} corpus is not NUL-delimited"
        entries = data.split(b"\0")[:-1]
        assert entries and all(entry.isascii() for entry in entries), "Expected ASCII replay text"
        result[f"Zork {game_number}"] = entries
    return result


def load_runtime_tokens(source_path: Path) -> list[str]:
    source = source_path.read_text()
    table = source.split("static const char *const tokens[] = {", 1)[1].split("\n};", 1)[0]
    return [ast.literal_eval(f'"{value}"') for value in re.findall(r'"((?:\\.|[^"\\])*)"', table)]


def train(corpora: dict[str, list[bytes]], count: int) -> list[str]:
    sequences = [[bytes((byte,)) for byte in entry]
                 for entries in corpora.values() for entry in entries]
    learned = []
    for _ in range(count):
        frequencies = Counter()
        for sequence in sequences:
            frequencies.update(zip(sequence, sequence[1:]))
        if not frequencies:
            break
        (left, right), frequency = frequencies.most_common(1)[0]
        if frequency < 2:
            break
        merged = left + right
        learned.append(merged.decode("ascii"))
        for index, sequence in enumerate(sequences):
            replaced = []
            cursor = 0
            while cursor < len(sequence):
                if cursor + 1 < len(sequence) and sequence[cursor] == left and sequence[cursor + 1] == right:
                    replaced.append(merged)
                    cursor += 2
                else:
                    replaced.append(sequence[cursor])
                    cursor += 1
            sequences[index] = replaced
    assert len(set(learned)) == len(learned), "BPE generated duplicate token strings"
    return learned


def encoded_size(corpora: dict[str, list[bytes]], tokens: list[str]) -> dict[str, int]:
    trie = {}
    for token_id, token in enumerate(tokens):
        node = trie
        for byte in token.encode("ascii"):
            node = node.setdefault(byte, {})
        node[None] = token_id

    result = {}
    for game, entries in corpora.items():
        total = 0
        for text in entries:
            position = 0
            while position < len(text):
                node = trie
                cursor = position
                matched_end = position
                while cursor < len(text) and text[cursor] in node:
                    node = node[text[cursor]]
                    cursor += 1
                    if None in node:
                        matched_end = cursor
                if matched_end > position:
                    total += 1
                    position = matched_end
                else:
                    total += 2 if text[position] == 0x1B or text[position] >= 0x80 else 1
                    position += 1
            total += 1  # NUL delimiter between transcript entries
        result[game] = total
    return result


def c_string(value: str) -> str:
    value = value.replace("\\", "\\\\").replace('"', '\\"')
    value = value.replace("\n", "\\n").replace("\r", "\\r").replace("\t", "\\t")
    return f'"{value}"'


def report(label: str, corpora: dict[str, list[bytes]], sizes: dict[str, int]) -> None:
    raw = {game: sum(map(len, entries)) + len(entries) for game, entries in corpora.items()}
    for game in corpora:
        saved = (raw[game] - sizes[game]) * 100 // raw[game]
        print(f"{label} {game}: {raw[game]:,} -> {sizes[game]:,} bytes ({saved}% saved)")
    raw_total = sum(raw.values())
    compressed_total = sum(sizes.values())
    saved_total = (raw_total - compressed_total) * 100 // raw_total
    print(f"{label} combined: {raw_total:,} -> {compressed_total:,} bytes ({saved_total}% saved)")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--corpus-dir", required=True, type=Path)
    parser.add_argument("--tokens", default=128, type=int)
    args = parser.parse_args()

    corpora = load_corpora(args.corpus_dir)
    runtime_source = Path(__file__).resolve().parents[1] / "src/transcript_codec.c"
    current = load_runtime_tokens(runtime_source)
    learned = train(corpora, args.tokens)
    print(f"Current runtime vocabulary ({len(current)} tokens):")
    report("  ", corpora, encoded_size(corpora, current))
    print(f"Trained byte-pair vocabulary ({len(learned)} tokens):")
    report("  ", corpora, encoded_size(corpora, learned))
    print("C initializer:")
    for token in learned:
        print(f"    {c_string(token)},")


if __name__ == "__main__":
    main()
