# Probe-driven primary actions Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the primary action menu offer the effectful actions the story accepts, and collapse actions whose probed replies are identical.

**Architecture:** Keep the existing heuristic action list as the first pass, then probe a fixed table of single-noun verbs through `z_probe`, drop refusals and no-effect replies, append accepted actions before `More actions...`, and de-duplicate by normalized reply keeping the earliest action. All work lives in `src/interaction.c` behind the existing `story_filter_defaults()` entry point, so callers do not change.

**Tech Stack:** C11, freestanding ARM (devkitARM), no libc; host test build via `make test`; existing `z_probe` oracle in `src/zmachine.c`.

**Spec:** `docs/superpowers/specs/2026-11-20-probe-driven-primary-actions-design.md`

## Global Constraints

- Freestanding build: no `<string.h>`/`<stdio.h>` in `src/`; use `append_text`, `text_has`, `text_equal`.
- `MAX_ACTIONS` is 18; the list must keep `More actions...` last and stay at or below `MAX_ACTIONS - 1`.
- EWRAM has about 2 KB free; do not add a cache in v1.
- Probe budget is 20000 instructions per `z_probe` call.
- Do not change `story_actions()` behavior for non-PRIMARY groups.
- The repository currently has no commit history. Do not create the first commit; finish each task by confirming `git status --short` lists only the files the task touched, and record the change in the audit note.

## Review Focus

- A probe that times out returns an empty reply: it must never be offered as effectful, and empty replies must not collide during de-duplication.
- `More actions...` must remain the last row and the count must stay within `MAX_ACTIONS - 1` after enrichment.
- Structured actions (target pickers, dials, dialogue, movement) must survive unchanged; only `DO_OBJECT` actions are enriched, filtered, or merged.
- Two different verbs can print the same sentence with different effects; keeping the first is accepted, but the choice must be deterministic (suggested actions before probed ones, then table order).
- Noun-form fallback must stay bounded (three shorter forms per probed verb) so a menu open cannot fan out into hundreds of probes.

---

### Task 1: Reply normalization and no-effect classification

**Files:**
- Modify: `src/interaction.h` (add two prototypes)
- Modify: `src/interaction.c` (add two functions and one pattern table)
- Test: `tests/story_test.c` (add `check_reply_classification`, call it first in `main`)

**Interfaces:**
- Consumes: `text_has()` from `src/chapter.h`.
- Produces:
  - `void action_reply_normalized(const char *reply, char *out, unsigned capacity);`
  - `bool action_reply_is_no_effect(const char *reply);`

- [ ] **Step 1: Write the failing test**

Add to `tests/story_test.c`:

```c
static void check_reply_classification(void) {
    char out[64];
    action_reply_normalized("With a great effort, the rug is moved to one side.", out, sizeof(out));
    assert(!strcmp(out, "with a great effort, the rug is moved to one side"));
    action_reply_normalized("Pushing the brass lantern has no effect.\nMore text.", out, sizeof(out));
    assert(!strcmp(out, "pushing the brass lantern has no effect"));
    action_reply_normalized("(with the sword) A clean cut.", out, sizeof(out));
    assert(!strcmp(out, "a clean cut"));
    assert(action_reply_is_no_effect("Pushing the brass lantern has no effect."));
    assert(action_reply_is_no_effect("You find nothing unusual."));
    assert(action_reply_is_no_effect("The rug is too heavy to lift, but in trying to do so you notice a trap door.") == false);
    assert(action_reply_is_no_effect("The rug is moved to one side, revealing a closed trap door.") == false);
}
```

Call `check_reply_classification();` as the first statement of `main()`.

- [ ] **Step 2: Run the test to verify it fails**

Run: `make test`
Expected: link error `undefined reference to 'action_reply_normalized'` (test cannot build).

- [ ] **Step 3: Add prototypes and implement**

In `src/interaction.h` add:

```c
void action_reply_normalized(const char *reply, char *out, unsigned capacity);
bool action_reply_is_no_effect(const char *reply);
```

In `src/interaction.c` add one pattern table and the two functions. Behavior:

- `action_reply_normalized`: skip leading spaces; if the text starts with `(`, skip through the first `)` and following spaces; copy until `.`, `!`, `?` or `\n`; lowercase `A`-`Z`; collapse runs of spaces to one; trim trailing spaces; NUL-terminate; never exceed capacity.
- `action_reply_is_no_effect`: return true if the normalized reply contains any of:
  `has no effect`, `reveals nothing`, `Fiddling with`, `Playing in this way`, `You can't do that`, `I don't think that`, `How does one`, `You must tell me how`, `What do you want to`, `You're not carrying`, `You don't have`, `already closed`, `already open`, `securely fastened`, `extremely heavy`, `cannot be carried`, `You find nothing unusual`, `There is nothing special`, `isn't sleeping`, `I'd sooner kiss`, `That's pretty weird`, `You have lost your mind`, `There is nothing to fill`, `is it empty`.
  Use `text_has` (case-insensitive) on the normalized string.

- [ ] **Step 4: Run the test to verify it passes**

Run: `make test`
Expected: `check_reply_classification` passes and the rest of the suite passes.

- [ ] **Step 5: Verify both builds**

Run: `make test && make zork-trilogy.gba`
Expected: both succeed; `git status --short` lists only `src/interaction.h`, `src/interaction.c`, `tests/story_test.c`.

---

### Task 2: Probe-driven enrichment of the primary list

**Files:**
- Modify: `src/interaction.c` (probe table, enrich inside `story_filter_defaults`)
- Test: `tests/story_test.c` (add `check_primary_enrichment`)

**Interfaces:**
- Consumes: `action_reply_normalized`, `action_reply_is_no_effect` (Task 1); `resolve_action_noun(action, object, value, chosen, reply)` (existing static).
- Produces: `story_filter_defaults(unsigned id, Action list[MAX_ACTIONS], unsigned count)` keeps its signature and now appends probed actions before `More actions...`.

- [ ] **Step 1: Write the failing test**

Add to `tests/story_test.c`:

```c
static void check_primary_enrichment(void) {
    // Living Room (75) holds the carpet (55), lantern (146), sword (227), trophy case (197).
    command("n"); command("e"); command("w"); command("w");
    assert(z_global(0) == 75);
    Action actions[MAX_ACTIONS];
    unsigned count = story_actions(0, 55, false, PRIMARY, actions);
    count = story_filter_defaults(55, actions, count);
    assert(count <= MAX_ACTIONS - 1);
    bool extra = false;
    for (unsigned i = 0; i < count; i++)
        if (!strcmp(actions[i].verb, "push") || !strcmp(actions[i].verb, "pull") ||
            !strcmp(actions[i].verb, "press")) extra = true;
    assert(extra); // The story moves the rug for these verbs; the old rules never offered them.
}
```

Call it in `main()` after `start(0)` and before the existing checks that navigate.

- [ ] **Step 2: Run the test to verify it fails**

Run: `make test`
Expected: assertion `extra` fails (carpet list is only Examine, Look under, Move, More actions).

- [ ] **Step 3: Implement enrichment**

In `src/interaction.c`:

1. Add the ordered table from the spec (32 entries), each `{const char *label, *verb;}`:
   `Take/take`, `Drop/drop`, `Open/open`, `Close/close`, `Read/read`, `Eat/eat`,
   `Drink/drink`, `Light/light`, `Extinguish/extinguish`, `Fill/fill`, `Wind/wind`,
   `Ring/ring`, `Knock on/knock on`, `Climb/climb`, `Enter/enter`, `Cross/cross`,
   `Push/push`, `Pull/pull`, `Turn/turn`, `Lift/lift`, `Lower/lower`, `Move/move`,
   `Press/press`, `Rub/rub`, `Shake/shake`, `Touch/touch`, `Wave/wave`, `Search/search`,
   `Smell/smell`, `Listen to/listen to`, `Look under/look under`, `Look inside/look in`.
2. In `story_filter_defaults(id, list, count)`:
   - Remember whether the last entry is the `SHOW_CATEGORIES` menu and remove it from the working count.
   - Filter the existing list as today: for `DO_OBJECT`, `resolve_action_noun`; drop on rejection (`parser_rejected`) or `reply_disallows_action` or `action_reply_is_no_effect` on the reply. Keep other kinds unchanged.
   - Append probed actions for each table entry whose verb is not already present in the surviving list: build `Action{label, verb, NULL, DO_OBJECT}`, call `resolve_action_noun` (which already bounds noun retries to the full name plus contiguous sub-phrases), and keep it only if a non-empty reply came back and `!parser_rejected && !reply_disallows_action && !action_reply_is_no_effect`.
   - Re-append `More actions...` as the last entry if it was present.
   - Stop appending once `count == MAX_ACTIONS - 1`.

Keep `resolve_action_noun` unchanged. Empty replies (probe timeout) must be treated as rejected for probed entries.

- [ ] **Step 4: Run the tests to verify they pass**

Run: `make test`
Expected: `check_primary_enrichment` passes; all prior checks pass.

- [ ] **Step 5: Verify the ARM build**

Run: `make zork-trilogy.gba`
Expected: builds; `git status --short` lists `src/interaction.c`, `tests/story_test.c`.

---

### Task 3: Result de-duplication, list cap, and broad audit

**Files:**
- Modify: `src/interaction.c` (dedup and cap in `story_filter_defaults`)
- Test: `tests/story_test.c` (add `check_primary_dedup`)
- Test: `tests/transcript_replay_test.c` (extend `audit_default_action_menu`)

**Interfaces:**
- Consumes: everything from Tasks 1–2.
- Produces: no new API; `story_filter_defaults` guarantees no two `DO_OBJECT` entries share a normalized reply.

- [ ] **Step 1: Write the failing test**

Add to `tests/story_test.c`:

```c
static void check_primary_dedup(void) {
    command("n"); command("e"); command("w"); command("w");
    assert(z_global(0) == 75);
    Action actions[MAX_ACTIONS];
    unsigned count = story_actions(0, 55, false, PRIMARY, actions);
    count = story_filter_defaults(55, actions, count);
    unsigned rug_moves = 0;
    char normalized[MAX_ACTIONS][48];
    unsigned replies = 0;
    for (unsigned i = 0; i < count; i++) {
        if (!strcmp(actions[i].verb, "push") || !strcmp(actions[i].verb, "pull") ||
            !strcmp(actions[i].verb, "press") || !strcmp(actions[i].verb, "move")) rug_moves++;
        if (actions[i].kind != DO_OBJECT && actions[i].kind != DO_LITERAL) continue;
        char line[COMMAND_SIZE], reply[192];
        assert(action_command(actions[i], 55, NULL, line));
        assert(z_probe(line, reply, sizeof(reply), 20000));
        action_reply_normalized(reply, normalized[replies], sizeof(normalized[replies]));
        assert(*normalized[replies]); // offered actions must have a reply
        for (unsigned j = 0; j < replies; j++) assert(strcmp(normalized[j], normalized[replies]));
        replies++;
    }
    assert(rug_moves == 1);
}
```

Call it in `main()` after `check_primary_enrichment`.

- [ ] **Step 2: Run the test to verify it fails**

Run: `make test`
Expected: `assert(rug_moves == 1)` fails because `Move`, `Push`, `Pull`, and `Press` all survive Task 2.

- [ ] **Step 3: Implement de-duplication and the cap**

In `story_filter_defaults`:

- After filtering and appending, walk the surviving `DO_OBJECT` entries in order. For each, compute `action_reply_normalized` from the reply captured during its probe (store replies in a local `char replies[MAX_ACTIONS][48]` alongside the list). Drop an entry whose normalized reply is empty or equal to an earlier surviving entry's normalized reply.
- Keep non-`DO_OBJECT` entries in place and never de-duplicate them.
- Enforce the cap by dropping the last surviving `DO_OBJECT` entry until `count <= MAX_ACTIONS - 1`; if `More actions...` was present, restore it last.

Do not probe twice: reuse the reply obtained in the filter/append pass.

- [ ] **Step 4: Extend the replay audit**

In `tests/transcript_replay_test.c`, inside `audit_default_action_menu`, after the existing per-action probe loop:

- Collect each `DO_OBJECT`/`DO_LITERAL` action's normalized reply (declare a local `char seen[MAX_ACTIONS][48]`), assert no duplicates.
- Assert `count <= MAX_ACTIONS - 1`.
- Assert the last entry, when a `SHOW_CATEGORIES` action exists, is `SHOW_CATEGORIES`.

- [ ] **Step 5: Run the full suite**

Run: `make test`
Expected: all pass, including `check_primary_dedup` and the room sweep; the printed counts remain sane.

- [ ] **Step 6: Verify the ARM build and memory**

Run: `make zork-trilogy.gba && arm-none-eabi-size build/trilogy.elf`
Expected: builds; `.bss` stays below 262,144 bytes with at least 1,500 bytes free.

- [ ] **Step 7: Record the change**

Add a short section to `docs/audits/parser-noun-fix.md` (or a new dated note linked from the README) naming the probe table, the reply-pattern table, the de-duplication rule, and the new tests. Confirm `git status --short` lists only the files this task touched.

---

## Self-Review

- **Spec coverage:** probe order (Task 2), exact 32-verb table (Task 2), reply classification (Task 1), de-duplication (Task 3), cap/MENU ordering (Tasks 2–3), cost bound (noun retries reused from `resolve_action_noun`, Task 2), tests (Tasks 1–3).
- **Type consistency:** `action_reply_normalized(const char*, char*, unsigned)`, `action_reply_is_no_effect(const char*)`, and the unchanged `story_filter_defaults(unsigned, Action[MAX_ACTIONS], unsigned)` are used consistently in all tasks.
- **Review Focus coverage:** timeout/empty replies (Tasks 2–3), MENU last and cap (Task 3), structured actions untouched (Tasks 2–3), deterministic tie-break (Task 3 order), bounded noun fallback (Task 2 reuses existing bounded resolver).
