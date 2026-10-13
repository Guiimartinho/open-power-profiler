# PIO Programs

The three programs that act without the processors: the capture of the
converter, the range sequencer and the over-current trip (D-40 of
[the specification](../specification.md)).

Status: sketches. On 2026-10-12 the four listings were assembled with
`pioasm` 2.3.1 of the Pico SDK, as programs of PIO version 1, with the
directives of the table below: the assembler accepts them, and the
instruction counts are the ones of the allocation table. That shows
their syntax and their size and nothing about their behavior: none has
run in an emulator or on a controller, and they are not in the firmware
tree yet. They enter it with the emulator tests. A clock is one cycle of
the 150 MHz system clock, 6.67 ns.

## Allocation

| Block | State machine | Program | Outputs | Inputs | Instructions |
| --- | --- | --- | --- | --- | --- |
| PIO 0 | 0 | Capture | GP19 ADC_SCK, GP20 ADC_CNV, GP21 SIDE_LOAD | GP16 ADC_DOUT, GP17 SIDE_DOUT | 10 of 32 |
| PIO 1 | 0 | Range sequencer | GP2 to GP6: three range gates, two address lines | GP8 CMP_UP, GP9 CMP_JUMP | 30 of 32 |
| PIO 2 | 0 | Output and trip | GP7 GATE_OUT | GP10 CMP_OC, GP4 GATE_R3 | 12 of 32 |
| PIO 2 | 1 | Supply guard | GP7 GATE_OUT | GP28 PWR_GOOD | 3 of 32 |

- One state machine writes the range lines and no other. Two writers of
  the same pin are resolved by priority in the same clock and by order
  otherwise, and a late write of a slower state machine could take range 3
  away again.
- GATE_OUT belongs to PIO 2. The trip program is the only code that
  raises it (F-20); the guard and firmware can only lower it.
- The project owner decided this split on 2026-10-12 (D-104), and the
  table of section 5 names PIO 2 for GATE_OUT and for CMP_OC. No pin
  moved.

The design relies on features that the RP2350 added to the PIO and the
DMA of the RP2040: the storage of the receive FIFO readable by index from
the program and writable by the processor, the count of input pins that
an input instruction sees, the state of the transmit FIFO as a readable
status, and a DMA channel whose count never ends, with an address that
wraps. All four are in the register headers of the Pico SDK for the
RP2350, read on 2026-10-12 (`FJOIN_RX_GET` with the `RXFx_PUTGETy`
registers, `IN_COUNT`, `STATUS_SEL`, the `ENDLESS` mode with
`RING_SIZE`). The assembler accepts the instruction that reads the
storage, `mov osr, rxfifo[y]`, and the directives that select these
features. Their behavior is confirmed in the emulator and on the
controller.

| Program | Directives |
| --- | --- |
| Capture | `.side_set 2`; `.in 2 left auto 32` |
| Range sequencer | `.fifo txget`; `.set 3`; `.out 5 right`; `.in 1 left`; `.mov_status txfifo < 1` |
| Output and trip | `.set 1`; `.in 1 left` |
| Supply guard | `.set 1` |

The set group of the sequencer is three pins wide, GP4 to GP6: the write
of the jump path reaches GATE_R3 and the address and leaves the two lower
gates as they are. Its out group is the five range lines.

## Capture (F-34)

One sample is 1500 clocks at 100 kSPS. The rising edge of convert-start
is the sampling instant, and every path of the program has a fixed
length, so the period does not depend on data.

| Phase | Clocks | Lines |
| --- | --- | --- |
| Load pulse | 3 | SIDE_LOAD low; it rises in the clock in which convert-start rises |
| Conversion | 114 | ADC_CNV high (108 or more; 710 ns of the converter are 107) |
| Data enable | 8 | ADC_CNV low, clock low |
| 16 bits | 16 × 16 | Clock high 8, low 8: 9.375 MHz |
| Idle | The rest | All low; 4 clocks or more before the next convert-start |

```text
.side_set 2                         ; bit 0 ADC_SCK, bit 1 ADC_CNV
    pull            side 0          ; once: idle count of the period
.wrap_target
    set pins, 0     side 0   [2]    ; SIDE_LOAD low for 3 clocks
    set pins, 1     side 2          ; load ends, conversion starts
    set x, 13       side 2
conv:
    jmp x-- conv    side 2   [7]    ; 14 x 8 = 112 clocks
    set x, 15       side 0   [7]    ; convert-start low, result enabled
bit:
    in pins, 2      side 1   [7]    ; read both data pins, clock high
    jmp x-- bit     side 0   [7]    ; clock low
    mov y, osr      side 0
idle:
    jmp y-- idle    side 0          ; fills the period
.wrap
```

- Each `in` shifts two pins, the converter and the registers, and 16 of
  them fill one word that is pushed by itself. The read happens in the
  clock that raises ADC_SCK; behind the two-stage synchronizer it shows
  the pins as they were two clocks earlier, 40 ns after the falling edge.
  The window of section 4.6 at 9.375 MHz is 36.9 ns wide and ends at the
  rising edge, so the reading lies 13 ns before its end (calculated from
  the figures of that section).
- The idle count is a word from firmware, checked by a pure function
  against F-34 before it is sent. The 500 kSPS option is another set of
  counts in the same program; its reading window is an open check of
  section 16.
- A receive FIFO that is full stops the program and stretches a period.
  The block sets a sticky flag then; the adapter reads it with every
  chunk and reports lost samples.

## Range Sequencer (F-16, F-17, F-20)

The five lines are one group, written with one instruction:

| Range | GATE_R1 | GATE_R2 | GATE_R3 | Address | Pattern |
| --- | --- | --- | --- | --- | --- |
| 0 | 0 | 0 | 0 | 00 | 0 |
| 1 | 1 | 0 | 0 | 01 | 9 |
| 2 | 0 | 1 | 0 | 10 | 18 |
| 3 | 0 | 0 | 1 | 11 | 28 |

A change writes two patterns: first the new gate with the new address
while the old gate stays on, then, one overlap later, the old gate off.
From range 1 to 2 that is 19, then 18.

The program holds the timing and nothing about ranges. What a step means
is in a table of four words that firmware writes, one word per range,
which the program reads by the range it is in:

| Bits | Field |
| --- | --- |
| 4:0, 9:5, 11:10 | Step up: overlap pattern, final pattern, range that follows |
| 16:12, 21:17, 23:22 | Step down: the same three |

```text
jump:                               ; from every state
    set pins, 7                     ; GATE_R3 high, address 11; lower gates as they are
    set y, 3
    set x, 24
jov:
    jmp x-- jov              [5]    ; overlap, 150 clocks
    set x, 28
    mov pins, x                     ; lower gates low
    jmp load
.wrap_target
load:
    pull noblock                    ; drop a request that waited through a change
idle:
    jmp pin jump                    ; CMP_JUMP, never blanked
    mov osr, rxfifo[y]              ; word of the present range
    mov x, pins                     ; CMP_UP
    jmp x-- step                    ; step up: its fields come first
    mov x, status                   ; all ones while no request waits
    jmp x-- idle
    pull                            ; one request: one step down
    jmp pin jump
    mov osr, rxfifo[y]
    out null, 12                    ; skip to the step-down fields
step:
    out pins, 5                     ; new gate and address
    set x, 24
ovl:
    jmp pin jump
    jmp x-- ovl              [4]    ; overlap, 150 clocks
    jmp pin jump
    out pins, 5                     ; old gate low
    out y, 2
    set x, 29
blk:
    jmp pin jump
    nop                      [3]
    jmp pin jump
    jmp x-- blk              [3]    ; blanking, 300 clocks
.wrap
```

How the rules are met, by count:

| Rule | In the program | Count |
| --- | --- | --- |
| F-16, 100 ns | CMP_JUMP is tested at the most 7 clocks apart in every loop and path | 3 clocks of synchronizer at the worst, 6 of waiting, 1 for the jump, 1 for the write: 11 clocks, 73 ns, plus the pads |
| F-16, never blanked, also when locked | The test is in the idle loop, in the overlap and in the blanking; the jump path reads no table | None |
| F-17, overlap | Second pattern 153 clocks after the first | 1.02 µs |
| F-17, blanking | CMP_UP is read again 305 clocks after the last write | 2.03 µs |
| Step down refused while a step up is due | A request is taken only in the pass that just read CMP_UP low; one that waited through a change is dropped | None |
| F-20 | Firmware starts the program with all lines low and then sends it to `jump` | None |

- Range 3 on request (F-20, F-22) is the jump path entered by firmware:
  it is valid from every state, which is why it reads no table.
- Automatic ranging and a locked range differ in the table only. Locked,
  the step-up fields of every range repeat its own pattern, so CMP_UP
  changes nothing; firmware walks to the range with step-down requests.
- A step up out of range 0 has no old gate. Its two patterns are equal,
  and its blanking is one overlap longer than 2 µs.
- The table is built by a pure function with a unit test: one-hot final
  patterns, both gates on in every overlap pattern, the address of the
  new range.

## Output and Trip (F-18, F-19, F-20)

```text
off:
    set pins, 0                     ; GATE_OUT low
    pull                            ; wait for the request; its word is the count
    set pins, 1                     ; GATE_OUT high: the only place
rearm:
    mov x, osr
watch:
    jmp pin over                    ; CMP_OC
    jmp rearm                       ; low: the time starts again
over:
    mov y, pins                     ; GATE_R3
    jmp !y rearm                    ; not range 3: the time does not count (D-99)
    jmp x-- watch                   ; 4 clocks a pass
    set pins, 0                     ; trip
    irq wait 0                      ; latch: stays here until firmware clears the flag
    jmp off
```

- The count is T_OC divided by 4: 450 passes for 12 µs. It is a build
  constant, checked at compile time against the 10 µs to 20 µs of F-18.
- The comparator is looked at every 4 clocks, so an interruption shorter
  than 27 ns can be missed.
- GATE_OUT can be high only inside the loop that watches the comparator:
  the trip is armed whenever the output is on, with no blanking.
- Firmware switches the output off by sending the program to `off`, which
  is valid from every state. It clears a trip by emptying the request FIFO
  and then clearing the flag, so that an old request cannot switch the
  output on.

The supply guard is three instructions: wait for PWR_GOOD low, write
GATE_OUT low, wait for PWR_GOOD high. It gives rule F-7 a reaction of a
few clocks for the output switch, ahead of the interrupt.

## Emulator and Tests

Section 18.2 asks that the programs run in an emulator on the PC first
and leaves its choice to phase 1. Recommended:

- The programs are `.pio` files, assembled by `pioasm` of the Pico SDK in
  both builds. The host build fetches the source of the assembler, pinned
  to a release, the way it fetches Unity.
- The emulator is written here, in C, in the test tree. The one found in
  the investigation of 2026-10-10 handles a single state machine and has
  no IRQ instruction, by its own project page; these programs need two
  blocks that see each other's pins, the two-stage synchronizer, forced
  instructions and the indexed FIFO storage. It models what the three
  programs use and nothing else, and its own behavior is checked against
  the controller in phase 1.

| Test | Shows |
| --- | --- |
| Capture: one period | Every figure of F-34 counted on the lines: 108 clocks, 16 pulses, 4 clocks of quiet, 16 clocks a bit, load low for 3, 1500 a period |
| Capture: data | A known pattern on both data pins arrives as the expected word |
| Sequencer: jump from every state | An edge of CMP_JUMP at every clock of a long scenario gives GATE_R3 high and address 11 within 15 clocks |
| Sequencer: steps | Every step up and down: both gates on for the overlap, never both off |
| Sequencer: blanking | CMP_UP held high climbs one range per window, never faster |
| Sequencer: requests | A step down is taken with CMP_UP low and dropped otherwise; one request is one step |
| Sequencer: locked | CMP_UP changes nothing; the jump still acts |
| Trip: qualification | 12 µs of CMP_OC in range 3 trips; 11.9 µs does not; an interruption restarts the time; time outside range 3 does not count |
| Trip: latch | GATE_OUT stays low until cleared; an old request does not switch it on |
| Guard | PWR_GOOD low takes GATE_OUT low within 6 clocks |

The emulator counts clocks. The delays of pads, comparators and gate
drivers are not in it: phase 1 measures the reaction from pin to pin
(section 13).
