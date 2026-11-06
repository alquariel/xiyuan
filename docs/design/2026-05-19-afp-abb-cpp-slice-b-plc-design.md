# AFP ABB C++ Slice B Design: PLC Direct UI Controls

Date: 2026-05-19

## Goal

Convert the direct Siemens S7-1200 PLC UI controls from the Python application into the C++ application.

This slice keeps the scope intentionally narrow. It covers the PLC actions that are directly triggered by the existing UI buttons for motor control, cylinders, laser control, temperature/power settings, and hysteresis ratio. It does not convert the DB18 print-command signaling in `plc_signal_manager.py`; that logic belongs with a later robot/process-control slice.

The Python source remains read-only. All new code and documentation are saved in `AFP_ABB_CPP`.

## Source Behavior To Preserve

The C++ implementation will preserve the behavior from:

- `AFP_ABB_Python/src/drivers/plcs/PLC.py`
- `AFP_ABB_Python/src/services/plc_service/plc_service.py`

The service will use the PLC connection settings already present in `config/env_config.json`:

- PLC IP address
- rack
- slot

The default values remain compatible with the Python project:

- IP: `192.168.0.201`
- rack: `0`
- slot: `1`

## Scope

Slice B includes these user-facing PLC actions:

- PLC connect/disconnect state handling used by the direct controls.
- Cylinder 1 command.
- Cylinder 2 toggle.
- Cylinder 3 toggle.
- Motor jog/continuous mode write.
- Motor enable toggle.
- Motor reset command.
- Motor actuation command.
- Wire feed rate and paving distance writes.
- Hysteresis ratio write.
- Laser connect.
- Red light open/close.
- Laser start/stop.
- Target temperature write.
- Temperature control toggle.
- Laser power write.
- Laser power control toggle.

Slice B excludes:

- `plc_signal_manager.py` DB18 command/ack logic for print start, pause, resume, and stop.
- Robot transport behavior that depends on PLC signaling.
- Any new PLC features not present in the Python code.
- Any UI redesign.

## Approach

Use the native Snap7 C/C++ client through a small wrapper.

This is the closest equivalent to the Python `snap7` dependency and keeps the C++ code lightweight. The wrapper keeps Snap7-specific API calls out of the UI service, making it possible to unit-test PLC data encoding without real hardware.

Alternatives considered:

- A different S7 library: rejected for this slice because it increases protocol risk and may not match the Python behavior as closely.
- A Python bridge: rejected because the final application must be a portable C++ executable without a Python runtime dependency.

## Components

### `PlcClient`

`PlcClient` is a low-level wrapper around the Snap7 client.

Responsibilities:

- Connect to the configured PLC IP, rack, and slot.
- Disconnect cleanly.
- Report whether the client is connected.
- Read and write PLC memory areas used by the Python code:
  - DB byte/bit values.
  - DB integer values.
  - DB real values.
  - M bit values needed by motor reset.
- Return structured success/failure results instead of printing to stdout.

`PlcClient` will not know about Qt widgets or button behavior.

### PLC Data Helpers

Small helper functions will encode and decode Siemens data formats.

Responsibilities:

- Set and clear one bit inside a byte.
- Encode/decode big-endian 16-bit integers.
- Encode/decode big-endian 32-bit floating-point values compatible with S7 REAL values.

These helpers are unit-testable without Snap7 or hardware.

### `PlcService`

`PlcService` is the Qt-facing behavior layer for direct PLC UI controls.

Responsibilities:

- Own the `PlcClient`.
- Track local UI-related state:
  - PLC connected.
  - motor enabled.
  - red light state.
  - laser connected state.
  - target temperature set.
  - temperature control active.
  - laser power set.
  - laser power control active.
- Implement slots matching the existing PLC button wiring in `MainWindow`.
- Read values from the existing UI fields and write the same PLC addresses used by Python.
- Update the same labels, checkboxes, and button styles that the Python service updates.
- Emit message signals for user-visible errors, following the existing placeholder service pattern.

`PlcService` may receive the generated UI object from `MainWindow`, but the generated `.ui` file itself will not be modified.

### `MainWindow`

`MainWindow` will stop using the placeholder PLC service and instead construct the real PLC service with:

- PLC configuration from `AppConfig`.
- Access to the generated UI object.

Other service placeholders remain unchanged.

## PLC Address Map

The implementation will keep these Python addresses:

| Behavior | PLC address |
| --- | --- |
| Motor jog mode | DB5, byte 38, int value `1` |
| Motor continuous mode | DB5, byte 38, int value `3` |
| Cylinder 1 | DB5, byte 14, bit 5 |
| Cylinder 2 | DB5, byte 14, bit 6 |
| Cylinder 3 | DB5, byte 14, bit 7 |
| Motor enable | DB5, byte 15, bit 1 |
| Motor reset | M12.7 |
| Motor actuation | DB5, byte 15, bit 0 and DB5, byte 40, bit 0 |
| Paving distance | DB5, byte 16, S7 REAL |
| Wire feed rate | DB5, byte 20, S7 REAL |
| Hysteresis ratio | DB5, byte 34, S7 REAL |
| Hysteresis apply flag | DB5, byte 40, bit 1 |
| Laser connect | DB5, byte 14, bit 4 |
| Red light open step 1 | DB5, byte 14, bit 3 |
| Red light open step 2 | DB5, byte 14, bit 0 |
| Red light close | DB5, byte 14, bit 1 |
| Laser start stop-red-light step | DB5, byte 14, bit 1 |
| Laser start | DB5, byte 14, bit 2 |
| Laser stop / red light on | DB5, byte 14, bit 3 |
| Target temperature | DB5, byte 0, S7 REAL |
| Temperature control | DB5, byte 4, bit 0 |
| Laser power | DB5, byte 6, S7 REAL |
| Laser power control | DB5, byte 4, bit 1 |

## Behavioral Notes

The Python code sometimes writes in `while read false: write true` loops. In C++, these will become bounded confirmation helpers:

- Write the requested value.
- Optionally read back the same value.
- Retry a small fixed number of times or until a short timeout expires.
- Report failure to the user if confirmation does not happen.

This keeps the application responsive and avoids freezing the UI forever if the PLC is offline or the bit cannot be confirmed.

The C++ code will not copy Python's global `plc` and global `P` state. The PLC client and state will be owned by `PlcService`.

The typo-like Python state `set_poewr` will become `powerSet_` in C++ while preserving the same behavior: laser power control cannot be enabled until power has been set.

## Error Handling

PLC operations can fail due to network, PLC state, address errors, or unavailable Snap7 runtime files.

The C++ service will:

- Check connection before PLC writes.
- Try to connect using `AppConfig::plc` when a direct PLC operation needs a connection.
- Report connection and write failures through a Qt signal that `MainWindow` displays in a message box.
- Keep local UI state unchanged when a required PLC write fails.
- Avoid throwing exceptions across Qt signal/slot boundaries.

## Portability

The portable Windows build must include:

- `AFP_ABB_CPP.exe`
- Qt runtime files copied by `windeployqt`
- Snap7 runtime DLL files required by the PLC client
- `config/`
- `assets/`

The README will document how to install/build with Snap7 and how to verify that the portable output folder contains the required PLC runtime DLL.

## Tests

Automated tests for Slice B:

- PLC bit helper tests.
- S7 integer encoding/decoding tests.
- S7 REAL encoding/decoding tests.
- Basic service-state tests where possible without hardware.

Manual hardware verification:

- Build and deploy the portable folder.
- Run on the development PC connected to the PLC network.
- Verify PLC connect/disconnect.
- Verify motor mode, enable, reset, and actuation.
- Verify cylinder toggles.
- Verify laser connect, red light, start/stop, target temperature, power, and control toggles.

## Redundant Python Code Not Converted

The following Python patterns are intentionally not converted:

- Global PLC singleton state.
- Print-only helper methods that are not part of UI behavior.
- Unused generic read/write helpers unrelated to this slice's UI actions.
- Infinite UI-thread polling loops.
- DB18 print-command signaling from `plc_signal_manager.py`.

## Acceptance Criteria

- The generated C++ UI design remains unchanged.
- The Python folder is not modified.
- PLC placeholder actions are replaced only for the approved direct PLC controls.
- The C++ project configures and builds on Windows.
- Existing Slice A tests still pass.
- New PLC helper tests pass.
- README documents the Snap7 dependency and portable deployment requirement.
- The implementation is committed in `AFP_ABB_CPP` only.
