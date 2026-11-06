# AFP ABB C++ Slice E Design: PLC DB18 Print Command Signaling

Date: 2026-05-22

Branch: `slice-e-plc-print-signal`

## Goal

Convert the Python PLC print command handshake into a small, tested C++ component.

Slice E adds the DB18 command/ACK/clear behavior used by the original Python print workflow, but it does not wire the main print buttons yet. This gives later robot-print slices a reliable C++ building block for start, pause, resume, and stop commands.

The main UI design remains unchanged. `AFP_ABB_Python` remains read-only.

## Source Behavior To Preserve

Slice E preserves the behavior from:

- `AFP_ABB_Python/src/services/plc_service/plc_signal_manager.py`
- the print-command calls inside `AFP_ABB_Python/src/services/robot_service/robot_service.py`

The Python behavior to preserve is:

- Commands are written to DB18.
- The command byte is `DB18.DBB0`.
- The acknowledgement byte is `DB18.DBB1`.
- The state byte is `DB18.DBB2`.
- The error byte is `DB18.DBB3`.
- The command values are:
  - `START = 10`
  - `PAUSE = 11`
  - `RESUME = 12`
  - `STOP = 13`
- After writing a command, the PC waits for `DB18.DBB1 == 1`.
- The ACK wait times out after 3 seconds.
- After a successful ACK, the PC clears the command byte by writing `DB18.DBB0 = 0`.

## Updated Slice E Scope

Slice E includes:

- Byte read/write support in the PLC client layer.
- A small print-signal manager for DB18 commands.
- Non-hardware automated tests using a fake PLC byte interface.
- README notes for the DB18 command channel.

Slice E excludes:

- UI wiring for `Robot_movebutton_Gcode`, `PauseRobotButton`, `ContinueRobotButton`, or `StopRobotButton`.
- Robot socket connection.
- Sending `LOAD_MOD:<filename>#` to the ABB robot socket.
- Checking FTP upload state before starting print.
- Reading or displaying print state from `DB18.DBB2`.
- Reading or displaying print errors from `DB18.DBB3`.
- Any change to the original `.ui` design.
- Any change to `AFP_ABB_Python`.

## Components

### PLC Byte Access

`PlcClient` currently supports PLC bit writes, INT writes, REAL writes, and marker-bit writes. Slice E needs byte-level DB access for DB18 command and ACK bytes.

The PLC client layer will add:

- `writeDbByte(int dbNumber, int byteOffset, quint8 value)`
- `readDbByte(int dbNumber, int byteOffset, quint8& value)`

These methods will use the same Snap7 connection management and error reporting style as the existing PLC methods.

### Testable PLC Byte Interface

To keep `PlcPrintSignalManager` testable without hardware, Slice E will introduce a small interface, for example:

```cpp
class IPlcByteAccess {
public:
    virtual ~IPlcByteAccess() = default;
    virtual PlcResult writeDbByte(int dbNumber, int byteOffset, quint8 value) = 0;
    virtual PlcResult readDbByte(int dbNumber, int byteOffset, quint8& value) = 0;
};
```

Slice E will make `PlcClient` implement this interface directly. That keeps the print-signal manager simple and avoids an extra adapter class that would only forward two methods.

### `PlcPrintSignalManager`

`PlcPrintSignalManager` is a focused C++ class that owns only DB18 print command signaling.

Responsibilities:

- Send `START`, `PAUSE`, `RESUME`, and `STOP` command values.
- Wait for ACK by polling `DB18.DBB1`.
- Clear the command byte after ACK.
- Return structured success/failure results.
- Keep the command constants and DB offsets in one place.

Planned public behavior:

- `startPrint(bool waitAck = true)`
- `pausePrint(bool waitAck = true)`
- `resumePrint(bool waitAck = true)`
- `stopPrint(bool waitAck = true)`
- a lower-level `sendCommand(PrintCommand command, bool waitAck = true)`

The manager will not show message boxes and will not know about Qt widgets.

## Data Flow

For a command such as START:

1. Caller invokes `startPrint(true)`.
2. `PlcPrintSignalManager` writes `DB18.DBB0 = 10`.
3. If ACK wait is disabled, the method returns success immediately.
4. If ACK wait is enabled, the manager polls `DB18.DBB1` every 50 ms.
5. If `DB18.DBB1 == 1` before 3 seconds, the manager writes `DB18.DBB0 = 0`.
6. The manager returns success.
7. If ACK is not observed before timeout, the manager returns failure and leaves the timeout message for the caller.

The command clear happens only after ACK, matching the Python behavior.

## Error Handling

The manager will return clear errors for:

- failed command write
- failed ACK read
- ACK timeout
- failed command clear
- invalid command value if a low-level command API is exposed

Hardware and network errors remain inside `PlcClient` as `PlcResult` failures.

## Redundant Python Code Not Converted

The following Python details are intentionally not converted:

- `PLCSignalManagerSimple`, because the DB18 manager is the active path.
- The old MB compatibility path, which the Python code already removed.
- Direct logging calls in the manager.
- UI message boxes inside robot print button methods.
- Any robot print-button decision logic.
- Any thread creation around pause/continue/stop socket messages.

## Testing

Automated non-hardware tests:

- START writes command value `10`.
- PAUSE writes command value `11`.
- RESUME writes command value `12`.
- STOP writes command value `13`.
- Successful ACK causes command clear to `0`.
- `waitAck=false` writes the command and does not poll ACK.
- ACK timeout returns failure.
- PLC read/write failure returns failure.
- The fake PLC records operation order so tests can verify `write command -> read ack -> clear command`.

Manual hardware test for a later slice:

- Configure PLC network settings in `config/env_config.json`.
- Trigger start/pause/resume/stop from the UI after the next slice wires the buttons.
- Confirm DB18 command values and ACK behavior on the PLC side.

Slice E itself should not require a live PLC for automated verification.

## Acceptance Criteria

- `AFP_ABB_Python` remains unchanged.
- The C++ UI design remains unchanged.
- `PlcClient` supports DB byte read/write.
- A print-signal manager can send START, PAUSE, RESUME, and STOP command values.
- The manager waits for ACK and clears the command after ACK.
- Automated tests pass without requiring a live PLC.
- Existing PLC, pressure, FTP, and app tests still pass.
- README documents the DB18 print command channel.
