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

## Primary action probing — November 20, 2026

The primary action list is now probe-driven rather than rule-only. `story_filter_defaults()`
first probes the object's existing suggested actions, then walks a fixed table of 32
single-noun verbs (`take`, `drop`, `open`, `close`, `read`, `eat`, `drink`, `light`,
`extinguish`, `fill`, `wind`, `ring`, `knock on`, `climb`, `enter`, `cross`, `push`,
`pull`, `turn`, `lift`, `lower`, `move`, `press`, `rub`, `shake`, `touch`, `wave`,
`search`, `smell`, `listen to`, `look under`, `look in`). Two-object and tool verbs stay
in the category menus.

- Replies are classified by a no-effect pattern table (`action_reply_is_no_effect`), so
  actions that only acknowledge the verb are not offered.
- Actions whose complete replies and observed VM-state effect fingerprints match are collapsed,
  keeping the earliest: suggested actions first, then table order. Deduplication tests
  compare full replies and effect fingerprints, not first sentences alone.
- "Not available here" replies (`You can't see any ...`) never group actions.
- `rub` and `touch` are probed only for objects with the LIGHT attribute: Zork I's
  RUB/TOUCH routine crashes (`Object id #0 referenced`) on objects such as the brass
  bell.
- `climb_action()` now verifies both directions instead of trusting the generic climb
  reply, which is misleading on Zork III's endless stair.
- `z_probe_last_line()` records the last probe command for test diagnostics.

Tests: `check_reply_classification`, `check_primary_enrichment`, and
`check_primary_dedup`; the replay audits now compare action visibility against the
no-effect rule and accept an action collapsed by de-duplication when another offered
action has the same reply. `make test` passes and the ARM build leaves 1,980 bytes of
EWRAM free.

### Final-review follow-ups (same day)

The whole-branch review found that applying the no-effect table to `Examine` removed
it whenever the object description contained phrases like "is closed" or "securely
fastened" (206 of 7,360 menu builds). `Examine` is now always offered, never
classified as no-effect, and still claims its reply for de-duplication so a duplicate
`Look inside` is collapsed.

Also from the review:

- The refusal table gained the common Zork phrasings the first pass missed (`you
  already have`, `a valiant attempt`, `isn't notably helpful`, `you can't move the`,
  `securely anchored`, `you can't turn that`, `contortion`, `accomplished enough
  juggler`, `doesn't seem to work`, `you can't open that`, `not openable`).
- Probe-time story faults are now recoverable. `z_probe` guards execution with a small
  naked-asm setjmp/longjmp (ARMv7 for the GBA, AArch64 for the host); a story bug such
  as Zork I's RUB on the brass bell unwinds, rewinds and reports the probe as rejected
  instead of resetting the game. Verified by disabling the rub/touch guard and running
  the full sweep: it passes.
- `probe_action_form` now treats a timeout or crashed probe as rejected rather than
  accepted, so partial output is never offered.
- Reply normalization skips leading punctuation (the boat label starts `!!!!`).
- The menu cap is enforced after de-duplication, including when `More actions...` is
  present.
- Noun-form fallback tries each individual word plus the final two words.
- The replay default-menu audit now asserts `Examine` is present, the menu is last,
  the count fits `MAX_ACTIONS - 1`, and no two offered actions share a full reply and
  effect signature. The filtered-action counter no longer underflows when enrichment
  adds actions.

### Follow-up to the probe review

The action-menu probe path now has a total budget of 48 probe attempts per menu
build, including probes needed to choose a climb direction. The menu is built
synchronously, so this prevents the old worst case of up to 128 candidate-form
probes from blocking input indefinitely.

Noun fallback tries each individual word, then the final two words, so it reaches
headwords in the middle of longer display nouns.
Descriptive smell replies such as "It smells like ..." are no longer classified
as no-effect responses.

Duplicate detection compares the entire captured reply and persistent dynamic
state. After capturing the action reply, the probe runs `inventory` in its
rewindable scratch branch to settle parser work tables; that output is suppressed
and the live game is restored afterward. The READ input and parse buffers, plus
the parser's `PRSA` and `P-TABLE` words in the bundled Zork I release 119 build,
are excluded from the state fingerprint. Other globals and story memory remain
included, so actions with matching text but different game effects stay separate.
The reply capture is 512 bytes so long outcomes such as the ghost's curse can
still be compared in full; replies that fill the capture buffer remain unique
because they may be truncated. Probe recovery marks the architecture-specific trampoline
`returns_twice`, supports ARM, AArch64, and SysV x86-64 host builds, and fails
compilation on unsupported ABIs instead of silently omitting recovery.

### Route-audit fixes and verification — November 20, 2026

The full route replay found three parser and test edge cases:

- Noun fallback now tries every individual word. This lets the visible Zork II flask
  resolve as `flask` when its full display name and surrounding words are rejected by
  the story grammar. A noun found while probing `Examine` is reused for the other
  primary-action probes in that menu build.
- Multiword targets are checked against `Examine` before being exposed as focus targets.
  If neither the complete name nor any individual word addresses the object, the focus
  target is omitted. This removes the unaddressable `rock structure` target in Zork
  III's Crystal Grotto.
- The refusal "can't see how to get in from here" is treated as an unavailable action,
  removing invalid Open/Enter suggestions for the white house.

The story tests no longer require unique first sentences. They assert uniqueness only
for matching full replies and persistent effect fingerprints, and use
phrase-construction checks when a test target is remote from the current room. The
Zork I rug regression checks that `move`, `push`, and `pull` collapse to one primary
action, while `press` remains because the VM reports a distinct state effect despite
the same visible reply. This guards both sides of de-duplication: remove repeated
choices while retaining a different outcome.

Verification: `make test` passes all three story replays, including 751 default-action
menus and 3,870 direct action commands. The replays report zero reference-state drifts.
`make zork-trilogy.gba` also builds successfully with the ARM7TDMI toolchain.

### Primary action-menu latency — November 21, 2026

Opening an Act menu previously paid the full state-effect cost for every candidate:
each `z_probe_with_effect()` ran the action, ran a canonical `inventory` command, then
scanned dynamic story memory to fingerprint changes. The menu now probes each candidate
reply with the cheaper `z_probe()` path and fingerprints effects only when two full
replies match. Those effect comparisons are capped at 24 per menu; uncertain comparisons
keep both actions, so the cap cannot hide a distinct outcome. The menu also reuses the
already-failed full noun when trying shorter noun forms.

This preserves the carpet regression: Move/Push/Pull collapse because both their full
replies and effects match, while Press remains because its effect differs despite the
same reply. `make test` passes all three route replays and their menu, parser, and action
family audits. Across the 751 audited menus, no menu needed more than 16 effect probes;
the ARM7TDMI ROM rebuild also succeeds. This reduces work by avoiding the extra inventory
execution and full-memory scan for every non-duplicate candidate.

### Generated candidate masks with live-state probes — November 21, 2026

The exact-menu lookup prototype was replaced because its room/object/attribute key could
miss hidden story flags. The current design generates a 32-bit candidate-verb mask per
game/object from interpreter-verified menu results, then probes only those verbs against
the current story state when the player opens Act. Room conditions and hidden flags now
affect the actual probe replies; the mask only limits which verbs are considered. The
candidate verb names are shared between C and the generator through
`src/action_probe_verbs.def`.

The profile fixture now includes full-route menus plus open/closed and climb states. QA
compares masked menus action-for-action with the full 32-verb sweep across 1,014 audited
states. They match at a 5,000-instruction probe budget: 7,114 masked attempts versus
31,482 full-sweep attempts, with maxima of 17 versus 35 per state. A 3,000-instruction
budget failed this comparison in Zork II, so it was rejected. `z_probe_batch_begin()` also
saves the interpreter snapshot once per menu batch; each candidate restores from that
baseline instead of recopying it before every probe. Effect comparisons remain capped at
24 per menu (the audited maximum is 16).

Run `build/transcript-replay-test --dump-action-profiles tests/fixtures/action-profiles.tsv`
to refresh the source data after changing story action behavior, then run `make test`.
The tests catch newly discovered verbs on the audited routes and state families; states
outside that audit can still need more mask coverage. `make test` and the ARM7TDMI ROM
build pass. Live mGBA latency is still unverified because the emulator is stuck in its
script console after an earlier timed-out QA script.
