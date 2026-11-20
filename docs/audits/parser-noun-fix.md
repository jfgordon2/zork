# Parser noun normalization — November 20, 2026

## Problem

Interacting with objects whose short name contains a token the story's dictionary
doesn't carry produced `I don't know the word "..."`. Example: the bird's nest in
Zork I has short name `bird's nest`, but `bird's` is not a dictionary word, so
`examine bird's nest` was rejected before the story could act on it. A dictionary
survey found 62/85/53 distinct unknown tokens in item names across Zork I/II/III,
affecting roughly 150 objects per story.

## Fix

- `src/chapter.c`: added `story_parser_noun(object, buffer)`. It starts from the
  existing `story_noun()` alias/name, then keeps only tokens the story's own
  dictionary recognizes. The VM exposes a check that reuses the interpreter's
  exact six-Z-character encoding and punctuation separators, instead of
  approximating them from ASCII character counts. Punctuation-joined words,
  suffixes, and unknown adjectives are dropped. If no token is known, the result
  is empty and `add_object()` omits that object from focus and carried targets
  rather than presenting actions the story cannot parse. This removes Zork II's
  generic `Arcane item` proxy; the room still describes the scattered objects
  on the workbench.
- `src/interaction.c` and `src/trilogy.c`: the three command-building paths now
  use `story_parser_noun()` instead of `story_noun()`. Display and action matching
  still use `story_noun()`, so object labels and menu logic are unchanged.
- The local-scenery availability probe also uses the normalized noun, so a
  punctuation-bearing scenery name does not bypass the same parser rules.
  `z_probe()` (src/zmachine.c) runs that probe safely: it submits `examine <noun>`
  with a 20,000-instruction budget, then rewinds via `z_load()` to the saved PC,
  dynamic memory, stack and RNG. Probe text is captured inside zmachine.c and
  never reaches `story_text()`/`visual_state_observe()`, so the narration a probe
  triggers cannot mutate visual state. Menu latency scales with the number of
  scenery candidates per room; the probe is bounded and no drift was observed in
  the route sweeps.
- Dictionary-word lookups use a small 16-slot direct-mapped cache for repeated
  target names, invalidated on every `z_start()`. Cache collisions only repeat
  the exact VM lookup. The cross-story sweep catches stale results even when a
  caller reuses the same story buffer for the next chapter.

The reported bird's-nest case is handled by the generic normalizer; its temporary
one-off alias was removed so the regression test exercises the general behavior.

## Verification (TDD)

- `check_parser_nouns()` exercises generated examine nouns against the actual
  interpreter for named, parented objects in all three stories and checks their
  dictionary tokens. It reports zero parser rejections. The only empty noun is
  Zork II's Arcane Item proxy, which is excluded from target lists. The regression test
  confirms `Bird's nest` normalizes to `nest` without a one-off alias.
- The route replay checks 1,018 changing screen states and 4,531 focused targets
  for focus-box placement, plus 46 dropped-item focus states. A separate sweep
  visits all 278 room definitions across the three stories. Combined route and
  room sweeps cover 11,447 visible/carried target appearances and 515 distinct
  target names; each generated name is accepted by the actual interpreter.
- `make test` passes in full, including all three winning routes.
- `make zork-trilogy.gba` builds with the freestanding ARM toolchain.
- The rebuilt ROM's EWRAM `.bss` is 260,160 of 262,144 bytes, leaving 1,984
  bytes free.
- The replay's `No scene box: Z3 room 45 ... Dungeon master` line is a known visual
  gap unrelated to noun parsing; it remains in the QA checklist.

## Scope note

The filter guarantees that generated noun tokens are accepted by the dictionary,
but it does not prove every shortened noun resolves to the intended entity when
two objects share the remaining words. Story routes and interaction outcome
tests are still needed for identity and behavior; a correct dictionary token by
itself is not proof of a correct target.
