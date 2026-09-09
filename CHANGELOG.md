# Changelog

## Board revisions

### rev B — *campv2* — 2026-08-09 — **current, manufactured**

`campv2/` · production files in `campv2/production/`

- 80 × 80 mm, 2 layers, 1.6 mm, all through-hole, no vias.
- Arduino Nano socket, four module headers (DHT11, MQ-7, I²C LCD, and two RGB LED
  sockets), three tactile buttons, a 2-way screw terminal on the back.
- `J5` "SEL" jumper added, so the RGB LEDs can be common-cathode **or** common-anode.
- Resistor count reduced from eight to six (`R4`–`R9`): one per LED colour channel, two
  LEDs.
- +5 V pour on the front, GND pour on the back — no power tracks routed.
- Four M3 mounting holes with plated pads tied to GND.
- Turin Polytechnic University in Tashkent and Robotics & Engineering Association logos
  on the back silkscreen.
- **PER ASPERA AD ASTRA** added across the top of the back silkscreen (2026-09-09), and
  the gerber package regenerated to include it.

### rev A — *camp* — 2026-08-07 — superseded, never manufactured

`archive/camp/`

Same concept, eight resistors (`R4`–`R11`), no LED-polarity jumper. Replaced by rev B
two days later.

### *camp_demo* — undated — teaching aid

`archive/camp_demo/`

A deliberately minimal board — one Arduino Nano footprint, one RGB LED — used to walk a
class through the whole KiCad flow, schematic to gerbers, in a single session.

### *camp_t9* — empty stub

`archive/camp_t9/`

An empty KiCad project (*Cubesat* rev A, 2026-08-18): no symbols, no footprints,
a 79-byte PCB file.

---

## Repository changes

### 2026-09-09 — restructured and documented

The project had been downloaded from GitHub as a zip and worked on without git, so the
whole history arrived as one flat folder with four KiCad projects side by side. It is now
organised around a single active project.

**Structure**

- `camp/`, `camp_demo/` and `camp_t9/` moved to `archive/`.
- The firmware moved inside the project it belongs to: `CubeSat/` → `campv2/firmware/`.
- `campv2/` now holds everything the current board needs — schematic, layout, library
  tables, firmware and the fab package — in one folder.
- `lib/` (KiCad symbols, footprints, 3D models) and the top-level `production/` folder
  (logo artwork and reference plots) stay shared at the root.

**Made standalone** — a fresh clone now opens in KiCad with no configuration:

- `campv2/fp-lib-table` and `sym-lib-table` contained **absolute Windows paths**
  (`C:/Users/admin/…`) and failed on every other machine. Rewritten to use `${KIPRJMOD}`.
- The two logo footprints on the back silkscreen came from a library nicknamed
  `production` that was only registered in the user's *global* KiCad configuration. The
  nickname is now registered in the project's own `fp-lib-table`, pointing at the
  top-level `production/` folder.
- 3D models referenced two user-defined path variables, `${CAMP_ROOT}` and `${THREED}`,
  which exist only in one person's KiCad preferences. Rewritten to `${KIPRJMOD}`-relative
  paths. *This is the only edit made inside `campv2.kicad_pcb`; it changes no geometry,
  no netlist and no gerber.*
- The same three fixes were applied to the archived `camp/` and `camp_demo/` projects, and
  `camp_demo/` was given the footprint table it had always been missing.
- The three Arduino libraries the sketch needs are vendored in `Arduino/libraries/` for
  classrooms without internet.

**Documentation**

- `README.md` (English) and `README.uz.md` (Uzbek) for students; `docs/hardware-reference.md`
  for engineers; `docs/assembly.md` and `docs/troubleshooting.md`.
- Board renders, schematic render and a block diagram generated into `docs/images/`.
- `.gitignore` and `.gitattributes` added. The `.gitignore` keeps the editor-history
  sidecars (`campv2/.history/` and friends, ~550 files) and KiCad's per-user state out of
  the repository; `tools/cleanup.ps1` deletes them from disk.

**Two pre-existing issues found and documented, not silently changed** — see
[`docs/hardware-reference.md` §7](docs/hardware-reference.md#7-known-issues):

1. Six reference designators differ between the schematic and the PCB (`A2`/`ARDUINO1`,
   `J2`/`DHT11`, `J3`/`LCD`, `J4`/`MQ-7`, `LED2`/`LED1`, `LED3`/`LED2`). The schematic↔PCB
   link is intact — all 21 footprints still match by UUID — but *Update PCB from
   Schematic* would rename the footprints and invalidate the manufactured silkscreen, the
   BOM and the pick-and-place file.
2. The `MQ-7` pin legend on the schematic has `DOUT` and `AOUT` transposed. Pin 1 is wired
   to `A7`, which is analogue-input-only, so it can only be the analogue output. The
   firmware already has it right; only the drawing's caption is wrong.
