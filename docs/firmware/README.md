# Firmware Design

Design of the firmware of the instrument: what has to be built, how it is
divided, which state machines it has and in which order it is written and
tested. It goes one level below section 6 of
[the specification](../specification.md), which stays the baseline: where
this design and the specification disagree, the specification wins and the
difference is an error of this document.

Status: a design. Nothing described here is implemented beyond the parts
listed under [What Exists](#what-exists), and nothing has run on a
controller. Figures quoted from the specification keep its evidence level
(calculated, simulated, datasheet); figures that this design adds are
estimates and say so.

| Page | Content |
| --- | --- |
| [Architecture](architecture.md) | Layers, components, ports, the two cores, the data path, budgets, design patterns, build and tests |
| [State machines](state-machines.md) | Device states, start, self-test, DUT power on and off, faults, the supervision cycle, the range rules of core 1 |
| [PIO programs](pio-programs.md) | Capture, range sequencer, output and trip: pins, sketches, timing by count, the emulator and its tests |
| [USB and host integration](usb-integration.md) | The link to the host: stack, frames, flow, what the protocol still lacks, how firmware and host are tested against each other |
| [Rules](rules.md) | The 36 firmware rules of section 6.6, each with the level it lives on, the component that holds it and the test that proves it |

## What the Firmware Has to Do

From sections 5 to 9 of the specification:

- Sample the converter at 100 kSPS through a PIO program and DMA, with the
  side data of every sample in the same word, and never drop a sample
  because the processor was busy (section 6.3).
- Run the range sequencer and the over-current trip as PIO programs, which
  act without the processors: jump to range 3 within 100 ns, make before
  break, blanking, a trip after 12 µs (rules F-16 to F-20).
- Bring the carrier up in a fixed order, supervise its rails, its budget
  and its temperature every 10 ms, and open every switch within 50 µs when
  the rails fall (rules F-1 to F-15).
- Start and stop the source and close and open the paths in the one order
  that the hardware bears (rules F-23 to F-33).
- Speak the protocol of section 7 over USB CDC: stream frames of 256
  samples, commands, responses and events.
- Keep the calibration record in flash (section 8).

## What Exists

Read from the repository on 2026-10-12.

| Part | State | Where |
| --- | --- | --- |
| Wire protocol codec | Done at version 2 of the protocol (D-103), unit-tested against the shared vectors: frames, sample word with its marks, stream payload, command envelopes, the records of GET_INFO and GET_STATUS, of the supply event and of the zero offsets | `firmware/components/proto` |
| Block queue between the cores | Done, unit-tested, also with two threads | `firmware/components/base` |
| Settling window after a range change | Done as an algorithm, unit-tested | `firmware/components/acq` |
| Step-down rule | Done as an algorithm, unit-tested | `firmware/components/acq` |
| Device state machine | Six states, ten events, one fault event without a cause | `firmware/components/app` |
| Raw capture word | Split into result and side bits, sample word with the over-range mark of F-35 and the output bit; unit-tested | `firmware/components/acq` |
| Under-range rule | F-22 as an algorithm: the flag after 100 samples below a code of 650, range 3 requested, no step down for 1 s after it, the reverse-current fault after 100 ms; unit-tested. Nothing calls it yet | `firmware/components/acq` |
| Pin table | Every pin of section 5 with direction, drive and pull; the checks of F-4 and F-5; unit-tested | `firmware/components/base` |
| DAC | The frame of F-6 and the nominal set-point code of F-30; unit-tested | `firmware/components/smu` |
| Monitor | Scale of the eight channels, source of the 5 V rail (F-11), rails test (F-12); unit-tested | `firmware/components/monitor` |
| Fault causes | The fifteen causes of section 6.4 and their flags of today; unit-tested | `firmware/components/app` |
| Timed sequences | A runner, and the four sequences of DUT power on and off with the test that F-28 asks for; unit-tested | `firmware/components/app` |
| Port to the analog front end | Six calls, older than draft A2; to be replaced | `firmware/components/afe` |
| Unit test project | CMake presets, Unity, coverage, clang-tidy, cppcheck | `firmware/test/host` |
| Build for the Pico 2 | A project of the Pico SDK: every component compiled for the RP2350, an image of 33 kB that starts the console and the state machine and drives no pin beyond the console output | `firmware/CMakeLists.txt`, `firmware/main` |
| Host package | Protocol, transports, client, capture, a device simulator | `host/` |

The rows from the raw capture word down were written on 2026-10-12, as
the first part of package 1 below, and the protocol codec was brought to
version 2 the same day. The core has 22 unit tests, all of its lines and
branches are covered, and clang-format, clang-tidy and cppcheck are clean
(run on Windows; the sanitizer builds need Linux).

What does not exist: any PIO program, any adapter, the components `cal`
and `usb_link`, the command handlers, the start
sequence and the self-test, the supervision cycle, the fault manager, and
the loop of core 1 that would run the sample rules. No
rule of section 6.6 is implemented end to end: [Rules](rules.md) says
which test exists for each.

Tools on the development machine on 2026-10-12: CMake, Ninja, MinGW gcc,
clang-tidy, cppcheck and gcovr for the unit tests, and, installed that
day on the owner's decision, the Pico SDK 2.3.1 with TinyUSB, the Arm
GNU Toolchain 15.2.Rel1, `pioasm` 2.3.1 and `picotool` 2.3.1. `openocd`
is not installed; it is needed for a debug probe only.

## How the Work Is Cut

The order follows one rule: what can be proven on a PC is written and
proven first, and hardware is added behind interfaces that already have a
fake. Each package ends with its gates of section 18.3 green.

| Package | Content | Needs | Done when |
| --- | --- | --- | --- |
| 1. Core | Fault causes, pin table, clock and timers, the step sequencer with the start, stop and power sequences, monitor logic (rails test, budget, CC class, thermal), source logic (DAC frame, set-point, F-30 to F-33), calibration record, command handlers, status | Nothing new | Unit tests for every rule that is logic; coverage floors kept |
| 2. PIO programs | Acquisition, range sequencer, trip, with an emulator that runs them on the PC cycle by cycle | `pioasm`, or the programs assembled by a tool of the repository | Timing of F-16, F-17, F-18 and F-34 shown in the emulator |
| 3. Target build | Pico SDK project, adapters for pads, PIO and DMA, SPI, watchdog, flash, USB, interrupts; the composition root | Pico SDK and the ARM compiler | A `.uf2` image builds with warnings as errors. The project and a first image exist; no adapter is written |
| 4. Link to the host | Version 2 of the protocol ([USB and host integration](usb-integration.md)), firmware core and host package tested against each other on the PC | Package 1 | Host tests pass against the firmware core. The protocol is done on both sides; the device twin is not written |
| 5. On the controller | Test application on a Pico 2, then on the risk prototype of phase 1 | A Pico 2, an ADC evaluation module | Exit criteria of phase 1 (section 13) measured and filed as a report |

Packages 1 to 4 need no hardware. Package 5 needs boards.

## Decisions

Decided by the project owner on 2026-10-12, each on the recommendation of
this design:

| Decision | What it is | State |
| --- | --- | --- |
| Tools of the Pico 2 on the development machine | The Pico SDK with TinyUSB, the ARM GNU toolchain, `pioasm` and `picotool` | Decided and installed: SDK 2.3.1, Arm GNU Toolchain 15.2.Rel1, `pioasm` and `picotool` 2.3.1 |
| Version 2 of the protocol (D-103) | Marks for over-range, under-range and the output in the sample word, one fault flag for each cause, a wider status and information record, the budget as a current, notices, the calibration record as one block | Decided and done in the definition, in both codecs and in their tests ([USB and host integration](usb-integration.md)) |
| The trip in PIO 2 (D-104) | GATE_OUT and the over-current trip are a program of their own; no pin moves | Decided; section 5 of the specification names the block |
| The project of the first controller | Removed in the change that adds the Pico SDK project, so that the build gate of section 18.3 names one build that exists | Decided and done |

Still open, each with one recommendation. None is taken until the owner
says so.

1. Real-time kernel on core 0. The specification leaves it to phase 1,
   from measured latencies. Recommended: start without one, as a
   cooperative loop with two interrupt handlers, and keep the loop bodies
   short enough that a kernel can be put under them later.
2. Emulator for the PIO programs. The specification leaves the choice to
   phase 1. Recommended: a small cycle-accurate emulator written in C in
   the test tree, with the programs assembled by `pioasm`, whose source
   the host build fetches. The emulator found in the investigation of
   2026-10-10 handles one state machine and has no IRQ instruction, by
   its own project page ([PIO programs](pio-programs.md)).
3. The old port `pp_afe_port_t`. Recommended: replace it by the ports of
   [Architecture](architecture.md); it predates draft A2 and no adapter
   implements it.
4. Where firmware evaluates F-21 and F-22 (open in section 16).
   Recommended: F-21 in a monitor tick of 125 µs that reads the two pins,
   F-22 on core 1 in chunks of 64 samples instead of whole blocks.
5. Faults while waiting in BOOT and the way back after PWR_GOOD was lost
   (open in section 16). Recommended: the flag is reported in BOOT without
   a change of state, and CLEAR_FAULT leads from FAULT to BOOT when the
   bring-up has to run again ([State machines](state-machines.md)).
6. A command that starts a timed sequence is answered when the sequence
   ends, up to 3.3 s for DUT power on in ampere mode. Recommended: yes;
   it changes no frame and gives the host one result per command.
7. The console and rules F-1, F-2 and F-7. Read word for word, the rules
   release every pin toward the carrier and drive none while PWR_GOOD is
   low, and GP0 and GP1 are pins of the headers. They lead to the console
   header alone and into no carrier circuit (section 5), and what the
   rules guard are the inputs of carrier parts. The image of today
   drives the console output from its start. Recommended: say in the
   three rules that the console pins are not among the lines they
   cover, so that a carrier that does not come up can still be
   diagnosed on the console.

## What This Design Cannot Settle

- The reaction time of the sequencer on the real controller: the emulator
  counts clocks, the pad and comparator delays are measured in phase 1.
- The sustained USB throughput and the processor load: measured in
  phase 1.
- Whether the acquisition window at 15 MHz holds for the 500 kSPS option:
  an open check of section 16.
- Every constant that the rules take "from the first board": the thermal
  limit, the input model of the budget, the qualification time of the
  trip.
