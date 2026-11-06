# AFP ABB C++ Slice A Design

## Purpose

Create the first controlled C++ migration slice for the AFP ABB PC software. This slice establishes a Windows C++/Qt application foundation in a new sibling folder named `AFP_ABB_CPP`, while preserving the original Python UI design and avoiding any new user-facing features.

The first slice must produce a buildable Qt Widgets executable shell, not a full hardware-control conversion. PLC, robot, FTP, serial, pressure, visible-light camera, and thermal-camera behavior will be migrated in later slices.

## Scope

Slice A includes:

- A new CMake-based C++ project under `AFP_ABB_CPP`.
- A Qt Widgets executable that uses the original `AFP_ABB_Python/src/ui/Interface4600_window.ui` design.
- Startup behavior equivalent to `AFP_ABB_Python/src/main.py`.
- Main-window initialization equivalent to the safe, non-hardware parts of `AFP_ABB_Python/src/ui/mywindow_class.py`.
- Loading `config/env_config.json` into typed C++ configuration structs.
- Runtime path handling for copied portable application folders.
- Placeholder C++ services for later PLC, robot, FTP, serial, pressure, visible-light camera, and thermal-camera migration.
- A Markdown README explaining how to build, run, and package the C++ software on Windows.

Slice A excludes:

- Any real PLC read/write behavior.
- ABB robot socket communication or PLC-forwarded robot movement.
- FTP upload/download.
- Serial port scanning and serial commands.
- Pressure sensor DLL acquisition and plotting.
- High-speed visible-light camera SDK calls.
- Thermal camera SDK calls.
- RAPID file conversion.
- New application features not present in the Python software.

## Required Portability

The C++ output must be portable as a copied Windows application folder. A user should be able to copy the packaged folder to another compatible Windows computer and run the `.exe` directly without installing this project, Python, CMake, Visual Studio, or Qt.

The portable folder must contain:

- The compiled `.exe`.
- Required Qt runtime DLLs.
- Required Qt platform plugins, especially `platforms/qwindows.dll`.
- Runtime assets such as `config/env_config.json`, `config/logging_config.json`, the original UI file if used at runtime, and `src/ui/resources/icons/icon.png`.
- Future hardware vendor DLLs only when their subsystem slice is implemented.

The README must explain how to create this folder using Qt's `windeployqt` or an equivalent scripted deployment step. Slice A should not rely on Python or Python virtual environments at runtime.

## Architecture

Use Qt Widgets and CMake. CMake should enable Qt's automatic UIC/MOC/RCC support where practical, compile the original `.ui` into C++ code, and build a native Windows GUI executable.

The app should be simple and lightweight:

- Use standard C++ and Qt classes before introducing third-party libraries.
- Use a small JSON reader suitable for Qt, preferably `QJsonDocument`, for `env_config.json`.
- Keep service classes thin and intentionally incomplete until their hardware slice is scheduled.
- Prefer direct, named Qt slots over generic dispatch logic.

The first slice proves structure, startup, UI preservation, configuration loading, asset handling, and packaging. It does not pretend hardware behavior has been migrated.

## Components

### `MainWindow`

`MainWindow` owns the generated UI object, sets the same window title and icon as the Python app, centers the window, initializes non-hardware state, and connects UI buttons to C++ slots.

It should initialize the same safe defaults visible in `mywindow_class.py`, including:

- Current top-level tab index.
- Default robot speed values.
- Default external-axis speed values.
- Default process values such as paving distance, wire-feed rate, and hysteresis ratio.
- Boolean state flags for robot link, pause, stop, PLC connection, motor enable, laser state, red light, target temperature state, temperature control, and power control.
- Hidden and disabled PLC connect button, matching the Python code.
- Word-wrapped robot status label when available.

Hardware buttons should call placeholder slots that show a clear status message such as "Not converted in Slice A" without performing hardware I/O.

### `AppConfig`

`AppConfig` loads `config/env_config.json` and exposes typed structs for:

- Robot connection settings.
- FTP settings.
- PLC settings.
- Thermal camera settings.
- Serial settings.

Missing or malformed fields should fall back to safe defaults and produce a startup warning. The application should still open when config loading fails.

### `AppPaths`

`AppPaths` resolves project/runtime paths relative to the executable location. This allows the packaged folder to be copied to another Windows computer.

It should locate:

- `config/env_config.json`.
- `config/logging_config.json`.
- `src/ui/resources/icons/icon.png`.
- Future `lib/sensors` DLL folders when later slices add hardware support.

### `StatusLogger`

`StatusLogger` provides lightweight startup and diagnostic messages. It may use Qt logging functions and optionally write to a simple local log file in a `logs` folder beside the executable.

It should avoid recreating Python's `colorlog` behavior in this slice.

### Placeholder Services

Create thin C++ classes with names and responsibilities aligned to the Python service boundaries:

- `SerialService`
- `RobotService`
- `FtpService`
- `PLCService`
- `PressureService`
- `HighSpeedCameraService`
- `ThermalCameraService`

These classes should not communicate with real hardware in Slice A. They exist to define clean C++ ownership boundaries for later conversions and to keep `MainWindow` from becoming a single large file.

## Data Flow

Startup flow:

1. `main.cpp` enables high-DPI behavior, creates `QApplication`, creates `MainWindow`, shows it, and starts the Qt event loop.
2. `MainWindow` constructs the UI from the unchanged `Interface4600_window.ui`.
3. `AppPaths` resolves runtime paths relative to the executable.
4. `AppConfig` loads `config/env_config.json`.
5. `MainWindow` initializes safe default state and status labels.
6. Button clicks call placeholder service slots.
7. Placeholder slots update status text or show a message box, but do not perform hardware I/O.

## Error Handling

Missing config:

- Launch the app with defaults.
- Show or log a startup warning.

Malformed config:

- Launch the app with defaults for malformed sections.
- Log the parse error.

Missing icon:

- Launch without the custom icon.
- Log a warning.

Missing UI at build time:

- Fail the CMake build clearly, because the UI is part of the source design.

Unsupported hardware action in Slice A:

- Show a clear "not converted in Slice A" message.
- Do not attempt partial communication.

## UI Preservation

The original UI design must not be redesigned. The C++ app should use `AFP_ABB_Python/src/ui/Interface4600_window.ui` as the source of truth.

Do not hand-code a replacement layout unless a later technical limitation makes it unavoidable. If the `.ui` file needs to be copied into `AFP_ABB_CPP`, it should be copied without design changes.

Do not convert `src/ui/Interface4600_window.py`, because it is generated Python output from the `.ui` file.

## Redundancy Decisions

Do not convert in Slice A:

- `rapid/`, because it contains ABB RAPID files outside the Python PC software conversion scope.
- `__pycache__/` and `.pyc` files.
- Existing logs.
- Generated `src/ui/Interface4600_window.py`.
- Python-only deployment dependencies such as Torch, Dash, Flask, Jupyter, PyInstaller, Matplotlib, and other packages not needed by the C++ shell.
- `src/drivers/sensors/pressure_reader.py`, because the later pressure slice should call `pressure.dll` directly from C++ instead of spawning Python.
- `src/drivers/sensors/ForceDataObtained.py`, because its global acquisition lists and infinite loop should be replaced later by a small C++ pressure acquisition class.
- Old or demo vendor scripts under `lib/sensors` unless a later hardware slice proves they are required.
- Duplicate quaternion utilities until the robot motion slice, where only the actually used math should be ported.

## Testing

Slice A is complete when:

- CMake configures successfully on Windows.
- The Qt Widgets executable builds.
- The executable opens the original UI.
- The window title and icon match the Python application where assets are available.
- `env_config.json` loads successfully from the runtime folder.
- Missing or malformed config does not prevent startup.
- Placeholder button handlers do not crash.
- Runtime assets are copied beside the executable.
- A portable folder created with `windeployqt` or an equivalent deployment step runs without Python, CMake, Visual Studio, or a Qt installation.
- README instructions accurately describe build, run, and portable packaging.

## Implementation Order

1. Create the `AFP_ABB_CPP` CMake/Qt project.
2. Copy or reference the original `.ui` and icon assets without design changes.
3. Implement `main.cpp`, `MainWindow`, `AppPaths`, and `AppConfig`.
4. Add placeholder service classes and connect safe UI slots.
5. Add asset-copy and deployment-related CMake commands.
6. Write `README.md`.
7. Build and verify the executable.
8. Package a portable Windows folder and verify it runs directly.

