# AFP ABB C++ Slice D Design: FTP MOD File Upload

Date: 2026-05-21

Branch: `slice-d-ftp-upload`

## Goal

Convert the Python FTP MOD file upload path into the C++ application as the next small, controlled migration slice.

Slice D replaces the current placeholder behavior behind the existing `locusselectsetButton`. The user will be able to select a `.mod` path file and upload it to the ABB controller FTP server using the FTP settings already present in `config/env_config.json`.

The main UI design remains unchanged. Slice D does not convert robot motion commands, print start/pause/continue/stop, camera workflows, serial workflows, or any RAPID files. `AFP_ABB_Python` remains read-only.

## Source Behavior To Preserve

Slice D preserves the behavior from:

- `AFP_ABB_Python/src/services/robot_service/ftp_service.py`
- the FTP-related state initialized in `AFP_ABB_Python/src/ui/mywindow_class.py`
- the upload checks used later by `Startprint_button()` in `AFP_ABB_Python/src/services/robot_service/robot_service.py`

The Python behavior to preserve is:

- Clicking `locusselectsetButton` opens a file picker.
- The Python file picker started in `./pythonLIb/printpath/`, but the C++ port will not preserve this Python-era runtime path.
- The C++ file picker starts in a portable `mod_files` folder beside `AFP_ABB_CPP.exe`.
- The selectable file type is `.mod`.
- Cancelling selection shows a warning and does not change upload state.
- The selected local path is saved as the current path file.
- The remote filename is the selected file basename.
- The selected file is uploaded through FTP using config values:
  - `ftp.ip`
  - `ftp.port`
  - `ftp.user`
  - `ftp.password`
- After upload, the remote listing is checked for the uploaded filename.
- On success, the service records:
  - local path
  - uploaded MOD filename
  - upload success flag
- On failure, the selected path is cleared and upload success is false.

## Updated Slice D Scope

Slice D includes:

- A low-level FTP client wrapper.
- A Qt-facing FTP service that owns upload state.
- UI wiring for the existing `locusselectsetButton`.
- File dialog behavior matching the Python service except for replacing the Python-specific start folder with a C++ portable start folder.
- User-visible success/failure messages.
- README updates for FTP upload behavior.
- Automated tests for upload-state handling and validation paths that do not require a live FTP server.

Slice D excludes:

- Robot socket connection.
- PLC print handshake.
- Sending the uploaded MOD filename to the robot.
- Start, pause, continue, and stop print behavior.
- Any changes to the original `.ui` design.
- Any dependency on Python scripts at runtime.
- Any FTP server implementation.

## Chosen FTP API

The C++ implementation will use the Windows WinINet FTP API.

Reasons:

- The target application is Windows-only.
- WinINet is already provided by Windows, so the portable folder does not need another third-party FTP runtime.
- It avoids fragile manual FTP protocol code.
- It is lighter than adding libcurl for this one workflow.

The low-level FTP client will be isolated so a later slice can replace it with another backend if needed.

## Components

### `FtpClient`

`FtpClient` is a small C++ wrapper around WinINet.

Responsibilities:

- Open an internet session.
- Connect to the configured FTP server.
- Log in with configured credentials.
- Upload one local file as one remote filename.
- Check whether the remote filename appears in the FTP directory listing.
- Close all WinINet handles.
- Return structured success/failure results with readable error text.

This class will not know about Qt widgets or file dialogs.

### `FtpService`

`FtpService` is the Qt-facing service that replaces the placeholder FTP service.

Responsibilities:

- Open the `.mod` file picker when `locusselectsetButton` is clicked.
- Use a portable default directory for MOD file selection.
- Preserve the Python-compatible `.mod` filter.
- Call `FtpClient` to upload the selected file.
- Store upload state for later robot-print slices.
- Emit message-box requests through the existing `MainWindow` message path.

Planned state:

- `selectedLocalPath`
- `uploadedModFilename`
- `uploadSucceeded`

This state is intentionally kept inside `FtpService` for Slice D. When the robot print workflow is converted, that slice will decide whether to pass state directly between services or introduce a small shared print-job state object.

## Data Flow

1. User clicks `locusselectsetButton`.
2. `MainWindow` calls `FtpService::openLocus()`.
3. `FtpService` opens a file dialog at the portable MOD directory with filter `*.mod`.
4. If the user cancels, `FtpService` emits a warning and leaves the previous successful upload state unchanged only if no new upload was attempted.
5. If a file is selected, `FtpService` stores the local path and extracts the basename for the remote filename.
6. `FtpClient` connects and uploads the file.
7. `FtpClient` verifies the remote filename through FTP listing.
8. `FtpService` records success or clears upload state on failure.
9. `FtpService` emits the same kind of user-visible result message the Python UI showed.

## Configuration

No new configuration keys are needed for Slice D.

Existing config is used:

```json
"ftp": {
  "ip": "192.168.0.104",
  "port": 21,
  "user": "Default User",
  "password": "robotics"
}
```

Validation:

- `ip` must be non-empty.
- `port` must be greater than 0.
- `user` and `password` may be empty only if an FTP server accepts anonymous/default login, but the default config remains unchanged.

## MOD File Start Directory

The Python default file picker directory `./pythonLIb/printpath/` is intentionally not converted because the C++ portable package will not contain a `pythonLIb` runtime folder.

Slice D will use this C++ runtime directory strategy:

1. Prefer a `mod_files` folder beside `AFP_ABB_CPP.exe`.
2. If `mod_files` does not exist, fall back to the application folder.
3. Include an empty `mod_files` folder in the deployable package so normal portable builds have the expected operator-facing location.

The folder is only the file picker starting point. The user can still browse to another folder and select any valid `.mod` file.

## Error Handling

The service will report clear errors for:

- user cancelling file selection
- selected file missing
- selected path not ending in `.mod`
- missing FTP host or invalid port
- connection failure
- login failure
- upload failure
- remote listing verification failure

Errors are shown through the existing `messageRequested(title, message)` signal path so the UI remains consistent with previous C++ slices.

## Redundant Python Code Not Converted

The following Python details are intentionally not converted:

- direct `QMessageBox` calls inside the low-level FTP upload function
- logging separator lines
- Python's implicit attachment of upload state to the main window object
- the Python-specific `./pythonLIb/printpath/` default directory
- robot-print checks beyond preserving the upload state that they will need later
- any RAPID file handling outside selecting and uploading a `.mod` file

## Testing

Automated non-network tests:

- selected file basename becomes the remote filename
- missing local file fails before network use
- non-`.mod` path is rejected
- successful upload result stores local path, remote filename, and success flag
- failed upload clears the selected path and success flag

Manual tests with ABB controller or RobotStudio FTP enabled:

- Configure `ftp` in `config/env_config.json`.
- Start the C++ app.
- Click `locusselectsetButton`.
- Select a `.mod` file.
- Confirm upload success message.
- Confirm the remote FTP directory contains the selected basename.

## Acceptance Criteria

- `AFP_ABB_Python` remains unchanged.
- The C++ UI design remains unchanged.
- `locusselectsetButton` no longer shows a placeholder.
- A selected `.mod` file is uploaded to the configured FTP server.
- Upload success state is available for a later robot print workflow slice.
- Automated tests pass without requiring a live FTP server.
- Existing PLC and pressure tests still pass.
- README explains the FTP upload behavior and configuration.
