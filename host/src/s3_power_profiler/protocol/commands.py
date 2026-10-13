"""Command, response and event payloads.

The envelopes are part of the protocol::

    command payload  = command id u8 | arguments
    response payload = command id u8 | status u8 | data
    event payload    = event id u8 | data

The layouts of the arguments, of the response data and of the event data are
**provisional** until the protocol is frozen. Every one of them is defined in
this module, so the client and the simulator cannot disagree, and a change has
a single place to happen.

Functions named ``build_*`` create a message and functions named ``parse_*``
read one. Arguments are parsed strictly: the length has to match exactly.
Response and event data are read tolerantly: extra trailing bytes are ignored,
so that a newer instrument can append fields.
"""

from __future__ import annotations

import enum
import struct
from collections.abc import Sequence
from dataclasses import dataclass
from typing import Any, TypeVar

from s3_power_profiler.errors import PayloadError
from s3_power_profiler.protocol._defs import (
    CAL_RECORD_MAX,
    COMMAND_MAX_PAYLOAD,
    FRAME_MAX_PAYLOAD,
    INFO_FORMAT,
    PROTOCOL_VERSION,
    RANGE_AUTO,
    RANGE_COUNT,
    SERIAL_SIZE,
    STATUS_FORMAT,
    SUPPLY_FORMAT,
    ZERO_FORMAT,
    ZERO_SCALE,
    CalState,
    CcClass,
    Command,
    DeviceState,
    Event,
    Fault,
    Mode,
    Notice,
    Status,
    StatusNotice,
    Supply,
)

_ID = struct.Struct("<B")
_RESPONSE_HEADER = struct.Struct("<BB")
_ID_LIMIT = 1 << 8

MAX_COMMAND_ARGUMENTS = COMMAND_MAX_PAYLOAD - _ID.size
"""Largest number of argument bytes a command may carry."""

MAX_RESPONSE_DATA = FRAME_MAX_PAYLOAD - _RESPONSE_HEADER.size
"""Largest number of data bytes a response may carry."""

MAX_EVENT_DATA = FRAME_MAX_PAYLOAD - _ID.size
"""Largest number of data bytes an event may carry."""


# --- Envelopes -------------------------------------------------------------


def _check_id(value: int, what: str) -> None:
    if not 0 <= value < _ID_LIMIT:
        raise ValueError(f"{what} {value} does not fit in 8 bits")


@dataclass(frozen=True, slots=True)
class CommandMessage:
    """A command sent by the host.

    The identifier is kept as a plain number, so that a receiver can answer an
    identifier it does not know.

    Attributes:
        command_id: Identifier of the command.
        arguments: Argument bytes, empty for commands without arguments.

    Raises:
        ValueError: If the identifier or the arguments are outside the limits.
    """

    command_id: int
    arguments: bytes = b""

    def __post_init__(self) -> None:
        _check_id(self.command_id, "command id")
        if len(self.arguments) > MAX_COMMAND_ARGUMENTS:
            raise ValueError(
                f"{len(self.arguments)} argument bytes exceed the limit of {MAX_COMMAND_ARGUMENTS}"
            )
        object.__setattr__(self, "arguments", bytes(self.arguments))


@dataclass(frozen=True, slots=True)
class ResponseMessage:
    """The answer of the instrument to one command.

    Attributes:
        command_id: Identifier of the command being answered.
        status_code: Outcome of the command, a ``Status`` value when known.
        data: Result bytes, empty when the command returns nothing.

    Raises:
        ValueError: If a field is outside the limits.
    """

    command_id: int
    status_code: int
    data: bytes = b""

    def __post_init__(self) -> None:
        _check_id(self.command_id, "command id")
        _check_id(self.status_code, "status code")
        if len(self.data) > MAX_RESPONSE_DATA:
            raise ValueError(f"{len(self.data)} data bytes exceed the limit of {MAX_RESPONSE_DATA}")
        object.__setattr__(self, "data", bytes(self.data))

    @property
    def ok(self) -> bool:
        """True when the command succeeded."""
        return self.status_code == Status.OK


@dataclass(frozen=True, slots=True)
class EventMessage:
    """An unsolicited notification from the instrument.

    Attributes:
        event_id: Identifier of the event, an ``Event`` value when known.
        data: Event data, empty when the event carries none.

    Raises:
        ValueError: If a field is outside the limits.
    """

    event_id: int
    data: bytes = b""

    def __post_init__(self) -> None:
        _check_id(self.event_id, "event id")
        if len(self.data) > MAX_EVENT_DATA:
            raise ValueError(f"{len(self.data)} data bytes exceed the limit of {MAX_EVENT_DATA}")
        object.__setattr__(self, "data", bytes(self.data))


def encode_command(message: CommandMessage) -> bytes:
    """Serialize a command into a frame payload."""
    return _ID.pack(message.command_id) + message.arguments


def decode_command(payload: bytes | bytearray | memoryview) -> CommandMessage:
    """Parse the payload of a command frame.

    Raises:
        PayloadError: If the payload is empty or longer than a command may be.
    """
    if not _ID.size <= len(payload) <= COMMAND_MAX_PAYLOAD:
        raise PayloadError(
            f"command payload of {len(payload)} bytes is outside "
            f"{_ID.size} to {COMMAND_MAX_PAYLOAD}"
        )
    return CommandMessage(command_id=payload[0], arguments=bytes(payload[_ID.size :]))


def encode_response(message: ResponseMessage) -> bytes:
    """Serialize a response into a frame payload."""
    return _RESPONSE_HEADER.pack(message.command_id, message.status_code) + message.data


def decode_response(payload: bytes | bytearray | memoryview) -> ResponseMessage:
    """Parse the payload of a response frame.

    Raises:
        PayloadError: If the payload is shorter than the response header.
    """
    if len(payload) < _RESPONSE_HEADER.size:
        raise PayloadError(
            f"response payload of {len(payload)} bytes is shorter than its "
            f"{_RESPONSE_HEADER.size}-byte header"
        )
    command_id, status_code = _RESPONSE_HEADER.unpack_from(payload)
    return ResponseMessage(
        command_id=command_id,
        status_code=status_code,
        data=bytes(payload[_RESPONSE_HEADER.size :]),
    )


def encode_event(message: EventMessage) -> bytes:
    """Serialize an event into a frame payload."""
    return _ID.pack(message.event_id) + message.data


def decode_event(payload: bytes | bytearray | memoryview) -> EventMessage:
    """Parse the payload of an event frame.

    Raises:
        PayloadError: If the payload is empty.
    """
    if len(payload) < _ID.size:
        raise PayloadError("event payload is empty")
    return EventMessage(event_id=payload[0], data=bytes(payload[_ID.size :]))


def describe_command(command_id: int) -> str:
    """Return the name of a command, or its number when it is not known."""
    try:
        return Command(command_id).name
    except ValueError:
        return f"0x{command_id:02X}"


def describe_status(status_code: int) -> str:
    """Return the name of a status code, or its number when it is not known."""
    try:
        return Status(status_code).name
    except ValueError:
        return f"status {status_code}"


# --- Provisional layouts -----------------------------------------------------
#
# Everything below may change before the protocol is frozen.

_U8 = struct.Struct("<B")
_U16 = struct.Struct("<H")
_INFO = struct.Struct(INFO_FORMAT)
_STATUS = struct.Struct(STATUS_FORMAT)
_SUPPLY = struct.Struct(SUPPLY_FORMAT)
_ZERO = struct.Struct(ZERO_FORMAT)

ZeroOffsets = tuple[float, float, float, float]
"""Zero offset of each range, in ADC codes."""

NO_ZERO: ZeroOffsets = (0.0, 0.0, 0.0, 0.0)

NO_NOTICES = StatusNotice(0)

_E = TypeVar("_E", bound=enum.Enum)


def _pack(layout: struct.Struct, what: str, *values: object) -> bytes:
    """Pack values chosen by the caller, reporting a bad one as ``ValueError``."""
    try:
        return layout.pack(*values)
    except (struct.error, OverflowError) as error:
        # A float too large for 32 bits raises OverflowError, not struct.error.
        raise ValueError(f"{what}: {error}") from error


def _unpack_exact(layout: struct.Struct, data: bytes, what: str) -> tuple[Any, ...]:
    """Unpack arguments, which must have exactly the size of the layout."""
    if len(data) != layout.size:
        raise PayloadError(f"{what} takes {layout.size} argument bytes, got {len(data)}")
    return layout.unpack(data)


def _unpack_prefix(layout: struct.Struct, data: bytes, what: str) -> tuple[Any, ...]:
    """Unpack the start of response or event data, ignoring any extra bytes."""
    if len(data) < layout.size:
        raise PayloadError(f"{what} needs {layout.size} data bytes, got {len(data)}")
    return layout.unpack_from(data)


def _known(enum_type: type[_E], value: int, source: str, what: str) -> _E:
    """Return the member of an enumeration, or report an unknown value."""
    try:
        return enum_type(value)
    except ValueError as error:
        raise PayloadError(f"{source} reports unknown {what} {value}") from error


def _zero_to_wire(offsets: Sequence[float]) -> tuple[int, ...]:
    """Convert zero offsets in codes to the integers of the wire."""
    if len(offsets) != RANGE_COUNT:
        raise ValueError(f"expected {RANGE_COUNT} zero offsets, got {len(offsets)}")
    return tuple(round(value * ZERO_SCALE) for value in offsets)


def _zero_from_wire(values: Sequence[int]) -> ZeroOffsets:
    """Convert the integers of the wire to zero offsets in codes."""
    first, second, third, fourth = (int(value) / ZERO_SCALE for value in values)
    return (first, second, third, fourth)


@dataclass(frozen=True, slots=True)
class DeviceInfo:
    """Identity of an instrument, returned by GET_INFO.

    Attributes:
        protocol_version: Version of the wire protocol the instrument speaks.
        hardware_revision: Revision of the board, 0 for the first one.
        firmware_version: Firmware version as major, minor and patch numbers.
        cal_state: Whether a calibration record of this instrument is in use.
        settle_samples: Samples flagged after a range change.
        chip_id: The 64-bit identifier of the controller.
        carrier_serial: Serial number of the carrier that the record holds.
        carrier_revision: Revision of the carrier that the record holds.
        zero_offsets: Zero offset of each range from the last zero
            calibration, in codes.
    """

    protocol_version: int
    hardware_revision: int
    firmware_version: tuple[int, int, int]
    cal_state: CalState = CalState.NOMINAL
    settle_samples: int = 0
    chip_id: int = 0
    carrier_serial: str = ""
    carrier_revision: int = 0
    zero_offsets: ZeroOffsets = NO_ZERO


@dataclass(frozen=True, slots=True)
class DeviceStatus:
    """State of an instrument, returned by GET_STATUS.

    Attributes:
        state: State of the device state machine.
        faults: Faults that are latched.
        dropped_blocks: Blocks dropped by the instrument since it started.
        mode: Operating mode.
        range_lock: Locked range, or None while ranging is automatic.
        setpoint_mv: Set-point of the source, in millivolts.
        vout_mv: Voltage at the ladder output, in millivolts.
        vin_mv: Voltage at the VIN terminal, in millivolts.
        rail5_mv: The 5 V rail of the carrier, in millivolts.
        temperature: Board temperature, in degrees Celsius.
        supply: Which input supplies the carrier.
        cc_class: What the USB-C source advertises.
        budget_ma: Input current the instrument allows itself, in milliamperes.
        trip_count: Consecutive over-current trips.
        notices: Conditions of the source that are no faults.
    """

    state: DeviceState
    faults: Fault
    dropped_blocks: int
    mode: Mode = Mode.AMPERE_METER
    range_lock: int | None = None
    setpoint_mv: int = 0
    vout_mv: int = 0
    vin_mv: int = 0
    rail5_mv: int = 0
    temperature: float = 0.0
    supply: Supply = Supply.MODULE
    cc_class: CcClass = CcClass.DEFAULT
    budget_ma: int = 0
    trip_count: int = 0
    notices: StatusNotice = NO_NOTICES


@dataclass(frozen=True, slots=True)
class SupplyReport:
    """Data of a SUPPLY_CHANGED event.

    Attributes:
        supply: Which input supplies the carrier.
        cc_class: What the USB-C source advertises.
        budget_ma: Input current the instrument allows itself, in milliamperes.
    """

    supply: Supply
    cc_class: CcClass
    budget_ma: int


@dataclass(frozen=True, slots=True)
class NoticeMessage:
    """Data of a NOTICE event.

    Attributes:
        notice: What happened.
        zero_offsets: The new zero offsets in codes, for ``Notice.ZERO_TAKEN``
            only.
    """

    notice: Notice
    zero_offsets: ZeroOffsets | None = None


def parse_no_arguments(arguments: bytes, what: str) -> None:
    """Check that a command without arguments received none.

    Raises:
        PayloadError: If there are argument bytes.
    """
    if arguments:
        raise PayloadError(f"{what} takes no arguments, got {len(arguments)} bytes")


def build_get_info() -> CommandMessage:
    """Create a GET_INFO command."""
    return CommandMessage(Command.GET_INFO)


def encode_device_info(info: DeviceInfo) -> bytes:
    """Serialize the data of a GET_INFO response.

    Raises:
        ValueError: If a field does not fit in its bytes.
    """
    try:
        serial = info.carrier_serial.encode("ascii")
    except UnicodeEncodeError as error:
        raise ValueError(f"carrier serial is not ASCII: {error}") from error
    if len(serial) > SERIAL_SIZE:
        raise ValueError(f"carrier serial is longer than {SERIAL_SIZE} characters")
    return _pack(
        _INFO,
        "device info",
        info.protocol_version,
        info.hardware_revision,
        *info.firmware_version,
        int(info.cal_state),
        info.settle_samples,
        info.chip_id,
        serial,
        info.carrier_revision,
        *_zero_to_wire(info.zero_offsets),
    )


def decode_device_info(data: bytes) -> DeviceInfo:
    """Parse the data of a GET_INFO response.

    Raises:
        PayloadError: If the data is too short, which is also how an
            instrument with an older protocol shows, or holds an unknown
            value.
    """
    if not data:
        raise PayloadError("GET_INFO returned no data")
    if len(data) < _INFO.size:
        raise PayloadError(
            f"GET_INFO returned {len(data)} data bytes and names protocol version {data[0]}; "
            f"version {PROTOCOL_VERSION} has {_INFO.size}"
        )
    (
        protocol_version,
        hardware_revision,
        major,
        minor,
        patch,
        cal_state,
        settle_samples,
        chip_id,
        serial,
        carrier_revision,
        *zero,
    ) = _INFO.unpack_from(data)
    try:
        carrier_serial = bytes(serial).rstrip(b"\x00").decode("ascii")
    except UnicodeDecodeError as error:
        raise PayloadError(
            f"GET_INFO reports a carrier serial that is not ASCII: {error}"
        ) from error
    return DeviceInfo(
        protocol_version=int(protocol_version),
        hardware_revision=int(hardware_revision),
        firmware_version=(int(major), int(minor), int(patch)),
        cal_state=_known(CalState, int(cal_state), "GET_INFO", "calibration state"),
        settle_samples=int(settle_samples),
        chip_id=int(chip_id),
        carrier_serial=carrier_serial,
        carrier_revision=int(carrier_revision),
        zero_offsets=_zero_from_wire(zero),
    )


def build_get_status() -> CommandMessage:
    """Create a GET_STATUS command."""
    return CommandMessage(Command.GET_STATUS)


def encode_device_status(status: DeviceStatus) -> bytes:
    """Serialize the data of a GET_STATUS response.

    Raises:
        ValueError: If a field does not fit in its bytes.
    """
    return _pack(
        _STATUS,
        "device status",
        int(status.state),
        int(status.faults),
        status.dropped_blocks,
        int(status.mode),
        RANGE_AUTO if status.range_lock is None else status.range_lock,
        status.setpoint_mv,
        status.vout_mv,
        status.vin_mv,
        status.rail5_mv,
        round(status.temperature * 10),
        int(status.supply),
        int(status.cc_class),
        status.budget_ma,
        status.trip_count,
        int(status.notices),
    )


def decode_device_status(data: bytes) -> DeviceStatus:
    """Parse the data of a GET_STATUS response.

    Raises:
        PayloadError: If the data is too short or holds an unknown value.
    """
    (
        state,
        faults,
        dropped_blocks,
        mode,
        range_lock,
        setpoint_mv,
        vout_mv,
        vin_mv,
        rail5_mv,
        temperature,
        supply,
        cc_class,
        budget_ma,
        trip_count,
        notices,
    ) = _unpack_prefix(_STATUS, data, "GET_STATUS")
    if range_lock != RANGE_AUTO and not 0 <= range_lock < RANGE_COUNT:
        raise PayloadError(f"GET_STATUS reports unknown range {range_lock}")
    return DeviceStatus(
        state=_known(DeviceState, int(state), "GET_STATUS", "state"),
        faults=Fault(int(faults)),
        dropped_blocks=int(dropped_blocks),
        mode=_known(Mode, int(mode), "GET_STATUS", "mode"),
        range_lock=None if range_lock == RANGE_AUTO else int(range_lock),
        setpoint_mv=int(setpoint_mv),
        vout_mv=int(vout_mv),
        vin_mv=int(vin_mv),
        rail5_mv=int(rail5_mv),
        temperature=int(temperature) / 10,
        supply=_known(Supply, int(supply), "GET_STATUS", "supply"),
        cc_class=_known(CcClass, int(cc_class), "GET_STATUS", "CC class"),
        budget_ma=int(budget_ma),
        trip_count=int(trip_count),
        notices=StatusNotice(int(notices)),
    )


def build_set_mode(mode: Mode) -> CommandMessage:
    """Create a SET_MODE command.

    Raises:
        ValueError: If ``mode`` is not a known mode.
    """
    return CommandMessage(Command.SET_MODE, _U8.pack(Mode(mode)))


def parse_set_mode(arguments: bytes) -> Mode:
    """Read the arguments of SET_MODE.

    Raises:
        PayloadError: If the length is wrong or the mode is unknown.
    """
    (value,) = _unpack_exact(_U8, arguments, "SET_MODE")
    try:
        return Mode(value)
    except ValueError as error:
        raise PayloadError(f"SET_MODE got unknown mode {value}") from error


def build_set_voltage(millivolts: int) -> CommandMessage:
    """Create a SET_VOLTAGE command for an output voltage in millivolts.

    Raises:
        ValueError: If the voltage does not fit in 16 bits.
    """
    return CommandMessage(Command.SET_VOLTAGE, _pack(_U16, "voltage in mV", millivolts))


def parse_set_voltage(arguments: bytes) -> int:
    """Read the arguments of SET_VOLTAGE and return the voltage in millivolts.

    Raises:
        PayloadError: If the length is wrong.
    """
    (millivolts,) = _unpack_exact(_U16, arguments, "SET_VOLTAGE")
    return int(millivolts)


def build_dut_power(on: bool) -> CommandMessage:
    """Create a DUT_POWER command that closes or opens the output switch."""
    return CommandMessage(Command.DUT_POWER, _U8.pack(1 if on else 0))


def parse_dut_power(arguments: bytes) -> bool:
    """Read the arguments of DUT_POWER and return True for "on".

    Raises:
        PayloadError: If the length is wrong or the value is neither 0 nor 1.
    """
    (value,) = _unpack_exact(_U8, arguments, "DUT_POWER")
    if value not in (0, 1):
        raise PayloadError(f"DUT_POWER takes 0 or 1, got {value}")
    return bool(value)


def build_set_range(range_index: int | None) -> CommandMessage:
    """Create a SET_RANGE command.

    Args:
        range_index: Range to lock, or None for automatic ranging.

    Raises:
        ValueError: If the range does not exist.
    """
    if range_index is None:
        return CommandMessage(Command.SET_RANGE, _U8.pack(RANGE_AUTO))
    if not 0 <= range_index < RANGE_COUNT:
        raise ValueError(f"range {range_index} is outside 0 to {RANGE_COUNT - 1}")
    return CommandMessage(Command.SET_RANGE, _U8.pack(range_index))


def parse_set_range(arguments: bytes) -> int | None:
    """Read the arguments of SET_RANGE.

    Returns:
        The range to lock, or None for automatic ranging.

    Raises:
        PayloadError: If the length is wrong or the range does not exist.
    """
    (value,) = _unpack_exact(_U8, arguments, "SET_RANGE")
    if value == RANGE_AUTO:
        return None
    if value >= RANGE_COUNT:
        raise PayloadError(f"SET_RANGE got unknown range {value}")
    return int(value)


def build_set_down_n(samples: int) -> CommandMessage:
    """Create a SET_DOWN_N command with the sample count of the step-down rule.

    Raises:
        ValueError: If the count does not fit in 16 bits.
    """
    return CommandMessage(Command.SET_DOWN_N, _pack(_U16, "sample count", samples))


def parse_set_down_n(arguments: bytes) -> int:
    """Read the arguments of SET_DOWN_N and return the sample count.

    Raises:
        PayloadError: If the length is wrong.
    """
    (samples,) = _unpack_exact(_U16, arguments, "SET_DOWN_N")
    return int(samples)


def build_start() -> CommandMessage:
    """Create a START command."""
    return CommandMessage(Command.START)


def build_stop() -> CommandMessage:
    """Create a STOP command."""
    return CommandMessage(Command.STOP)


def build_cal_zero() -> CommandMessage:
    """Create a CAL_ZERO command."""
    return CommandMessage(Command.CAL_ZERO)


def encode_zero_offsets(offsets: Sequence[float]) -> bytes:
    """Serialize the zero offsets that a CAL_ZERO response carries, in codes.

    Raises:
        ValueError: If there are not four offsets or one cannot be sent.
    """
    return _pack(_ZERO, "zero offsets", *_zero_to_wire(offsets))


def decode_zero_offsets(data: bytes) -> ZeroOffsets:
    """Parse the data of a CAL_ZERO response: the zero offsets, in codes.

    Raises:
        PayloadError: If the data is too short.
    """
    return _zero_from_wire(_unpack_prefix(_ZERO, data, "CAL_ZERO"))


def build_cal_read() -> CommandMessage:
    """Create a CAL_READ command, which asks for the calibration record."""
    return CommandMessage(Command.CAL_READ)


def build_cal_store(record: bytes) -> CommandMessage:
    """Create a CAL_STORE command carrying a calibration record.

    The record is a block that this layer does not look into; section 8 of
    the specification defines it.

    Raises:
        ValueError: If the record is empty or longer than ``CAL_RECORD_MAX``.
    """
    if not 0 < len(record) <= CAL_RECORD_MAX:
        raise ValueError(f"a calibration record has 1 to {CAL_RECORD_MAX} bytes, got {len(record)}")
    return CommandMessage(Command.CAL_STORE, bytes(record))


def parse_cal_store(arguments: bytes) -> bytes:
    """Read the arguments of CAL_STORE: the calibration record.

    Raises:
        PayloadError: If the record is empty or too long.
    """
    if not 0 < len(arguments) <= CAL_RECORD_MAX:
        raise PayloadError(
            f"CAL_STORE takes 1 to {CAL_RECORD_MAX} argument bytes, got {len(arguments)}"
        )
    return bytes(arguments)


def build_clear_fault() -> CommandMessage:
    """Create a CLEAR_FAULT command."""
    return CommandMessage(Command.CLEAR_FAULT)


def build_fault_raised(faults: Fault) -> EventMessage:
    """Create a FAULT_RAISED event carrying the latched faults."""
    return EventMessage(Event.FAULT_RAISED, _pack(_U16, "fault flags", int(faults)))


def parse_fault_raised(data: bytes) -> Fault:
    """Read the data of a FAULT_RAISED event.

    Raises:
        PayloadError: If the data is too short.
    """
    (faults,) = _unpack_prefix(_U16, data, "FAULT_RAISED")
    return Fault(int(faults))


def build_supply_changed(report: SupplyReport) -> EventMessage:
    """Create a SUPPLY_CHANGED event.

    Raises:
        ValueError: If the budget does not fit in 16 bits.
    """
    data = _pack(
        _SUPPLY, "supply report", int(report.supply), int(report.cc_class), report.budget_ma
    )
    return EventMessage(Event.SUPPLY_CHANGED, data)


def parse_supply_changed(data: bytes) -> SupplyReport:
    """Read the data of a SUPPLY_CHANGED event.

    Raises:
        PayloadError: If the data is too short or holds an unknown value.
    """
    supply, cc_class, budget_ma = _unpack_prefix(_SUPPLY, data, "SUPPLY_CHANGED")
    return SupplyReport(
        supply=_known(Supply, int(supply), "SUPPLY_CHANGED", "supply"),
        cc_class=_known(CcClass, int(cc_class), "SUPPLY_CHANGED", "CC class"),
        budget_ma=int(budget_ma),
    )


def build_notice(notice: Notice, zero_offsets: Sequence[float] | None = None) -> EventMessage:
    """Create a NOTICE event.

    Args:
        notice: What happened.
        zero_offsets: The new zero offsets in codes; required for
            ``Notice.ZERO_TAKEN`` and not allowed otherwise.

    Raises:
        ValueError: If the offsets are missing, not wanted or cannot be sent.
    """
    code = _U8.pack(Notice(notice))
    if notice is Notice.ZERO_TAKEN:
        if zero_offsets is None:
            raise ValueError("the notice ZERO_TAKEN carries the zero offsets")
        return EventMessage(Event.NOTICE, code + encode_zero_offsets(zero_offsets))
    if zero_offsets is not None:
        raise ValueError(f"the notice {notice.name} carries no zero offsets")
    return EventMessage(Event.NOTICE, code)


def parse_notice(data: bytes) -> NoticeMessage:
    """Read the data of a NOTICE event.

    Raises:
        PayloadError: If the data is too short or names an unknown notice.
    """
    (code,) = _unpack_prefix(_U8, data, "NOTICE")
    notice = _known(Notice, int(code), "NOTICE", "code")
    if notice is Notice.ZERO_TAKEN:
        return NoticeMessage(notice, _zero_from_wire(_unpack_prefix(_ZERO, data[1:], "NOTICE")))
    return NoticeMessage(notice)


def build_state_changed(state: DeviceState) -> EventMessage:
    """Create a STATE_CHANGED event carrying the new state."""
    return EventMessage(Event.STATE_CHANGED, _U8.pack(DeviceState(state)))


def parse_state_changed(data: bytes) -> DeviceState:
    """Read the data of a STATE_CHANGED event.

    Raises:
        PayloadError: If the data is too short or names an unknown state.
    """
    (state,) = _unpack_prefix(_U8, data, "STATE_CHANGED")
    return _known(DeviceState, int(state), "STATE_CHANGED", "state")
