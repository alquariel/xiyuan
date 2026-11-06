# AFP ABB C++ Slice F Design: Robot Print Workflow

Date: 2026-05-22

Branch: `codex/slice-f-robot-print-workflow`

## Goal

Convert the smallest useful part of the ABB robot print workflow into the C++ application.

Slice F connects the pieces created by earlier slices:

- Slice D FTP upload state: the selected `.mod` filename must be known before print starts.
- Slice E PLC DB18 print signaling: start, pause, resume, and stop commands must be sent through the PLC handshake.
- ABB robot socket filename transfer: the uploaded MOD filename must be sent to the robot before the PLC start command.

The main UI design remains unchanged. Non-print robot movement, robot jog, external-axis movement, and RAPID files remain out of scope. `AFP_ABB_Python` remains read-only.

## Source Behavior To Preserve

Slice F preserves the print-control behavior from:

- `AFP_ABB_Python/src/services/robot_service/robot_service.py`
- `AFP_ABB_Python/src/drivers/robots/abb.py`
- the upload state exposed by `AFP_ABB_Python/src/services/robot_service/ftp_service.py`
- the DB18 command behavior already converted from `AFP_ABB_Python/src/services/plc_service/plc_signal_manager.py`

The Python behavior to preserve is:

- Start print requires a successful MOD upload.
- Start print requires a non-empty uploaded MOD filename.
- Start print requires the robot/control link to be connected.
- Start print sends `LOAD_MOD:<uploaded_mod_filename>#` through the ABB robot motion socket.
- Start print then sends the PLC DB18 START command and waits for ACK.
- Pause print sends the PLC DB18 PAUSE command and waits for ACK.
- Continue print sends the PLC DB18 RESUME command and waits for ACK.
- Stop print sends the PLC DB18 STOP command and waits for ACK.
- Pause, continue, and stop also try to send `PAUSE#`, `CONTINUE#`, and `STOP#` to a robot-side monitor socket when available in Python, but the PLC handshake is the main control path.

## Slice F Scope

Slice F includes:

- A lightweight robot socket client for the existing string-based ABB socket protocol.
- A Qt-facing robot print service that replaces placeholder behavior for:
  - `ConnectionRobotButton`
  - `Robot_movebutton_Gcode`
  - `PauseRobotButton`
  - `ContinueRobotButton`
  - `StopRobotButton`
- Checks for FTP upload state before starting print.
- Sending `LOAD_MOD:<filename>#` before the PLC START command.
- PLC DB18 start, pause, resume, and stop calls through `PlcPrintSignalManager`.
- User-visible messages through the existing `messageRequested(title, message)` path.
- Automated tests using fake socket, fake FTP state, and fake PLC byte access.
- README notes for the robot print workflow and configuration.

Slice F excludes:

- Robot motion commands such as move, reset, jog, axis move, or external-axis move.
- Pose conversion and quaternion math.
- RAPID file conversion or generation.
- A robot-side monitor socket server.
- Reading or displaying print state from `DB18.DBB2`.
- Reading or displaying print error from `DB18.DBB3`.
- New UI controls or any `.ui` design change.
- Any dependency on Python at runtime.
- Any change to `AFP_ABB_Python`.

## Components

### `RobotSocketClient`

`RobotSocketClient` is a small C++ wrapper around the ABB robot TCP socket used by the Python print workflow.

Responsibilities:

- Connect to `robot.ip` and `robot.port_motion` from `config/env_config.json`.
- Disconnect and release the socket cleanly.
- Report whether the socket is currently connected.
- Send UTF-8 command strings exactly as the Python workflow sends them.
- Return structured success/failure results with readable error messages.

Planned public behavior:

- `connectToRobot()`
- `disconnectFromRobot()`
- `isConnected()`
- `sendRawMessage(QString message)`
- `sendLoadMod(QString modFilename)`
- `sendPause()`
- `sendContinue()`
- `sendStop()`

This class will not know about Qt widgets, FTP upload state, PLC DB addresses, pose math, or message boxes.

The implementation will use `QTcpSocket`. Qt is already part of the application, so this keeps the code lightweight and avoids another networking dependency.

### Socket Result Object

The socket client will use a small result object similar to `PlcResult`, for example:

```cpp
struct RobotSocketResult {
    bool ok = false;
    QString message;
};
```

This keeps network errors out of UI code and makes tests simple.

### Robot Print Service

The current `RobotService` placeholder will be replaced or narrowed into a real print-focused service while leaving non-print robot controls as placeholders.

Responsibilities:

- Connect the robot socket when `ConnectionRobotButton` is clicked.
- Track the print-link state used by print buttons.
- On start print:
  - verify FTP upload success
  - verify uploaded MOD filename is not empty
  - verify robot socket/control link is connected
  - send `LOAD_MOD:<filename>#`
  - send PLC DB18 START and wait for ACK
  - update stop/pause state to match Python behavior
- On pause print:
  - verify the print link is connected
  - send PLC DB18 PAUSE and wait for ACK
  - try the optional socket `PAUSE#` side message only after PLC ACK
- On continue print:
  - verify the print link is connected
  - send PLC DB18 RESUME and wait for ACK
  - try the optional socket `CONTINUE#` side message only after PLC ACK
- On stop print:
  - verify the print link is connected
  - send PLC DB18 STOP and wait for ACK
  - try the optional socket `STOP#` side message only after PLC ACK

The service will not own the generated UI object directly except where status text must match the original window. It should use existing signals for user messages.

### PLC Client Sharing

`PlcService` currently owns its own `PlcClient`. Slice F needs the print workflow to use the same configured PLC access path without duplicating PLC configuration logic.

The simple C++ design is:

- keep `PlcClient` as the low-level Snap7 owner
- let the print service own a `PlcClient` configured from `AppConfig::plc`
- construct a `PlcPrintSignalManager` from that client

This duplicates the network handle, not the PLC implementation. It is acceptable for this slice because the hidden Python PLC connect button was already replaced by automatic per-command connection behavior. A later refactor can share one application-level PLC client if it becomes necessary.

## Data Flow

### Robot Connection

1. User clicks `ConnectionRobotButton`.
2. `MainWindow` calls the robot print service connection slot.
3. `RobotSocketClient` connects to `robot.ip:robot.port_motion`.
4. The service updates `robot_state_show` with connected or failed status.
5. The service records whether print commands are allowed.

### Start Print

1. User uploads a `.mod` file through Slice D FTP behavior.
2. User clicks `Robot_movebutton_Gcode`.
3. Robot print service checks `FtpService::uploadSucceeded()`.
4. Robot print service reads `FtpService::uploadedModFilename()`.
5. If either value is invalid, the service shows the same kind of warning as the Python UI and stops.
6. If the robot socket is not connected, the service shows a robot-not-connected warning and stops.
7. `RobotSocketClient` sends `LOAD_MOD:<uploadedModFilename>#`.
8. `PlcPrintSignalManager::startPrint(true)` writes DB18 START, waits for ACK, and clears the command after ACK.
9. The service shows success or PLC handshake failure.

### Pause, Continue, Stop

1. User clicks the corresponding button.
2. The service checks the print link state.
3. The service sends the matching PLC DB18 command and waits for ACK.
4. If PLC ACK succeeds, the service updates local pause/stop state.
5. The service sends the optional socket side message:
   - pause: `PAUSE#`
   - continue: `CONTINUE#`
   - stop: `STOP#`
6. Optional side-message failure is reported as non-fatal because the Python workflow already depends primarily on the PLC handshake for these three controls.

## Configuration

Slice F uses the existing robot configuration keys:

```json
"robot": {
  "ip": "192.168.0.104",
  "port_motion": 5010,
  "port_server": 5012,
  "transport": {
    "type": "plc"
  }
}
```

`robot.port_motion` is the socket used by Slice F for `LOAD_MOD:<filename>#`.

`robot.port_server` remains parsed but unused in this slice. The Python code passes it into `abb.Robot`, but the print-start path sends the MOD filename through the motion socket. Converting the server/monitor socket can be revisited when a later slice converts broader robot motion or monitoring behavior.

The existing `robot.transport` value will not add a new behavior path in Slice F. The C++ print workflow still requires the robot socket for `LOAD_MOD:<filename>#`, matching the Python start-print requirement.

## Error Handling

The service will report clear errors for:

- robot socket IP missing
- robot motion port invalid
- robot socket connection failure
- FTP upload not completed before start print
- uploaded MOD filename empty
- robot socket not connected before start print
- failed `LOAD_MOD:<filename>#` send
- PLC START/PAUSE/RESUME/STOP write failure
- PLC ACK timeout
- PLC command-clear failure after ACK

For pause, continue, and stop:

- PLC command failure is fatal for the requested action.
- Optional socket side-message failure is non-fatal and reported as an informational warning if needed.

## Redundant Python Code Not Converted

The following Python details are intentionally not converted:

- global `R` assignment
- thread creation for pause, continue, and stop side messages
- reconnect dialog with Retry/Ok
- Python logging separator lines
- broad pose setup during robot connection
- quaternion conversion
- robot jog/move/external-axis methods
- monitor socket setup
- RAPID file handling
- any `pythonLIb` runtime path

## Testing

Automated non-hardware tests:

- robot socket client formats `LOAD_MOD:<filename>#`.
- start print fails if FTP upload did not succeed.
- start print fails if uploaded MOD filename is empty.
- start print fails if robot socket is not connected.
- start print sends `LOAD_MOD` before PLC START.
- start print reports PLC handshake failure.
- pause sends DB18 PAUSE and then optional `PAUSE#`.
- continue sends DB18 RESUME and then optional `CONTINUE#`.
- stop sends DB18 STOP and then optional `STOP#`.
- optional pause/continue/stop socket side-message failure does not hide a successful PLC ACK.

Manual hardware or RobotStudio tests:

- Configure `robot.ip` and `robot.port_motion`.
- Configure PLC settings and FTP settings.
- Upload a `.mod` file from the C++ UI.
- Click `ConnectionRobotButton`.
- Click `Robot_movebutton_Gcode`.
- Confirm the robot receives the MOD filename.
- Confirm PLC DB18 receives START and ACK clears the command.
- Test pause, continue, and stop buttons.

## Acceptance Criteria

- `AFP_ABB_Python` remains unchanged.
- The C++ UI design remains unchanged.
- `ConnectionRobotButton` no longer shows a placeholder for the print socket path.
- `Robot_movebutton_Gcode` checks FTP upload state before attempting print start.
- Start print sends `LOAD_MOD:<filename>#` before DB18 START.
- Pause, continue, and stop use the DB18 command manager.
- Non-print robot controls remain placeholders.
- Automated tests pass without requiring a live PLC or robot.
- Existing app, PLC, pressure, and FTP tests still pass.
- README explains robot print workflow configuration and limitations.
