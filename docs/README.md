# Documentation

The index of every document of the repository, with the state of each one.

The project is in early development. The carrier board exists as draft A2:
a complete schematic and a board whose layout was reviewed on 2026-10-10.
All 984 connections are routed, and the copper that an autorouter does not
draw is drawn: the pours of the 1 A path, the guard, the Kelvin pairs, the
copper of the converters, the 5 V rail and the ground fills. That review
was done with scripts and checked by independent calculation; the board has
not yet been looked at by a person in the KiCad editor, and it is not a
design to fabricate. The seven points in which it deviates from the earlier
guidelines of section 10 of the specification are recorded as decisions
D-87 to D-93. The circuits of the schematic were simulated on 2026-10-10
from its netlist, in a package of their own in `simulation/` (D-94): 134
benches, whose results are filed. The central design figures of the
specification come out again. The simulations raised four points for a
decision before boards are ordered, and the project owner decided them the
same day: decisions D-95 to D-98. Three of them are drawn since that day,
as changes of values, part numbers and texts that move no copper (D-95,
D-97 and D-98): the schematic, the board, the bill of materials and the
netlist snapshot of the simulations hold them, and the two rule checks
report nothing on these files (run on 2026-10-12). The fourth is not
drawn: a position for a capacitor at the buffer of the driver rail
(D-96), which needs a footprint and copper. The simulations were not run
again, so their results still describe the circuit before the three
changes. In fourteen places the simulations differed from the text of the
specification: twelve are corrected there since 2026-10-10, three of them
by decisions D-99 to D-101, two wait for a new run of the benches, and
seven questions are open with one recommendation each. D-102 settles the
license of the simulation folder. The software foundations exist and are
tested without hardware, and so do the package that calculates the
figures of the board layout and the package that runs the simulations.
Since 2026-10-12 the firmware has a design of its own, in `firmware/`
beside this page, the host protocol is at version 2 (D-103), and the
firmware directory of the repository builds an image for the Pico 2,
which has not been loaded onto a board.

Nothing has been built and nothing has been measured: every figure in these
documents is a datasheet value, a calculation, a simulation or an estimate,
and says which. Every figure of the board is calculated from the drawn
copper, and the package in `tools/board/` calculates them again from the
board file. A figure of the circuit simulations of 2026-10-10 comes from
a circuit whose parts were taken from the netlist of that day, with models
written from datasheets. The package in `simulation/` runs the benches
again, but its snapshot of the netlist now holds the three drawn
decisions: a new run of a bench of the rail monitor takes 10 nF where the
results on file took 1 nF, and the benches of the power input still leave
the damper out. The figures that the specification marks "simulated" are
older: they were taken from simulations made while the draft was
designed, whose files are not filed.

## In This Directory

| Document | Content | State |
| --- | --- | --- |
| [specification.md](specification.md) | The baseline: requirements R-01 to R-17, architecture and analog design, pin map, firmware with the rules F-1 to F-36 that guard hardware, host protocol, calibration, board guidelines, verification plan, development phases, risk register, decision log, open checks and bill of materials | Written for draft A2 of the carrier board, with decisions D-01 to D-104. No value in it is measured. The seven points in which the reviewed board deviates from the earlier guidelines of section 10 are decisions D-87 to D-93, and sections 10.3, 10.4, 10.6 and 10.8 describe the board as it is drawn. D-94 places the circuit simulations in `simulation/`, and D-95 to D-98 are the decisions of the project owner on the four points that the simulations raised: it states them as the design. Three of them are drawn since 2026-10-10 (D-95, D-97 and D-98) and D-96 is not; its head, sections 10.8, 13 and 17 and the first item of "Before the board is ordered" in section 16 say what is drawn and what is open. Of the fourteen other places where those simulations differed from a figure or a rule, twelve are corrected in the text, three of them by decisions D-99 to D-101; the end of section 16 lists the two that wait for a new run and seven open questions. D-102 is the license of the simulation folder. D-103 and D-104 are the decisions of 2026-10-12 on the software: version 2 of the host protocol, with a layout for the calibration record in section 8, and the over-current trip as a program of its own in PIO 2. Section 6 states that the firmware builds for the Pico 2 |
| [checks/](checks/README.md) | Component checks: each candidate part, and each peripheral of the controller, against the documentation of its manufacturer. The tables mirror section 16 of the specification, the open points of the circuit simulations included | Every check is open; no record is filed, so every part is a candidate. The four points of the simulations are decided (D-95 to D-98), and three of them are drawn since 2026-10-10. The first item before the board is ordered holds what is left of them: the position of D-96, which is not drawn, and the benches that the drawn decisions touch, which were not run again. Of the fourteen places where the simulations and the specification differed, twelve are corrected; the fourth list holds the two that wait for a new run and seven questions for the project owner |
| [checks/TEMPLATE.md](checks/TEMPLATE.md) | Form of a check record | In use from the first record |
| [reports/](reports/README.md) | Test reports: the measurements that close each development phase of section 13 | No report is filed; no phase is closed |
| [reports/TEMPLATE.md](reports/TEMPLATE.md) | Form of a phase report | In use from the first report |
| [firmware/](firmware/README.md) | Firmware design, one level below section 6 of the specification: architecture with layers, ports, the two cores and the data path; the device state machine with the start, self-test, DUT power and fault sequences; the three PIO programs; the USB link and version 2 of the protocol; the place and the test of each of the 36 firmware rules; the work cut into five packages; the decisions that are taken and the points that are open | A design, written on 2026-10-12. The PIO programs are sketches: the assembler of the Pico SDK accepts them and none has run. The project owner decided four of its points that day, two of them as D-103 and D-104 of the specification; six are open, each with one recommendation, and none of those is taken. Of what it describes, a first part of the core that needs no hardware is written and tested, and the build for the Pico 2 exists |
| [protocol/](protocol/README.md) | Host protocol: where it is defined, what version 2 holds, what is open, and what the user reference will hold | Pointer page. The protocol is at version 2 since 2026-10-12 (D-103) and not frozen; the reference is planned for phase 6 |
| [calibration/](calibration/README.md) | Calibration: what section 8 of the specification defines, the calibration record and how it travels, the reference equipment, and what is missing | Summary page. Since 2026-10-12 the record has a layout, a design that no code implements, and the protocol carries it as a block. The procedure is not written; planned for phase 6 |

## In the Rest of the Repository

| Document | Content | State |
| --- | --- | --- |
| [README](../README.md) | The project in one page: the state of every area and the next steps, target specifications with a comparison with the PPK2, how it works, connections, the hardware draft in pictures, the circuit simulations with two of their graphs, roadmap | Follows draft A2 with the reviewed layout, the simulations of 2026-10-10 and the four decisions that followed them, three of them drawn and one not; its software rows follow the firmware design, version 2 of the protocol and the build for the Pico 2 of 2026-10-12 |
| [Hardware guide](../hardware/README.md) | The carrier board: status of the draft, schematic, board, connectors, libraries, checks and the work still to do | Draft A2: 428 parts on 15 schematic pages; a board of 150 mm × 100 mm on four layers with 984 of 984 connections routed and a design rules check with no violation and no unconnected pad. The layout is reviewed with scripts; its figures are calculated from the drawn copper. Its seven deviations from the earlier guidelines are recorded decisions (D-87 to D-93). A review by a person, the open items of the layout and the thermal check of D-93 stand before fabrication. It carries the four decisions that follow the simulations (D-95 to D-98) with what each changes on the schematic, the board or the bill of materials. Three are drawn since 2026-10-10 (D-95, D-97 and D-98), with no copper changed, and both rule checks report nothing on the changed files (run on 2026-10-12). Its first step is what is left: D-96 drawn, and the benches that the drawn decisions touch run again |
| [The draft in pictures](../hardware/doc/README.md) | Every schematic sheet and the board as pictures, with the design values of each block; the schematic is also a [PDF](../hardware/doc/schematic.pdf), and the bill of materials a [CSV file](../hardware/doc/bom.csv) | Plotted and exported from draft A2; the board pictures are plotted from the reviewed board file, and the views of the assembled board are renderings, not photographs. The PDF, the bill of materials (151 lines for 370 parts), the pictures of the three sheets that decisions D-95, D-97 and D-98 changed and the two renderings were exported again on 2026-10-10 and show the drawn state. The position of D-96 is not drawn; the text under the Signal Chain sheet says so |
| [Simulation guide](../simulation/README.md) | The circuit simulations: the state of the results block by block, what they reproduce, the four points that are decided, where they differ from the text of the specification, what stays for the bench, the limits of the models, the traps of the simulator, setup, commands and checks | The package `circuit-sim` exists: 134 benches run in the ngspice library of KiCad 10, 93 that take the parts of their circuits from a snapshot of the netlist and 41 that put the models into the test circuits of their datasheets. 1834 figures pass, 122 fail and 1206 carry no limit. 777 tests, 100 % of lines and branches covered outside the one module that loads the simulator (floor 90 %), ruff and mypy in strict mode, run locally on Windows, last on 2026-10-12. Its workflow has not been started yet. It simulates with models written from datasheets and measures nothing. Its netlist snapshot holds decisions D-95, D-97 and D-98 since 2026-10-10. Its results do not: they are those of the run of 2026-10-10, on the circuit before these changes. The benches were not run again, and those of the power input still leave the damper out |
| [Simulation results](../simulation/results/README.md) | One line for every bench, and a page for each block with every figure beside the value that the specification states and its limit, the graphs of the waveforms and the decks, the circuit files that the simulator ran | Written by the package from the result files of 2026-10-10; not edited by hand. Everything in them is simulated, on the circuit as it was drawn that day, before decisions D-95, D-97 and D-98 were drawn |
| [Firmware guide](../firmware/README.md) | What exists of the firmware and what does not, item by item against the specification; its architecture and layout, the build for the Pico 2, the unit tests on the PC, coverage and static analysis | A project of the Pico SDK since 2026-10-12. It builds an image for the Pico 2 with warnings as errors, which starts the console and the device state machine and has not been loaded onto a board. The hardware-independent core is unit-tested: 22 tests, 100 % of 997 lines and of 540 branches covered, clang-format, clang-tidy and cppcheck clean, run locally on Windows. It holds the protocol codec at version 2, the block queue, the raw capture word, the settling window, the step-down and under-range rules, the device state machine, the fault causes, the sequences of DUT power on and off, the pin table, the DAC frame and the monitor scales with the rails test. No adapter, no PIO program, no USB link and no command handler is written, and no firmware rule is implemented end to end |
| [Host guide](../host/README.md) | The Python package: what it has and what draft A2 asks of it, layers, setup, the simulated instrument, checks | Protocol codec at version 2, transports, device client, capture helpers and simulator exist and are tested: 500 tests, 100 % of lines and branches covered, ruff and mypy in strict mode clean, run locally on Windows. The nominal calibration of draft A2, the fill of flagged samples, the content of the calibration record, the viewer and the export are not written |
| [Protocol directory](../protocol/README.md) | The definition of the wire protocol, what the generator writes, the shared test vectors, the state against draft A2 and how a change is made | Version 2 since 2026-10-12 (D-103), generated files up to date. What draft A2 reports is in the definition and in both codecs; no firmware fills those fields yet |
| [Tools](../tools/README.md) | The tools of the repository: the board figures, and what the calibration tool and the production test will have to do | The board figures exist and are tested without hardware. The calibration tool and the production test are not started; their directories are empty |
| [Board figures guide](../tools/board/README.md) | The package `board-figures`: how to install it, dump the board from KiCad and run each command; the method, the assumptions and the limits of each calculation; its checks | The package exists: 260 tests on synthetic boards, 100 % of lines and branches covered (floor 90 %), ruff and mypy in strict mode, run locally on Windows. Its workflow has not been started yet. It calculates from the drawn copper and measures nothing |
| [Contributing guide](../CONTRIBUTING.md) | Workflow, commit message format, quality gates, versioning | Current |
| [Changelog](../CHANGELOG.md) | Notable changes by release | Everything is under "Unreleased"; no release exists |

One directory of the hardware area is empty and has no document:
`hardware/fabrication/`, for the outputs of a board that is ordered. The
directory `hardware/simulation/` is gone: the simulations are in
`simulation/`, at the root of the repository (D-94). The scripts that
calculate the figures of the reviewed board (resistance of a pour in
squares, a path on one layer, surface leakage into the measured node,
lengths of the pairs) are filed in `tools/board/`, so a reader can repeat
those figures from the board file. The scripts that drew the copper of the
board are not in the repository, and neither are the model files of the
manufacturers, whose licenses do not allow it.

## What Comes Next

The first part of the board step is done: the 28 connections that the
autorouter left open are closed, and the layout is drawn against section 10
of the specification. The seven points in which the drawn board deviates
from the earlier text of that section are decided: the project owner
accepted them on 2026-10-10, and they are decisions D-87 to D-93 of the
log. The scripts that calculate the figures of the board are filed in
`tools/board/`. The simulation step is done as well: the circuits were
simulated from the netlist of the schematic as it stood on 2026-10-10, and
the package, the models, the benches and the results are filed in
`simulation/`. Of the rest, the first step is done in part, the software
step has begun, on 2026-10-12, and the others are not started. It is the
work that stands between the draft and the phases of section 13 of the
specification:

1. What is left of the decisions that follow the simulations. The project
   owner decided the four points on 2026-10-10 (D-95 to D-98). Three are
   drawn since that day: 10 nF at three capacitors of the rail monitor,
   the two parts of the damper on the module input fitted, and the text
   of the detector position at the boost converter, which stays without a
   part. The two rule checks report nothing on the changed files, and the
   bill of materials, the pictures and the netlist snapshot of the
   simulations were made again from them. Open: the position for a
   capacitor at the buffer of the driver rail (D-96), which adds a
   footprint to the board, and the benches of the blocks that the changes
   touch, which have to be brought in line with the new snapshot and run
   again. Until then the results of the simulations describe the circuit
   before the changes. The
   [hardware guide](../hardware/README.md#still-to-do) has the steps.
2. Decisions of the project owner that are still open:
   - The seven questions that the corrections of the specification
     leave, listed at the end of its section 16 and in
     [checks/](checks/README.md#open-points-of-the-circuit-simulations),
     each with its options and one recommendation.
   - Whether proof-of-concept boards are ordered on this state.
3. Board, four things, with the lists in the
   [hardware guide](../hardware/README.md):
   - A person opens the board in KiCad and reviews it.
   - The open items that need a part moved or another footprint: a
     placement change with a local redraw. The capacitor loops of the
     pre-regulator, the boost converter and the charge pump are the
     largest of them.
   - The silkscreen and the test points: net names, the function of the two
     jumpers, a frame for the serial number, a probe ground near each test
     point.
   - The thermal check of D-93: the temperature rise of the linear
     regulator U18 at full dissipation on its island of copper, read on
     the first board.
4. Software. The project owner took this step up on 2026-10-12, ahead of
   the three above:
   - Done: the protocol items of section 16, as version 2 of the protocol
     in the definition and in both codecs (D-103); the fault causes of
     section 6.4 in the firmware core; the build of the firmware for the
     Pico 2 with the Pico SDK.
   - Open, in the order of the [firmware design](firmware/README.md): the
     six points of that design that wait for the owner; the rest of the
     firmware core on the PC; the three PIO programs in an emulator; the
     adapters for the hardware; the firmware core tested against the host
     package.
   - Open in the host package: the nominal calibration values of
     section 8 and the statistics that section 9 asks for.
5. The open checks of section 16 and the risk prototypes of phase 1, each
   closed by a record in `checks/` or a report in `reports/`. No check is
   closed. The prototypes also settle what the simulations take as an
   input or leave open: the reaction time of the range sequencer, the loop
   of the pre-regulator and the start of the boost converter.

## How the Documents Relate

- The specification is the baseline. It names requirements `R-xx`,
  decisions `D-xx` and the firmware rules `F-xx` of its section 6.6; other
  documents refer to them by ID, and the IDs and section numbers are never
  renumbered.
- Section 16 of the specification lists the open checks. Each one is closed
  by a record in `checks/`, and the tables in `checks/` mirror the section.
- Section 13 lists the development phases. Each one is closed by a report in
  `reports/`, with recorded measurements.
- A change that deviates from the specification adds a row to its decision
  log (section 15).
- Every protocol number lives in `protocol/definition.toml`; section 7 of
  the specification describes the protocol, and the definition decides it.
  The calibration record, which the protocol carries as one block, is
  defined in section 8 of the specification and not in the definition.
- The firmware design in `firmware/` goes one level below section 6 of the
  specification and changes nothing in it: where the two disagree, the
  specification wins. The firmware guide says what of the design exists
  in the tree.
- The hardware guide and the picture guide describe the KiCad project as it
  is; the reasons behind its values are in the specification.
- The figures of the board layout in every document come from the package
  in `tools/board/`, run on the board file in `hardware/kicad/`. A change
  of the board is followed by a new run and by the figures in the
  documents.
- A figure of the circuit simulations in any document comes from the
  result pages in `simulation/results/`, which the package in
  `simulation/` writes from its runs on a snapshot of the netlist. A
  change of the schematic is followed by a new snapshot, a new run of the
  blocks it touches and the figures in the documents. For the change of
  2026-10-10 only the first of the three is done: the snapshot is new,
  the results and the figures in the documents are those of the run
  before it.
- The figures that the specification marks "simulated" were taken from
  simulations made while draft A2 was designed, with circuits typed by
  hand, whose files are not filed. The simulations in `simulation/` were
  made again from the netlist of the schematic as it stood on 2026-10-10,
  before the three drawn decisions. Where the two differed, the
  specification follows the simulations of 2026-10-10 since that day,
  with the figure marked "simulated on 2026-10-10"; its section 16 lists
  what is still open. Four points that the simulations raised are
  decisions (D-95 to D-98), and the specification states them as the
  design.
- The specification is ahead of the drawings in one decision: D-96 is
  not drawn. D-95, D-97 and D-98 are drawn since 2026-10-10, and the
  hardware guide, the pictures and the bill of materials describe that
  state. The results of the simulations are behind the drawings: they
  describe the circuit before those three changes. The simulation guide
  says so; the result pages, which the package wrote before the change,
  do not.
- A simulation result is not a check record and not a phase report. It
  closes no item of section 16 and no phase of section 13.

## Conventions

- American English.
- Prose wraps at 80 columns; tables and code blocks are exempt.
- SI units, with a space between the number and the unit: `100 mV`.
- Estimates, calculations, simulations, datasheet values and measurements
  are labeled as such. A datasheet value cites its check record and a
  measurement its report; until those exist, parts stay "candidate" and no
  figure is called verified.
- "Datasheet" is one word. The board is "draft A2" until it is fabricated
  as "revision A". A figure of its layout is "calculated from the drawn
  copper", and a picture of the assembled board is a rendering. A figure
  of a circuit simulation is "simulated", and a graph of a run is a plot
  of a simulation, not a capture from an instrument.
- Check the Markdown from the repository root before committing:

  ```sh
  npx --yes markdownlint-cli2@0.23.3
  ```
