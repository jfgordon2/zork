# Embedded Art Focus Review

**Pass:** 2026-11-23, initial survey

## Method

The scene target audit verifies that a focused item or feature has one in-bounds
box, but cannot tell whether a box surrounds the right thing. The optional
`ZORK_FOCUS_REVIEW_DIR` export captures the rendered 240×76 art band with the
current focus box. The host replay links the real compiled room backgrounds,
so these frames show the game's artwork and scene overlays. They are not full
mGBA screenshots and do not include the text/UI bands.

This pass surveyed contact sheets for all 153 captured rooms and expanded
ambiguous targets for individual review. The route replay produced 1,147
distinct feature frames. The full mGBA room gallery remains the reference for
the complete GBA screen and UI.

## Corrections

- **Zork I, Temple (room 2):** moved the Stairs focus box from the central
  entrance to the descending stair opening at the right. A replay assertion
  keeps it right of the entrance and low in the scene.
- **Zork II, Top of Well (room 8):** narrowed the Well focus region so it stays
  on the left-side well and no longer extends into the doorway. Checked its
  captured frame with the bucket states.
- **Zork III, Inside Mirror (room 160):** removed the stale Guardians target
  and its artificial focus anchor. The room description only covers the
  painted mechanism, and the parser rejects the remote Guardians noun. The
  target filter now recognizes the story's “You used the word …” parser refusal;
  a replay assertion ensures the Guardians are not focusable in this room.
- **Zork III, Inside Mirror (room 160):** moved the Stone Channel focus region
  off the long pole and onto the floor groove below the compass depression.
  Replay assertions keep this box on the floor, and keep the Well box clear of
  its doorway in future changes.

## Verification

`make test` passed after these corrections. The replay reported 1,018 audited
states, 4,427 focused targets, no missing or out-of-bounds focus boxes, and no
box-to-reported-visual mismatches. The refreshed focus gallery is generated
from the final pass under `/tmp/zork-focus-review-pass4/review/index.html`.

This is an initial visual survey, not yet a frame-by-frame review of every
alternate state. Continue by examining remaining state variants and the
full-screen mGBA captures where UI overlap or player readability matters.
