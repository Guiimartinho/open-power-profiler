# Fabrication Files of Draft A2

Outputs for a first order of proof-of-concept boards, exported on
2026-10-13 from `../../kicad/power-profiler-carrier.kicad_pcb` as it stands
in the same commit. The board is draft A2 of the carrier board; its
silkscreen reads "rev A2 draft".

No board has been made from these files. They were exported with
`kicad-cli` of KiCad 10.0.0 and checked as described under
[Checks](#checks); no Gerber viewer has read them back, and no
manufacturer has reviewed them. What is known and not known about the
design itself is in the [hardware guide](../../README.md).

## Files

| File | Content |
| --- | --- |
| `open-power-profiler-carrier-a2-gerbers.zip` | The eleven Gerber files of the four-layer board and the two drill files, ready to upload |
| `gerbers/*.gtl`, `*.g1`, `*.g2`, `*.gbl` | The four copper layers: top, ground plane, second inner layer, bottom |
| `gerbers/*.gts`, `*.gbs` | Solder mask, top and bottom |
| `gerbers/*.gtp`, `*.gbp` | Solder paste, top and bottom (the bottom is empty) |
| `gerbers/*.gto`, `*.gbo` | Silkscreen, top and bottom (the bottom has no print) |
| `gerbers/*.gm1` | Board outline |
| `gerbers/*-PTH.drl`, `*-NPTH.drl` | Plated holes and slots; the file of unplated holes is empty |
| `gerbers/*-drl_map.gbr`, `drill-report.txt`, `*.gbrjob` | Drill maps, drill report and job file, for reference; not in the archive |
| `jlcpcb-bom.csv` | Parts for machine assembly, in the column layout of JLCPCB |
| `jlcpcb-cpl.csv` | Their positions and rotations, in the same layout |

The full bill of materials, with the parts fitted by hand, is
[`../../doc/bom.csv`](../../doc/bom.csv).

## Order Data for the Bare Board

Taken from the board file. The comparison with the manufacturer's limits
uses the capabilities page of JLCPCB as read on 2026-10-13; the
manufacturer's own check of the files stays the judge.

| Item | Value |
| --- | --- |
| Layers | 4 |
| Size | 150 mm × 100 mm, corners of 3 mm radius |
| Thickness | 1.6 mm |
| Copper | 35 µm on the outer layers, 17.5 µm on the inner layers |
| Build | Prepreg 0.2 mm, core 1.075 mm, prepreg 0.2 mm in the board file: the standard four-layer build of 1.6 mm. No controlled impedance |
| Surface finish | ENIG. The guard copper around the measured node is bare by design |
| Solder mask | Both sides; openings equal to the pads (the manufacturer adds its own expansion) |
| Vias | Covered with solder mask, not plugged |
| Smallest track | 0.15 mm (limit 0.09 mm) |
| Smallest clearance | 0.127 mm between pads, 0.15 mm elsewhere (limit 0.09 mm) |
| Smallest via | 0.6 mm with a hole of 0.3 mm, a ring of 0.15 mm (limit 0.25 mm with 0.15 mm) |
| Holes | 787, all plated: 0.3 mm to 3.2 mm, and four slots of 0.6 mm × 1.2 mm (limit for a slot 0.35 mm) |
| Silkscreen | Top side only; lines of 0.15 mm, text 1.0 mm high (both at the limit) |

Things to decide in the order form:

- The surface finish is ENIG; a tin finish would leave the bare guard
  copper with an uneven surface.
- The manufacturer prints an order number on the silkscreen unless told
  where to put it or to leave it out. No place is reserved for it.
- Five thermal pads hold open vias (U11, U16, U18, Q5, Q8). They are
  ordinary covered vias, not filled ones: part of the solder under these
  parts can drain into them.

## Assembly

`jlcpcb-bom.csv` and `jlcpcb-cpl.csv` hold the 360 surface-mount
placements of the top side in 142 lines. Read this before they are used:

- The column "LCSC Part #" is empty. The project knows manufacturer part
  numbers only; whether the assembler stocks a part was not checked. Every
  part of the bill of materials is a candidate (see the hardware guide).
- Rotations are those of KiCad. The assembler counts the rotation of some
  packages from another zero. Check every part with a polarity in the
  preview of the order before it is confirmed: integrated circuits,
  transistors, diodes, the LED D5, the capacitor C11, the resistor arrays.
- Eleven parts have no visible leads and need reflow soldering: Q4, Q5,
  Q8, Q9, Q14, Q15, Q16, U5, U11, U16, U18.

Not in the two files, to be fitted by hand:

| Part | What |
| --- | --- |
| Two socket strips at U1 | 1 × 20, 2.54 mm (61302011821); the controller module plugs into them |
| J1, J3, J5 | Pin headers |
| J4 | Lever terminal block |
| SW1 | Push button |
| SH1 | Frame of the shield can, soldered last, after the parts under it were inspected; then its cover |
| U9 | Stays empty (decision D-97) |

[`../../doc/images/board-placement.png`](../../doc/images/board-placement.png)
shows every reference designator; 95 of them are not on the silkscreen.

## How the Files Were Made

From `hardware/kicad/`, with `kicad-cli` of KiCad 10:

```sh
kicad-cli pcb export gerbers --output ../fabrication/draft-a2/gerbers/ \
  --layers "F.Cu,In1.Cu,In2.Cu,B.Cu,F.Paste,B.Paste,F.SilkS,B.SilkS,F.Mask,B.Mask,Edge.Cuts" \
  --no-x2 --no-netlist --subtract-soldermask power-profiler-carrier.kicad_pcb
kicad-cli pcb export drill --output ../fabrication/draft-a2/gerbers/ \
  --format excellon --drill-origin absolute --excellon-units mm \
  --excellon-zeros-format decimal --excellon-oval-format alternate \
  --excellon-separate-th --generate-map --map-format gerberx2 \
  --generate-report --report-path ../fabrication/draft-a2/gerbers/drill-report.txt \
  power-profiler-carrier.kicad_pcb
kicad-cli pcb export pos --output positions.csv --format csv --units mm \
  --side front --smd-only --exclude-dnp power-profiler-carrier.kicad_pcb
```

The two assembly files are that position file and the netlist of the
schematic with other column names: without the three fiducials and the
frame of the shield can, grouped by value, footprint and part number. The
archive holds the eleven Gerber files (four copper layers, two solder
masks, two paste layers, two silkscreens and the outline) and the two
drill files.

## Checks

Run on 2026-10-13 on the board file and on these outputs:

- Design rules check with schematic parity: 0 violations, 0 unconnected
  pads, 0 footprint errors. Electrical rules check: 0 messages.
- The drill file holds 787 plated holes, the 694 vias and the 93 holes of
  pads of the board, and no unplated hole.
- Every layer, the drill file and the placement file lie in one coordinate
  frame: the outline runs from 50 mm to 200 mm and from −150 mm to −50 mm,
  and everything else lies inside it.

Not done: the files were not opened in a Gerber viewer, and the positions
and rotations were not laid over the copper in a viewer.
