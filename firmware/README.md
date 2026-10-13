# Firmware

Firmware of the controller of the instrument, a Raspberry Pi Pico 2
(RP2350). Its job is to acquire the samples, to control the analog front
end and to stream the data to the host over USB. It is written in C11 and
built with the Pico SDK. Three programs for the PIO blocks of the RP2350
will do what has to act without the processors; none of them is written
yet.

## Status

As of 2026-10-12. In short: this directory is a project of the Pico SDK
that builds an image for the Pico 2, around a hardware-independent core
with unit tests that run on a PC. The image starts the console and the
device state machine. Beyond the console output it drives no pin: every
line into the circuits of the carrier board stays released. No adapter
for any hardware, no PIO program and no USB link is written, and none of
the 36 firmware rules that guard the carrier board of draft A2 is
implemented end to end. The image has not been loaded onto a board:
nothing here has run on a controller.

The design of the whole firmware is written:
[`docs/firmware/`](../docs/firmware/README.md) has the architecture, the
state machines, the PIO programs as sketches, the USB link and the place
and the test of every rule. On 2026-10-12 the project owner decided four
of its points:

- the tools of the Pico 2 on the development machine;
- version 2 of the host protocol, which holds what draft A2 reports
  (D-103);
- the output switch and the over-current trip as a program of their own
  in PIO 2, with the range sequencer in PIO 1 and no pin moved (D-104);
- the removal of the project of the first plan, for the ESP32-S3, in the
  change that added the project of the Pico SDK.

Six points of the design are open, each with one recommendation; the
[design](../docs/firmware/README.md#decisions) lists them.

What exists:

| Part | State | Where |
| --- | --- | --- |
| Build for the Pico 2 | A project of the Pico SDK with the preset `pico2`. It compiles the seven components of the core for the RP2350 with warnings as errors and links them with a composition root that starts the console on UART0 and the device state machine. The `.uf2` image is 33 kB. Beyond the console output it drives no pin, it has no USB function, and it has not been loaded onto a board | `CMakeLists.txt`, `CMakePresets.json`, `cmake/`, `main/main.c` |
| Wire protocol | Done and unit-tested at version 2 of the protocol (D-103): CRC-16, frame encoder and incremental parser, sample word with its marks, stream payload, envelopes of commands, responses and events, and the codec of the four records with a fixed layout: the data of GET_INFO and of GET_STATUS, of the event "supply changed" and of the zero offsets. No handler fills a record yet | `components/proto` |
| Block queue between the two cores | Done and unit-tested, also under two real threads | `components/base` |
| Raw word of the capture | Done and unit-tested: the split into conversion result and side bits, the fields of the side bits, the over-range test of rule F-35, and the sample word built from a raw word with code, range, logic inputs, the output bit and the over-range mark (sections 4.7 and 7.3) | `components/acq/include/acq/pp_raw.h` |
| Settling window after a range change | Done and unit-tested as an algorithm. The window length is an argument: the core holds no default of seven samples | `components/acq/src/pp_range_tracker.c` |
| Step-down rule of the ranging | Done and unit-tested: N consecutive samples (default 100), hold-off of four blocks. The thresholds are an argument; no value is stored | `components/acq/src/pp_downrange.c` |
| Under-range rule | Done and unit-tested, with the numbers of rule F-22 as defaults: the flag after 100 samples below a code of 650 while a mode pair is closed, range 3 requested at once and again while the range does not follow, no step down until 1 s after the flag cleared, the reverse-current fault after 100 ms below the level in range 3. Nothing calls it yet | `components/acq/src/pp_underrange.c` |
| Device state machine | Done and unit-tested: the six states of section 6.4 and the transitions between them. One fault event without a cause | `components/app/src/pp_fsm.c` |
| Fault causes | Done and unit-tested: the fifteen causes of section 6.4 as a set, each with the fault flag of the protocol that reports it; the watchdog restart is reported without the FAULT state. Nothing raises a cause yet | `components/app/src/pp_fault.c` |
| Timed sequences | Done and unit-tested: a runner for tables of steps, and the four tables of DUT power on and off in both modes, with the test that rule F-28 asks for. The actions and checks of the steps are operations that the caller supplies; nothing implements them yet | `components/app/src/pp_sequence.c`, `pp_power_seq.c` |
| Pin table | Done and unit-tested: every pin of section 5 with direction, drive strength and pad pull; the checks of rules F-4 and F-5. The composition root takes the two console pins from it; no adapter applies it to the pads yet | `components/base/src/pp_pins.c` |
| Set-point DAC | Done and unit-tested: the frame of rule F-6, the limits and the nominal code of F-30. Calibration at nine points and the chip selects are not written | `components/smu/src/pp_dac.c` |
| Monitor channels | Done and unit-tested: codes to millivolts with the nominal factors of section 4.2, which input supplies the 5 V rail (F-11), the rails test (F-12). The scan, the averages, the CC class, the budget and the thermal limit are not written | `components/monitor/src/pp_monitor.c` |
| Port to the analog front end | Declared, with a test double. It has six calls and predates draft A2; the design replaces it | `components/afe/include/afe/pp_afe_port.h` |
| Unit test project | CMake presets, Unity, 22 tests, coverage, clang-format, clang-tidy and cppcheck | `test/host` |

What does not exist:

| Part | State |
| --- | --- |
| PIO programs | None written. The three are sketches in the [design](../docs/firmware/pio-programs.md): capture in PIO 0, range sequencer in PIO 1, output switch and over-current trip in PIO 2 (D-40, D-104). None is assembled, none has run in an emulator or on a controller |
| Emulator for the PIO programs | Not written and not chosen: section 18.2 of the specification leaves the choice to phase 1 |
| Adapters (`port/` directories) | None written: the pads from the pin table, capture through PIO and DMA, the sequencer and the trip, SPI to the DAC and the monitor converter, USB, flash, watchdog, the two interrupt handlers |
| The loop of core 1 | Not written. Nothing runs the raw word, the settling window, the step-down rule and the under-range rule on a stream of samples, and no block is built |
| Components `cal`, `usb_link` | Empty directories with a placeholder file; neither build lists them |
| Command handlers, start sequence, self-test, supervision, fault manager | Not started in `app` |
| Device twin | Not written: the core built as a program for the PC, against which the host package would be tested ([design](../docs/firmware/usb-integration.md#tested-together-without-hardware)) |
| Firmware rules F-1 to F-36 | None implemented end to end. Tests exist for the tables, scales and sequence orders behind F-4 to F-6, F-11, F-12, F-23 to F-26 and F-28 to F-31, for the under-range rule of F-22 and the step-down rule beside it, and for the settling window and the over-range test of F-35; [the rules page](../docs/firmware/rules.md) says what each test covers |

The controller changed from the ESP32-S3 to the Pico 2 with decision D-39 of
the [specification](../docs/specification.md). The core does not depend on
the controller, so it and its tests carried over as they were, and the
build followed on 2026-10-12. Sections 5 and 6 of the specification
describe the pins and the execution model that the adapters have to
follow.

Nothing here has run on a board, neither on a Pico 2 nor on a carrier:
timing, throughput and loss figures are design targets of the
specification, and the figures quoted below are its calculated, simulated
or datasheet values, not measurements.

What the code is worth today. Its logic is checked on a PC by the unit
tests, the coverage floors and the static analysis described under
[Host Tests](#host-tests) and
[Formatting and Static Analysis](#formatting-and-static-analysis), and it
compiles for the RP2350 with warnings as errors. The results of
2026-10-12, run locally on Windows: 22 tests pass; lines and branches of
the core are covered in full, 997 lines and 540 branches, against floors
of 90 % and 80 %; clang-format, clang-tidy and cppcheck report nothing;
the image builds without a warning. The sanitizer builds need Linux or
macOS and did not run. That shows that the code does what its tests say;
it shows nothing about timing or about the hardware.

## What Draft A2 Asks of the Firmware

The specification was rewritten for draft A2 of the carrier board, and the
firmware it describes is much larger than the code here. This section
follows sections 5 to 8 and 16 of the specification and compares each item
with the tree. "In the core" means hardware-independent code with a unit
test; it never means code that has run on a controller.

### Pins and PIO Blocks (Section 5)

Section 5 fixes the 26 GPIO pins of the headers; every one is in use. One
file holds the pin numbers, `components/base/include/base/pp_pins.h`, and
the table behind it gives each pin its direction, its drive strength and
its pad pull. A unit test checks the table against section 5 and rules
F-4 and F-5. No adapter exists yet to apply it to the pads; the
composition root uses it for the two pins of the console. The table
takes these groups from section 5, and nothing else decides a pin:

| Pins | Signals | Peripheral |
| --- | --- | --- |
| GP0, GP1 | DBG_TX, DBG_RX | Console, UART0 |
| GP2 to GP6 | GATE_R1 to GATE_R3, MUX_A0, MUX_A1 | Outputs of the range sequencer, PIO 1 |
| GP7 | GATE_OUT | Output of the trip program, PIO 2 (D-104) |
| GP8, GP9 | CMP_UP, CMP_JUMP | Inputs of the range sequencer, PIO 1 |
| GP10 | CMP_OC | Input of the trip program, PIO 2 |
| GP11 | VIN_OV | Input, interrupt source (F-27) |
| GP12 to GP15, GP22 | SPI_MISO, MON_CS, SPI_SCK, SPI_MOSI, DAC_CS | SPI1 to the monitor converter and the DAC; the two chip selects are plain outputs (F-6) |
| GP16, GP17 | ADC_DOUT, SIDE_DOUT | Inputs of the acquisition state machine, PIO 0 |
| GP18 | SMU_ON | Output, enable of the pre-regulator |
| GP19 to GP21 | ADC_SCK, ADC_CNV, SIDE_LOAD | Outputs of the acquisition state machine, PIO 0 |
| GP26, GP27 | GATE_SRC, GATE_AMP | Outputs, the two mode requests |
| GP28 | PWR_GOOD | Input, interrupt source (F-7) |

The trip program also reads the range line GATE_R3 on GP4, which the
sequencer drives (D-104). The status indicator is the LED of the Pico 2 on
GP25. GP1, GP8 to GP12, GP16, GP17 and GP28 are driven by the carrier and
are never outputs (F-4). The supported module is the Pico 2 with headers; a
Pico 2 W fits the sockets, but the firmware does not support it, because
its LED and its VBUS sense are pins of the radio chip. Pin numbers, like
protocol numbers, are a breaking change when they move.

### Execution Model (Section 6)

| Item of the specification | State | What shows it |
| --- | --- | --- |
| Built with the Pico SDK, TinyUSB as the USB stack | The build exists and produces a `.uf2` image. TinyUSB is not linked: the image has no USB function, and its console is the UART | `CMakeLists.txt`, `main/main.c` |
| Core 1 runs the acquisition loop and nothing else; core 0 runs USB, commands, the monitor and the state machine as a cooperative loop (section 6.2) | Not started. `main` creates the state machine, prints its state on the console and idles; core 1 is not started | `main/main.c` |
| No interrupt and no driver call per sample: a PIO state machine clocks the converter and DMA moves blocks of 256 words (section 6.3, timing in F-34) | Not started. The block size exists as the protocol constant `PP_BLOCK_SAMPLES` | No `.pio` file in the tree |
| Range sequencer and over-current trip as PIO state machines (section 4.4, D-40, D-104, F-16 to F-20) | Not started; sketched in the design | No `.pio` file in the tree |
| Separation of the conversion result from the side bits | In the core | `components/acq/include/acq/pp_raw.h` |
| Decimation for the 500 kSPS option | Not started | |
| Sample word assembly | In the core: the word of section 7.3 with every mark, and the word built from a raw word | `components/proto/include/proto/pp_sample.h`, `components/acq/include/acq/pp_raw.h` |
| Settling window after a change of the range bits | In the core, without its default | `components/acq/src/pp_range_tracker.c` |
| Step-down rule evaluated on the block | In the core | `components/acq/src/pp_downrange.c` |
| Under-range rule evaluated on the block (F-22) | In the core as a rule per sample. Nothing calls it | `components/acq/src/pp_underrange.c` |
| Ring buffer between the cores; a full buffer drops the block and counts it | In the core. The capacity is an argument; the 64 blocks of section 6.3 are not set anywhere | `components/base/src/pp_blockq.c` |
| Stream frames with the count of dropped blocks | In the core | `components/proto/src/pp_stream.c`, `pp_frame.c` |
| Command frames parsed from a byte stream | In the core, envelope only. No command is executed | `components/proto/src/pp_frame.c`, `pp_command.c` |
| Response and event data of section 7.4 | In the core as a codec of the four records. No handler fills them | `components/proto/src/pp_report.c` |
| Monitor work every 10 ms: eight slow channels, rails test, budget, thermal limit, watchdog reload | Partly in the core: the scales of the eight channels, the source of the 5 V rail and the rails test, as arithmetic on one scan. The scan schedule, the averages, the CC class, the budget, the thermal limit and the watchdog reload are not started | `components/monitor/src/pp_monitor.c` |
| Interrupt handlers in RAM for PWR_GOOD and VIN_OV (F-7, F-27) | Not started | |
| USB CDC ACM with host-open detection; 500 mA declared in the descriptor (section 6.5, F-14) | Not started | `components/usb_link` is empty |

Whether a real-time kernel is added on core 0 is open: section 6.2 leaves
it to phase 1, to be decided from measured latencies. The design
recommends to start without one, as a cooperative loop with two interrupt
handlers; that is one of its six open points.

Section 6.3 also sets budgets for this model: the acquisition loop under
30 % of core 1 at 100 kSPS, a ring buffer of 64 blocks (about 66 kB,
164 ms of data) and under 20 ms from sample to host as a typical value.
They are design targets; none is measured, and the first figures come from
the prototypes of phase 1.

### Device State Machine and Fault Causes (Section 6.4)

`components/app/include/app/pp_fsm.h` declares ten events, and
`components/app/src/pp_fsm.c` holds the transition table for the six states
BOOT, SELFTEST, IDLE, ARMED, STREAMING and FAULT. The table agrees with the
diagram of section 6.4: a fault can be raised in every state after BOOT,
only CLEAR_FAULT leaves FAULT, the output cannot be switched off while
STREAMING, and a host that closes the port ends the stream. An event that
the state does not allow is refused.

What section 6.4 describes around that table is mostly not written:

| Item of section 6.4 | State |
| --- | --- |
| BOOT: release of every pin, wait for PWR_GOOD, bring-up in the order of F-3 | Not started |
| SELFTEST: rails test, ADC response, a pass through all ranges, start-up zero | Not started. The machine has the two events for its result |
| SET_MODE only in IDLE, and the other command rules of section 7.4 | Not started. They are decisions of the command handlers; today only the device simulator of the host package applies them |
| DUT_POWER on: the start sequence of F-28 in source mode, the checks of F-24 to F-26 in ampere mode, ARMED only 50 ms after GATE_OUT rose, FAULT when a check fails | Partly. The two sequences are tables of steps with a runner, unit-tested for their order and their times (`pp_power_seq.c`, `pp_sequence.c`). The operations behind their actions and checks are not written, and nothing starts a sequence: the machine goes from IDLE to ARMED on one event, which a handler will have to dispatch at the end of the sequence |
| DUT_POWER off: output pair first, then the mode pair, the source stopped as F-29 and F-31 say | Partly, in the same way: the two stop sequences are tables with tests |
| FAULT opens the output switch first and reports second | Not started. The machine calls an observer after each accepted event; the observer that opens the switch is not written |
| Bring-up of BOOT again after PWR_GOOD was lost and returned | Not started. The table has no event for it; section 16 lists the transition as an open point |
| Watchdog restart reported with its own fault flag (F-8) | The cause and its flag exist; nothing detects a watchdog restart |
| DUT_POWER on refused for 100 ms after an over-current trip (F-19) | Not started |

Fault causes. The machine has one event for every fault,
`PP_FSM_EVENT_FAULT_RAISED`, without a cause, plus
`PP_FSM_EVENT_SELFTEST_FAILED`. The cause travels as a set of fault flags.
Since version 2 of the protocol (D-103) the generated header
`proto/pp_proto_defs.h` defines fifteen of them, one for each cause.
Section 6.4 lists ten entries, three of which join two conditions, so
thirteen conditions in all, and adds the watchdog restart; rule F-19 adds
the "start over-current", a trip within 100 ms of GATE_OUT, which is
reported together with the over-current flag.

Inside the firmware `components/app/include/app/pp_fault.h` names the same
fifteen causes as a set of bits, and `pp_cause_wire_flags` translates a
set of causes into the flags of the wire. The causes are called
`PP_CAUSE_*`, because the generated header owns the names `PP_FAULT_*`.
The watchdog restart is a cause that is reported and does not need the
FAULT state (F-8). Nothing raises a cause yet, and the state machine
still takes one event without it. The table is the one of section 7.4:

| Cause in section 6.4 | Rules | Fault flag | Bit |
| --- | --- | --- | --- |
| Over-current trip | F-18, F-19 | Over-current | 0 |
| Power limit | F-14 | Power limit | 1 |
| Thermal limit | F-15 | Thermal | 2 |
| Failed self-test | Section 6.4 | Self-test | 3 |
| Sequencer that does not act | F-21 | Sequencer not acting | 4 |
| Reverse current | F-22 | Reverse current | 5 |
| Loss of PWR_GOOD ("carrier supply") | F-2, F-7 | Carrier supply | 6 |
| Failed rails test ("analog rails") | F-3, F-12 | Analog rails | 7 |
| VIN over-voltage | F-27 | VIN over-voltage | 8 |
| VIN out of range (absent or reversed) | F-26 | VIN out of range | 9 |
| Overload of the source output | F-32 | Source overload | 10 |
| External voltage on the source output | F-33 | External voltage | 11 |
| Failed start check (ladder node in ampere mode, output of the source) | F-25, F-28 | Start check | 12 |
| Trip within 100 ms of GATE_OUT | F-19 | Start over-current, set together with bit 0 | 13 |
| Watchdog restart | F-8 | Watchdog restart | 14 |

The reports that the rules name beside the faults have a place in the
protocol since version 2: the count of consecutive trips is a field of
the status record (F-19), and "set-point settled" (F-30) and "DUT above
the set-point" (F-31) are notices, sent as an event and held as two bits
of the status record. No firmware counts a trip or sends a notice yet.

The state machine and its header were written before draft A2. The comment
of `PP_FSM_EVENT_FAULT_RAISED` still names three causes, over-current trip,
power limit and thermal; the list above is the one of the specification.

### The 36 Rules That Guard Hardware (Section 6.6)

The carrier is built so that no state of the controller pins damages it.
What the parts cannot do is left to firmware, and section 6.6 is the one
list of it (D-81): rules F-1 to F-36, each with its figure and with what it
guards. The section also asks that the hardware-independent logic of every
rule has a unit test before a board is powered for the first time. The
groups below are the six tables of that section. "In the core" in the last
column means logic with a unit test, not a rule that is kept: nothing
calls that logic yet.

| Group | Rules | What they ask | State |
| --- | --- | --- | --- |
| Controller pins and start | F-1 to F-9 | Every pin released after any restart (F-1); nothing driven while PWR_GOOD is low, valid after 10 ms (F-2); the bring-up order after PWR_GOOD (F-3); the pins that are never outputs, with a unit test of the pin table (F-4); drive strengths and pad pulls (F-5); the two chip selects and the DAC frame `0x5000 \| code`, with a unit test (F-6); the reaction to a falling PWR_GOOD within 50 µs (F-7); the watchdog with a time-out of 100 ms or less (F-8); no header pin as activity indicator of the boot loader (F-9) | F-4 and F-5: the pin table with its unit test, in the core; nothing applies it to the pads. F-6: the DAC frame with its unit test, in the core; the chip selects not. The others not started |
| Monitor, supplies and budget | F-10 to F-15 | Eight monitor channels at 100 SPS (F-10); which input supplies, read from the channel of the 5 V rail at 1.67 V (F-11); the rails test every 10 ms (F-12); the CC class after three equal readings (F-13); the budget as an input current of 0.45 A, 1.4 A or 1.7 A and the 4.25 V limit of the rail (F-14); the thermal limit (F-15) | F-11: in the core. F-12: the windows of the rails test in the core; the 10 ms average and what a failure opens not. F-10 and F-13 to F-15 not started |
| Range sequencer and over-current trip | F-16 to F-22 | R3 forced within 100 ns of the jump comparator, never blanked (F-16); the step-up comparator ignored for 2.0 µs after a range change, make-before-break (F-17); the trip after 12 µs in R3, latched (F-18); no retry, 100 ms of refusal, trips counted (F-19); the sequencer started after the rails test, GATE_OUT only through the trip program (F-20, D-104); a sequencer that does not act found within 2 ms (F-21); under-range below a code of 650 for 100 samples, R3 requested, no step down for 1 s, reverse-current fault after 100 ms (F-22) | F-16 to F-18 are PIO programs: sketched in the design, not written. F-19 and F-20 are what firmware does around them (refusal, counting, the order of the start): not started. F-21 is firmware: not started. F-22: the rule is in the core, see below |
| Path switches | F-23 to F-27 | The output pair closes last and opens first, 5 ms between mode requests (F-23); the order and the times of DUT power on, 40 ms and 50 ms (F-24); the ladder node no higher than VIN plus 0.1 V before the ampere pair closes (F-25); VIN inside 0.6 V to 5.4 V for 100 ms (F-26); the VIN_OV interrupt (F-27) | F-23 to F-26: the order and the times are in the tables of the four sequences, with their unit test; the comparisons that the checks of the steps stand for are not written. F-27 not started |
| Source | F-28 to F-33 | The one start sequence of the source, with a unit test (F-28); the stop order (F-29); the set-point limited to 0.80 V to 5.00 V and calibrated at nine points (F-30); a DUT above the set-point followed down (F-31); overload (F-32); an external voltage above 5.3 V within 0.5 ms (F-33) | F-28: the sequence as a table, with the unit test that the rule asks for. F-29 and F-31: the order of the stop sequence, in the same tables. F-30: the limits and the nominal code in the core; the nine points and "settled" not. F-32 and F-33 not started |
| Acquisition and samples | F-34 to F-36 | The timing of the acquisition program (F-34); which samples are flagged: the settling window, 5 ms after a change of supply, over-range (F-35); when a zero calibration may read (F-36) | F-34 and F-36 not started; F-35 partly, see below |

The figures in the table are those of the specification: calculated,
simulated or taken from datasheets, as each rule says. None is measured.

The circuit simulations of 2026-10-10 ran against these rules without any
firmware: the [simulation guide](../simulation/README.md) has the account.
The range sequencer and the trip are a model of F-16 to F-18 in them,
because no program exists. The 100 ns of F-16, the blanking of F-17 and
the 12 µs of F-18 are inputs of those runs and not results. Phase 1
measures the reaction time of a PIO state machine (F-16); phase 3 runs
the programs of the sequencer and the trip and confirms the qualification
time (F-18). Where a run steps down or closes a switch, the instants are
written into it as the rules give them.

The simulations touched the text of these rules, and section 6.6 follows
them since 2026-10-10. One rule changed, by decision D-99: F-18 counts its
time with R3 selected only. In F-8, F-23 and F-24 the simulated figure
replaced the earlier one in the reason of the rule. F-26, F-28, F-34 and
F-35 now state a simulated fact that their limits do not hold, and each
is an open question of section 16 of the specification. F-7 waits for a
new run of the benches. The four decisions D-95 to D-98 change hardware
and no rule. All figures are simulated:

- **F-18.** While a large capacitor at the DUT recharges after a step to
  1.0 A, the over-current comparator is high for up to 12.83 µs, longer
  than the 12 µs of the rule, of which 8.174 µs at the most in R3. No run
  trips, because the model counts in R3 only. Decided (D-99): the rule
  says that the 12 µs count with R3 selected, the program has to count
  the same way, and the test of section 11 reads the comparator in R3.
- **F-7.** PWR_GOOD is below 2.0 V at the controller pin 145.1 µs after
  the supervisor falls, not 0.05 ms to 0.07 ms after the rails begin to
  fall, and below 0.8 V after 604.5 µs, not 0.3 ms. The flag still does
  not lead the rails. These times are those of the rail monitor with
  1 nF at its comparator inputs. Decision D-95 makes the three capacitors
  10 nF, which is drawn since 2026-10-10: the nodes are ten times slower
  (calculated), and the times have not been simulated again with that
  value.
- **F-8 and F-24.** The gate of the output pair is below 2 V within 7 µs
  at 5 V (6.288 µs). At 0.8 V it takes 7.274 µs to 7.448 µs, and 8.109 µs
  in the slow corner. With 100 µF the DUT voltage is at 10 % 6.458 ms
  after GATE_OUT at 5 V and 5.381 ms at 0.8 V, and over the corners the
  start spans from 3.888 ms to 10.03 ms. The reasons of F-8 and F-24
  carry these figures now; they had 7 µs and 6 ms to 7 ms.
- **F-23.** A mode pair that opens under load lifts VIN to 16.01 V to
  25.12 V. The reason of the rule carries the figure now; it had an
  estimate of 20 V to 24 V.
- **F-26 and F-28.** With the input leakage of the monitor converter at
  its datasheet maximum of 1 µA, the VIN channel reads 430 mV off, where
  F-26 allows 130 mV before calibration, and the channel of the ladder
  output 120 mV off, where F-28 asks for 100 mV. Both rules state this
  now. Whether the limits change is an open question; the recommendation
  is to keep them and to check the leakage of the part against 0.30 µA.
- **F-34.** By its datasheet the converter shifts its result out after
  convert-start has fallen; section 4.6 says so now, where it said "while
  that line is still high". With one reading instant for weak and for
  strong pads the
  reading window at 15 MHz, the 500 kSPS option, is 11.43 ns against the
  13.33 ns of two system clocks; at 100 kSPS the window is far wider.
- **F-35.** A step of the 5 V rail of more than 0.3 V flags samples for
  5 ms. A load step of 1 A moves the rail by 282.3 mV behind a cable of
  0.15 Ω, which is an assumption, so ordinary load steps come close to
  that level: the 1.34 A that section 4.1 calculates for 5 V at 1 A would
  move the rail by about 0.38 V (calculated from the 0.28 V per ampere of
  that run). The rule states the 282.3 mV now. Its threshold is an open
  question; the recommendation is to keep 0.3 V until the rail is
  measured in phase 4.

What the core already holds of these rules:

- **F-35, settling window and over-range: partly.**
  `pp_range_tracker_update` flags the sample whose range bits differ from
  the sample before it and the ones that follow, the first one included,
  and a new change restarts the count. That is the rule. The length is the
  argument `settle_samples` of `pp_range_tracker_init`: the default of
  seven samples (D-76) is not a constant of the core, and nothing stores
  it. A test uses seven as one of its lengths, no more. The third sentence
  of F-35 is in the core as well: `pp_raw_is_over_range` in
  `acq/pp_raw.h` is true for a code of 65535 or a set CMP_JUMP bit, and
  the sample word that `pp_raw_sample_word` builds from a raw word carries
  the mark. The second sentence is not started: the 5 ms after a change of
  the supplying input or a step of the 5 V rail.
- **Step-down rule (section 4.4): in the core.** `pp_downrange.h` has the
  default of 100 consecutive samples and the hold-off of four blocks that
  section 4.4 gives, and SET_DOWN_N maps to
  `pp_downrange_set_consecutive`. The thresholds are a table of ADC codes
  the caller supplies. Section 4.3 gives the levels as currents, 60 µA for
  R1, 1.8 mA for R2 and 60 mA for R3, and calls them firmware thresholds;
  neither these values nor their conversion to codes is in the code.
- **F-22, under-range: in the core, not yet joined to the step-down
  rule.** `components/acq/src/pp_underrange.c` holds the rule per sample.
  With a mode pair closed, 100 samples in a row below a code of 650 raise
  the flag, and every sample is marked from there until the first one
  that is not below the level. When the flag rises range 3 is requested,
  also with the range locked, and the request is repeated while the range
  bits of the samples do not follow. A code that stays below the level in
  range 3 for 100 ms is reported once as a reverse current. The numbers
  of F-22 are defaults, counted in samples at 100 kSPS.
  `pp_underrange_blocks_step_down` says when no step down may be
  requested: while the flag is up, and for 1 s after it cleared.
  `pp_downrange_update` has no input for that, so the loop that runs both
  rules has to ask, and that loop is not written. One point is to be
  settled with it: section 6.3 evaluates the rule on a block of 256
  samples, which is 2.56 ms at 100 kSPS, and F-22 asks for R3 at the
  latest 2 ms after the first sample below the level. F-21 has the same
  limit of 2 ms and does not say where firmware reads CMP_JUMP. Section
  16 lists both as an open point; the design recommends chunks of 64
  samples on core 1 for F-22 and a monitor tick of 125 µs for F-21.
- **Port to the front end.** `pp_afe_port.h` has six calls: range lock,
  step down, mode, output, fault latch read and clear. Draft A2 needs more
  behind it: the request for R3 of F-22, the trip count of F-19, the times
  of F-23 and F-24, the checks of F-25 and F-26. The design replaces it
  by the ports of its
  [architecture page](../docs/firmware/architecture.md#ports): carrier
  pins, sequencer, output and trip. Whether the old port goes is one of
  the six open points.

### Calibration Record (Section 8)

`components/cal` is empty: no code for the record, no flash access, no
zero routine. The device sends raw codes and the host converts them, so
the firmware side of calibration is storage, identity, the zero
measurements and the few fields of the record that the device itself
uses.

Since 2026-10-12 section 8 gives the record a layout (D-103): 78 to 142
bytes with a mark, a version, its length, the chip identifier, the serial
number and the revision of the carrier, the settling window, the
calibration temperature, the offset of the VIN channel, the thermal
limit, gain and offset of each range, the closed-switch zero of R0, up to
16 points of the DAC and a CRC. That layout is a design. No firmware
component stores the record, and no code on either side builds or parses
it.

| Item of section 8 | State |
| --- | --- |
| The record in the flash of the module, in the layout of section 8, with its mark, version, length and CRC | Not started. The layout exists in the specification only |
| CAL_STORE, accepted in IDLE only: mark, version, length and CRC checked; the chip identifier of a record that arrives is zero or the one of the device, which writes its own before it stores the record; a record with another identifier is refused | Not started |
| CAL_READ returns the record, and no data when none is stored | Not started |
| The 64-bit chip identifier of the RP2350 in the record; a record with another identifier is reported as not calibrated and nominal values are used (D-82) | Not started. The three states of the calibration have their codes in the protocol and their field in the data of GET_INFO |
| Serial number and revision of the carrier, entered at calibration, so that the host can show them at every connection | Not started. Both are fields of the data of GET_INFO |
| A missing or corrupt record falls back to nominal values and is flagged in GET_INFO | Not started |
| What the device itself uses of the record: the settling window, the offset of the VIN channel (F-26), the thermal limit (F-15) and the points of the DAC (F-30) | Not started. `pp_range_tracker_init` takes the window length and `pp_mon_vin_mv` the offset as arguments |
| Open-switch zero of every range at start-up and on CAL_ZERO, 200 ms or more after the output opened (F-36), kept in RAM; taken again when the board temperature has moved by more than 2 °C with the output off | Not started. The four offsets have their record in the protocol: in the data of GET_INFO, as the response of CAL_ZERO and with the notice of a new zero |
| Closed-switch zero of R0 with the output on and the terminals open, 300 ms or more after GATE_OUT rose, stored and subtracted (D-59, F-36) | Not started. It is a field of the record, for the host |
| DAC set-points: nine points or more (D-58, F-30) | Not started. The record holds none, or 9 to 16 |
| No flash write while streaming or while a path is closed (section 6.3); only `cal` writes flash (section 6.1) | Not started |

How the record travels is decided (D-103): the host builds it and sends it
as one block with CAL_STORE, and CAL_READ returns it. The protocol carries
250 bytes at the most and does not look into them, so a change of the
fields is a new version of the record and not of the protocol. CAL_ZERO
returns the four zero offsets. The core has the codec of the zero offsets
and of the data of GET_INFO; the handlers of the three commands are not
written.

Two of the three constants that the rules of section 6.6 take from a board
have a field in the record: the offset of the VIN channel, stored at
calibration (F-26), and the thermal limit (F-15). The constants of the
input model behind the budget (F-14) have none in version 1 of the
record: they are defined on the first board and enter with a new version
of it, and section 16 lists them as an open point.

### Protocol Version 2 (Sections 7 and 16)

Draft A2 reports more to the host than version 1 of the protocol held.
The project owner decided on 2026-10-12 to add all of it in one step
(D-103), and [`protocol/definition.toml`](../protocol/definition.toml) is
at version 2. What the core holds of it; where the table says "codec",
an encoder and a decoder exist and are tested against the shared vectors:

| Item of version 2 | In the core | Where |
| --- | --- | --- |
| Sample word: the marks for over-range (bit 20) and under-range (bit 21) and the request of the output switch (bit 22), from which the host counts the 250 ms of output settling | Pack and unpack of every field, and `pp_sample_is_flagged`, which is true for a sample with the invalid, the over-range or the under-range mark: the samples that the host keeps out of its statistics | `components/proto/include/proto/pp_sample.h` |
| The invalid bit also for the 5 ms after a change of supply (F-35) | The bit is the one of the settling window; nothing sets it for a change of supply | |
| Fifteen fault flags, one for each cause of section 6.4 | The flags in the generated header, and the translation of a set of causes into them | `proto/pp_proto_defs.h`, `components/app/src/pp_fault.c` |
| Data of GET_INFO, 48 bytes: protocol version, hardware revision, firmware version, the state of the calibration, the settling window, the chip identifier, serial number and revision of the carrier, the four zero offsets | Codec | `components/proto/src/pp_report.c` |
| Data of GET_STATUS, 25 bytes: state, fault flags, dropped blocks, mode, locked range, set-point, VOUT, VIN, the 5 V rail, temperature, supply in use, CC class, budget in mA, consecutive trips, notices | Codec | The same file |
| Event "supply changed", 4 bytes: supply in use, CC class and the budget as a current (F-14, D-49) | Codec | The same file |
| The four zero offsets, 16 bytes, in 1/256 of a code: the response of CAL_ZERO and the data of the notice of a new zero | Codec | The same file |
| Event "notice": set-point settled (F-30), DUT above the set-point (F-31), a new zero taken (section 8) | The three codes in the generated header. Nothing builds the event | `proto/pp_proto_defs.h` |
| Commands CAL_READ and CAL_STORE, which carry the calibration record as one block | The two codes in the list of known commands. No handler | `components/proto/src/pp_command.c` |

Version 1 had a command that wrote one gain and one offset, and an event
that carried the budget in milliwatts. Both are gone, and their codes,
`0x31` and `0x02`, are not used again.

The codec is all that exists. The item "Protocol" of section 16 says what
is open: no firmware fills these fields yet. The handlers that would fill
a record, the supervision that would send an event and the component that
would store the calibration record are not written.

Section 16 also lists three points of sections 6 to 8 that are not
decided, and the firmware cannot build on them before each has a
decision: where the constants of the input model of F-14 are kept, the
step in which firmware evaluates F-21 and F-22, and how a fault is
reported while firmware still waits in BOOT, with the transition that
runs the bring-up again when PWR_GOOD returns. The design gives a
recommendation for the second and for the third.

For every later change the order stays the one of section 7.5: the
definition file first, with the generated files and the shared test
vectors, then the code on both sides. No protocol number is ever typed in
C. The header `proto/pp_proto_defs.h` is generated; a new state or
command then stops the build where the code has to follow, for instance
at the list of known commands in `pp_command.c`. A change that an older
peer cannot read is a breaking change and raises the protocol version.

## Next Steps for the Firmware

The [design](../docs/firmware/README.md#how-the-work-is-cut) cuts the work
into five packages by one rule: what can be proven on a PC is written and
proven first, and hardware comes behind interfaces that already have a
fake. Packages 1 to 4 need no board. The phases are those of section 13
of the specification; a phase is closed by a report with measurements,
and none is closed.

1. **Decisions of the project owner.** Six points of the design are open,
   each with one recommendation: a real-time kernel on core 0; the
   emulator for the PIO programs; the old port `pp_afe_port_t`; where
   firmware evaluates F-21 and F-22; how a fault is reported while
   firmware waits in BOOT, with the way back after PWR_GOOD was lost; and
   a command that is answered when the sequence it started ends. The
   fourth and the fifth are open points of section 16 of the
   specification. None is taken until the owner says so.
2. **The rest of the core, on the PC (package 1).** No board is needed
   for: the default of the settling window as a stored constant, the 5 ms
   mark of F-35, decimation and the block builder that runs the rules of
   core 1 on every sample; the scan schedule of the monitor, its
   averages, the CC class, the budget and the thermal limit (F-10, F-13
   to F-15); the calibrated set-point and rules F-31 to F-33 of the
   source; the logic of `afe` around the sequencer and the trip (F-19 to
   F-21, F-23 to F-27); the start sequence and the self-test; the command
   table with the rules of section 7.4, its handlers and the status
   record; the fault manager and the supervision cycle; the calibration
   record and the zero calibration in `cal` (section 8, F-36); the
   transmit scheduler of `usb_link`. Each comes with its unit test.
3. **PIO programs in an emulator (package 2).** The capture, the range
   sequencer and the trip as `.pio` files, run cycle by cycle on the PC:
   the timing of F-34, the reaction of F-16 from every state, the
   blanking of F-17 and the qualification of F-18, each by count of
   clocks. Which emulator is the second open point. An emulator counts
   clocks; the delays of pads and comparators are measured on the
   controller.
4. **Adapters and the composition root (package 3).** The pads from the
   pin table, PIO and DMA, the slow bus, the watchdog, flash, USB through
   TinyUSB and the two interrupt handlers, each behind its port, and
   `main` with both cores. The project and a first image exist; no
   adapter is written.
5. **Firmware and host tested against each other (package 4).** The core
   built as a program for the PC, the device twin, and one list of
   scenarios run against it and against the simulator of the host
   package. The protocol is done on both sides; the twin is not written.
6. **Risk prototypes on a Pico 2 with an ADC evaluation module (package
   5, phase 1).** The first step on hardware is the image that exists,
   loaded onto a Pico 2. Exit criteria of the phase: the converter
   captured through PIO and DMA at 100 kSPS and at 500 kSPS for
   10 minutes with no lost sample; the side data read in the same word;
   the reaction of a PIO state machine to an input edge measured, 100 ns
   or less from the pin of the jump comparator to the gate line of
   range 3 (F-16); USB throughput of 500 kB/s or more sustained; CPU load
   recorded. The decision on a real-time kernel for core 0 follows from
   the measured latencies. If the sustained rate stays below 500 kB/s,
   section 6.5 packs a sample into 3 bytes before the sample rate is
   lowered, which is a change of the protocol. The test application on
   the controller module starts here.
7. **Acquisition on the analog front end (phase 2).** Settling window
   confirmed with a fast load step; the oversampling decision, which
   decides whether `acq` decimates.
8. **Range sequencer and trip (phase 3).** The PIO programs of the
   sequencer and the trip (F-16 to F-18, started and used as F-19 and F-20
   say) run on the controller after the emulator; the qualification time
   of the trip confirmed (12 µs by default, F-18); F-21 and F-22 with the
   reverse-current test.
9. **Source and supplies (phase 4).** `smu` and `monitor`: start and stop
   of the source, set-point, budget, rails test, thermal limit (F-10 to
   F-15, F-23 to F-33), against the tests of R-08, R-09, R-13 and R-14.
10. **Carrier board (phase 5).** Start-up rules F-1 to F-9 on the real
    board. Before a DUT is connected, section 16 asks for the output open
    within 100 ms of a halted supervision task (F-8) and for gate lines
    that stay low across a restart.
11. **Calibration and protocol freeze (phase 6).** The calibration
    procedure on a board, with the record in flash, and the layouts of
    section 7.4 made final.

## Architecture

The firmware follows the ports and adapters pattern. The logic of the
instrument is plain C11 that knows nothing about the SDK of the controller
or about the board. It reaches the hardware only through small interfaces,
the ports. Adapters implement the ports with the drivers of the SDK, and
`main` wires the two together. This guide describes the tree as it is; the
[architecture page](../docs/firmware/architecture.md) of the design
describes the whole firmware, with its ports, its two cores and its data
path.

```text
main                 composition root: the only code that includes SDK headers
 │
 ├──► app            device state machine, fault causes, timed sequences
 ├──► acq            raw word, settling window, step-down and under-range rules
 ├──► afe            port to the analog front end
 ├──► smu            DAC frame, nominal set-point
 ├──► monitor        scales of the monitor channels, rails test
 │     │
 │     ▼
 ├──► proto          CRC, frames, sample word, stream, commands, records
 │     │
 │     ▼
 └──► base           status codes, byte access, block queue, pin table
```

Dependencies point downward only. `base` depends on nothing, and no
component below `main` includes an SDK header in its hardware-independent
part.

| Component | Hardware-independent part | Depends on | State |
| --- | --- | --- | --- |
| `base` | Status codes, little-endian access, block queue, pin table | Nothing | Done |
| `proto` | CRC-16, frame encoder and parser, sample word, stream payload, command, response and event envelopes, the four records of protocol version 2 | `base` | Done, at version 2 of the protocol |
| `acq` | Raw word of the capture with the over-range test, settling window after a range change, step-down rule, under-range rule | `base`, `proto` | Those four done. Decimation, the 5 ms mark after a change of supply, the block builder that runs them on every sample and the PIO capture adapter not started |
| `afe` | Port to the analog front end | `base`, `proto` | Port declared, no source file. Range lock and fault logic, the PIO programs of the sequencer and the trip and the adapters not started |
| `app` | Device state machine, fault causes, sequence runner, sequences of DUT power on and off | `base`, `proto` | Those done. Command handlers, start sequence, self-test, fault manager, supervision and watchdog not started |
| `smu` | DAC frame and nominal set-point code | `base` | That done. Calibrated set-point, rules F-31 to F-33 and the SPI adapter not started |
| `monitor` | Scale of the eight channels, source of the 5 V rail, rails test | `base`, `proto` | Those done. Scan schedule, CC class, budget, thermal limit and the SPI adapter not started |
| `cal`, `usb_link` | As in section 6.1 of the specification | | Not started: each directory holds a placeholder file only, and the builds do not list them |
| `main` | Composition root | Every component above | `main.c` for the Pico SDK: the console on UART0 (GP0 and GP1, 115200 baud), the device state machine and its state on the console. It creates no adapter, starts one core only and drives no pin but the console output |

`base` holds what every other component shares, so that none of them has to
depend on a sibling for a status code or for the queue. The components are
the ones of section 6.1 of the specification.

### Rules for the Hardware-Independent Code

- C11, compiled with the same strict warnings on the PC and for the
  target, and a warning is an error in both builds.
- No SDK header, no dynamic allocation, no global variable. Every function
  works on a context structure and on buffers the caller supplies.
- Every number of the wire protocol comes from `proto/pp_proto_defs.h`, which
  is generated from [`protocol/definition.toml`](../protocol/definition.toml).
  Do not edit the header; change the definition and run
  `python protocol/generate.py`.
- Every GPIO number comes from `base/pp_pins.h`, which follows section 5 of
  the specification. No other file holds one.
- Public names start with `pp_`.
- Each module has a unit test, and the coverage gate below applies to it.

### Patterns and Why They Are Used

- **Ports and adapters.** The core is tested on a PC in milliseconds, without
  a board, and before the board exists. It is also why the change of
  controller left the core untouched. `afe/pp_afe_port.h` is the first port;
  `test/host/support/fake_afe.c` is its test double.
- **Lock-free single-producer, single-consumer queue** (`pp_blockq`). The
  acquisition side must never wait for the USB side. Each index has one
  writer, a full queue refuses the block and counts the drop, and the storage
  belongs to the caller.
- **Incremental parser** (`pp_frame_parser_feed`). Bytes arrive from USB in
  pieces of any size. The parser keeps its own state, does no input or
  output, and resynchronizes after noise or a damaged frame.
- **Table-driven state machine with an observer** (`pp_fsm`). The states and
  transitions of section 6.4 are one table that can be compared with the
  specification. The machine decides the next state; the observer performs
  the actions, so the machine itself touches no hardware.
- **Sequence as data** (`pp_sequence`, `pp_power_seq`). The order and the
  times of a rule are one table of steps, and a runner walks it without
  ever blocking. A test replaces the operations by a recorder and reads the
  table back against the text of the rule.
- **Caller-owned memory.** No heap means no fragmentation and no allocation
  failure in the real-time path, and it makes the memory budget visible.
- **One manifest per component** (`sources.cmake`). The target build and the
  host test build read the same list of sources and dependencies through
  `cmake/pp_components.cmake`, so they cannot drift apart.

## Layout

```text
firmware/
├── CMakeLists.txt        Target project: the Pico SDK build for the Pico 2
├── CMakePresets.json     Preset of the target build: pico2
├── cmake/
│   ├── pico_sdk_import.cmake   Helper that locates the Pico SDK (a copy from the SDK, under its own license)
│   ├── pp_components.cmake     List of components and the function that builds one, for both builds
│   └── pp_warnings.cmake       Warning flags, for both builds
├── main/
│   └── main.c            Composition root; the only file that includes SDK headers
├── components/
│   └── <name>/
│       ├── sources.cmake     Sources and dependencies of the component, for both builds
│       ├── include/<name>/   Public headers
│       ├── src/              Hardware-independent sources
│       └── port/             Adapters for the Pico SDK and PIO programs (none yet)
├── test/
│   ├── .clang-tidy       Static analysis checks relaxed for the tests
│   └── host/
│       ├── CMakeLists.txt    Host test project
│       ├── CMakePresets.json Presets: debug, coverage, sanitize, tsan
│       ├── generated/        Protocol test vectors (generated)
│       ├── support/          Test doubles
│       └── tests/            One test file per module
├── gcovr.cfg             Coverage filters and thresholds
├── .clang-format         Code format
├── .clang-format-ignore  Generated protocol files, which the formatter skips
├── .clang-tidy           Static analysis checks
└── .clangd               Editor support: points clangd at the host test build
```

The components `base`, `proto`, `acq`, `app`, `smu` and `monitor` have
this structure. `afe` has a header and no `src/` yet. `cal` and `usb_link`
are empty directories. Both builds write into a `build/` directory beside
their project file, which is not committed.

## Building the Firmware

The image for the Pico 2 is built with CMake and the Pico SDK, from this
directory. The image is `build/pico2/power_profiler.uf2`.

| Tool | Version of the build of 2026-10-12 | How the build finds it |
| --- | --- | --- |
| Pico SDK | 2.3.1, with its TinyUSB submodule | The variable `PICO_SDK_PATH` |
| Arm GNU Toolchain | 15.2.Rel1, which is `arm-none-eabi-gcc` 15.2.1 | The variable `PICO_TOOLCHAIN_PATH`; without it the SDK looks for `arm-none-eabi-gcc` on `PATH` |
| `picotool` | 2.3.1 | Optional. The variable `picotool_DIR` names the folder that holds `picotoolConfig.cmake`; without it the SDK builds its own `picotool` |
| CMake 3.20 or later, Ninja | | On `PATH` |

In Git Bash, or in a shell of Linux or macOS:

```sh
export PICO_SDK_PATH=<folder of the Pico SDK 2.3.1>
export PICO_TOOLCHAIN_PATH=<folder of the Arm GNU Toolchain>
export picotool_DIR=<folder that holds picotoolConfig.cmake>   # optional
cd firmware
cmake --preset pico2
cmake --build --preset pico2      # writes build/pico2/power_profiler.uf2
```

In PowerShell the three variables are set as `$env:PICO_SDK_PATH = "..."`
and so on, and the two `cmake` commands are the same.

Result on 2026-10-12, on Windows with the tools of the table: the build
completes with warnings as errors and without a warning. The `.uf2` image
is 33 kB and the binary in it 16 kB. `picotool info` reads the name
"Open Power Profiler", version 0.1.0, the RP2350, SDK 2.3.1 and the board
`pico2` from the image.

What the build shows and what the image is:

- Every component of the core compiles for the RP2350, from the same
  manifests as the unit tests and with the same strict warnings.
- The linker keeps only what `main` calls. Of the core that is the device
  state machine and the names of the status codes. The codecs, the rules
  and the tables are compiled and are not in the image, because nothing
  calls them yet.
- By its source it starts the console on UART0, GP0 and GP1 at
  115200 baud, prints its version, the protocol version and the state of
  the device state machine, which is BOOT, and then idles.
- The console output on GP0 is the one pin it drives. That line leads
  straight to the console header of the carrier and to none of its
  circuits (section 5 of the specification). Every other line stays as
  the reset leaves it, released, and the resistors of the carrier then
  hold every switch open, keep the pre-regulator off and leave both
  converters of the slow bus unselected.
- No adapter is in it, no PIO program and no USB function: the host
  protocol is not on the wire yet.

The image has not been loaded onto a board, so nothing of this has run on
a controller. Loading needs no programmer: the Pico 2 shows a USB drive
when it starts with its BOOTSEL button held, and the `.uf2` file is copied
to it.

The `build` job of the `Firmware` workflow runs the same two `cmake`
commands on Ubuntu, with the `gcc-arm-none-eabi` package of the
distribution and the SDK fetched at its tag 2.3.1. It was rewritten with
the project and has never been started: the workflows start by hand only
(D-22).

`pioasm` 2.3.1 is on the development machine as well and is not used yet:
no PIO program is written. `openocd`, which serves a debug probe, is not
installed and not needed for this build.

## Host Tests

The unit tests compile the hardware-independent sources with a native
compiler and run them with [Unity](https://github.com/ThrowTheSwitch/Unity),
which CMake downloads at the first configuration. They need CMake 3.20 or
later, Ninja and gcc. They depend neither on the controller nor on the
Pico SDK.

Run these from `firmware/test/host`:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

On 2026-10-12 the run has 22 tests, and all pass:

| Component | Tests |
| --- | --- |
| `base` | `status`, `bytes`, `blockq`, `blockq_threads`, `pins` |
| `proto` | `crc16`, `frame`, `sample`, `stream`, `command`, `report` |
| `acq` | `raw`, `range_tracker`, `downrange`, `underrange` |
| `afe` | `afe_port` |
| `smu` | `dac` |
| `monitor` | `monitor` |
| `app` | `fault`, `fsm`, `sequence`, `power_seq` |

How the tests are organized:

- One executable per module, `tests/test_<module>.c`, registered with CTest
  under the name of the module.
- The CRC, the frames, the sample word and the four records are checked
  against the vectors in `generated/pp_proto_vectors.h`. The host software
  checks the same vectors, so both ends of the protocol agree byte for
  byte.
- The frame parser is also tested with every chunk size, with noise, with
  damaged and truncated frames, and with frames hidden inside a false frame.
- The pin table is read back against section 5 of the specification and
  rules F-4 and F-5, and the tables of the four power sequences against the
  text of rules F-23 to F-31: a recorder stands in for the hardware and
  notes every action with its time.
- `blockq_threads` runs the queue under two real threads. It is built when
  POSIX threads are available.

### Coverage

```sh
cmake --preset coverage
cmake --build --preset coverage
ctest --preset coverage
cmake --build --preset coverage --target coverage
```

The last command runs [gcovr](https://gcovr.com/) with `firmware/gcovr.cfg`
and writes `build/coverage/coverage.html`. It measures the code under
`components/` only and fails below 90 % line coverage or 80 % branch
coverage. Running `gcovr` in `firmware/` prints the same report. On
2026-10-12 it reads 100 % of 997 lines and of 540 branches.

### Sanitizers

```sh
cmake --preset sanitize && cmake --build --preset sanitize && ctest --preset sanitize
cmake --preset tsan && cmake --build --preset tsan && ctest --preset tsan
```

`sanitize` builds with AddressSanitizer and UndefinedBehaviorSanitizer and
`tsan` with ThreadSanitizer. Both need Linux or macOS: MinGW on Windows ships
no sanitizer runtime. If ThreadSanitizer stops with "unexpected memory
mapping", run the tests with address space randomization disabled, for
example `setarch $(uname -m) -R ctest --preset tsan`. Neither preset has
run on the code of 2026-10-12: they run in the `Firmware` workflow, which
has not been started since.

## Formatting and Static Analysis

These targets belong to the host test project. Run them from
`firmware/test/host` after `cmake --preset debug`:

| Command | What it does |
| --- | --- |
| `cmake --build --preset debug --target format` | Formats every C file with clang-format |
| `cmake --build --preset debug --target format-check` | Fails if a C file is not formatted |
| `cmake --build --preset debug --target tidy` | Runs clang-tidy on the sources and the tests |
| `cmake --build --preset debug --target cppcheck` | Runs cppcheck on `components/`, `main/` and the test doubles |

- `.clang-format`: four spaces, 100 columns, the brace of a function on its
  own line, the star next to the name.
- `.clang-tidy`: the bugprone, cert, clang-analyzer, misc, performance,
  portability and readability groups, with every finding an error. The file
  lists each disabled check with its reason. `test/.clang-tidy` relaxes one
  check for the tests.
- cppcheck runs with the warning, style, performance and portability checks
  at the exhaustive level, and any finding fails the run.
- The generated protocol headers are excluded from all three.
- `main/main.c` needs the headers of the Pico SDK, so clang-tidy does not
  analyze it. clang-format and cppcheck cover it, and the target build
  compiles it with `-Wall -Wextra -Werror`.

On 2026-10-12 the three checks report nothing, run locally on Windows.

## Adding Code

- **A hardware-independent module:** put the header in
  `components/<name>/include/<name>/`, the source in `src/`, add the source
  to `sources.cmake`, write `test/host/tests/test_<module>.c` and register it
  with `pp_add_test` in `test/host/CMakeLists.txt`. Run the `format` target
  on the new files.
- **A component:** give it a `sources.cmake`, add its name to
  `PP_COMPONENTS` in `cmake/pp_components.cmake`, in the order of the
  dependencies, and add its include directory to the cppcheck target in
  `test/host/CMakeLists.txt`. Both builds then compile it.
- **An adapter:** it goes in `components/<name>/port/`, is compiled by the
  target build only and is never listed in `sources.cmake`. It implements a
  port declared in a public header and is created in `main`. None exists
  yet, so the lines of the target build that list the adapters come with
  the first one. No adapter drives a line of the carrier before the rule
  of section 6.6 that guards the line has its unit test.
- **A protocol constant:** change `protocol/definition.toml` at the root of
  the repository and regenerate. The compiler then points at the code that
  has to follow, for instance the list of known commands.
- **A firmware rule of section 6.6:** put its hardware-independent logic in
  the component that section 6.1 names for it, as a module with its unit
  test like any other. The specification asks for that test before a board
  is powered for the first time. Refer to the rule by its ID (F-22, not
  "the under-range rule" alone), so that the code can be compared with the
  specification. The [rules page](../docs/firmware/rules.md) of the design
  names the component and the test of each rule.
- **A PIO program:** a `.pio` file beside the adapter that loads it, in
  `components/<name>/port/`. None exists yet. Section 13 asks that the
  programs of the range sequencer are verified in an emulator and on the
  controller, and section 18.2 that every program runs in an emulator on
  the PC before it runs on hardware.
