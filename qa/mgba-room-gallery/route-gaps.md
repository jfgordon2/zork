# Traced room gaps and capture verification

The gallery displays a screenshot only when the current ROM reached the manifest's expected room ID in mGBA. The first table lists rooms whose traced route is transient, fatal, or unsafe. The checkpoint summary reports whether route saves restored to their expected rooms; any failed restores are listed separately.

| Room | Finding | Route evidence |
| --- | --- | --- |
| Ladder Top (20) | No safe, stable route found. The declared ladder-top exit is not actionable on the transient visit to Ladder Bottom. | [Zork III route coverage](../../zork-iii-route-coverage.md); [ladder route probe](route-data/zork-iii-audit/route-ladder-57-transient.txt) |
| Narrow Room (48) | Guardian-side mirror branch is a trap; opening the relevant panel exposes the player to the Guardians and leads to death, not a stable arrival. | [Zork III route coverage](../../zork-iii-route-coverage.md); [guardian trap route](route-data/zork-iii-audit/route-narrow-guardians-trap.txt) |
| Ladder Bottom (57) | Reached only as narration during travel from Scenic Vista, then returns to the Vista before a player prompt. | [transient ladder route](route-data/zork-iii-audit/route-ladder-57-transient.txt) |
| Machine Room (70) | The southward passage from Drafty Room narrates the Machine Room and returns to Scenic Vista before a separate prompt. | [transient machine route](route-data/zork-iii-audit/route-machine-70-transient.txt) |
| Murky Room (Zork II, 237) | Fatal in both tank states: entering the intact aquarium feeds you to the serpent, and entering the broken one cuts you fatally. No stable arrival exists. | [aquarium entry probe](checkpoints/zork-2-room-237-unreachable.txt) |
| Mine Dead End (138) | No safe, stable route found in the traced paths; the transient ladder visit does not expose the exit as a separate move. | [Zork III route coverage](../../zork-iii-route-coverage.md); [ladder route probe](route-data/zork-iii-audit/route-ladder-57-transient.txt) |
| Narrow Room (151) | Guardian-side mirror branch is a trap; the recorded route reaches the Guardians and death instead of a stable room prompt. | [Zork III route coverage](../../zork-iii-route-coverage.md); [guardian trap route](route-data/zork-iii-audit/route-narrow-guardians-trap.txt) |
| Sacrificial Altar (210) | View IV at Scenic Vista triggers fatal altar narration and revival at Endless Stair; no stable altar prompt. | [Zork III route coverage](../../zork-iii-route-coverage.md); [Vista route traces](route-data/zork-iii-audit/vista-destinations.log) |
| Prison Cell (225) | Appears during Guardian-death narration; the player receives no prompt before revival. | [Zork III route coverage](../../zork-iii-route-coverage.md); [guardian death route](route-data/zork-iii-audit/route-narrow-guardians-death.txt) |

The Zork III route notes classify these as transient, fatal, or trap outcomes; the Zork II Murky Room was probed directly. They also document stable paths to side rooms 51, 58, 161, 178 and Drafty Room 228. Zork II's Red, Blue, and White Mist rooms are death-recovery destinations; the gallery includes them using the traced Riddle Room → darkness/grue recovery → westward sphere path.

No stable room route was missing from the Zork I or Zork II screenshot set. The Zork I alternate grating route contributes Clearing (160) and Grating Room (216) screenshots. The Zork II route set adds its three death-only sphere rooms to the 77 regular route checkpoints.


## Last full mGBA checkpoint verification

The last full gallery pass verified 527 of 527 save-restoration captures, plus 1 route-only frame was verified by targeted input replay. The last completed full pass predates the current ROM build, so the full gallery still needs a refresh against this build. All 104 Zork III entries reached their expected room ID in mGBA.
