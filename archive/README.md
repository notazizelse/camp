# Archive

**Nothing in this folder is part of the current board.** It is kept so the design history
is not lost, and so a student can see how the board got to where it is. If you are
building, flashing or manufacturing CAMP v2, you want `hardware/`, `firmware/` and
`production/` instead.

| Folder | What it is | Why it is here |
|---|---|---|
| `v1-camp/` | *TechnoCamp2026 CubeSat*, rev A, 2026-08-07 | The first version. Same idea, but eight resistors (`R4`–`R11`) and no LED-polarity jumper. Replaced by rev B two days later; no boards were ever made from it. |
| `demo/` | *TechnoCamp2026Demo* | A deliberately tiny board — one Arduino Nano footprint and one RGB LED — used to demonstrate the whole KiCad workflow in a single class. Good starting point if you want to draw your own first board. |
| `logo-iterations/` | Earlier logo footprints and source artwork | The two logos on the back of rev B (`turin_logo2`, `assotsiatsiya_logo`) live in `libraries/logos.pretty/`. These are the versions that did not make it, plus the PNG/JPG originals they were traced from. |
| `misc/` | Odds and ends from the original download | Files that came with the folder and belong to nothing. |

## Also dropped, not archived

`camp_t9/` — an empty KiCad project (*Cubesat*, rev A, 2026-08-18) containing no symbols,
no footprints and a 79-byte PCB file. It was a "File → New Project" stub that never got
any content.

## Opening the archived projects

`v1-camp/` and `demo/` are KiCad projects and their library tables have been fixed to
point at `libraries/` the same way the current project does, so they open without errors.
They are frozen, though — do not develop them further. If you want to build on one, copy
it out into `hardware/` under a new name first.
