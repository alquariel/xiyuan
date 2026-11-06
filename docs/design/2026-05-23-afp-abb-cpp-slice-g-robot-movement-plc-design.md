# AFP ABB C++ Slice G Design: PLC-Forward Robot Movement Controls

Date: 2026-05-23

Branch: `codex/slice-g-robot-movement-plc`

## Goal

Convert the remaining operator-facing ABB robot movement controls using the PLC-forward path from the Python software.

Slice G will replace the placeholder behavior behind these existing buttons:

- `Robot_movebutton`
- `RobotGohomeButton`
- `RobotaxismoveButton`
- `PositionermoveButton`
- `PositionercomebackButton`

The main UI design remains unchanged. `AFP_ABB_Python` remains read-only.

## Source Behavior To Preserve

Slice G preserves behavior from:

- `AFP_ABB_Python/src/services/robot_service/robot_service.py`
- `AFP_ABB_Python/src/drivers/robots/abb_plc.py`
- the robot configuration already copied into `AFP_ABB_CPP/config/env_config.json`

The current C++ config uses:

```json
"robot": {
  "transport": {
    "type": "plc"
  }
}
```

Because of that, Slice G will follow the Python `PlcRobot` path rather than the socket-only `abb.Robot` movement path.

The Python behavior to preserve is:

- Movement controls require an active robot/control link.
- Cartesian movement reads X, Y, Z, RX, RY, and RZ fields.
- Cartesian reset reads zero-position X, Y, and Z fields.
- Joint movement reads six axis fields.
- External-axis movement reads three positioner fields.
- External-axis reset sends `[-1900, 0, 0, 0, 0, 0]`.
- Robot and external-axis speed fields are optional and fall back to configured defaults that preserve the Python default values.
- PLC-forward movement writes scaled INT32 values to mapped PLC process output bytes.
- Each movement command increments a sequence number, writes a command code, waits for ACK, then waits for DONE.

## Slice G Scope

Slice G includes:

- PLC process-area INT32/byte read and write support needed by `abb_plc.py`.
- A small PLC-forward robot movement client.
- Quaternion math needed by the existing Cartesian move and reset buttons.
- Robot movement default speed values loaded from `config/env_config.json`.
- A Qt-facing movement service for the five existing movement buttons.
- UI wiring for only those five movement buttons.
- User-visible messages through the existing `messageRequested(title, message)` path.
- Hardware-free automated tests using fake PLC process-area access and fake UI inputs.
- README notes for PLC-forward robot movement configuration and limitations.

Slice G excludes:

- Any change to `src/ui/Interface4600_window.ui`.
- Any change to `AFP_ABB_Python`.
- Socket-based robot movement commands such as `01`, `02`, `08`, or `34`.
- RAPID file changes.
- Print workflow changes from Slice F.
- Camera workflows.
- Serial-device workflows.
- Reading and displaying live robot pose.
- Robot monitoring socket behavior.

## Components

### PLC Process Area Access

The Python `PlcRobot` movement path uses Snap7 process areas:

- SEND area: process outputs, Snap7 `PA`
- RECV area: process inputs, Snap7 `PE`

Slice G will add process-area methods to the PLC client layer, for example:

- `writeProcessOutputInt32(int byteOffset, qint32 value)`
- `readProcessOutputInt32(int byteOffset, qint32& value)`
- `readProcessInputInt32(int byteOffset, qint32& value)`
- `readProcessInputByte(int byteOffset, quint8& value)`

The C++ implementation will preserve the Python little-endian INT32 packing from `abb_plc.py`:

```python
struct.pack("<i", value)
```

This is different from S7 DB INT/REAL encoding already used elsewhere in the project, so process-area packing will be kept separate and explicitly tested.

### Runtime Speed Configuration

The Python app stores movement speed defaults in the main-window state. Slice G will move those values into the C++ runtime config so operators can tune them without recompiling.

Add a `default_speed` object under `robot`:

```json
"robot": {
  "default_speed": {
    "tcp_linear": 200.0,
    "tcp_orientation": 200.0,
    "external_linear": 200.0,
    "external_orientation": 100.0
  }
}
```

These values preserve the Python defaults:

```text
robot_line_speed = 200.0
robot_orientation_speed = 200.0
external_axis_line_speed = 200.0
external_axis_orientation_speed = 100.0
```

If the config keys are missing, the C++ loader will use the same fallback values. UI speed fields still override the defaults when the operator enters values.

### PLC-Forward Robot Client

Add a focused robot movement client, for example `PlcRobotMotionClient`.

Responsibilities:

- Keep the same mapped byte offsets as Python `PlcRobotConfig`.
- Store pending speed and external-axis state.
- Convert movement values to scaled INT32 values using `scale = 1000`.
- Send Cartesian, joint, and external-axis absolute commands.
- Increment `md_send_seq`.
- Wait for ACK and DONE using the receive-area sequence/status bytes.
- Return structured success/failure results.

Command values:

```text
CMD_COMPOSITE  = 120
CMD_JOINT_ABSJ = 2
CMD_EXTAX_ABSJ = 34
```

Important send offsets:

```text
99  pos_x
103 pos_y
107 pos_z
111 q1 / joint4
115 q2 / joint5
119 q3 / joint6
123 q4
127 ext_inc
131 ext_spin
135 ext_gr
139 speed_vtcp
143 speed_vori
147 speed_vleax
151 speed_vreax
155 command_code
159 sequence
```

Important receive offsets:

```text
145 ack_sequence
149 done_sequence
153 status byte
154 error code byte
```

Status bits:

```text
bit 0 = busy
bit 1 = error
```

### Quaternion Helper

The Python UI converts RX, RY, RZ into a quaternion by calling:

```python
toq.calc_to_quaternion([a, b, c], -30)
```

Slice G will convert the needed math into a small C++ helper. It will not depend on SciPy or Python at runtime.

The helper will preserve the behavior needed by the robot movement UI:

- Convert the input vector to Euler angles using the Python `to_eulerangles` logic.
- Convert those Euler angles to a quaternion using Python `to_quaternion` logic.
- Apply the local Z-axis `-30` degree rotation used by Python.
- Return the ABB-style quaternion order expected by the movement client.

This helper will be covered with deterministic tests against known Python-derived values.

### Qt-Facing Robot Movement Service

The current `RobotService` placeholder will be replaced or narrowed for movement controls while keeping unconverted methods as placeholders where needed.

Responsibilities:

- Read existing UI line edits.
- Validate required fields for each movement command.
- Use configured default speeds, falling back to the Python values above when config keys are missing.
- Update speed values when optional speed fields are filled.
- Send the matching movement command through `PlcRobotMotionClient`.
- Show success/failure messages through the existing message signal.

Required fields:

- Cartesian move: `RobotXlineEdit`, `RobotYlineEdit`, `RobotZlineEdit`, `RobotRXlineEdit`, `RobotRYlineEdit`, `RobotRZlineEdit`
- Reset/home move: `zero_position_x`, `zero_position_y`, `zero_position_z`
- Joint move: `axis1lineEdit` through `axis6lineEdit`
- External-axis move: `Positioner1lineEdit`, `Positioner2lineEdit`, `Positioner3lineEdit`
- External-axis reset: no required position fields

The service will not modify the `.ui` file.

## Data Flow

### Cartesian Move

1. User fills X, Y, Z, RX, RY, and RZ fields.
2. User clicks `Robot_movebutton`.
3. The movement service validates all six fields.
4. The service reads optional speed fields.
5. The quaternion helper builds the pose.
6. The movement client writes pose, pending external-axis values, speed values, command `120`, and the next sequence.
7. The client waits for ACK, then DONE.
8. The service shows success or failure.

### Robot Home

1. User fills zero-position X, Y, and Z.
2. User clicks `RobotGohomeButton`.
3. The movement service builds the home pose using orientation vector `[0, 0, -1]`.
4. The movement client sends the same Cartesian command path as above.

### Joint Move

1. User fills axis 1 through axis 6.
2. User clicks `RobotaxismoveButton`.
3. The movement client writes joint values into the existing position/quaternion mapped fields.
4. The movement client writes command `2`, sequence, and waits for ACK/DONE.

### External-Axis Move

1. User fills positioner 1 through positioner 3.
2. User clicks `PositionermoveButton`.
3. The service preserves Python ordering by sending `[Axis3, Axis1, Axis2, 0, 0, 0]`.
4. The movement client writes command `34`, sequence, and waits for ACK/DONE.

### External-Axis Reset

1. User clicks `PositionercomebackButton`.
2. The service sends `[-1900, 0, 0, 0, 0, 0]`.
3. The movement client writes command `34`, sequence, and waits for ACK/DONE.

## Error Handling

The movement service will report clear failures for:

- missing required fields
- invalid numeric values
- PLC connection failure
- process-area read/write failure
- ACK timeout
- DONE timeout
- robot error status bit while waiting ACK or DONE

ACK wait default: 10 seconds, matching Python.

DONE wait default: 60 seconds, matching Python.

Polling interval: 50 ms, matching Python.

## Redundant Python Code Not Converted

The following Python details are intentionally not converted:

- Python threads around movement calls; Qt can keep the first C++ slice synchronous and bounded.
- Global `R`.
- Socket movement driver path from `abb.py`.
- RAPID handling.
- Robot pose readback display.
- Robot monitor/server socket behavior.
- Logging separator lines.
- Retry dialogs.
- Any SciPy or Python runtime dependency.

## Testing

Automated non-hardware tests:

- Config loading preserves Python speed defaults when `robot.default_speed` is missing.
- Config loading accepts explicit `robot.default_speed` values.
- Process-area INT32 little-endian encode/decode.
- PLC-forward client writes the correct offsets for Cartesian command `120`.
- PLC-forward client writes the correct offsets for joint command `2`.
- PLC-forward client writes the correct offsets for external-axis command `34`.
- Sequence number increments from the current send sequence.
- ACK timeout returns failure.
- DONE timeout returns failure.
- Robot error status returns failure.
- Movement service rejects missing required Cartesian fields.
- Movement service sends external-axis move as `[Axis3, Axis1, Axis2, 0, 0, 0]`.
- Movement service sends external-axis reset as `[-1900, 0, 0, 0, 0, 0]`.
- Quaternion helper returns known Python-derived values.

Manual hardware or RobotStudio tests:

1. Configure PLC settings in `config/env_config.json`.
2. Confirm robot transport type is `plc`.
3. Connect the robot/control link from the existing UI.
4. Test Cartesian move with a safe small movement.
5. Test home/reset movement with safe zero-position values.
6. Test joint move with safe axis values.
7. Test external-axis move and reset.
8. Confirm movement commands appear in the PLC mapped process output area.

## Acceptance Criteria

- `AFP_ABB_Python` remains unchanged.
- The C++ UI design remains unchanged.
- Five remaining movement buttons no longer show placeholders.
- Movement commands follow the Python PLC-forward process-area mapping.
- The existing Slice F print workflow remains unchanged.
- Non-hardware tests cover movement command formatting and timeout/error behavior.
- Existing app, PLC, pressure, FTP, and print workflow tests still pass.
- README documents PLC-forward robot movement behavior and limitations.
- README documents the robot movement default-speed config keys.
