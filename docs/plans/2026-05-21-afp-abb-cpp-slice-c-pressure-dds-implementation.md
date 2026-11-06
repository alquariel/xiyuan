# AFP ABB C++ Slice C Pressure DDS Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement pressure sensor acquisition in C++, publish raw and processed pressure values through eProsima Fast DDS, keep the original UI design unchanged, and make user-tunable numeric pressure parameters configurable.

**Architecture:** The pressure slice is split into focused units: config loading, pressure statistics, pressure DLL access, DDS publishing, acquisition worker, and Qt service wiring. The UI keeps the existing buttons and pressure widgets, but pressure curves are published externally instead of drawn in the main window.

**Tech Stack:** C++17, Qt Widgets/Qt Test, CMake, vcpkg, eProsima Fast DDS/Fast CDR/Fast DDS-Gen, vendor `pressure.dll`, existing `AppConfig`, existing portable deployment target.

---

## File Structure

- Modify `config/env_config.json`: add a `pressure` section with Python-compatible defaults.
- Modify `src/app/AppConfig.h`: add `PressureConfig`.
- Modify `src/app/AppConfig.cpp`: parse and validate pressure config values.
- Modify `tests/test_app_config.cpp`: test pressure config loading and fallback.
- Create `src/pressure/PressureStatistics.h`.
- Create `src/pressure/PressureStatistics.cpp`.
- Create `tests/test_pressure_statistics.cpp`.
- Create `src/pressure/PressureSample.idl`.
- Create generated files under `src/pressure/generated/` after running Fast DDS-Gen. Fast DDS-Gen v4.0.6 emits `.hpp`, `.ipp`, and `.cxx` files for this IDL; there is no separate `PressureSample.cxx` file.
- Create `src/pressure/PressureDdsTypes.h`: tiny conversion helper between internal values and generated DDS type.
- Create `tests/test_pressure_dds_types.cpp`.
- Create `src/pressure/PressureSensorClient.h`.
- Create `src/pressure/PressureSensorClient.cpp`.
- Create `tests/test_pressure_sensor_client.cpp`.
- Create `src/pressure/PressureDdsPublisher.h`.
- Create `src/pressure/PressureDdsPublisher.cpp`.
- Create `src/pressure/PressureAcquisitionWorker.h`.
- Create `src/pressure/PressureAcquisitionWorker.cpp`.
- Create `src/services/pressure/PressureService.h`.
- Create `src/services/pressure/PressureService.cpp`.
- Modify `src/services/PlaceholderServices.h`: remove the placeholder `PressureService` class.
- Modify `src/services/PlaceholderServices.cpp`: remove placeholder pressure methods.
- Modify `src/ui/MainWindow.h`: include the real pressure service.
- Modify `src/ui/MainWindow.cpp`: pass config/paths/UI to the real pressure service and wire `FC_machine`.
- Modify `CMakeLists.txt`: add Fast DDS packages, generated DDS files, pressure source files, and pressure tests.
- Modify `cmake/DeployQt.cmake`: copy pressure runtime DLL/config files and Fast DDS runtime DLLs.
- Modify `README.md`: document pressure config, DDS dependency, visualizer/subscriber workflow, and portable runtime files.

## Implementation Notes

- Do not modify `AFP_ABB_Python`.
- Do not modify `src/ui/Interface4600_window.ui`.
- Add explanatory comments in new C++ files, especially where code maps Python behavior or wraps external DLL/DDS APIs.
- For this and future slices, inspect source-code numeric parameters and decide whether they belong in `config/env_config.json` before writing C++ implementation code.
- The generated `ui_Interface4600_window.h` remains a build artifact and must not be edited.

---

### Task 1: Dependency And Baseline Check

**Files:**
- Read-only check: `AFP_ABB_Python/src/drivers/sensors/pressure_reader.py`
- Read-only check: `AFP_ABB_Python/src/services/sensor_service/pressure_monitor.py`
- Read-only check: `AFP_ABB_Python/src/ui/ForceDataPlot.py`
- Read-only unless dependency discovery requires a note: `README.md`

- [ ] **Step 1: Confirm branch and working tree**

Run:

```powershell
git status --short --branch
```

Expected:

```text
## slice-c-pressure-dds
?? build/
```

Only `build/` should be untracked before implementation begins.

- [ ] **Step 2: Reconfirm Python numeric parameters**

Run from `AFP_ABB_Python`:

```powershell
rg -n "sleep\(0\.1\)|\[-40:\]|/ 40|start\(1000\)" src\drivers\sensors\pressure_reader.py src\services\sensor_service\pressure_monitor.py src\ui\ForceDataPlot.py
```

Expected evidence:

```text
src\drivers\sensors\pressure_reader.py:14:            time.sleep(0.1)
src\services\sensor_service\pressure_monitor.py:75:            recent_data = self.y_list[-40:]
src\services\sensor_service\pressure_monitor.py:78:            mean_val = sum(recent_data) / 40
src\services\sensor_service\pressure_monitor.py:82:            var_val = sum((x - mean_val) ** 2 for x in recent_data) / 40
src\ui\ForceDataPlot.py:85:    self.timer1.start(1000)
```

Decision:

- `0.1` seconds becomes `pressure.sample_interval_ms`, default `100`.
- `40` samples becomes `pressure.processing_window_samples`, default `40`.
- `1000` ms plot refresh is not ported because Slice C does not implement embedded plotting.

- [ ] **Step 3: Install or verify Fast DDS dependency**

Run:

```powershell
C:\vcpkg\vcpkg.exe list fastdds
C:\vcpkg\vcpkg.exe install fastdds:x64-windows
```

Expected:

```text
fastdds:x64-windows ...
fastcdr:x64-windows ...
```

If vcpkg reports the package cannot be installed for `x64-windows`, stop and report the exact error. Do not replace Fast DDS with another middleware.

- [ ] **Step 4: Verify Fast DDS-Gen availability**

Run:

```powershell
Get-ChildItem C:\vcpkg\installed\x64-windows -Recurse -Filter fastddsgen*.bat
Get-ChildItem C:\vcpkg\installed\x64-windows -Recurse -Filter fastddsgen*.exe
```

Expected: at least one Fast DDS-Gen executable or batch file exists.

Task 1 result: vcpkg `fastdds:x64-windows` installed Fast DDS and Fast CDR successfully, but did not provide `fastddsgen`. The official eProsima Fast DDS-Gen source tag `v4.0.6` was cloned and built separately at:

```text
C:\Tools\Fast-DDS-Gen-v4.0.6\scripts\fastddsgen.bat
```

OpenJDK 17 was installed at:

```text
C:\Program Files\Microsoft\jdk-17.0.19.10-hotspot
```

When invoking `fastddsgen.bat`, set `JAVA_HOME` to this JDK and prepend `%JAVA_HOME%\bin` to `PATH`, because the system `java` command may still resolve to Java 8.

Run the generator from a Visual Studio Developer PowerShell or Developer Command Prompt so `cl.exe` is available for the IDL preprocessor. A normal PowerShell session may fail with `Cannot run program "cl.exe"`.

- [ ] **Step 5: Baseline build and test**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64;C:\vcpkg\installed\x64-windows"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Expected:

```text
100% tests passed
```

- [ ] **Step 6: Commit dependency notes if README changed**

If README was changed because Fast DDS-Gen has a non-obvious path:

```powershell
git add README.md
git commit -m "docs: note fast dds generator path"
```

---

### Task 2: Add Pressure Configuration

**Files:**
- Modify: `config/env_config.json`
- Modify: `src/app/AppConfig.h`
- Modify: `src/app/AppConfig.cpp`
- Modify: `tests/test_app_config.cpp`

- [ ] **Step 1: Write failing config tests**

Add these test methods to `tests/test_app_config.cpp`:

```cpp
void loadsPressureConfig() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "pressure": {
            "sample_interval_ms": 250,
            "processing_window_samples": 12,
            "dds_domain_id": 3,
            "dds_topic": "afp.test.pressure"
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.pressure.sampleIntervalMs, 250);
    QCOMPARE(config.pressure.processingWindowSamples, 12);
    QCOMPARE(config.pressure.ddsDomainId, 3);
    QCOMPARE(config.pressure.ddsTopic, QString("afp.test.pressure"));
}

void invalidPressureConfigFallsBackToDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "pressure": {
            "sample_interval_ms": 0,
            "processing_window_samples": -2,
            "dds_domain_id": -1,
            "dds_topic": ""
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.pressure.sampleIntervalMs, 100);
    QCOMPARE(config.pressure.processingWindowSamples, 40);
    QCOMPARE(config.pressure.ddsDomainId, 0);
    QCOMPARE(config.pressure.ddsTopic, QString("afp.pressure.sample"));
    QVERIFY(config.warnings().join('\n').contains("pressure.sample_interval_ms"));
    QVERIFY(config.warnings().join('\n').contains("pressure.processing_window_samples"));
    QVERIFY(config.warnings().join('\n').contains("pressure.dds_domain_id"));
    QVERIFY(config.warnings().join('\n').contains("pressure.dds_topic"));
}
```

Declare the methods in the `private slots:` section before implementing them.

- [ ] **Step 2: Run config test and verify failure**

Run:

```powershell
cmake --build build --target test_app_config --config Release
.\build\Release\test_app_config.exe
```

Expected: compile fails because `AppConfig::pressure` does not exist.

- [ ] **Step 3: Add `PressureConfig`**

Add to `src/app/AppConfig.h` after `SerialConfig`:

```cpp
struct PressureConfig {
    // Matches pressure_reader.py time.sleep(0.1), but lives in config so users
    // can tune acquisition without recompiling the C++ application.
    int sampleIntervalMs = 100;

    // Matches pressure_monitor.py's 40-sample rolling processing window.
    int processingWindowSamples = 40;

    // Fast DDS defaults. Domain 0 is the standard local discovery domain.
    int ddsDomainId = 0;
    QString ddsTopic = "afp.pressure.sample";
};
```

Add a member inside `class AppConfig`:

```cpp
PressureConfig pressure;
```

- [ ] **Step 4: Add config validation helpers**

Add these helpers inside the anonymous namespace in `src/app/AppConfig.cpp`:

```cpp
int readPositiveInt(AppConfig& config,
                    const QJsonObject& object,
                    const QString& key,
                    int fallback,
                    const QString& warningName) {
    const QJsonValue value = object.value(key);
    if (!value.isDouble()) {
        return fallback;
    }
    const int parsed = value.toInt();
    if (parsed <= 0) {
        config.addWarning(QString("%1 must be greater than 0; using %2").arg(warningName).arg(fallback));
        return fallback;
    }
    return parsed;
}

int readNonNegativeInt(AppConfig& config,
                       const QJsonObject& object,
                       const QString& key,
                       int fallback,
                       const QString& warningName) {
    const QJsonValue value = object.value(key);
    if (!value.isDouble()) {
        return fallback;
    }
    const int parsed = value.toInt();
    if (parsed < 0) {
        config.addWarning(QString("%1 must be non-negative; using %2").arg(warningName).arg(fallback));
        return fallback;
    }
    return parsed;
}

QString readNonEmptyString(AppConfig& config,
                           const QJsonObject& object,
                           const QString& key,
                           const QString& fallback,
                           const QString& warningName) {
    const QJsonValue value = object.value(key);
    if (!value.isString()) {
        return fallback;
    }
    const QString parsed = value.toString().trimmed();
    if (parsed.isEmpty()) {
        config.addWarning(QString("%1 must be non-empty; using %2").arg(warningName, fallback));
        return fallback;
    }
    return parsed;
}
```

Add this public method to `AppConfig` in `src/app/AppConfig.h`:

```cpp
void addWarning(const QString& warning);
```

Add implementation in `src/app/AppConfig.cpp`:

```cpp
void AppConfig::addWarning(const QString& warning) {
    warnings_ << warning;
}
```

- [ ] **Step 5: Parse pressure config**

Add this block before `config.loaded_ = true;` in `AppConfig::load`:

```cpp
const QJsonObject pressure = root.value("pressure").toObject();
config.pressure.sampleIntervalMs = readPositiveInt(
    config, pressure, "sample_interval_ms", config.pressure.sampleIntervalMs, "pressure.sample_interval_ms");
config.pressure.processingWindowSamples = readPositiveInt(
    config, pressure, "processing_window_samples", config.pressure.processingWindowSamples, "pressure.processing_window_samples");
config.pressure.ddsDomainId = readNonNegativeInt(
    config, pressure, "dds_domain_id", config.pressure.ddsDomainId, "pressure.dds_domain_id");
config.pressure.ddsTopic = readNonEmptyString(
    config, pressure, "dds_topic", config.pressure.ddsTopic, "pressure.dds_topic");
```

- [ ] **Step 6: Add defaults to `env_config.json`**

Add this top-level section after `plc`:

```json
"pressure": {
    "_source_note": "Defaults match Python: pressure_reader.py sleep(0.1), pressure_monitor.py 40-sample processing window.",
    "sample_interval_ms": 100,
    "processing_window_samples": 40,
    "dds_domain_id": 0,
    "dds_topic": "afp.pressure.sample"
},
```

- [ ] **Step 7: Run config test**

Run:

```powershell
cmake --build build --target test_app_config --config Release
.\build\Release\test_app_config.exe
```

Expected:

```text
Totals: ... passed, 0 failed
```

- [ ] **Step 8: Commit**

Run:

```powershell
git add config/env_config.json src/app/AppConfig.h src/app/AppConfig.cpp tests/test_app_config.cpp
git commit -m "feat: add pressure runtime config"
```

---

### Task 3: Add Pressure Statistics

**Files:**
- Create: `src/pressure/PressureStatistics.h`
- Create: `src/pressure/PressureStatistics.cpp`
- Create: `tests/test_pressure_statistics.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing statistics tests**

Create `tests/test_pressure_statistics.cpp`:

```cpp
#include <QtTest/QtTest>

#include "pressure/PressureStatistics.h"

class TestPressureStatistics final : public QObject {
    Q_OBJECT

private slots:
    void windowIsNotReadyBeforeConfiguredSampleCount();
    void computesMeanVarianceStddevAndPeak();
    void keepsOnlyNewestSamples();
};

void TestPressureStatistics::windowIsNotReadyBeforeConfiguredSampleCount() {
    PressureStatistics stats(4);

    stats.addSample(1.0);
    stats.addSample(2.0);
    stats.addSample(3.0);

    QVERIFY(!stats.windowReady());
    QCOMPARE(stats.sampleCount(), 3);
}

void TestPressureStatistics::computesMeanVarianceStddevAndPeak() {
    PressureStatistics stats(4);

    stats.addSample(1.0);
    stats.addSample(2.0);
    stats.addSample(3.0);
    stats.addSample(4.0);

    QVERIFY(stats.windowReady());
    QCOMPARE(stats.sampleCount(), 4);
    QCOMPARE(stats.mean(), 2.5);
    QCOMPARE(stats.variance(), 1.25);
    QCOMPARE(stats.standardDeviation(), std::sqrt(1.25));
    QCOMPARE(stats.peak(), 4.0);
}

void TestPressureStatistics::keepsOnlyNewestSamples() {
    PressureStatistics stats(4);

    stats.addSample(1.0);
    stats.addSample(2.0);
    stats.addSample(3.0);
    stats.addSample(4.0);
    stats.addSample(100.0);

    QVERIFY(stats.windowReady());
    QCOMPARE(stats.sampleCount(), 4);
    QCOMPARE(stats.mean(), 27.25);
    QCOMPARE(stats.peak(), 100.0);
}

QTEST_MAIN(TestPressureStatistics)
#include "test_pressure_statistics.moc"
```

- [ ] **Step 2: Register the test target**

Add to `CMakeLists.txt` after the S7 data test block:

```cmake
set(TEST_PRESSURE_STATISTICS_SOURCES
    tests/test_pressure_statistics.cpp
    src/pressure/PressureStatistics.cpp
)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_pressure_statistics.cpp")
    add_executable(test_pressure_statistics ${TEST_PRESSURE_STATISTICS_SOURCES})
    target_include_directories(test_pressure_statistics PRIVATE src)
    target_link_libraries(test_pressure_statistics PRIVATE Qt6::Core Qt6::Test)
    copy_qt_test_runtime(test_pressure_statistics)
    add_test(NAME test_pressure_statistics COMMAND test_pressure_statistics)
endif()
```

- [ ] **Step 3: Run test and verify failure**

Run:

```powershell
cmake --build build --target test_pressure_statistics --config Release
```

Expected: compile fails because `PressureStatistics.h` does not exist.

- [ ] **Step 4: Implement `PressureStatistics.h`**

Create `src/pressure/PressureStatistics.h`:

```cpp
#pragma once

#include <deque>

class PressureStatistics final {
public:
    explicit PressureStatistics(int windowSamples);

    void addSample(double value);
    void clear();

    bool windowReady() const;
    int sampleCount() const;
    int windowSamples() const;

    double mean() const;
    double variance() const;
    double standardDeviation() const;
    double peak() const;

private:
    double computeMean() const;

    int windowSamples_;
    std::deque<double> samples_;
};
```

- [ ] **Step 5: Implement `PressureStatistics.cpp`**

Create `src/pressure/PressureStatistics.cpp`:

```cpp
#include "pressure/PressureStatistics.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

PressureStatistics::PressureStatistics(int windowSamples)
    : windowSamples_(windowSamples) {
    if (windowSamples_ <= 0) {
        throw std::invalid_argument("Pressure statistics window must be greater than 0.");
    }
}

void PressureStatistics::addSample(double value) {
    samples_.push_back(value);
    while (static_cast<int>(samples_.size()) > windowSamples_) {
        samples_.pop_front();
    }
}

void PressureStatistics::clear() {
    samples_.clear();
}

bool PressureStatistics::windowReady() const {
    return static_cast<int>(samples_.size()) >= windowSamples_;
}

int PressureStatistics::sampleCount() const {
    return static_cast<int>(samples_.size());
}

int PressureStatistics::windowSamples() const {
    return windowSamples_;
}

double PressureStatistics::mean() const {
    return windowReady() ? computeMean() : 0.0;
}

double PressureStatistics::variance() const {
    if (!windowReady()) {
        return 0.0;
    }
    const double currentMean = computeMean();
    double sum = 0.0;
    for (double sample : samples_) {
        const double diff = sample - currentMean;
        sum += diff * diff;
    }
    return sum / static_cast<double>(samples_.size());
}

double PressureStatistics::standardDeviation() const {
    return std::sqrt(variance());
}

double PressureStatistics::peak() const {
    if (!windowReady()) {
        return 0.0;
    }
    return *std::max_element(samples_.begin(), samples_.end());
}

double PressureStatistics::computeMean() const {
    const double sum = std::accumulate(samples_.begin(), samples_.end(), 0.0);
    return sum / static_cast<double>(samples_.size());
}
```

- [ ] **Step 6: Run statistics test**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64;C:\vcpkg\installed\x64-windows"
cmake --build build --target test_pressure_statistics --config Release
.\build\Release\test_pressure_statistics.exe
```

Expected:

```text
Totals: ... passed, 0 failed
```

- [ ] **Step 7: Commit**

Run:

```powershell
git add CMakeLists.txt src/pressure/PressureStatistics.h src/pressure/PressureStatistics.cpp tests/test_pressure_statistics.cpp
git commit -m "feat: add pressure statistics"
```

---

### Task 4: Add DDS IDL And Sample Mapping

**Files:**
- Create: `src/pressure/PressureSample.idl`
- Create: `src/pressure/PressureDdsTypes.h`
- Create: `tests/test_pressure_dds_types.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create the IDL**

Create `src/pressure/PressureSample.idl`:

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

- [ ] **Step 2: Generate Fast DDS type files**

Run the discovered generator. Example:

```powershell
New-Item -ItemType Directory -Force src\pressure\generated
$env:JAVA_HOME='C:\Program Files\Microsoft\jdk-17.0.19.10-hotspot'
$env:PATH="$env:JAVA_HOME\bin;$env:PATH"
C:\Tools\Fast-DDS-Gen-v4.0.6\scripts\fastddsgen.bat -d src\pressure\generated src\pressure\PressureSample.idl
```

Expected generated files include `PressureSample.hpp`, `PressureSampleCdrAux.hpp`, `PressureSampleCdrAux.ipp`, `PressureSamplePubSubTypes.hpp`, `PressureSamplePubSubTypes.cxx`, `PressureSampleTypeObjectSupport.hpp`, and `PressureSampleTypeObjectSupport.cxx`. Keep only generated source/header files needed by CMake.

- [ ] **Step 3: Write failing mapping test**

Create `tests/test_pressure_dds_types.cpp`:

```cpp
#include <QtTest/QtTest>

#include "pressure/PressureDdsTypes.h"

class TestPressureDdsTypes final : public QObject {
    Q_OBJECT

private slots:
    void mapsInternalSampleToDdsSample();
};

void TestPressureDdsTypes::mapsInternalSampleToDdsSample() {
    PressureSampleData sample;
    sample.timestampMs = 1234;
    sample.sequence = 7;
    sample.rawValue = 1.5;
    sample.meanValue = 2.5;
    sample.varianceValue = 3.5;
    sample.standardDeviationValue = 4.5;
    sample.peakValue = 5.5;
    sample.windowReady = true;
    sample.sensorRunning = true;

    const AfpPressureSample ddsSample = toDdsSample(sample);

    QCOMPARE(ddsSample.timestamp_ms(), static_cast<std::uint64_t>(1234));
    QCOMPARE(ddsSample.sequence(), static_cast<std::uint64_t>(7));
    QCOMPARE(ddsSample.raw_value(), 1.5);
    QCOMPARE(ddsSample.mean_value(), 2.5);
    QCOMPARE(ddsSample.variance_value(), 3.5);
    QCOMPARE(ddsSample.standard_deviation_value(), 4.5);
    QCOMPARE(ddsSample.peak_value(), 5.5);
    QVERIFY(ddsSample.window_ready());
    QVERIFY(ddsSample.sensor_running());
}

QTEST_MAIN(TestPressureDdsTypes)
#include "test_pressure_dds_types.moc"
```

- [ ] **Step 4: Add mapping helper**

Create `src/pressure/PressureDdsTypes.h`:

```cpp
#pragma once

#include <cstdint>

#include "pressure/generated/PressureSample.hpp"

struct PressureSampleData {
    std::uint64_t timestampMs = 0;
    std::uint64_t sequence = 0;
    double rawValue = 0.0;
    double meanValue = 0.0;
    double varianceValue = 0.0;
    double standardDeviationValue = 0.0;
    double peakValue = 0.0;
    bool windowReady = false;
    bool sensorRunning = false;
};

inline AfpPressureSample toDdsSample(const PressureSampleData& sample) {
    AfpPressureSample ddsSample;
    ddsSample.timestamp_ms(sample.timestampMs);
    ddsSample.sequence(sample.sequence);
    ddsSample.raw_value(sample.rawValue);
    ddsSample.mean_value(sample.meanValue);
    ddsSample.variance_value(sample.varianceValue);
    ddsSample.standard_deviation_value(sample.standardDeviationValue);
    ddsSample.peak_value(sample.peakValue);
    ddsSample.window_ready(sample.windowReady);
    ddsSample.sensor_running(sample.sensorRunning);
    return ddsSample;
}
```

- [ ] **Step 5: Register generated files and mapping test in CMake**

Add variables near `APP_SOURCES`:

```cmake
set(PRESSURE_DDS_GENERATED_SOURCES
    src/pressure/generated/PressureSamplePubSubTypes.cxx
    src/pressure/generated/PressureSampleTypeObjectSupport.cxx
)
```

Add generated headers/sources and `src/pressure/PressureDdsTypes.h` to `APP_SOURCES`.

Add a test target:

```cmake
add_executable(test_pressure_dds_types
    tests/test_pressure_dds_types.cpp
    ${PRESSURE_DDS_GENERATED_SOURCES}
)
target_include_directories(test_pressure_dds_types PRIVATE src src/pressure/generated)
target_link_libraries(test_pressure_dds_types PRIVATE Qt6::Core Qt6::Test fastcdr fastdds)
copy_qt_test_runtime(test_pressure_dds_types)
add_test(NAME test_pressure_dds_types COMMAND test_pressure_dds_types)
```

Use the actual imported target names exposed by vcpkg. If vcpkg exposes `fastcdr` and `fastdds` with namespace-qualified targets, update both app and test links consistently.

- [ ] **Step 6: Run mapping test**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64;C:\vcpkg\installed\x64-windows"
cmake --build build --target test_pressure_dds_types --config Release
.\build\Release\test_pressure_dds_types.exe
```

Expected:

```text
Totals: ... passed, 0 failed
```

- [ ] **Step 7: Commit**

Run:

```powershell
git add CMakeLists.txt src/pressure/PressureSample.idl src/pressure/PressureDdsTypes.h src/pressure/generated tests/test_pressure_dds_types.cpp
git commit -m "feat: add pressure dds sample type"
```

---

### Task 5: Add Pressure Sensor DLL Wrapper

**Files:**
- Create: `src/pressure/PressureSensorClient.h`
- Create: `src/pressure/PressureSensorClient.cpp`
- Create: `tests/test_pressure_sensor_client.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write missing-DLL test**

Create `tests/test_pressure_sensor_client.cpp`:

```cpp
#include <QtTest/QtTest>
#include <QTemporaryDir>

#include "pressure/PressureSensorClient.h"

class TestPressureSensorClient final : public QObject {
    Q_OBJECT

private slots:
    void reportsMissingDll();
};

void TestPressureSensorClient::reportsMissingDll() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    PressureSensorClient client(tempDir.filePath("missing-pressure.dll"));

    QVERIFY(!client.load());
    QVERIFY(!client.lastError().isEmpty());
    QVERIFY(client.lastError().contains("pressure"));
}

QTEST_MAIN(TestPressureSensorClient)
#include "test_pressure_sensor_client.moc"
```

- [ ] **Step 2: Implement `PressureSensorClient.h`**

Create `src/pressure/PressureSensorClient.h`:

```cpp
#pragma once

#include <QLibrary>
#include <QString>

class PressureSensorClient final {
public:
    explicit PressureSensorClient(QString dllPath);
    ~PressureSensorClient();

    bool load();
    bool start();
    bool read(double& value);
    void stop();

    bool loaded() const;
    bool running() const;
    QString lastError() const;

private:
    using WorkStartFn = int (*)();
    using WorkStopFn = int (*)();
    using CatchDataFn = double (*)();

    bool resolveFunctions();
    void setError(const QString& message);

    QString dllPath_;
    QLibrary library_;
    WorkStartFn workStart_ = nullptr;
    WorkStopFn workStop_ = nullptr;
    CatchDataFn catchData_ = nullptr;
    bool running_ = false;
    QString lastError_;
};
```

- [ ] **Step 3: Implement `PressureSensorClient.cpp`**

Create `src/pressure/PressureSensorClient.cpp`:

```cpp
#include "pressure/PressureSensorClient.h"

#include <utility>

PressureSensorClient::PressureSensorClient(QString dllPath)
    : dllPath_(std::move(dllPath)), library_(dllPath_) {
}

PressureSensorClient::~PressureSensorClient() {
    stop();
}

bool PressureSensorClient::load() {
    if (library_.isLoaded()) {
        return true;
    }
    if (!library_.load()) {
        setError(QString("Could not load pressure sensor DLL: %1").arg(library_.errorString()));
        return false;
    }
    return resolveFunctions();
}

bool PressureSensorClient::start() {
    if (!load()) {
        return false;
    }
    if (running_) {
        return true;
    }
    if (workStart_ && workStart_() != 0) {
        setError("pressure.dll workstart() reported failure.");
        return false;
    }
    running_ = true;
    return true;
}

bool PressureSensorClient::read(double& value) {
    if (!running_ || !catchData_) {
        setError("Pressure sensor is not running.");
        return false;
    }
    value = catchData_();
    return true;
}

void PressureSensorClient::stop() {
    if (running_ && workStop_) {
        workStop_();
    }
    running_ = false;
}

bool PressureSensorClient::loaded() const {
    return library_.isLoaded();
}

bool PressureSensorClient::running() const {
    return running_;
}

QString PressureSensorClient::lastError() const {
    return lastError_;
}

bool PressureSensorClient::resolveFunctions() {
    workStart_ = reinterpret_cast<WorkStartFn>(library_.resolve("workstart"));
    workStop_ = reinterpret_cast<WorkStopFn>(library_.resolve("workstop"));
    catchData_ = reinterpret_cast<CatchDataFn>(library_.resolve("catchdata"));

    if (!workStart_ || !workStop_ || !catchData_) {
        setError("pressure.dll is missing workstart, workstop, or catchdata export.");
        library_.unload();
        return false;
    }
    return true;
}

void PressureSensorClient::setError(const QString& message) {
    lastError_ = message;
}
```

If hardware testing shows `workstart()` or `workstop()` return `void`, adjust `WorkStartFn` and `WorkStopFn` to `void (*)()` and update comments to explain the vendor DLL export behavior.

- [ ] **Step 4: Register test target**

Add to `CMakeLists.txt`:

```cmake
add_executable(test_pressure_sensor_client
    tests/test_pressure_sensor_client.cpp
    src/pressure/PressureSensorClient.cpp
)
target_include_directories(test_pressure_sensor_client PRIVATE src)
target_link_libraries(test_pressure_sensor_client PRIVATE Qt6::Core Qt6::Test)
copy_qt_test_runtime(test_pressure_sensor_client)
add_test(NAME test_pressure_sensor_client COMMAND test_pressure_sensor_client)
```

- [ ] **Step 5: Run test**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64;C:\vcpkg\installed\x64-windows"
cmake --build build --target test_pressure_sensor_client --config Release
.\build\Release\test_pressure_sensor_client.exe
```

Expected:

```text
Totals: ... passed, 0 failed
```

- [ ] **Step 6: Commit**

Run:

```powershell
git add CMakeLists.txt src/pressure/PressureSensorClient.h src/pressure/PressureSensorClient.cpp tests/test_pressure_sensor_client.cpp
git commit -m "feat: add pressure sensor dll wrapper"
```

---

### Task 6: Add DDS Publisher

**Files:**
- Create: `src/pressure/PressureDdsPublisher.h`
- Create: `src/pressure/PressureDdsPublisher.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Implement `PressureDdsPublisher.h`**

Create `src/pressure/PressureDdsPublisher.h`:

```cpp
#pragma once

#include <QString>

#include "app/AppConfig.h"
#include "pressure/PressureDdsTypes.h"

namespace eprosima::fastdds::dds {
class DomainParticipant;
class Publisher;
class Topic;
class DataWriter;
}

class PressureDdsPublisher final {
public:
    explicit PressureDdsPublisher(PressureConfig config);
    ~PressureDdsPublisher();

    bool initialize();
    bool publish(const PressureSampleData& sample);
    void shutdown();

    bool initialized() const;
    QString topicName() const;
    QString lastError() const;

private:
    void setError(const QString& message);

    PressureConfig config_;
    eprosima::fastdds::dds::DomainParticipant* participant_ = nullptr;
    eprosima::fastdds::dds::Publisher* publisher_ = nullptr;
    eprosima::fastdds::dds::Topic* topic_ = nullptr;
    eprosima::fastdds::dds::DataWriter* writer_ = nullptr;
    QString lastError_;
};
```

- [ ] **Step 2: Implement `PressureDdsPublisher.cpp`**

Create `src/pressure/PressureDdsPublisher.cpp`:

```cpp
#include "pressure/PressureDdsPublisher.h"

#include "pressure/generated/PressureSamplePubSubTypes.h"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

using eprosima::fastdds::dds::DataWriterQos;
using eprosima::fastdds::dds::DomainParticipantFactory;
using eprosima::fastdds::dds::DomainParticipantQos;
using eprosima::fastdds::dds::PublisherQos;
using eprosima::fastdds::dds::TopicQos;
using eprosima::fastdds::dds::TypeSupport;

namespace {
constexpr const char* kTypeName = "AfpPressureSample";
}

PressureDdsPublisher::PressureDdsPublisher(PressureConfig config)
    : config_(std::move(config)) {
}

PressureDdsPublisher::~PressureDdsPublisher() {
    shutdown();
}

bool PressureDdsPublisher::initialize() {
    if (initialized()) {
        return true;
    }

    DomainParticipantQos participantQos;
    participant_ = DomainParticipantFactory::get_instance()->create_participant(
        static_cast<uint32_t>(config_.ddsDomainId), participantQos);
    if (!participant_) {
        setError("Could not create Fast DDS domain participant.");
        return false;
    }

    TypeSupport type(new AfpPressureSamplePubSubType());
    type.register_type(participant_, kTypeName);

    publisher_ = participant_->create_publisher(PublisherQos());
    if (!publisher_) {
        setError("Could not create Fast DDS publisher.");
        shutdown();
        return false;
    }

    topic_ = participant_->create_topic(config_.ddsTopic.toStdString(), kTypeName, TopicQos());
    if (!topic_) {
        setError(QString("Could not create Fast DDS topic %1.").arg(config_.ddsTopic));
        shutdown();
        return false;
    }

    DataWriterQos writerQos;
    writerQos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
    writer_ = publisher_->create_datawriter(topic_, writerQos);
    if (!writer_) {
        setError("Could not create Fast DDS data writer.");
        shutdown();
        return false;
    }

    return true;
}

bool PressureDdsPublisher::publish(const PressureSampleData& sample) {
    if (!initialize()) {
        return false;
    }
    AfpPressureSample ddsSample = toDdsSample(sample);
    if (!writer_->write(&ddsSample)) {
        setError("Fast DDS write() failed for pressure sample.");
        return false;
    }
    return true;
}

void PressureDdsPublisher::shutdown() {
    if (participant_) {
        if (publisher_ && writer_) {
            publisher_->delete_datawriter(writer_);
        }
        if (participant_ && publisher_) {
            participant_->delete_publisher(publisher_);
        }
        if (participant_ && topic_) {
            participant_->delete_topic(topic_);
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }
    writer_ = nullptr;
    topic_ = nullptr;
    publisher_ = nullptr;
    participant_ = nullptr;
}

bool PressureDdsPublisher::initialized() const {
    return participant_ && publisher_ && topic_ && writer_;
}

QString PressureDdsPublisher::topicName() const {
    return config_.ddsTopic;
}

QString PressureDdsPublisher::lastError() const {
    return lastError_;
}

void PressureDdsPublisher::setError(const QString& message) {
    lastError_ = message;
}
```

Adjust include names only if the generated files use different names. Keep the public interface stable for later tasks.

- [ ] **Step 3: Add source files to app target**

Add to `APP_SOURCES`:

```cmake
src/pressure/PressureDdsPublisher.h
src/pressure/PressureDdsPublisher.cpp
```

Link `AFP_ABB_CPP` against the same Fast DDS/Fast CDR targets used by `test_pressure_dds_types`.

- [ ] **Step 4: Build**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64;C:\vcpkg\installed\x64-windows"
cmake --build build --config Release
```

Expected: build succeeds. If Fast DDS target names differ, inspect `C:\vcpkg\installed\x64-windows\share\fastdds` and adjust CMake.

- [ ] **Step 5: Commit**

Run:

```powershell
git add CMakeLists.txt src/pressure/PressureDdsPublisher.h src/pressure/PressureDdsPublisher.cpp
git commit -m "feat: add pressure dds publisher"
```

---

### Task 7: Add Acquisition Worker And Real Pressure Service

**Files:**
- Create: `src/pressure/PressureAcquisitionWorker.h`
- Create: `src/pressure/PressureAcquisitionWorker.cpp`
- Create: `src/services/pressure/PressureService.h`
- Create: `src/services/pressure/PressureService.cpp`
- Modify: `src/services/PlaceholderServices.h`
- Modify: `src/services/PlaceholderServices.cpp`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Implement `PressureAcquisitionWorker.h`**

Create `src/pressure/PressureAcquisitionWorker.h`:

```cpp
#pragma once

#include <QObject>
#include <memory>

#include "app/AppConfig.h"
#include "pressure/PressureDdsPublisher.h"
#include "pressure/PressureSensorClient.h"
#include "pressure/PressureStatistics.h"

class PressureAcquisitionWorker final : public QObject {
    Q_OBJECT

public:
    PressureAcquisitionWorker(QString dllPath, PressureConfig config, QObject* parent = nullptr);

public slots:
    void start();
    void stop();

signals:
    void started();
    void stopped();
    void errorOccurred(const QString& message);
    void samplePublished(const PressureSampleData& sample);

private slots:
    void acquireOneSample();

private:
    QString dllPath_;
    PressureConfig config_;
    PressureSensorClient sensor_;
    PressureStatistics statistics_;
    PressureDdsPublisher publisher_;
    QTimer* timer_ = nullptr;
    std::uint64_t sequence_ = 0;
    bool running_ = false;
};
```

- [ ] **Step 2: Implement `PressureAcquisitionWorker.cpp`**

Create `src/pressure/PressureAcquisitionWorker.cpp`:

```cpp
#include "pressure/PressureAcquisitionWorker.h"

#include <QDateTime>
#include <QTimer>

PressureAcquisitionWorker::PressureAcquisitionWorker(QString dllPath, PressureConfig config, QObject* parent)
    : QObject(parent),
      dllPath_(std::move(dllPath)),
      config_(std::move(config)),
      sensor_(dllPath_),
      statistics_(config_.processingWindowSamples),
      publisher_(config_) {
}

void PressureAcquisitionWorker::start() {
    if (running_) {
        return;
    }
    if (!publisher_.initialize()) {
        emit errorOccurred(publisher_.lastError());
        emit stopped();
        return;
    }
    if (!sensor_.start()) {
        emit errorOccurred(sensor_.lastError());
        emit stopped();
        return;
    }
    timer_ = new QTimer(this);
    timer_->setTimerType(Qt::PreciseTimer);
    timer_->setInterval(config_.sampleIntervalMs);
    connect(timer_, &QTimer::timeout, this, &PressureAcquisitionWorker::acquireOneSample);

    running_ = true;
    sequence_ = 0;
    statistics_.clear();
    timer_->start();
    emit started();
}

void PressureAcquisitionWorker::stop() {
    if (!running_) {
        emit stopped();
        return;
    }
    if (timer_) {
        timer_->stop();
        timer_->deleteLater();
        timer_ = nullptr;
    }
    sensor_.stop();
    running_ = false;
    emit stopped();
}

void PressureAcquisitionWorker::acquireOneSample() {
    double raw = 0.0;
    if (!sensor_.read(raw)) {
        if (timer_) {
            timer_->stop();
        }
        sensor_.stop();
        running_ = false;
        emit errorOccurred(sensor_.lastError());
        emit stopped();
        return;
    }

    statistics_.addSample(raw);

    PressureSampleData sample;
    sample.timestampMs = static_cast<std::uint64_t>(QDateTime::currentMSecsSinceEpoch());
    sample.sequence = sequence_++;
    sample.rawValue = raw;
    sample.windowReady = statistics_.windowReady();
    sample.sensorRunning = running_;
    if (sample.windowReady) {
        sample.meanValue = statistics_.mean();
        sample.varianceValue = statistics_.variance();
        sample.standardDeviationValue = statistics_.standardDeviation();
        sample.peakValue = statistics_.peak();
    }

    if (!publisher_.publish(sample)) {
        if (timer_) {
            timer_->stop();
        }
        sensor_.stop();
        running_ = false;
        emit errorOccurred(publisher_.lastError());
        emit stopped();
        return;
    }

    emit samplePublished(sample);
}
```

- [ ] **Step 3: Implement `PressureService.h`**

Create `src/services/pressure/PressureService.h`:

```cpp
#pragma once

#include <QObject>
#include <QThread>
#include <memory>

#include "app/AppConfig.h"
#include "app/AppPaths.h"

namespace Ui { class ContinuosfiberInterface; }
class PressureAcquisitionWorker;

class PressureService final : public QObject {
    Q_OBJECT

public:
    PressureService(const AppConfig& config,
                    const AppPaths& paths,
                    Ui::ContinuosfiberInterface* ui,
                    QObject* parent = nullptr);
    ~PressureService() override;

signals:
    void messageRequested(const QString& title, const QString& message);

public slots:
    void initializeMachine();
    void startCollection();
    void stopCollection();
    void showData();

private slots:
    void handleStarted();
    void handleStopped();
    void handleError(const QString& message);

private:
    QString pressureDllPath() const;
    bool workerRunning() const;

    PressureConfig config_;
    AppPaths paths_;
    Ui::ContinuosfiberInterface* ui_ = nullptr;
    std::unique_ptr<QThread> workerThread_;
    PressureAcquisitionWorker* worker_ = nullptr;
    bool running_ = false;
};
```

- [ ] **Step 4: Implement `PressureService.cpp`**

Create `src/services/pressure/PressureService.cpp`:

```cpp
#include "services/pressure/PressureService.h"

#include "pressure/PressureAcquisitionWorker.h"
#include "ui_Interface4600_window.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

PressureService::PressureService(const AppConfig& config,
                                 const AppPaths& paths,
                                 Ui::ContinuosfiberInterface* ui,
                                 QObject* parent)
    : QObject(parent), config_(config.pressure), paths_(paths), ui_(ui) {
}

PressureService::~PressureService() {
    stopCollection();
}

void PressureService::initializeMachine() {
    const QFileInfo dll(pressureDllPath());
    if (!dll.exists()) {
        emit messageRequested("Pressure", QString("Pressure sensor DLL was not found: %1").arg(dll.absoluteFilePath()));
        return;
    }
    emit messageRequested("Pressure",
                          QString("Pressure sensor runtime found.\nDDS topic: %1\nSample interval: %2 ms\nWindow: %3 samples")
                              .arg(config_.ddsTopic)
                              .arg(config_.sampleIntervalMs)
                              .arg(config_.processingWindowSamples));
}

void PressureService::startCollection() {
    if (running_) {
        emit messageRequested("Pressure", "Pressure acquisition is already running.");
        return;
    }

    workerThread_ = std::make_unique<QThread>();
    worker_ = new PressureAcquisitionWorker(pressureDllPath(), config_);
    worker_->moveToThread(workerThread_.get());

    connect(workerThread_.get(), &QThread::started, worker_, &PressureAcquisitionWorker::start);
    connect(worker_, &PressureAcquisitionWorker::started, this, &PressureService::handleStarted);
    connect(worker_, &PressureAcquisitionWorker::stopped, this, &PressureService::handleStopped);
    connect(worker_, &PressureAcquisitionWorker::errorOccurred, this, &PressureService::handleError);
    connect(worker_, &PressureAcquisitionWorker::stopped, workerThread_.get(), &QThread::quit);
    connect(workerThread_.get(), &QThread::finished, worker_, &QObject::deleteLater);

    workerThread_->start();
}

void PressureService::stopCollection() {
    if (!workerThread_) {
        running_ = false;
        return;
    }
    if (worker_) {
        QMetaObject::invokeMethod(worker_, "stop", Qt::QueuedConnection);
    }
    workerThread_->wait(2000);
    workerThread_.reset();
    worker_ = nullptr;
    running_ = false;
}

void PressureService::showData() {
    emit messageRequested("Pressure DDS",
                          QString("Publish topic: %1\nType: AfpPressureSample\nDomain ID: %2\nUse PlotJuggler eProsima Edition or a Fast DDS subscriber to view pressure data.")
                              .arg(config_.ddsTopic)
                              .arg(config_.ddsDomainId));
}

void PressureService::handleStarted() {
    running_ = true;
    if (ui_ && ui_->FC_start) {
        ui_->FC_start->setEnabled(false);
    }
    if (ui_ && ui_->FC_stop) {
        ui_->FC_stop->setEnabled(true);
    }
}

void PressureService::handleStopped() {
    running_ = false;
    if (ui_ && ui_->FC_start) {
        ui_->FC_start->setEnabled(true);
    }
    if (ui_ && ui_->FC_stop) {
        ui_->FC_stop->setEnabled(true);
    }
}

void PressureService::handleError(const QString& message) {
    emit messageRequested("Pressure", message);
    handleStopped();
}

QString PressureService::pressureDllPath() const {
    return QDir(QCoreApplication::applicationDirPath()).filePath("lib/sensors/pressure_sensor/pressure.dll");
}

bool PressureService::workerRunning() const {
    return running_;
}
```

Keep comments in the final source explaining why `FC_RD`, `FC_VV`, `FC_VAR`, `FC_SD`, and `FC_PEAK` are not updated in Slice C.

- [ ] **Step 5: Remove placeholder pressure service**

In `src/services/PlaceholderServices.h`, delete the placeholder `PressureService` class block.

In `src/services/PlaceholderServices.cpp`, delete:

```cpp
PressureService::PressureService(QObject* parent) : PlaceholderService("Pressure", parent) {}
void PressureService::startCollection() { reportNotConverted("Pressure collection start"); }
void PressureService::stopCollection() { reportNotConverted("Pressure collection stop"); }
void PressureService::showData() { reportNotConverted("Pressure data display"); }
```

- [ ] **Step 6: Update `MainWindow` includes and member**

In `src/ui/MainWindow.h`, replace:

```cpp
#include "services/PlaceholderServices.h"
#include "services/plc/PlcService.h"
```

with:

```cpp
#include "services/PlaceholderServices.h"
#include "services/plc/PlcService.h"
#include "services/pressure/PressureService.h"
```

Leave the member name as:

```cpp
PressureService pressureService_;
```

Update constructor initialization in `src/ui/MainWindow.cpp` to construct `pressureService_` with config, paths, and UI:

```cpp
pressureService_(config_, paths_, ui_, this)
```

- [ ] **Step 7: Wire `FC_machine`**

In `MainWindow::connectSignals`, add:

```cpp
connect(ui_->FC_machine, &QPushButton::clicked, &pressureService_, &PressureService::initializeMachine);
```

Keep existing:

```cpp
connect(ui_->FC_data, &QPushButton::clicked, &pressureService_, &PressureService::showData);
connect(ui_->FC_start, &QPushButton::clicked, &pressureService_, &PressureService::startCollection);
connect(ui_->FC_stop, &QPushButton::clicked, &pressureService_, &PressureService::stopCollection);
```

- [ ] **Step 8: Add source files to CMake**

Add to `APP_SOURCES`:

```cmake
src/pressure/PressureAcquisitionWorker.h
src/pressure/PressureAcquisitionWorker.cpp
src/pressure/PressureSensorClient.h
src/pressure/PressureSensorClient.cpp
src/pressure/PressureStatistics.h
src/pressure/PressureStatistics.cpp
src/pressure/PressureDdsPublisher.h
src/pressure/PressureDdsPublisher.cpp
src/services/pressure/PressureService.h
src/services/pressure/PressureService.cpp
```

- [ ] **Step 9: Build**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64;C:\vcpkg\installed\x64-windows"
cmake --build build --config Release
```

Expected: build succeeds.

- [ ] **Step 10: Commit**

Run:

```powershell
git add CMakeLists.txt src/pressure src/services/pressure src/services/PlaceholderServices.h src/services/PlaceholderServices.cpp src/ui/MainWindow.h src/ui/MainWindow.cpp
git commit -m "feat: add pressure acquisition service"
```

---

### Task 8: Portable Runtime Files And Documentation

**Files:**
- Modify: `cmake/DeployQt.cmake`
- Modify: `README.md`

- [ ] **Step 1: Copy pressure sensor runtime files into the C++ tree**

Create this folder in `AFP_ABB_CPP`:

```text
lib/sensors/pressure_sensor/
```

Copy from `AFP_ABB_Python/lib/sensors/pressure_sensor/` into `AFP_ABB_CPP/lib/sensors/pressure_sensor/`:

```text
pressure.dll
BD802NHMC_DAQ.dll
BD80xmcsdconfig.ini
vk70xmcdllconfig.ini
```

Do not copy `.lib`, `.exp`, or `.pdb` files unless hardware testing proves they are required at runtime.

- [ ] **Step 2: Add runtime copy to app post-build**

In `CMakeLists.txt`, extend the `add_custom_command(TARGET AFP_ABB_CPP POST_BUILD ...)` with:

```cmake
COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
        $<TARGET_FILE_DIR:AFP_ABB_CPP>/lib
```

- [ ] **Step 3: Add Fast DDS and pressure runtime copy to deploy script**

Add to `cmake/DeployQt.cmake` after Snap7 copy:

```cmake
find_file(FASTDDS_RUNTIME_DLL
    NAMES fastdds.dll
    HINTS
        "C:/vcpkg/installed/x64-windows/bin"
        "$ENV{VCPKG_ROOT}/installed/x64-windows/bin"
)

find_file(FASTCDR_RUNTIME_DLL
    NAMES fastcdr.dll
    HINTS
        "C:/vcpkg/installed/x64-windows/bin"
        "$ENV{VCPKG_ROOT}/installed/x64-windows/bin"
)

if(NOT FASTDDS_RUNTIME_DLL)
    message(FATAL_ERROR "fastdds.dll was not found. Install fastdds with vcpkg or set VCPKG_ROOT.")
endif()

if(NOT FASTCDR_RUNTIME_DLL)
    message(FATAL_ERROR "fastcdr.dll was not found. Install fastdds with vcpkg or set VCPKG_ROOT.")
endif()

file(COPY "${FASTDDS_RUNTIME_DLL}" DESTINATION "${QT_DEPLOY_OUTPUT_DIR}")
file(COPY "${FASTCDR_RUNTIME_DLL}" DESTINATION "${QT_DEPLOY_OUTPUT_DIR}")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/../lib" DESTINATION "${QT_DEPLOY_OUTPUT_DIR}")
message(STATUS "Copied Fast DDS runtime: ${FASTDDS_RUNTIME_DLL}")
message(STATUS "Copied Fast CDR runtime: ${FASTCDR_RUNTIME_DLL}")
```

If vcpkg installs additional required runtime DLLs, add explicit `find_file` and `file(COPY ...)` blocks for each one.

- [ ] **Step 4: Update README**

Update the README sections:

```markdown
Slice C adds pressure sensor acquisition and publishes raw/processed pressure data through eProsima Fast DDS. The main window keeps the original pressure plot widgets, but Slice C does not draw pressure curves inside the application.
```

Add required dependency:

```markdown
- vcpkg Fast DDS package for pressure data publishing.
```

Add install command:

```powershell
C:\vcpkg\vcpkg.exe install fastdds:x64-windows
```

Add pressure config docs:

```markdown
"pressure": {
  "sample_interval_ms": 100,
  "processing_window_samples": 40,
  "dds_domain_id": 0,
  "dds_topic": "afp.pressure.sample"
}
```

Explain:

- `sample_interval_ms=100` matches Python `time.sleep(0.1)`.
- `processing_window_samples=40` matches Python `pressure_monitor.py`.
- Acquisition rate is derived from `sample_interval_ms`; it is not configured separately.
- Use PlotJuggler eProsima Edition or a custom Fast DDS subscriber to subscribe to `afp.pressure.sample`.

- [ ] **Step 5: Build deploy target**

Run:

```powershell
cmake --build build --target deploy_portable --config Release
```

Expected files:

```text
build\Release\AFP_ABB_CPP.exe
build\Release\snap7.dll
build\Release\fastdds.dll
build\Release\fastcdr.dll
build\Release\lib\sensors\pressure_sensor\pressure.dll
build\Release\lib\sensors\pressure_sensor\BD802NHMC_DAQ.dll
build\Release\lib\sensors\pressure_sensor\BD80xmcsdconfig.ini
build\Release\lib\sensors\pressure_sensor\vk70xmcdllconfig.ini
build\Release\config\env_config.json
```

- [ ] **Step 6: Commit**

Run:

```powershell
git add CMakeLists.txt cmake/DeployQt.cmake README.md lib/sensors/pressure_sensor
git commit -m "docs: document pressure dds deployment"
```

---

### Task 9: Full Verification And Merge

**Files:**
- Verify all changed files.
- No source edits unless verification reveals a bug.

- [ ] **Step 1: Run full configure/build/test**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64;C:\vcpkg\installed\x64-windows"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Expected:

```text
100% tests passed
```

- [ ] **Step 2: Run deploy**

Run:

```powershell
cmake --build build --target deploy_portable --config Release
```

Expected: deployment completes. Existing `windeployqt` MSVC redistributable warnings may remain as already documented.

- [ ] **Step 3: Confirm runtime files**

Run:

```powershell
Test-Path build\Release\AFP_ABB_CPP.exe
Test-Path build\Release\snap7.dll
Test-Path build\Release\fastdds.dll
Test-Path build\Release\fastcdr.dll
Test-Path build\Release\lib\sensors\pressure_sensor\pressure.dll
Test-Path build\Release\config\env_config.json
```

Expected:

```text
True
True
True
True
True
True
```

- [ ] **Step 4: Confirm Python folder remains untouched**

Run from `AFP_ABB_Python`:

```powershell
git status --short --branch
```

Expected: no modified files.

- [ ] **Step 5: Manual UI smoke test**

Run:

```powershell
.\build\Release\AFP_ABB_CPP.exe
```

Manual checks:

- Main window opens.
- UI layout matches original design.
- `FC_machine` reports pressure runtime/config status.
- `FC_data` reports DDS topic/type/domain.
- `FC_start` starts acquisition if hardware is attached.
- `FC_stop` stops acquisition.
- Existing PLC controls still work or fail only due to absent hardware/network, not due to build/link errors.

- [ ] **Step 6: Manual DDS subscriber smoke test**

Use PlotJuggler eProsima Edition or a small Fast DDS subscriber:

- Confirm topic `afp.pressure.sample` appears.
- Confirm raw pressure field updates.
- Confirm processed fields remain zero until `processing_window_samples` readings are available.
- Confirm processed fields update after the window is ready.

- [ ] **Step 7: Final status**

Run:

```powershell
git status --short --branch
git log --oneline --max-count=8
```

Expected:

- Branch is `slice-c-pressure-dds`.
- Only `build/` is untracked.
- Slice C commits are visible.

- [ ] **Step 8: Merge if successful**

If all automated and required manual checks pass:

```powershell
git switch master
git merge --ff-only slice-c-pressure-dds
git branch -d slice-c-pressure-dds
```

If manual hardware/DDS checks cannot be completed in the current environment, do not merge automatically. Report the exact unverified items and wait for user approval.
