# InvisiClues source data

These three JSON files are the Zork trilogy guides from the
[PRIZM HTML edition](https://github.com/dfabulich/infocom-hints-html/tree/main/scraped-json).
The project owner confirmed permission to include the hint text in this port.
This material has its own copyright and is outside the trilogy source code's
MIT license.

Original material copyright 1993, 1999 Activision, Inc. PRIZM hints copyright
1999 Digby McWiggle and Steven Marsh. Graeme Cree and Paul David Doherty
supplied material, and Dan Fabulich converted PRIZM to HTML.
[Full credits](https://dfabulich.github.io/infocom-hints-html/#about-these-hints)

The build reads the checked-in files using `tools/invisiclues.py`. The
chapter menus include every public guide section, question, clue, and appendix.
The two shared About and Credits pages are documented here and online.

`context.tsv` associates selected guide questions with room IDs and, where
applicable, an object that must appear in Nearby objects. The menu displays
the shorter context label before revealing the guide's first clue.
