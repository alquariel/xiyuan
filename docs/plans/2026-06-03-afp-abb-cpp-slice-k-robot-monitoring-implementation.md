# AFP ABB C++ Slice K Robot Monitoring Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add config-only ABB robot monitoring that records live RAPID socket samples to CSV/raw logs without changing existing robot connection, print, or PLC-forward movement behavior.

**Architecture:** Build a separate `robot_monitor` C++ subsystem with config, parser, socket client, recorder, and Qt service boundaries. Start the service from `MainWindow::startBackgroundServices()` only when `robot.monitor.enabled` and `robot.monitor.autostart` are true. Update only the copied RAPID files under `AFP_ABB_CPP\rapid`, focusing first on a minimal `#ROBOT_MONITOR ...#` stream.

**Tech Stack:** C++17, Qt6 Core/Network/Widgets/Test, CMake, ABB RAPID, CSV text output. DDS and InfluxDB robot data are extension points, not part of this slice.

---

## Guardrails

- Do not modify anything under `AFP_ABB_Python`.
- Ignore unrelated dirty/untracked files unless they block build or tests.
- Keep new C++ code heavily commented, especially file/class/function purpose comments.
- Do not change `ConnectionRobotButton` behavior.
- Do not extend `RobotSocketClient` for monitoring; keep print command socket and monitor socket separate.
- Keep `robot.monitor` nested under the existing `robot` config object.
- Remove `robot.port_server` only if a repository-wide search confirms it has no remaining C++ use after implementation.

## File Structure

Create:

- `src/robot_monitor/RobotMonitorSample.h`: value type for one parsed robot monitor sample.
- `src/robot_monitor/RobotMonitorMessageParser.h`
- `src/robot_monitor/RobotMonitorMessageParser.cpp`: parse `#ROBOT_MONITOR key=value ...#` text.
- `src/robot_monitor/RobotMonitorRecorder.h`
- `src/robot_monitor/RobotMonitorRecorder.cpp`: CSV and optional raw log writer.
- `src/robot_monitor/RobotMonitorClient.h`
- `src/robot_monitor/RobotMonitorClient.cpp`: non-blocking Qt socket client and `#` frame splitter.
- `src/services/robot_monitor/RobotMonitoringService.h`
- `src/services/robot_monitor/RobotMonitoringService.cpp`: config-driven startup orchestration.
- `tests/test_robot_monitor_message_parser.cpp`
- `tests/test_robot_monitor_recorder.cpp`
- `tests/test_robot_monitor_client.cpp`
- `tests/test_robot_monitoring_service.cpp`

Modify:

- `src/app/AppConfig.h`: add `RobotMonitorConfig`; nest it inside `RobotConfig`.
- `src/app/AppConfig.cpp`: parse `robot.monitor`.
- `config/env_config.json`: add `robot.monitor`; remove `robot.port_server` if unused.
- `src/ui/MainWindow.h`: add `RobotMonitoringService` member.
- `src/ui/MainWindow.cpp`: construct service, connect status signal, call service from `startBackgroundServices()`.
- `CMakeLists.txt`: add new app sources and new test targets.
- `README.md`: document monitor config, CSV/raw log locations, copied RAPID location, and future DDS/InfluxDB path.
- `rapid/RAPID/TASK2/PROGMOD/UPLOAD_ROBOT.mod`: add minimal monitor send routine and call it from the main loop.
- `rapid/RAPID/TASK1/PROGMOD/Motion.mod`: add only if TASK2 cannot directly read live pose/speed on the controller.
- `rapid/RAPID/TASK3/PROGMOD/PLC_ROBOT_COMM_GI_GO.mod`: avoid edits unless monitor state/progress needs to be exposed.
- `rapid/HOME/testmod.mod`: add only comments or minimal sample usage notes if needed for testing.

---

### Task 1: Add Nested Robot Monitor Config

**Files:**

- Modify: `src/app/AppConfig.h`
- Modify: `src/app/AppConfig.cpp`
- Modify: `config/env_config.json`
- Modify: `tests/test_app_config.cpp`

- [ ] **Step 1: Write the failing config test**

Add a test case in `tests/test_app_config.cpp` that writes a temporary `env_config.json` containing nested monitor settings and verifies the values are loaded:

```cpp
void TestAppConfig::loadsNestedRobotMonitorConfig() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QDir root(dir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"json({
      "robot": {
        "ip": "192.168.0.104",
        "port_motion": 5010,
        "monitor": {
          "enabled": true,
          "autostart": true,
          "port": 5012,
          "sample_interval_ms": 100,
          "record_csv": true,
          "record_raw_messages": true,
          "recording_dir": "logs/robot_monitor",
          "flush_every_sample": true
        }
      }
    })json");
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QCOMPARE(config.robot.ip, QString("192.168.0.104"));
    QCOMPARE(config.robot.portMotion, 5010);
    QVERIFY(config.robot.monitor.enabled);
    QVERIFY(config.robot.monitor.autostart);
    QCOMPARE(config.robot.monitor.port, 5012);
    QCOMPARE(config.robot.monitor.sampleIntervalMs, 100);
    QVERIFY(config.robot.monitor.recordCsv);
    QVERIFY(config.robot.monitor.recordRawMessages);
    QCOMPARE(config.robot.monitor.recordingDir, QString("logs/robot_monitor"));
    QVERIFY(config.robot.monitor.flushEverySample);
}
```

Declare the slot in the test class:

```cpp
void loadsNestedRobotMonitorConfig();
```

- [ ] **Step 2: Run the config test and verify it fails**

Run:

```powershell
ctest --test-dir build -C Release -R test_app_config --output-on-failure
```

Expected: fail to compile or fail test because `RobotConfig::monitor` is not defined.

- [ ] **Step 3: Add config types**

In `src/app/AppConfig.h`, add:

```cpp
struct RobotMonitorConfig {
    // Enables the independent robot monitoring socket workflow.
    bool enabled = false;

    // Starts monitoring from MainWindow::startBackgroundServices(), not from
    // ConnectionRobotButton. This preserves the existing robot connection path.
    bool autostart = false;

    // RAPID TASK2 upload/monitor socket port.
    int port = 5012;

    // RAPID-side requested publication interval. The C++ side records whatever
    // the robot actually sends, but keeps this value in config for documentation
    // and future controller-side generation.
    int sampleIntervalMs = 100;

    bool recordCsv = true;
    bool recordRawMessages = true;
    QString recordingDir = "logs/robot_monitor";

    // Flush each row immediately during hardware validation so the newest data
    // survives crashes and forced exits.
    bool flushEverySample = true;
};
```

Add to `RobotConfig`:

```cpp
RobotMonitorConfig monitor;
```

Search for `portServer`:

```powershell
rg -n "portServer|port_server" src config tests
```

If only `AppConfig` and `env_config.json` still use it, remove `int portServer` from `RobotConfig`, remove parsing of `robot.port_server`, and remove `"port_server": 5012` from `config/env_config.json`.

- [ ] **Step 4: Parse nested config**

In `src/app/AppConfig.cpp`, after parsing `robot.default_speed`, add:

```cpp
const QJsonObject robotMonitor = robot.value("monitor").toObject();
config.robot.monitor.enabled = readBool(robotMonitor, "enabled", config.robot.monitor.enabled);
config.robot.monitor.autostart = readBool(robotMonitor, "autostart", config.robot.monitor.autostart);
config.robot.monitor.port = readPositiveInt(
    config, robotMonitor, "port", config.robot.monitor.port, "robot.monitor.port");
config.robot.monitor.sampleIntervalMs = readPositiveInt(
    config,
    robotMonitor,
    "sample_interval_ms",
    config.robot.monitor.sampleIntervalMs,
    "robot.monitor.sample_interval_ms");
config.robot.monitor.recordCsv = readBool(robotMonitor, "record_csv", config.robot.monitor.recordCsv);
config.robot.monitor.recordRawMessages =
    readBool(robotMonitor, "record_raw_messages", config.robot.monitor.recordRawMessages);
config.robot.monitor.recordingDir = readNonEmptyString(
    config,
    robotMonitor,
    "recording_dir",
    config.robot.monitor.recordingDir,
    "robot.monitor.recording_dir");
config.robot.monitor.flushEverySample =
    readBool(robotMonitor, "flush_every_sample", config.robot.monitor.flushEverySample);
```

- [ ] **Step 5: Update default config JSON**

In `config/env_config.json`, update the `robot` object to include:

```json
"monitor": {
  "enabled": true,
  "autostart": true,
  "port": 5012,
  "sample_interval_ms": 100,
  "record_csv": true,
  "record_raw_messages": true,
  "recording_dir": "logs/robot_monitor",
  "flush_every_sample": true
}
```

If `port_server` is confirmed unused, remove it from `robot`.

- [ ] **Step 6: Run config test**

Run:

```powershell
cmake --build build --config Release --target test_app_config
ctest --test-dir build -C Release -R test_app_config --output-on-failure
```

Expected: `test_app_config` passes.

- [ ] **Step 7: Commit**

Run:

```powershell
git add src/app/AppConfig.h src/app/AppConfig.cpp config/env_config.json tests/test_app_config.cpp
git commit -m "config: add robot monitor settings"
```

---

### Task 2: Add Robot Monitor Sample and Parser

**Files:**

- Create: `src/robot_monitor/RobotMonitorSample.h`
- Create: `src/robot_monitor/RobotMonitorMessageParser.h`
- Create: `src/robot_monitor/RobotMonitorMessageParser.cpp`
- Create: `tests/test_robot_monitor_message_parser.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add parser test target to CMake**

Add to `CMakeLists.txt` near other robot tests:

```cmake
add_executable(test_robot_monitor_message_parser
    tests/test_robot_monitor_message_parser.cpp
    src/robot_monitor/RobotMonitorSample.h
    src/robot_monitor/RobotMonitorMessageParser.h
    src/robot_monitor/RobotMonitorMessageParser.cpp
)
target_include_directories(test_robot_monitor_message_parser PRIVATE src)
target_link_libraries(test_robot_monitor_message_parser PRIVATE Qt6::Core Qt6::Test)
copy_qt_test_runtime(test_robot_monitor_message_parser)
add_test(NAME test_robot_monitor_message_parser COMMAND test_robot_monitor_message_parser)
```

- [ ] **Step 2: Write failing parser tests**

Create `tests/test_robot_monitor_message_parser.cpp`:

```cpp
#include <QtTest/QtTest>

#include "robot_monitor/RobotMonitorMessageParser.h"

class TestRobotMonitorMessageParser final : public QObject {
    Q_OBJECT

private slots:
    void parsesValidMonitorMessage();
    void rejectsWrongPrefix();
    void rejectsMissingRequiredField();
};

void TestRobotMonitorMessageParser::parsesValidMonitorMessage() {
    const QString raw =
        "#ROBOT_MONITOR seq=12 x=100.20 y=50.10 z=300.00 "
        "q1=0.707 q2=0 q3=0 q4=0 "
        "j1=1.0 j2=2.0 j3=3.0 j4=4.0 j5=5.0 j6=6.0 "
        "eax1=10 eax2=20 eax3=30 speed_tcp=120.5 speed_ori=40 speed_leax=20 speed_reax=10#";

    const RobotMonitorParseResult result = RobotMonitorMessageParser::parse(raw);

    QVERIFY(result.ok);
    QCOMPARE(result.sample.sequence, 12);
    QCOMPARE(result.sample.x, 100.20);
    QCOMPARE(result.sample.y, 50.10);
    QCOMPARE(result.sample.z, 300.00);
    QCOMPARE(result.sample.q1, 0.707);
    QCOMPARE(result.sample.j6, 6.0);
    QCOMPARE(result.sample.externalAxis[0], 10.0);
    QCOMPARE(result.sample.commandedTcpSpeed, 120.5);
}

void TestRobotMonitorMessageParser::rejectsWrongPrefix() {
    const RobotMonitorParseResult result =
        RobotMonitorMessageParser::parse("#OTHER seq=1 x=1 y=2 z=3#");

    QVERIFY(!result.ok);
    QVERIFY(result.error.contains("ROBOT_MONITOR"));
}

void TestRobotMonitorMessageParser::rejectsMissingRequiredField() {
    const RobotMonitorParseResult result =
        RobotMonitorMessageParser::parse("#ROBOT_MONITOR seq=1 x=1 y=2#");

    QVERIFY(!result.ok);
    QVERIFY(result.error.contains("z"));
}

QTEST_MAIN(TestRobotMonitorMessageParser)
#include "test_robot_monitor_message_parser.moc"
```

- [ ] **Step 3: Run parser test and verify it fails**

Run:

```powershell
cmake --build build --config Release --target test_robot_monitor_message_parser
```

Expected: fail because parser files do not exist.

- [ ] **Step 4: Create sample type**

Create `src/robot_monitor/RobotMonitorSample.h`:

```cpp
#pragma once

#include <array>

#include <QDateTime>

// One parsed robot monitor sample received from the RAPID monitoring socket.
// The type is intentionally plain so it can later be published through DDS or
// mapped into InfluxDB without depending on Qt widgets or robot services.
struct RobotMonitorSample {
    QDateTime receivedAtUtc;
    int sequence = 0;

    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double q1 = 0.0;
    double q2 = 0.0;
    double q3 = 0.0;
    double q4 = 0.0;

    std::array<double, 6> joints = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::array<double, 6> externalAxis = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    double commandedTcpSpeed = 0.0;
    double commandedOrientationSpeed = 0.0;
    double commandedExternalLinearSpeed = 0.0;
    double commandedExternalOrientationSpeed = 0.0;

    double estimatedTcpSpeed = 0.0;
};
```

- [ ] **Step 5: Create parser interface**

Create `src/robot_monitor/RobotMonitorMessageParser.h`:

```cpp
#pragma once

#include <QString>

#include "robot_monitor/RobotMonitorSample.h"

struct RobotMonitorParseResult {
    bool ok = false;
    RobotMonitorSample sample;
    QString error;
};

// Converts one RAPID '#ROBOT_MONITOR key=value ...#' frame into a typed sample.
class RobotMonitorMessageParser final {
public:
    static RobotMonitorParseResult parse(const QString& rawMessage);
};
```

- [ ] **Step 6: Implement parser**

Create `src/robot_monitor/RobotMonitorMessageParser.cpp`:

```cpp
#include "robot_monitor/RobotMonitorMessageParser.h"

#include <QDateTime>
#include <QMap>
#include <QStringList>

namespace {
bool readDouble(const QMap<QString, QString>& fields, const QString& key, double& value, QString& error) {
    if (!fields.contains(key)) {
        error = QString("Missing required field '%1'.").arg(key);
        return false;
    }
    bool ok = false;
    const double parsed = fields.value(key).toDouble(&ok);
    if (!ok) {
        error = QString("Field '%1' is not a valid number.").arg(key);
        return false;
    }
    value = parsed;
    return true;
}

bool readInt(const QMap<QString, QString>& fields, const QString& key, int& value, QString& error) {
    if (!fields.contains(key)) {
        error = QString("Missing required field '%1'.").arg(key);
        return false;
    }
    bool ok = false;
    const int parsed = fields.value(key).toInt(&ok);
    if (!ok) {
        error = QString("Field '%1' is not a valid integer.").arg(key);
        return false;
    }
    value = parsed;
    return true;
}

double optionalDouble(const QMap<QString, QString>& fields, const QString& key) {
    bool ok = false;
    const double parsed = fields.value(key).toDouble(&ok);
    return ok ? parsed : 0.0;
}
}

RobotMonitorParseResult RobotMonitorMessageParser::parse(const QString& rawMessage) {
    RobotMonitorParseResult result;

    QString message = rawMessage.trimmed();
    if (message.startsWith('#')) {
        message.remove(0, 1);
    }
    if (message.endsWith('#')) {
        message.chop(1);
    }

    const QStringList parts = message.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty() || parts.first() != "ROBOT_MONITOR") {
        result.error = "Message does not start with ROBOT_MONITOR.";
        return result;
    }

    QMap<QString, QString> fields;
    for (int i = 1; i < parts.size(); ++i) {
        const int equalsIndex = parts.at(i).indexOf('=');
        if (equalsIndex <= 0 || equalsIndex == parts.at(i).size() - 1) {
            result.error = QString("Malformed key=value field '%1'.").arg(parts.at(i));
            return result;
        }
        fields.insert(parts.at(i).left(equalsIndex), parts.at(i).mid(equalsIndex + 1));
    }

    RobotMonitorSample sample;
    sample.receivedAtUtc = QDateTime::currentDateTimeUtc();

    QString error;
    if (!readInt(fields, "seq", sample.sequence, error)
        || !readDouble(fields, "x", sample.x, error)
        || !readDouble(fields, "y", sample.y, error)
        || !readDouble(fields, "z", sample.z, error)
        || !readDouble(fields, "q1", sample.q1, error)
        || !readDouble(fields, "q2", sample.q2, error)
        || !readDouble(fields, "q3", sample.q3, error)
        || !readDouble(fields, "q4", sample.q4, error)) {
        result.error = error;
        return result;
    }

    for (int i = 0; i < 6; ++i) {
        if (!readDouble(fields, QString("j%1").arg(i + 1), sample.joints[i], error)) {
            result.error = error;
            return result;
        }
        sample.externalAxis[i] = optionalDouble(fields, QString("eax%1").arg(i + 1));
    }

    sample.commandedTcpSpeed = optionalDouble(fields, "speed_tcp");
    sample.commandedOrientationSpeed = optionalDouble(fields, "speed_ori");
    sample.commandedExternalLinearSpeed = optionalDouble(fields, "speed_leax");
    sample.commandedExternalOrientationSpeed = optionalDouble(fields, "speed_reax");

    result.ok = true;
    result.sample = sample;
    return result;
}
```

- [ ] **Step 7: Run parser test**

Run:

```powershell
cmake --build build --config Release --target test_robot_monitor_message_parser
ctest --test-dir build -C Release -R test_robot_monitor_message_parser --output-on-failure
```

Expected: parser test passes.

- [ ] **Step 8: Commit**

Run:

```powershell
git add CMakeLists.txt src/robot_monitor/RobotMonitorSample.h src/robot_monitor/RobotMonitorMessageParser.h src/robot_monitor/RobotMonitorMessageParser.cpp tests/test_robot_monitor_message_parser.cpp
git commit -m "robot: add monitor message parser"
```

---

### Task 3: Add CSV and Raw Message Recorder

**Files:**

- Create: `src/robot_monitor/RobotMonitorRecorder.h`
- Create: `src/robot_monitor/RobotMonitorRecorder.cpp`
- Create: `tests/test_robot_monitor_recorder.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add recorder test target**

Add to `CMakeLists.txt`:

```cmake
add_executable(test_robot_monitor_recorder
    tests/test_robot_monitor_recorder.cpp
    src/robot_monitor/RobotMonitorSample.h
    src/robot_monitor/RobotMonitorRecorder.h
    src/robot_monitor/RobotMonitorRecorder.cpp
)
target_include_directories(test_robot_monitor_recorder PRIVATE src)
target_link_libraries(test_robot_monitor_recorder PRIVATE Qt6::Core Qt6::Test)
copy_qt_test_runtime(test_robot_monitor_recorder)
add_test(NAME test_robot_monitor_recorder COMMAND test_robot_monitor_recorder)
```

- [ ] **Step 2: Write failing recorder tests**

Create `tests/test_robot_monitor_recorder.cpp`:

```cpp
#include <QtTest/QtTest>

#include <QDir>
#include <QTemporaryDir>

#include "robot_monitor/RobotMonitorRecorder.h"

class TestRobotMonitorRecorder final : public QObject {
    Q_OBJECT

private slots:
    void writesCsvHeaderAndSample();
    void writesRawMessagesWhenEnabled();
};

void TestRobotMonitorRecorder::writesCsvHeaderAndSample() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    RobotMonitorRecorder recorder(dir.path(), true, false, true);
    QVERIFY2(recorder.start(), qPrintable(recorder.lastError()));

    RobotMonitorSample sample;
    sample.receivedAtUtc = QDateTime::fromString("2026-06-03T01:02:03.004Z", Qt::ISODateWithMs);
    sample.sequence = 7;
    sample.x = 1.1;
    sample.y = 2.2;
    sample.z = 3.3;
    sample.q1 = 0.1;
    sample.q2 = 0.2;
    sample.q3 = 0.3;
    sample.q4 = 0.4;
    sample.joints = {1, 2, 3, 4, 5, 6};
    sample.externalAxis = {10, 20, 30, 40, 50, 60};
    sample.commandedTcpSpeed = 120.5;
    sample.estimatedTcpSpeed = 99.9;

    QVERIFY(recorder.recordSample(sample));
    recorder.close();

    const QStringList files = QDir(dir.path()).entryList(QStringList() << "*.csv", QDir::Files);
    QCOMPARE(files.size(), 1);

    QFile csv(QDir(dir.path()).filePath(files.first()));
    QVERIFY(csv.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString content = QString::fromUtf8(csv.readAll());
    QVERIFY(content.contains("received_at_utc,seq,x,y,z,q1,q2,q3,q4,j1,j2,j3,j4,j5,j6"));
    QVERIFY(content.contains("2026-06-03T01:02:03.004Z,7,1.1,2.2,3.3"));
}

void TestRobotMonitorRecorder::writesRawMessagesWhenEnabled() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    RobotMonitorRecorder recorder(dir.path(), false, true, true);
    QVERIFY2(recorder.start(), qPrintable(recorder.lastError()));
    QVERIFY(recorder.recordRawMessage("#ROBOT_MONITOR seq=1#"));
    recorder.close();

    const QStringList files = QDir(dir.path()).entryList(QStringList() << "*.raw.log", QDir::Files);
    QCOMPARE(files.size(), 1);

    QFile raw(QDir(dir.path()).filePath(files.first()));
    QVERIFY(raw.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(raw.readAll()).trimmed(), QString("#ROBOT_MONITOR seq=1#"));
}

QTEST_MAIN(TestRobotMonitorRecorder)
#include "test_robot_monitor_recorder.moc"
```

- [ ] **Step 3: Run recorder test and verify it fails**

Run:

```powershell
cmake --build build --config Release --target test_robot_monitor_recorder
```

Expected: fail because recorder files do not exist.

- [ ] **Step 4: Create recorder interface**

Create `src/robot_monitor/RobotMonitorRecorder.h`:

```cpp
#pragma once

#include <utility>

#include <QFile>
#include <QString>
#include <QTextStream>

#include "robot_monitor/RobotMonitorSample.h"

// Writes robot monitor samples to plain files for first-stage hardware
// validation. This deliberately avoids DDS/InfluxDB so socket correctness can
// be confirmed with easy-to-inspect CSV and raw logs.
class RobotMonitorRecorder final {
public:
    RobotMonitorRecorder(QString recordingDir,
                         bool recordCsv,
                         bool recordRawMessages,
                         bool flushEverySample);

    bool start();
    bool recordSample(const RobotMonitorSample& sample);
    bool recordRawMessage(const QString& rawMessage);
    void close();

    QString csvFilePath() const;
    QString rawLogFilePath() const;
    QString lastError() const;

private:
    bool writeCsvHeader();
    QString makeBasePath() const;

    QString recordingDir_;
    bool recordCsv_ = true;
    bool recordRawMessages_ = true;
    bool flushEverySample_ = true;

    QFile csvFile_;
    QFile rawFile_;
    QString lastError_;
};
```

- [ ] **Step 5: Implement recorder**

Create `src/robot_monitor/RobotMonitorRecorder.cpp` with:

```cpp
#include "robot_monitor/RobotMonitorRecorder.h"

#include <QDateTime>
#include <QDir>
#include <QLocale>

RobotMonitorRecorder::RobotMonitorRecorder(QString recordingDir,
                                           bool recordCsv,
                                           bool recordRawMessages,
                                           bool flushEverySample)
    : recordingDir_(std::move(recordingDir)),
      recordCsv_(recordCsv),
      recordRawMessages_(recordRawMessages),
      flushEverySample_(flushEverySample) {
}

bool RobotMonitorRecorder::start() {
    QDir dir(recordingDir_);
    if (!dir.exists() && !dir.mkpath(".")) {
        lastError_ = QString("Could not create robot monitor recording directory: %1").arg(recordingDir_);
        return false;
    }

    const QString base = makeBasePath();
    if (recordCsv_) {
        csvFile_.setFileName(base + ".csv");
        if (!csvFile_.open(QIODevice::WriteOnly | QIODevice::Text)) {
            lastError_ = QString("Could not open robot monitor CSV file: %1").arg(csvFile_.errorString());
            return false;
        }
        if (!writeCsvHeader()) {
            return false;
        }
    }

    if (recordRawMessages_) {
        rawFile_.setFileName(base + ".raw.log");
        if (!rawFile_.open(QIODevice::WriteOnly | QIODevice::Text)) {
            lastError_ = QString("Could not open robot monitor raw log file: %1").arg(rawFile_.errorString());
            return false;
        }
    }

    return true;
}

bool RobotMonitorRecorder::recordSample(const RobotMonitorSample& sample) {
    if (!recordCsv_ || !csvFile_.isOpen()) {
        return true;
    }

    QTextStream out(&csvFile_);
    out << sample.receivedAtUtc.toUTC().toString(Qt::ISODateWithMs) << ','
        << sample.sequence << ','
        << QLocale::c().toString(sample.x, 'g', 15) << ','
        << QLocale::c().toString(sample.y, 'g', 15) << ','
        << QLocale::c().toString(sample.z, 'g', 15) << ','
        << QLocale::c().toString(sample.q1, 'g', 15) << ','
        << QLocale::c().toString(sample.q2, 'g', 15) << ','
        << QLocale::c().toString(sample.q3, 'g', 15) << ','
        << QLocale::c().toString(sample.q4, 'g', 15);

    for (double joint : sample.joints) {
        out << ',' << QLocale::c().toString(joint, 'g', 15);
    }
    for (double axis : sample.externalAxis) {
        out << ',' << QLocale::c().toString(axis, 'g', 15);
    }

    out << ',' << QLocale::c().toString(sample.commandedTcpSpeed, 'g', 15)
        << ',' << QLocale::c().toString(sample.commandedOrientationSpeed, 'g', 15)
        << ',' << QLocale::c().toString(sample.commandedExternalLinearSpeed, 'g', 15)
        << ',' << QLocale::c().toString(sample.commandedExternalOrientationSpeed, 'g', 15)
        << ',' << QLocale::c().toString(sample.estimatedTcpSpeed, 'g', 15)
        << '\n';

    if (flushEverySample_) {
        out.flush();
        csvFile_.flush();
    }
    return true;
}

bool RobotMonitorRecorder::recordRawMessage(const QString& rawMessage) {
    if (!recordRawMessages_ || !rawFile_.isOpen()) {
        return true;
    }
    QTextStream out(&rawFile_);
    out << rawMessage.trimmed() << '\n';
    if (flushEverySample_) {
        out.flush();
        rawFile_.flush();
    }
    return true;
}

void RobotMonitorRecorder::close() {
    if (csvFile_.isOpen()) {
        csvFile_.close();
    }
    if (rawFile_.isOpen()) {
        rawFile_.close();
    }
}

QString RobotMonitorRecorder::csvFilePath() const { return csvFile_.fileName(); }
QString RobotMonitorRecorder::rawLogFilePath() const { return rawFile_.fileName(); }
QString RobotMonitorRecorder::lastError() const { return lastError_; }

bool RobotMonitorRecorder::writeCsvHeader() {
    QTextStream out(&csvFile_);
    out << "received_at_utc,seq,x,y,z,q1,q2,q3,q4,"
        << "j1,j2,j3,j4,j5,j6,"
        << "eax1,eax2,eax3,eax4,eax5,eax6,"
        << "speed_tcp,speed_ori,speed_leax,speed_reax,estimated_tcp_speed\n";
    if (flushEverySample_) {
        out.flush();
        csvFile_.flush();
    }
    return true;
}

QString RobotMonitorRecorder::makeBasePath() const {
    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    return QDir(recordingDir_).filePath("robot_monitor_" + stamp);
}
```

- [ ] **Step 6: Run recorder test**

Run:

```powershell
cmake --build build --config Release --target test_robot_monitor_recorder
ctest --test-dir build -C Release -R test_robot_monitor_recorder --output-on-failure
```

Expected: recorder test passes.

- [ ] **Step 7: Commit**

Run:

```powershell
git add CMakeLists.txt src/robot_monitor/RobotMonitorRecorder.h src/robot_monitor/RobotMonitorRecorder.cpp tests/test_robot_monitor_recorder.cpp
git commit -m "robot: add monitor csv recorder"
```

---

### Task 4: Add Non-Blocking Robot Monitor Socket Client

**Files:**

- Create: `src/robot_monitor/RobotMonitorClient.h`
- Create: `src/robot_monitor/RobotMonitorClient.cpp`
- Create: `tests/test_robot_monitor_client.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add client test target**

Add to `CMakeLists.txt`:

```cmake
add_executable(test_robot_monitor_client
    tests/test_robot_monitor_client.cpp
    src/robot_monitor/RobotMonitorSample.h
    src/robot_monitor/RobotMonitorMessageParser.h
    src/robot_monitor/RobotMonitorMessageParser.cpp
    src/robot_monitor/RobotMonitorClient.h
    src/robot_monitor/RobotMonitorClient.cpp
)
target_include_directories(test_robot_monitor_client PRIVATE src)
target_link_libraries(test_robot_monitor_client PRIVATE Qt6::Core Qt6::Network Qt6::Test)
copy_qt_test_runtime(test_robot_monitor_client)
add_test(NAME test_robot_monitor_client COMMAND test_robot_monitor_client)
```

- [ ] **Step 2: Write failing socket client tests**

Create `tests/test_robot_monitor_client.cpp`:

```cpp
#include <QtTest/QtTest>

#include <QTcpServer>
#include <QTcpSocket>

#include "robot_monitor/RobotMonitorClient.h"

class TestRobotMonitorClient final : public QObject {
    Q_OBJECT

private slots:
    void receivesSplitFrames();
};

void TestRobotMonitorClient::receivesSplitFrames() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    RobotMonitorClient client;
    QList<QString> rawMessages;
    QList<RobotMonitorSample> samples;
    QStringList errors;

    connect(&client, &RobotMonitorClient::rawMessageReceived, this, [&](const QString& raw) {
        rawMessages << raw;
    });
    connect(&client, &RobotMonitorClient::sampleReceived, this, [&](const RobotMonitorSample& sample) {
        samples << sample;
    });
    connect(&client, &RobotMonitorClient::errorReported, this, [&](const QString& error) {
        errors << error;
    });

    client.connectToRobot("127.0.0.1", server.serverPort());
    QVERIFY(QTest::qWaitFor([&]() { return server.hasPendingConnections(); }, 1000));

    QTcpSocket* socket = server.nextPendingConnection();
    QVERIFY(socket != nullptr);
    QVERIFY(QTest::qWaitFor([&]() { return client.isConnected(); }, 1000));

    socket->write("#ROBOT_MONITOR seq=1 x=1 y=2 z=3 q1=1 q2=0 ");
    socket->flush();
    QTest::qWait(20);
    QCOMPARE(samples.size(), 0);

    socket->write("q3=0 q4=0 j1=1 j2=2 j3=3 j4=4 j5=5 j6=6#");
    socket->flush();

    QVERIFY(QTest::qWaitFor([&]() { return samples.size() == 1; }, 1000));
    QCOMPARE(rawMessages.size(), 1);
    QCOMPARE(samples.first().sequence, 1);
    QCOMPARE(errors.size(), 0);
}

QTEST_MAIN(TestRobotMonitorClient)
#include "test_robot_monitor_client.moc"
```

- [ ] **Step 3: Run client test and verify it fails**

Run:

```powershell
cmake --build build --config Release --target test_robot_monitor_client
```

Expected: fail because client files do not exist.

- [ ] **Step 4: Create client interface**

Create `src/robot_monitor/RobotMonitorClient.h`:

```cpp
#pragma once

#include <QObject>
#include <QTcpSocket>

#include "robot_monitor/RobotMonitorSample.h"

// Non-blocking receiver for the RAPID robot monitor socket. It only reads
// '#ROBOT_MONITOR ...#' frames; it does not send motion or print commands.
class RobotMonitorClient final : public QObject {
    Q_OBJECT

public:
    explicit RobotMonitorClient(QObject* parent = nullptr);

    void connectToRobot(const QString& ip, int port);
    void disconnectFromRobot();
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void rawMessageReceived(const QString& rawMessage);
    void sampleReceived(const RobotMonitorSample& sample);
    void parseErrorReported(const QString& rawMessage, const QString& error);
    void errorReported(const QString& error);

private slots:
    void onReadyRead();

private:
    QTcpSocket socket_;
    QByteArray pendingBytes_;
};
```

- [ ] **Step 5: Implement client**

Create `src/robot_monitor/RobotMonitorClient.cpp`:

```cpp
#include "robot_monitor/RobotMonitorClient.h"

#include "robot_monitor/RobotMonitorMessageParser.h"

RobotMonitorClient::RobotMonitorClient(QObject* parent)
    : QObject(parent) {
    connect(&socket_, &QTcpSocket::connected, this, &RobotMonitorClient::connected);
    connect(&socket_, &QTcpSocket::disconnected, this, &RobotMonitorClient::disconnected);
    connect(&socket_, &QTcpSocket::readyRead, this, &RobotMonitorClient::onReadyRead);
    connect(&socket_, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit errorReported(socket_.errorString());
    });
}

void RobotMonitorClient::connectToRobot(const QString& ip, int port) {
    pendingBytes_.clear();
    socket_.abort();
    socket_.connectToHost(ip, static_cast<quint16>(port));
}

void RobotMonitorClient::disconnectFromRobot() {
    socket_.disconnectFromHost();
}

bool RobotMonitorClient::isConnected() const {
    return socket_.state() == QAbstractSocket::ConnectedState;
}

void RobotMonitorClient::onReadyRead() {
    pendingBytes_.append(socket_.readAll());

    while (true) {
        const int startIndex = pendingBytes_.indexOf('#');
        if (startIndex < 0) {
            pendingBytes_.clear();
            return;
        }
        if (startIndex > 0) {
            pendingBytes_.remove(0, startIndex);
        }

        const int endIndex = pendingBytes_.indexOf('#', 1);
        if (endIndex < 0) {
            return;
        }

        const QByteArray frame = pendingBytes_.left(endIndex + 1);
        pendingBytes_.remove(0, endIndex + 1);

        const QString raw = QString::fromUtf8(frame).trimmed();
        emit rawMessageReceived(raw);

        const RobotMonitorParseResult parsed = RobotMonitorMessageParser::parse(raw);
        if (parsed.ok) {
            emit sampleReceived(parsed.sample);
        } else {
            emit parseErrorReported(raw, parsed.error);
        }
    }
}
```

- [ ] **Step 6: Run client test**

Run:

```powershell
cmake --build build --config Release --target test_robot_monitor_client
ctest --test-dir build -C Release -R test_robot_monitor_client --output-on-failure
```

Expected: client test passes.

- [ ] **Step 7: Commit**

Run:

```powershell
git add CMakeLists.txt src/robot_monitor/RobotMonitorClient.h src/robot_monitor/RobotMonitorClient.cpp tests/test_robot_monitor_client.cpp
git commit -m "robot: add monitor socket client"
```

---

### Task 5: Add Robot Monitoring Service and MainWindow Startup

**Files:**

- Create: `src/services/robot_monitor/RobotMonitoringService.h`
- Create: `src/services/robot_monitor/RobotMonitoringService.cpp`
- Create: `tests/test_robot_monitoring_service.cpp`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add service test target**

Add to `CMakeLists.txt`:

```cmake
add_executable(test_robot_monitoring_service
    tests/test_robot_monitoring_service.cpp
    src/app/AppPaths.h
    src/app/AppPaths.cpp
    src/app/AppConfig.h
    src/app/AppConfig.cpp
    src/robot_monitor/RobotMonitorSample.h
    src/robot_monitor/RobotMonitorMessageParser.h
    src/robot_monitor/RobotMonitorMessageParser.cpp
    src/robot_monitor/RobotMonitorRecorder.h
    src/robot_monitor/RobotMonitorRecorder.cpp
    src/robot_monitor/RobotMonitorClient.h
    src/robot_monitor/RobotMonitorClient.cpp
    src/services/robot_monitor/RobotMonitoringService.h
    src/services/robot_monitor/RobotMonitoringService.cpp
)
target_include_directories(test_robot_monitoring_service PRIVATE src)
target_link_libraries(test_robot_monitoring_service PRIVATE Qt6::Core Qt6::Network Qt6::Test)
copy_qt_test_runtime(test_robot_monitoring_service)
add_test(NAME test_robot_monitoring_service COMMAND test_robot_monitoring_service)
```

- [ ] **Step 2: Write service test**

Create `tests/test_robot_monitoring_service.cpp`:

```cpp
#include <QtTest/QtTest>

#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>

#include "services/robot_monitor/RobotMonitoringService.h"

class TestRobotMonitoringService final : public QObject {
    Q_OBJECT

private slots:
    void disabledServiceDoesNotConnect();
    void autostartRecordsIncomingSample();
};

void TestRobotMonitoringService::disabledServiceDoesNotConnect() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    AppConfig config;
    config.robot.monitor.enabled = false;
    config.robot.monitor.autostart = true;

    RobotMonitoringService service(config, AppPaths(dir.path()));
    service.start();

    QVERIFY(!service.isRunning());
}

void TestRobotMonitoringService::autostartRecordsIncomingSample() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    AppConfig config;
    config.robot.ip = "127.0.0.1";
    config.robot.monitor.enabled = true;
    config.robot.monitor.autostart = true;
    config.robot.monitor.port = server.serverPort();
    config.robot.monitor.recordingDir = "logs/robot_monitor";
    config.robot.monitor.recordCsv = true;
    config.robot.monitor.recordRawMessages = true;
    config.robot.monitor.flushEverySample = true;

    RobotMonitoringService service(config, AppPaths(dir.path()));
    service.start();

    QVERIFY(QTest::qWaitFor([&]() { return server.hasPendingConnections(); }, 1000));
    QTcpSocket* socket = server.nextPendingConnection();
    QVERIFY(socket != nullptr);

    socket->write("#ROBOT_MONITOR seq=1 x=1 y=2 z=3 q1=1 q2=0 q3=0 q4=0 j1=1 j2=2 j3=3 j4=4 j5=5 j6=6#");
    socket->flush();

    const QString recordingDir = QDir(dir.path()).filePath("logs/robot_monitor");
    QVERIFY(QTest::qWaitFor([&]() {
        return !QDir(recordingDir).entryList(QStringList() << "*.csv", QDir::Files).isEmpty();
    }, 1000));
}

QTEST_MAIN(TestRobotMonitoringService)
#include "test_robot_monitoring_service.moc"
```

- [ ] **Step 3: Run service test and verify it fails**

Run:

```powershell
cmake --build build --config Release --target test_robot_monitoring_service
```

Expected: fail because service files do not exist.

- [ ] **Step 4: Create service interface**

Create `src/services/robot_monitor/RobotMonitoringService.h`:

```cpp
#pragma once

#include <QObject>

#include <memory>
#include <optional>

#include "app/AppConfig.h"
#include "app/AppPaths.h"
#include "robot_monitor/RobotMonitorClient.h"
#include "robot_monitor/RobotMonitorRecorder.h"

// Starts and owns the optional robot monitoring workflow. This service is
// intentionally independent of ConnectionRobotButton and existing robot command
// services so monitoring cannot change print or movement behavior.
class RobotMonitoringService final : public QObject {
    Q_OBJECT

public:
    RobotMonitoringService(const AppConfig& config, const AppPaths& paths, QObject* parent = nullptr);
    ~RobotMonitoringService() override;

    bool isRunning() const;

signals:
    void statusChanged(const QString& status);

public slots:
    void start();
    void stop();

private:
    QString recordingDirectoryPath() const;
    void updateEstimatedSpeed(RobotMonitorSample& sample);

    AppConfig config_;
    AppPaths paths_;
    RobotMonitorClient client_;
    std::unique_ptr<RobotMonitorRecorder> recorder_;
    std::optional<RobotMonitorSample> previousSample_;
    bool running_ = false;
};
```

- [ ] **Step 5: Implement service**

Create `src/services/robot_monitor/RobotMonitoringService.cpp`:

```cpp
#include "services/robot_monitor/RobotMonitoringService.h"

#include <cmath>

#include <QDir>

RobotMonitoringService::RobotMonitoringService(const AppConfig& config, const AppPaths& paths, QObject* parent)
    : QObject(parent),
      config_(config),
      paths_(paths) {
    connect(&client_, &RobotMonitorClient::connected, this, [this]() {
        running_ = true;
        emit statusChanged("Robot monitor connected.");
    });
    connect(&client_, &RobotMonitorClient::disconnected, this, [this]() {
        running_ = false;
        emit statusChanged("Robot monitor disconnected.");
    });
    connect(&client_, &RobotMonitorClient::errorReported, this, [this](const QString& error) {
        emit statusChanged(QString("Robot monitor socket error: %1").arg(error));
    });
    connect(&client_, &RobotMonitorClient::rawMessageReceived, this, [this](const QString& raw) {
        if (recorder_) {
            recorder_->recordRawMessage(raw);
        }
    });
    connect(&client_, &RobotMonitorClient::parseErrorReported, this, [this](const QString& raw, const QString& error) {
        emit statusChanged(QString("Robot monitor parse error: %1 | raw=%2").arg(error, raw));
    });
    connect(&client_, &RobotMonitorClient::sampleReceived, this, [this](RobotMonitorSample sample) {
        updateEstimatedSpeed(sample);
        if (recorder_) {
            recorder_->recordSample(sample);
        }
    });
}

RobotMonitoringService::~RobotMonitoringService() {
    stop();
}

bool RobotMonitoringService::isRunning() const {
    return running_;
}

void RobotMonitoringService::start() {
    if (!config_.robot.monitor.enabled || !config_.robot.monitor.autostart) {
        return;
    }
    if (config_.robot.ip.trimmed().isEmpty()) {
        emit statusChanged("Robot monitor disabled: robot.ip is empty.");
        return;
    }

    recorder_ = std::make_unique<RobotMonitorRecorder>(
        recordingDirectoryPath(),
        config_.robot.monitor.recordCsv,
        config_.robot.monitor.recordRawMessages,
        config_.robot.monitor.flushEverySample);

    if (!recorder_->start()) {
        emit statusChanged(QString("Robot monitor recorder warning: %1").arg(recorder_->lastError()));
        recorder_.reset();
    }

    emit statusChanged(QString("Robot monitor connecting to %1:%2.")
        .arg(config_.robot.ip)
        .arg(config_.robot.monitor.port));
    client_.connectToRobot(config_.robot.ip, config_.robot.monitor.port);
}

void RobotMonitoringService::stop() {
    client_.disconnectFromRobot();
    if (recorder_) {
        recorder_->close();
        recorder_.reset();
    }
    running_ = false;
}

QString RobotMonitoringService::recordingDirectoryPath() const {
    QDir configured(config_.robot.monitor.recordingDir);
    if (configured.isAbsolute()) {
        return configured.absolutePath();
    }
    return QDir(paths_.rootDir().absolutePath()).filePath(config_.robot.monitor.recordingDir);
}

void RobotMonitoringService::updateEstimatedSpeed(RobotMonitorSample& sample) {
    if (!previousSample_) {
        previousSample_ = sample;
        sample.estimatedTcpSpeed = 0.0;
        return;
    }

    const double seconds = previousSample_->receivedAtUtc.msecsTo(sample.receivedAtUtc) / 1000.0;
    if (seconds > 0.0) {
        const double dx = sample.x - previousSample_->x;
        const double dy = sample.y - previousSample_->y;
        const double dz = sample.z - previousSample_->z;
        sample.estimatedTcpSpeed = std::sqrt(dx * dx + dy * dy + dz * dz) / seconds;
    }
    previousSample_ = sample;
}
```

- [ ] **Step 6: Wire service into MainWindow**

In `src/ui/MainWindow.h`, include and add a member:

```cpp
#include "services/robot_monitor/RobotMonitoringService.h"
```

```cpp
RobotMonitoringService robotMonitoringService_;
```

In `src/ui/MainWindow.cpp`, initialize it after other background services:

```cpp
robotMonitoringService_(config_, paths_, this),
```

Connect status in `connectSignals()`:

```cpp
connect(&robotMonitoringService_, &RobotMonitoringService::statusChanged,
        this, [this](const QString& status) {
            const QString existingTip = ui_->robot_state_show->toolTip();
            ui_->robot_state_show->setToolTip(
                existingTip.isEmpty() ? status : existingTip + "\n" + status);
        });
```

Start from `startBackgroundServices()`:

```cpp
if (config_.robot.monitor.enabled && config_.robot.monitor.autostart) {
    robotMonitoringService_.start();
}
```

- [ ] **Step 7: Add app sources to CMake**

Add these to `APP_SOURCES`:

```cmake
src/robot_monitor/RobotMonitorSample.h
src/robot_monitor/RobotMonitorMessageParser.h
src/robot_monitor/RobotMonitorMessageParser.cpp
src/robot_monitor/RobotMonitorRecorder.h
src/robot_monitor/RobotMonitorRecorder.cpp
src/robot_monitor/RobotMonitorClient.h
src/robot_monitor/RobotMonitorClient.cpp
src/services/robot_monitor/RobotMonitoringService.h
src/services/robot_monitor/RobotMonitoringService.cpp
```

- [ ] **Step 8: Run service test**

Run:

```powershell
cmake --build build --config Release --target test_robot_monitoring_service
ctest --test-dir build -C Release -R test_robot_monitoring_service --output-on-failure
```

Expected: service test passes.

- [ ] **Step 9: Build main app**

Run:

```powershell
cmake --build build --config Release --target AFP_ABB_CPP
```

Expected: build succeeds.

- [ ] **Step 10: Commit**

Run:

```powershell
git add CMakeLists.txt src/ui/MainWindow.h src/ui/MainWindow.cpp src/services/robot_monitor src/robot_monitor tests/test_robot_monitoring_service.cpp
git commit -m "robot: autostart monitor service from config"
```

---

### Task 6: Add Minimal RAPID Monitor Stream in Copied RAPID Files

**Files:**

- Modify: `rapid/RAPID/TASK2/PROGMOD/UPLOAD_ROBOT.mod`
- Modify if required: `rapid/RAPID/TASK1/PROGMOD/Motion.mod`
- Modify only if required: `rapid/RAPID/TASK3/PROGMOD/PLC_ROBOT_COMM_GI_GO.mod`
- Modify only if helpful: `rapid/HOME/testmod.mod`

- [ ] **Step 1: Add TASK2 monitor state**

In `rapid/RAPID/TASK2/PROGMOD/UPLOAD_ROBOT.mod`, near existing upload globals, add clearly marked monitor variables:

```rapid
      ! ===== Robot monitor stream (C++ Slice K) =====
      PERS bool monitor_enabled:=TRUE;
      PERS num monitor_seq:=0;
      PERS num monitor_wait_time:=0.1;
      VAR robtarget monitor_pose;
      VAR jointtarget monitor_joints;
      VAR string monitor_msg;
```

- [ ] **Step 2: Add TASK2 monitor message builder**

Add this procedure before `main()`:

```rapid
      ! Builds one compact key=value monitor message for the C++ parser.
      PROC BuildRobotMonitorMessage()
            monitor_seq:=monitor_seq+1;
            monitor_pose:=CRobT();
            monitor_joints:=CJointT();

            monitor_msg:="ROBOT_MONITOR seq="+NumToStr(monitor_seq,0);
            monitor_msg:=monitor_msg+" x="+NumToStr(monitor_pose.trans.x,3);
            monitor_msg:=monitor_msg+" y="+NumToStr(monitor_pose.trans.y,3);
            monitor_msg:=monitor_msg+" z="+NumToStr(monitor_pose.trans.z,3);
            monitor_msg:=monitor_msg+" q1="+NumToStr(monitor_pose.rot.q1,6);
            monitor_msg:=monitor_msg+" q2="+NumToStr(monitor_pose.rot.q2,6);
            monitor_msg:=monitor_msg+" q3="+NumToStr(monitor_pose.rot.q3,6);
            monitor_msg:=monitor_msg+" q4="+NumToStr(monitor_pose.rot.q4,6);
            monitor_msg:=monitor_msg+" j1="+NumToStr(monitor_joints.robax.rax_1,3);
            monitor_msg:=monitor_msg+" j2="+NumToStr(monitor_joints.robax.rax_2,3);
            monitor_msg:=monitor_msg+" j3="+NumToStr(monitor_joints.robax.rax_3,3);
            monitor_msg:=monitor_msg+" j4="+NumToStr(monitor_joints.robax.rax_4,3);
            monitor_msg:=monitor_msg+" j5="+NumToStr(monitor_joints.robax.rax_5,3);
            monitor_msg:=monitor_msg+" j6="+NumToStr(monitor_joints.robax.rax_6,3);
            monitor_msg:=monitor_msg+" eax1="+NumToStr(monitor_joints.extax.eax_a,3);
            monitor_msg:=monitor_msg+" eax2="+NumToStr(monitor_joints.extax.eax_b,3);
            monitor_msg:=monitor_msg+" eax3="+NumToStr(monitor_joints.extax.eax_c,3);
            monitor_msg:=monitor_msg+" eax4="+NumToStr(monitor_joints.extax.eax_d,3);
            monitor_msg:=monitor_msg+" eax5="+NumToStr(monitor_joints.extax.eax_e,3);
            monitor_msg:=monitor_msg+" eax6="+NumToStr(monitor_joints.extax.eax_f,3);
            monitor_msg:=monitor_msg+" speed_tcp="+NumToStr(currentSpeed.v_tcp,3);
            monitor_msg:=monitor_msg+" speed_ori="+NumToStr(currentSpeed.v_ori,3);
            monitor_msg:=monitor_msg+" speed_leax="+NumToStr(currentSpeed.v_leax,3);
            monitor_msg:=monitor_msg+" speed_reax="+NumToStr(currentSpeed.v_reax,3);
      ENDPROC
```

- [ ] **Step 3: Add TASK2 monitor sender**

Add:

```rapid
      ! Sends one monitor frame when the PC monitor client is connected.
      PROC SendRobotMonitorMessage()
            IF connected=TRUE AND monitor_enabled=TRUE THEN
                  BuildRobotMonitorMessage;
                  SocketSend clientSocket_upload\Str:="#"+monitor_msg+"#";
            ENDIF
      ENDPROC
```

- [ ] **Step 4: Call monitor sender from TASK2 main loop**

In `TASK2` `main()`, change the infinite loop to:

```rapid
            WHILE TRUE DO
                  IF cmdReady=TRUE THEN
                        ProcessCommand;
                  ENDIF
                  SendRobotMonitorMessage;
                  WaitTime monitor_wait_time;
            ENDWHILE
```

- [ ] **Step 5: Validate RAPID controller behavior**

Load the copied RAPID changes into RobotStudio or the target controller.

Expected:

- If `CRobT()` and `CJointT()` compile and run in TASK2, keep the TASK2-only implementation.
- If the controller rejects pose reads from TASK2, revert TASK2 direct pose reads and implement a TASK1 snapshot.

- [ ] **Step 6: TASK1 snapshot fallback only if TASK2 pose read is rejected**

If needed, add to `rapid/RAPID/TASK1/PROGMOD/Motion.mod` globals:

```rapid
      ! ===== Robot monitor snapshot for TASK2 (C++ Slice K fallback) =====
      PERS robtarget monitor_pose_snapshot;
      PERS jointtarget monitor_joints_snapshot;
      PERS speeddata monitor_speed_snapshot;
      PERS num monitor_snapshot_seq:=0;
```

Add to TASK1 server loop near the top of `WHILE TRUE DO`:

```rapid
                  monitor_pose_snapshot:=CRobT(\Tool:=currentTool\WObj:=currentWobj);
                  monitor_joints_snapshot:=CJointT();
                  monitor_speed_snapshot:=currentSpeed;
                  monitor_snapshot_seq:=monitor_snapshot_seq+1;
```

Then update TASK2 `BuildRobotMonitorMessage()` to read the snapshot variables instead of calling `CRobT()` and `CJointT()` directly.

- [ ] **Step 7: Avoid TASK3 changes unless needed**

Do not edit `TASK3` unless hardware testing shows state/progress must be included in monitor messages. If state/progress is needed, add fields only after the first position/speed CSV is confirmed.

- [ ] **Step 8: Optional testmod note**

If useful for operators, add a short comment near `PROC main_task()` in `rapid/HOME/testmod.mod`:

```rapid
      ! Robot monitoring is handled by TASK2 UPLOAD_ROBOT.mod and does not
      ! require calls inside this sample path program.
```

- [ ] **Step 9: Commit RAPID changes**

Run:

```powershell
git add rapid/RAPID/TASK2/PROGMOD/UPLOAD_ROBOT.mod rapid/RAPID/TASK1/PROGMOD/Motion.mod rapid/RAPID/TASK3/PROGMOD/PLC_ROBOT_COMM_GI_GO.mod rapid/HOME/testmod.mod
git commit -m "rapid: add robot monitor stream"
```

If TASK1, TASK3, or `testmod.mod` were not edited, leave them out of `git add`.

---

### Task 7: Documentation and Full Verification

**Files:**

- Modify: `README.md`
- Modify if needed: `docs/design/2026-06-03-afp-abb-cpp-slice-k-robot-monitoring-design.md`

- [ ] **Step 1: Update README slice summary**

Add to the top slice list:

```markdown
Slice K adds an optional ABB robot monitoring socket workflow. When enabled by `robot.monitor`, the main app starts a separate non-blocking monitor client, receives RAPID `ROBOT_MONITOR` frames, and records parsed samples to CSV plus optional raw socket logs.
```

- [ ] **Step 2: Document config**

Add a `Robot Monitoring` section:

````markdown
## Robot Monitoring

Robot monitoring is independent of `ConnectionRobotButton`. It starts only from config:

```json
"robot": {
  "ip": "192.168.0.104",
  "port_motion": 5010,
  "monitor": {
    "enabled": true,
    "autostart": true,
    "port": 5012,
    "sample_interval_ms": 100,
    "record_csv": true,
    "record_raw_messages": true,
    "recording_dir": "logs/robot_monitor",
    "flush_every_sample": true
  }
}
```

CSV files are written to `logs\robot_monitor\robot_monitor_YYYYMMDD_HHMMSS.csv`.
Raw socket logs are written to `logs\robot_monitor\robot_monitor_YYYYMMDD_HHMMSS.raw.log` when `record_raw_messages` is true.

The raw log stores the exact RAPID text received before parsing. Use it to diagnose whether a bad value came from RAPID or from C++ parsing.
````

- [ ] **Step 3: Document RAPID location**

Add:

````markdown
## RAPID Files

The C++ repository includes a copied RAPID backup in:

```text
rapid
```

The original Python RAPID backup remains untouched. Slice K changes, if any, are made only under `AFP_ABB_CPP\rapid`.
````

- [ ] **Step 4: Run targeted tests**

Run:

```powershell
ctest --test-dir build -C Release -R "test_app_config|test_robot_monitor_message_parser|test_robot_monitor_recorder|test_robot_monitor_client|test_robot_monitoring_service" --output-on-failure
```

Expected: all targeted tests pass.

- [ ] **Step 5: Run full test suite**

Run:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 6: Build main app**

Run:

```powershell
cmake --build build --config Release --target AFP_ABB_CPP
```

Expected: `build\Release\AFP_ABB_CPP.exe` builds successfully.

- [ ] **Step 7: Build portable deployment**

Run:

```powershell
cmake --build build --target deploy_portable --config Release
```

Expected: deployment succeeds and `build\Release\config\env_config.json` includes `robot.monitor`.

- [ ] **Step 8: Commit docs and verification fixes**

Run:

```powershell
git add README.md docs/design/2026-06-03-afp-abb-cpp-slice-k-robot-monitoring-design.md
git commit -m "docs: describe robot monitor workflow"
```

---

## Manual Hardware Validation Checklist

- [ ] Copy the rebuilt portable `build\Release` folder to the robot PC if needed.
- [ ] Confirm Windows Firewall allows `AFP_ABB_CPP.exe` on the robot network.
- [ ] Confirm `config\env_config.json` has the correct `robot.ip` and `robot.monitor.port`.
- [ ] Load/start the edited copied RAPID modules on the controller.
- [ ] Start `AFP_ABB_CPP.exe`.
- [ ] Confirm GUI opens without waiting for robot monitoring.
- [ ] Confirm `ConnectionRobotButton` behavior is unchanged.
- [ ] Confirm CSV file appears under `logs\robot_monitor`.
- [ ] Confirm raw log appears when `record_raw_messages` is true.
- [ ] Move the robot slowly.
- [ ] Compare CSV `x,y,z` and joints against teach pendant values.
- [ ] Check `estimated_tcp_speed` is near zero while stationary and positive while moving.

## Self-Review Checklist

- Spec coverage:
  - Config-only autostart: Task 1 and Task 5.
  - Separate C++ code: Tasks 2 through 5.
  - CSV/raw validation path: Task 3 and Task 7.
  - No `ConnectionRobotButton` behavior change: Task 5 and manual checklist.
  - RAPID copy/edit boundary: Task 6.
  - Future DDS/InfluxDB path preserved: Task 7 README text.

- Marker scan:
  - The plan has no unresolved marker text or unspecified edge handling.
  - Controller-specific RAPID uncertainty is handled by a concrete TASK2-first path and TASK1 fallback path.

- Type consistency:
  - `RobotMonitorConfig` field names match JSON keys and service usage.
  - `RobotMonitorSample` field names match parser, recorder, and tests.
  - `RobotMonitoringService` uses `AppConfig`, `AppPaths`, `RobotMonitorClient`, and `RobotMonitorRecorder` only.
