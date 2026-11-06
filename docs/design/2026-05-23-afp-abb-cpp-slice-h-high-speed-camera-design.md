# AFP ABB C++ Slice H Design: High-Speed Camera Core Workflow

Date: 2026-05-23

## Goal

Convert the core high-speed visible-light camera workflow from the Python application into the C++ application.

This slice keeps the scope intentionally narrow. It covers camera discovery, camera opening, live preview, stop/close behavior, and basic camera parameter read/write. It does not convert video recording, YOLO flaw prediction, abrasion/misalignment plotting, or external camera-frame streaming.

The Python source remains read-only. All new code and documentation are saved in `AFP_ABB_CPP`.

## Source Behavior To Preserve

The C++ implementation will preserve the user-facing behavior from:

- `AFP_ABB_Python/src/services/sensor_service/highspeed_camera_service.py`
- `AFP_ABB_Python/src/core/subprocess/CamOperation_class.py`
- `AFP_ABB_Python/src/core/subprocess/MvCameraControl_class.py`

The original Python workflow uses the Hikrobot/MVS SDK to enumerate GigE and USB cameras, open the selected camera, start continuous image grabbing, display frames in the `HC_RD` label, and read/write these camera parameters:

- `AcquisitionFrameRate`
- `ExposureTime`
- `Gain`

The C++ implementation will preserve the existing UI and button meanings:

- `enum_cam`: enumerate available cameras into `camera_list`.
- `HC_machine`: open the selected camera.
- `HC_start`: start live preview.
- `HC_stop`: stop preview and close the camera.
- `get_parameter`: read frame rate, exposure time, and gain into the existing line edits.
- `set_parameter`: write frame rate, exposure time, and gain from the existing line edits.
- `HC_data`: show a clear "not converted yet" message for flaw prediction.

## Scope

Slice H includes:

- Dynamic loading of the Hikrobot/MVS camera SDK DLL.
- Enumeration of GigE and USB cameras.
- Open/close lifecycle for the selected camera.
- Continuous preview in the existing `HC_RD` label.
- Frame acquisition on a worker thread.
- UI updates on the Qt main thread only.
- Reading and writing frame rate, exposure time, and gain.
- User-facing error messages for missing SDK DLL, no camera devices, camera open failures, grabbing failures, and parameter failures.
- Hardware-free tests using a fake camera client.

Slice H excludes:

- YOLO flaw prediction and `HC_PData` image updates.
- Abrasion and misalignment plot updates.
- Video recording.
- DDS camera-frame publishing.
- Any UI redesign.
- Any camera feature not used by the original Python workflow.

## Camera SDK Loading

The C++ application will load the MVS SDK dynamically instead of linking directly against a local SDK installation.

This keeps the executable able to start when the camera runtime is missing, and it lets the application show a useful message instead of failing during process startup.

The SDK DLL search order will be:

1. A portable path beside the executable: `lib/sensors/high_speed_camera/MvCameraControl.dll`.
2. A configured path from `config/env_config.json`.
3. The original installed MVS runtime path used by the Python project: `C:/Program Files (x86)/Common Files/MVS/Runtime/Win64_x64/MvCameraControl.dll`.

The portable folder is preferred because the C++ release package should be copyable to another Windows computer. The installed MVS path remains a fallback for development machines.

The README will explain that the vendor DLL and any required vendor runtime files must be supplied according to the camera SDK license.

## Preview Pipeline

The live preview will not use DDS.

DDS is useful when another process or computer needs to subscribe to structured data. It is not a good default path for showing camera frames inside the same GUI because raw camera frames are large and would require extra serialization, generated types, QoS tuning, and runtime complexity.

The preview pipeline will be:

1. The UI asks `HighSpeedCameraService` to start grabbing.
2. The service starts a camera acquisition worker.
3. The worker grabs frames from the MVS SDK.
4. The worker converts each displayable frame into a `QImage`.
5. The worker emits the image through a Qt signal.
6. `HighSpeedCameraService` receives the image on the main Qt thread and updates `HC_RD`.

If frames arrive faster than the GUI can display them, the preview path will keep the newest frame and allow older preview frames to be dropped. This keeps the GUI responsive and avoids memory growth.

The implementation will first use an MVS SDK function that returns RGB image data, if that function is available in the resolved DLL. If the resolved SDK path only supports Bayer/raw frame access for this workflow, the implementation will add a small local Bayer BG to RGB conversion helper for preview display. Slice H will not add OpenCV only for live preview.

## Components

### `HighSpeedCameraClient`

`HighSpeedCameraClient` is the low-level wrapper around the Hikrobot/MVS SDK.

Responsibilities:

- Load and unload the camera SDK DLL.
- Resolve only the SDK functions required by this slice.
- Enumerate GigE and USB cameras.
- Open the selected camera.
- Start and stop grabbing.
- Read one displayable frame for the acquisition worker.
- Read and write frame rate, exposure time, and gain.
- Close and destroy the SDK camera handle.
- Store the most recent SDK error code for diagnostics.

The wrapper keeps SDK-specific types and DLL calls out of the UI service.

### `HighSpeedCameraDevice`

`HighSpeedCameraDevice` is a small value type describing one enumerated camera.

Responsibilities:

- Store the SDK index needed to open the camera.
- Store the transport type, such as GigE or USB.
- Store a display name matching the Python-style combo box text.
- Store the IP address or serial number when available.

### `HighSpeedCameraFrameWorker`

`HighSpeedCameraFrameWorker` owns the blocking frame-grabbing loop.

Responsibilities:

- Run on a non-UI thread.
- Ask the client for frames while preview is active.
- Convert SDK frame data into a `QImage`.
- Emit the newest frame to the service.
- Stop cleanly when requested.

### `HighSpeedCameraService`

`HighSpeedCameraService` replaces the current placeholder service.

Responsibilities:

- Connect existing UI buttons to camera actions.
- Populate `camera_list` after enumeration.
- Open the selected camera.
- Start and stop the preview worker.
- Update `HC_RD` on the Qt main thread.
- Read/write the existing parameter line edits.
- Report user-facing errors through the existing message-box path.
- Leave `HC_data` as a clear "flaw prediction is not converted yet" action.

## Error Handling

Missing SDK DLL:

- Show a clear message that the high-speed camera SDK DLL could not be loaded.
- Include the searched paths so deployment problems can be corrected.
- Keep the application running.

No devices:

- Show the same kind of informational message as the Python application.
- Leave `camera_list` empty.

Open/grab/parameter failures:

- Show the failing action and SDK return code in hexadecimal.
- Keep service state consistent so the user can retry or stop safely.

Stop/close:

- Must be safe when the camera is not open.
- Must stop the worker before closing the SDK camera handle.
- Must clear `HC_RD` and parameter fields in the same user-facing cases as the Python application.

## Configuration

Add a high-speed camera section to `config/env_config.json`.

Planned fields:

- `high_speed_camera.sdk_dll_path`: optional explicit path to `MvCameraControl.dll`.
- `high_speed_camera.portable_sdk_dll_path`: relative portable DLL path, defaulting to `lib/sensors/high_speed_camera/MvCameraControl.dll`.
- `high_speed_camera.installed_sdk_dll_path`: installed fallback path, defaulting to `C:/Program Files (x86)/Common Files/MVS/Runtime/Win64_x64/MvCameraControl.dll`.
- `high_speed_camera.frame_timeout_ms`: frame grab timeout, defaulting to `1000` to match the Python code.

The implementation will keep defaults in `AppConfig` and allow deployment-specific values to be changed without recompiling.

## Testing

Hardware-free tests will use a fake camera client.

Tests will cover:

- Enumeration populates the expected camera list text.
- Opening fails with a useful message when no camera is selected.
- Missing SDK DLL is reported without crashing.
- Start preview requires an open camera.
- A fake frame updates the preview path through the service.
- Stop preview is safe when called before opening.
- Parameter read writes the UI fields.
- Parameter write validates numeric input and calls the client with the expected values.

Manual hardware verification will be documented separately in the implementation plan because it requires a Hikrobot/MVS camera or compatible runtime.

## Redundancy Removed From The Python Version

The C++ slice will not convert these Python details:

- `sys.path` edits used only for Python import discovery.
- The unused thermal camera DLL load inside the high-speed camera service.
- Direct UI updates from the acquisition thread.
- YOLO and plotting state that belongs to a later flaw-prediction slice.
- Optional video writer fields that are not wired into the current C++ slice.

## Portability Notes

The release package should include the application executable, Qt runtime files, existing project runtime files, and the high-speed camera SDK DLLs permitted by the vendor license.

If the camera DLL is not bundled, the application should still run and all unrelated functions should remain usable. Camera buttons will report the missing DLL when used.
