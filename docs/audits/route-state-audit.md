# Route state audit

This audit traces the Paths menu against saved states from the Zork I, II, and III QA runs. Each route attempt starts from a restored snapshot, so trying one direction does not carry its state changes into the next attempt. The traces include room properties and command outcomes, then compare whether the story changes the current room.

## Findings

| Chapter | Saved snapshots | Rooms reached | Room/direction pairs with an exit property | Pairs that changed rooms in some states and stayed in others |
| --- | ---: | ---: | ---: | ---: |
| Zork I | 516 | 85 | 270 | 23 |
| Zork II | 287 | 78 | 203 | 19 |
| Zork III | 7,056 | 76 | 312 | 39 |

Across these snapshots, a room's set of direction properties did not change. The route outcome did change for some directions: doors, gates, creatures, vehicles, objects, and puzzle conditions can block movement until the story state changes. Examples include the Zork I Hades gate, a closed kitchen window, and the boat; Zork II's Guarded Room and Crypt doors; and Zork III's Museum door, prison cell, and Great Door. Paths therefore lists known route candidates and lets the original story return its condition-specific response. It does not spend a turn probing each route before displaying it.

## Special route handling

- **Zork II, Oddly-angled Rooms:** the story accepts all 12 directions and reorients the passages. Those directions are not all present as ordinary room exit properties, so the menu adds all 12 for this room.
- **Zork II, Carousel Room:** the story accepts **In** and **Out** as randomized moves even though those directions have no ordinary exit property. The menu adds those two commands.
- **Zork III, Royal Puzzle:** compass movement stays inside the same story room while moving across the internal grid. After a map has been surveyed, the menu hides compass directions whose adjacent tile is marked unknown (the story legend identifies those tiles as blocked by walls). Marble and sandstone tiles are traversable floor types, not walls. **Up** remains listed as a conditional route; it works when the ladder opening is at the player's current puzzle position.

The command replay found 11, 51, and 47 apparent movements through directions absent from ordinary room properties in Zork I, II, and III respectively. In Zork I and III, those were refusal messages followed by a secondary event such as a forced relocation or death. In Zork II, 38 of the apparent movements were the Oddly-angled Rooms behavior and two were the Carousel's **In**/**Out** moves; the remaining 11 were refusal messages followed by the Dusty Room's explosive brick event. They are not omitted ordinary exits.

## Limits and QA follow-up

The trace covers the saved audit snapshots, not every possible inventory combination or every state reachable through arbitrary play. A room/direction pair that did not change the room may still move the player inside a same-room puzzle, trigger a story event, or be blocked by a condition. The Zork III puzzle was checked separately for in-grid movement, wall-blocked tiles, and ladder exit behavior.

For each chapter, keep checking both sides of conditional routes: verify a blocked attempt produces the intended story text, change the relevant state, then verify that the route succeeds. In the Royal Puzzle, survey the map after entering or restoring, ensure its displayed position matches the player, and compare the filtered directions to the adjacent tiles. Also scroll the Paths list in the Oddly-angled Rooms to verify all 12 options remain reachable in the UI.
