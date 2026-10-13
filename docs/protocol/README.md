# Host Protocol

The wire protocol between the instrument and the PC: binary frames over the
USB serial port (CDC ACM), the same frame format in both directions.

## Where It Is Defined

The protocol has two sources that are kept in agreement:

- Section 7 of the [specification](../specification.md) describes the frame
  format, the sample word, the commands, the events and the layouts of
  their data.
- [`protocol/definition.toml`](../../protocol/definition.toml) holds every
  number. The constants used by the firmware and by the host are generated
  from it, together with the shared test vectors. See the
  [protocol directory](../../protocol/README.md).

Two neighbors of the protocol are described elsewhere. The calibration
record, which the protocol carries as one block, has its layout in section
8 of the specification (see [calibration](../calibration/README.md)). The
firmware side of the link, with the USB stack, the order of the frames and
the way firmware and host are tested against each other without hardware,
is in the [firmware design](../firmware/usb-integration.md).

## Status

As of 2026-10-12:

- The protocol is at version 2 and is not frozen. The project owner
  decided the step from version 1 as decision D-103 of the specification,
  while no instrument and no capture exist. The layouts of the response
  and event data are fixed and stay provisional until the protocol is
  frozen in phase 6 (section 7.4).
- Version 2 holds what draft A2 of the carrier board reports, which was an
  open check of section 16 until that day:
  - in the sample word, marks for over-range and under-range samples
    (F-22, F-35) and the request of the output switch, the instant from
    which the host counts the 250 ms of output settling; the invalid mark
    also covers the 5 ms after a change of supply (F-35);
  - fifteen fault flags, one for each cause of section 6.4, where
    version 1 had four;
  - a status record of 25 bytes with the mode, the locked range, the
    set-point, the readings of the monitor, the input in use with the CC
    class of the source and the budget as a current (F-14, D-49), the
    count of trips and the notices of the source;
  - an identity record of 48 bytes with the state of the calibration, the
    chip identifier, the serial number and the revision of the carrier,
    the settling window and the four zero offsets;
  - the events "supply changed" and "notice";
  - the calibration record as one block of 250 bytes at the most, read
    with CAL_READ and stored with CAL_STORE, and the four zero offsets as
    the answer to CAL_ZERO.
- Both codecs exist and reproduce the shared test vectors, the records
  included: the C one in the firmware core and the Python one in the host
  package, which also has a simulated instrument. That is the evidence
  there is. No frame has passed between a controller and a PC: the
  firmware builds for the Raspberry Pi Pico 2, but it has no USB link and
  no command handler, its image has not been loaded onto a board, and no
  hardware is built.
- What is open, by the item "Protocol" of section 16 and the list under
  it: no firmware fills these fields yet; the host uses the marks in its
  statistics only; the calibration record is a design that no code builds,
  parses or stores; the constants of the input model of F-14 have no field
  in version 1 of that record; and two more points of sections 6 to 8 are
  not decided, the step in which firmware evaluates F-21 and F-22 and how
  a fault is reported while firmware still waits in BOOT. The
  [protocol directory](../../protocol/README.md) gives every item with its
  place in the definition, and the way a later item enters: the definition
  first, then the generated files and the test vectors in the same change.

## What Comes Here

A reference written for users of the protocol, for someone who writes a
client of their own. It is added when the protocol is frozen in phase 6
(section 13 of the specification); until then section 7 is the description,
and it can still change. The reference will hold:

- the frame, byte by byte, with the CRC and the way a receiver finds the
  next frame after a damaged one;
- the stream payload and the sample word, with the meaning of every mark
  and how flagged samples are treated in statistics;
- every command with its arguments, its response data and the states that
  accept it, and every event with its data;
- the status record and the fault flags, with the cause behind each flag
  and what clears it;
- the calibration record, field by field, and what the instrument checks
  before it stores one;
- the conversion from ADC codes to current with the calibration that the
  instrument holds (see [calibration](../calibration/README.md));
- the version rule: what a client checks at connection, and which changes
  raise the protocol version;
- a worked example of a capture, with the bytes on the wire.
