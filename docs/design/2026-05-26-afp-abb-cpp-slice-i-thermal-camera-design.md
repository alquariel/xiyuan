# AFP ABB C++ Slice I Design: Thermal Camera JPEG Cache Workflow

Date: 2026-05-26

## Goal

Convert the existing thermal camera workflow from the Python application into the C++ application while preserving the original UI and behavior.

This slice intentionally uses the Python-style JPEG cache pipeline first. That makes the C++ behavior easier to compare against the original Python software during real hardware testing. A lower-latency in-memory display pipeline can be considered later only after the original workflow is working reliably.

The Python source remains read-only. All new code and documentation are saved in `AFP_ABB_CPP`.

The unresolved high-speed camera hardware/runtime issue, currently seen as SDK return code `0x80000006`, stays open and is not part of this slice.

## Source Behavior To Preserve

The C++ implementation will preserve the user-facing behavior from:

- `AFP_ABB_Python/src/drivers/sensors/thermal_camera.py`
- `AFP_ABB_Python/src/services/sensor_service/thermal_camera_display.py`
- `AFP_ABB_Python/src/ui/mywindow_class.py`

The original Python workflow uses the Hikvision network camera SDK path:

- `HCNetSDK.dll` logs into the thermal camera using `thermal_camera.ip`, `thermal_camera.port`, `thermal_camera.user`, and `thermal_camera.password`.
- `NET_DVR_RealPlay_V40` starts the preview stream.
- `PlayCtrl.dll` receives stream data, decodes frames, and converts decoded frames to JPEG files.
- JPEG files are written into `logs/pic/cache`.
- The same JPEG files are copied into `logs/pic/saved`.
- A Qt timer reads recent cache JPEGs and displays them in the existing UI labels.

The existing UI meanings are preserved:

- `TIC_machine`: start thermal capture.
- `TIC_data`: start periodic thermal display updates.
- `TIC_start`: clear the thermal image cache.
- `TIC_stop`: stop thermal capture, stop display updates, and clear the thermal image cache.
- `TIC_RD`: display the full thermal frame.
- `TIC_PD`: display the cropped thermal frame or ROI.

Mouse ROI behavior is also preserved:

- Left click records ROI points while thermal ROI tracking is active.
- Middle click applies the selected ROI.
- Right click cancels ROI selection and returns to full cropped display.

## Scope

Slice I includes:

- Dynamic loading of the Hikvision thermal camera runtime DLLs.
- Startup diagnostics for missing DLLs and missing dependent runtime files.
- Login to the configured network thermal camera.
- Preview stream start and stop.
- Decode callback path that writes JPEG frames to `logs/pic/cache`.
- Copying decoded JPEG frames to `logs/pic/saved`, matching Python behavior.
- Cache pruning so the cache directory stays bounded.
- A display timer that reads the newest valid cache JPEG.
- Display of full and cropped thermal images in `TIC_RD` and `TIC_PD`.
- ROI selection behavior matching the Python mouse handling.
- Safe shutdown when the window closes.
- Hardware-free tests using fake thermal camera and frame-store interfaces.
- README updates explaining DLL placement, config, and manual hardware checks.

Slice I excludes:

- Any UI redesign.
- Temperature measurement extraction from radiometric data.
- DDS publishing of thermal camera frames.
- Replacement of the JPEG cache pipeline with an in-memory frame pipeline.
- Features exposed by `HCUSBSDK.dll`, because the active Python thermal workflow does not call that SDK.
- Any changes to the unresolved high-speed camera hardware issue.

## Why The JPEG Cache Pipeline Is Used First

The JPEG cache pipeline is not the cleanest long-term design, but it closely matches the Python application.

That is valuable for this migration because:

- The original workflow can be compared frame-by-frame against the C++ workflow.
- Captured JPEG files remain on disk for hardware debugging.
- The display side can be tested using sample JPEG files without the thermal camera connected.
- The first slice avoids changing both the SDK integration and the display architecture at the same time.

The main cost is extra disk I/O: each frame is encoded to JPEG, written to disk, read back, decoded, and displayed. That cost is acceptable for this slice because the Python UI already updates the thermal display once per second.

## Runtime Files And Portability

The C++ application should prefer a portable thermal camera runtime folder beside the executable:

`lib/sensors/thermal_camera/lib/win`

The initial DLL search order will be:

1. Portable folder beside the executable: `lib/sensors/thermal_camera/lib/win`.
2. Optional configured folder from `config/env_config.json`.

The portable folder must contain the Hikvision runtime files required by the original Python workflow, including:

- `HCNetSDK.dll`
- `PlayCtrl.dll`
- `libcrypto-1_1-x64.dll`
- `libssl-1_1-x64.dll`
- `HCNetSDKCom` runtime subfolder
- other dependent DLLs shipped with the vendor SDK and permitted by the license

The C++ program should not fail at application startup if these DLLs are missing. Instead, the thermal camera buttons should show clear messages when the user tries to start thermal capture.

## Configuration

The existing `thermal_camera` config section will remain the source of the network camera login settings:

- `thermal_camera.ip`
- `thermal_camera.port`
- `thermal_camera.user`
- `thermal_camera.password`

Slice I will add deployment/runtime settings to the same section:

- `thermal_camera.sdk_dir`: optional explicit Hikvision SDK runtime folder.
- `thermal_camera.portable_sdk_dir`: relative portable SDK folder, defaulting to `lib/sensors/thermal_camera/lib/win`.
- `thermal_camera.cache_max_files`: maximum number of cache JPEGs to keep, defaulting to `60` to match Python.
- `thermal_camera.display_interval_ms`: display timer interval, defaulting to `1000` to match Python.

These values belong in config because camera deployment paths and display/cache timing are installation parameters, not C++ source-code behavior.

## Components

### `ThermalCameraTypes`

Small shared value types for the thermal camera workflow.

Responsibilities:

- Store camera login settings.
- Store runtime path settings.
- Store display/cache timing settings.
- Format SDK error codes for user-facing diagnostics.

### `IThermalCameraClient`

Interface used by the service to start and stop thermal capture.

Responsibilities:

- Allow hardware-free tests.
- Keep Hikvision SDK details out of the UI service.
- Provide the latest diagnostic message.

### `ThermalCameraClient`

Concrete wrapper around `HCNetSDK.dll` and `PlayCtrl.dll`.

Responsibilities:

- Load required DLLs from the configured runtime folder.
- Set the SDK runtime path and OpenSSL DLL paths.
- Initialize and clean up the SDK.
- Login to the configured thermal camera.
- Start preview with `NET_DVR_RealPlay_V40`.
- Pass stream data into `PlayCtrl.dll`.
- Convert decoded frames to JPEG files in the cache directory.
- Copy cache JPEGs to the saved directory.
- Stop preview, logout, release PlayCtrl port, and clean up safely.
- Keep callback function objects alive for as long as the SDK can call them.

This class should be heavily commented because the SDK callback lifetime rules are easy to get wrong.

### `ThermalFrameStore`

Filesystem helper for the JPEG cache pipeline.

Responsibilities:

- Resolve `logs/pic/cache`, `logs/pic/saved`, and `logs/pic/roi` under the executable directory.
- Create those directories when needed.
- Clear the cache directory.
- Prune old cache JPEGs beyond `cache_max_files`.
- Return the newest readable JPEG for display.
- Save the ROI image when needed to match Python's `logs/pic/roi/uarter.png` behavior.

### `ThermalCameraService`

Qt UI service replacing the current thermal placeholder.

Responsibilities:

- Wire the existing thermal UI buttons to real behavior.
- Start thermal capture when `TIC_machine` is clicked.
- Start the display timer when `TIC_data` is clicked.
- Clear the cache when `TIC_start` is clicked.
- Stop capture and display when `TIC_stop` is clicked.
- Update `TIC_RD` and `TIC_PD` only on the Qt main thread.
- Apply the same square-crop ROI calculation used by Python.
- Provide clear message boxes for missing DLLs, login failure, preview failure, and unreadable cache frames.

### `MainWindow`

The main window keeps the same UI file and button layout.

Responsibilities:

- Construct `ThermalCameraService` with config, app paths, and `ui_`.
- Forward mouse press events to `ThermalCameraService` only for thermal ROI selection.
- Stop thermal capture during close handling.

## Error Handling

Missing runtime folder or DLL:

- Show the searched folder and missing DLL names.
- Keep the application running.

Login failure:

- Show the configured camera IP/port and Hikvision SDK last-error code.
- Do not start the display timer automatically.

Preview failure:

- Stop any partially-started SDK state.
- Show the SDK last-error code.

Display failure:

- If no cache JPEG exists yet, quietly wait for the next timer tick.
- If recent cache JPEGs cannot be decoded, show a concise message and keep the timer alive so the next valid frame can recover.

Stop/close:

- Must be safe when capture was never started.
- Must stop the display timer before clearing cache files.
- Must release SDK resources in reverse order: preview, PlayCtrl stream, camera login, SDK cleanup.

## Redundancy Removed From The Python Version

The C++ slice will not convert these Python details:

- `sys.path` edits used only for Python imports.
- `tkinter` imports that are unused in the active workflow.
- The long `sleep(1000000)` used to keep the Python process alive.
- `HCUSBSDK.dll` loading in unrelated Python modules, because the active thermal workflow uses `HCNetSDK.dll` and `PlayCtrl.dll`.
- Linux SDK loading paths, because the C++ release target is Windows.
- Global variables for process state where a focused C++ object can own the state directly.

## Testing

Hardware-free tests will cover:

- Missing DLL/runtime folder produces a useful message without crashing.
- Starting capture calls the thermal camera client once.
- Starting display creates and starts the timer.
- Clearing cache removes cache JPEGs but keeps the directory available.
- Stopping capture stops the client, timer, and cache path safely.
- Newest readable JPEG is selected when multiple cache files exist.
- Unreadable newest JPEG falls back to another recent valid JPEG.
- ROI calculation matches the Python square-crop behavior.
- `TIC_RD` and `TIC_PD` receive pixmaps when a sample JPEG exists.

Manual hardware verification will cover:

1. Place the Hikvision runtime folder under `build/Release/lib/sensors/thermal_camera/lib/win`.
2. Confirm `config/env_config.json` contains the correct thermal camera IP, port, user, and password.
3. Start `AFP_ABB_CPP.exe`.
4. Click `TIC_machine` and confirm no error message appears.
5. Click `TIC_data` and confirm `TIC_RD` and `TIC_PD` update about once per second.
6. Confirm JPEGs appear in `build/Release/logs/pic/cache` and `build/Release/logs/pic/saved`.
7. Test ROI selection with left, middle, and right mouse clicks.
8. Click `TIC_stop` and confirm capture stops and the cache is cleared.

## README Updates

The README will explain:

- This slice uses the Hikvision network SDK path, not the USB SDK path.
- Where the thermal camera runtime DLLs must be placed for portable deployment.
- Which `thermal_camera` config fields are used.
- How to run the workflow manually with real hardware.
- How to interpret missing-DLL, login, and preview errors.
