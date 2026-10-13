# Calibration

How the instrument turns ADC codes into amperes and DAC codes into volts,
and how the values behind that are found for one board and stored.

## Status

As of 2026-10-12. The calibration procedure is not written. It is planned
for phase 6 (section 13 of the [specification](../specification.md)), and
it cannot be written earlier: no board exists, and a procedure has to be
tried on one. Until then section 8 of the specification defines what is
calibrated, how the values are stored and which reference equipment is
needed. Since 2026-10-12 it also gives the stored record a layout, byte by
byte, and the host protocol carries that record as one block (D-103). The
protocol side exists, in the definition and in the code of firmware and
host that follows it. The layout is a design: no firmware stores a
record, and no code builds or parses one. This page summarizes the
section and says what is missing around it.

Nothing below is measured. Each figure is a datasheet value, a calculation,
a simulation or an estimate, as the specification marks it.

## What Section 8 Defines

The conversion runs on the host. The instrument sends raw ADC codes with
the range that was active, and the host computes
`I = (code − offset[r]) × gain[r]` with one gain and one offset per range.
Requirement R-12 asks for these values to be stored on the device.

- **Nominal values.** Before a board is calibrated the gain of a range is
  `VREF / (65536 × G × R[r])`, with the amplifier gain G = 19.93 and, for
  R[r], the shunt of the range in parallel with the shunt of range 0, which
  always stays in the path: 1 kΩ, 31.95 Ω, 0.999 Ω and 0.1 Ω for ranges 0
  to 3 (calculated, section 4.3). The set-point of the source is
  10 mV + code × 1.2817 mV before calibration (F-30). Nominal values are
  design targets, never a calibration.
- **The stored record.** Gain and offset of each range, the set-points of
  the DAC, the closed-switch zero, the length of the settling window, the
  calibration temperature, the offset of the VIN channel and the thermal
  limit, with a version field and a CRC. The record lives in the
  flash of the Pico 2, because the carrier board has no memory of its own
  (D-82).
- **What binds the record to a module and to a carrier.** The record
  carries the 64-bit chip identifier of the RP2350 and the serial number
  and revision of the carrier. The serial number is entered at calibration
  and written by hand into a frame on the silkscreen of the carrier
  (section 10.7). A record whose chip identifier does not match, which is a
  flash image copied to another module, is reported by GET_INFO as not
  calibrated, and so is a missing or corrupt record; nominal values are
  then used and flagged. A module moved to another carrier is not detected
  by firmware. For that case the host shows the stored serial number at
  every connection, to be compared with the label (section 9).
- **Open-switch zero.** With the output switch open the current is zero,
  so firmware measures the offset of every range, locked one by one. It
  runs at start-up and on the command CAL_ZERO, no earlier than 200 ms
  after the output switch opened or after a reset, because the gate of
  that switch needs this time to rest (F-36). The offsets are kept in RAM,
  not in flash. The zero of range 0 drifts by up to 0.7 nA/°C from the
  amplifier alone (datasheet maxima, calculated), so firmware takes the
  zero again whenever the output is off and the board temperature has
  moved by more than 2 °C.
- **Closed-switch zero (D-59).** The open-switch zero does not see what
  loads the node behind the shunts beyond the output switch. With the
  output on and the terminals open, at 5.0 V and at the working voltage,
  300 ms or more after the output was switched on (F-36), the reading of
  range 0 is stored and subtracted. The limit for a board is 50 nA at
  5.0 V and room temperature; the typical figures add up to 4 nA to 6 nA
  at 40 °C (estimate). The step needs the DUT disconnected once per
  calibration.
- **Gain calibration.** In source mode into precision resistors, with the
  output voltage read by a reference multimeter; two points per range,
  near 10 % and 90 % of full scale. It removes the tolerance of the
  shunts, 0.1 % to 0.5 % (D-68), the share of the range 0 shunt in the
  higher ranges and, at the calibration temperature, the leakage across
  the ladder. It does not remove the drift with temperature, 25 ppm/°C for
  ranges 0 to 2 and 50 ppm/°C for range 3 (datasheet values), which is why
  the calibration temperature is stored. After a reported sequencer fault
  (F-21) the gain of range 2 is calibrated again: its 1 Ω shunt has
  carried 6.3 W to 8.4 W for up to 2 ms (calculated).
- **DAC calibration.** Nine set-points or more, at room temperature,
  measured with the reference multimeter (D-58, F-30). Two points do not
  cover the integral non-linearity of the DAC, 15.4 mV at its limit and
  2.6 mV typical (datasheet values, calculated), against the criterion of
  10 mV in the test of R-08 (section 11).
- **Settling window.** After a range change the samples are flagged
  invalid for 7 samples by default (F-35; simulated: 45 µs to 0.1 % of
  range). The length is measured on the prototype with a fast load step
  and then stored as a constant.
- **Output settling.** For 250 ms after the output switch closes the
  samples read low by the charging current of its gate, 0.3 µA after 50 ms
  and 4 nA after 200 ms (simulated). The time is a constant of the host,
  which marks these samples and leaves them out of statistics below 1 µA
  (section 9).

Three more values are found on a board and stored, by the firmware rules of
section 6.6. Two of them have a field in the record since 2026-10-12. The
third has none, and section 16 lists it among the points to decide:

- the offset of the monitor channel that reads VIN, stored at calibration
  (F-26), a field of the record; before it a true 0.8 V can read 0.67 V
  (calculated);
- the thermal limit, a constant taken from the first board (F-15), a field
  of the record;
- the constants of the input model behind the power budget, calibrated on
  the first board (F-14): they are not in version 1 of the record and
  enter with a new version of it.

## The Record and How It Travels

Section 8 gives the record its layout since 2026-10-12, and section 7.4
the commands that carry it (D-103). Nothing of this is implemented beyond
the protocol itself; the last section of this page says what exists.

- The record is 78 to 142 bytes long, all fields little-endian: a mark,
  the version of the record, its length, the chip identifier of the
  controller, the serial number and the revision of the carrier, the
  settling window, the calibration temperature, the offset of the VIN
  channel (F-26), the thermal limit (F-15), the gain and the offset of
  each range as 4-byte floats, the closed-switch zero of range 0 (D-59),
  the points of the DAC as code and output voltage, none or 9 to 16 of
  them (F-30), and a CRC-16 over the bytes before it.
- The host builds the record and sends it with the command CAL_STORE.
  CAL_READ returns it, and returns no data when no record is stored. The
  protocol carries the record as one block of 250 bytes at the most and
  does not look into it, so a change of the fields is a new version of
  the record and not of the protocol.
- CAL_STORE is accepted in IDLE only: a record is written to flash with
  every path open (section 6.3). The device checks the mark, the version,
  the length and the CRC. The chip identifier in a record that arrives is
  zero or the one of the device, which writes its own and computes the
  CRC again before it stores the record. A record with another identifier
  is refused, so the record of one instrument cannot be stored into
  another.
- The device uses the settling window, the offset of the VIN channel, the
  thermal limit and the points of the DAC. The gains, the offsets and the
  closed-switch zero are for the host, which converts the codes; the
  device keeps them and returns them.
- GET_INFO reports the state of the calibration, which is one of three: a
  valid record of this chip, no record or a corrupt one, the record of
  another chip. With it come the settling window, the chip identifier, the
  serial number and the revision of the carrier, and the four zero offsets
  of the open-switch zero, in 1/256 of a code.
- CAL_ZERO returns the four zero offsets, and the notice "a new zero was
  taken" carries them as well.

## What the Circuit Simulations Add

On 2026-10-10 the circuits of draft A2 were simulated from the netlist of
the schematic ([simulation guide](../../simulation/README.md)). Nothing in
them is measured, and section 8 is unchanged.

- The nominal values come out of the drawn circuit: the amplifier sees
  shunts of 1 kΩ, 31.95 Ω, 999 mΩ and 99.99 mΩ, the gain is 19.93, and
  the code at zero current is 1313 (simulated). Over 120 sets of
  tolerances the gain of the chain lies between 19.87 and 19.97; with the
  tolerance of the shunts, that is what the gain calibration removes.
- One question for section 8 came up. With the amplifier at the limit of
  its common-mode rejection the zero of range 0 moves with the output
  voltage by 2.515 nA per volt, 12.58 nA at 5 V (simulated;
  [`signal_chain/common-mode`](../../simulation/results/signal_chain/README.md#signal_chaincommon-mode)).
  Section 8 does not say at which ladder voltage the open-switch zero is
  taken. The point stays for the bench and for the procedure that is not
  written; the closed-switch zero is taken at 5.0 V and at the working
  voltage.
- The settling after a range change comes out again: 45.37 µs to 0.1 %
  of the range, where the specification has about 45 µs. The charging
  current of the gates of the output switch is 293.1 nA after 50 ms,
  where the specification has 0.3 µA, and 2.748 nA after 200 ms, where it
  has 4 nA: 31 % less, inside the factor of two that the bench sets as
  its limit (simulated).

## Reference Equipment

Section 8 names three things, and no model or accuracy class beyond the
tolerance of the resistors:

| Equipment | Used for |
| --- | --- |
| Precision resistors, 0.01 % | The loads of the gain calibration, two points per range |
| Reference multimeter | The output voltage in the gain calibration and the nine or more set-points of the DAC calibration |
| A fast load step | The length of the settling window, measured once on the prototype |

The DAC calibration is done at room temperature, and the calibration
temperature is stored with the record. Which multimeter is
good enough for the target of R-05, ±1 % of reading ±0.1 % of range, is
part of the procedure that is not written.

## What Exists and What Is Not Written

- **Protocol support: exists.** Version 2 of the host protocol carries the
  calibration (D-103): CAL_READ and CAL_STORE move the record as one
  block, CAL_ZERO returns the zero offsets, and GET_INFO reports the state
  of the calibration with the identity. The definition, both codecs and
  their tests hold it, and the layouts stay provisional until the protocol
  is frozen in phase 6. See the [protocol page](../protocol/README.md).
- **The procedure: not written.** The steps, the resistor values of each
  range, the order, the pass limits and the form of the result. It belongs
  on this page.
- **The tool: not written.** The scripts under
  [`tools/calibration/`](../../tools/README.md) that drive the instrument
  and the reference equipment. The directory is empty.
- **Firmware support: not written.** The `cal` component of the firmware
  has no code: no record, no storage in flash, no check of the chip
  identifier, no zero calibration, and no handler for CAL_ZERO, CAL_READ
  or CAL_STORE. What the firmware core has is the codec of the four zero
  offsets and of the data of GET_INFO. The firmware builds for the
  Pico 2, and its image holds none of this (see the
  [firmware guide](../../firmware/README.md)).
- **Host support: in part.** The client of the host package runs CAL_ZERO
  and gets the four zero offsets back, and it reads and stores the record
  as a block of bytes with CAL_READ and CAL_STORE, tested against the
  simulated instrument. No code builds a record or reads its fields, so
  no table comes from an instrument, and the package converts codes with
  a nominal table. That table still uses the values of the first plan, a
  gain of 20 and shunts of 1 kΩ, 33 Ω, 1 Ω and 0.1 Ω, not the nominal
  values above. The host decodes the state of the calibration and the
  serial number of the carrier from GET_INFO and shows neither, does not
  mark the output settling, and has no guided calibration (see the
  [host guide](../../host/README.md)).

The order of that work follows the next steps in the
[README](../../README.md#project-status) of the repository. How the record
travels is decided, and the firmware builds for the Pico 2, both since
2026-10-12. What comes first is software, without a board: the nominal
values of draft A2 in the host package, the record in the `cal` component
of the firmware with its storage and its checks, the code of the host that
builds and parses a record, and the zero calibration with its waits
(F-36). The procedure and the tool come in phase 6, on a carrier board of
revision A.

A change to the calibration format is a breaking change (see the
[contributing guide](../../CONTRIBUTING.md)). The record has a version
field for it: version 1 is the layout of section 8, which no code
implements yet.
