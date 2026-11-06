# AFP_ABB_CPP

This is the C++ migration of the AFP ABB Python PC software.

Slice A provides the Qt Widgets application shell, keeps the original UI design, loads runtime configuration, and builds a portable Windows application folder.

Slice B adds direct Siemens S7-1200 PLC controls for the motor, cylinders, laser, target temperature, laser power, and hysteresis ratio. It does not convert the DB18 print-command signaling, ABB robot workflow, cameras, FTP server, serial devices, or pressure sensor yet.

Slice C adds pressure sensor DLL acquisition and publishes raw/processed pressure values through eProsima Fast DDS. It keeps the original UI design and does not add embedded pressure plotting.

Slice D adds MOD path-file upload to the configured ABB controller FTP server. It keeps the original UI design and replaces the old Python `pythonLIb\printpath` start folder with a portable `mod_files` folder beside the executable.

Slice E adds the PLC DB18 print command handshake used by the original Python workflow.

Slice F adds the robot print workflow around the existing UI buttons: MOD upload state is checked, `LOAD_MOD:<filename>#` is sent to the ABB robot socket, and DB18 PLC print commands are used for start, pause, continue, and stop.

Slice G adds PLC-forward ABB robot movement controls for Cartesian move, robot home, joint move, external-axis move, and external-axis reset. It uses the existing UI and implements only the PLC-forward robot movement path; socket-based movement remains unsupported.

Slice H adds the core high-speed visible-light camera workflow using the Hikrobot/MVS SDK: camera enumeration, opening the selected camera, live preview in the existing `HC_RD` label, stop/close behavior, and frame-rate/exposure/gain read-write. It does not convert YOLO flaw prediction, video recording, abrasion/misalignment plotting, or DDS camera-frame publishing.

Slice I adds the Hikvision network thermal camera workflow using the original JPEG cache behavior: thermal preview startup, decoded JPEG cache files, saved frame copies, periodic display in the existing `TIC_RD` and `TIC_PD` labels, ROI cropping, and safe stop/cache cleanup.

Slice J adds an optional pressure DDS archive workflow. When enabled, `AFP_ABB_CPP.exe` can start InfluxDB 3 Core, ensure the configured database exists, and launch `dds_influx_subscriber.exe` to subscribe to pressure DDS samples and write them to InfluxDB.

Slice K adds an optional ABB robot monitoring socket workflow. When enabled by `robot.monitor`, the main app starts a non-blocking monitor client, receives RAPID `ROBOT_MONITOR` frames, and records live robot position/speed samples to CSV plus optional raw socket logs.

Slice L publishes parsed robot monitor samples through eProsima Fast DDS and extends `dds_influx_subscriber.exe` so pressure and robot monitor data can be archived concurrently into the same InfluxDB database.

## What Slice A Contains

- C++17 Qt Widgets executable.
- Original UI copied from `AFP_ABB_Python/src/ui/Interface4600_window.ui`.
- Runtime config copied from `AFP_ABB_Python/config`.
- Placeholder service classes for future hardware slices.
- Portable deployment using Qt `windeployqt`.

## What Slice A Does Not Convert

- Siemens S7-1200 PLC communication.
- ABB IRB4600 robot communication.
- FTP upload/download.
- Serial port control.
- Pressure sensor DLL acquisition.
- High-speed camera SDK calls.
- Thermal camera SDK calls.
- RAPID files.

## Required Build Tools

Install these on the build computer:

- Windows 10 or Windows 11.
- Visual Studio 2022 Build Tools with C++ workload.
- CMake 3.20 or newer.
- Qt 6 for MSVC 2022 64-bit, for example `C:\Qt\6.9.1\msvc2022_64`.
- vcpkg Snap7 package for Siemens S7 communication.
- vcpkg Fast DDS package for pressure and robot monitor data publishing.

The packaged application folder produced by this project does not require these tools on the target computer.

## PLC Dependency

Slice B uses Snap7 for Siemens S7-1200 communication. On the development PC this project expects the vcpkg package:

```powershell
C:\vcpkg\vcpkg.exe install snap7:x64-windows
```

The C++ code calls Snap7 through a small `PlcClient` wrapper. The rest of the application remains normal Qt/C++ code.

## Pressure DDS Dependency

Slice C uses eProsima Fast DDS for pressure sample publishing. On the development PC this project expects:

```powershell
C:\vcpkg\vcpkg.exe install fastdds:x64-windows
```

The pressure sample contract is defined in:

```text
src\pressure\PressureSample.idl
```

The files in `src\pressure\generated` were generated from that IDL with Fast DDS-Gen v4.0.6. On this development machine, Fast DDS-Gen is installed outside vcpkg at `C:\Tools\Fast-DDS-Gen-v4.0.6\scripts\fastddsgen.bat`; it is not on `PATH` by default. Do not edit generated files by hand; edit the IDL and regenerate them from a shell where Java 17 and the MSVC preprocessor are available:

```powershell
$env:JAVA_HOME='C:\Program Files\Microsoft\jdk-17.0.19.10-hotspot'
$env:PATH="$env:JAVA_HOME\bin;$env:PATH"
C:\Tools\Fast-DDS-Gen-v4.0.6\scripts\fastddsgen.bat -replace -flat-output-dir -ppPath 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe' -d src\pressure\generated src\pressure\PressureSample.idl
```

Slice L uses eProsima Fast DDS for robot monitor sample publishing. The robot monitor sample contract is defined in:

```text
src\robot_monitor\RobotMonitorSample.idl
```

The files in `src\robot_monitor\generated` were generated from that IDL with the same Fast DDS-Gen v4.0.6 installation used for the pressure DDS files. Do not edit generated files by hand; edit the IDL and regenerate them with:

```powershell
$env:JAVA_HOME='C:\Program Files\Microsoft\jdk-17.0.19.10-hotspot'
$env:PATH="$env:JAVA_HOME\bin;$env:PATH"
C:\Tools\Fast-DDS-Gen-v4.0.6\scripts\fastddsgen.bat -replace -flat-output-dir -ppPath 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe' -d src\robot_monitor\generated src\robot_monitor\RobotMonitorSample.idl
```

The pressure and robot DDS IDLs use `sample_sequence` because `sequence` is reserved by the Fast DDS-Gen IDL parser. The C++ pressure and robot samples, CSV diagnostic files, and InfluxDB line protocol still expose this value as `sequence`.

## Configure

From this folder:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64;C:\vcpkg\installed\x64-windows"
```

If Qt or vcpkg is installed elsewhere, replace those paths with your local installation paths.

## Build

```powershell
cmake --build build --config Release
```

The executable is created at:

```text
build\Release\AFP_ABB_CPP.exe
```

## Run On The Build Computer

```powershell
.\build\Release\AFP_ABB_CPP.exe
```

The app should open the original AFP ABB UI. Hardware behavior is converted slice by slice, so unconverted buttons still show placeholder messages.
PLC controls from Slice B, pressure controls from Slice C, FTP MOD upload from Slice D, DB18 print command signaling from Slice E, robot print workflow controls from Slice F, PLC-forward robot movement controls from Slice G, high-speed camera controls from Slice H, thermal camera controls from Slice I, optional pressure DDS archiving from Slice J, optional robot monitoring from Slice K, and robot monitor DDS/Influx archiving from Slice L now have real C++ implementations. They still require the matching hardware, runtime DLLs, database runtime, RAPID modules, and network configuration to perform real operations.

## Run Tests

```powershell
ctest --test-dir build -C Release --output-on-failure
```

## Create Portable Windows Folder

Build the deployment target:

```powershell
cmake --build build --target deploy_portable --config Release
```

This runs Qt `windeployqt` and copies the required Qt runtime files into `build\Release`.

Before copying to another computer, confirm these files exist:

```text
build\Release\AFP_ABB_CPP.exe
build\Release\dds_influx_subscriber.exe
build\Release\pressure_probe.exe
build\Release\Qt6Core.dll
build\Release\Qt6Gui.dll
build\Release\Qt6Widgets.dll
build\Release\snap7.dll
build\Release\fastdds-3.4.dll
build\Release\fastcdr-2.3.dll
build\Release\foonathan_memory-0.7.4.dll
build\Release\tinyxml2.dll
build\Release\platforms\qwindows.dll
build\Release\config\env_config.json
build\Release\config\logging_config.json
build\Release\assets\icons\icon.png
build\Release\mod_files
build\Release\lib\sensors\pressure_sensor\pressure.dll
build\Release\lib\sensors\pressure_sensor\BD802NHMC_DAQ.dll
build\Release\lib\sensors\pressure_sensor\BD80xmcsdconfig.ini
build\Release\lib\sensors\pressure_sensor\vk70xmcdllconfig.ini
build\Release\lib\sensors\high_speed_camera\MvCameraControl.dll
build\Release\lib\sensors\thermal_camera\lib\win
```

Copy the entire `build\Release` folder to another compatible Windows computer. Run `AFP_ABB_CPP.exe` directly from that copied folder. Python, CMake, Visual Studio, and Qt do not need to be installed on the target computer.

If `windeployqt` warns that it cannot find a Visual Studio redistributable directory, install the matching Microsoft Visual C++ Redistributable on the target computer or copy the correct MSVC runtime DLLs from the build environment according to Microsoft's redistribution rules.

## Runtime Configuration

The executable reads:

```text
config\env_config.json
```

relative to the executable folder. Edit the copied config file before running on a machine with different PLC, robot, camera, FTP, or serial settings.

Pressure settings are in the same file:

```json
"pressure": {
  "sample_interval_ms": 100,
  "processing_window_samples": 40,
  "dds_domain_id": 0,
  "dds_topic": "afp.pressure.sample"
}
```

`sample_interval_ms=100` matches the Python pressure reader's `time.sleep(0.1)`. `processing_window_samples=40` matches the rolling pressure calculation in `pressure_monitor.py`. The acquisition rate is derived from `sample_interval_ms`; there is no separate Hz setting.

To view pressure curves, subscribe to the DDS topic shown by the `FC_data` button. PlotJuggler eProsima Edition is a good fit for time-series pressure fields; Fast DDS Monitor can also confirm that the publisher and topic exist.

The pressure vendor `.ini` files are expected in the pressure runtime folder:

```text
build\Release\lib\sensors\pressure_sensor\BD80xmcsdconfig.ini
build\Release\lib\sensors\pressure_sensor\vk70xmcdllconfig.ini
```

Root-level copies are not required:

```text
build\Release\BD80xmcsdconfig.ini
build\Release\vk70xmcdllconfig.ini
```

If these root-level files exist from old manual tests, they can be deleted. CMake does not intentionally copy them to the Release root.

## Pressure Sensor Diagnostics

The current source intentionally keeps two pressure diagnostic aids that were added during hardware testing. They are not part of the original Python workflow, but they are useful while validating portable deployments.

### Temporary Raw-Read Logging

Temporary raw-read logging is located in:

```text
src\pressure\PressureAcquisitionWorker.cpp
src\pressure\PressureAcquisitionWorker.h
```

Every line added for this logging is marked with:

```text
TEMP PRESSURE RAW DIAGNOSTIC
```

When `FC_start` starts pressure acquisition, the main application logs the first 80 raw values returned directly by `pressure.dll::catchdata()` before statistics, DDS publishing, or InfluxDB archiving can change them.

The log file is:

```text
build\Release\afp_abb_cpp.log
```

Look for lines like:

```text
[pressure-raw-diagnostic] sequence=0 raw_value=...
```

This is useful when pressure values are zero in the subscriber or InfluxDB. If these raw values are already zero, the issue is before DDS/InfluxDB and usually involves the pressure DLL runtime, device communication, network/firewall permission, config files, or the sensor itself.

After pressure hardware behavior is trusted, this diagnostic can be removed by searching for `TEMP PRESSURE RAW DIAGNOSTIC` and deleting the marked lines. Alternatively, set `kEnableRawReadDiagnostic` to `false` in `PressureAcquisitionWorker.cpp`.

### Standalone Pressure Probe

The build also produces:

```text
build\Release\pressure_probe.exe
```

This is a standalone console diagnostic for the pressure DLL. It does not start the main GUI, DDS subscriber, InfluxDB, PLC, robot, or cameras. It prints:

- application directory and current working directory
- pressure DLL path and existence check
- root-level and nested `vk70xmcdllconfig.ini` `DAQ-1` values
- raw `catchdata()` values using the C++ `PressureSensorClient`/`QLibrary` path
- raw `catchdata()` values using a Python-like WinAPI `LoadLibraryW` path

Run it from the portable Release folder on the hardware computer:

```powershell
.\pressure_probe.exe --samples 80 --interval-ms 100
```

If Python reads nonzero values but `pressure_probe.exe` reads zero values, first check Windows Defender Firewall. The LAN pressure DAQ may require allowing both `AFP_ABB_CPP.exe` and `pressure_probe.exe` on Private networks, and the vendor runtime uses port `8234`.

### Windows Defender Firewall For Pressure Sensor

The pressure DAQ communicates over the LAN. Windows Defender Firewall can allow `python.exe` while blocking a newly copied C++ executable, which makes `pressure.dll::catchdata()` return zero even when the sensor and config files are correct.

On the hardware computer, allow these executables on Private networks:

```text
D:\New Software_C++\Release4.0\Release\AFP_ABB_CPP.exe
D:\New Software_C++\Release4.0\Release\pressure_probe.exe
```

Use the actual copied Release folder path if it is different.

GUI steps:

1. Open **Windows Security**.
2. Select **Firewall & network protection**.
3. Click **Allow an app through firewall**.
4. Click **Change settings**. Administrator permission may be required.
5. Click **Allow another app...**.
6. Click **Browse...**.
7. Select `AFP_ABB_CPP.exe` from the copied Release folder.
8. Click **Add**.
9. In the allowed apps list, find `AFP_ABB_CPP.exe`.
10. Check **Private**.
11. Leave **Public** unchecked unless the production network policy explicitly requires it.
12. Repeat steps 5-11 for `pressure_probe.exe`.
13. Click **OK**.
14. Close any already-running `AFP_ABB_CPP.exe` or `pressure_probe.exe`, then start them again.

PowerShell alternative, run as Administrator:

```powershell
New-NetFirewallRule `
  -DisplayName "AFP ABB CPP Main App" `
  -Direction Inbound `
  -Program "D:\New Software_C++\Release4.0\Release\AFP_ABB_CPP.exe" `
  -Action Allow `
  -Profile Private

New-NetFirewallRule `
  -DisplayName "AFP ABB CPP Pressure Probe" `
  -Direction Inbound `
  -Program "D:\New Software_C++\Release4.0\Release\pressure_probe.exe" `
  -Action Allow `
  -Profile Private
```

If pressure values are still zero, also allow outbound traffic for both executables:

```powershell
New-NetFirewallRule `
  -DisplayName "AFP ABB CPP Main App Outbound" `
  -Direction Outbound `
  -Program "D:\New Software_C++\Release4.0\Release\AFP_ABB_CPP.exe" `
  -Action Allow `
  -Profile Private

New-NetFirewallRule `
  -DisplayName "AFP ABB CPP Pressure Probe Outbound" `
  -Direction Outbound `
  -Program "D:\New Software_C++\Release4.0\Release\pressure_probe.exe" `
  -Action Allow `
  -Profile Private
```

If the local security policy uses port rules instead of application rules, allow the vendor server port `8234` on the Private network profile. After changing firewall settings, run `pressure_probe.exe` first; it should print nonzero values before the full GUI workflow is tested.

## DDS Archive To InfluxDB

Slice J can archive pressure DDS samples into InfluxDB 3 Core. Slice L extends the same subscriber process to archive robot monitor DDS samples. This is optional and controlled by `config\env_config.json`.

The archive path uses two processes:

```text
AFP_ABB_CPP.exe starts and supervises startup
dds_influx_subscriber.exe subscribes to enabled DDS topics and writes InfluxDB line protocol
```

InfluxDB itself is not bundled into the portable AFP folder. The default config expects the user-installed InfluxDB 3 Core executable at:

```text
D:\InfluxDB 3 Core\influxdb3-core-3.9.2-windows_amd64\influxdb3.exe
```

### Create The Admin Token File

Run this once on the InfluxDB computer:

```powershell
cd "D:\InfluxDB 3 Core\influxdb3-core-3.9.2-windows_amd64"
.\influxdb3.exe create token --admin `
  --name afp-admin `
  --offline `
  --output-file "D:\InfluxDB 3 Core\afp-admin-token.json"
```

The file must contain a non-empty `token` field. It may also contain other InfluxDB metadata. Protect this file because it grants administrative access to the local InfluxDB instance.

### Automatic Startup

These config values control automatic startup:

```json
"timeseries_archive": {
  "enabled": true,
  "start_influxdb": true,
  "influxdb_show_console": true,
  "stop_influxdb_on_exit": true,
  "influxdb_exe_path": "D:/InfluxDB 3 Core/influxdb3-core-3.9.2-windows_amd64/influxdb3.exe",
  "host_url": "http://localhost:8181",
  "admin_token_file": "D:/InfluxDB 3 Core/afp-admin-token.json",
  "database": "afp_process",
  "ensure_database_on_start": true,
  "subscriber_autostart": true,
  "subscriber_executable": "dds_influx_subscriber.exe",
  "subscriber_show_console": true,
  "subscriber_print_live_data": true,
  "write_batch_size": 10,
  "write_flush_interval_ms": 1000,
  "reachability_probe_timeout_ms": 500,
  "influxdb_startup_timeout_ms": 30000,
  "cli_timeout_ms": 10000
}
```

`start_influxdb: true` lets `AFP_ABB_CPP.exe` start InfluxDB only when `host_url` is not already reachable. It avoids starting a second server if one is already running. `influxdb_show_console` controls whether the InfluxDB server has its own visible console for server logs/status. `stop_influxdb_on_exit: true` makes the main app clean up the configured `influxdb3.exe` process tree on exit; set it to `false` only when you want to manage a manually started InfluxDB server yourself.

`ensure_database_on_start: true` checks existing databases and creates `afp_process` only if it is missing. It does not overwrite or clear an existing database.

`subscriber_autostart: true` starts `dds_influx_subscriber.exe` automatically. `subscriber_show_console` controls whether a console window is opened for the subscriber. `subscriber_print_live_data` controls whether incoming pressure and robot monitor line-protocol rows are printed in that console.

`write_batch_size: 10` means the subscriber normally writes about once per second when pressure samples arrive every 100 ms. `write_flush_interval_ms` still writes smaller batches when data arrives more slowly.

The archive startup runs asynchronously after the main window is created. The main GUI should appear without waiting for InfluxDB cold start, database checks, or subscriber startup. `influxdb_startup_timeout_ms` should stay long enough for a first cold start; 30 seconds is the default because InfluxDB can need several seconds before database CLI commands are reliable.

### Grafana Startup

Grafana startup is separate from the time-series archive service. `AFP_ABB_CPP.exe` only requests `grafana.exe server`; it does not stop Grafana when the main app exits.

```json
"grafana": {
  "enabled": true,
  "autostart": true,
  "exe_path": "D:/Grafana OSS/grafana-13.0.1+security-01/bin/grafana.exe",
  "working_dir": "D:/Grafana OSS/grafana-13.0.1+security-01",
  "host_url": "http://localhost:3000",
  "show_console": true,
  "startup_timeout_ms": 10000
}
```

`working_dir` should be the Grafana root folder, not the `bin` folder, so Grafana can find its `conf` and `public` folders. `show_console` controls whether the Grafana server has its own visible console. If Grafana is already running, the new server process may report a port-in-use message in that console; use the existing server at `host_url`.

### Manual InfluxDB Startup

For troubleshooting, start InfluxDB manually from the InfluxDB folder:

```powershell
.\influxdb3.exe serve `
  --node-id afp-node `
  --object-store file `
  --data-dir "D:\InfluxDB 3 Core\data" `
  --admin-token-file "D:\InfluxDB 3 Core\afp-admin-token.json"
```

Then confirm the configured host URL is reachable:

```text
http://localhost:8181
```

### Manual Database Creation

The C++ startup service uses the `influxdb3` CLI to check and create the database. It passes the token through the child process environment instead of placing the token on the command line.

Manual equivalent:

```powershell
$env:INFLUXDB3_HOST_URL = "http://localhost:8181"
$env:INFLUXDB3_AUTH_TOKEN = "paste_the_token_value_from_afp-admin-token.json"
.\influxdb3.exe create database --retention-period 30d afp_process
```

The retention period is applied when the database is first created. Changing `retention_period` later does not modify an existing database automatically.

### DDS Archive Topics And Tables

Pressure and robot monitor archive settings are separate from InfluxDB server settings:

```json
"pressure_archive": {
  "enabled": true,
  "dds_domain_id": 0,
  "dds_topic": "afp.pressure.sample",
  "table": "pressure_sensor",
  "source_tag": "pressure_sensor_1"
},
"robot_monitor_archive": {
  "enabled": true,
  "dds_domain_id": 0,
  "dds_topic": "afp.robot.monitor.sample",
  "table": "robot_monitor",
  "source_tag": "abb_irb4600"
}
```

The subscriber starts one DDS reader for each enabled archive section. Pressure rows are written to `pressure_sensor`; robot monitor rows are written to `robot_monitor`. `source_tag` is a stable low-cardinality tag, while sequence numbers, pose, joint, external-axis, and speed values are stored as fields.

### Manual Archive Smoke Test

1. Confirm `D:\InfluxDB 3 Core\afp-admin-token.json` exists and contains a `token` field.
2. Confirm `build\Release\dds_influx_subscriber.exe` exists beside `AFP_ABB_CPP.exe`.
3. Start `build\Release\AFP_ABB_CPP.exe`.
4. If `subscriber_show_console` is true, confirm the subscriber console appears.
5. Click `FC_machine`, then `FC_start` to begin pressure acquisition.
6. Start the RAPID TASK2 robot monitor program if robot monitor archive validation is needed.
7. If `subscriber_print_live_data` is true, confirm live `pressure_sensor,...` and/or `robot_monitor,...` line-protocol rows appear in the subscriber console.
8. Query InfluxDB or use InfluxDB Explorer to confirm rows appear in `afp_process.pressure_sensor` and `afp_process.robot_monitor`.

## FTP MOD File Upload

The `locusselectsetButton` control uploads one selected `.mod` path file to the FTP server configured in `config\env_config.json`.

FTP settings use the existing config keys:

```json
"ftp": {
  "ip": "192.168.0.104",
  "port": 21,
  "user": "Default User",
  "password": "robotics"
}
```

The file picker starts in the portable `mod_files` folder beside `AFP_ABB_CPP.exe`. This replaces the old Python-only `pythonLIb\printpath` start folder. The folder is only a convenience location; operators can browse to any valid `.mod` file.

Manual FTP smoke test:

1. Confirm the FTP server IP, port, user, and password in `config\env_config.json`.
2. Put a `.mod` file in `mod_files` or browse to another `.mod` file.
3. Start `AFP_ABB_CPP.exe`.
4. Click the existing MOD path selection button.
5. Select the `.mod` file.
6. Confirm the upload success message.
7. Confirm the remote FTP directory contains the selected filename.

## PLC DB18 Print Command Channel

Slice E adds the DB18 print command handshake used by the original Python workflow. Slice F wires the existing print buttons to this command channel together with FTP upload state and ABB robot socket filename sending.

DB18 layout:

```text
DB18.DBB0 = command
DB18.DBB1 = ACK
DB18.DBB2 = state
DB18.DBB3 = error
```

Command values:

```text
START  = 10
PAUSE  = 11
RESUME = 12
STOP   = 13
```

The command flow is:

```text
PC writes command to DB18.DBB0
PC waits up to 3 seconds for DB18.DBB1 == 1
PC clears DB18.DBB0 back to 0 after ACK
```

## Robot Print Workflow

Slice F wires the existing print buttons without changing the UI design.

Before starting print:

1. Upload a `.mod` file with `locusselectsetButton`.
2. Connect the robot socket with `ConnectionRobotButton`.
3. Click `Robot_movebutton_Gcode`.

The C++ workflow preserves the Python order:

```text
check FTP upload success
check uploaded MOD filename
send LOAD_MOD:<filename># to robot.ip:robot.port_motion
write DB18 START and wait for ACK
clear DB18 command after ACK
```

Pause, continue, and stop use the DB18 PLC command manager first. After a successful PLC ACK, the app also attempts the matching socket side message:

```text
PAUSE#
CONTINUE#
STOP#
```

The side message is non-fatal for pause, continue, and stop because the Python workflow already treats the PLC handshake as the main control path.

Robot socket settings are read from `config\env_config.json`:

```json
"robot": {
  "ip": "192.168.0.104",
  "port_motion": 5010,
  "transport": {
    "type": "plc"
  }
}
```

Slice F uses `robot.ip` and `robot.port_motion` for the print socket. Robot monitoring uses the separate `robot.monitor.port` setting described below.

This slice does not convert socket-based robot movement or live robot plotting.

## PLC-Forward Robot Movement

Slice G wires the existing robot movement buttons without changing the UI design:

```text
Robot_movebutton          Cartesian move
RobotGohomeButton         Cartesian home/reset move
RobotaxismoveButton       Joint move
PositionermoveButton      External-axis move
PositionercomebackButton  External-axis reset
```

The movement path follows `AFP_ABB_Python/src/drivers/robots/abb_plc.py`. Commands are written to PLC process output bytes, and ACK/DONE state is read from PLC process input bytes.

Robot movement speed defaults are read from `config\env_config.json`:

```json
"default_speed": {
  "tcp_linear": 200.0,
  "tcp_orientation": 200.0,
  "external_linear": 200.0,
  "external_orientation": 100.0
}
```

These values match the Python defaults. Empty UI speed fields use these config values; filled UI speed fields override them for the command. Non-empty UI speed overrides must be positive numeric values; invalid overrides are rejected before motion.

Slice G does not convert socket-based robot movement or live pose readback. Slice K adds the first RAPID-backed robot monitoring proof below.

## Robot Monitoring Workflow

Slice K adds a separate monitor client that starts from config only. It is independent of `ConnectionRobotButton`, so the existing robot print connection behavior is unchanged.

Robot monitoring settings are nested under the existing `robot` config object:

```json
"robot": {
  "ip": "192.168.0.104",
  "port_motion": 5010,
  "monitor": {
    "enabled": true,
    "autostart": true,
    "port": 5012,
    "sample_interval_ms": 100,
    "dds_publish_enabled": true,
    "dds_domain_id": 0,
    "dds_topic": "afp.robot.monitor.sample",
    "record_csv": false,
    "record_raw_messages": false,
    "recording_dir": "logs/robot_monitor",
    "flush_every_sample": true
  }
}
```

`robot.ip` is shared by the motion/print socket and the monitor socket. `robot.monitor.port` is the TASK2 monitor socket port. The old `robot.port_server` setting is no longer used.

`robot.monitor.dds_publish_enabled` controls whether the main app publishes parsed monitor samples to Fast DDS. `robot_monitor_archive.enabled` controls whether `dds_influx_subscriber.exe` subscribes to that topic and writes rows to InfluxDB. The DDS topic and domain values must match between these two sections.

`sample_interval_ms` documents the intended monitor interval for the PC-side workflow. The actual first-stage stream is generated by RAPID in `rapid\RAPID\TASK2\PROGMOD\UPLOAD_ROBOT.mod` using `monitor_wait_time:=0.1`, so keep both values synchronized during hardware validation.

The first RAPID monitor frame is compact to keep the controller socket string simple:

```text
#ROBOT_MONITOR s=7 x=10.5 y=20.5 z=30.5 v=80#
```

The parser also accepts the fuller future form with `seq`, quaternion, joints, external axes, and speed fields. Missing orientation/joint values are recorded as zero in the CSV until RAPID is expanded to send them.

DDS/InfluxDB is now the default robot monitor archive path. CSV and raw socket logs are diagnostic-only and disabled by default. When `record_csv` is true, parsed samples are written under:

```text
build\Release\logs\robot_monitor\robot_monitor_YYYYMMDD_HHMMSS.csv
```

When `record_raw_messages` is true, exact socket frames are also written under:

```text
build\Release\logs\robot_monitor\robot_monitor_YYYYMMDD_HHMMSS.raw.log
```

The raw log is useful for checking whether a bad value came from RAPID formatting or C++ parsing.

Manual robot monitor DDS/InfluxDB validation:

1. Confirm InfluxDB is running or `timeseries_archive.start_influxdb` is true.
2. Confirm `timeseries_archive.subscriber_autostart` is true.
3. Confirm `robot.monitor.dds_topic` matches `robot_monitor_archive.dds_topic`.
4. Start `AFP_ABB_CPP.exe`.
5. Start the RAPID TASK2 robot monitor program.
6. If live printing is enabled, confirm the subscriber console prints `robot_monitor,...` rows.
7. Query InfluxDB table `robot_monitor`.
8. Start pressure acquisition and confirm pressure rows still appear in `pressure_sensor`.

Robot monitor status messages from `RobotMonitoringService::statusChanged` are appended to the tooltip of the existing `robot_state_show` GUI field and written to:

```text
build\Release\afp_abb_cpp.log
```

If `AFP_ABB_CPP.exe` starts before the RAPID monitor program is running, the first socket attempt may report a timeout in the tooltip. The service keeps retrying in the background and should connect after RAPID starts.

After a TCP connection is established, the tooltip should also show `Robot monitor first frame received.` once data starts arriving. If it instead shows `Robot monitor connected, but no monitor data received yet.`, the TCP connection exists but RAPID has not sent monitor frames to this client.

While connected, the C++ monitor client sends a lightweight `#ROBOT_MONITOR_CLIENT_ALIVE#` heartbeat. This is not a robot command. The edited TASK2 RAPID code polls for it at a low rate and closes/reaccepts the client socket if the heartbeat stops. This handles the hardware case where `AFP_ABB_CPP.exe` is closed and restarted while the RAPID program keeps running.

### RAPID Files

The C++ repository includes a copied RAPID backup in:

```text
rapid
```

The original Python RAPID backup remains untouched. Slice K changes are made only under `AFP_ABB_CPP\rapid`.

The current monitor stream was added in:

```text
rapid\RAPID\TASK2\PROGMOD\UPLOAD_ROBOT.mod
```

TASK2 also re-enters `SocketAccept` after the PC client disconnects, a monitor send fails, or the C++ heartbeat stops. This allows `AFP_ABB_CPP.exe` to be closed and started again without restarting the robot RAPID program. Heartbeat receive polling happens after the current monitor frame is sent, and only every 0.5 s by default, so it does not block the current sample from being transmitted.

If RobotStudio reports TASK2 error `41595` / `ERR_SOCK_CLOSED` at `UPLOAD_ROBOT/SendRobotMonitorMessage/SocketSend`, it means the PC-side TCP client closed while RAPID was sending a monitor frame. The edited `SendRobotMonitorMessage()` error handler treats this as recoverable, calls `ReconnectUploadClient()`, and uses `TRYNEXT` so TASK2 skips only that failed sample instead of stopping.

TASK1 and TASK3 were intentionally left unchanged in this first monitor proof. If the real controller rejects direct `CRobT()` reads from TASK2, the documented fallback is to add a TASK1 snapshot later and let TASK2 stream that snapshot.

### Manual Robot Monitor Check

With the ABB controller reachable on the robot network:

1. Confirm Windows Firewall allows `AFP_ABB_CPP.exe` on the robot Private network.
2. Confirm `config\env_config.json` has the correct `robot.ip` and `robot.monitor.port`.
3. Load/start the edited copied RAPID modules on the controller.
4. Start `AFP_ABB_CPP.exe`.
5. Confirm the GUI opens without waiting for robot monitoring.
6. Confirm `ConnectionRobotButton` behavior is unchanged.
7. Confirm CSV and raw log files appear under `build\Release\logs\robot_monitor`.
8. Move the robot slowly and compare CSV `x`, `y`, `z` values against the teach pendant.
9. Confirm `estimated_tcp_speed` is near zero while stationary and positive while moving.
10. Close and restart `AFP_ABB_CPP.exe` while RAPID keeps running; confirm new CSV/raw files receive data.
11. Start `AFP_ABB_CPP.exe` before RAPID; confirm it retries and connects after RAPID starts.
12. If restart-without-RAPID-restart still produces empty CSV/raw files, check the RAPID teach pendant for `UPLOAD: Monitor heartbeat timeout. Waiting for reconnection ...`.

## High-Speed Camera Runtime

Slice H loads the Hikrobot/MVS SDK DLL dynamically. The default search order is:

1. `lib\sensors\high_speed_camera\MvCameraControl.dll` beside the executable.
2. `high_speed_camera.sdk_dll_path` from `config\env_config.json`, when set.
3. `C:\Program Files (x86)\Common Files\MVS\Runtime\Win64_x64\MvCameraControl.dll`.

For a portable release, place the vendor runtime files allowed by the Hikrobot/MVS license under:

```text
build\Release\lib\sensors\high_speed_camera\
```

The committed `MvCameraControl.dll` in `lib\sensors\high_speed_camera` is copied there automatically when `AFP_ABB_CPP` is built. If Windows reports that this DLL cannot be loaded because a module is missing, add the matching dependent Hikrobot/MVS runtime DLLs allowed by the vendor license to the same folder.

If the DLL is absent, the application still starts. The high-speed camera buttons show a clear missing-DLL message when used.

## Thermal Camera Workflow

Slice I converts the Hikvision network thermal camera path used by the Python software. It keeps the same visible UI behavior and the same JPEG cache pipeline:

1. `TIC_machine` starts the Hikvision SDK preview stream.
2. Decoded frames are written to `logs\pic\cache`.
3. The same frame files are copied to `logs\pic\saved`.
4. `TIC_data` displays the newest readable cache JPEG in `TIC_RD` and the square crop or ROI in `TIC_PD`.
5. `TIC_start` clears the cache.
6. `TIC_stop` stops capture, stops display updates, and clears the cache.

Portable thermal camera runtime files should be placed in:

```text
lib\sensors\thermal_camera\lib\win
```

When the application is built, this folder is copied to:

```text
build\Release\lib\sensors\thermal_camera\lib\win
```

At minimum, the folder must contain the matching Hikvision runtime files used by the original Python workflow, including:

```text
HCNetSDK.dll
PlayCtrl.dll
libcrypto-1_1-x64.dll
libssl-1_1-x64.dll
HCNetSDKCom
```

Also include any other dependent Hikvision DLLs required by that SDK version and allowed by the vendor license. The application still starts if these files are missing; the thermal camera button will report the missing runtime when used.

Thermal camera settings are read from `config\env_config.json`:

```json
"thermal_camera": {
  "ip": "192.168.0.64",
  "port": 8000,
  "user": "admin",
  "password": "abcd1234",
  "sdk_dir": "",
  "portable_sdk_dir": "lib/sensors/thermal_camera/lib/win",
  "cache_max_files": 60,
  "display_interval_ms": 1000
}
```

`sdk_dir` can point to an installed Hikvision SDK runtime folder. Leave it empty for portable deployment and place the runtime files in `portable_sdk_dir`.

## Manual Thermal Camera Check

With a Hikvision-compatible thermal camera and runtime available:

1. Confirm `build\Release\lib\sensors\thermal_camera\lib\win` contains the Hikvision runtime files.
2. Edit `build\Release\config\env_config.json` so `thermal_camera.ip`, `thermal_camera.port`, `thermal_camera.user`, and `thermal_camera.password` match the camera.
3. Start `AFP_ABB_CPP.exe`.
4. Click `TIC_machine` and confirm no error message appears.
5. Click `TIC_data` and confirm `TIC_RD` and `TIC_PD` update about once per second.
6. Confirm JPEGs appear in `build\Release\logs\pic\cache` and `build\Release\logs\pic\saved`.
7. Left-click two ROI points on `TIC_RD`, then middle-click on `TIC_RD` to apply the ROI.
8. Confirm `TIC_PD` changes to the selected crop and `build\Release\logs\pic\roi\uarter.png` appears.
9. Right-click on `TIC_RD` to cancel ROI selection.
10. Click `TIC_stop` and confirm capture stops and the cache is cleared.

## Manual High-Speed Camera Check

With a Hikrobot/MVS-compatible camera and runtime available:

1. Start `AFP_ABB_CPP.exe`.
2. Click `enum_cam` and confirm the camera appears in `camera_list`.
3. Click `HC_machine` and confirm the success message appears.
4. Click `get_parameter` and confirm `FrameRate`, `ExposureTime`, and `Gain` are populated.
5. Click `HC_start` and confirm live preview appears in `HC_RD`.
6. Click `HC_stop` and confirm preview and parameter fields are cleared.

## Future Conversion Slices

Recommended next slices:

1. Remaining socket-based ABB robot movement and RAPID-related robot workflow.
2. Remaining high-speed camera work: YOLO flaw prediction, video recording, plotting, and optional DDS frame publishing.
3. Remaining serial-device workflow.
