# Probe-driven primary actions with result de-duplication

Date: 2026-11-20
Status: awaiting approval

## Intent

The primary action list is built from hardcoded rules (name substrings, object IDs,
attributes) and then filtered by probing. Filtering only removes; an action the rules
never propose can never appear. The goal is to make the primary list include actions
the story actually accepts, and to collapse actions that produce the same result.

## Goals

- Offer primary OBJECT actions that the story accepts with a non-refusal reply, even
  when no existing rule proposed them.
- De-duplicate actions whose probed reply (the visible result) is identical, keeping
  the most natural one.
- Keep the list short and bounded; keep `More actions...` reachable.
- Leave structured actions (target pickers, item pickers, number dials, dialogue,
  movement sentences) exactly as they are.

## Non-goals

- Replacing the category menus or the `story_filter_defaults` contract.
- Probing two-object verbs (`put ... in ...`, `unlock ... with ...`).
- Changing how a chosen action is executed.

## Approaches considered

1. Broaden the name/attribute heuristics. Cheap, but each new object still needs a
   rule and the structural miss remains.
2. A completeness menu listing every verb the story knows. No misses, but long menus
   full of no-effect entries and no ordering by usefulness.
3. Probe-driven candidates plus result de-duplication. Chosen: it directly targets
   missing effectful actions and the duplicate noise, at a bounded probe cost.

## Design

1. **Probe order.** Start with the object's existing suggested actions (the
   heuristic-derived list for that object, in its current order). Then walk the
   generic candidate table below, skipping any verb already accepted. The suggested
   actions therefore claim their de-duplication slot first.
2. **Candidate table (exact list, in order).** 32 single-noun, effectful verbs:

   | Group | Verbs |
   | --- | --- |
   | Possession | `take`, `drop` |
   | Doors and containers | `open`, `close` |
   | Reading and consumption | `read`, `eat`, `drink` |
   | Light and heat | `light`, `extinguish` |
   | Filling and mechanisms | `fill`, `wind`, `ring`, `knock` |
   | Movement | `climb`, `enter`, `cross` |
   | Manipulation | `push`, `pull`, `turn`, `lift`, `lower`, `move`, `press`, `rub`, `shake`, `touch`, `wave` |
   | Senses and searching | `search`, `smell`, `listen`, `look under`, `look in` |

   Deliberately excluded: two-object verbs (`put in/on/under`, `give`, `throw`,
   `tie`, `pour`, `spray`, `apply`) and tool verbs (`open/unlock/light/burn/cut/dig/
   attack/plug/inflate ... with`). Those stay in the category menus, where the
   second-object picker exists. `examine` is always present and needs no probe.
3. **Noun resolution.** Resolve the object's parser noun once with `examine`, reusing
   `resolve_action_noun`. Probed verbs use that noun; a rejected verb retries at most
   three shorter contiguous noun forms, then is dropped.
4. **Reply classification.** One table of refusal/no-effect patterns (e.g. "has no
   effect", "reveals nothing", "Fiddling with", "Playing in this way", "You can't do
   that", "I don't think that", "How does one", "You must tell me how", "What do you
   want to", "You're not carrying", "You don't have", "already closed", "securely
   fastened", "extremely heavy", "You find nothing unusual", "There is nothing
   special"). Actions whose normalized reply matches are not offered.
5. **De-duplication.** Normalize a reply to its first sentence, lowercased with
   whitespace collapsed. Actions sharing a normalized reply collapse to the first in
   list order: existing heuristic actions come before probed ones, and the candidate
   table supplies the tie-break among probed verbs. Example: `push`/`pull`/`press`
   and the rug's `move` all print the same sentence, so only `Move` is offered.
6. **Assembly.** Keep the structured/special actions and the existing
   `story_filter_defaults` pass for them. Append probed actions after the heuristics,
   de-duplicate the whole list, then cap probed additions so the total stays under
   `MAX_ACTIONS - 1` and `More actions...` remains last.
7. **Cost.** Probes use the existing 20,000-instruction budget. Typical cost is one
   noun resolution plus up to 30 probes when an object's menu opens. If measurement
   shows this is too slow, add a six-entry cache keyed by
   `(chapter, room, object, moves)` holding a 32-bit accepted mask and 5-bit result
   group per verb (about 170 bytes, within the ~2 KB EWRAM headroom). The design does
   not add the cache up front.

## Constraints

- Freestanding ARM build: no libc string/stdio; use the existing helpers.
- `MAX_ACTIONS` is 18; EWRAM has roughly 2 KB free.
- `z_probe` already snapshots and rewinds dynamic memory, stack and RNG, and captures
  replies without touching visual state.

## Tests

- Carpet in the Living Room: primary list contains exactly one rug-moving action and
  not both `Move` and `Push`/`Pull`/`Press`.
- Lantern: no duplicate no-effect actions; `Switch on` retained.
- For every focusable object in all three stories: list is non-empty, no two offered
  actions have identical normalized probe replies, and count stays within
  `MAX_ACTIONS - 1`.
- Replay harness: run the same invariants across the 278 room sweep.

## Risks

- Reply-pattern classification is heuristic; new phrasings need a table entry. Tests
  pin the known cases.
- De-duplication could merge two verbs with the same sentence but different effects;
  keeping the first is the accepted trade-off.
- Probe cost on GBA is the main performance risk; the cache is the fallback.
