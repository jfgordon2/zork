# SDD ledger — plan: /Users/jeff/GitHub/zork/docs/superpowers/plans/2026-11-20-probe-driven-primary-actions.md
Setup: Ruling: no git history (all files untracked, no commits) — execute in place and track the ledger manually; skip worktree/commit scripts — cost if wrong: none, no branch to isolate.
Pre-flight: Task 1 produces action_reply_normalized/action_reply_is_no_effect consumed by Tasks 2 and 3; names match. Task 2 extends story_filter_defaults, Task 3 extends it again sequentially; no conflict.
Task 1: complete (tests: make test → pass; make zork-trilogy.gba → builds). Added action_reply_normalized/action_reply_is_no_effect + pattern table; unit test check_reply_classification.
Task 2: Ruling: probing surfaced a latent story crash ("Object id #0 referenced") that aborted the app — made probe-time fatals recoverable via __builtin_setjmp/longjmp in zmachine.c so an invalid probe is treated as rejected instead of fatal — cost if wrong: probes that would crash the story are silently dropped (acceptable; they were unreachable from menus before).
Task 2: Ruling: extended no-effect patterns beyond the plan's list (is closed, is locked, how exactly, why knock, your bare hands, an interesting idea, can't cross, can't look inside, smells like, hear nothing, nothing but dust, nothing happens, you wave the) and updated the replay test's visibility oracle to include the no-effect rule — cost if wrong: a few informative-but-effectless actions are hidden.
Task 3: complete (tests: make test → pass; make zork-trilogy.gba → builds, .bss 260,164/262,144). Dedup by normalized first sentence, cap MAX_ACTIONS-1, MENU last. TDD: dedup test observed failing before implementation.
Task 3: Ruling: dedup ignores "not available here" replies (you can't see any / you're not at / there is no) so out-of-room menu builds keep their actions; cost if wrong: duplicates could remain for inaccessible objects (not user-visible; menus only open for visible targets).
Task 3: Ruling: updated the replay visibility oracles (assert_candidate_filter, audit_enter_exit_menu) to judge viability and accept dedup-collapsed actions; cost if wrong: audit could accept an action hidden for the wrong reason (the reply-equality check limits this).
Final review: fixed Critical/Important via failing tests where feasible:
- Examine removed (Critical): check_examine_always_offered RED→GREEN; Examine bypasses no-effect and participates in dedup.
- Junk refusals (Important): check_carried_item_menu RED→GREEN; patterns extended.
- Probe crash recovery (Important): implemented naked setjmp/longjmp per arch; validated by disabling the rub/touch guard and running full sweep (passes). probe_action_form treats failed probes as rejected.
- Minor 5/6/7/8 fixed: punctuation normalization, strict cap, 3-form fallback, replay audit assertions + counter underflow.
Final: Ruling: kept the improved climb_action for the MOVE group (non-PRIMARY) — the direction verification fixes a misleading default there too — cost if wrong: MOVE-group climb defaults differ from the old heuristic.
Final: Ruling: no probe cache — worst case is 32 verbs × up to 4 noun forms per menu open; tests pass and hardware latency is unmeasured — cost if wrong: a perceptible menu-open pause on GBA.
Final: Ruling: keep the ledger workspace (no git history exists to preserve it) — cost if wrong: an extra untracked scratch directory.
Final: minor (deferred): refusal-pattern table remains heuristic; new story phrasings may still offer a no-effect action.
Final: minor (deferred): probe-driven menus add ~1-4 KB of stack and up to 128 probes per open on paper; no cache.
