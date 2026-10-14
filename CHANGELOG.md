# Changelog

All notable changes to this project are documented in this file.

The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project
adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

Each entry starts with the area it affects: **firmware**, **hardware**,
**host**, **protocol**, **tools**, **simulation**, **docs** or **repo**.

## [Unreleased]

### Added

- **docs:** system specification with requirements, analog and firmware
  architecture, host protocol, calibration, verification plan, development
  phases, risk register and decision log.
- **docs:** templates and indexes for component checks and phase reports.
- **repo:** monorepo layout with `firmware/`, `hardware/`, `host/`, `tools/`
  and `docs/`.
- **repo:** contributing guide, issue and pull request templates, editor and
  lint configuration, documentation lint workflow.
- **repo:** MIT license for everything outside `hardware/` and CERN-OHL-P v2
  for the hardware design files.
- **protocol:** one definition of the wire protocol in
  `protocol/definition.toml`, with a generator for the firmware and host
  constants and for the test vectors that both codecs must reproduce.
- **firmware:** ESP-IDF project for the ESP32-S3 with the
  hardware-independent core, unit-tested on the PC: CRC, frame encoder and
  parser, sample word, stream payload, command envelopes, the block queue
  between the cores, the settling window, the step-down rule and the device
  state machine. It drives no hardware yet.
- **host:** Python package `s3-power-profiler` with the protocol codec,
  serial and in-memory transports, a device client, a capture reader with gap
  detection, nominal calibration and statistics, a device simulator and the
  `s3pp` command.
- **hardware:** KiCad 10 project of the carrier board with the sockets of the
  development board and the 19 signals between the boards.
- **hardware:** draft A0 of the carrier board: the complete schematic on
  thirteen A4 sheets drawn with wires, and a board file with every footprint
  placed by functional block and no tracks. A review draft with candidate
  parts, not a design to fabricate.
- **hardware:** project symbols for the ADS8860, the MUX509 and the TPS63020,
  and a power-symbol library for the supply rails.
- **hardware:** pictures of every schematic sheet and of the board, and the
  schematic as PDF, in `hardware/doc/`.
- **hardware:** draft A1 of the carrier board. A Raspberry Pi Pico 2 on
  sockets is the only programmable part; two shift registers carry the
  side data; the DUT connects through a pin header and a lever terminal
  block in the pin order of the PPK2, and the logic port has that order
  too. The board is 130 mm × 100 mm with every part placed near the pin it
  serves.
- **repo:** continuous integration for firmware, host software, protocol and
  hardware, with coverage floors and static analysis.
- **hardware:** draft A2 of the carrier schematic: 428 parts on 15 A4
  pages, 14 sheets below the root sheet (D-83). It passes the electrical
  rules check and was checked independently against datasheets; the parts
  stay candidates, and nothing is built or measured.
- **hardware:** input stage with a current limiter on each USB input and a
  priority multiplexer that supplies the 5 V rail from USB-C whenever it is
  present (D-47). A supervisor on the 5 V rail switches the supplies of the
  carrier off as one below 3.9 V (calculated, D-48).
- **hardware:** rail monitor on a sheet of its own: PWR_GOOD comes from
  four comparators that watch 3V3_A, +12 V_A, −4 V_A and the reference
  (D-54).
- **hardware:** VIN protection that withstands −20 V to +20 V while the
  ampere switch is open (simulated), with a bidirectional suppressor, a
  fuse and a detector that refuses to close the switch above 5.46 V
  (calculated, D-60). An interlock transistor keeps the two mode switches
  from closing together (D-62).
- **hardware:** ESD arrays at the CC pins and at the VCC pin of the logic
  port (D-50), test points on the nodes that draft A2 adds, and positions
  without parts for a voltage detector at the boost converter and for a
  damper on the input from the controller module (D-84).
- **hardware:** a project symbol for the input multiplexer TPS2116, and
  project footprints for the lever terminal block and for the controller
  module (D-83, D-85).
- **docs:** one list of the firmware rules that guard hardware, in section
  6.6 of the specification, with the hardware watchdog of the controller as
  the last line of defense (D-81).
- **docs:** the README states where every area of the project stands and
  the next steps in their order, and shows the routed layers, the open
  connections, the placement and the check results of draft A2.
- **hardware:** the bill of materials of draft A2 is filed as
  `hardware/doc/bom.csv`: 151 lines, each with a part number and a maker.
- **docs:** the hardware guide lists the 28 open connections of the
  board one by one, with what each breaks while it is open and a map of
  them, and the simulations whose files have to be filed.
- **docs:** the firmware, host and protocol guides compare the code with
  what the specification of draft A2 asks, item by item, and give the next
  steps of each area; the pages on calibration, tools and test reports say
  what the specification defines today and what is not written.
- **hardware:** two pictures of the reviewed board in
  `hardware/doc/images/`: `board-front-end.png`, the measured node under
  the shield can with the guard and the Kelvin lines, and
  `board-1a-path.png`, the pours from the linear regulator to the terminal
  block. Both are plots of the board file, not photographs.
- **docs:** the hardware guide lists, one by one, the seven points in which
  the reviewed board deviates from the layout guidelines that section 10
  of the specification had for draft A2, each recorded as a decision (D-87
  to D-93), and the open items of the layout that need a part moved or
  another footprint.
- **tools:** the package `board-figures` in `tools/board/`, with a command
  of the same name. It calculates the layout figures that the documents
  quote from a dump of the board file of the carrier: the resistance of
  the copper of a net between pads in squares and in milliohms, a path on
  one layer alone, the center-line lengths of the two lines of a pair, the
  surface leakage into the measured node, and all figures of the carrier
  in one report. What is specific to the carrier board is in
  `tools/board/carrier.toml`, and an adapter that runs under the Python of
  KiCad 10 writes the dump. Every figure it gives is calculated from the
  drawn copper; it measures nothing.
- **tools:** the package is tested without hardware and without KiCad: 260
  tests on small synthetic boards with answers known by hand, 100 % of
  lines and branches covered against a floor of 90 %, ruff, and mypy in
  strict mode. These checks ran locally on Windows. The package brings
  numpy, scipy and shapely as dependencies, for this package only.
- **repo:** the workflow `Tools` runs the checks of `tools/board/` on
  Windows, Linux and macOS and builds and installs the package. Like the
  other workflows it is started by hand (D-22); it has not been started
  yet.
- **docs:** a guide of the board figures in `tools/board/README.md`: how
  to install the package, make the dump and run each command, with the
  method, the assumptions and the limits of each calculation.
- **simulation:** the package `circuit-sim` in `simulation/`, with a
  command of the same name, simulates the circuits of the carrier board
  (D-94). A bench names parts of the schematic by their reference
  designators; the package writes their elements from a snapshot of the
  netlist that KiCad exports, runs the circuit in the ngspice shared
  library that KiCad 10 ships, takes figures from the waveforms, and
  judges each against its limit: a figure of the specification, a rating
  of a datasheet, or a limit that the bench sets and names. It files the
  figures, a graph of the waveforms and the decks, the circuit files
  that the simulator ran, and writes a page for each block from them. No
  part of the board is typed by hand; sources, loads and cables are.
- **simulation:** models of the parts, written for the project from their
  datasheets, each with the source of its figures and a list of what it
  leaves out. 41 benches put them into the test circuits of their
  datasheets; four models have no such bench, and the simulation guide
  names them. The model files of the manufacturers are not in the
  repository, because their licenses do not allow a copy; where a copy is
  present, a second run gives a second value beside the first.
- **simulation:** the results of 2026-10-10 on the netlist of draft A2, in
  `simulation/results/`: 134 benches, 93 on the circuits of the board in
  ten blocks (shunt ladder, signal chain, range logic, source meter,
  output stage, path switching, power input, analog rails, digital lines,
  whole measuring path) and 41 on the models. Of 3162 figures 1834 pass,
  122 fail and 1206 carry no limit; a figure that fails is kept as it is.
  The folder holds 251 graphs and 289 decks. Everything in it is
  simulated: no board is built and nothing is measured.
- **simulation:** what those results say. The central design figures of
  the specification come out again block by block, among them the drop of
  requirement R-07 on a step from 1 µA to 500 mA with 1 µF: 301.9 mV with
  nominal delays, and 388.2 mV with every delay at its limit and the
  capacitor 10 % low, against the limit of 500 mV (simulated). The
  simulations raised four points for a decision before boards are
  ordered: the capacitors of the rail monitor, the buffer of the driver
  rail, the detector position at the boost converter and the damper on
  the module input. The project owner decided them (D-95 to D-98, under
  "Changed"). In fourteen places the simulation and the text of the
  specification differed; twelve are corrected in the specification
  since (under "Changed").
- **simulation:** the package is tested: 777 tests, of which 11 need the
  ngspice library, 100 % of lines and branches covered against a floor of
  90 %, the module that loads the simulator excepted, ruff, and mypy in
  strict mode over the package, its tests and the benches. These checks
  ran locally on Windows with Python 3.11 and ngspice 45.2. The package
  brings numpy and matplotlib as dependencies.
- **repo:** the workflow `Simulation` runs the checks of `simulation/` on
  Windows, Linux and macOS, runs the tests that need the simulator with
  the ngspice library of a Linux distribution, and builds and installs the
  package. It does not run the benches. Like the other workflows it is
  started by hand (D-22); it has not been started yet.
- **docs:** a guide of the simulations in `simulation/README.md`: the state
  of the results block by block, what they reproduce, the four points
  that are decided, the places where they differ from the text of the
  specification, what stays for the bench, what the 122 failing figures
  are, the limits of the models, the traps of the simulator, and how to
  set up, run and extend the package.
- **docs:** the README shows the circuit simulations on the front page: a
  row in the status table, the results by block, what they say, and two
  graphs of simulation runs, each called a simulation.
- **docs:** a design of the firmware in `docs/firmware/`: architecture
  with layers, ports, the two cores and the data path; the device state
  machine with the start, self-test, DUT power and fault sequences; the
  three PIO programs as sketches with their timing by count; the USB link
  and what version 2 of the protocol added; and the place and the test
  of each of the 36 firmware rules. It is a design: the sketches of the
  programs assemble with `pioasm`, none has run in an emulator, and
  nothing has run on a controller.
- **firmware:** more of the hardware-independent core, each part with
  unit tests on the PC: the raw word of the capture split into result and
  side bits (section 4.7); the pin table with the checks of rules F-4 and
  F-5; the DAC frame and the nominal set-point code (F-6, F-30); the
  scale of the eight monitor channels, the source of the 5 V rail and the
  rails test (F-11, F-12); the fifteen fault causes of section 6.4; a
  runner for timed sequences and the four sequences of DUT power on and
  off, with the test that rule F-28 asks for (order and times of F-23 to
  F-26 and F-28 to F-31); the under-range rule of F-22 and the
  over-range mark of F-35 as algorithms on samples. No rule is
  implemented end to end yet, and nothing of it drives hardware.
- **firmware:** a codec for the records of protocol version 2: the data
  of GET_INFO and GET_STATUS, of the event "supply changed" and the zero
  offsets, tested against the shared vectors.
- **protocol:** layouts of fixed records in the definition, as lists of
  fields. The generator derives their sizes and field offsets for C,
  their struct formats for Python, and record vectors that both codecs
  reproduce byte for byte.
- **hardware:** a third project footprint,
  `Texas_R-PVSON-N14_3x4mm_P0.5mm_EP1.58x2.85mm`, in
  `hardware/kicad/lib/PowerProfiler.pretty/`: the land pattern of the DSJ
  package of Texas Instruments (R-PVSON-N14) for the pre-regulator U16,
  from the drawings in the datasheet SLVS916I of the TPS63020. The finger
  lands that the drawing shows at the two short ends of the package are
  not drawn.
- **hardware:** the silkscreen of the carrier board carries the frames
  and names of the sixteen placement blocks, as lines of 0.15 mm that
  are cut wherever a pad, an opening of the solder mask, a part or a text
  is in the way, and the pin names TX, RX and GND at the console header
  J1. Until 2026-10-13 the frames and names were on a documentation layer
  of the board; they are removed there.
- **docs:** the hardware guide has a section on the review of footprints
  and placement of 2026-10-13: what was compared with what, the five
  findings and what was done about each, the frames on the silkscreen,
  the figures of the board after the round, the checks, and what stays
  open. The picture guide and the README carry it at smaller size, and
  section 10.8 of the specification gives the state of the board on that
  day.
- **hardware:** the fabrication outputs of draft A2 in
  `hardware/fabrication/draft-a2/`, for an order of proof-of-concept
  boards (D-105): eleven Gerber files of the four-layer board (four
  copper layers, two solder masks, two paste layers, two silkscreens and
  the outline) and two drill files, also as one archive, with the drill
  maps, the drill report and the job file beside them; two assembly
  files in the column layout of JLCPCB with the 360 surface-mount
  placements of the top side in 142 lines; and a README with the order
  data of the bare board, what the assembly files hold and lack, and the
  checks. They were exported from the board file with `kicad-cli` of
  KiCad 10.0.0. The drill file holds 787 plated holes, which are the 694
  vias and the 93 holes of pads of the board, and every layer, the drill
  file and the placement file lie in one coordinate frame. No Gerber
  viewer has read the files back, and no manufacturer has reviewed them;
  the assembly files carry manufacturer part numbers only, and their
  rotations are those of KiCad. No order is recorded: no board has been
  ordered or made as far as the repository shows.
- **docs:** the hardware guide has a section on the fabrication outputs
  of draft A2: what the files are, what was checked on them and what was
  not, the smallest features of the board against the limits of the
  manufacturer, and what the order still needs from the project owner.
  The README has a row for them in its status table, and the
  documentation index lists their README.

### Changed

- **docs:** the instrument is now two boards: an ESP32-S3-DevKitC-1
  compatible development board plugged into a carrier board (D-14). The
  carrier has its own USB-C power connector (D-15), the slow monitors move to
  an external converter (D-16), and the pin map is rewritten for the headers
  of the development board.
- **docs:** the host protocol gains its exact CRC parameters, the type,
  command, status and event codes, the command arguments and the rules for
  which state accepts which command.
- **docs:** software engineering practices and quality gates (section 18,
  requirement R-17, D-17 to D-21).
- **docs:** the development board is the two-port clone with an N8R2 or
  N16R2 module, 25.40 mm between its header rows; the pin map gains PATH_EN
  on GPIO 35 and a spare line on GPIO 36 (D-23, D-29).
- **docs:** the specification follows the draft schematic: range logic in a
  programmable logic device, −4 V negative rail, dual Kelvin multiplexer,
  comparators on the amplifier output, 0.45 V of pre-regulator headroom,
  buffered ladder output, candidate parts for every block (D-24 to D-38).
- **hardware:** the socket footprint of the development board accepts
  25.40 mm and 22.86 mm between the rows.
- **repo:** the Hardware workflow also checks the board against the
  schematic.
- **repo:** the continuous integration workflows are started by hand
  instead of on every push and pull request (D-22).
- **docs:** the controller is a Raspberry Pi Pico 2 in place of the
  ESP32-S3 development board and of the programmable logic device: range
  logic and sampling clock become PIO programs, the pin map, the firmware
  plan and the phases are rewritten (D-39 to D-43). The firmware in the
  repository is not ported yet.
- **hardware:** reference designators follow the default annotation of
  KiCad (D-45), and the outline shrinks from 160 mm × 100 mm to
  130 mm × 100 mm (D-46).
- **repo:** the repository is renamed to `open-power-profiler`. Its former
  address, `esp32-s3-power-profiler`, redirects to the new one.
- **docs:** the README presents the project as the Open Power Profiler,
  with its connections and the state of each part; the firmware guide
  separates what is valid for the Raspberry Pi Pico 2 from the ESP-IDF
  build of the first plan.
- **host:** the description of the package, its keywords and the help of
  the `s3pp` command no longer name the ESP32-S3.
- **docs:** the specification follows draft A2 of the schematic: decisions
  D-47 to D-86, the open checks of section 16, the phases of section 13
  with three risk prototypes in phase 1, and the layout guidelines of
  section 10.
- **docs:** **BREAKING** seven requirements are re-baselined to the
  figures that the architecture delivers. None of the figures is measured;
  section 2 of the specification states the evidence of each and compares
  it with the PPK2.
  - R-04: RMS noise in the lowest range at most 5 nA in ampere mode from a
    quiet supply and at most 40 nA in source mode (D-59).
  - R-06: total path drop of 200 mV at 1 A in place of 150 mV; the shunt
    drop stays at 100 mV (D-64).
  - R-07: on a step from 1 µA to 500 mA the instrument adds at most 0.5 V
    of drop with 1 µF at the output and at most 0.25 V with 10 µF (D-65).
  - R-08: 1.0 A up to 2.0 V, falling in a straight line to 0.6 A at 5.0 V,
    in place of 1 A at every voltage (D-56).
  - R-09: the VIN terminal withstands −20 V to +20 V while the ampere
    switch is open (D-60).
  - R-10: logic inputs from 1.65 V to 5.5 V in place of 1.6 V to 5.5 V
    (D-79).
  - R-14: the budget is an input current for each source, 0.45 A, 1.4 A or
    1.7 A, and the 5 V rail may fall to 4.25 V (D-49).
- **docs:** **BREAKING** the calibration record of section 8 is tied to
  the module and the carrier: it gains the chip identifier of the RP2350
  and the serial number and revision of the carrier (D-82), a
  closed-switch zero (D-59) and nine or more set-points of the DAC in
  place of two (D-58).
- **hardware:** source meter: the pre-regulator tracks the output of the
  linear regulator (D-55), and the set-point DAC is referenced to half the
  reference, which caps the output at about 5.26 V (calculated, D-58).
- **hardware:** the over-current trip is at 1.15 A in the highest range in
  place of 1.2 A (D-74), and the output switch closes slowly and opens fast
  (D-71).
- **hardware:** the ladder clamp is two diode-connected MOSFETs (D-66), and
  VOUT is protected by one 15 V suppressor to ground (D-70).
- **hardware:** the shunts of ranges 0 and 1 are thin-film resistors, range
  2 uses a two-terminal part on a Kelvin land and range 3 a four-terminal
  part (D-68).
- **hardware:** the board grows from 130 mm × 100 mm to 150 mm × 100 mm
  with a shield can over the front end (D-85). Its parts are placed by a
  script and its tracks come from an autorouter, which leaves 28 of the
  984 connections open (D-86): an autorouted draft that needs a layout
  review before fabrication.
- **hardware:** the pictures and the PDF in `hardware/doc/` show draft A2,
  and the reference designators are numbered anew for it by the rule of
  D-45.
- **docs:** the table of open component checks follows section 16 of the
  specification for draft A2, and the documentation index gives the state
  of every document and the work that comes next.
- **docs:** section 16 of the specification lists the points of its
  sections 6 to 8 that are not decided yet: how the calibration record
  travels, reports without a place in the protocol, where three stored
  constants live, the evaluation step of two firmware rules and a fault
  during start-up.
- **repo:** the contributing guide and the issue and pull request forms
  follow the present state: the Pico 2, draft A2, checks started by hand.
- **hardware:** the layout of the carrier board of draft A2 is reviewed
  against section 10 of the specification. All 984 connections are routed,
  where the autorouter had left 28 open, and the design rules check
  reports no violation, no unconnected pad, no footprint error and no
  difference from the schematic. The schematic and the bill of materials
  are unchanged; 35 of the 425 footprints moved or turned. The review was
  done with scripts and checked by independent calculation; nobody has
  reviewed the board item by item in the KiCad editor. The seven points
  in which it deviates from the earlier text of section 10 are recorded
  as decisions (D-87 to D-93), and the board stays a draft with open
  items.
  Every figure of it is calculated from the drawn copper; nothing is built
  and nothing is measured.
- **hardware:** the 1 A path is drawn as pours on the top layer, with the
  supply band doubled on the bottom layer: 19.5 squares of copper in
  source mode and 23.3 squares in ampere mode against the limit of 30
  squares, 10.4 mΩ and 12.3 mΩ at 40 °C (calculated from the drawn copper,
  copper only). The autorouted tracks had 397 and 217 squares.
- **hardware:** the measured node has a guard around it under the shield
  can, with the solder mask open over 84 % of the guard track, and 10 of
  the 11 sense and guarded nets run on the top layer without a via. The
  Kelvin pair of the 0.1 Ω shunt runs side by side over 83 % of its
  length; the pair of the 1 Ω shunt and the pair to the amplifier are
  equal in length within 0.9 mm and not side by side throughout, and the
  taps of the 1 kΩ and 33 Ω shunts run through a via each on the second
  inner layer. The calculated surface leakage into the measured
  node is 5.1 nA against a budget of 10 nA, with the assumptions of
  section 10.3 (10¹¹ Ω per square on a clean surface; solder mask,
  cleanliness and humidity not modeled).
- **hardware:** the switch nodes and the output copper of the converters
  are pours, the 5 V rail is a pour on the second inner layer, and the
  reference leaves its output pin as a star of ten branches. The capacitor
  loops of the pre-regulator, the boost converter and the charge pump are
  longer than section 10.5 asks and stay open: their capacitors have to be
  placed again.
- **hardware:** ground fills on the second inner layer and on the bottom
  layer, stitched to the ground plane with 41 added vias; the ground plane
  stays one piece with no track on its layer. Through-hole ground pads
  have thermal reliefs, and the pads of the 1 A path are joined solid.
- **hardware:** the rule file of the board is rewritten and stricter. The
  exemption of the larger spacings near pins applies on the top layer
  only; the 0.5 mm of the measured node apply on the bottom layer too;
  copper fills keep 1.0 mm from the measured node on the outer layers; the
  pours of the measured node keep 1.0 mm from ground and rail copper, pads
  included; the VIN input keeps 1.0 mm on the outer layers and 0.5 mm on
  the inner layers; the guard may run 0.2 mm beside the measured node.
  These rules are recorded as decision D-87.
- **hardware:** the pictures of the board in `hardware/doc/images/` are
  plotted again from the reviewed board file. The views of the assembled
  board are renderings.
- **repo:** the Hardware workflow also fails on an unconnected pad of the
  board. It could not while the autorouted board had open connections;
  the design rules check of the reviewed board reports none.
- **docs:** the specification gives the state of the board after the
  layout review in its head, in section 10.8 and in the open checks of
  section 16, and section 10.3 carries the calculated leakage figure in
  place of the earlier estimate of 1.3 nA to 2.9 nA.
- **docs:** the specification records the seven points in which the
  reviewed board deviates from its layout guidelines as decisions, and
  sections 10.3, 10.4, 10.6, 10.8, 13, 14 and 16 follow them. The project
  owner accepted each on the recommendation of the layout review. With
  them the decision log reached D-93.
  - D-87: the rule file of the board states every spacing, among them
    1.0 mm from VIN on the outer layers and 0.5 mm on the inner layers.
  - D-88: the high-side sense line of the 0.1 Ω shunt keeps two vias and
    1.9 mm on the second inner layer.
  - D-89: the Kelvin pairs are routed as on draft A2: equal in length
    within 1 mm, and side by side only where the pins allow.
  - D-90: the guard is one piece of copper, open on the top layer at the
    three exits of the measured node and joined on the other layers.
  - D-91: the supply, address and enable pins of the multiplexer and their
    parts stand inside the guard, with no ground pour inside it.
  - D-92: the 1 A path is judged by its resistance, 30 squares or less in
    either mode, and two of its pieces run on the bottom layer.
  - D-93: the island of output copper under the linear regulator U18 is
    291 mm² on the bottom layer, joined by 22 vias to 102 mm² on the top
    layer. The temperature rise of U18 at full dissipation is an open
    check of section 16, to be read on the first board.
- **hardware:** the board carries the name of the project. The silkscreen
  and the title blocks of the board and of the schematic read "Open Power
  Profiler - Carrier Board". The schematic PDF and the picture of its root
  page are exported again; the other 14 pages are unchanged.
- **docs:** the README, the guides, the indexes of the checks and of the
  reports and the contributing guide describe the board after the layout
  review: what is drawn, which figures are calculated, what is decided,
  what is open and what comes next. The tools guide and the contributing
  guide name the board figures and their checks.
- **docs:** the specification records where the circuit simulations live
  and how they are made, as decision D-94 of the project owner: a package
  of their own in `simulation/`, circuits built from a snapshot of the
  netlist, models written for the project. Its head, the repository
  structure of section 12 and the practices of section 18 follow, and
  section 16 gains a list at its end: the fourteen places where the
  simulation and the text differ. In those places no requirement, figure,
  rule or earlier decision changes: the text stands until the owner
  decides.
- **hardware:** four decisions follow the circuit simulations, taken by
  the project owner on 2026-10-10 on their recommendation. The three
  capacitors C32 to C34 at the comparator inputs of the rail monitor are
  10 nF, not 1 nF (D-95, which changes the value of D-54). A position for
  a capacitor from the non-inverting input of the buffer U28 of the
  driver rail to ground is added; its value is chosen on the bench, and
  the position may stay empty (D-96). The position U9 at the enable pin
  of the boost converter stays without a part and is no longer meant for
  the 803 type with its time-out of 0.24 s (D-97, which changes D-84).
  The damper R14 with C5 on the input from the controller module is
  fitted (D-98, which changes D-84).
- **hardware:** decisions D-95, D-97 and D-98 are drawn, as changes of
  values, part numbers, fitted flags and texts in the schematic and on
  the board, with no change of copper: C32 to C34 are 10 nF
  (GRM1885C1H103JA01D), R14 (ERJ-6RQFR33V) and C5 (CL21A106KAYNNNE) are
  fitted, and the text of the position U9 no longer names the 803 type.
  The electrical rules check and the design rules check with schematic
  parity report nothing. The bill of materials has 151 lines for 370
  parts; the schematic PDF, the pictures of the three sheets and the two
  renderings of the board are exported again. D-96, the position at U28,
  is not drawn: it needs a footprint and copper.
- **simulation:** the netlist snapshot is written again from the
  schematic with the three drawn decisions. The benches were not run
  again: the results are those of the circuit before the changes, and
  the benches of the power input still leave R14 and C5 out.
- **docs:** the specification records decisions D-95 to D-98 and follows
  them in its head and in sections 3, 4.1, 4.5, 4.7, 10.8, 11, 13, 14, 16
  and 17. The four points leave the list of open points at the end of
  section 16; what the decisions leave to check stays in its lists. With
  them the decision log reached D-98. Since three of them are drawn, the
  first item before the board is ordered is the fourth, D-96, with a new
  run of the benches that the changes touch.
- **docs:** the specification follows its simulations of 2026-10-10 in
  twelve of the fourteen places where the two differed. The simulated
  figure, marked "simulated on 2026-10-10", replaces the earlier one in
  sections 4.2, 4.3, 4.4, 4.6, 4.7, 4.8, 4.9 and 4.10, in the reasons of
  the firmware rules F-8, F-18, F-23 and F-24, in the risk register and
  in the open checks. Three corrections are decisions of the project
  owner: rule F-18 counts the time of the over-current trip with range 3
  selected only, and the test of section 11 reads the comparator in the
  same way (D-99); the advice on a capacitor at the VIN terminals and its
  test are limited to supply leads of 50 mΩ per microhenry or more
  (D-100); the range of a logic input is stated as 0 V to 5.5 V (D-101).
  The times at power-off and the figures of the power input wait for a
  run of the benches on the drawn decisions. Section 16 keeps seven
  questions open, each with its options and one recommendation: the
  criterion of the check on TP33, the limits of F-26 and F-28 against
  the leakage of the monitor converter, the upper limit of F-12 against
  the full scale of channel 2, the threshold of F-35, the capacitor at
  the VIN terminals with heavy leads, a withstand voltage of the logic
  inputs and the gaps of the test of TP4.
- **repo:** the license of the simulation folder follows the two
  licenses of the repository (D-102): the package, its tests, the
  benches, the models and the guide are under MIT; the snapshot of the
  netlist in `simulation/netlist/` and the decks, records, graphs and
  pages in `simulation/results/` derive from the hardware design and are
  under CERN-OHL-P v2, with a copy of that license in each of the two
  folders. The README, the contributing guide and the simulation guide
  say so.
- **docs:** the README, the hardware guide, the picture guide, the
  documentation index, the index of the checks and the pages on tools,
  firmware, host software, calibration and test reports describe the
  project after the simulations of 2026-10-10: what was simulated and
  with which models, what is reproduced, where the simulation and the
  specification differ, and what only a bench can settle. The README,
  the hardware guide, the picture guide, the documentation index, the
  index of the checks, the firmware guide and the page on test reports
  also carry the four decisions that followed, which of them are drawn,
  and the state of the fourteen places after the corrections of the
  specification. The next steps of the hardware begin with drawing
  decision D-96 and running the touched benches again; the open
  questions of section 16 and the order of proof-of-concept boards are
  the open decisions of the owner; filing the simulations is no longer a
  step. The order was decided later, on 2026-10-13 (D-105).
- **repo:** the contributing guide and the issue and pull request forms
  name the simulation area: the commit scope and changelog area
  `simulation`, the four checks of the package, what a change of the
  schematic, of a model or of a bench has to carry, and the rule that a
  figure that fails in a simulation is kept.
- **protocol:** version 2 of the host protocol, in one step (D-103).
  The sample word marks over-range, under-range and the request of the
  output switch in three of its reserved bits. The fault flags grow from
  four to fifteen, one for each cause of section 6.4 of the
  specification. GET_STATUS carries 25 bytes and GET_INFO 48: mode,
  locked range, set-point, readings, supply in use with its CC class and
  its budget as a current, trips, notices, state of the calibration,
  identity and zero offsets. New events "supply changed" and "notice";
  new commands CAL_READ and CAL_STORE, which carry the calibration
  record of section 8 as one block; CAL_ZERO returns the zero offsets.
  This is a breaking change of the protocol: a host of version 1 and a
  device of version 2 do not understand each other.
- **firmware:** the sample word has the three new marks, every fault
  cause has a flag of its own, and the list of known commands and events
  follows version 2 of the protocol.
- **firmware:** the build is a project of the Pico SDK for the Raspberry
  Pi Pico 2 (preset `pico2`), built with SDK 2.3.1 and warnings as
  errors. It compiles every hardware-independent component for the
  RP2350 and produces a `.uf2` image whose `main` starts the console and
  the device state machine and drives no pin beyond the console output. The
  list of components is shared with the unit test build. The image has
  not been loaded onto a board.
- **firmware:** the build job of the `Firmware` workflow builds for the
  Pico 2. It has not been started yet.
- **host:** the package follows version 2 of the protocol: records,
  marks, events and the calibration commands in the codec, the simulator
  and the client. `DeviceClient.calibrate_zero` returns the zero
  offsets, and `read_calibration` and `store_calibration` replace
  `write_calibration`. The statistics leave over-range and under-range
  samples out, as they do with the samples of a settling window.
- **spec:** decisions D-103 (protocol version 2) and D-104 (the
  over-current trip and GATE_OUT in PIO 2, the range sequencer in
  PIO 1). Sections 5, 6, 7, 8, 16 and 18.3 follow them and the ported
  build; the calibration record has a layout in section 8.
- **hardware:** the footprints and the placement of the carrier board of
  draft A2 were reviewed on 2026-10-13, on a copy of the project, with
  scripts on the board file, 3D renderings of every region of the board
  and the datasheets. All 425 footprints equaled the footprint of the
  same name in their library; 55 of the 59 symbols of the parts are
  unchanged symbols of the KiCad library, and the four symbols of the
  project agree pin by pin with their datasheets; the terminal block,
  the USB-C receptacle, the socket strips and the 0.1 Ω shunt agree with
  the drawings of their makers; no courtyard overlaps another, and no
  part stands under the module. It is not a review in the KiCad editor,
  nothing is measured, and it closes no component check. Its five
  findings are under "Fixed". The project owner opened the project in
  the KiCad editor that day and found the schematic and the board
  coherent: a look, not a review item by item.
- **hardware:** the six pictures of the board in `hardware/doc/images/`
  are plotted again from the board file after the round of 2026-10-13,
  and the bill of materials is exported again, with one line changed:
  the footprint of U16. The schematic PDF is exported again too; its
  pages are the same as before. After that round the board has 8.40 m of
  track in 3,743 segments and 694 vias. The 1 A path, 19.5 and 23.3
  squares, and the surface leakage into the measured node, 5.1 nA, are
  unchanged (calculated from the drawn copper), and both rule checks
  report nothing.
- **hardware:** the project file `power-profiler-carrier.kicad_pro` holds
  the default `erc` and `schematic` settings blocks that KiCad 10 writes
  when its schematic editor opens a project: 334 lines added, nothing
  changed or removed. The severities of the board checks and the net
  classes are as they were.
- **repo:** git ignores the folder `.history/` that KiCad 10 keeps beside
  a project.
- **docs:** the README, the hardware guide, the picture guide, the
  documentation index, the contributing guide, the indexes of the checks
  and of the reports and the specification describe the board after the
  round of 2026-10-13: the look of the project owner in the KiCad
  editor, what the review of footprints and placement compared, the five
  findings and what was done about each, the figures after the round,
  and what is open. An item-by-item review in the KiCad editor still
  precedes revision A; 95 reference texts are hidden, two vias stand at
  a pad, the lower edge of the frame of the shield can is not confirmed
  in a drawing of its maker, and the finger lands of U16 are not drawn.
  The board figures guide shows the output of the dump and of the report
  on the board of that day.
- **docs:** the specification records decision D-105 of the project
  owner, of 2026-10-13: the fabrication outputs of draft A2 are filed for
  an order of proof-of-concept boards while the items of section 16
  under "Before the board is ordered" are still open. Boards made from
  them are draft A2 and not revision A, and those items stay open and
  remain conditions of revision A. Its head and sections 4.2, 10, 10.8,
  12, 13, 14 and 16 follow, and with it the decision log reaches D-105.
  The README, the guides and the indexes name the decision where they
  spoke of an open question whether proof-of-concept boards are ordered:
  the question is decided, the outputs are filed, and the order is the
  owner's.

### Removed

- **hardware:** the symbol and the socket footprint of the ESP32-S3
  development board, the programmable logic device and the buffer between
  the two boards.
- **hardware:** the resettable fuse and the OR-ing diode of the power
  input, replaced by the input stage of D-47.
- **hardware:** the reverse clamp diode of the VIN terminal (D-60), the
  anti-parallel diode pair across the shunt ladder (D-66) and the diode
  clamp of VOUT to a 5.6 V node (D-70).
- **hardware:** the picture `board-open-connections.png` and, in the
  hardware guide, the list and the map of the 28 open connections: no
  connection is open after the layout review.
- **hardware:** the empty folder `hardware/simulation/` and, in the
  hardware guide, the list of the simulations that had to be filed: the
  simulations are in `simulation/`, made again from the final netlist
  (D-94).
- **firmware:** the ESP-IDF project of the first plan, for the ESP32-S3:
  its project file, its `main` and the build files of its components.
- **protocol:** the command CAL_WRITE and the event "power budget
  changed" of version 1. Their codes are not used again.

### Fixed

- **hardware:** five findings of the review of footprints and placement
  of 2026-10-13 are worked into the board of draft A2, at those points
  only and with no new routing. No part was added or moved, and the
  circuit did not change.
  - The pre-regulator U16 (TPS63020DSJR) stood on a library footprint
    for another package, with a thermal land of 1.70 mm × 3.30 mm. It
    has the land pattern of its own package now: a thermal land of
    1.58 mm × 2.85 mm, fourteen lands of 0.60 mm × 0.24 mm and eight
    paste openings, with the six vias under the part inside the smaller
    land. The Source Meter sheet, the project symbol, the board and the
    bill of materials name the new project footprint.
  - Reference texts of the silkscreen stood at another part of the same
    package or up to 7 mm from their own, and 123 were hidden. 61 texts
    are moved to their own part and 31 hidden ones are shown again: 330
    of the 425 are visible and 95 are hidden, those of the diodes D6,
    D11 and D17 among them. Every hidden reference is on the
    fabrication layer.
  - The text "USB DATA/POWER", which stood beside the console header
    J1, reads "PICO USB" over "DATA/POWER".
  - Nine vias whose hole reached into a pad or stood within 0.05 mm of
    one, where solder can drain into the hole, are moved by 0.2 mm to
    1.6 mm, to 0.25 mm or more from the pad. Two were left where no
    free place exists, at pin 6 of U12 and at pad 1 of R67.
  - Two tented vias that stood under the wall line of the shield can
    are moved inside it.

[Unreleased]: https://github.com/Guiimartinho/open-power-profiler/commits/main
