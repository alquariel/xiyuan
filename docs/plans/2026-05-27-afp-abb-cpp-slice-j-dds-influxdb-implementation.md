# AFP ABB C++ Slice J DDS InfluxDB Archive Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an optional Fast DDS pressure subscriber that starts with `AFP_ABB_CPP.exe` and archives pressure samples to InfluxDB 3 Core.

**Architecture:** Keep the database archive path in a separate `dds_influx_subscriber.exe` process. The Qt main app owns startup orchestration: optionally start `influxdb3.exe serve`, ensure the database exists through the `influxdb3` CLI, then launch the subscriber. The subscriber converts DDS samples into generic time-series points and writes batched line protocol through HTTP.

**Tech Stack:** C++17, Qt6 Core/Network/Widgets/Test, eProsima Fast DDS generated pressure types, InfluxDB 3 Core CLI and HTTP line-protocol write endpoint, CMake.

---

## Design Reference

Implement this plan against:

- `docs/design/2026-05-27-afp-abb-cpp-slice-j-dds-influxdb-archive-design.md`

Do not modify `AFP_ABB_Python`.

## File Structure

Create these focused files:

- `src/archive/TimeSeriesPoint.h`: backend-neutral point model and line-protocol escaping helpers.
- `src/archive/TimeSeriesPoint.cpp`: line-protocol formatting implementation.
- `src/archive/InfluxAdmin.h`: token parsing and CLI command construction for database ensure.
- `src/archive/InfluxAdmin.cpp`: admin token JSON parsing, `show databases`, `create database`, and server reachability checks.
- `src/archive/InfluxLineProtocolWriter.h`: buffered HTTP writer interface.
- `src/archive/InfluxLineProtocolWriter.cpp`: batch/interval flushing to InfluxDB HTTP write endpoint.
- `src/archive/PressureSampleReader.h`: DDS pressure subscriber interface.
- `src/archive/PressureSampleReader.cpp`: Fast DDS `DataReader` implementation for `AfpPressureSample`.
- `src/archive/DdsInfluxSubscriberApp.h`: command-line subscriber app coordinator.
- `src/archive/DdsInfluxSubscriberApp.cpp`: reader/writer wiring and event loop control.
- `src/archive/main_dds_influx_subscriber.cpp`: `dds_influx_subscriber.exe` entry point.
- `src/services/archive/TimeseriesArchiveService.h`: Qt-main-process startup service.
- `src/services/archive/TimeseriesArchiveService.cpp`: `QProcess` orchestration for InfluxDB and subscriber.

Modify these files:

- `src/app/AppConfig.h`: add `TimeseriesArchiveConfig` and `PressureArchiveConfig`.
- `src/app/AppConfig.cpp`: parse and validate the new config sections.
- `config/env_config.json`: add default `timeseries_archive` and `pressure_archive` sections.
- `src/ui/MainWindow.h`: own `TimeseriesArchiveService`.
- `src/ui/MainWindow.cpp`: construct/start the archive service without changing the UI design.
- `src/main.cpp`: no behavior change unless shutdown needs one extra cleanup hook; prefer keeping it unchanged.
- `CMakeLists.txt`: add archive library, subscriber executable, tests, and portable copy behavior.
- `README.md`: document token generation, config, manual startup, automatic startup, and verification.

Create these tests:

- `tests/test_influx_token.cpp`
- `tests/test_time_series_point.cpp`
- `tests/test_influx_line_protocol_writer.cpp`
- `tests/test_timeseries_archive_service.cpp`

---

### Task 1: Config Model And Defaults

**Files:**
- Modify: `src/app/AppConfig.h`
- Modify: `src/app/AppConfig.cpp`
- Modify: `config/env_config.json`
- Test: `tests/test_app_config.cpp`

- [ ] **Step 1: Add failing config tests**

Add private slots to `tests/test_app_config.cpp`:

```cpp
void loadsTimeseriesArchiveConfig();
void invalidTimeseriesArchiveConfigFallsBackToDefaults();
void loadsPressureArchiveConfig();
void invalidPressureArchiveConfigFallsBackToDefaults();
```

Add this test body for valid archive config:

```cpp
void TestAppConfig::loadsTimeseriesArchiveConfig() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "timeseries_archive": {
            "enabled": true,
            "start_influxdb": true,
            "influxdb_exe_path": "D:/InfluxDB 3 Core/influxdb3.exe",
            "host_url": "http://127.0.0.1:8181",
            "node_id": "afp-node-test",
            "object_store": "file",
            "data_dir": "D:/InfluxDB 3 Core/test-data",
            "admin_token_file": "D:/InfluxDB 3 Core/afp-admin-token.json",
            "database": "afp_process_test",
            "retention_period": "7d",
            "ensure_database_on_start": true,
            "subscriber_autostart": true,
            "subscriber_show_console": false,
            "subscriber_print_live_data": false,
            "write_batch_size": 10,
            "write_flush_interval_ms": 250
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QVERIFY(config.timeseriesArchive.enabled);
    QVERIFY(config.timeseriesArchive.startInfluxdb);
    QCOMPARE(config.timeseriesArchive.influxdbExePath, QString("D:/InfluxDB 3 Core/influxdb3.exe"));
    QCOMPARE(config.timeseriesArchive.hostUrl, QString("http://127.0.0.1:8181"));
    QCOMPARE(config.timeseriesArchive.nodeId, QString("afp-node-test"));
    QCOMPARE(config.timeseriesArchive.objectStore, QString("file"));
    QCOMPARE(config.timeseriesArchive.dataDir, QString("D:/InfluxDB 3 Core/test-data"));
    QCOMPARE(config.timeseriesArchive.adminTokenFile, QString("D:/InfluxDB 3 Core/afp-admin-token.json"));
    QCOMPARE(config.timeseriesArchive.database, QString("afp_process_test"));
    QCOMPARE(config.timeseriesArchive.retentionPeriod, QString("7d"));
    QVERIFY(config.timeseriesArchive.ensureDatabaseOnStart);
    QVERIFY(config.timeseriesArchive.subscriberAutostart);
    QVERIFY(!config.timeseriesArchive.subscriberShowConsole);
    QVERIFY(!config.timeseriesArchive.subscriberPrintLiveData);
    QCOMPARE(config.timeseriesArchive.writeBatchSize, 10);
    QCOMPARE(config.timeseriesArchive.writeFlushIntervalMs, 250);
}
```

Add valid pressure archive config:

```cpp
void TestAppConfig::loadsPressureArchiveConfig() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "pressure_archive": {
            "enabled": true,
            "dds_domain_id": 2,
            "dds_topic": "afp.test.pressure",
            "table": "pressure_sensor_test",
            "source_tag": "pressure_sensor_lab"
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QVERIFY(config.pressureArchive.enabled);
    QCOMPARE(config.pressureArchive.ddsDomainId, 2);
    QCOMPARE(config.pressureArchive.ddsTopic, QString("afp.test.pressure"));
    QCOMPARE(config.pressureArchive.table, QString("pressure_sensor_test"));
    QCOMPARE(config.pressureArchive.sourceTag, QString("pressure_sensor_lab"));
}
```

Add invalid-config tests that assert defaults:

```cpp
void TestAppConfig::invalidTimeseriesArchiveConfigFallsBackToDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "timeseries_archive": {
            "host_url": "",
            "node_id": "",
            "object_store": "",
            "database": "",
            "admin_token_file": "",
            "write_batch_size": 0,
            "write_flush_interval_ms": -1
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QCOMPARE(config.timeseriesArchive.hostUrl, QString("http://localhost:8181"));
    QCOMPARE(config.timeseriesArchive.nodeId, QString("afp-node"));
    QCOMPARE(config.timeseriesArchive.objectStore, QString("file"));
    QCOMPARE(config.timeseriesArchive.database, QString("afp_process"));
    QCOMPARE(config.timeseriesArchive.adminTokenFile, QString("D:/InfluxDB 3 Core/afp-admin-token.json"));
    QCOMPARE(config.timeseriesArchive.writeBatchSize, 10);
    QCOMPARE(config.timeseriesArchive.writeFlushIntervalMs, 1000);
    const QString warnings = config.warnings().join('\n');
    QVERIFY(warnings.contains("timeseries_archive.host_url"));
    QVERIFY(warnings.contains("timeseries_archive.write_batch_size"));
}
```

```cpp
void TestAppConfig::invalidPressureArchiveConfigFallsBackToDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "pressure_archive": {
            "dds_domain_id": -2,
            "dds_topic": "",
            "table": "",
            "source_tag": ""
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QCOMPARE(config.pressureArchive.ddsDomainId, 0);
    QCOMPARE(config.pressureArchive.ddsTopic, QString("afp.pressure.sample"));
    QCOMPARE(config.pressureArchive.table, QString("pressure_sensor"));
    QCOMPARE(config.pressureArchive.sourceTag, QString("pressure_sensor_1"));
    const QString warnings = config.warnings().join('\n');
    QVERIFY(warnings.contains("pressure_archive.dds_domain_id"));
    QVERIFY(warnings.contains("pressure_archive.dds_topic"));
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake --build build --config Release --target test_app_config
ctest --test-dir build -C Release -R test_app_config --output-on-failure
```

Expected: compile failure because `AppConfig` does not have `timeseriesArchive` or `pressureArchive`.

- [ ] **Step 3: Add config structs**

Add to `src/app/AppConfig.h` after `PressureConfig`:

```cpp
struct TimeseriesArchiveConfig {
    // Master switch for the optional InfluxDB archive feature.
    bool enabled = false;

    // When true, the main app may start influxdb3.exe if hostUrl is not reachable.
    bool startInfluxdb = false;

    // Configurable because InfluxDB is installed outside the portable AFP folder.
    QString influxdbExePath =
        "D:/InfluxDB 3 Core/influxdb3-core-3.9.2-windows_amd64/influxdb3.exe";
    QString hostUrl = "http://localhost:8181";
    QString nodeId = "afp-node";
    QString objectStore = "file";
    QString dataDir = "D:/InfluxDB 3 Core/data";
    QString adminTokenFile = "D:/InfluxDB 3 Core/afp-admin-token.json";
    QString database = "afp_process";
    QString retentionPeriod = "30d";

    // Creates the database only when it is missing. Existing data is not replaced.
    bool ensureDatabaseOnStart = true;

    // Controls whether AFP_ABB_CPP.exe starts dds_influx_subscriber.exe.
    bool subscriberAutostart = true;
    bool subscriberShowConsole = true;
    bool subscriberPrintLiveData = true;

    // Low-latency defaults for pressure data. At 100 ms/sample, 10 points is about 1 second.
    int writeBatchSize = 10;
    int writeFlushIntervalMs = 1000;
};

struct PressureArchiveConfig {
    bool enabled = true;
    int ddsDomainId = 0;
    QString ddsTopic = "afp.pressure.sample";
    QString table = "pressure_sensor";
    QString sourceTag = "pressure_sensor_1";
};
```

Add members to `class AppConfig`:

```cpp
TimeseriesArchiveConfig timeseriesArchive;
PressureArchiveConfig pressureArchive;
```

- [ ] **Step 4: Add boolean reader helper**

Add to `src/app/AppConfig.cpp` anonymous namespace:

```cpp
bool readBool(const QJsonObject& object, const QString& key, bool fallback) {
    const QJsonValue value = object.value(key);
    return value.isBool() ? value.toBool() : fallback;
}
```

- [ ] **Step 5: Parse config sections**

In `AppConfig::load`, after pressure parsing, add:

```cpp
const QJsonObject timeseriesArchive = root.value("timeseries_archive").toObject();
config.timeseriesArchive.enabled = readBool(
    timeseriesArchive, "enabled", config.timeseriesArchive.enabled);
config.timeseriesArchive.startInfluxdb = readBool(
    timeseriesArchive, "start_influxdb", config.timeseriesArchive.startInfluxdb);
config.timeseriesArchive.influxdbExePath = readNonEmptyString(
    config,
    timeseriesArchive,
    "influxdb_exe_path",
    config.timeseriesArchive.influxdbExePath,
    "timeseries_archive.influxdb_exe_path");
config.timeseriesArchive.hostUrl = readNonEmptyString(
    config, timeseriesArchive, "host_url", config.timeseriesArchive.hostUrl, "timeseries_archive.host_url");
config.timeseriesArchive.nodeId = readNonEmptyString(
    config, timeseriesArchive, "node_id", config.timeseriesArchive.nodeId, "timeseries_archive.node_id");
config.timeseriesArchive.objectStore = readNonEmptyString(
    config, timeseriesArchive, "object_store", config.timeseriesArchive.objectStore, "timeseries_archive.object_store");
config.timeseriesArchive.dataDir = readNonEmptyString(
    config, timeseriesArchive, "data_dir", config.timeseriesArchive.dataDir, "timeseries_archive.data_dir");
config.timeseriesArchive.adminTokenFile = readNonEmptyString(
    config, timeseriesArchive, "admin_token_file", config.timeseriesArchive.adminTokenFile, "timeseries_archive.admin_token_file");
config.timeseriesArchive.database = readNonEmptyString(
    config, timeseriesArchive, "database", config.timeseriesArchive.database, "timeseries_archive.database");
config.timeseriesArchive.retentionPeriod = readNonEmptyString(
    config, timeseriesArchive, "retention_period", config.timeseriesArchive.retentionPeriod, "timeseries_archive.retention_period");
config.timeseriesArchive.ensureDatabaseOnStart = readBool(
    timeseriesArchive, "ensure_database_on_start", config.timeseriesArchive.ensureDatabaseOnStart);
config.timeseriesArchive.subscriberAutostart = readBool(
    timeseriesArchive, "subscriber_autostart", config.timeseriesArchive.subscriberAutostart);
config.timeseriesArchive.subscriberShowConsole = readBool(
    timeseriesArchive, "subscriber_show_console", config.timeseriesArchive.subscriberShowConsole);
config.timeseriesArchive.subscriberPrintLiveData = readBool(
    timeseriesArchive, "subscriber_print_live_data", config.timeseriesArchive.subscriberPrintLiveData);
config.timeseriesArchive.writeBatchSize = readPositiveInt(
    config, timeseriesArchive, "write_batch_size", config.timeseriesArchive.writeBatchSize, "timeseries_archive.write_batch_size");
config.timeseriesArchive.writeFlushIntervalMs = readPositiveInt(
    config,
    timeseriesArchive,
    "write_flush_interval_ms",
    config.timeseriesArchive.writeFlushIntervalMs,
    "timeseries_archive.write_flush_interval_ms");

const QJsonObject pressureArchive = root.value("pressure_archive").toObject();
config.pressureArchive.enabled = readBool(pressureArchive, "enabled", config.pressureArchive.enabled);
config.pressureArchive.ddsDomainId = readNonNegativeInt(
    config, pressureArchive, "dds_domain_id", config.pressureArchive.ddsDomainId, "pressure_archive.dds_domain_id");
config.pressureArchive.ddsTopic = readNonEmptyString(
    config, pressureArchive, "dds_topic", config.pressureArchive.ddsTopic, "pressure_archive.dds_topic");
config.pressureArchive.table = readNonEmptyString(
    config, pressureArchive, "table", config.pressureArchive.table, "pressure_archive.table");
config.pressureArchive.sourceTag = readNonEmptyString(
    config, pressureArchive, "source_tag", config.pressureArchive.sourceTag, "pressure_archive.source_tag");
```

- [ ] **Step 6: Add default JSON config**

Append to `config/env_config.json` before `serial`:

```json
"timeseries_archive": {
    "_source_note": "Optional new feature: start InfluxDB and archive pressure DDS samples to a time-series database.",
    "enabled": true,
    "start_influxdb": true,
    "influxdb_exe_path": "D:/InfluxDB 3 Core/influxdb3-core-3.9.2-windows_amd64/influxdb3.exe",
    "host_url": "http://localhost:8181",
    "node_id": "afp-node",
    "object_store": "file",
    "data_dir": "D:/InfluxDB 3 Core/data",
    "admin_token_file": "D:/InfluxDB 3 Core/afp-admin-token.json",
    "database": "afp_process",
    "retention_period": "30d",
    "ensure_database_on_start": true,
    "subscriber_autostart": true,
    "subscriber_show_console": true,
    "subscriber_print_live_data": true,
    "write_batch_size": 10,
    "write_flush_interval_ms": 1000
},
"pressure_archive": {
    "enabled": true,
    "dds_domain_id": 0,
    "dds_topic": "afp.pressure.sample",
    "table": "pressure_sensor",
    "source_tag": "pressure_sensor_1"
},
```

- [ ] **Step 7: Run config test**

Run:

```powershell
cmake --build build --config Release --target test_app_config
ctest --test-dir build -C Release -R test_app_config --output-on-failure
```

Expected: `1/1 Test #...: test_app_config ... Passed`.

- [ ] **Step 8: Commit**

```powershell
git add src/app/AppConfig.h src/app/AppConfig.cpp config/env_config.json tests/test_app_config.cpp
git commit -m "feat: add time-series archive config"
```

---

### Task 2: Time-Series Point And Line Protocol

**Files:**
- Create: `src/archive/TimeSeriesPoint.h`
- Create: `src/archive/TimeSeriesPoint.cpp`
- Test: `tests/test_time_series_point.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing line-protocol tests**

Create `tests/test_time_series_point.cpp`:

```cpp
#include <QtTest/QtTest>

#include "archive/TimeSeriesPoint.h"

class TestTimeSeriesPoint : public QObject {
    Q_OBJECT

private slots:
    void formatsPressurePoint();
    void escapesNamesTagsAndStrings();
};

void TestTimeSeriesPoint::formatsPressurePoint() {
    TimeSeriesPoint point;
    point.table = "pressure_sensor";
    point.tags.insert("source", "pressure_sensor_1");
    point.floatFields.insert("raw_value", 12.3);
    point.integerFields.insert("sequence", 42);
    point.boolFields.insert("window_ready", true);
    point.timestampNs = 1710000000000000000ULL;

    QCOMPARE(
        point.toLineProtocol(),
        QString("pressure_sensor,source=pressure_sensor_1 raw_value=12.3,sequence=42i,window_ready=true 1710000000000000000"));
}

void TestTimeSeriesPoint::escapesNamesTagsAndStrings() {
    TimeSeriesPoint point;
    point.table = "pressure sensor";
    point.tags.insert("source name", "sensor,1");
    point.stringFields.insert("status text", "ready \"now\"");
    point.timestampNs = 10;

    QCOMPARE(
        point.toLineProtocol(),
        QString("pressure\\ sensor,source\\ name=sensor\\,1 status\\ text=\"ready \\\"now\\\"\" 10"));
}

QTEST_MAIN(TestTimeSeriesPoint)
#include "test_time_series_point.moc"
```

- [ ] **Step 2: Add test target and verify failure**

Add to `CMakeLists.txt`:

```cmake
add_executable(test_time_series_point
    tests/test_time_series_point.cpp
    src/archive/TimeSeriesPoint.cpp
)
target_include_directories(test_time_series_point PRIVATE src)
target_link_libraries(test_time_series_point PRIVATE Qt6::Core Qt6::Test)
copy_qt_test_runtime(test_time_series_point)
add_test(NAME test_time_series_point COMMAND test_time_series_point)
```

Run:

```powershell
cmake --build build --config Release --target test_time_series_point
```

Expected: compile failure because `archive/TimeSeriesPoint.h` does not exist.

- [ ] **Step 3: Implement point model**

Create `src/archive/TimeSeriesPoint.h`:

```cpp
#pragma once

#include <QMap>
#include <QString>

#include <cstdint>

// Backend-neutral time-series point. DDS readers produce this structure, and
// the Influx writer is the only layer that converts it to line protocol.
struct TimeSeriesPoint {
    QString table;
    QMap<QString, QString> tags;
    QMap<QString, double> floatFields;
    QMap<QString, qint64> integerFields;
    QMap<QString, bool> boolFields;
    QMap<QString, QString> stringFields;
    std::uint64_t timestampNs = 0;

    QString toLineProtocol() const;
};

QString escapeMeasurementOrKey(QString value);
QString escapeTagValue(QString value);
QString escapeStringField(QString value);
```

Create `src/archive/TimeSeriesPoint.cpp`:

```cpp
#include "archive/TimeSeriesPoint.h"

#include <QStringList>

namespace {
QString formatDouble(double value) {
    return QString::number(value, 'g', 15);
}
}

QString escapeMeasurementOrKey(QString value) {
    value.replace("\\", "\\\\");
    value.replace(" ", "\\ ");
    value.replace(",", "\\,");
    value.replace("=", "\\=");
    return value;
}

QString escapeTagValue(QString value) {
    value.replace("\\", "\\\\");
    value.replace(" ", "\\ ");
    value.replace(",", "\\,");
    value.replace("=", "\\=");
    return value;
}

QString escapeStringField(QString value) {
    value.replace("\\", "\\\\");
    value.replace("\"", "\\\"");
    return value;
}

QString TimeSeriesPoint::toLineProtocol() const {
    QString line = escapeMeasurementOrKey(table);
    for (auto it = tags.cbegin(); it != tags.cend(); ++it) {
        line += "," + escapeMeasurementOrKey(it.key()) + "=" + escapeTagValue(it.value());
    }

    QStringList fields;
    for (auto it = floatFields.cbegin(); it != floatFields.cend(); ++it) {
        fields << escapeMeasurementOrKey(it.key()) + "=" + formatDouble(it.value());
    }
    for (auto it = integerFields.cbegin(); it != integerFields.cend(); ++it) {
        fields << escapeMeasurementOrKey(it.key()) + "=" + QString::number(it.value()) + "i";
    }
    for (auto it = boolFields.cbegin(); it != boolFields.cend(); ++it) {
        fields << escapeMeasurementOrKey(it.key()) + "=" + (it.value() ? "true" : "false");
    }
    for (auto it = stringFields.cbegin(); it != stringFields.cend(); ++it) {
        fields << escapeMeasurementOrKey(it.key()) + "=\"" + escapeStringField(it.value()) + "\"";
    }

    line += " " + fields.join(',');
    if (timestampNs > 0) {
        line += " " + QString::number(timestampNs);
    }
    return line;
}
```

- [ ] **Step 4: Run test**

```powershell
cmake --build build --config Release --target test_time_series_point
ctest --test-dir build -C Release -R test_time_series_point --output-on-failure
```

Expected: `test_time_series_point` passes.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt src/archive/TimeSeriesPoint.h src/archive/TimeSeriesPoint.cpp tests/test_time_series_point.cpp
git commit -m "feat: add InfluxDB line protocol point model"
```

---

### Task 3: InfluxDB Token And Admin CLI Helper

**Files:**
- Create: `src/archive/InfluxAdmin.h`
- Create: `src/archive/InfluxAdmin.cpp`
- Test: `tests/test_influx_token.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write token and command tests**

Create `tests/test_influx_token.cpp`:

```cpp
#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QFile>

#include "archive/InfluxAdmin.h"

class TestInfluxToken : public QObject {
    Q_OBJECT

private slots:
    void readsTokenFromJsonFile();
    void rejectsMissingTokenField();
    void buildsServeArgumentsFromConfig();
    void buildsCreateDatabaseEnvironment();
};

void TestInfluxToken::readsTokenFromJsonFile() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("afp-admin-token.json");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"({"token":"apiv3_test_token","name":"afp-admin"})");
    file.close();

    QString error;
    QCOMPARE(readInfluxAdminToken(path, &error), QString("apiv3_test_token"));
    QVERIFY(error.isEmpty());
}

void TestInfluxToken::rejectsMissingTokenField() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("afp-admin-token.json");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"({"name":"afp-admin"})");
    file.close();

    QString error;
    QVERIFY(readInfluxAdminToken(path, &error).isEmpty());
    QVERIFY(error.contains("token"));
}

void TestInfluxToken::buildsServeArgumentsFromConfig() {
    TimeseriesArchiveConfig config;
    config.nodeId = "afp-node";
    config.objectStore = "file";
    config.dataDir = "D:/InfluxDB 3 Core/data";
    config.adminTokenFile = "D:/InfluxDB 3 Core/afp-admin-token.json";

    QCOMPARE(
        buildInfluxServeArguments(config),
        QStringList({"serve",
                     "--node-id", "afp-node",
                     "--object-store", "file",
                     "--data-dir", "D:/InfluxDB 3 Core/data",
                     "--admin-token-file", "D:/InfluxDB 3 Core/afp-admin-token.json"}));
}

void TestInfluxToken::buildsCreateDatabaseEnvironment() {
    TimeseriesArchiveConfig config;
    config.hostUrl = "http://localhost:8181";
    const QProcessEnvironment env = buildInfluxCliEnvironment(config, "apiv3_test_token");

    QCOMPARE(env.value("INFLUXDB3_HOST_URL"), QString("http://localhost:8181"));
    QCOMPARE(env.value("INFLUXDB3_AUTH_TOKEN"), QString("apiv3_test_token"));
}

QTEST_MAIN(TestInfluxToken)
#include "test_influx_token.moc"
```

- [ ] **Step 2: Add CMake target and verify failure**

Add to `CMakeLists.txt`:

```cmake
add_executable(test_influx_token
    tests/test_influx_token.cpp
    src/archive/InfluxAdmin.cpp
    src/app/AppConfig.cpp
    src/app/AppPaths.cpp
)
target_include_directories(test_influx_token PRIVATE src)
target_link_libraries(test_influx_token PRIVATE Qt6::Core Qt6::Network Qt6::Test)
copy_qt_test_runtime(test_influx_token)
add_custom_command(TARGET test_influx_token POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Network>
            $<TARGET_FILE_DIR:test_influx_token>
)
add_test(NAME test_influx_token COMMAND test_influx_token)
```

Run:

```powershell
cmake --build build --config Release --target test_influx_token
```

Expected: compile failure because `archive/InfluxAdmin.h` does not exist.

- [ ] **Step 3: Implement helper interface**

Create `src/archive/InfluxAdmin.h`:

```cpp
#pragma once

#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

#include "app/AppConfig.h"

QString readInfluxAdminToken(const QString& tokenFilePath, QString* error);
QStringList buildInfluxServeArguments(const TimeseriesArchiveConfig& config);
QStringList buildInfluxShowDatabasesArguments();
QStringList buildInfluxCreateDatabaseArguments(const TimeseriesArchiveConfig& config);
QProcessEnvironment buildInfluxCliEnvironment(const TimeseriesArchiveConfig& config, const QString& token);
```

Create `src/archive/InfluxAdmin.cpp`:

```cpp
#include "archive/InfluxAdmin.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

QString readInfluxAdminToken(const QString& tokenFilePath, QString* error) {
    QFile file(tokenFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error != nullptr) {
            *error = QString("Could not open InfluxDB admin token file: %1").arg(tokenFilePath);
        }
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error != nullptr) {
            *error = QString("Could not parse InfluxDB admin token JSON: %1").arg(parseError.errorString());
        }
        return {};
    }

    const QString token = document.object().value("token").toString().trimmed();
    if (token.isEmpty()) {
        if (error != nullptr) {
            *error = "InfluxDB admin token file is missing a non-empty 'token' field.";
        }
        return {};
    }

    if (error != nullptr) {
        error->clear();
    }
    return token;
}

QStringList buildInfluxServeArguments(const TimeseriesArchiveConfig& config) {
    return {"serve",
            "--node-id", config.nodeId,
            "--object-store", config.objectStore,
            "--data-dir", config.dataDir,
            "--admin-token-file", config.adminTokenFile};
}

QStringList buildInfluxShowDatabasesArguments() {
    return {"show", "databases", "--format", "json"};
}

QStringList buildInfluxCreateDatabaseArguments(const TimeseriesArchiveConfig& config) {
    return {"create", "database", "--retention-period", config.retentionPeriod, config.database};
}

QProcessEnvironment buildInfluxCliEnvironment(const TimeseriesArchiveConfig& config, const QString& token) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("INFLUXDB3_HOST_URL", config.hostUrl);
    env.insert("INFLUXDB3_AUTH_TOKEN", token);
    return env;
}
```

- [ ] **Step 4: Run test**

```powershell
cmake --build build --config Release --target test_influx_token
ctest --test-dir build -C Release -R test_influx_token --output-on-failure
```

Expected: `test_influx_token` passes.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt src/archive/InfluxAdmin.h src/archive/InfluxAdmin.cpp tests/test_influx_token.cpp
git commit -m "feat: add InfluxDB admin helpers"
```

---

### Task 4: HTTP Line-Protocol Writer

**Files:**
- Create: `src/archive/InfluxLineProtocolWriter.h`
- Create: `src/archive/InfluxLineProtocolWriter.cpp`
- Test: `tests/test_influx_line_protocol_writer.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write writer unit tests**

Create `tests/test_influx_line_protocol_writer.cpp` with a fake transport:

```cpp
#include <QtTest/QtTest>

#include "archive/InfluxLineProtocolWriter.h"

class FakeInfluxHttpClient final : public IInfluxHttpClient {
public:
    bool postLineProtocol(const QString& url,
                          const QString& database,
                          const QString& token,
                          const QByteArray& body,
                          QString* error) override {
        Q_UNUSED(error);
        urls << url;
        databases << database;
        tokens << token;
        bodies << QString::fromUtf8(body);
        return true;
    }

    QStringList urls;
    QStringList databases;
    QStringList tokens;
    QStringList bodies;
};

class TestInfluxLineProtocolWriter : public QObject {
    Q_OBJECT

private slots:
    void flushesWhenBatchSizeReached();
    void manualFlushWritesPartialBatch();
};

void TestInfluxLineProtocolWriter::flushesWhenBatchSizeReached() {
    TimeseriesArchiveConfig config;
    config.hostUrl = "http://localhost:8181";
    config.database = "afp_process";
    config.writeBatchSize = 2;

    auto fake = std::make_unique<FakeInfluxHttpClient>();
    auto* fakePtr = fake.get();
    InfluxLineProtocolWriter writer(config, "apiv3_token", std::move(fake));

    TimeSeriesPoint first;
    first.table = "pressure_sensor";
    first.floatFields.insert("raw_value", 1.0);
    first.timestampNs = 10;

    TimeSeriesPoint second = first;
    second.floatFields["raw_value"] = 2.0;
    second.timestampNs = 20;

    QVERIFY(writer.enqueue(first));
    QVERIFY(writer.enqueue(second));

    QCOMPARE(fakePtr->bodies.size(), 1);
    QVERIFY(fakePtr->bodies.first().contains("raw_value=1"));
    QVERIFY(fakePtr->bodies.first().contains("raw_value=2"));
    QCOMPARE(fakePtr->databases.first(), QString("afp_process"));
    QCOMPARE(fakePtr->tokens.first(), QString("apiv3_token"));
}

void TestInfluxLineProtocolWriter::manualFlushWritesPartialBatch() {
    TimeseriesArchiveConfig config;
    config.writeBatchSize = 10;

    auto fake = std::make_unique<FakeInfluxHttpClient>();
    auto* fakePtr = fake.get();
    InfluxLineProtocolWriter writer(config, "token", std::move(fake));

    TimeSeriesPoint point;
    point.table = "pressure_sensor";
    point.integerFields.insert("sequence", 1);

    QVERIFY(writer.enqueue(point));
    QCOMPARE(fakePtr->bodies.size(), 0);
    QVERIFY(writer.flush());
    QCOMPARE(fakePtr->bodies.size(), 1);
    QVERIFY(fakePtr->bodies.first().contains("sequence=1i"));
}

QTEST_MAIN(TestInfluxLineProtocolWriter)
#include "test_influx_line_protocol_writer.moc"
```

- [ ] **Step 2: Add target and verify failure**

Add CMake target:

```cmake
add_executable(test_influx_line_protocol_writer
    tests/test_influx_line_protocol_writer.cpp
    src/archive/InfluxLineProtocolWriter.cpp
    src/archive/TimeSeriesPoint.cpp
)
target_include_directories(test_influx_line_protocol_writer PRIVATE src)
target_link_libraries(test_influx_line_protocol_writer PRIVATE Qt6::Core Qt6::Network Qt6::Test)
copy_qt_test_runtime(test_influx_line_protocol_writer)
add_custom_command(TARGET test_influx_line_protocol_writer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Network>
            $<TARGET_FILE_DIR:test_influx_line_protocol_writer>
)
add_test(NAME test_influx_line_protocol_writer COMMAND test_influx_line_protocol_writer)
```

Run:

```powershell
cmake --build build --config Release --target test_influx_line_protocol_writer
```

Expected: compile failure because `InfluxLineProtocolWriter` does not exist.

- [ ] **Step 3: Implement writer**

Create `src/archive/InfluxLineProtocolWriter.h`:

```cpp
#pragma once

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QVector>

#include <memory>

#include "app/AppConfig.h"
#include "archive/TimeSeriesPoint.h"

class IInfluxHttpClient {
public:
    virtual ~IInfluxHttpClient() = default;
    virtual bool postLineProtocol(const QString& url,
                                  const QString& database,
                                  const QString& token,
                                  const QByteArray& body,
                                  QString* error) = 0;
};

class QtInfluxHttpClient final : public IInfluxHttpClient {
public:
    bool postLineProtocol(const QString& url,
                          const QString& database,
                          const QString& token,
                          const QByteArray& body,
                          QString* error) override;
};

class InfluxLineProtocolWriter final {
public:
    InfluxLineProtocolWriter(TimeseriesArchiveConfig config,
                             QString token,
                             std::unique_ptr<IInfluxHttpClient> httpClient);

    bool enqueue(const TimeSeriesPoint& point);
    bool flush();
    QString lastError() const;

private:
    TimeseriesArchiveConfig config_;
    QString token_;
    std::unique_ptr<IInfluxHttpClient> httpClient_;
    QVector<TimeSeriesPoint> pending_;
    QString lastError_;
};
```

Create `src/archive/InfluxLineProtocolWriter.cpp` with:

```cpp
#include "archive/InfluxLineProtocolWriter.h"

#include <QEventLoop>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

bool QtInfluxHttpClient::postLineProtocol(const QString& url,
                                          const QString& database,
                                          const QString& token,
                                          const QByteArray& body,
                                          QString* error) {
    QUrl endpoint(url + "/api/v3/write_lp");
    QUrlQuery query;
    query.addQueryItem("db", database);
    endpoint.setQuery(query);

    QNetworkRequest request(endpoint);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain; charset=utf-8");
    request.setRawHeader("Authorization", QByteArray("Bearer ") + token.toUtf8());

    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkReply* reply = manager.post(request, body);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const bool ok = reply->error() == QNetworkReply::NoError;
    if (!ok && error != nullptr) {
        *error = reply->errorString();
    }
    reply->deleteLater();
    return ok;
}

InfluxLineProtocolWriter::InfluxLineProtocolWriter(TimeseriesArchiveConfig config,
                                                   QString token,
                                                   std::unique_ptr<IInfluxHttpClient> httpClient)
    : config_(std::move(config)),
      token_(std::move(token)),
      httpClient_(std::move(httpClient)) {
}

bool InfluxLineProtocolWriter::enqueue(const TimeSeriesPoint& point) {
    pending_.push_back(point);
    if (pending_.size() >= config_.writeBatchSize) {
        return flush();
    }
    return true;
}

bool InfluxLineProtocolWriter::flush() {
    if (pending_.isEmpty()) {
        return true;
    }

    QByteArray body;
    for (const TimeSeriesPoint& point : std::as_const(pending_)) {
        body += point.toLineProtocol().toUtf8();
        body += '\n';
    }

    QString error;
    if (!httpClient_->postLineProtocol(config_.hostUrl, config_.database, token_, body, &error)) {
        lastError_ = QString("InfluxDB write failed: %1").arg(error);
        return false;
    }

    pending_.clear();
    return true;
}

QString InfluxLineProtocolWriter::lastError() const {
    return lastError_;
}
```

- [ ] **Step 4: Run writer tests**

```powershell
cmake --build build --config Release --target test_influx_line_protocol_writer
ctest --test-dir build -C Release -R test_influx_line_protocol_writer --output-on-failure
```

Expected: writer tests pass.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt src/archive/InfluxLineProtocolWriter.h src/archive/InfluxLineProtocolWriter.cpp tests/test_influx_line_protocol_writer.cpp
git commit -m "feat: add InfluxDB line protocol writer"
```

---

### Task 5: Pressure DDS Reader

**Files:**
- Create: `src/archive/PressureSampleReader.h`
- Create: `src/archive/PressureSampleReader.cpp`
- Test: extend `tests/test_time_series_point.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add conversion test**

Extend `tests/test_time_series_point.cpp` with:

```cpp
#include "archive/PressureSampleReader.h"
```

Add slot:

```cpp
void convertsPressureSampleToPoint();
```

Add test:

```cpp
void TestTimeSeriesPoint::convertsPressureSampleToPoint() {
    PressureArchiveConfig config;
    config.table = "pressure_sensor";
    config.sourceTag = "pressure_sensor_1";

    AfpPressureSample sample;
    sample.timestamp_ms(1234);
    sample.sequence(7);
    sample.raw_value(1.5);
    sample.mean_value(1.2);
    sample.variance_value(0.4);
    sample.standard_deviation_value(0.632);
    sample.peak_value(2.0);
    sample.window_ready(true);
    sample.sensor_running(true);

    const TimeSeriesPoint point = pressureSampleToPoint(sample, config);

    QCOMPARE(point.table, QString("pressure_sensor"));
    QCOMPARE(point.tags.value("source"), QString("pressure_sensor_1"));
    QCOMPARE(point.floatFields.value("raw_value"), 1.5);
    QCOMPARE(point.integerFields.value("sequence"), 7);
    QVERIFY(point.boolFields.value("window_ready"));
    QCOMPARE(point.timestampNs, 1234000000ULL);
}
```

- [ ] **Step 2: Run test to verify failure**

```powershell
cmake --build build --config Release --target test_time_series_point
```

Expected: compile failure because `PressureSampleReader.h` does not exist.

- [ ] **Step 3: Implement conversion and reader shell**

Create `src/archive/PressureSampleReader.h`:

```cpp
#pragma once

#include <functional>

#include <QString>

#include "app/AppConfig.h"
#include "archive/TimeSeriesPoint.h"
#include "pressure/generated/PressureSample.hpp"

using TimeSeriesPointHandler = std::function<void(const TimeSeriesPoint&)>;

TimeSeriesPoint pressureSampleToPoint(const AfpPressureSample& sample, const PressureArchiveConfig& config);

class PressureSampleReader final {
public:
    PressureSampleReader(PressureArchiveConfig config, TimeSeriesPointHandler handler);
    ~PressureSampleReader();

    bool start();
    void stop();
    QString lastError() const;

private:
    PressureArchiveConfig config_;
    TimeSeriesPointHandler handler_;
    QString lastError_;
};
```

Create `src/archive/PressureSampleReader.cpp` initially with conversion and DDS object lifetime comments:

```cpp
#include "archive/PressureSampleReader.h"

TimeSeriesPoint pressureSampleToPoint(const AfpPressureSample& sample, const PressureArchiveConfig& config) {
    TimeSeriesPoint point;
    point.table = config.table;
    point.tags.insert("source", config.sourceTag);
    point.floatFields.insert("raw_value", sample.raw_value());
    point.floatFields.insert("mean_value", sample.mean_value());
    point.floatFields.insert("variance_value", sample.variance_value());
    point.floatFields.insert("standard_deviation_value", sample.standard_deviation_value());
    point.floatFields.insert("peak_value", sample.peak_value());
    point.integerFields.insert("sequence", static_cast<qint64>(sample.sequence()));
    point.boolFields.insert("window_ready", sample.window_ready());
    point.boolFields.insert("sensor_running", sample.sensor_running());
    point.timestampNs = sample.timestamp_ms() * 1000000ULL;
    return point;
}

PressureSampleReader::PressureSampleReader(PressureArchiveConfig config, TimeSeriesPointHandler handler)
    : config_(std::move(config)),
      handler_(std::move(handler)) {
}

PressureSampleReader::~PressureSampleReader() {
    stop();
}

bool PressureSampleReader::start() {
    // This first pass is intentionally conversion-only. The same class is
    // expanded in Step 5 with the Fast DDS DataReader lifetime management.
    return true;
}

void PressureSampleReader::stop() {
}

QString PressureSampleReader::lastError() const {
    return lastError_;
}
```

- [ ] **Step 4: Run conversion test**

```powershell
cmake --build build --config Release --target test_time_series_point
ctest --test-dir build -C Release -R test_time_series_point --output-on-failure
```

Expected: conversion tests pass.

- [ ] **Step 5: Implement Fast DDS subscriber**

Replace the shell internals with Fast DDS objects matching `PressureDdsPublisher.cpp`:

- Create `DomainParticipant` in `config_.ddsDomainId`.
- Register `AfpPressureSamplePubSubType` with type name `AfpPressureSample`.
- Create subscriber, topic from `config_.ddsTopic`, and reliable `DataReader`.
- Add a `DataReaderListener` that calls `reader->take_next_sample(&sample, &info)` while samples are available and invokes `handler_(pressureSampleToPoint(sample, config_))` for valid samples.
- In `stop`, delete reader, subscriber, topic, participant, and generated type support in the reverse order.

Use the same error style as `PressureDdsPublisher`: set `lastError_` to concrete messages such as:

```cpp
"Could not create Fast DDS pressure subscriber participant in domain 0."
"Could not create Fast DDS pressure subscriber topic 'afp.pressure.sample'."
"Could not create Fast DDS pressure data reader."
```

- [ ] **Step 6: Build subscriber-related code**

```powershell
cmake --build build --config Release --target test_time_series_point
```

Expected: compile succeeds.

- [ ] **Step 7: Commit**

```powershell
git add CMakeLists.txt src/archive/PressureSampleReader.h src/archive/PressureSampleReader.cpp tests/test_time_series_point.cpp
git commit -m "feat: add pressure DDS archive reader"
```

---

### Task 6: Subscriber Executable

**Files:**
- Create: `src/archive/DdsInfluxSubscriberApp.h`
- Create: `src/archive/DdsInfluxSubscriberApp.cpp`
- Create: `src/archive/main_dds_influx_subscriber.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create subscriber app coordinator**

Create `src/archive/DdsInfluxSubscriberApp.h`:

```cpp
#pragma once

#include <QObject>
#include <QTimer>

#include <memory>

#include "app/AppConfig.h"
#include "app/AppPaths.h"
#include "archive/InfluxLineProtocolWriter.h"
#include "archive/PressureSampleReader.h"

class DdsInfluxSubscriberApp final : public QObject {
    Q_OBJECT

public:
    DdsInfluxSubscriberApp(AppConfig config, AppPaths paths, QObject* parent = nullptr);

    bool start();
    QString lastError() const;

private:
    void handlePoint(const TimeSeriesPoint& point);

    AppConfig config_;
    AppPaths paths_;
    QString lastError_;
    std::unique_ptr<InfluxLineProtocolWriter> writer_;
    std::unique_ptr<PressureSampleReader> pressureReader_;
    QTimer flushTimer_;
};
```

Create `src/archive/DdsInfluxSubscriberApp.cpp`:

```cpp
#include "archive/DdsInfluxSubscriberApp.h"

#include <QDebug>

#include "archive/InfluxAdmin.h"

DdsInfluxSubscriberApp::DdsInfluxSubscriberApp(AppConfig config, AppPaths paths, QObject* parent)
    : QObject(parent),
      config_(std::move(config)),
      paths_(std::move(paths)) {
}

bool DdsInfluxSubscriberApp::start() {
    if (!config_.timeseriesArchive.enabled || !config_.pressureArchive.enabled) {
        lastError_ = "Time-series archive or pressure archive is disabled.";
        return false;
    }

    QString tokenError;
    const QString token = readInfluxAdminToken(config_.timeseriesArchive.adminTokenFile, &tokenError);
    if (token.isEmpty()) {
        lastError_ = tokenError;
        return false;
    }

    writer_ = std::make_unique<InfluxLineProtocolWriter>(
        config_.timeseriesArchive,
        token,
        std::make_unique<QtInfluxHttpClient>());

    pressureReader_ = std::make_unique<PressureSampleReader>(
        config_.pressureArchive,
        [this](const TimeSeriesPoint& point) { handlePoint(point); });

    connect(&flushTimer_, &QTimer::timeout, this, [this]() {
        if (writer_ != nullptr && !writer_->flush()) {
            qWarning().noquote() << writer_->lastError();
        }
    });
    flushTimer_.start(config_.timeseriesArchive.writeFlushIntervalMs);

    if (!pressureReader_->start()) {
        lastError_ = pressureReader_->lastError();
        return false;
    }

    qInfo().noquote() << "DDS InfluxDB subscriber started for topic"
                      << config_.pressureArchive.ddsTopic;
    return true;
}

void DdsInfluxSubscriberApp::handlePoint(const TimeSeriesPoint& point) {
    if (config_.timeseriesArchive.subscriberPrintLiveData) {
        qInfo().noquote() << point.toLineProtocol();
    }
    if (writer_ != nullptr && !writer_->enqueue(point)) {
        qWarning().noquote() << writer_->lastError();
    }
}

QString DdsInfluxSubscriberApp::lastError() const {
    return lastError_;
}
```

- [ ] **Step 2: Create entry point**

Create `src/archive/main_dds_influx_subscriber.cpp`:

```cpp
#include <QCoreApplication>
#include <QDebug>

#include "app/AppConfig.h"
#include "app/AppPaths.h"
#include "archive/DdsInfluxSubscriberApp.h"

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    AppPaths paths(QCoreApplication::applicationDirPath());
    AppConfig config = AppConfig::load(paths);
    if (!config.isLoaded()) {
        qWarning().noquote() << config.warnings().join('\n');
    }

    DdsInfluxSubscriberApp subscriber(config, paths);
    if (!subscriber.start()) {
        qCritical().noquote() << subscriber.lastError();
        return 1;
    }

    return app.exec();
}
```

- [ ] **Step 3: Add CMake executable**

Add archive source variables and executable to `CMakeLists.txt`:

```cmake
set(ARCHIVE_COMMON_SOURCES
    src/archive/TimeSeriesPoint.h
    src/archive/TimeSeriesPoint.cpp
    src/archive/InfluxAdmin.h
    src/archive/InfluxAdmin.cpp
    src/archive/InfluxLineProtocolWriter.h
    src/archive/InfluxLineProtocolWriter.cpp
    src/archive/PressureSampleReader.h
    src/archive/PressureSampleReader.cpp
    src/archive/DdsInfluxSubscriberApp.h
    src/archive/DdsInfluxSubscriberApp.cpp
    src/app/AppConfig.h
    src/app/AppConfig.cpp
    src/app/AppPaths.h
    src/app/AppPaths.cpp
    src/pressure/generated/PressureSamplePubSubTypes.cxx
    src/pressure/generated/PressureSampleTypeObjectSupport.cxx
)

add_executable(dds_influx_subscriber
    src/archive/main_dds_influx_subscriber.cpp
    ${ARCHIVE_COMMON_SOURCES}
)
target_include_directories(dds_influx_subscriber PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pressure/generated
)
target_link_libraries(dds_influx_subscriber PRIVATE Qt6::Core Qt6::Network fastdds)
add_custom_command(TARGET dds_influx_subscriber POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/config
            $<TARGET_FILE_DIR:dds_influx_subscriber>/config
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Core>
            $<TARGET_FILE:Qt6::Network>
            $<TARGET_FILE_DIR:dds_influx_subscriber>
)
```

- [ ] **Step 4: Build subscriber**

```powershell
cmake --build build --config Release --target dds_influx_subscriber
```

Expected: `build/Release/dds_influx_subscriber.exe` exists.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt src/archive/DdsInfluxSubscriberApp.h src/archive/DdsInfluxSubscriberApp.cpp src/archive/main_dds_influx_subscriber.cpp
git commit -m "feat: add DDS InfluxDB subscriber executable"
```

---

### Task 7: Main App Startup Service

**Files:**
- Create: `src/services/archive/TimeseriesArchiveService.h`
- Create: `src/services/archive/TimeseriesArchiveService.cpp`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Test: `tests/test_timeseries_archive_service.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write service tests**

Create `tests/test_timeseries_archive_service.cpp`:

```cpp
#include <QtTest/QtTest>

#include "services/archive/TimeseriesArchiveService.h"

class TestTimeseriesArchiveService : public QObject {
    Q_OBJECT

private slots:
    void disabledConfigDoesNotStart();
    void missingInfluxExecutableReportsError();
};

void TestTimeseriesArchiveService::disabledConfigDoesNotStart() {
    AppConfig config;
    config.timeseriesArchive.enabled = false;
    TimeseriesArchiveService service(config, AppPaths(QDir::tempPath()));

    QVERIFY(!service.start());
    QVERIFY(service.lastStatus().contains("disabled"));
}

void TestTimeseriesArchiveService::missingInfluxExecutableReportsError() {
    AppConfig config;
    config.timeseriesArchive.enabled = true;
    config.timeseriesArchive.startInfluxdb = true;
    config.timeseriesArchive.influxdbExePath = "Z:/missing/influxdb3.exe";
    config.timeseriesArchive.subscriberAutostart = false;

    TimeseriesArchiveService service(config, AppPaths(QDir::tempPath()));

    QVERIFY(!service.start());
    QVERIFY(service.lastStatus().contains("InfluxDB executable"));
}

QTEST_MAIN(TestTimeseriesArchiveService)
#include "test_timeseries_archive_service.moc"
```

- [ ] **Step 2: Add target and verify failure**

Add CMake target:

```cmake
add_executable(test_timeseries_archive_service
    tests/test_timeseries_archive_service.cpp
    src/services/archive/TimeseriesArchiveService.cpp
    src/archive/InfluxAdmin.cpp
    src/app/AppConfig.cpp
    src/app/AppPaths.cpp
)
target_include_directories(test_timeseries_archive_service PRIVATE src)
target_link_libraries(test_timeseries_archive_service PRIVATE Qt6::Core Qt6::Network Qt6::Test)
copy_qt_test_runtime(test_timeseries_archive_service)
add_custom_command(TARGET test_timeseries_archive_service POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Network>
            $<TARGET_FILE_DIR:test_timeseries_archive_service>
)
add_test(NAME test_timeseries_archive_service COMMAND test_timeseries_archive_service)
```

Run:

```powershell
cmake --build build --config Release --target test_timeseries_archive_service
```

Expected: compile failure because `TimeseriesArchiveService.h` does not exist.

- [ ] **Step 3: Implement service**

Create `src/services/archive/TimeseriesArchiveService.h`:

```cpp
#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

#include "app/AppConfig.h"
#include "app/AppPaths.h"

class TimeseriesArchiveService final : public QObject {
    Q_OBJECT

public:
    TimeseriesArchiveService(AppConfig config, AppPaths paths, QObject* parent = nullptr);
    ~TimeseriesArchiveService() override;

    bool start();
    void stopSubscriber();
    QString lastStatus() const;

signals:
    void statusChanged(const QString& status);

private:
    bool startInfluxdbIfNeeded();
    bool ensureDatabase();
    bool startSubscriber();
    void setStatus(const QString& status);

    AppConfig config_;
    AppPaths paths_;
    QProcess influxProcess_;
    QProcess subscriberProcess_;
    QString lastStatus_;
};
```

Create `src/services/archive/TimeseriesArchiveService.cpp` with these first-pass behaviors:

- If `timeseriesArchive.enabled == false`, set status `"Time-series archive is disabled."` and return `false`.
- If `startInfluxdb == true` and `influxdbExePath` is missing, set status with the exact configured path, for example `"InfluxDB executable does not exist: Z:/missing/influxdb3.exe"`, and return `false`.
- If `startInfluxdb == true`, start `influxdb3.exe` using `buildInfluxServeArguments(config_.timeseriesArchive)`.
- Use a short reachability check against `hostUrl` with `QNetworkAccessManager` before and after starting.
- If `ensureDatabaseOnStart == true`, read the token and run `influxdb3 show databases --format json`; run `influxdb3 create database ...` only if the database name is not present.
- If `subscriberAutostart == true`, start `dds_influx_subscriber.exe` from `paths_.rootDir()`.
- If `subscriberShowConsole == true` on Windows, use `QProcess::startDetached` for visible console; otherwise keep `subscriberProcess_` owned by the service.

Add explanatory comments for why the service uses CLI for database admin and HTTP is reserved for streaming writes.

- [ ] **Step 4: Wire into MainWindow**

Modify `src/ui/MainWindow.h`:

```cpp
#include "services/archive/TimeseriesArchiveService.h"
```

Add member near other services:

```cpp
TimeseriesArchiveService timeseriesArchiveService_;
```

Modify constructor initializer list in `src/ui/MainWindow.cpp` after `config_`-dependent services:

```cpp
timeseriesArchiveService_(config_, paths_, this),
```

After `connectSignals();`, start the service:

```cpp
if (config_.timeseriesArchive.enabled) {
    timeseriesArchiveService_.start();
}
```

Connect status to the existing status label in `connectSignals()`:

```cpp
connect(&timeseriesArchiveService_, &TimeseriesArchiveService::statusChanged,
        this, [this](const QString& status) {
            ui_->robot_state_show->setToolTip(ui_->robot_state_show->toolTip() + "\n" + status);
        });
```

- [ ] **Step 5: Run service tests**

```powershell
cmake --build build --config Release --target test_timeseries_archive_service
ctest --test-dir build -C Release -R test_timeseries_archive_service --output-on-failure
```

Expected: service tests pass.

- [ ] **Step 6: Build main app**

```powershell
cmake --build build --config Release --target AFP_ABB_CPP
```

Expected: main app builds.

- [ ] **Step 7: Commit**

```powershell
git add CMakeLists.txt src/services/archive/TimeseriesArchiveService.h src/services/archive/TimeseriesArchiveService.cpp src/ui/MainWindow.h src/ui/MainWindow.cpp tests/test_timeseries_archive_service.cpp
git commit -m "feat: start time-series archive from main app"
```

---

### Task 8: Portable Release And CMake Integration

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Copy subscriber beside main executable**

Add to the `AFP_ABB_CPP` post-build command:

```cmake
COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:dds_influx_subscriber>
        $<TARGET_FILE_DIR:AFP_ABB_CPP>
```

Add `dds_influx_subscriber` as a dependency of `AFP_ABB_CPP`:

```cmake
add_dependencies(AFP_ABB_CPP dds_influx_subscriber)
```

- [ ] **Step 2: Ensure runtime DLLs for subscriber**

If the current CMake/vcpkg setup does not automatically place Fast DDS runtime DLLs beside `dds_influx_subscriber.exe`, add explicit post-build copy commands for the discovered imported targets. Keep this narrow: copy only required runtime DLLs, not whole dependency trees with unknown contents.

- [ ] **Step 3: Build Release app**

```powershell
cmake --build build --config Release --target AFP_ABB_CPP
```

Expected:

```text
build/Release/AFP_ABB_CPP.exe
build/Release/dds_influx_subscriber.exe
build/Release/config/env_config.json
```

- [ ] **Step 4: Commit**

```powershell
git add CMakeLists.txt
git commit -m "build: include DDS InfluxDB subscriber in portable release"
```

---

### Task 9: README Documentation

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Add InfluxDB section**

Add a section named `Pressure DDS Archive To InfluxDB`.

Include the token generation command exactly with the user's filename:

```powershell
cd "D:\InfluxDB 3 Core\influxdb3-core-3.9.2-windows_amd64"
.\influxdb3.exe create token --admin `
  --name afp-admin `
  --offline `
  --output-file "D:\InfluxDB 3 Core\afp-admin-token.json"
```

Explain that `afp-admin-token.json` must contain a non-empty `token` field and should be protected.

- [ ] **Step 2: Document manual InfluxDB startup**

Add:

```powershell
.\influxdb3.exe serve `
  --node-id afp-node `
  --object-store file `
  --data-dir "D:\InfluxDB 3 Core\data" `
  --admin-token-file "D:\InfluxDB 3 Core\afp-admin-token.json"
```

- [ ] **Step 3: Document automatic startup**

Explain:

- `start_influxdb: true` lets `AFP_ABB_CPP.exe` start InfluxDB if `host_url` is not reachable.
- `ensure_database_on_start: true` checks existing databases and creates `afp_process` only if missing.
- `subscriber_autostart: true` starts `dds_influx_subscriber.exe`.
- `subscriber_show_console` controls whether a console window appears.
- `subscriber_print_live_data` controls live sample printing.
- `write_batch_size: 10` means roughly 1-second batches at 100 ms/sample; `write_flush_interval_ms` writes smaller batches when data is slower.

- [ ] **Step 4: Document manual database creation**

Add the command:

```powershell
$env:INFLUXDB3_HOST_URL = "http://localhost:8181"
$env:INFLUXDB3_AUTH_TOKEN = "paste_the_token_value_from_afp-admin-token.json"
.\influxdb3.exe create database --retention-period 30d afp_process
```

- [ ] **Step 5: Commit docs**

```powershell
git add README.md
git commit -m "docs: explain InfluxDB pressure archive setup"
```

---

### Task 10: Full Verification

**Files:**
- No source changes unless verification reveals a defect.

- [ ] **Step 1: Configure**

```powershell
cmake -S . -B build
```

Expected: configure succeeds.

- [ ] **Step 2: Build Release**

```powershell
cmake --build build --config Release
```

Expected: build succeeds.

- [ ] **Step 3: Run focused tests**

```powershell
ctest --test-dir build -C Release -R "test_app_config|test_time_series_point|test_influx_token|test_influx_line_protocol_writer|test_timeseries_archive_service" --output-on-failure
```

Expected: all focused tests pass.

- [ ] **Step 4: Run all tests**

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 5: Verify portable outputs**

Run:

```powershell
Test-Path build\Release\AFP_ABB_CPP.exe
Test-Path build\Release\dds_influx_subscriber.exe
Test-Path build\Release\config\env_config.json
```

Expected: all three commands print `True`.

- [ ] **Step 6: Final commit if fixes were needed**

If verification required any fixes:

```powershell
git add CMakeLists.txt src README.md tests
git commit -m "fix: complete DDS InfluxDB archive verification"
```

If no fixes were needed, do not create an empty commit.

---

## Manual Hardware/InfluxDB Smoke Test

Run this after automated verification, when InfluxDB and pressure DDS publishing are available:

1. Confirm `D:\InfluxDB 3 Core\afp-admin-token.json` exists.
2. Start `build\Release\AFP_ABB_CPP.exe`.
3. Confirm `influxdb3.exe` is running or `http://localhost:8181` is reachable.
4. Confirm `build\Release\dds_influx_subscriber.exe` starts when `subscriber_autostart` is true.
5. Click `FC_machine`, then `FC_start`.
6. Watch the subscriber console if `subscriber_show_console` and `subscriber_print_live_data` are true.
7. Query InfluxDB or use InfluxDB Explorer to confirm `afp_process.pressure_sensor` contains rows.
8. Click `FC_stop` and close `AFP_ABB_CPP.exe`.

## Self-Review Checklist

- The plan keeps the original UI design unchanged.
- The plan does not modify `AFP_ABB_Python`.
- All tunable paths, database names, topic names, batching, and console options are in config.
- The admin token is read from `afp-admin-token.json`; it is not hardcoded.
- InfluxDB startup uses `influxdb3 serve`.
- Database ensure/create uses the `influxdb3` CLI.
- Real-time writes use HTTP line protocol.
- The default `write_batch_size` is `10`.
- README token-generation instructions are explicitly included.
