# Host Software

Python package that talks to the instrument: the wire protocol, the
transports, a device client, capture helpers and a simulated instrument for
working without hardware.

The package knows the instrument only through the wire protocol, so the
change of controller from the ESP32-S3 to the Raspberry Pi Pico 2 (D-39 of
the [specification](../docs/specification.md)) did not touch it. Its names,
`s3-power-profiler` and the `s3pp` command, date from the first controller
and are kept for now; see [Names](#names).

The package was written before draft A2 of the carrier board, and the
specification was rewritten for that draft. On 2026-10-12 the package
followed in one part: it speaks version 2 of the protocol, which holds what
draft A2 reports (D-103). It has not followed in the others: the nominal
calibration, the statistics and the application on top of the stack.
[What Draft A2 Asks of the Host Software](#what-draft-a2-asks-of-the-host-software)
lists the differences one by one, and [Next Steps](#next-steps) puts the
work in order.

## Status

As of 2026-10-12, in one sentence: the communication stack is written at
version 2 of the protocol and tested against a simulated instrument, the
application on top of it is not written, and nothing has run against
hardware, because no hardware exists yet and the firmware has no command
handler.

What exists, at version `0.1.0.dev0`, classified as pre-alpha in
`pyproject.toml`. Every module below has its own test module, except the
`Transport` interface in `transport/base.py`, which holds no logic, and the
client is tested end to end against the simulator:

| Layer | What it does today | Source |
| --- | --- | --- |
| `protocol` | CRC, frame encoder and incremental decoder with resynchronization, the sample word with its five marks, stream payload, the thirteen commands and the four events of version 2, and the four records with a fixed layout: the data of GET_INFO and of GET_STATUS, of the event "supply changed" and the zero offsets | `protocol/crc.py`, `frame.py`, `sample.py`, `stream.py`, `commands.py` |
| `transport` | The `Transport` interface, an in-memory pipe and a serial port adapter | `transport/base.py`, `memory.py`, `serial.py` |
| `device` | `DeviceClient`: one method per command, responses matched by sequence number, stream frames and events queued, link counters | `device/client.py` |
| `capture` | Gap detection from the sample index, conversion of codes into current with one gain and one offset per range, mean, minimum and maximum of a capture, with flagged samples counted and left out | `capture/reader.py`, `calibration.py`, `statistics.py` |
| `sim` | The instrument side of the protocol with the state rules of section 7.4, a synthetic waveform, a status and an identity filled from its configuration, and a calibration record kept as a block | `sim/simulator.py`, `loopback.py` |
| `cli` | `s3pp --version` and `s3pp simulate` | `cli.py` |

What is not written, most of it asked by sections 8 and 9 of the
specification:

- A capture command for a real instrument. `s3pp` has one subcommand,
  `simulate`. The serial adapter exists, but no command opens a port.
- The desktop viewer: live plot with min/max decimation, digital channels
  below the trace, controls for mode, voltage, DUT power, range and
  calibration. Its toolkit is not chosen.
- Selection statistics with charge and energy, and the mean of 100 samples
  in range 0.
- Export to CSV and to a compact binary format.
- Burst detection and the battery-life estimate.
- What the host does with the marks of draft A2. The codec decodes them
  since version 2, and the statistics leave flagged samples out. Not
  written: the fill of flagged samples with the first valid one after
  them, the mark of output settling, and logic channels shown as invalid
  at a low output voltage.
- The content of the calibration record. The client reads and stores the
  record as a block of bytes; no code builds one or reads a field of it.
- Anything that acts on the events "supply changed" and "notice": the
  codec builds and parses them, and the client queues events as they
  arrive.
- The guided calibration procedure. The scripts planned in
  [`tools/`](../tools/README.md) are not started either.

What the results are worth:

- The tests show that the code does what its tests say, against a simulator
  written from the same specification. They say nothing about the firmware,
  which has no command handler yet, and nothing about the instrument.
- This codec and the C codec of the firmware core reproduce the same shared
  vectors, the four records included. That is all that ties the two sides
  together today: no frame has passed between them.
- The serial adapter is tested only with the loopback port of pyserial. The
  throughput of the 100 kSPS stream through it has not been measured.
- Every current that the package prints comes from the nominal calibration
  table, a set of design targets. No value in this package is a measurement,
  and the nominal table does not match the nominal values of draft A2 yet.
- Results of 2026-10-12, run locally on Windows with Python 3.11: 500 tests
  pass; lines and branches are covered in full, 1461 statements and 264
  branches, against a floor of 90 %; mypy in strict mode finds nothing in
  41 files; ruff finds nothing. Linux, macOS and the other versions of
  Python have not run this code: the Host workflow covers them, and it
  starts by hand only (D-22).

## What Draft A2 Asks of the Host Software

This section follows sections 7, 8, 9 and 16 of the specification and
compares each with the code. Paths are relative to
`src/s3_power_profiler/`. Where a figure is marked "calculated", it was
computed for this comparison from the constants named beside it; nothing
here is measured.

### Nominal Calibration

The host converts a code into a current with
`I = (code − offset[r]) × gain[r]`, and the nominal gain is
`VREF / (65536 × G × R[r])` (section 8). `nominal_table()` in
`capture/calibration.py` implements that formula, with constants that
predate draft A2:

| Constant in `capture/calibration.py` | Code | Draft A2 | Where the specification says it |
| --- | --- | --- | --- |
| `NOMINAL_VREF` | 2.5 V | 2.5 V | Section 4.6 |
| `NOMINAL_AMPLIFIER_GAIN` | 20.0 | 19.93, set by the 523 Ω gain resistor R123 | Sections 4.5 and 8 |
| `NOMINAL_SHUNTS`, range 0 | 1000 Ω | 1 kΩ | Section 8 |
| `NOMINAL_SHUNTS`, range 1 | 33 Ω | 31.95 Ω: 33 Ω with the 1 kΩ of range 0 in parallel | Sections 4.3 and 8 |
| `NOMINAL_SHUNTS`, range 2 | 1 Ω | 0.999 Ω, for the same reason | Section 8 |
| `NOMINAL_SHUNTS`, range 3 | 0.1 Ω | 0.1 Ω | Section 8 |
| `NOMINAL_PEDESTAL` | 0.050 V, 1310.7 codes | About 1313 codes at zero current, from the divider R121 and R122 | Section 4.5 |

Range 0 has no switch and stays in circuit, so in ranges 1 to 3 the current
divides between the selected shunt and the 1 kΩ. That is why the nominal
resistance of a range is the parallel value and not the value printed on
the part.

The effect on the current of one code (calculated from both sets of
constants):

| Range | Code today | With the values of draft A2 | The code reads low by |
| --- | --- | --- | --- |
| R0 | 1.907 nA | 1.914 nA | 0.35 % |
| R1 | 57.80 nA | 59.91 nA | 3.5 % |
| R2 | 1.907 µA | 1.916 µA | 0.45 % |
| R3 | 19.07 µA | 19.14 µA | 0.35 % |

An error of 3.5 % in range 1 is larger than the accuracy target of R-05,
±1 % of reading ±0.1 % of range, which is 1.1 % of reading at full scale.
R-05 is a target after calibration, so the comparison shows the size of the
error and is not a test of the requirement. A real instrument is meant to
report its own calibrated table, so the nominal table matters in three
places: the simulator, the first look at an uncalibrated board, and the
fallback when the stored record is missing or corrupt (section 8).

The tests in `tests/test_calibration.py` hold the old values too and change
together with the constants:

- `FULL_SCALE_CURRENTS` is 100 µA, 3.03 mA, 100 mA and 1 A for 2.0 V above
  the pedestal. With the values of draft A2 that is 100.4 µA, 3.141 mA,
  100.5 mA and 1.004 A (calculated).
- `test_nominal_resolution_matches_the_specification` expects 58 nA per code
  in range 1; the table of section 4.3 says 60 nA.
- `test_nominal_offset_is_the_pedestal_in_codes` expects about 1300 codes;
  section 4.5 says about 1313.

The circuit simulations of 2026-10-10 give the values of draft A2 again,
from the netlist of the schematic: a gain of 19.93, a code of 1313 at zero
current, and 1.914 nA, 59.91 nA, 1.916 µA and 19.14 µA for one code in
ranges 0 to 3 (simulated;
[`signal_chain/transfer`](../simulation/results/signal_chain/README.md#signal_chaintransfer)).
With the whole path as one circuit the values are the same within the
last digit, 59.92 nA in range 1
([`system/accuracy`](../simulation/results/system/README.md#systemaccuracy)).
The simulations do not run this package: a bench converts codes with
arithmetic of its own and the nominal values of the specification, so its
result says nothing about `capture/calibration.py`.

### Calibration Record

Section 8 defines what an instrument stores in the flash of its module, and
section 9 what the host does with it. Since version 2 of the protocol the
record travels as one block (D-103): the host builds it and sends it with
CAL_STORE, and CAL_READ returns it. The package carries the block and does
not look into it. `DeviceClient.read_calibration` returns the bytes, empty
when the instrument has no record, and `store_calibration` sends 1 to 250
of them. No code builds a record or reads a field of one, so the layout of
section 8 is a design on this side as well.

| Item of section 8 | In the package today |
| --- | --- |
| Gain and offset of each range | Fields of the record, carried inside the block. Nothing reads them, so the conversion still uses the nominal table |
| DAC calibration at nine set-points or more (D-58, F-30) | Fields of the record: none, or 9 to 16 points of code and output. Carried inside the block; nothing builds them |
| Closed-switch zero of range 0 (D-59) | A field of the record, carried inside the block. No code uses it: `RangeCalibration` has one offset per range |
| Calibration temperature, offset of the VIN channel, thermal limit | Fields of the record, carried inside the block |
| Settling window length | A field of the record, and reported by GET_INFO as `DeviceInfo.settle_samples` |
| Mark, version, length and CRC of the record | Part of the block. The host has to write them when it builds a record; no code does |
| 64-bit chip identifier of the RP2350; "not calibrated" when it does not match | Reported by GET_INFO: `DeviceInfo.chip_id`, and `DeviceInfo.cal_state`, which tells a valid record of this chip from a missing or corrupt one and from the record of another chip |
| Serial number and revision of the carrier, shown at every connection (D-82) | Reported by GET_INFO: `DeviceInfo.carrier_serial` and `carrier_revision`. Nothing shows them: `s3pp simulate` prints the protocol version, the hardware revision and the firmware version |
| Fallback to nominal values, flagged in GET_INFO | `DeviceInfo.cal_state` carries the flag. `CalibrationTable.nominal` exists as a flag on the host side; nothing sets it from an instrument |
| Open-switch zero of each range | Reported as four offsets in codes, with a resolution of 1/256 of a code: `DeviceInfo.zero_offsets`, the return value of `DeviceClient.calibrate_zero`, and the notice of a new zero. Nothing puts them into a calibration table |

The data of GET_INFO is 48 bytes in version 2 and stays provisional until
the protocol is frozen in phase 6. `s3pp simulate` still always uses
`nominal_table()` (`run_simulate` in `cli.py`): the gains and offsets of
an instrument are in its calibration record, and nothing reads that.

The zero calibration (CAL_ZERO) takes no arguments and returns the four
zero offsets. The waits that guard it (200 ms after the output opened,
300 ms after it closed, F-36) belong to firmware; the simulator answers at
once, in IDLE only, with the offsets of its configuration.

The simulator keeps the block that CAL_STORE brings, in IDLE only, and
returns it on CAL_READ. It does not check the mark, the version, the
length, the CRC or the chip identifier, which section 8 asks of the
device: a block of any content makes it report a valid record, with the
serial number and the revision of its configuration.

### Simulator Constants

`SimulatorConfig` in `sim/simulator.py` holds the behavior of the simulated
instrument. Its defaults against the specification:

| Field | Default | Specification of draft A2 | Agrees |
| --- | --- | --- | --- |
| `settle_samples` | 4 | Seven samples flagged invalid after a range change, a stored constant (F-35; 45 µs to 0.1 % of range, simulated, section 4.5) | No |
| `down_n` | 100 | 100 samples for the step-down rule (section 4.4) | Yes |
| `min_voltage_mv`, `max_voltage_mv` | 800, 5000 | SET_VOLTAGE from 800 mV to 5000 mV (section 7.4, F-30) | Yes |
| `block_samples` | 256 | 256 samples in a stream frame (section 7.2) | Yes |
| `adc_low`, `adc_high` | 1311, 53740 | No requirement: the pedestal and 2.0 V above it, used as the ends of a triangle | Arbitrary |
| `waveform_period`, `range_dwell` | 2000, 1000 | No requirement: the simulator steps through the four ranges on a fixed schedule, whatever the code is | Arbitrary |
| `zero_offsets` | 1313 codes in every range | About 1313 codes at zero current (section 4.5) | Yes, as a nominal value; the nominal table of the host still has 1310.7 |
| `budget_ma` | 450 | 0.45 A while the input of the module supplies (F-14) | Yes |
| `chip_id`, `carrier_serial`, `carrier_revision` | 1, `SIM-0001`, 1 | No requirement: the identity that GET_INFO reports, the serial number and the revision only once a record is stored | Arbitrary |
| `rail5_mv`, `temperature` | 5000, 25.0 | No requirement: two readings of GET_STATUS | Arbitrary |

The host itself holds no settling constant: it trusts the invalid bit of
each sample. The seven samples matter for the simulator, which also
reports its value as the settling window of GET_INFO, and later for the
calibration record that carries the measured length.

What the simulator does not model, although the protocol carries it since
version 2, or because it is timing of the firmware:

- Over-range and under-range samples, and the 5 ms of invalid samples after
  a change of supply (F-22, F-35): it never sets the two marks. It sets
  the output bit in every sample of a stream, because a stream runs with
  the output on.
- The fault bit of the sample word: `_sample_word` never sets it.
- The conditions behind the faults. `Fault` has the fifteen flags of
  version 2 and `inject_fault` latches any of them, but nothing in the
  simulator raises one by itself.
- The events "supply changed" and "notice": it sends neither.
- Most readings of the status record. It reports the mode, the locked
  range, the set-point, the output voltage in source mode with the output
  on, and the 5 V rail, the temperature and the budget of its
  configuration. VIN, the supply in use, the CC class, the count of trips
  and the notices stay at their defaults.
- The start sequences behind DUT_POWER (F-24, F-28), which take tens to
  hundreds of milliseconds, and the 100 ms for which DUT_POWER on is refused
  after an over-current trip (F-19). The simulator changes state at once.
- A capture paced in real time. `Simulator.serve` sends one frame per turn.

The docstring of the module says what follows from this: the simulator
models the behavior of the protocol, and says nothing about the performance
of the instrument.

### Statistics, Marks and Display

Section 9 asks for more than `capture/statistics.py` does, and on one point
for something different.

| Section 9 asks | In the package today |
| --- | --- |
| Flagged samples (settling window, over-range, under-range) are not dropped: plot, mean, charge and energy use the first valid sample after them in their place (section 4.10) | Different. `StatisticsAccumulator.add` leaves the words with the invalid, the over-range or the under-range bit out and computes on the rest; `test_invalid_samples_are_counted_and_left_out` and `test_over_range_and_under_range_samples_are_left_out_too` fix that behavior. Leaving samples out shortens the time base: a mean moves little, but charge and energy, which are sums over time, would come out too small |
| Average, maximum, charge (µC and mAh) and energy of a selection | Mean, minimum and maximum of a whole capture. No charge and no energy: the package holds no sample period, and nothing joins the output voltage, which GET_STATUS reports since version 2, with the samples. No selection: statistics run over everything that was added |
| Mark for over-range samples: code 65535 or the jump comparator bit set (F-35) | Decoded as `Sample.over_range` and as a column of `SampleColumns`; the statistics leave these samples out. Nothing shows the mark |
| Mark for under-range samples: code below 650 for 100 samples (F-22) | Decoded as `Sample.under_range` and as a column; the statistics leave these samples out. Nothing shows the mark |
| Output settling: the samples of the first 250 ms after the output is switched on are marked and kept out of statistics below 1 µA; the time is a constant of the host (sections 4.2 and 8) | Not written. The instant is in the protocol since version 2: the first sample whose output bit is set after one in which it was clear (section 7.3), decoded as `Sample.output`. The package has no constant of 250 ms and no sample period to count it with, and it marks nothing |
| Logic channels shown as invalid while the monitored output voltage is below 1.67 V, with a setting for jumper position 2-3 of JP2; channels off by default (section 4.8) | Not written. The eight bits are decoded (`Sample.logic`, `SampleColumns.logic`), without any validity. GET_STATUS carries the output voltage since version 2 (`DeviceStatus.vout_mv`); nothing compares it with 1.67 V |
| Mean of 100 samples in range 0, the figure of the test of R-04 in source mode (section 11) | Not written |
| Serial number of the carrier shown at every connection | The number arrives with GET_INFO; nothing shows it. See [Calibration Record](#calibration-record) |
| Live plot with min/max decimation, digital channels below the trace | Not written |
| Controls for mode, voltage, DUT power, range and calibration | `DeviceClient` has a method for each command. No user interface |
| Export to CSV and a compact binary format | Not written |
| Burst detection and battery-life estimate | Not written |

What section 9 asks and the package has: the protocol layer without I/O,
transports behind one interface, the simulator, resynchronization on the
magic word, the CRC check and gap detection from the sample index
(`capture/reader.py`).

Samples with the fault bit are counted and kept in the statistics
(`fault_samples`). Section 7.3 names the bits that keep a sample out, the
invalid, the over-range and the under-range bit, and the fault bit is not
among them.

Two things that the circuit simulations of 2026-10-10 show concern what the
host presents to a user. Both are simulated, and neither is in section 9
([simulation guide](../simulation/README.md#what-the-runs-show-about-the-instrument)):

- The reading of range 0 follows the load with the time constant of shunt
  and capacitor: 100 µs with the 100 nF of the instrument alone, and
  1.1 ms with 1 µF beside the load. A sample taken after the settling
  window is valid and can still be on its way to the load current.
- After a fast step with a capacitor beside the load the instrument can
  rest one range above the one that the current asks for, and it stays
  there: the step down from range 3 comes only below 60 mA (the table of
  section 4.3). 80 mA is then read in range 3, where one code is 19.14 µA
  in place of the 1.916 µA of range 2.

### Protocol Version 2

Draft A2 reports more than version 1 of the protocol held, and section 16
of the specification listed the missing items as an open check. The
project owner decided on 2026-10-12 to add all of them in one step
(D-103). They are in
[`protocol/definition.toml`](../protocol/definition.toml), from which
`protocol/_defs.py` is generated, and in the codec of this package:

| Item of version 2 | In the package | What the host still has to do with it |
| --- | --- | --- |
| Marks for over-range and under-range samples (F-22, F-35), bits 20 and 21 of the sample word | `Sample.over_range`, `Sample.under_range`, the property `Sample.flagged` and two columns of `SampleColumns`; the statistics leave these samples out | Fill them with the first valid sample after them (section 9), and show the marks |
| The invalid mark also for the 5 ms after a change of supply (F-35) | The same bit as after a range change; nothing new to decode | Nothing beyond the fill of every flagged sample |
| The request of the output switch, bit 22 of the sample word: the instant from which the host counts the 250 ms of output settling | `Sample.output` and its column | The mark of output settling |
| One fault flag for each cause of section 6.4, fifteen in all | `Fault` in the generated `_defs.py`; `parse_fault_raised` and `DeviceStatus.faults` carry them | Tell the user why the output opened |
| The data of GET_STATUS, 25 bytes: after the state, the fault flags and the dropped blocks come the mode, the locked range, the set-point, VOUT, VIN, the 5 V rail, the temperature, the supply in use, the CC class, the budget in mA, the consecutive trips and the notices | `DeviceStatus` with `encode_device_status` and `decode_device_status` | Show them; use VOUT for the validity of the logic channels and for energy |
| The data of GET_INFO, 48 bytes: after the protocol version, the hardware revision and the firmware version come the state of the calibration, the settling window, the chip identifier, the serial number and revision of the carrier and the four zero offsets | `DeviceInfo` with `encode_device_info` and `decode_device_info`. Data shorter than 48 bytes is refused, with the protocol version that it names in the message | Show the serial number at every connection, and say when an instrument is not calibrated |
| The event "supply changed": supply in use, CC class and the budget as a current (F-14, D-49) | `SupplyReport`, `build_supply_changed`, `parse_supply_changed` | Read the event: the client queues events as they arrive and looks into none |
| The event "notice": set-point settled (F-30), DUT above the set-point (F-31), a new zero taken with its four offsets | `NoticeMessage`, `build_notice`, `parse_notice` | The same |
| CAL_ZERO returns the four zero offsets | `encode_zero_offsets`, `decode_zero_offsets`; `DeviceClient.calibrate_zero` returns them | Use them in the conversion |
| CAL_READ and CAL_STORE carry the calibration record as one block of 250 bytes at the most | `build_cal_read`, `build_cal_store`, `parse_cal_store`; `DeviceClient.read_calibration` and `store_calibration` | Build and parse the record of section 8 |

Version 1 had a command that wrote one gain and one offset, and an event
that carried the budget in milliwatts. Both are gone from the definition
and from this package, with the client method and the functions that
served them, and their codes are not used again.

The four fixed layouts are lists of fields in the definition. The
generator writes a struct format, the field names and the size of each
into `_defs.py` (`INFO_FORMAT`, `STATUS_FORMAT`, `SUPPLY_FORMAT` and
`ZERO_FORMAT` with their `_FIELDS` and `_SIZE`), and
`protocol/commands.py` packs and unpacks with those formats, so a layout
has one source. Record vectors in
[`protocol/vectors.json`](../protocol/vectors.json) hold this codec and
the one of the firmware to the same bytes.

What section 16 leaves open: no firmware fills these fields yet, and the
host does not use the marks beyond its statistics. Three points of
sections 6 to 8 are not decided there. One of them reaches this package
later: the constants of the input model of F-14 have no field in version 1
of the calibration record and enter with a new version of it.

The rule for every later change, from section 7.5: the definition file
changes first. Nobody types a protocol number in Python or in C.

1. Edit `protocol/definition.toml`, and `protocol/vectors.toml` when a
   vector should cover the change.
2. Run `python protocol/generate.py` from the root of the repository. It
   rewrites `protocol/_defs.py` here, the C header of the firmware and the
   shared vectors.
3. Update section 7 of the specification and record the decision in its
   log. A change that an older peer cannot understand raises the protocol
   version, which is 2 today.
4. Change the code in `protocol/commands.py`, then the simulator and the
   client, with their tests. The firmware codec follows in the same change.

The steps are described in full in the
[protocol guide](../protocol/README.md).

### Names

The distribution is `s3-power-profiler`, the module `s3_power_profiler` and
the command `s3pp` (`pyproject.toml`). "s3" stands for the ESP32-S3, the
controller that D-39 replaced, and the repository itself was renamed to
`open-power-profiler`. The description, the keywords and the help text of
the command no longer name the old controller; the three names still do.

No new names are chosen, and no decision on them is in the decision log.
A rename is a change of its own, because the names are used outside this
directory: in the path that `protocol/generate.py` writes to, in the Host
and Protocol workflows, in the protocol guide and in the changelog. Until
the owner of the project picks names, every command in this guide uses the
present ones.

## Next Steps

In order. Section 13 says that firmware and host software start in phase 1
and grow with each phase; the phase named with each step is the one whose
work needs it.

One step of the earlier list is done. Version 2 of the protocol is in the
definition, the generated files, the vectors, the codec, the simulator and
the client since 2026-10-12, with the firmware codec in the same change
(D-103). None of the steps below has started.

1. **Nominal values of draft A2** (before phase 1). Gain 19.93, shunts
   1 kΩ, 31.95 Ω, 0.999 Ω and 0.1 Ω and the pedestal of section 4.5 in
   `capture/calibration.py`, with the tests that hold the old values, and
   seven settling samples as the default of the simulator. This needs no
   protocol change and no hardware.
2. **Statistics that follow section 9** (phase 1). Flagged samples filled
   with the first valid sample after them, where they are left out today;
   charge and energy; the mark of output settling, counted from the output
   bit of the samples; the validity of the logic channels, from the output
   voltage of the status record; the mean of 100 samples in range 0. The
   simulator learns to produce each case, over-range and under-range
   samples included, so that the tests cover it.
3. **The calibration record on the host** (before phase 6, with the `cal`
   component of the firmware). Build and parse the record of section 8,
   with its mark, its version, its length and its CRC; make a calibration
   table from the record that CAL_READ returns; show the serial number of
   the carrier at every connection and say when an instrument is not
   calibrated. The layout is a design today, on both sides.
4. **Tested against the firmware core** (before the first run on a
   Pico 2). A transport that starts the firmware core built as a program
   for the PC, and one list of scenarios run against that program and
   against the simulator. The
   [firmware design](../docs/firmware/usb-integration.md#tested-together-without-hardware)
   describes it; neither the program nor the transport is written.
5. **Capture from a real port** (phase 1). A capture command in `s3pp`
   beside `simulate`, and the first run against the firmware on a Pico 2.
   The exit criteria of phase 1 include a sustained USB throughput of at
   least 500 kB/s; the serial adapter has to keep up with it, which is not
   measured.
6. **Recording and export** (phases 2 to 4). CSV and the compact binary
   format, so that the measurements of the analog phases are kept as files
   and can go into their reports.
7. **Calibration** (phase 6). The guided procedure of section 8 together
   with the scripts of `tools/`: per-range gain at two points,
   closed-switch zero, DAC at nine set-points or more, and the record
   written to the instrument at its end.
8. **Desktop viewer** (phase 6). Live plot with min/max decimation, digital
   channels, controls, selection statistics, burst detection and the
   battery-life estimate. The toolkit and any numeric library are open
   choices; the package has one runtime dependency today, pyserial.
9. **Protocol freeze** (phase 6). The provisional layouts of section 7.4
   become final. Phase 6 ends when R-05 (accuracy) and R-15 (capture, live
   view, statistics and export on Windows, Linux and macOS) are
   demonstrated end to end, with a report in `docs/reports/`.

## Architecture

The package is `s3_power_profiler`. It is built in layers, and each layer
depends only on what the table lists.

| Package | Role | Depends on |
| --- | --- | --- |
| `protocol` | Frames, CRC, sample word, stream payload, command, response and event envelopes, and the layouts of their arguments and data | Nothing |
| `transport` | The `Transport` interface and its adapters: `SerialTransport` and `MemoryTransport` | Nothing |
| `device` | `DeviceClient`: sends commands, matches responses, collects stream frames and events | `protocol`, the `Transport` interface |
| `capture` | `StreamReader` with gap detection, calibration, statistics | `protocol` |
| `sim` | `Simulator`, a deterministic fake instrument, and `connect_simulator` | `protocol`, the `Transport` interface; `connect_simulator` also uses `MemoryTransport` |
| `cli` | The `s3pp` command | `sim`, `device`, `capture`, `protocol` |

`errors` holds every exception of the package and is used by all layers.

```text
                 cli
                  │
      ┌───────────┼───────────┐
      ▼           ▼           ▼
     sim        device     capture
      │           │           │
      ├───────────┤           │
      ▼           ▼           ▼
  Transport    protocol ◄─────┘
  interface
      ▲
      │ implemented by
  SerialTransport, MemoryTransport
```

The design rules behind it:

- **The protocol does no I/O.** Encoders return bytes and decoders take
  bytes. `FrameDecoder` accepts chunks of any size, resynchronizes on the
  magic word and counts what it discards, so it can be tested with plain byte
  strings.
- **Ports and adapters.** `DeviceClient` and `Simulator` receive a
  `Transport` through their constructors and never import a concrete one. A
  serial port, an in-memory pipe or a test double are interchangeable.
- **Value objects are immutable.** Frames, samples, payloads and messages are
  frozen dataclasses that validate their fields when created.
- **One source for the protocol numbers.** `protocol/_defs.py` is generated
  from [`protocol/definition.toml`](../protocol/definition.toml) at the root
  of the repository. Do not edit it; change the definition and run
  `python protocol/generate.py`.
- **One place for the provisional layouts.** The arguments of the commands
  and the data of the responses and events are not frozen yet. They are
  defined only in `protocol/commands.py`, which the client and the simulator
  share. The four records with a fixed layout take their struct format from
  the generated `_defs.py`, so the definition decides their bytes too.
- **Typed errors.** Everything the package raises derives from
  `ProfilerError`. A wrong argument from the caller raises `ValueError`.

## Setup

Python 3.10 or later is required. From this directory:

```sh
python -m venv .venv
.venv/Scripts/python -m pip install -e ".[dev]"   # Windows
.venv/bin/python -m pip install -e ".[dev]"       # Linux and macOS
```

Activate the environment, or call its `python` directly, for the commands
below.

## Try It Without Hardware

The `simulate` command runs the client against the simulated instrument and
prints the statistics of the capture:

```sh
s3pp --version
s3pp simulate --blocks 100
s3pp simulate --blocks 100 --drop-every 10
s3pp simulate --blocks 20 --range 3
```

| Option | Meaning | Default |
| --- | --- | --- |
| `--blocks` | Stream frames to read, 256 samples each | 100 |
| `--drop-every N` | The instrument drops one block after every N frames, to show the gap detection | 0, never |
| `--range` | Lock one range, 0 to 3, instead of automatic ranging | Automatic |

`python -m s3_power_profiler` is the same command. It exits with 0, or with
1 if the stream stopped before all blocks were read. The currents in its
last line come from the nominal table and a synthetic waveform: they show
that the stack works, and are not readings of anything.

The same from Python:

```python
from s3_power_profiler.capture import StatisticsAccumulator, StreamReader, nominal_table
from s3_power_profiler.device import DeviceClient
from s3_power_profiler.sim import connect_simulator

transport, simulator = connect_simulator()
reader = StreamReader(start_index=0)
statistics = StatisticsAccumulator(nominal_table())

with DeviceClient(transport) as client:
    print(client.get_info())
    client.set_dut_power(True)
    client.start()
    for _ in range(10):
        payload = client.read_stream()
        if payload is None:
            break
        statistics.add(reader.push(payload).words)
    client.stop()

print(reader.gaps)
print(statistics.result())
```

For a real instrument, replace the first line of the session with
`transport = SerialTransport.open("COM5")`, using the port name of the
instrument (`SerialTransport` is in `s3_power_profiler.transport`), and
leave the simulator out. That path is tested only with the loopback port of
pyserial: no instrument and no firmware with command handlers exists to
answer it.

## Checks

Run these from this directory:

| Check | Command |
| --- | --- |
| Tests | `python -m pytest` |
| Tests with coverage | `python -m pytest --cov` |
| Type check, strict | `python -m mypy` |

Run these from the root of the repository, where the shared `ruff.toml` is:

| Check | Command |
| --- | --- |
| Lint | `python -m ruff check host` |
| Formatting | `python -m ruff format --check host` |

ruff and mypy are pinned in the `dev` extra of `pyproject.toml`, because
their findings change between releases. mypy runs in strict mode over `src`
and `tests`.

The Host workflow runs the tests on Windows, Linux and macOS with the
oldest and the newest supported Python (3.10 and 3.14), and on Linux with
the versions in between. Lint, formatting and the type check run once, on
Linux, and one more job builds the wheel, installs it in a clean
environment and runs `s3pp simulate`. The workflow starts by hand only
(D-22), so run the checks locally before a pull request. A local run covers
one system and one Python version.

How the tests are organized:

- One test module per source module, in `tests/`. The generated
  `protocol/_defs.py` and the `Transport` interface have none of their own,
  and `__main__.py` is run by the tests of the command.
- The CRC, the frames, the sample word and the four records are checked
  against [`protocol/vectors.json`](../protocol/vectors.json), the vectors
  shared with the firmware tests. The codec has to reproduce them byte for
  byte in both directions.
- Property tests cover the round trips, decoding with arbitrary chunk
  boundaries and resynchronization after noise and after a corrupted frame.
  They run with a fixed set of examples, so a run is repeatable.
- The client is tested end to end against the simulator, error paths
  included.
- Coverage is measured with branches. The run fails below 90 %; the
  configuration is in `pyproject.toml`. The floor is not a target: a change
  does not lower the coverage of the code it touches (section 18.3).
- No test needs an instrument. When such tests exist, they are kept apart
  from the ones that run anywhere (section 18.2).

## Layout

```text
host/
├── pyproject.toml           Package metadata and tool configuration
├── src/s3_power_profiler/
│   ├── protocol/            crc, frame, sample, stream, commands, _defs
│   ├── transport/           base (interface), memory, serial
│   ├── device/              client
│   ├── capture/             reader, calibration, statistics
│   ├── sim/                 simulator, loopback
│   ├── cli.py               The s3pp command
│   ├── __main__.py          python -m s3_power_profiler
│   └── errors.py            Every exception of the package
└── tests/                   One module per source module
```
