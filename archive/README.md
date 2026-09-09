# Archive

**Nothing in this folder is part of the current board.** It is kept so the design history
is not lost, and so a student can see how the board got to where it is. If you are
building, flashing or manufacturing CAMP v2, everything you need is in `campv2/`.

| Folder | What it is | Why it is here |
|---|---|---|
| `camp/` | *TechnoCamp2026 CubeSat*, rev A, 2026-08-07 | The first version. Same idea, but eight resistors (`R4`–`R11`) and no LED-polarity jumper. Replaced by rev B two days later; no boards were ever made from it. |
| `camp_demo/` | *TechnoCamp2026Demo* | A deliberately tiny board — one Arduino Nano footprint and one RGB LED — used to demonstrate the whole KiCad workflow in a single class. Good starting point if you want to draw your own first board. |
| `camp_t9/` | *Cubesat*, rev A, 2026-08-18 | An empty project stub: no symbols, no footprints, a 79-byte PCB file. Someone pressed File → New Project and never came back to it. |

## Opening the archived projects

`camp/` and `camp_demo/` are real KiCad projects, and their library tables have been
fixed to point at the shared `lib/` folder the same way the current project does, so
they open without errors. `camp_demo/` was also given the footprint table it had always
been missing.

They are frozen, though — do not develop them further. If you want to build on one, copy
it out to the top level under a new name first.

`camp_t9/` has nothing in it to open.

## About the logos

Earlier versions of the two silkscreen logos are **not** here — they live alongside the
live ones in the top-level `production/` folder, together with the PNG and JPG artwork
they were traced from. Only `turin_logo2` and `assotsiatsiya_logo` are on the board.
