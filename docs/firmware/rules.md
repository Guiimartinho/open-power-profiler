# Firmware Rules: Where Each Lives and What Proves It

Section 6.6 of [the specification](../specification.md) lists 36 rules
that guard the carrier board and asks that the hardware-independent logic
of every rule has a unit test before a board is powered. This page gives
each rule a home in the design and names that test. The text of a rule is
in the specification and is not repeated here.

Status: a plan with a first part done. On 2026-10-12 these tests exist,
each in `firmware/test/host/tests/`:

| Rule | What is tested today | Test |
| --- | --- | --- |
| F-4, F-5 | The whole pin table | `test_pins.c` |
| F-6 | The DAC frame; not yet the chip selects | `test_dac.c` |
| F-11, F-12 | The scales, the two bands, the windows of the rails test; not yet the average and what opens | `test_monitor.c` |
| F-22 | The rule on samples: flag, request for range 3, hold of the step down, fault after 100 ms. Not yet its call from the loop of core 1, nor the 2 ms with chunks of 64 | `test_underrange.c` |
| F-23 to F-26, F-28, F-29, F-31 | The order and the times of the four sequences; the comparisons that their checks stand for are not written | `test_power_seq.c`, `test_sequence.c` |
| F-30 | The limits and the nominal code; not yet calibration and "settled" | `test_dac.c` |
| F-35 | The settling window; over-range by the code and by the jump bit; the bits of the marks in the sample word. Not yet the 5 ms after a change of supply | `test_range_tracker.c`, `test_raw.c`, `test_sample.c` |

Every other test below is to be written with its code.

"Where" is the context of [Architecture](architecture.md): a PIO program,
the power-fail interrupt, the monitor tick, the loop of core 0 (start,
sequences, supervision) or the loop of core 1.

## Controller Pins and Start

| Rule | Where | Component | Test on the PC |
| --- | --- | --- | --- |
| F-1 | Start, first step | `app`, `afe` | Start sequence: the release of every pin comes before any other call |
| F-2 | Start | `app` | Start sequence: nothing driven until PWR_GOOD has been high for 10 ms; an interruption restarts the 10 ms; the flag after 1 s |
| F-3 | Start | `app`, `monitor` | Start sequence: chip selects, bus, DAC zero, scan, rails test from 200 ms, programs, lines low, range 3; fault after 3 s |
| F-4 | Pin table | `base` | The table: GP1, GP8 to GP12, GP16, GP17 and GP28 are inputs, and no function can make them outputs |
| F-5 | Pin table | `base` | The table: 12 mA on GP2, GP3, GP4, GP7 and GP26, 4 mA elsewhere; the pulls; no isolation |
| F-6 | Slow bus | `smu`, `monitor` | DAC frame: the upper four bits are 0101 for every code; bus model: the selects are never low together, both high before the first clock |
| F-7 | Power-fail interrupt; supply guard in PIO 2 | `afe` | The order as a list: GATE_OUT low, state machines stopped, outputs low then released, selects released undriven; one edge latches and masks; emulator: the guard |
| F-8 | Supervision | `app` | The reload follows a complete cycle only; a missing scan means no reload; the restart flag ends in IDLE with the DAC at zero |
| F-9 | Restart into the boot loader | `app` | The call names no activity pin |

## Monitor, Supplies and Budget

| Rule | Where | Component | Test on the PC |
| --- | --- | --- | --- |
| F-10 | Monitor tick | `monitor` | Schedule: each channel once per 10 ms; channels 0, 2 and 7 never faster; the fast reading of channel 0 only in source mode with the output on |
| F-11 | Supervision | `monitor` | Both bands, the 1.67 V limit, a result above 5.7 V counts as a low rail |
| F-12 | Supervision | `monitor` | Every window at and beside its limit; code 0 and 4095; a converter that does not answer; the 10 ms average; the order of what opens |
| F-13 | Supervision | `monitor` | Three equal classifications change the class, two do not; above 2.04 V is invalid |
| F-14 | Supervision | `monitor` | The three budgets; more than the default only on USB-C; the 10 ms average of current and rail |
| F-15 | Supervision | `monitor` | The limit; the order: source pair, then SMU_ON |

## Range Sequencer and Over-Current Trip

| Rule | Where | Component | Test on the PC |
| --- | --- | --- | --- |
| F-16 | PIO 1 | `afe` | Emulator: range 3 within 15 clocks of an edge at every clock of a scenario, locked or not |
| F-17 | PIO 1 | `afe` | Emulator: overlap and blanking counted; the table: both gates on in every overlap pattern |
| F-18 | PIO 2 | `afe` | Emulator: 12 µs trips, less does not, time outside range 3 does not count; the build refuses a time outside 10 µs to 20 µs |
| F-19 | Loop of core 0 | `afe` | Refusal for 100 ms; no retry; the count; "start over-current" within 100 ms of GATE_OUT |
| F-20 | Start and power-on sequences; PIO 2 | `app`, `afe` | Sequences: range 3 before a mode pair; emulator: GATE_OUT rises in the trip program only |
| F-21 | Monitor tick | `afe` | High for more than 1 ms with range 3 not selected forces the output off by 2 ms; shorter does not; a full-scale code alone does not |
| F-22 | Loop of core 1, then core 0 | `acq`, `app` | 100 samples below 650 flag and ask for range 3 within 2 ms, in chunks of 64; no step down until 1 s after; the fault after 100 ms in range 3 |

## Path Switches

| Rule | Where | Component | Test on the PC |
| --- | --- | --- | --- |
| F-23 | Every sequence | `afe` | The output closes last and opens first; the two requests are never high together; 5 ms after a request fell; low before release |
| F-24 | Power-on sequences | `app` | The order and the 40 ms and 50 ms of both modes |
| F-25 | Power-on sequence, ampere mode | `app` | GATE_AMP only below VIN plus 0.1 V or below 0.3 V; the fault after 3 s |
| F-26 | Supervision | `monitor`, `app` | 100 ms inside 0.6 V to 5.4 V before closing; open within 20 ms below 0.6 V; the scale of 44 and the stored offset |
| F-27 | Power-fail interrupt | `afe` | The output off, GATE_AMP low for 5 ms or more, the fault |

## Source

| Rule | Where | Component | Test on the PC |
| --- | --- | --- | --- |
| F-28 | Power-on sequence, source mode | `app`, `smu` | The test that the rule asks for: the eight steps in order with their waits; no other path sets SMU_ON |
| F-29 | Stop sequence | `app`, `smu` | Output, source pair, SMU_ON, DAC zero; SMU_ON never low with a path closed |
| F-30 | Command handler, supervision | `smu` | 800 mV to 5000 mV; the code before calibration; nine points; settled after three readings in the band |
| F-31 | Supervision, stop sequence | `smu` | The DAC follows the reading down; no switch opens on a command before the 30 ms |
| F-32 | Supervision | `smu` | 0.3 V below for 20 ms with the set-point unchanged for 100 ms |
| F-33 | Monitor tick | `smu`, `monitor` | The comparison with 5.3 V on the fast reading; the output off in the same tick |

## Acquisition and Samples

| Rule | Where | Component | Test on the PC |
| --- | --- | --- | --- |
| F-34 | PIO 0 | `acq` | Emulator: every figure of the rule counted; the counts sent to the program pass a check of the same figures |
| F-35 | Loop of core 1 | `acq` | Seven samples from the change, restart on every change (exists); the 5 ms mark; over-range by code and by the jump bit |
| F-36 | Zero calibration | `cal`, `app` | No reading before 200 ms after GATE_OUT fell or a reset, 300 ms after it rose, 5 s after PWR_GOOD at the most |

## What No Test on the PC Shows

- Times on the pins: the 100 ns of F-16, the 50 µs of F-7, the 0.5 ms of
  F-33. The emulator and the fakes count clocks and calls; phase 1 and
  phase 3 measure.
- The pad registers as the adapter writes them (F-1, F-5), the watchdog
  reset of the pads (F-8) and the boot loader (F-9): a test application
  on the Pico 2 reads them back.
- Every constant that a rule takes from the first board.
