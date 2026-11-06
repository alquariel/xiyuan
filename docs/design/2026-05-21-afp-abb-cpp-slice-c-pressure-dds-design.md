# AFP ABB C++ Slice C Design: Pressure Sensor DDS Publishing

Date: 2026-05-21

Branch: `slice-c-pressure-dds`

## Goal

Convert the pressure sensor control and acquisition path from the Python application into the C++ application, but do not mirror the Python embedded plotting path.

Instead, the C++ application will acquire pressure data, compute the same processed values used by the Python code, and publish raw and processed samples in real time through eProsima Fast DDS. External programs can then subscribe to the pressure topic for storage, analysis, display, or process monitoring.

The main UI design remains unchanged. Existing pressure display widgets stay in the `.ui` file, but Slice C will not draw pressure curves in the main window.

The Python source remains read-only. All new code, documentation, generated IDL, runtime files, and build changes belong in `AFP_ABB_CPP`.

## Source Behavior To Preserve

Slice C preserves the pressure behavior from these Python files:

- `AFP_ABB_Python/src/drivers/sensors/pressure_reader.py`
- `AFP_ABB_Python/src/services/sensor_service/pressure_monitor.py`
- `AFP_ABB_Python/src/core/subprocess/pressure_subprocess.py`

The Python behavior to preserve is:

- Load the pressure sensor DLL.
- Start the pressure sensor with `workstart()`.
- Read pressure data with `catchdata()`, interpreted as a `double`.
- Sample approximately every 100 ms, matching `time.sleep(0.1)` in the Python pressure reader.
- Stop the sensor with `workstop()`.
- Compute processed values from the most recent 40 samples, matching the current Python pressure monitor:
  - rolling mean
  - variance
  - standard deviation
  - peak value

The C++ implementation will not preserve Python's extra subprocess boundary. That boundary exists because Python runs a separate pressure reader script and forwards stdout lines back to Qt. In C++, the service can load the DLL directly and publish structured samples directly.

## Updated Slice C Scope

Slice C includes:

- A pressure sensor DLL wrapper for `pressure.dll`.
- A pressure acquisition worker that reads the sensor at the configured interval.
- A small rolling-statistics component that computes the Python-equivalent processed values.
- A Fast DDS publisher for raw and processed pressure samples.
- UI control wiring for the existing pressure buttons:
  - `FC_machine`
  - `FC_start`
  - `FC_stop`
  - `FC_data`
- README updates for pressure DLL runtime files, Fast DDS dependencies, and external visualization.
- Automated tests for pressure statistics and service state that do not require real pressure hardware.

Slice C excludes:

- Embedded plotting inside the main C++ window.
- Any `.ui` design changes.
- Any new pressure processing algorithm beyond the Python-equivalent rolling values.
- Hardcoded operational timing/window values that users may reasonably need to tune.
- Any storage/database layer.
- Any subscriber application.
- Automatic dependence on a specific external visualizer executable path.
- Robot, PLC, camera, or AFP process-control changes unrelated to pressure acquisition.

## Visualization Decision

The application will publish DDS data and leave visualization to external DDS tools or user-written subscribers.

Official eProsima documentation describes:

- Fast DDS as a DDS publish-subscribe middleware for typed data exchange.
- Fast DDS-Gen as the IDL-to-C++ code generator.
- Fast DDS Monitor as a graphical DDS environment and communication monitor.
- PlotJuggler eProsima Edition as the eProsima Fast DDS Visualizer Plugin for time-series DDS data.

For pressure curves, PlotJuggler eProsima Edition is the better visualization target because pressure data is time-series numeric data. Fast DDS Monitor may still be useful to confirm that the publisher, topic, and data writer exist and that communication is active.

The C++ application will not embed either tool. The `FC_data` button will show the active DDS topic/type details and a short status message so the user knows what to subscribe to.

References:

- Fast DDS documentation: https://fast-dds.docs.eprosima.com/
- Fast DDS-Gen documentation: https://fast-dds.docs.eprosima.com/en/latest/fastddsgen/
- Fast DDS Monitor documentation: https://fast-dds-monitor.readthedocs.io/
- PlotJuggler eProsima Edition documentation: https://plotjuggler-fastdds-plugins.readthedocs.io/

## Components

### `PressureSensorClient`

`PressureSensorClient` is a low-level wrapper around the pressure vendor DLL.

Responsibilities:

- Load `pressure.dll` from the application runtime folder.
- Resolve the required functions:
  - `workstart`
  - `catchdata`
  - `workstop`
- Start the sensor.
- Read one pressure value as `double`.
- Stop the sensor.
- Report structured errors instead of printing to stdout.

This class will not know about Qt widgets, DDS, or plotting.

### `PressureStatistics`

`PressureStatistics` is a small pure C++ helper that keeps the latest pressure samples and computes rolling values.

Responsibilities:

- Accept each raw pressure sample.
- Keep the most recent configured number of samples.
- Report whether the processing window is ready.
- Compute:
  - mean
  - variance
  - standard deviation
  - peak

This class will be unit-tested without pressure hardware or Fast DDS.

### Pressure Configuration

The source Python code contains operational numeric values directly in source files:

- `pressure_reader.py` sleeps for `0.1` seconds between samples, which implies an intended sampling interval of about 100 ms.
- `pressure_monitor.py` processes `self.y_list[-40:]` and divides by `40`, which makes 40 samples the effective processing window.
- `ForceDataPlot.py` refreshes embedded plots every 1000 ms; Slice C does not use the embedded plot path.

In C++, these pressure parameters will be read from `config/env_config.json` with Python-compatible defaults:

```json
{
  "pressure": {
    "sample_interval_ms": 100,
    "processing_window_samples": 40,
    "dds_domain_id": 0,
    "dds_topic": "afp.pressure.sample"
  }
}
```

The C++ application will not store a separate acquisition-rate field such as `10 Hz` because that value is derived from `sample_interval_ms`. Keeping only the interval avoids conflicting configuration values.

Configuration validation:

- `sample_interval_ms` must be greater than 0.
- `processing_window_samples` must be greater than 0.
- `dds_domain_id` must be accepted by Fast DDS as a valid domain ID.
- `dds_topic` must be non-empty.

If a pressure configuration value is missing or invalid, the application will fall back to the Python-compatible default and report the fallback through the status log or user-visible message where appropriate.

### `PressureSample.idl`

The DDS data contract will be defined in an IDL file so subscribers can generate matching code.

Planned topic type:

```idl
struct AfpPressureSample
{
    unsigned long long timestamp_ms;
    unsigned long long sequence;
    double raw_value;
    double mean_value;
    double variance_value;
    double standard_deviation_value;
    double peak_value;
    boolean window_ready;
    boolean sensor_running;
};
```

Field meanings:

- `timestamp_ms`: local application timestamp in milliseconds since Unix epoch.
- `sequence`: monotonically increasing sample number from this publisher.
- `raw_value`: latest raw pressure reading from `catchdata()`.
- `mean_value`: rolling mean over the configured processing window when ready.
- `variance_value`: rolling variance over the configured processing window when ready.
- `standard_deviation_value`: rolling standard deviation over the configured processing window when ready.
- `peak_value`: maximum value in the configured processing window when ready.
- `window_ready`: `false` until enough samples are available for the configured processing window.
- `sensor_running`: `true` while the C++ service considers acquisition active.

One DDS topic will be used:

- Topic name: read from config, default `afp.pressure.sample`
- Type name: `AfpPressureSample`

A single topic keeps subscribers simple and lets visualizer tools choose individual numeric fields to plot.

### `PressureDdsPublisher`

`PressureDdsPublisher` owns the Fast DDS participant, publisher, topic, data writer, and generated type support.

Responsibilities:

- Initialize the DDS publisher.
- Register the generated `AfpPressureSample` type.
- Publish each sample from the acquisition worker.
- Track whether at least one subscriber has matched, if Fast DDS exposes that through listener callbacks.
- Return errors in a way the Qt service can show to the user.

Default DDS behavior:

- Domain ID: read from config, default `0`
- Reliability: reliable
- History: keep last samples
- Topic name: read from config, default `afp.pressure.sample`
- Publication rate: driven by the configured sensor acquisition interval, default approximately 10 Hz

Reliable delivery is preferred for this slice because pressure data may be used by storage or analysis subscribers. If later testing shows visual display latency or blocking risk, this can be revisited in a later slice by adding XML QoS profiles or a configuration setting.

### `PressureAcquisitionWorker`

`PressureAcquisitionWorker` runs sensor acquisition outside the UI thread.

Responsibilities:

- Start `PressureSensorClient`.
- Read one raw sample at the configured sampling interval.
- Update `PressureStatistics`.
- Build an `AfpPressureSample`.
- Publish the sample through `PressureDdsPublisher`.
- Stop cleanly when requested.
- Always call `workstop()` if `workstart()` succeeded.

The worker will use Qt threading primitives because the rest of the application is already Qt-based.

### `PressureService`

`PressureService` becomes the Qt-facing pressure service.

Responsibilities:

- Own the sensor client, DDS publisher, and acquisition worker.
- Wire existing UI buttons to pressure actions.
- Show user-visible errors through the existing message-signal pattern.
- Keep the UI responsive while acquisition is running.
- Avoid modifying the main UI design.

Button behavior:

| Button | C++ behavior |
| --- | --- |
| `FC_machine` | Initialize/check pressure sensor DLL and DDS publisher. |
| `FC_start` | Start pressure acquisition and DDS publishing. |
| `FC_stop` | Stop acquisition and call the sensor stop path. |
| `FC_data` | Show DDS topic/type/status information for external subscribers and visualizers. |

The pressure plot labels/widgets such as `FC_RD`, `FC_VV`, `FC_VAR`, `FC_SD`, and `FC_PEAK` remain in the UI untouched. Slice C will not write plotted data to them.

## Runtime Files

The Python pressure runtime folder contains:

- `pressure.dll`
- `BD802NHMC_DAQ.dll`
- `BD80xmcsdconfig.ini`
- `vk70xmcdllconfig.ini`
- optional development files such as `.lib`, `.exp`, and `.pdb`

The C++ portable application needs only the runtime files required by the sensor DLL, not development artifacts.

Planned C++ runtime folder:

- `AFP_ABB_CPP/lib/sensors/pressure_sensor/`

The portable build output will copy the required runtime files beside the executable or under the same relative folder used by the application. The implementation plan will verify the correct DLL search behavior on Windows before choosing the final copy location.

## Dependency Plan

Fast DDS is installed through vcpkg for the active toolchain.

Expected dependency path:

- vcpkg package: `fastdds:x64-windows`
- generated type code from `PressureSample.idl`
- CMake links to Fast DDS and Fast CDR targets exposed by vcpkg

Task 1 found that the vcpkg `fastdds` package does not include Fast DDS-Gen on this machine. The generator was therefore built separately from the official eProsima Fast DDS-Gen source tag `v4.0.6`:

```text
C:\Tools\Fast-DDS-Gen-v4.0.6\scripts\fastddsgen.bat
```

OpenJDK 17 is available at:

```text
C:\Program Files\Microsoft\jdk-17.0.19.10-hotspot
```

If vcpkg Fast DDS installation is unavailable or incompatible, or if the separate Fast DDS-Gen tool cannot be built, the implementation must stop and report the exact dependency blocker instead of replacing Fast DDS with another middleware.

## Error Handling

Pressure acquisition can fail because:

- sensor DLL files are missing
- DLL function exports do not match the expected names
- the pressure hardware is disconnected
- `workstart()` fails
- `catchdata()` returns invalid values or the DLL raises an error
- Fast DDS initialization fails
- DDS publishing fails

The service will:

- check DLL availability before acquisition starts
- refuse to start twice
- keep UI responsive during failures
- stop the worker cleanly on `FC_stop`
- emit message-box errors through `MainWindow`
- include DDS topic/type information in status messages
- avoid throwing exceptions across Qt signal/slot boundaries

## Redundant Python Code Not Converted

The following Python code and patterns are intentionally discarded:

- Launching `pressure_reader.py` as a subprocess.
- Hardcoded Anaconda Python executable path in `pressure_monitor.py`.
- Stdout text parsing for numeric samples.
- Matplotlib canvases and `ForceDataPlot.py` plotting helpers.
- Old direct-thread pressure code in `ForceDataObtained.py`.
- Old `pressure_display.py` code path.
- The unrelated thermal-camera `HCUSBSDK.dll` load inside `pressure_monitor.py`.
- Global lists used only for Matplotlib plotting.

## Testing

Automated tests:

- Pressure rolling window is not ready before the configured number of samples.
- Pressure rolling mean matches the Python-equivalent calculation.
- Variance, standard deviation, and peak match expected values.
- Pressure configuration defaults match the Python source behavior.
- Invalid pressure configuration values fall back safely.
- Starting the pressure service twice is rejected safely.
- Stopping when not running is harmless.
- DDS sample-building maps raw and processed values into the IDL struct correctly.

Manual tests with hardware and DDS tools:

- Build the application in Release mode.
- Confirm the portable output contains pressure sensor runtime DLLs and Fast DDS runtime DLLs.
- Open the app and click `FC_machine`.
- Start acquisition with `FC_start`.
- Confirm samples are published on `afp.pressure.sample`.
- Subscribe using PlotJuggler eProsima Edition or a small Fast DDS subscriber.
- Confirm raw and processed values update at approximately the configured sampling interval, default 10 Hz.
- Stop acquisition with `FC_stop`.
- Confirm sensor stop is called and DDS publishing stops.

## Acceptance Criteria

- `AFP_ABB_Python` remains unchanged.
- The C++ UI design remains unchanged.
- Existing pressure buttons remain functional in the main UI.
- Pressure data is acquired directly from the vendor DLL in C++.
- Raw and processed pressure data are published through Fast DDS.
- Embedded pressure plotting is not implemented in the main window.
- The pressure DDS topic and IDL are documented.
- The portable Windows output includes required runtime DLLs and configuration files.
- Automated non-hardware tests pass.
- Existing Slice A and Slice B tests still pass.
- README explains how to build, deploy, and subscribe to pressure DDS data.
