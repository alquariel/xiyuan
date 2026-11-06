# AFP ABB C++ Slice L Design: Robot Monitor DDS Archive To InfluxDB

Date: 2026-06-06

## Goal

Publish ABB robot monitor samples in real time with eProsima Fast DDS, then archive those samples to InfluxDB through the existing DDS-to-InfluxDB subscriber process.

This slice standardizes robot monitor data handling with the pressure data path from Slice C and Slice J:

```text
RAPID TASK2 socket
  -> RobotMonitoringService
  -> Fast DDS robot monitor topic
  -> dds_influx_subscriber.exe
  -> InfluxDB 3 Core
```

CSV and raw socket recording remain available, but they become diagnostic options rather than the default archive path. The normal archive path for robot monitor data becomes DDS plus InfluxDB.

## Current Context

Slice K already added a non-blocking robot monitor socket workflow:

- `RobotMonitorClient` receives framed `#ROBOT_MONITOR ...#` messages from RAPID TASK2.
- `RobotMonitorMessageParser` converts those frames into `RobotMonitorSample`.
- `RobotMonitoringService` computes `estimatedTcpSpeed`.
- `RobotMonitorRecorder` can write CSV and raw socket logs.

Slice J already added a reusable pressure archive pipeline:

- `dds_influx_subscriber.exe`
- `DdsInfluxSubscriberApp`
- `PressureSampleReader`
- `TimeSeriesPoint`
- `InfluxLineProtocolWriter`
- `TimeseriesArchiveService`

The design should reuse that pipeline instead of adding a second archive process or a second InfluxDB writer.

## Scope

Slice L includes:

- A robot monitor DDS IDL contract.
- Generated Fast DDS robot monitor type files.
- A `RobotMonitorDdsPublisher` used by `RobotMonitoringService`.
- A `RobotMonitorSampleReader` inside `dds_influx_subscriber.exe`.
- Conversion from robot monitor DDS samples to the existing `TimeSeriesPoint`.
- New config fields for robot monitor DDS publication and robot monitor InfluxDB archive settings.
- README documentation for robot DDS/InfluxDB validation.
- Tests for mapping, line protocol, publisher/reader startup boundaries, and subscriber app multi-reader behavior.

Slice L excludes:

- Any RAPID full-monitor-message expansion. This slice publishes the fields already represented by `RobotMonitorSample`; fields not yet sent by RAPID remain zero.
- Any Qt UI redesign.
- Any embedded InfluxDB writes from the main GUI process.
- Any second DDS-to-InfluxDB subscriber executable.
- Any new InfluxDB database. Robot monitor rows use the existing `timeseries_archive.database`, defaulting to `afp_process`.
- Any change to `ConnectionRobotButton` behavior.

## Recommended Architecture

Use one shared subscriber process for all time-series DDS archive readers:

```text
AFP_ABB_CPP.exe
  | publishes pressure DDS topic
  | publishes robot monitor DDS topic
  |
  v
Fast DDS discovery/domain
  |
  +--> dds_influx_subscriber.exe
         |
         +--> PressureSampleReader
         +--> RobotMonitorSampleReader
         |
         v
       TimeSeriesPoint queue
         |
         v
       InfluxLineProtocolWriter
         |
         v
       InfluxDB database: afp_process
```

`PressureSampleReader` and `RobotMonitorSampleReader` read concurrently through independent Fast DDS `DataReader` objects. Their Fast DDS listener callbacks are event-driven and are not polled sequentially by the subscriber app.

Both readers feed `TimeSeriesPoint` objects into the same Qt-thread queue through the existing queued callback pattern in `DdsInfluxSubscriberApp`. The Influx writer can batch points from multiple tables because each `TimeSeriesPoint` carries its own table name.

## Timestamp Synchronization

Timestamps must be sample timestamps, not database write timestamps.

Pressure samples already carry `timestamp_ms` from the pressure acquisition worker. Robot monitor DDS samples should carry `timestamp_ms` from the moment the C++ application receives/parses the RAPID monitor frame, using the existing `RobotMonitorSample::receivedAtUtc`.

InfluxDB writes may be batched and delayed, but line protocol should include each sample's own timestamp. This means writer latency does not shift the stored pressure or robot timestamps.

Remaining timing uncertainty is source-side:

- pressure DLL acquisition timing,
- RAPID socket send timing,
- PC receive/parse timing.

DDS delivery, subscriber queuing, and InfluxDB flush timing should not change stored timestamps.

## Robot Monitor DDS Contract

Add:

```text
src/robot_monitor/RobotMonitorSample.idl
```

The IDL type should be named `AfpRobotMonitorSample`.

Fields:

- `timestamp_ms`: unsigned 64-bit epoch milliseconds, assigned from `receivedAtUtc`.
- `sequence`: unsigned 64-bit sequence number.
- `x`, `y`, `z`: TCP Cartesian position.
- `q1`, `q2`, `q3`, `q4`: TCP orientation quaternion.
- `j1` through `j6`: robot joint positions.
- `eax1` through `eax6`: external axis positions.
- `commanded_tcp_speed`
- `commanded_orientation_speed`
- `commanded_external_linear_speed`
- `commanded_external_orientation_speed`
- `estimated_tcp_speed`

The generated files should live under:

```text
src/robot_monitor/generated
```

Generated files must not be edited by hand. The README should document the Fast DDS-Gen command, matching the existing pressure generated-file guidance.

## Main Application Publisher

Add:

```text
src/robot_monitor/RobotMonitorDdsTypes.h
src/robot_monitor/RobotMonitorDdsPublisher.h
src/robot_monitor/RobotMonitorDdsPublisher.cpp
```

`RobotMonitorDdsTypes.h` maps the hand-written `RobotMonitorSample` to the generated DDS type. This mirrors `PressureDdsTypes.h`, keeping generated classes out of most hand-written application code.

`RobotMonitorDdsPublisher` mirrors `PressureDdsPublisher`:

- owns the participant, publisher, topic, data writer, and type support;
- uses reliable QoS to match the pressure DDS path;
- lazily initializes on first publish;
- reports clear errors through `lastError()`;
- shuts down safely when the service stops.

`RobotMonitoringService` should create the publisher when `robot.monitor.dds_publish_enabled` is true. It should publish after `updateEstimatedSpeed(sample)` so the DDS stream and InfluxDB rows include the same estimated speed that CSV currently records.

DDS publish failures must not stop robot monitoring. They should be reported with `reportStatus()` and written to `afp_abb_cpp.log`.

## Subscriber Process Extension

Add:

```text
src/archive/RobotMonitorSampleReader.h
src/archive/RobotMonitorSampleReader.cpp
```

`RobotMonitorSampleReader` should mirror `PressureSampleReader`:

- own its Fast DDS participant/subscriber/topic/data reader/type support;
- subscribe to `robot_monitor_archive.dds_topic`;
- convert every valid `AfpRobotMonitorSample` into `TimeSeriesPoint`;
- invoke the existing `TimeSeriesPointHandler`.

`DdsInfluxSubscriberApp` should own optional readers:

```text
std::unique_ptr<PressureSampleReader> pressureReader_;
std::unique_ptr<RobotMonitorSampleReader> robotMonitorReader_;
```

Startup should allow one reader to be disabled while the other remains enabled:

- If both pressure and robot monitor archive are disabled, startup should return a clear error.
- If pressure archive is enabled and fails to start, startup should fail as it does today.
- If robot monitor archive is enabled and fails to start, startup should fail with a clear robot-specific DDS error.
- If one archive is disabled, the other can still run.

This keeps the subscriber extensible for future data sources without multiplying executables.

## InfluxDB Mapping

Add config:

```json
"robot_monitor_archive": {
  "enabled": true,
  "dds_domain_id": 0,
  "dds_topic": "afp.robot.monitor.sample",
  "table": "robot_monitor",
  "source_tag": "abb_irb4600"
}
```

Default line protocol:

```text
robot_monitor,source=abb_irb4600 sequence=12i,x=100.2,y=50.1,z=300,estimated_tcp_speed=8.5,...
```

Tags:

- `source`

Fields:

- `sequence`
- `x`, `y`, `z`
- `q1`, `q2`, `q3`, `q4`
- `j1` through `j6`
- `eax1` through `eax6`
- `commanded_tcp_speed`
- `commanded_orientation_speed`
- `commanded_external_linear_speed`
- `commanded_external_orientation_speed`
- `estimated_tcp_speed`

The source tag must stay low-cardinality. Do not use timestamps, sequence numbers, program names, or positions as tags.

## Config Changes

Update the default `robot.monitor` config:

```json
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
```

CSV/raw are off by default because DDS/InfluxDB becomes the normal archive path. Users can turn CSV/raw back on for diagnostics, especially during RAPID socket troubleshooting.

The existing `timeseries_archive` settings remain shared:

- InfluxDB executable path
- host URL
- database
- admin token
- batch size
- flush interval
- subscriber console visibility
- live data printing

No robot-specific InfluxDB server or writer settings should be introduced.

## Live Console Printing

Keep the existing `timeseries_archive.subscriber_print_live_data` behavior.

When enabled, the shared subscriber console should print both pressure and robot monitor line-protocol rows. This is useful during hardware validation because it shows both streams arriving through the same archive process.

If future console filtering is needed, add it later as an optional diagnostic setting. Do not add it in this slice unless hardware testing shows the mixed console output is too noisy.

## Error Handling

Main app:

- If robot DDS publish is disabled, robot monitoring still works and can still use CSV/raw diagnostics when enabled.
- If the robot DDS publisher cannot initialize, report status and continue socket monitoring.
- If a publish call fails, report status but do not disconnect the robot monitor socket.

Subscriber:

- If InfluxDB token loading fails, existing subscriber startup failure behavior remains.
- If no archive readers are enabled, fail with a clear message.
- If enabled robot reader creation fails, fail with a clear message naming the topic and domain.
- If InfluxDB writing fails, keep the existing writer warning and retry-on-next-flush behavior.

Data conversion:

- Non-finite floating-point fields should be skipped by the existing `TimeSeriesPoint::toLineProtocol()` behavior.
- A zero or overflowing timestamp should ask InfluxDB to apply server time, matching the pressure fallback behavior.

## Testing

Unit tests:

- `RobotMonitorDdsTypes` maps `RobotMonitorSample` to `AfpRobotMonitorSample`.
- `RobotMonitorSampleReader` converts a generated DDS sample to `TimeSeriesPoint`.
- `TimeSeriesPoint` formats robot monitor line protocol with the configured table and source tag.
- `DdsInfluxSubscriberApp` starts when only pressure archive is enabled.
- `DdsInfluxSubscriberApp` starts when only robot monitor archive is enabled.
- `DdsInfluxSubscriberApp` starts both readers when both archives are enabled.
- `DdsInfluxSubscriberApp` fails clearly when both archives are disabled.
- `RobotMonitoringService` publishes samples when DDS publishing is enabled and does not publish when disabled.

Integration/manual tests:

1. Confirm `build\Release\dds_influx_subscriber.exe` exists beside `AFP_ABB_CPP.exe`.
2. Confirm `config\env_config.json` has matching `robot.monitor.dds_topic` and `robot_monitor_archive.dds_topic`.
3. Start InfluxDB and the main application.
4. Start RAPID robot monitor task.
5. Confirm the subscriber console prints `robot_monitor` rows when live printing is enabled.
6. Confirm InfluxDB table `robot_monitor` receives rows.
7. Confirm pressure rows still arrive in `pressure_sensor` when pressure acquisition is running.
8. Confirm CSV/raw robot monitor files are not created by default.
9. Enable `record_csv` and `record_raw_messages` manually and confirm diagnostic files can still be created.

## Acceptance Criteria

- `AFP_ABB_Python` remains untouched.
- Robot monitor data is published through Fast DDS when `robot.monitor.dds_publish_enabled` is true.
- `dds_influx_subscriber.exe` can subscribe to both pressure and robot monitor DDS streams.
- One shared `InfluxLineProtocolWriter` writes both pressure and robot monitor tables.
- DDS/InfluxDB is the default robot monitor archive path.
- CSV/raw robot monitor files are disabled by default but still available as diagnostics.
- Pressure DDS/InfluxDB behavior remains compatible with Slice J.
- Stored InfluxDB timestamps come from source sample timestamps, not subscriber write time.
- README documents robot monitor DDS config, InfluxDB validation, and CSV/raw diagnostic behavior.
