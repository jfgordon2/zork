# Zork Trilogy for Game Boy Advance

Play Infocom’s complete Zork I, II, and III on one GBA cartridge. The original story programs drive the puzzles, movement, and scoring, while the GBA interface adds illustrated rooms, selectable commands, and an exploration map.

**[Download Zork Trilogy](https://github.com/jfgordon2/zork/releases/latest/download/zork-trilogy.gba)** · [White House demo](https://github.com/jfgordon2/zork/releases/latest/download/zork-white-house.gba) · [mGBA screenshot gallery](https://jfgordon2.github.io/zork/)

![The Dam, Gazebo, Lake Shore, and Royal Puzzle as displayed in mGBA](docs/images/visual-update.png)

Open the ROM in mGBA or load it on a GBA flash cartridge. Choose a chapter, then select **New game** or **Continue saved game**. Use **Start → Save game** before quitting or switching chapters. Each chapter has its own save slot.

## Controls

| Button | In a room | In a menu |
| --- | --- | --- |
| Up | Open **Paths**. | Move up. |
| Down | | Move down. |
| Left / Right | Read the previous or next page of the current response. | Move through grids and pickers. |
| L / R | Cycle through nearby objects and **Room actions**. | Page through long lists. |
| A | Open actions for the selected object or room. | Confirm. |
| B | Look around. | Go back. |
| Select | Open your bag. | |
| Start | Open the pause menu. | Resume from pause. |

- **Explore.** Open **Paths**, choose a direction, and press **A**. **Way back** marks the known return route. **Conditional** marks exits that may require a story action first.
- **Act.** Use **L / R** to focus an object and **A** to see what you can do with it. Use **Room actions → Choose command words** to build a command with the buttons. The on-screen keyboard is available under **Start → Enter command**.
- **Get help.** Open **Start → Hints** for contextual clues, possible **Next steps**, and the complete InvisiClues guides. Press **A** to reveal clues one at a time. **Start → Map** shows known connections, and **Start → Story so far** opens the transcript.

See the [player guide](docs/player-guide.md) for exit indicators, word selection, saves, and puzzle-specific controls.

## Build and test

Build both ROMs into the checkout with the devkitPro devkitARM container.

```sh
docker build -t zork-gba-builder .
docker run --rm --user "$(id -u):$(id -g)" -v "$PWD:/workspace" zork-gba-builder
```

For a local build, install GNU Make, devkitARM, Python 3, and Pillow, then run `make all`. Set `CROSS` and `PYTHON` if your toolchain prefix or Python executable differs from the Makefile defaults.

Run `make test` with a host C compiler. It covers story loading, parser behavior, saves, and controller-only routes through all three chapters, including alternate routes, recovery scenarios, and hints at puzzle milestones. Use `make test-buttons` for the controller replays on their own.

Every push to `main` runs the tests, builds both ROMs, and publishes a release with SHA-256 checksums. [GitHub Releases](https://github.com/jfgordon2/zork/releases)

## Project documentation

- [Visual audits, QA reports, and art history](docs/README.md)
- [Scene artwork and prompts](assets/README.md)
- [Story releases and license information](stories/README.md)
- [InvisiClues sources and credits](stories/hints/README.md)
