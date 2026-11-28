# Player guide

See the [quick controls reference](../README.md#controls) for the room and menu buttons.

## Paths and room actions

Press **Up** in a room to open **Paths**. Use the D-pad to choose a direction and **A** to try it. Paths lists possible exits and spells out diagonal names. The same exits appear in the top bar, including in dark rooms. Triangles show diagonal exits. **U**, **D**, **I**, and **O** mean up, down, in, and out.

Teal marks the known route back to the room you just left. A conditional return route uses dim text with a teal underline. Other conditional routes appear dimmed.

In Zork III's Royal Puzzle, a surveyed map hides compass moves blocked by the maze walls. Zork II's Oddly-angled Rooms list every direction because the story reorients their passages; the Carousel also offers its randomized **In** and **Out** moves.

You can also open Paths through **Room actions**. Choose **Room actions → Nearby objects** to see the room's objects in one list. **More actions** groups ways to inspect, handle, and combine objects. For actions that require a tool, the target picker starts with the items in your bag. See [the route state audit](audits/route-state-audit.md) for trace results and remaining QA checks.

In **Paths**, look for **Way back** beside the known return route and **Conditional** beside exits that may have a requirement. Hold a D-pad direction to scroll through lists, move through pickers, or page through story text. Backing out of a menu returns you to your current story page.

## Story history

Choose **Start → Story so far** to review commands and story responses, newest first. Use **R** for older pages, **L** for newer pages, and **B** to return to the pause menu. The game keeps as much recent transcript as fits in memory and in your save. Older entries are trimmed when space runs out.

## Hints

Choose **Start → Hints** for help. When a topic matches your room or something you can see, it appears before the complete InvisiClues guide. **Next steps** suggests actions for selected puzzles based on where you have been, what you carry, and your progress, including actions in other rooms. Open a hint topic and press **A** to reveal one clue at a time. Use **Up / Down** to page through long clues, **Left** to revisit an earlier clue, and **B** to go back. Reading hints does not advance the story. [Source and credits](../stories/hints/README.md)

The complete original InvisiClues guides are available online for [Zork I](https://dfabulich.github.io/infocom-hints-html/zork-i-the-great-underground-empire.html), [Zork II](https://dfabulich.github.io/infocom-hints-html/zork-ii-the-wizard-of-frobozz.html), and [Zork III](https://dfabulich.github.io/infocom-hints-html/zork-iii-the-dungeon-master.html).

## Map

Choose **Start → Map** to see the current room and its known connections. Use the D-pad to select a room and A to center it on the map. L or R switches between compass directions and **Up / Down / In / Out**. Select opens the list of visited rooms. Browsing the map leaves your character in place.

## Commands and conversations

Choose **Room actions → Speak** for **Say**, **Answer**, **Greet**, **Yes**, and **No**. For direct parser commands, choose **Room actions → Choose command words**. In either word picker, choose a first letter and press A to see the chapter’s dictionary words. Use L and R to page, A to add a word, Select to erase the last word, and Start to send up to three words.

In Zork II, **Speak → Learned spells** appears after you hear the Wizard speak a spell aloud. Select a learned spell to say it. Use **Say** to try other words.

Use **Give item** from a character's menu to choose an offering from your bag. Some characters also have a **Give orders** menu. Look for specific actions on objects such as **Cross** on the rainbow and **Hail sailor** on the Viking ship. Dials open a number picker. Use up or down to change a number and left or right to move between digits.

**Start → Enter command** opens the original parser for commands outside the menus. Use the D-pad and A to enter characters, R for space, B to erase, and Start to send. Select goes back.

## Saving and restoring

Choose **Start → Save game** before quitting or switching chapters. Each story has its own in-game save slot. **Continue saved game** restores the story and exploration history. Earlier cartridge saves also load and show known fixed exits between previously visited rooms. On an emulator, keep the cartridge's `.sav` file with the ROM when moving it between devices.

At the end of any chapter, press **A** on the last story page to open the on-screen **Restart / Restore / Quit** menu. Use the D-pad to choose and **A** to confirm.
