# Robot Monitor DDS Influx Archive Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Publish parsed robot monitor samples through eProsima Fast DDS and archive them to InfluxDB through the existing `dds_influx_subscriber.exe`.

**Architecture:** Reuse the pressure DDS/Influx architecture. The main app publishes robot monitor DDS samples from `RobotMonitoringService`; the existing subscriber process owns both pressure and robot DDS readers and feeds one shared `InfluxLineProtocolWriter` with `TimeSeriesPoint` rows.

**Tech Stack:** C++17, Qt 6 Core/Network/Test, eProsima Fast DDS, Fast DDS-Gen, CMake, InfluxDB 3 line protocol.

---

## File Structure

Create:

- `src/robot_monitor/RobotMonitorSample.idl`: DDS contract for robot monitor samples.
- `src/robot_monitor/generated/*`: Fast DDS-Gen output for `AfpRobotMonitorSample`.
- `src/robot_monitor/RobotMonitorDdsTypes.h`: hand-written mapping from `RobotMonitorSample` to generated DDS type.
- `src/robot_monitor/RobotMonitorDdsPublisher.h`
- `src/robot_monitor/RobotMonitorDdsPublisher.cpp`
- `src/archive/RobotMonitorSampleReader.h`
- `src/archive/RobotMonitorSampleReader.cpp`
- `tests/test_robot_monitor_dds_types.cpp`

Modify:

- `src/app/AppConfig.h`: add robot monitor DDS config fields and `RobotMonitorArchiveConfig`.
- `src/app/AppConfig.cpp`: parse new config.
- `config/env_config.json`: enable DDS/Influx as default robot archive, turn CSV/raw off by default.
- `src/services/robot_monitor/RobotMonitoringService.h/.cpp`: own/publish through `RobotMonitorDdsPublisher`.
- `src/archive/DdsInfluxSubscriberApp.h/.cpp`: own optional pressure and robot readers.
- `src/archive/TimeSeriesPoint.h/.cpp`: no structural change expected; add robot conversion tests through reader helper.
- `CMakeLists.txt`: add generated sources, new publisher/reader sources, and tests.
- `README.md`: document robot DDS/Influx config, validation, and CSV/raw diagnostic behavior.
- `docs/design/2026-06-06-afp-abb-cpp-slice-l-robot-monitor-dds-influx-design.md`: update only if implementation findings require clarification.

---

## Task 1: Add Config Fields

**Files:**
- Modify: `src/app/AppConfig.h`
- Modify: `src/app/AppConfig.cpp`
- Modify: `config/env_config.json`
- Test: `tests/test_app_config.cpp`

- [ ] **Step 1: Write failing config tests**

Add declarations to `TestAppConfig`:

```cpp
void loadsRobotMonitorDdsConfig();
void invalidRobotMonitorDdsConfigFallsBackToDefaults();
void loadsRobotMonitorArchiveConfig();
void invalidRobotMonitorArchiveConfigFallsBackToDefaults();
```

Add this test after `loadsRobotMonitorConfig()`:

```cpp
void TestAppConfig::loadsRobotMonitorDdsConfig() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "robot": {
            "monitor": {
                "dds_publish_enabled": true,
                "dds_domain_id": 5,
                "dds_topic": "afp.test.robot.monitor"
            }
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QVERIFY(config.robot.monitor.ddsPublishEnabled);
    QCOMPARE(config.robot.monitor.ddsDomainId, 5);
    QCOMPARE(config.robot.monitor.ddsTopic, QString("afp.test.robot.monitor"));
}
```

Add this test after `invalidRobotMonitorConfigFallsBackToDefaults()`:

```cpp
void TestAppConfig::invalidRobotMonitorDdsConfigFallsBackToDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "robot": {
            "monitor": {
                "dds_domain_id": -1,
                "dds_topic": ""
            }
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.robot.monitor.ddsPublishEnabled);
    QCOMPARE(config.robot.monitor.ddsDomainId, 0);
    QCOMPARE(config.robot.monitor.ddsTopic, QString("afp.robot.monitor.sample"));
    const QString warnings = config.warnings().join('\n');
    QVERIFY(warnings.contains("robot.monitor.dds_domain_id"));
    QVERIFY(warnings.contains("robot.monitor.dds_topic"));
}
```

Add this test after `loadsPressureArchiveConfig()`:

```cpp
void TestAppConfig::loadsRobotMonitorArchiveConfig() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "robot_monitor_archive": {
            "enabled": true,
            "dds_domain_id": 6,
            "dds_topic": "afp.test.robot.monitor",
            "table": "robot_monitor_test",
            "source_tag": "abb_test_cell"
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.robotMonitorArchive.enabled);
    QCOMPARE(config.robotMonitorArchive.ddsDomainId, 6);
    QCOMPARE(config.robotMonitorArchive.ddsTopic, QString("afp.test.robot.monitor"));
    QCOMPARE(config.robotMonitorArchive.table, QString("robot_monitor_test"));
    QCOMPARE(config.robotMonitorArchive.sourceTag, QString("abb_test_cell"));
}
```

Add this test after `invalidPressureArchiveConfigFallsBackToDefaults()`:

```cpp
void TestAppConfig::invalidRobotMonitorArchiveConfigFallsBackToDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "robot_monitor_archive": {
            "dds_domain_id": -2,
            "dds_topic": "",
            "table": "",
            "source_tag": ""
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.robotMonitorArchive.enabled);
    QCOMPARE(config.robotMonitorArchive.ddsDomainId, 0);
    QCOMPARE(config.robotMonitorArchive.ddsTopic, QString("afp.robot.monitor.sample"));
    QCOMPARE(config.robotMonitorArchive.table, QString("robot_monitor"));
    QCOMPARE(config.robotMonitorArchive.sourceTag, QString("abb_irb4600"));
    const QString warnings = config.warnings().join('\n');
    QVERIFY(warnings.contains("robot_monitor_archive.dds_domain_id"));
    QVERIFY(warnings.contains("robot_monitor_archive.dds_topic"));
    QVERIFY(warnings.contains("robot_monitor_archive.table"));
    QVERIFY(warnings.contains("robot_monitor_archive.source_tag"));
}
```

- [ ] **Step 2: Run the test and verify it fails**

Run:

```powershell
cmake --build build --config Release --target test_app_config
```

Expected: compile fails because `ddsPublishEnabled`, `ddsDomainId`, `ddsTopic`, and `robotMonitorArchive` do not exist.

- [ ] **Step 3: Add config structs and defaults**

Update `RobotMonitorConfig` in `src/app/AppConfig.h`:

```cpp
    // Normal robot monitor archive path. CSV/raw recording remain diagnostics.
    bool ddsPublishEnabled = true;
    int ddsDomainId = 0;
    QString ddsTopic = "afp.robot.monitor.sample";

    // Writes parsed samples to a simple CSV file for hardware diagnostics.
    bool recordCsv = false;

    // Writes exact socket text before parsing, useful when checking RAPID/C++ parser agreement.
    bool recordRawMessages = false;
```

Add this struct after `PressureArchiveConfig`:

```cpp
struct RobotMonitorArchiveConfig {
    // Master switch for archiving DDS robot monitor samples.
    bool enabled = true;

    // Fast DDS source selection for robot monitor samples.
    int ddsDomainId = 0;
    QString ddsTopic = "afp.robot.monitor.sample";

    // InfluxDB line protocol destination and stable source tag.
    QString table = "robot_monitor";
    QString sourceTag = "abb_irb4600";
};
```

Add this member to `AppConfig`:

```cpp
    RobotMonitorArchiveConfig robotMonitorArchive;
```

- [ ] **Step 4: Parse new config**

In `AppConfig::load`, after existing `robot.monitor.sample_interval_ms` parsing, add:

```cpp
    config.robot.monitor.ddsPublishEnabled = readBool(
        robotMonitor, "dds_publish_enabled", config.robot.monitor.ddsPublishEnabled);
    config.robot.monitor.ddsDomainId = readNonNegativeInt(
        config,
        robotMonitor,
        "dds_domain_id",
        config.robot.monitor.ddsDomainId,
        "robot.monitor.dds_domain_id");
    config.robot.monitor.ddsTopic = readNonEmptyString(
        config,
        robotMonitor,
        "dds_topic",
        config.robot.monitor.ddsTopic,
        "robot.monitor.dds_topic");
```

After `pressure_archive` parsing, add:

```cpp
    const QJsonObject robotMonitorArchive = root.value("robot_monitor_archive").toObject();
    config.robotMonitorArchive.enabled = readBool(
        robotMonitorArchive, "enabled", config.robotMonitorArchive.enabled);
    config.robotMonitorArchive.ddsDomainId = readNonNegativeInt(
        config,
        robotMonitorArchive,
        "dds_domain_id",
        config.robotMonitorArchive.ddsDomainId,
        "robot_monitor_archive.dds_domain_id");
    config.robotMonitorArchive.ddsTopic = readNonEmptyString(
        config,
        robotMonitorArchive,
        "dds_topic",
        config.robotMonitorArchive.ddsTopic,
        "robot_monitor_archive.dds_topic");
    config.robotMonitorArchive.table = readNonEmptyString(
        config,
        robotMonitorArchive,
        "table",
        config.robotMonitorArchive.table,
        "robot_monitor_archive.table");
    config.robotMonitorArchive.sourceTag = readNonEmptyString(
        config,
        robotMonitorArchive,
        "source_tag",
        config.robotMonitorArchive.sourceTag,
        "robot_monitor_archive.source_tag");
```

- [ ] **Step 5: Update default JSON config**

In `config/env_config.json`, update `robot.monitor`:

```json
"dds_publish_enabled": true,
"dds_domain_id": 0,
"dds_topic": "afp.robot.monitor.sample",
"record_csv": false,
"record_raw_messages": false
```

Add top-level section:

```json
"robot_monitor_archive": {
  "enabled": true,
  "dds_domain_id": 0,
  "dds_topic": "afp.robot.monitor.sample",
  "table": "robot_monitor",
  "source_tag": "abb_irb4600"
}
```

- [ ] **Step 6: Run config tests**

Run:

```powershell
cmake --build build --config Release --target test_app_config
ctest -C Release -R test_app_config --output-on-failure
```

Expected: `test_app_config` passes.

- [ ] **Step 7: Commit**

```powershell
git add src/app/AppConfig.h src/app/AppConfig.cpp config/env_config.json tests/test_app_config.cpp
git commit -m "config: add robot monitor dds archive settings"
```

---

## Task 2: Add Robot Monitor DDS Generated Type

**Files:**
- Create: `src/robot_monitor/RobotMonitorSample.idl`
- Create: `src/robot_monitor/generated/RobotMonitorSample.hpp`
- Create: `src/robot_monitor/generated/RobotMonitorSamplePubSubTypes.hpp`
- Create: `src/robot_monitor/generated/RobotMonitorSamplePubSubTypes.cxx`
- Create: `src/robot_monitor/generated/RobotMonitorSampleTypeObjectSupport.hpp`
- Create: `src/robot_monitor/generated/RobotMonitorSampleTypeObjectSupport.cxx`
- Create: `src/robot_monitor/generated/RobotMonitorSampleCdrAux.hpp`
- Create: `src/robot_monitor/generated/RobotMonitorSampleCdrAux.ipp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create IDL**

Create `src/robot_monitor/RobotMonitorSample.idl`:

```idl
struct AfpRobotMonitorSample
{
    unsigned long long timestamp_ms;
    unsigned long long sequence;

    double x;
    double y;
    double z;
    double q1;
    double q2;
    double q3;
    double q4;

    double j1;
    double j2;
    double j3;
    double j4;
    double j5;
    double j6;

    double eax1;
    double eax2;
    double eax3;
    double eax4;
    double eax5;
    double eax6;

    double commanded_tcp_speed;
    double commanded_orientation_speed;
    double commanded_external_linear_speed;
    double commanded_external_orientation_speed;
    double estimated_tcp_speed;
};
```

- [ ] **Step 2: Generate Fast DDS files**

Run from a Visual Studio Developer PowerShell where `cl.exe` is available:

```powershell
fastddsgen -replace -d src\robot_monitor\generated src\robot_monitor\RobotMonitorSample.idl
```

Expected: the generated folder contains `RobotMonitorSample.hpp`, `RobotMonitorSamplePubSubTypes.*`, `RobotMonitorSampleTypeObjectSupport.*`, and CDR auxiliary files.

- [ ] **Step 3: Add generated sources to CMake variables**

In `CMakeLists.txt`, add these files to `APP_SOURCES`:

```cmake
    src/robot_monitor/RobotMonitorSample.idl
    src/robot_monitor/generated/RobotMonitorSample.hpp
    src/robot_monitor/generated/RobotMonitorSamplePubSubTypes.cxx
    src/robot_monitor/generated/RobotMonitorSampleTypeObjectSupport.cxx
```

Add these files to `ARCHIVE_COMMON_SOURCES`:

```cmake
    src/robot_monitor/generated/RobotMonitorSample.hpp
    src/robot_monitor/generated/RobotMonitorSamplePubSubTypes.cxx
    src/robot_monitor/generated/RobotMonitorSampleTypeObjectSupport.cxx
```

Add this include directory to `dds_influx_subscriber`:

```cmake
    src/robot_monitor/generated
```

- [ ] **Step 4: Configure/build to catch generated-code errors**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target dds_influx_subscriber
```

Expected: `dds_influx_subscriber` builds with the new generated sources present.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt src/robot_monitor/RobotMonitorSample.idl src/robot_monitor/generated
git commit -m "dds: add robot monitor sample type"
```

---

## Task 3: Map Robot Samples To DDS

**Files:**
- Create: `src/robot_monitor/RobotMonitorDdsTypes.h`
- Create: `tests/test_robot_monitor_dds_types.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing DDS type mapping test**

Create `tests/test_robot_monitor_dds_types.cpp`:

```cpp
#include <QtTest/QtTest>

#include "robot_monitor/RobotMonitorDdsTypes.h"

class TestRobotMonitorDdsTypes final : public QObject {
    Q_OBJECT

private slots:
    void mapsInternalSampleToDdsSample();
};

void TestRobotMonitorDdsTypes::mapsInternalSampleToDdsSample() {
    RobotMonitorSample sample;
    sample.receivedAtUtc = QDateTime::fromMSecsSinceEpoch(1234, Qt::UTC);
    sample.sequence = 7;
    sample.x = 10.5;
    sample.y = 20.5;
    sample.z = 30.5;
    sample.q1 = 1.0;
    sample.q2 = 0.1;
    sample.q3 = 0.2;
    sample.q4 = 0.3;
    sample.joints = {1, 2, 3, 4, 5, 6};
    sample.externalAxis = {11, 12, 13, 14, 15, 16};
    sample.commandedTcpSpeed = 100.0;
    sample.commandedOrientationSpeed = 200.0;
    sample.commandedExternalLinearSpeed = 300.0;
    sample.commandedExternalOrientationSpeed = 400.0;
    sample.estimatedTcpSpeed = 12.25;

    const AfpRobotMonitorSample ddsSample = toDdsSample(sample);

    QCOMPARE(ddsSample.timestamp_ms(), 1234ULL);
    QCOMPARE(ddsSample.sequence(), 7ULL);
    QCOMPARE(ddsSample.x(), 10.5);
    QCOMPARE(ddsSample.y(), 20.5);
    QCOMPARE(ddsSample.z(), 30.5);
    QCOMPARE(ddsSample.q1(), 1.0);
    QCOMPARE(ddsSample.q2(), 0.1);
    QCOMPARE(ddsSample.q3(), 0.2);
    QCOMPARE(ddsSample.q4(), 0.3);
    QCOMPARE(ddsSample.j1(), 1.0);
    QCOMPARE(ddsSample.j6(), 6.0);
    QCOMPARE(ddsSample.eax1(), 11.0);
    QCOMPARE(ddsSample.eax6(), 16.0);
    QCOMPARE(ddsSample.commanded_tcp_speed(), 100.0);
    QCOMPARE(ddsSample.commanded_orientation_speed(), 200.0);
    QCOMPARE(ddsSample.commanded_external_linear_speed(), 300.0);
    QCOMPARE(ddsSample.commanded_external_orientation_speed(), 400.0);
    QCOMPARE(ddsSample.estimated_tcp_speed(), 12.25);
}

QTEST_MAIN(TestRobotMonitorDdsTypes)
#include "test_robot_monitor_dds_types.moc"
```

- [ ] **Step 2: Add CMake target and verify failure**

Add:

```cmake
add_executable(test_robot_monitor_dds_types
    tests/test_robot_monitor_dds_types.cpp
)
target_include_directories(test_robot_monitor_dds_types PRIVATE
    src
    src/robot_monitor/generated
)
target_link_libraries(test_robot_monitor_dds_types PRIVATE Qt6::Core Qt6::Test)
copy_qt_test_runtime(test_robot_monitor_dds_types)
add_test(NAME test_robot_monitor_dds_types COMMAND test_robot_monitor_dds_types)
```

Run:

```powershell
cmake --build build --config Release --target test_robot_monitor_dds_types
```

Expected: compile fails because `RobotMonitorDdsTypes.h` does not exist.

- [ ] **Step 3: Implement mapping header**

Create `src/robot_monitor/RobotMonitorDdsTypes.h`:

```cpp
#pragma once

#include <cstdint>

#include "robot_monitor/RobotMonitorSample.h"
#include "robot_monitor/generated/RobotMonitorSample.hpp"

// Converts the hand-written robot monitor sample used by socket parsing into
// the generated Fast DDS sample type. Keeping this mapping in one header makes
// DDS field audits straightforward and keeps generated classes out of services.
inline AfpRobotMonitorSample toDdsSample(const RobotMonitorSample& sample) {
    AfpRobotMonitorSample ddsSample;
    ddsSample.timestamp_ms(static_cast<std::uint64_t>(sample.receivedAtUtc.toMSecsSinceEpoch()));
    ddsSample.sequence(sample.sequence < 0 ? 0ULL : static_cast<std::uint64_t>(sample.sequence));
    ddsSample.x(sample.x);
    ddsSample.y(sample.y);
    ddsSample.z(sample.z);
    ddsSample.q1(sample.q1);
    ddsSample.q2(sample.q2);
    ddsSample.q3(sample.q3);
    ddsSample.q4(sample.q4);
    ddsSample.j1(sample.joints[0]);
    ddsSample.j2(sample.joints[1]);
    ddsSample.j3(sample.joints[2]);
    ddsSample.j4(sample.joints[3]);
    ddsSample.j5(sample.joints[4]);
    ddsSample.j6(sample.joints[5]);
    ddsSample.eax1(sample.externalAxis[0]);
    ddsSample.eax2(sample.externalAxis[1]);
    ddsSample.eax3(sample.externalAxis[2]);
    ddsSample.eax4(sample.externalAxis[3]);
    ddsSample.eax5(sample.externalAxis[4]);
    ddsSample.eax6(sample.externalAxis[5]);
    ddsSample.commanded_tcp_speed(sample.commandedTcpSpeed);
    ddsSample.commanded_orientation_speed(sample.commandedOrientationSpeed);
    ddsSample.commanded_external_linear_speed(sample.commandedExternalLinearSpeed);
    ddsSample.commanded_external_orientation_speed(sample.commandedExternalOrientationSpeed);
    ddsSample.estimated_tcp_speed(sample.estimatedTcpSpeed);
    return ddsSample;
}
```

- [ ] **Step 4: Run mapping test**

Run:

```powershell
cmake --build build --config Release --target test_robot_monitor_dds_types
ctest -C Release -R test_robot_monitor_dds_types --output-on-failure
```

Expected: test passes.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt src/robot_monitor/RobotMonitorDdsTypes.h tests/test_robot_monitor_dds_types.cpp
git commit -m "robot: map monitor samples to dds type"
```

---

## Task 4: Add Robot Monitor DDS Publisher

**Files:**
- Create: `src/robot_monitor/RobotMonitorDdsPublisher.h`
- Create: `src/robot_monitor/RobotMonitorDdsPublisher.cpp`
- Modify: `CMakeLists.txt`
- Test: build target and later service tests

- [ ] **Step 1: Create publisher header**

Create `src/robot_monitor/RobotMonitorDdsPublisher.h`:

```cpp
#pragma once

#include <QString>

#include "app/AppConfig.h"
#include "robot_monitor/RobotMonitorSample.h"

namespace eprosima::fastdds::dds {
class DataWriter;
class DomainParticipant;
class Publisher;
class Topic;
class TypeSupport;
}

// Owns Fast DDS writer objects for robot monitor samples. This mirrors
// PressureDdsPublisher so future sensor streams follow one DDS pattern.
class RobotMonitorDdsPublisher final {
public:
    explicit RobotMonitorDdsPublisher(RobotMonitorConfig config);
    ~RobotMonitorDdsPublisher();

    bool initialize();
    bool publish(const RobotMonitorSample& sample);
    void shutdown();

    bool initialized() const;
    QString topicName() const;
    QString lastError() const;

private:
    void setError(const QString& message);

    RobotMonitorConfig config_;
    eprosima::fastdds::dds::DomainParticipant* participant_ = nullptr;
    eprosima::fastdds::dds::Publisher* publisher_ = nullptr;
    eprosima::fastdds::dds::Topic* topic_ = nullptr;
    eprosima::fastdds::dds::DataWriter* writer_ = nullptr;
    eprosima::fastdds::dds::TypeSupport* typeSupport_ = nullptr;
    QString lastError_;
};
```

- [ ] **Step 2: Create publisher implementation**

Create `src/robot_monitor/RobotMonitorDdsPublisher.cpp` by mirroring `PressureDdsPublisher.cpp`, changing type names:

```cpp
#include "robot_monitor/RobotMonitorDdsPublisher.h"

#include "robot_monitor/RobotMonitorDdsTypes.h"
#include "robot_monitor/generated/RobotMonitorSamplePubSubTypes.hpp"

#include <fastdds/dds/core/ReturnCode.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include <utility>

using eprosima::fastdds::dds::DATAWRITER_QOS_DEFAULT;
using eprosima::fastdds::dds::DomainParticipantFactory;
using eprosima::fastdds::dds::PARTICIPANT_QOS_DEFAULT;
using eprosima::fastdds::dds::PUBLISHER_QOS_DEFAULT;
using eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
using eprosima::fastdds::dds::RETCODE_OK;
using eprosima::fastdds::dds::TOPIC_QOS_DEFAULT;
using eprosima::fastdds::dds::TypeSupport;

namespace {
constexpr const char* kRobotMonitorTypeName = "AfpRobotMonitorSample";
}

RobotMonitorDdsPublisher::RobotMonitorDdsPublisher(RobotMonitorConfig config)
    : config_(std::move(config)) {
}

RobotMonitorDdsPublisher::~RobotMonitorDdsPublisher() {
    shutdown();
}

bool RobotMonitorDdsPublisher::initialize() {
    if (initialized()) {
        return true;
    }

    auto* factory = DomainParticipantFactory::get_instance();
    participant_ = factory->create_participant(config_.ddsDomainId, PARTICIPANT_QOS_DEFAULT);
    if (!participant_) {
        setError(QString("Could not create Fast DDS robot monitor participant in domain %1.")
                     .arg(config_.ddsDomainId));
        return false;
    }

    typeSupport_ = new TypeSupport(new AfpRobotMonitorSamplePubSubType());
    if (typeSupport_->register_type(participant_, kRobotMonitorTypeName) != RETCODE_OK) {
        setError("Could not register AfpRobotMonitorSample with Fast DDS.");
        shutdown();
        return false;
    }

    publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
    if (!publisher_) {
        setError("Could not create Fast DDS robot monitor publisher.");
        shutdown();
        return false;
    }

    topic_ = participant_->create_topic(config_.ddsTopic.toStdString(), kRobotMonitorTypeName, TOPIC_QOS_DEFAULT);
    if (!topic_) {
        setError(QString("Could not create Fast DDS robot monitor topic '%1'.").arg(config_.ddsTopic));
        shutdown();
        return false;
    }

    auto writerQos = DATAWRITER_QOS_DEFAULT;
    writerQos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    writer_ = publisher_->create_datawriter(topic_, writerQos, nullptr);
    if (!writer_) {
        setError("Could not create Fast DDS robot monitor data writer.");
        shutdown();
        return false;
    }

    lastError_.clear();
    return true;
}

bool RobotMonitorDdsPublisher::publish(const RobotMonitorSample& sample) {
    if (!initialize()) {
        return false;
    }

    AfpRobotMonitorSample ddsSample = toDdsSample(sample);
    if (writer_->write(&ddsSample) != RETCODE_OK) {
        setError("Fast DDS failed to publish a robot monitor sample.");
        return false;
    }

    return true;
}

void RobotMonitorDdsPublisher::shutdown() {
    if (participant_) {
        if (publisher_ && writer_) {
            publisher_->delete_datawriter(writer_);
            writer_ = nullptr;
        }
        if (publisher_) {
            participant_->delete_publisher(publisher_);
            publisher_ = nullptr;
        }
        if (topic_) {
            participant_->delete_topic(topic_);
            topic_ = nullptr;
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
        participant_ = nullptr;
    }

    delete typeSupport_;
    typeSupport_ = nullptr;
}

bool RobotMonitorDdsPublisher::initialized() const {
    return participant_ && publisher_ && topic_ && writer_;
}

QString RobotMonitorDdsPublisher::topicName() const {
    return config_.ddsTopic;
}

QString RobotMonitorDdsPublisher::lastError() const {
    return lastError_;
}

void RobotMonitorDdsPublisher::setError(const QString& message) {
    lastError_ = message;
}
```

- [ ] **Step 3: Add publisher files to CMake**

Add to `APP_SOURCES`:

```cmake
    src/robot_monitor/RobotMonitorDdsTypes.h
    src/robot_monitor/RobotMonitorDdsPublisher.h
    src/robot_monitor/RobotMonitorDdsPublisher.cpp
```

- [ ] **Step 4: Build the main app target**

Run:

```powershell
cmake --build build --config Release --target AFP_ABB_CPP
```

Expected: main app target builds.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt src/robot_monitor/RobotMonitorDdsPublisher.h src/robot_monitor/RobotMonitorDdsPublisher.cpp
git commit -m "robot: add monitor dds publisher"
```

---

## Task 5: Publish Robot Monitor Samples From Service

**Files:**
- Modify: `src/services/robot_monitor/RobotMonitoringService.h`
- Modify: `src/services/robot_monitor/RobotMonitoringService.cpp`
- Modify: `tests/test_robot_monitoring_service.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add test seam for DDS publisher**

Create a small interface in `src/robot_monitor/IRobotMonitorDdsPublisher.h`:

```cpp
#pragma once

#include <QString>

#include "robot_monitor/RobotMonitorSample.h"

class IRobotMonitorDdsPublisher {
public:
    virtual ~IRobotMonitorDdsPublisher() = default;
    virtual bool publish(const RobotMonitorSample& sample) = 0;
    virtual QString lastError() const = 0;
};
```

Make `RobotMonitorDdsPublisher` inherit this interface:

```cpp
class RobotMonitorDdsPublisher final : public IRobotMonitorDdsPublisher {
```

- [ ] **Step 2: Write service publish tests**

In `tests/test_robot_monitoring_service.cpp`, add a fake:

```cpp
class FakeRobotMonitorDdsPublisher final : public IRobotMonitorDdsPublisher {
public:
    QList<RobotMonitorSample> published;
    bool publishResult = true;
    QString error = "fake publish failed";

    bool publish(const RobotMonitorSample& sample) override {
        published.append(sample);
        return publishResult;
    }

    QString lastError() const override {
        return error;
    }
};
```

Add test declaration:

```cpp
void publishesSamplesToDdsWhenEnabled();
void doesNotPublishSamplesToDdsWhenDisabled();
```

Add test:

```cpp
void TestRobotMonitoringService::publishesSamplesToDdsWhenEnabled() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    AppConfig config;
    config.robot.ip = "127.0.0.1";
    config.robot.monitor.enabled = true;
    config.robot.monitor.autostart = true;
    config.robot.monitor.port = server.serverPort();
    config.robot.monitor.ddsPublishEnabled = true;
    config.robot.monitor.recordCsv = false;
    config.robot.monitor.recordRawMessages = false;

    auto fakePublisher = std::make_unique<FakeRobotMonitorDdsPublisher>();
    FakeRobotMonitorDdsPublisher* fakePublisherPtr = fakePublisher.get();

    RobotMonitoringService service(config, AppPaths(dir.path()), nullptr, std::move(fakePublisher));
    service.start();

    QVERIFY(QTest::qWaitFor([&]() { return server.hasPendingConnections(); }, 1000));
    QTcpSocket* socket = server.nextPendingConnection();
    QVERIFY(socket != nullptr);
    socket->setParent(&server);
    socket->write("#ROBOT_MONITOR s=3 x=1 y=2 z=3 v=4#");
    socket->flush();

    QVERIFY(QTest::qWaitFor([&]() { return fakePublisherPtr->published.size() == 1; }, 1000));
    QCOMPARE(fakePublisherPtr->published.first().sequence, 3);
    QCOMPARE(fakePublisherPtr->published.first().x, 1.0);
    QCOMPARE(fakePublisherPtr->published.first().estimatedTcpSpeed, 0.0);
}
```

Add test:

```cpp
void TestRobotMonitoringService::doesNotPublishSamplesToDdsWhenDisabled() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    AppConfig config;
    config.robot.ip = "127.0.0.1";
    config.robot.monitor.enabled = true;
    config.robot.monitor.autostart = true;
    config.robot.monitor.port = server.serverPort();
    config.robot.monitor.ddsPublishEnabled = false;

    auto fakePublisher = std::make_unique<FakeRobotMonitorDdsPublisher>();
    FakeRobotMonitorDdsPublisher* fakePublisherPtr = fakePublisher.get();

    RobotMonitoringService service(config, AppPaths(dir.path()), nullptr, std::move(fakePublisher));
    service.start();

    QVERIFY(QTest::qWaitFor([&]() { return server.hasPendingConnections(); }, 1000));
    QTcpSocket* socket = server.nextPendingConnection();
    QVERIFY(socket != nullptr);
    socket->setParent(&server);
    socket->write("#ROBOT_MONITOR s=4 x=5 y=6 z=7 v=8#");
    socket->flush();

    QTest::qWait(100);
    QCOMPARE(fakePublisherPtr->published.size(), 0);
}
```

- [ ] **Step 3: Run service test and verify failure**

Run:

```powershell
cmake --build build --config Release --target test_robot_monitoring_service
```

Expected: compile fails because constructor overload and publisher interface do not exist.

- [ ] **Step 4: Update service constructor and members**

In `RobotMonitoringService.h`, include:

```cpp
#include "robot_monitor/IRobotMonitorDdsPublisher.h"
```

Change constructor:

```cpp
RobotMonitoringService(
    const AppConfig& config,
    const AppPaths& paths,
    QObject* parent = nullptr,
    std::unique_ptr<IRobotMonitorDdsPublisher> ddsPublisher = nullptr);
```

Add member:

```cpp
std::unique_ptr<IRobotMonitorDdsPublisher> ddsPublisher_;
```

In `RobotMonitoringService.cpp`, include:

```cpp
#include "robot_monitor/RobotMonitorDdsPublisher.h"
```

Initialize:

```cpp
RobotMonitoringService::RobotMonitoringService(
    const AppConfig& config,
    const AppPaths& paths,
    QObject* parent,
    std::unique_ptr<IRobotMonitorDdsPublisher> ddsPublisher)
    : QObject(parent),
      config_(config),
      paths_(paths),
      client_(this),
      ddsPublisher_(std::move(ddsPublisher)) {
    if (config_.robot.monitor.ddsPublishEnabled && !ddsPublisher_) {
        ddsPublisher_ = std::make_unique<RobotMonitorDdsPublisher>(config_.robot.monitor);
    }
```

- [ ] **Step 5: Publish after estimated speed update**

In the `sampleReceived` connection, after `updateEstimatedSpeed(sample);`, add:

```cpp
        if (config_.robot.monitor.ddsPublishEnabled && ddsPublisher_ && !ddsPublisher_->publish(sample)) {
            reportStatus(QString("Robot monitor DDS publish warning: %1").arg(ddsPublisher_->lastError()));
        }
```

- [ ] **Step 6: Update CMake test target**

Add to `test_robot_monitoring_service` sources:

```cmake
    src/robot_monitor/IRobotMonitorDdsPublisher.h
    src/robot_monitor/RobotMonitorDdsTypes.h
    src/robot_monitor/RobotMonitorDdsPublisher.h
    src/robot_monitor/RobotMonitorDdsPublisher.cpp
    src/robot_monitor/generated/RobotMonitorSamplePubSubTypes.cxx
    src/robot_monitor/generated/RobotMonitorSampleTypeObjectSupport.cxx
```

Add include:

```cmake
    src/robot_monitor/generated
```

Add link library:

```cmake
fastdds
```

- [ ] **Step 7: Run tests**

Run:

```powershell
cmake --build build --config Release --target test_robot_monitoring_service
ctest -C Release -R test_robot_monitoring_service --output-on-failure
```

Expected: service tests pass.

- [ ] **Step 8: Commit**

```powershell
git add CMakeLists.txt src/robot_monitor/IRobotMonitorDdsPublisher.h src/services/robot_monitor/RobotMonitoringService.h src/services/robot_monitor/RobotMonitoringService.cpp tests/test_robot_monitoring_service.cpp
git commit -m "robot: publish monitor samples to dds"
```

---

## Task 6: Add Robot Monitor DDS Reader And Influx Mapping

**Files:**
- Create: `src/archive/RobotMonitorSampleReader.h`
- Create: `src/archive/RobotMonitorSampleReader.cpp`
- Modify: `tests/test_time_series_point.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add mapping test in `test_time_series_point.cpp`**

Include:

```cpp
#include "archive/RobotMonitorSampleReader.h"
```

Add declaration:

```cpp
void convertsRobotMonitorSampleToPoint();
```

Add test:

```cpp
void TestTimeSeriesPoint::convertsRobotMonitorSampleToPoint() {
    RobotMonitorArchiveConfig config;
    config.table = "robot_monitor";
    config.sourceTag = "abb_irb4600";

    AfpRobotMonitorSample sample;
    sample.timestamp_ms(1234);
    sample.sequence(7);
    sample.x(10.5);
    sample.y(20.5);
    sample.z(30.5);
    sample.q1(1.0);
    sample.q2(0.1);
    sample.q3(0.2);
    sample.q4(0.3);
    sample.j1(1);
    sample.j6(6);
    sample.eax1(11);
    sample.eax6(16);
    sample.commanded_tcp_speed(100.0);
    sample.estimated_tcp_speed(12.25);

    const TimeSeriesPoint point = robotMonitorSampleToPoint(sample, config);

    QCOMPARE(point.table, QString("robot_monitor"));
    QCOMPARE(point.tags.value("source"), QString("abb_irb4600"));
    QCOMPARE(point.integerFields.value("sequence"), 7);
    QCOMPARE(point.floatFields.value("x"), 10.5);
    QCOMPARE(point.floatFields.value("z"), 30.5);
    QCOMPARE(point.floatFields.value("q2"), 0.1);
    QCOMPARE(point.floatFields.value("j1"), 1.0);
    QCOMPARE(point.floatFields.value("j6"), 6.0);
    QCOMPARE(point.floatFields.value("eax1"), 11.0);
    QCOMPARE(point.floatFields.value("eax6"), 16.0);
    QCOMPARE(point.floatFields.value("commanded_tcp_speed"), 100.0);
    QCOMPARE(point.floatFields.value("estimated_tcp_speed"), 12.25);
    QCOMPARE(point.timestampNs, 1234000000ULL);
}
```

- [ ] **Step 2: Verify compile failure**

Run:

```powershell
cmake --build build --config Release --target test_time_series_point
```

Expected: compile fails because `RobotMonitorSampleReader.h` does not exist.

- [ ] **Step 3: Create reader header**

Create `src/archive/RobotMonitorSampleReader.h`:

```cpp
#pragma once

#include <functional>

#include <QString>

#include "app/AppConfig.h"
#include "archive/TimeSeriesPoint.h"
#include "robot_monitor/generated/RobotMonitorSample.hpp"

namespace eprosima::fastdds::dds {
class DataReader;
class DomainParticipant;
class Subscriber;
class Topic;
class TypeSupport;
}

TimeSeriesPoint robotMonitorSampleToPoint(
    const AfpRobotMonitorSample& sample,
    const RobotMonitorArchiveConfig& config);

class RobotMonitorSampleReader final {
public:
    RobotMonitorSampleReader(RobotMonitorArchiveConfig config, TimeSeriesPointHandler handler);
    ~RobotMonitorSampleReader();

    RobotMonitorSampleReader(const RobotMonitorSampleReader&) = delete;
    RobotMonitorSampleReader& operator=(const RobotMonitorSampleReader&) = delete;

    bool start();
    void stop();
    QString lastError() const;

private:
    class Listener;

    void setError(const QString& message);

    RobotMonitorArchiveConfig config_;
    TimeSeriesPointHandler handler_;
    eprosima::fastdds::dds::DomainParticipant* participant_ = nullptr;
    eprosima::fastdds::dds::Subscriber* subscriber_ = nullptr;
    eprosima::fastdds::dds::Topic* topic_ = nullptr;
    eprosima::fastdds::dds::DataReader* reader_ = nullptr;
    eprosima::fastdds::dds::TypeSupport* typeSupport_ = nullptr;
    Listener* listener_ = nullptr;
    QString lastError_;
};
```

- [ ] **Step 4: Create reader implementation**

Create `src/archive/RobotMonitorSampleReader.cpp` by mirroring `PressureSampleReader.cpp`, changing type/config names. Use this mapping body:

```cpp
TimeSeriesPoint robotMonitorSampleToPoint(
    const AfpRobotMonitorSample& sample,
    const RobotMonitorArchiveConfig& config) {
    TimeSeriesPoint point;
    point.table = config.table;
    point.tags.insert("source", config.sourceTag);
    point.integerFields.insert(
        "sequence",
        sample.sequence() > static_cast<std::uint64_t>(std::numeric_limits<qint64>::max())
            ? std::numeric_limits<qint64>::max()
            : static_cast<qint64>(sample.sequence()));
    point.floatFields.insert("x", sample.x());
    point.floatFields.insert("y", sample.y());
    point.floatFields.insert("z", sample.z());
    point.floatFields.insert("q1", sample.q1());
    point.floatFields.insert("q2", sample.q2());
    point.floatFields.insert("q3", sample.q3());
    point.floatFields.insert("q4", sample.q4());
    point.floatFields.insert("j1", sample.j1());
    point.floatFields.insert("j2", sample.j2());
    point.floatFields.insert("j3", sample.j3());
    point.floatFields.insert("j4", sample.j4());
    point.floatFields.insert("j5", sample.j5());
    point.floatFields.insert("j6", sample.j6());
    point.floatFields.insert("eax1", sample.eax1());
    point.floatFields.insert("eax2", sample.eax2());
    point.floatFields.insert("eax3", sample.eax3());
    point.floatFields.insert("eax4", sample.eax4());
    point.floatFields.insert("eax5", sample.eax5());
    point.floatFields.insert("eax6", sample.eax6());
    point.floatFields.insert("commanded_tcp_speed", sample.commanded_tcp_speed());
    point.floatFields.insert("commanded_orientation_speed", sample.commanded_orientation_speed());
    point.floatFields.insert("commanded_external_linear_speed", sample.commanded_external_linear_speed());
    point.floatFields.insert("commanded_external_orientation_speed", sample.commanded_external_orientation_speed());
    point.floatFields.insert("estimated_tcp_speed", sample.estimated_tcp_speed());
    constexpr std::uint64_t kNsPerMs = 1000000ULL;
    point.timestampNs = sample.timestamp_ms() <= std::numeric_limits<std::uint64_t>::max() / kNsPerMs
        ? sample.timestamp_ms() * kNsPerMs
        : 0;
    return point;
}
```

Use type name:

```cpp
constexpr const char* kRobotMonitorTypeName = "AfpRobotMonitorSample";
```

Use generated type:

```cpp
typeSupport_ = new TypeSupport(new AfpRobotMonitorSamplePubSubType());
```

- [ ] **Step 5: Update CMake for reader tests**

Update `test_time_series_point` sources:

```cmake
    src/archive/RobotMonitorSampleReader.cpp
    src/robot_monitor/generated/RobotMonitorSamplePubSubTypes.cxx
    src/robot_monitor/generated/RobotMonitorSampleTypeObjectSupport.cxx
```

Add include:

```cmake
    src/robot_monitor/generated
```

- [ ] **Step 6: Run tests**

Run:

```powershell
cmake --build build --config Release --target test_time_series_point
ctest -C Release -R test_time_series_point --output-on-failure
```

Expected: `test_time_series_point` passes.

- [ ] **Step 7: Commit**

```powershell
git add CMakeLists.txt src/archive/RobotMonitorSampleReader.h src/archive/RobotMonitorSampleReader.cpp tests/test_time_series_point.cpp
git commit -m "archive: map robot monitor samples to influx points"
```

---

## Task 7: Extend Shared DDS Influx Subscriber

**Files:**
- Modify: `src/archive/DdsInfluxSubscriberApp.h`
- Modify: `src/archive/DdsInfluxSubscriberApp.cpp`
- Modify: `CMakeLists.txt`
- Test: existing and new subscriber-app behavior through unit tests

- [ ] **Step 1: Update subscriber app members**

In `DdsInfluxSubscriberApp.h`, forward declare:

```cpp
class RobotMonitorSampleReader;
```

Add member:

```cpp
std::unique_ptr<RobotMonitorSampleReader> robotMonitorReader_;
```

- [ ] **Step 2: Update startup guards**

In `DdsInfluxSubscriberApp::start()`, replace:

```cpp
    if (!config_.pressureArchive.enabled) {
        lastError_ = "Pressure archive is disabled.";
        return false;
    }
```

with:

```cpp
    if (!config_.pressureArchive.enabled && !config_.robotMonitorArchive.enabled) {
        lastError_ = "No DDS archive readers are enabled.";
        return false;
    }
```

- [ ] **Step 3: Start pressure reader only when enabled**

Wrap pressure reader creation/start:

```cpp
    if (config_.pressureArchive.enabled) {
        pressureReader_ = std::make_unique<PressureSampleReader>(
            config_.pressureArchive,
            [this](const TimeSeriesPoint& point) {
                QMetaObject::invokeMethod(
                    this,
                    [this, point]() {
                        handlePoint(point);
                    },
                    Qt::QueuedConnection);
            });

        if (!pressureReader_->start()) {
            flushTimer_.stop();
            lastError_ = pressureReader_->lastError().isEmpty()
                ? QString("Could not start pressure DDS reader.")
                : pressureReader_->lastError();
            pressureReader_.reset();
            writer_.reset();
            return false;
        }
    }
```

- [ ] **Step 4: Start robot monitor reader when enabled**

Include:

```cpp
#include "archive/RobotMonitorSampleReader.h"
```

After pressure reader startup, add:

```cpp
    if (config_.robotMonitorArchive.enabled) {
        robotMonitorReader_ = std::make_unique<RobotMonitorSampleReader>(
            config_.robotMonitorArchive,
            [this](const TimeSeriesPoint& point) {
                QMetaObject::invokeMethod(
                    this,
                    [this, point]() {
                        handlePoint(point);
                    },
                    Qt::QueuedConnection);
            });

        if (!robotMonitorReader_->start()) {
            flushTimer_.stop();
            lastError_ = robotMonitorReader_->lastError().isEmpty()
                ? QString("Could not start robot monitor DDS reader.")
                : robotMonitorReader_->lastError();
            robotMonitorReader_.reset();
            pressureReader_.reset();
            writer_.reset();
            return false;
        }
    }
```

- [ ] **Step 5: Stop both readers**

In `stop()`, after pressure reader stop, add:

```cpp
    if (robotMonitorReader_) {
        robotMonitorReader_->stop();
        robotMonitorReader_.reset();
    }
```

- [ ] **Step 6: Update status log**

Replace single pressure startup log with:

```cpp
    qInfo().noquote() << QString("DDS InfluxDB subscriber started. pressure=%1 robot_monitor=%2")
                             .arg(config_.pressureArchive.enabled ? "enabled" : "disabled")
                             .arg(config_.robotMonitorArchive.enabled ? "enabled" : "disabled");
```

- [ ] **Step 7: Add reader to archive common sources**

In `ARCHIVE_COMMON_SOURCES`, add:

```cmake
    src/archive/RobotMonitorSampleReader.h
    src/archive/RobotMonitorSampleReader.cpp
```

- [ ] **Step 8: Build subscriber**

Run:

```powershell
cmake --build build --config Release --target dds_influx_subscriber
```

Expected: subscriber builds.

- [ ] **Step 9: Run related tests**

Run:

```powershell
ctest -C Release -R "test_time_series_point|test_influx_line_protocol_writer" --output-on-failure
```

Expected: tests pass.

- [ ] **Step 10: Commit**

```powershell
git add CMakeLists.txt src/archive/DdsInfluxSubscriberApp.h src/archive/DdsInfluxSubscriberApp.cpp
git commit -m "archive: subscribe to robot monitor dds samples"
```

---

## Task 8: CMake Integration And Full Build

**Files:**
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Ensure all app sources include robot DDS files**

Confirm `APP_SOURCES` contains:

```cmake
    src/robot_monitor/RobotMonitorDdsTypes.h
    src/robot_monitor/RobotMonitorDdsPublisher.h
    src/robot_monitor/RobotMonitorDdsPublisher.cpp
    src/robot_monitor/generated/RobotMonitorSample.hpp
    src/robot_monitor/generated/RobotMonitorSamplePubSubTypes.cxx
    src/robot_monitor/generated/RobotMonitorSampleTypeObjectSupport.cxx
```

- [ ] **Step 2: Ensure archive executable includes robot reader files**

Confirm `ARCHIVE_COMMON_SOURCES` contains:

```cmake
    src/archive/RobotMonitorSampleReader.h
    src/archive/RobotMonitorSampleReader.cpp
    src/robot_monitor/generated/RobotMonitorSample.hpp
    src/robot_monitor/generated/RobotMonitorSamplePubSubTypes.cxx
    src/robot_monitor/generated/RobotMonitorSampleTypeObjectSupport.cxx
```

- [ ] **Step 3: Build main and subscriber**

Run:

```powershell
cmake --build build --config Release --target AFP_ABB_CPP
cmake --build build --config Release --target dds_influx_subscriber
```

Expected: both targets build.

- [ ] **Step 4: Verify runtime copy**

Run:

```powershell
Test-Path build\Release\dds_influx_subscriber.exe
Test-Path build\Release\config\env_config.json
```

Expected: both print `True`.

- [ ] **Step 5: Commit if CMake-only fixes were needed**

If Task 8 changed CMake beyond earlier tasks:

```powershell
git add CMakeLists.txt
git commit -m "build: wire robot monitor dds archive targets"
```

If no changes were needed, do not create an empty commit.

---

## Task 9: README And Validation Notes

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Document robot monitor DDS generated files**

Add near the pressure Fast DDS documentation:

```markdown
Slice L uses eProsima Fast DDS for robot monitor sample publishing. The robot monitor sample contract is defined in:

```text
src\robot_monitor\RobotMonitorSample.idl
```

The files in `src\robot_monitor\generated` are generated from that IDL with Fast DDS-Gen. Do not edit generated files by hand; edit the IDL and regenerate from a Visual Studio Developer PowerShell:

```powershell
fastddsgen -replace -d src\robot_monitor\generated src\robot_monitor\RobotMonitorSample.idl
```
```

- [ ] **Step 2: Document robot archive config**

Add to the robot monitor section:

```markdown
DDS/InfluxDB is the default robot monitor archive path:

```json
"robot_monitor_archive": {
  "enabled": true,
  "dds_domain_id": 0,
  "dds_topic": "afp.robot.monitor.sample",
  "table": "robot_monitor",
  "source_tag": "abb_irb4600"
}
```

`robot.monitor.dds_publish_enabled` controls whether the main app publishes parsed robot monitor samples. `robot_monitor_archive.enabled` controls whether `dds_influx_subscriber.exe` subscribes to that topic and writes rows to InfluxDB.

CSV and raw socket logs are diagnostic only and are disabled by default:

```json
"record_csv": false,
"record_raw_messages": false
```
```

- [ ] **Step 3: Document validation procedure**

Add:

```markdown
Manual robot monitor DDS/InfluxDB validation:

1. Confirm InfluxDB is running or `timeseries_archive.start_influxdb` is true.
2. Confirm `timeseries_archive.subscriber_autostart` is true.
3. Confirm `robot.monitor.dds_topic` matches `robot_monitor_archive.dds_topic`.
4. Start `AFP_ABB_CPP.exe`.
5. Start the RAPID TASK2 robot monitor program.
6. If live printing is enabled, confirm the subscriber console prints `robot_monitor,...` rows.
7. Query InfluxDB table `robot_monitor`.
8. Start pressure acquisition and confirm pressure rows still appear in `pressure_sensor`.
```

- [ ] **Step 4: Commit**

```powershell
git add README.md
git commit -m "docs: document robot monitor dds archive"
```

---

## Task 10: Full Verification And Branch Finish

**Files:**
- All changed files

- [ ] **Step 1: Configure**

Run:

```powershell
cmake -S . -B build
```

Expected: configure succeeds.

- [ ] **Step 2: Build Release**

Run:

```powershell
cmake --build build --config Release
```

Expected: build succeeds. If the full Visual Studio build runs longer than the tool timeout but child `MSBuild` processes continue, wait for them to complete and then continue verification.

- [ ] **Step 3: Run full tests**

Run:

```powershell
ctest -C Release --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 4: Check worktree**

Run:

```powershell
git status --short --branch
```

Expected: only known unrelated high-speed-camera runtime files and `build/` remain dirty/untracked. No source changes should be unstaged.

- [ ] **Step 5: Ask before merge**

Report:

```text
Slice L implementation is complete on codex/slice-l-robot-monitor-dds-influx.
Release tests pass.
Options:
1. Merge to master locally
2. Keep branch for hardware testing
3. Push/create PR later
```

Do not merge unless the user chooses option 1.

---

## Self-Review Notes

- Spec coverage: all approved Slice L design sections map to tasks above.
- No second subscriber executable is introduced.
- The existing `InfluxLineProtocolWriter` remains the only Influx writer.
- Pressure archive behavior remains supported.
- Robot CSV/raw defaults are changed to diagnostic-only in Task 1.
- Timestamp source is `RobotMonitorSample::receivedAtUtc`, mapped to `timestamp_ms` before DDS publication.
- Full RAPID monitor-message expansion is explicitly excluded.
