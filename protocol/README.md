# Protocol

The single definition of the wire protocol between the instrument and the
host. Section 7 of the [specification](../docs/specification.md) describes
the protocol; the files in this directory decide it.

| File | Content |
| --- | --- |
| `definition.toml` | Every constant: protocol version, frame layout and size limits, CRC parameters, sample word fields, the type, command, status, event, state and mode codes, the codes of the supply in use, of the CC class, of the notices and of the state of the calibration, the bits of the fault flags and of the notices in the status record, the layouts of the four records with a fixed size, and the constants of the command arguments |
| `vectors.toml` | Inputs of the shared test vectors: CRC inputs, frames, sample words and records |
| `generate.py` | Generator, with its own reference encoder |
| `vectors.json` | Generated: the encoded vectors, read by the host tests |

The generator also writes these files in other directories. They are
committed, and they are never edited by hand:

| Generated file | Used by |
| --- | --- |
| `firmware/components/proto/include/proto/pp_proto_defs.h` | Firmware: constants, sizes of the records and offsets of their fields |
| `firmware/test/host/generated/pp_proto_vectors.h` | Firmware tests: vectors |
| `host/src/s3_power_profiler/protocol/_defs.py` | Host package: constants, struct formats and field names of the records |

## Status

As of 2026-10-12 the definition is at protocol version 2, and the generated
files are up to date with it. Version 2 holds what draft A2 of the carrier
board reports to the host; the project owner decided it in one step, as
decision D-103 of the specification, and
[State Against Draft A2](#state-against-draft-a2) says what it added.

Both codecs, the C one of the firmware core and the Python one of the host
package, reproduce the shared vectors in their unit tests: 4 CRC inputs,
8 frames, 8 sample words and 10 records. That is all the evidence there
is. No frame has passed between a controller and a PC: the firmware builds
for the Raspberry Pi Pico 2, but it has no USB link and no command
handler, its image has not been loaded onto a board, and nothing of the
instrument is built.

What the definition holds and what it does not:

- It holds the numbers: codes, bit positions, sizes and limits.
- Since version 2 it holds four layouts, each a list of fields with their
  formats: the data of GET_INFO (`info`, 48 bytes), the data of GET_STATUS
  (`status`, 25 bytes), the data of the event "supply changed" (`supply`,
  4 bytes) and the four zero offsets (`zero`, 16 bytes), which CAL_ZERO
  returns and the notice of a new zero carries. The generator derives the
  sizes, the offsets and the record vectors from these lists. The layouts
  are fixed, and they stay provisional until the protocol is frozen in
  phase 6: a receiver ignores the bytes after the fields it knows, so
  fields can be appended.
- It does not hold the layout of the command arguments or of the data of
  the other events, which are one or two fields each. Section 7.4 of the
  specification describes them. The host keeps them in one module,
  `host/src/s3_power_profiler/protocol/commands.py`, which its client and
  its simulator share. The firmware core has the envelopes and the codec
  of the four records; its command handlers are not written.
- It does not hold the calibration record. The protocol carries the record
  as one block of 250 bytes at the most (`CAL_RECORD_MAX`), with the
  commands CAL_READ and CAL_STORE. Section 8 of the specification gives
  its layout, which is a design: no code on either side builds or parses
  a record, and no firmware stores one.

## State Against Draft A2

Draft A2 gave the firmware 36 rules that guard hardware (F-1 to F-36,
section 6.6 of the specification). Several of them end in something the
instrument has to tell the host. Version 1 of the protocol had no place
for it, and section 16 of the specification listed that as an open check.
On 2026-10-12 the project owner decided to add all of it in one step,
while no instrument and no capture exist (D-103). Every item below is in
`definition.toml`, in the generated files, in both codecs and in their
tests:

| What the instrument reports | Behind it | In the definition since version 2 |
| --- | --- | --- |
| A mark for an over-range sample: the code is 65535, or the jump comparator is high | F-35 | `over_range`, bit 20 of `[sample_word]` |
| A mark for an under-range sample: the code stays below 650, which means a current that flows backward through the ladder | F-22, D-69 | `under_range`, bit 21 of `[sample_word]` |
| The invalid mark for the 5 ms after the supplying input changes or the 5 V rail steps by more than 0.3 V | F-35 | No new bit: `invalid`, bit 18, has this second meaning in section 7.3 |
| The instant from which the host counts the 250 ms of output settling after the output is switched on | Sections 8 and 9, F-24 | `output`, bit 22 of `[sample_word]`: the request line of the output switch, which the carrier puts among the 16 side bits of every sample (section 4.7) |
| A fault flag for each cause of section 6.4 | Section 6.4 | `[faults]` has fifteen bits of a 16-bit field. To `OVERCURRENT`, `POWER_LIMIT`, `THERMAL` and `SELFTEST` came `SEQUENCER` (F-21), `REVERSE_CURRENT` (F-22), `CARRIER_SUPPLY` (F-2, F-7), `ANALOG_RAILS` (F-3, F-12), `VIN_OVERVOLTAGE` (F-27), `VIN_RANGE` (F-26), `SOURCE_OVERLOAD` (F-32), `EXTERNAL_VOLTAGE` (F-33), `START_CHECK` (F-25, F-28), `START_OVERCURRENT` (F-19) and `WATCHDOG` (F-8) |
| The input in use (USB-C connector or the USB connector of the Pico 2) and the CC class of the source | F-14, with F-11 and F-13 | The tables `[supplies]` and `[cc_classes]`, with a field each in the layouts `status` and `supply` |
| The power budget as a current: 0.45 A, 1.4 A or 1.7 A at the input | D-49, F-14 | `budget_ma` in the layouts `status` and `supply`; the event `SUPPLY_CHANGED`, code `0x04` |
| The mode, the locked range, the set-point and the readings of the monitor: VOUT, VIN, the 5 V rail, the board temperature | Section 7.4 | Fields of the layout `status` |
| The count of consecutive trips; "set-point settled" and "DUT above the set-point" | F-19, F-30, F-31 | `trip_count` and the notice byte of the layout `status`, with the bits of `[status_notices]`; the event `NOTICE`, code `0x05`, with the codes of `[notices]` |
| Whether the instrument is calibrated; the chip identifier; the serial number and the revision of the carrier; the settling window | Section 8, D-82, F-35 | The table `[cal_states]` and fields of the layout `info` |
| The zero offsets of the four ranges | Section 8, F-36 | The layout `zero`, in 1/256 of a code (`ZERO_SCALE`), and the last four fields of the layout `info` |
| The calibration record | Section 8, D-58, D-59, D-82 | The commands `CAL_READ`, code `0x32`, and `CAL_STORE`, code `0x33`, and the limit `CAL_RECORD_MAX`. The record itself is not in the definition |

Two codes of version 1 are retired and not used again: the command
`CAL_WRITE` (`0x31`), which carried one gain and one offset, and the event
`POWER_BUDGET_CHANGED` (`0x02`), which carried milliwatts. A comment in
`definition.toml` marks each of the two codes as not to be used again.

What is still open. The first two points are the item "Protocol" of
section 16 of the specification:

- No firmware fills these fields. The firmware core has the codec; the
  handlers, the supervision that would raise an event and the storage of
  the calibration record are not written.
- The host decodes all of it and uses the marks in its statistics only.
  Those leave flagged samples out, where section 9 asks to fill them with
  the first valid sample after them.
- The calibration record is a design: no code builds or parses one.
- The constants of the input model of rule F-14 have no field in version 1
  of the calibration record. They are defined on the first board and enter
  with a new version of the record, not of the protocol.
- Section 16 lists two more points of sections 6 to 8 that are not
  decided: the step in which firmware evaluates F-21 and F-22, and how a
  fault is reported while firmware still waits in BOOT.
- Every layout is provisional until the protocol is frozen in phase 6.

### How an Item Enters

The definition comes first, and the rest follows from it in one change:

1. `definition.toml`: the new bit, code, field or constant. A sample mark
   takes the reserved bit of `[sample_word]`, bit 23, the last one free.
   A fault cause takes a bit of `[faults]`, where bit 15 is the last one
   free. A new set of codes is a new table. A new field of a record is
   appended to its list in `[layouts]`, so that an older receiver still
   reads the fields it knows.
2. `generate.py`, where the definition gains something the generator does
   not render or check yet. A new table of codes needs an entry in its
   list of enumerations, and a new table of flag bits an entry in its list
   of flag tables. Everything else is rendered without a change, because
   the generator walks the tables: a new bit of `[faults]` or of
   `[status_notices]`, a new field of the sample word, a new field of a
   layout and a whole new layout.
3. `vectors.toml`: a vector that exercises the new item, so that both
   codecs are held to it.
4. The generated files of the firmware and of the host, written by the
   generator, and the code and the tests on both sides that use the item.
   A new field of a layout does not reach the two codecs of the records by
   itself: `pp_report.c` and `commands.py` name the fields one by one and
   have to gain it. A record vector with the field set is what holds them
   to it.
5. Section 7 of the specification, with the entry of section 16 updated
   and the decision recorded.

`version` in `[protocol]` is raised where the frame, the sample word or a
command changes, because an older firmware or host would read the new data
wrongly. Data appended to a response or to an event does not need it by
itself: a receiver ignores the bytes after the fields it knows (section
7.4). The steps are spelled out in the next section.

## Changing the Protocol

1. Edit `definition.toml`, and `vectors.toml` when a vector should cover the
   change.
2. Run the generator from the repository root. It needs Python 3.11 or later
   and nothing outside the standard library:

   ```sh
   python protocol/generate.py
   ```

3. Update section 7 of the specification and record the decision in its
   decision log. A change that an older firmware or host cannot understand
   raises `version` and is a breaking change: its commit carries `!` and a
   `BREAKING CHANGE:` footer, as the
   [contributing guide](../CONTRIBUTING.md) describes.
4. Run the firmware host tests and the host tests. Both codecs must reproduce
   the new vectors. The commands are in the
   [firmware guide](../firmware/README.md) and the
   [host guide](../host/README.md).
5. Commit the edited files and the generated files together.

The generator refuses a definition that contradicts itself:

- a header size that does not match the frame layout;
- CRC parameters that do not give the check value;
- a code used twice in one of the ten tables of codes: frame types,
  commands, status, events, states, modes, supplies, CC classes, notices
  and states of the calibration;
- sample word fields that overlap or do not add up to 32 bits;
- a bit used twice in `[faults]` or in `[status_notices]`, or a bit outside
  the 16 or 8 bits of its field;
- a field name used twice in a layout, or a layout that does not fit in a
  response;
- a limit of the calibration record that does not fit in a command;
- a full stream frame that does not fit in the largest payload;
- two vectors with one name, or a vector with a value that does not fit
  its field.

Of the values of `[constants]` it checks `CAL_RECORD_MAX` only.

## What the Generator Writes

From `definition.toml`:

| Part of the definition | In the C header `pp_proto_defs.h` | In the Python module `_defs.py` |
| --- | --- | --- |
| `[protocol]` and `[crc]` | `#define` constants | Module constants |
| The ten tables of codes | One `enum` type each | One `IntEnum` class each |
| The two tables of flag bits, `[faults]` and `[status_notices]` | `#define` masks: `PP_FAULT_*` and `PP_STATUS_NOTICE_*` | The `IntFlag` classes `Fault` and `StatusNotice` |
| `[sample_word]` | The shift and the mask of every field | The same |
| `[layouts]` | The size of every record and the offset of every field: `PP_INFO_SIZE`, `PP_INFO_OFFSET_CHIP_ID` and the like | For every record a format of the `struct` module, the names of its fields and its size: `INFO_FORMAT`, `INFO_FIELDS`, `INFO_SIZE` and the like |
| `[constants]` | `#define` constants | Module constants |

From `vectors.toml`, each entry encoded with the reference encoder:

- `vectors.json`, with the groups `crc`, `frames`, `samples` and `records`;
  the last holds one list for each layout.
- `pp_proto_vectors.h`, the same vectors as C arrays. A record vector
  holds the bytes of the record, its numeric fields in the order of the
  layout, and its text.

The rendering of the flag tables, of the layouts and of the record vectors
came with version 2.

## Checks

Run this from the repository root before you commit. It writes nothing and
fails when a generated file is stale:

```sh
python protocol/generate.py --check
```

The Protocol workflow runs the same command, and it lints the generator
with ruff and with mypy in strict mode. Like every workflow of the
repository it starts by hand only (decision D-22 of the specification), so
the local run is the one that counts day to day. The lint commands of the
workflow, also from the repository root:

```sh
ruff check protocol
ruff format --check protocol
mypy --strict protocol/generate.py
```

ruff and mypy are installed with the development tools of the host package
(see the [host guide](../host/README.md)).

On 2026-10-12 the four commands ran locally on Windows: the generated
files are up to date, and ruff and mypy find nothing. The workflow has not
been started since.

## Reference Encoder

The vectors are not produced by the code they test. The generator has a small
bit-by-bit CRC and frame encoder of its own, and it checks that CRC against
`binascii.crc_hqx` from the Python standard library and against the check
value in `definition.toml` before writing anything. Sample words and
records are packed by the generator as well, from the fields and formats of
the definition, so a record vector is the layout of the definition and not
the output of either codec.
