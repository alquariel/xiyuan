# Slice M Design: EGM Pressure Path Correction

Date: 2026-11-27

Status: Consolidated design. This document merges the 2026-11-24 Slice M design with the two 2026-11-25 change notes (panel UI simplification and Kd / correction-button changes). It is the single design reference for the feature; the as-built code is recorded in `docs/plans/2026-11-27-afp-abb-cpp-slice-m-egm-pressure-path-correction-implementation.md`.

## 1. Purpose

Add an in-process EGM path-correction module to `AFP_ABB_CPP.exe`. The module subscribes to the pressure and robot-monitor DDS topics, runs a PID control law along the layup normal, and returns a bounded Z offset to the ABB controller through the EGM path-correction channel every 48 ms. The controller adds that offset to the nominal RAPID path.

Scope:

- New "EGM Pressure Correction" tab in the existing UI.
- Fast DDS subscribers for `afp.pressure.sample` and `afp.robot.monitor.sample`.
- UDP server on port 6510 implementing `EgmRobot` in / `EgmSensorPathCorr` out.
- PID control law with output and rate limits, stale-data ramp-down, and anti-windup.
- RAPID `Motion.mod` CASE 33 using `EGMSetupUC \PathCorr \APTR`, `EGMActMove`, `EGMMoveL`, and `EGMReset`.
- Unit and loopback tests that run without robot, PLC, or sensor hardware.

Non-goals:

- No changes to the pressure publisher, robot-monitor publisher, or `dds_influx_subscriber.exe`.
- No changes to the Siemens PLC print/motion channel or its Snap7 connection.
- No pressure-unit calibration.
- No multi-robot or multi-EGM-instance support.
- No ROS replacement; Fast DDS remains the transport.

## 2. Field Prerequisites

| Item | Requirement |
|---|---|
| Controller | ABB IRC5 |
| RobotWare | 6.15 (EGM path correction instruction set) |
| EGM option | Option 689-1 installed |
| Controller IP | `192.168.0.104` (shared with motion socket 5010, monitor socket 5012, FTP 21) |
| PC network | Static IP in the same subnet; controller and PC can ping each other |
| Tool | `currentTool` calibrated, with tool +Z perpendicular to the layup surface |
| UDP ports | PC listens on 6510; controller uses local port 6511 |
| Firewall | Inbound UDP 6510 allowed on the PC network profile |
| RAPID | Modified `rapid/RAPID/TASK1/PROGMOD/Motion.mod` loaded and compiled |
| Data sources | Pressure and robot-monitor DDS publishers running on domain 0 |

Controller-side system parameters:

1. **UdpUc device** `UCdevice`: Remote Address = PC IP, Remote Port = 6510, Local Port = 6511. Restart the controller after adding it.
2. **External Motion Interface Data** `pathCorr` under the Motion topic. The name is case-sensitive and must match the RAPID string. Restart the controller.
3. Keep the default ramp time and default gains. The outer loop runs on the PC; raising controller-side gains would couple two loops.
4. Start PC listening **before** setting `useEgmPathCorr` to TRUE. `EGMSetupUC`/`EGMActMove` wait for UdpUc communication and raise `ERR_UDPUC_COMM` if the PC is not ready.

Operating preconditions for closed-loop testing:

- Controller in Auto mode, motors on, RAPID program ready.
- Do not validate the loop in manual or step mode.
- First commissioning round uses `kp = ki = kd = 0` (zero correction).

## 3. Architecture

### 3.1 Process And Thread Model

`AFP_ABB_CPP.exe` hosts UI, PLC, print workflow, and the EGM module.

```text
Qt main thread      : UI tab, parameter edits, 100 ms status refresh
EGM worker QThread  : blocking recvfrom(EgmRobot) -> PID -> sendto(EgmSensorPathCorr)
DDS listener threads: write latest-value snapshots only
```

Rules:

- The EGM loop is driven by received `EgmRobot` frames, never by a UI timer.
- DDS callbacks only store a snapshot; no control computation runs in a DDS thread.
- The EGM worker never shares a thread, socket, or Snap7 connection with the print channel.
- UI reads immutable snapshots; parameter updates are guarded by a mutex.

### 3.2 Data Path

```text
PressureAcquisitionWorker --DDS afp.pressure.sample--------+
                                                           +--> EGM module (QThread)
RobotMonitoringService ----DDS afp.robot.monitor.sample----+      PID: P_mean -> z_corr
                                                                  every 48 ms
                                                                     |
                                                                     v
                                       UDP 6510 EgmSensorPathCorr (pathCorr.pos.z)
                                                                     |
                                                                     v
                                              ABB controller EGM path correction
                                                                     |
                                                                     v
                                       RAPID nominal path + external normal offset
```

### 3.3 Time Scales

| Stage | Period | Notes |
|---|---|---|
| Pressure DDS | 100 ms | `pressure.sample_interval_ms` |
| Robot monitor DDS | 100 ms | `robot.monitor.sample_interval_ms` |
| EGM exchange | 48 ms | `SampleRate:=48`; legal values 24 / 48 / 72 |

DDS is slower than EGM. One pressure sample is reused for about two EGM cycles; this is intentional sample-and-hold, smoothed by the integral term and the per-cycle rate limit.

## 4. EGM Message Contract

- The controller is the UDP client; the PC is the UDP server bound to 6510.
- The controller sends `EgmRobot`; the PC must reply with `EgmSensorPathCorr` in the same cycle.
- No reply may be sent before the first `EgmRobot` frame arrives.
- `seqno` increases by exactly one per sent frame.
- The reply contains `header` and `pathCorr` only.

`EgmSensorPathCorr` (PC to controller):

```text
EgmSensorPathCorr
  header
    seqno : increments once per frame
    tm    : PC monotonic milliseconds since server start
    mtype : MSGTYPE_PATH_CORRECTION (= 4)
  pathCorr
    pos { x = 0, y = 0, z = z_corr (mm) }
    age : uint32, milliseconds (default 48)
```

`EgmRobot` (controller to PC) is decoded for:

- `header.seqno`, `header.tm`, `header.mtype`
- `feedBack.cartesian.pos` and quaternion
- `motorState`
- `mciState`

The field contract is `third_party/egm/egm.proto`; it is a reference only and is not compiled. Encoding and decoding are implemented in `src/egm/EgmWireCodec` and verified byte-for-byte against the Python `egm_pb2` output from the reference implementation.

## 5. Control Law

The controller is a PID law running once per `EgmRobot` frame with `dt` equal to the measured frame interval (48 ms nominal, clamped to 0.001-0.5 s).

```text
e          = P_mean - setpoint
derivative = kd * (P_mean - P_prev) / dt          (clamped to ±z_max; 0 on first frame)
i_next     = clamp(i_term + ki * e * dt, ±z_max)
raw        = kp * e + derivative + i_next
if raw is saturated and e drives further saturation: keep i_term (conditional integration)
raw        = clamp(raw, ±z_max)
z_corr     = clamp(raw, output ± dz_max)          (per-cycle rate limit)
```

Sign contract:

```text
Tool +Z = lift off the workpiece
P_mean > setpoint (over pressure) -> z_corr > 0 -> lift
P_mean < setpoint (under pressure) -> z_corr < 0 -> press
```

Units: pressure stays in raw sensor units until calibrated; `z_corr` is millimetres.

Behavior rules:

- `kd = 0` reproduces the original PI behavior exactly.
- The derivative is taken on the measurement, so setpoint edits do not create spikes.
- Parameter changes apply immediately and do not reset the integral term.
- The integral term is clamped to `±z_max` and frozen while saturated in the error direction.
- First cycle starts from zero output and zero integral.
- If the sample is invalid, stale (`sample_age_ms > stale_timeout_ms`), or `sensor_running` is false, the integral and derivative history are cleared and `z_corr` ramps to zero at `ramp_rate_mm_per_s`.
- `reset()` clears output, integral, derivative history, and the saturation flag.

## 6. Safety Behavior

- **Hard output clamp:** `EgmUdpServer` clamps every outgoing `z` to `z_max_mm`, independent of the PID result.
- **Rate limit:** output changes by at most `dz_max_per_cycle_mm` per EGM cycle.
- **Stale pressure:** data older than `stale_timeout_ms` forces a smooth ramp to zero; the tab reports an error.
- **Link timeout:** if no `EgmRobot` arrives within `receive_timeout_ms`, the server stops replying and reports `linkTimeout`. The controller then performs its own EGM timeout stop.
- **First-frame gate:** no datagram is sent before the controller is seen.
- **Stop sequence:** "stop listening" disables correction, ramps to zero, then closes the socket, so the controller does not hit an EGM timeout in the middle of a print.
- **Recovery:** after a link gap longer than `receive_timeout_ms`, the integral and last output are reset before resuming control.
- **Deferred:** an application-level soft deviation interlock (for example, abort if the monitored TCP departs the nominal path by more than 5 mm) is not implemented in this slice. The robot pose snapshot is currently used for status display only.

## 7. UI Design

The tab is added to `Other_tabWidget_Robotplatform` as "EGM Pressure Correction" when `egm_pressure.enabled` is true.

Final layout:

| Area | Content |
|---|---|
| Connection status | Pressure DDS, pose DDS, EGM link state |
| Live values | Pressure mean, correction output (mm), received/sent frame counts |
| Parameters | Setpoint, Kp, Ki, Kd, Z limit (mm), per-cycle limit (mm) |
| Actions | Start listening, stop listening, enable correction, stop correction, zero |
| Alerts | DDS errors, link timeout, worker errors |

Design decisions from 2026-11-25:

- The two 60 s trend plots (pressure mean and Z correction) were removed; the tab now uses text status only.
- The single toggle for correction was split into two explicit buttons, "Enable Correction" and "Stop Correction".
- `Kd` was added to the parameter row with a default of 0.
- Parameters apply immediately; editing them does not reset the integral term.
- Button enablement:
  - Not listening: only "start listening" is enabled.
  - Listening and not correcting: "stop listening", "zero", and "enable correction" are enabled.
  - Listening and correcting: "stop listening", "zero", and "stop correction" are enabled.
- Stopping listening or zeroing returns the correction state to disabled.

## 8. Configuration

`config/env_config.json`:

```json
"egm_pressure": {
  "enabled": true,
  "robot_ip": "192.168.0.104",
  "egm_udp_port": 6510,
  "control_period_ms": 48,
  "correction_age_ms": 48,
  "receive_timeout_ms": 500,
  "pressure_topic": "afp.pressure.sample",
  "robot_monitor_topic": "afp.robot.monitor.sample",
  "dds_domain_id": 0,
  "pressure_setpoint": 0.0,
  "kp": 0.0,
  "ki": 0.0,
  "kd": 0.0,
  "z_max_mm": 3.0,
  "dz_max_per_cycle_mm": 0.1,
  "stale_timeout_ms": 300,
  "ramp_rate_mm_per_s": 5.0
}
```

Notes:

- `enabled: false` skips tab creation, DDS subscriptions, and the worker thread.
- `kp = ki = kd = 0` means zero correction and is the required first-round setting.
- `receive_timeout_ms` should be longer than the EGM period and shorter than the operator's reaction time; 500 ms is the default.
- `stale_timeout_ms` should cover a few pressure periods; 300 ms is the default.
- Invalid values are reported through the existing `AppConfig` warning mechanism and fall back to defaults.

## 9. RAPID Integration

Only the copied RAPID tree under `rapid/` is modified. `Motion.mod` CASE 33 wraps the buffered path:

```rapid
PERS bool useEgmPathCorr:=FALSE;
PERS string egmExtConfigName:="pathCorr";
PERS string egmUcDevice:="UCdevice";
PERS num egmSampleRate:=48;
VAR egmident egmId1;

IF useEgmPathCorr AND BUFFER_POS>1 THEN
    EGMGetId egmId1;
    EGMSetupUC ROB_1,egmId1,egmExtConfigName,egmUcDevice\PathCorr\APTR;
    EGMActMove egmId1,currentTool.tframe\SampleRate:=egmSampleRate;
ENDIF

FOR i FROM 1 TO BUFFER_POS DO
    IF useEgmPathCorr AND BUFFER_POS>1 THEN
        IF i=1 THEN
            MoveL bufferTargets{i},bufferSpeeds{i},fine,currentTool\WObj:=currentWobj;
        ELSEIF i=BUFFER_POS THEN
            EGMMoveL egmId1,bufferTargets{i},bufferSpeeds{i},fine,currentTool\WObj:=currentWobj;
        ELSE
            EGMMoveL egmId1,bufferTargets{i},bufferSpeeds{i},currentZone,currentTool\WObj:=currentWobj;
        ENDIF
    ELSE
        MoveL bufferTargets{i},bufferSpeeds{i},currentZone,currentTool\WObj:=currentWobj;
    ENDIF
ENDFOR

IF useEgmPathCorr AND BUFFER_POS>1 THEN
    EGMReset egmId1;
ENDIF
```

Requirements:

- First point is a normal `MoveL ... fine`; the last `EGMMoveL` must also be `fine`.
- `\APTR` (at-point) matches a sensor mounted at the TCP. `\LATR` is not used.
- `BUFFER_POS <= 1` bypasses EGM entirely.
- The switch defaults to FALSE. Rollback is setting it back to FALSE or restoring the previous `Motion.mod`.
- The PLC single-point forwarding path (`PLC_SERVER.mod`) is not wrapped by EGM.

## 10. Build And Test Strategy

- The module is part of the `AFP_ABB_CPP` target; no separate executable and no `QProcess`.
- No protobuf runtime is added. The two messages are encoded/decoded by `src/egm/EgmWireCodec`; generated Python `egm_pb2` bytes are used as the golden reference in tests.
- MinGW builds define `MINGW_COMPILER` for Fast DDS headers.
- Runtime deployment: `POST_BUILD` copies `config/`, `assets/`, `lib/`, creates `mod_files/`, runs `windeployqt`, copies `TARGET_RUNTIME_DLLS`, and adds Fast DDS indirect dependencies (`libcrypto`, `libssl`, `libtinyxml2`, `libfoonathan_memory`).
- Tests:
  - `test_egm_pressure_pi_controller`: pure control law, no Qt event loop required beyond QtTest.
  - `test_egm_udp_server`: codec golden bytes plus localhost loopback with simulated `EgmRobot` frames.

## 11. Commissioning Plan

Phase 1, pure host self-test (no robot, PLC, or sensor):

| ID | Check | Method | Pass criterion |
|---|---|---|---|
| T1 | Zero correction reply is constant 0 | Loopback server with `kp=ki=kd=0` or correction disabled | Every reply `z = 0`, `x = y = 0`, `mtype = 4`, `age = 48`, `seqno` strictly +1 |
| T2 | Step response | PID plus simulated first-order plant | Rise, overshoot, settling, and steady-state error within agreed baselines |
| T3 | `z_max` saturation | Large error and gain | `max |z| = z_max`, never exceeded |
| T4 | `dz_max` rate limit | Step from zero | Every `|Δz| <= dz_max`; reaching `z_max` takes at least `ceil(z_max / dz_max)` cycles |
| T5 | DDS stream loss | Invalidate or age the sample | Smooth ramp to zero at `ramp_rate`, integral cleared, no step |
| T6 | UDP timeout | Stop sending `EgmRobot` | No reply after `receive_timeout_ms`; send counters freeze; recovery works |
| T7 | No reply before first frame | Bind port, wait | No datagram is sent |

Because a host without pressure data always produces `z_corr = 0`, T2-T4 must be driven by the unit tests or a synthetic pressure publisher, not by observing the tab alone.

Phase 2, hardware commissioning (strict order):

1. Zero-correction dry run (`kp=ki=kd=0`): confirm the TCP follows the nominal path and `EgmRobot` arrives continuously.
2. Open-loop step: apply `z_corr = +1 mm` manually and confirm the tool moves away from the surface by 1 mm. If the direction is inverted, fix the sign contract, not the gains.
3. Closed-loop tuning: raise `kp` first, add `ki` last, then `kd` only if overshoot must be suppressed.
4. Fault injection: stop pressure DDS, stop pressure publication, trigger `sensor_running = false`, and stop UDP. Confirm ramp-to-zero, alarms, and no unexpected motion.
5. Rollback: click "zero" and wait for `z_corr` to reach 0 before stopping listening; then set `useEgmPathCorr := FALSE` or restore the backup.

## 12. Risks And Open Items

| # | Risk | Mitigation |
|---|---|---|
| 1 | Path-frame Z and tool-frame Z may not be exactly aligned | Zero-correction dry run plus open-loop step before enabling the loop; sign contract is a fixed constant |
| 2 | Pressure units are uncalibrated, so gains are dimensionless | Tune in raw units first; recalibrate and retune if engineering units are adopted |
| 3 | Pressure DDS at 100 ms is slower than the 48 ms EGM loop | Sample-and-hold plus integral term and rate limit |
| 4 | Controller-side gain and ramp settings can couple with the PC loop | Keep controller defaults |
| 5 | Operator error can cause a sudden offset | Hard clamp, rate limit, zero button, ramp-down on stop, no integral reset on parameter edits |
| 6 | 6.15 controller syntax must be confirmed on site | Compile `Motion.mod` in RobotStudio/flexpendant; keep `useEgmPathCorr` FALSE until then |
| 7 | Soft path-deviation interlock not implemented | Tracked as deferred work; current pose DDS is display-only |

## 13. Out Of Scope

- Changes to `PressureDdsPublisher`, `RobotMonitorDdsPublisher`, or `dds_influx_subscriber`.
- Changes to PLC print/motion commands and the Snap7 connection.
- Physical pressure calibration.
- Separate EGM executable, process supervision, or `QProcess`.
- Multiple robots or multiple simultaneous EGM sessions.
- Socket-based movement or RAPID monitor stream changes.

## 14. Superseded Documents

- `docs/design/2026-11-24-afp-abb-cpp-slice-m-egm-pressure-path-correction-design.md`
- `docs/design/2026-11-25-egm-pressure-panel-ui-change.md`
- `docs/design/2026-11-25-egm-pressure-kd-and-correction-buttons.md`
