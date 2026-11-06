# AFP ABB C++ Slice G PLC Robot Movement Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Convert the five remaining ABB robot movement buttons using the Python PLC-forward robot path.

**Architecture:** Extend the config and PLC client layers first, then add focused robot math and PLC-forward motion components that can be tested with fake process-area access. Finally, wire only the existing movement buttons in `MainWindow`, leaving print workflow and UI design unchanged.

**Tech Stack:** C++17, Qt 6 Core/Widgets/Test, Snap7 C API through existing `PlcClient`, CMake, CTest.

---

## Scope Guardrails

- Do not edit anything under `AFP_ABB_Python`.
- Do not change `src/ui/Interface4600_window.ui`.
- Do not change Slice F print workflow behavior.
- Do not add socket-based robot movement commands.
- Do not convert RAPID files, camera workflows, thermal camera workflows, or serial workflows.
- Treat hardware/process parameters as config candidates before making them source constants.
- Keep comments generous in new C++ code.

## File Structure

- Modify `src/app/AppConfig.h`: add robot default speed config fields.
- Modify `src/app/AppConfig.cpp`: parse `robot.default_speed` with fallback values.
- Modify `config/env_config.json`: add Python-compatible robot speed defaults.
- Modify `tests/test_app_config.cpp`: test default and explicit robot speed loading.
- Modify `src/plc/S7Data.h/.cpp`: add little-endian INT32 helpers for PLC process areas.
- Modify `src/plc/PlcClient.h/.cpp`: add process output/input read/write methods.
- Modify `tests/test_s7_data.cpp`: test little-endian INT32 helpers.
- Modify `tests/test_plc_client_state.cpp`: compile-check process-area interface methods without hardware access.
- Create `src/robot/RobotQuaternion.h/.cpp`: convert the needed Python quaternion math.
- Create `tests/test_robot_quaternion.cpp`: test known Python-derived quaternion values.
- Create `src/robot/PlcRobotMotionClient.h/.cpp`: implement PLC-forward Cartesian, joint, and external-axis commands.
- Create `tests/test_plc_robot_motion_client.cpp`: test command offsets, sequence, ACK/DONE, and error paths with a fake PLC process-area interface.
- Create `src/services/robot/RobotMovementService.h/.cpp`: implement Qt-facing movement service for the five movement buttons.
- Create `tests/test_robot_movement_service.cpp`: test input validation and command argument mapping with fakes.
- Modify `src/ui/MainWindow.h/.cpp`: wire only the five movement buttons to `RobotMovementService`.
- Modify `CMakeLists.txt`: add app sources and test targets.
- Modify `README.md`: document PLC-forward robot movement and default-speed config.

## Task 1: Add Robot Movement Speed Config

**Files:**
- Modify: `src/app/AppConfig.h`
- Modify: `src/app/AppConfig.cpp`
- Modify: `config/env_config.json`
- Modify: `tests/test_app_config.cpp`

- [ ] **Step 1: Add config fields**

In `src/app/AppConfig.h`, add a nested struct before `RobotConfig`:

```cpp
struct RobotDefaultSpeedConfig {
    // Defaults preserve the Python main-window state used when speed fields are empty.
    double tcpLinear = 200.0;
    double tcpOrientation = 200.0;
    double externalLinear = 200.0;
    double externalOrientation = 100.0;
};
```

Add this field to `RobotConfig`:

```cpp
RobotDefaultSpeedConfig defaultSpeed;
```

- [ ] **Step 2: Add a positive double reader**

In `src/app/AppConfig.cpp`, add this helper inside the anonymous namespace:

```cpp
double readPositiveDouble(AppConfig& config,
                          const QJsonObject& object,
                          const QString& key,
                          double fallback,
                          const QString& warningName) {
    const QJsonValue value = object.value(key);
    if (!value.isDouble()) {
        return fallback;
    }
    const double parsed = value.toDouble();
    if (parsed <= 0.0) {
        config.addWarning(QString("%1 must be greater than 0; using %2").arg(warningName).arg(fallback));
        return fallback;
    }
    return parsed;
}
```

- [ ] **Step 3: Parse `robot.default_speed`**

In `AppConfig::load`, after transport parsing, add:

```cpp
const QJsonObject defaultSpeed = robot.value("default_speed").toObject();
config.robot.defaultSpeed.tcpLinear = readPositiveDouble(
    config, defaultSpeed, "tcp_linear", config.robot.defaultSpeed.tcpLinear, "robot.default_speed.tcp_linear");
config.robot.defaultSpeed.tcpOrientation = readPositiveDouble(
    config, defaultSpeed, "tcp_orientation", config.robot.defaultSpeed.tcpOrientation, "robot.default_speed.tcp_orientation");
config.robot.defaultSpeed.externalLinear = readPositiveDouble(
    config, defaultSpeed, "external_linear", config.robot.defaultSpeed.externalLinear, "robot.default_speed.external_linear");
config.robot.defaultSpeed.externalOrientation = readPositiveDouble(
    config, defaultSpeed, "external_orientation", config.robot.defaultSpeed.externalOrientation, "robot.default_speed.external_orientation");
```

- [ ] **Step 4: Update runtime config**

In `config/env_config.json`, add this object under `"robot"` after `"transport"`:

```json
        "default_speed": {
            "tcp_linear": 200.0,
            "tcp_orientation": 200.0,
            "external_linear": 200.0,
            "external_orientation": 100.0
        },
```

Keep valid JSON commas.

- [ ] **Step 5: Write config tests**

In `tests/test_app_config.cpp`, add private slots:

```cpp
void loadsRobotDefaultSpeedConfig();
void missingRobotDefaultSpeedUsesPythonDefaults();
void invalidRobotDefaultSpeedFallsBackToDefaults();
```

Add this test for explicit values:

```cpp
void TestAppConfig::loadsRobotDefaultSpeedConfig() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "robot": {
            "default_speed": {
                "tcp_linear": 111.5,
                "tcp_orientation": 222.5,
                "external_linear": 333.5,
                "external_orientation": 44.5
            }
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.robot.defaultSpeed.tcpLinear, 111.5);
    QCOMPARE(config.robot.defaultSpeed.tcpOrientation, 222.5);
    QCOMPARE(config.robot.defaultSpeed.externalLinear, 333.5);
    QCOMPARE(config.robot.defaultSpeed.externalOrientation, 44.5);
}
```

Add this test for missing keys:

```cpp
void TestAppConfig::missingRobotDefaultSpeedUsesPythonDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({"robot": {"ip": "10.0.0.4"}})";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.robot.defaultSpeed.tcpLinear, 200.0);
    QCOMPARE(config.robot.defaultSpeed.tcpOrientation, 200.0);
    QCOMPARE(config.robot.defaultSpeed.externalLinear, 200.0);
    QCOMPARE(config.robot.defaultSpeed.externalOrientation, 100.0);
}
```

Add this test for invalid values:

```cpp
void TestAppConfig::invalidRobotDefaultSpeedFallsBackToDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "robot": {
            "default_speed": {
                "tcp_linear": 0,
                "tcp_orientation": -1,
                "external_linear": 0,
                "external_orientation": -2
            }
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.robot.defaultSpeed.tcpLinear, 200.0);
    QCOMPARE(config.robot.defaultSpeed.tcpOrientation, 200.0);
    QCOMPARE(config.robot.defaultSpeed.externalLinear, 200.0);
    QCOMPARE(config.robot.defaultSpeed.externalOrientation, 100.0);
    const QString warnings = config.warnings().join('\n');
    QVERIFY(warnings.contains("robot.default_speed.tcp_linear"));
    QVERIFY(warnings.contains("robot.default_speed.tcp_orientation"));
    QVERIFY(warnings.contains("robot.default_speed.external_linear"));
    QVERIFY(warnings.contains("robot.default_speed.external_orientation"));
}
```

- [ ] **Step 6: Run config tests**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_app_config
ctest --test-dir build -C Release -R test_app_config --output-on-failure
```

Expected:

```text
100% tests passed
```

- [ ] **Step 7: Commit Task 1**

```powershell
git add src\app\AppConfig.h src\app\AppConfig.cpp config\env_config.json tests\test_app_config.cpp
git commit -m "feat: add robot movement speed config"
```

## Task 2: Add PLC Process Area Access

**Files:**
- Modify: `src/plc/S7Data.h`
- Modify: `src/plc/S7Data.cpp`
- Modify: `src/plc/PlcClient.h`
- Modify: `src/plc/PlcClient.cpp`
- Modify: `tests/test_s7_data.cpp`
- Modify: `tests/test_plc_client_state.cpp`

- [ ] **Step 1: Add little-endian INT32 helpers**

In `src/plc/S7Data.h`, declare:

```cpp
static std::array<std::uint8_t, 4> encodeInt32LittleEndian(qint32 value);
static qint32 decodeInt32LittleEndian(const std::array<std::uint8_t, 4>& data);
```

In `src/plc/S7Data.cpp`, implement:

```cpp
std::array<std::uint8_t, 4> S7Data::encodeInt32LittleEndian(qint32 value) {
    const auto raw = static_cast<quint32>(value);
    return {
        static_cast<std::uint8_t>(raw & 0xFF),
        static_cast<std::uint8_t>((raw >> 8) & 0xFF),
        static_cast<std::uint8_t>((raw >> 16) & 0xFF),
        static_cast<std::uint8_t>((raw >> 24) & 0xFF),
    };
}

qint32 S7Data::decodeInt32LittleEndian(const std::array<std::uint8_t, 4>& data) {
    const quint32 raw = static_cast<quint32>(data[0])
        | (static_cast<quint32>(data[1]) << 8)
        | (static_cast<quint32>(data[2]) << 16)
        | (static_cast<quint32>(data[3]) << 24);
    return static_cast<qint32>(raw);
}
```

- [ ] **Step 2: Test little-endian packing**

In `tests/test_s7_data.cpp`, add tests:

```cpp
void int32LittleEndianRoundTrip();
void int32LittleEndianMatchesPythonPacking();
```

Use these assertions:

```cpp
void TestS7Data::int32LittleEndianRoundTrip() {
    const auto encoded = S7Data::encodeInt32LittleEndian(-1900000);
    QCOMPARE(S7Data::decodeInt32LittleEndian(encoded), qint32(-1900000));
}

void TestS7Data::int32LittleEndianMatchesPythonPacking() {
    const auto encoded = S7Data::encodeInt32LittleEndian(120000);
    QCOMPARE(encoded[0], static_cast<std::uint8_t>(0xC0));
    QCOMPARE(encoded[1], static_cast<std::uint8_t>(0xD4));
    QCOMPARE(encoded[2], static_cast<std::uint8_t>(0x01));
    QCOMPARE(encoded[3], static_cast<std::uint8_t>(0x00));
}
```

- [ ] **Step 3: Add process-area interface**

In `src/plc/PlcClient.h`, add an interface after `IPlcByteAccess`:

```cpp
class IPlcProcessAccess {
public:
    virtual ~IPlcProcessAccess() = default;

    virtual PlcResult writeProcessOutputInt32(int byteOffset, qint32 value) = 0;
    virtual PlcResult readProcessOutputInt32(int byteOffset, qint32& value) = 0;
    virtual PlcResult readProcessInputInt32(int byteOffset, qint32& value) = 0;
    virtual PlcResult readProcessInputByte(int byteOffset, quint8& value) = 0;
};
```

Change:

```cpp
class PlcClient final : public IPlcByteAccess {
```

to:

```cpp
class PlcClient final : public IPlcByteAccess, public IPlcProcessAccess {
```

Add public overrides:

```cpp
PlcResult writeProcessOutputInt32(int byteOffset, qint32 value) override;
PlcResult readProcessOutputInt32(int byteOffset, qint32& value) override;
PlcResult readProcessInputInt32(int byteOffset, qint32& value) override;
PlcResult readProcessInputByte(int byteOffset, quint8& value) override;
```

- [ ] **Step 4: Implement process-area access**

In `src/plc/PlcClient.cpp`, add Snap7 constants:

```cpp
constexpr int s7AreaPa = S7AreaPA;
constexpr int s7AreaPe = S7AreaPE;
```

Add methods:

```cpp
PlcResult PlcClient::writeProcessOutputInt32(int byteOffset, qint32 value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    auto data = S7Data::encodeInt32LittleEndian(value);
    const int code = Cli_WriteArea(client_, s7AreaPa, 0, byteOffset, static_cast<int>(data.size()), s7WordLenByte, data.data());
    if (code != 0) {
        connected_ = false;
        return snap7Failure("write process output int32", code);
    }

    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::readProcessOutputInt32(int byteOffset, qint32& value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    std::array<std::uint8_t, 4> data{};
    const int code = Cli_ReadArea(client_, s7AreaPa, 0, byteOffset, static_cast<int>(data.size()), s7WordLenByte, data.data());
    if (code != 0) {
        connected_ = false;
        return snap7Failure("read process output int32", code);
    }

    value = S7Data::decodeInt32LittleEndian(data);
    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::readProcessInputInt32(int byteOffset, qint32& value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    std::array<std::uint8_t, 4> data{};
    const int code = Cli_ReadArea(client_, s7AreaPe, 0, byteOffset, static_cast<int>(data.size()), s7WordLenByte, data.data());
    if (code != 0) {
        connected_ = false;
        return snap7Failure("read process input int32", code);
    }

    value = S7Data::decodeInt32LittleEndian(data);
    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::readProcessInputByte(int byteOffset, quint8& value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    std::array<std::uint8_t, 1> data{};
    const int code = Cli_ReadArea(client_, s7AreaPe, 0, byteOffset, 1, s7WordLenByte, data.data());
    if (code != 0) {
        connected_ = false;
        return snap7Failure("read process input byte", code);
    }

    value = static_cast<quint8>(data[0]);
    lastError_.clear();
    return PlcResult::success();
}
```

- [ ] **Step 5: Add compile-only client test**

In `tests/test_plc_client_state.cpp`, add:

```cpp
void plcClientImplementsProcessAccessInterface();
```

Implementation:

```cpp
void TestPlcClientState::plcClientImplementsProcessAccessInterface() {
    PlcConfig config;
    PlcClient client(config);
    IPlcProcessAccess* processAccess = &client;

    QVERIFY(processAccess != nullptr);
    QVERIFY(!client.isConnected());
}
```

Do not call process-area methods in this test because that would contact hardware.

- [ ] **Step 6: Run PLC data tests**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_s7_data
cmake --build build --config Release --target test_plc_client_state
ctest --test-dir build -C Release -R "test_s7_data|test_plc_client_state" --output-on-failure
```

Expected:

```text
100% tests passed
```

- [ ] **Step 7: Commit Task 2**

```powershell
git add src\plc\S7Data.h src\plc\S7Data.cpp src\plc\PlcClient.h src\plc\PlcClient.cpp tests\test_s7_data.cpp tests\test_plc_client_state.cpp
git commit -m "feat: add plc process area access"
```

## Task 3: Add Robot Quaternion Helper

**Files:**
- Create: `src/robot/RobotQuaternion.h`
- Create: `src/robot/RobotQuaternion.cpp`
- Create: `tests/test_robot_quaternion.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create quaternion header**

Create `src/robot/RobotQuaternion.h`:

```cpp
#pragma once

#include <array>

struct RobotPose {
    std::array<double, 3> position{};
    std::array<double, 4> quaternion{};
};

// Converts the Python toQuaternion.calc_to_quaternion(vector, -30) behavior
// into a small C++ helper with no Python/SciPy runtime dependency.
class RobotQuaternion {
public:
    static std::array<double, 4> fromOrientationVector(double x, double y, double z);
    static RobotPose makePose(double x, double y, double z, double rx, double ry, double rz);

private:
    static std::array<double, 3> toEulerAngles(double i, double j, double k);
    static std::array<double, 4> toQuaternionWxyz(const std::array<double, 3>& eulerDegrees);
};
```

- [ ] **Step 2: Implement quaternion helper**

Create `src/robot/RobotQuaternion.cpp` with:

```cpp
#include "robot/RobotQuaternion.h"

#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kLocalRotationDeg = -30.0;

double degrees(double radians) { return radians * 180.0 / kPi; }
double radians(double degreesValue) { return degreesValue * kPi / 180.0; }

std::array<double, 4> multiplyXyzw(const std::array<double, 4>& a, const std::array<double, 4>& b) {
    const double ax = a[0], ay = a[1], az = a[2], aw = a[3];
    const double bx = b[0], by = b[1], bz = b[2], bw = b[3];
    return {
        aw * bx + ax * bw + ay * bz - az * by,
        aw * by - ax * bz + ay * bw + az * bx,
        aw * bz + ax * by - ay * bx + az * bw,
        aw * bw - ax * bx - ay * by - az * bz,
    };
}

std::array<double, 4> conjugateXyzw(const std::array<double, 4>& q) {
    return {-q[0], -q[1], -q[2], q[3]};
}

std::array<double, 3> rotateVectorByQuaternion(const std::array<double, 3>& v, const std::array<double, 4>& q) {
    const std::array<double, 4> p{v[0], v[1], v[2], 0.0};
    const auto rotated = multiplyXyzw(multiplyXyzw(q, p), conjugateXyzw(q));
    return {rotated[0], rotated[1], rotated[2]};
}

std::array<double, 4> fromRotVec(const std::array<double, 3>& axis, double angleRad) {
    const double norm = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    const double s = std::sin(angleRad / 2.0);
    return {axis[0] / norm * s, axis[1] / norm * s, axis[2] / norm * s, std::cos(angleRad / 2.0)};
}
}

std::array<double, 4> RobotQuaternion::fromOrientationVector(double x, double y, double z) {
    const auto euler = toEulerAngles(x, y, z);
    const auto qStartWxyz = toQuaternionWxyz(euler);
    const std::array<double, 4> qStartXyzw{qStartWxyz[1], qStartWxyz[2], qStartWxyz[3], qStartWxyz[0]};
    const auto axisWorld = rotateVectorByQuaternion({0.0, 0.0, 1.0}, qStartXyzw);
    const auto delta = fromRotVec(axisWorld, radians(kLocalRotationDeg));
    const auto qEndXyzw = multiplyXyzw(delta, qStartXyzw);
    return {-qEndXyzw[3], -qEndXyzw[0], -qEndXyzw[1], -qEndXyzw[2]};
}

RobotPose RobotQuaternion::makePose(double x, double y, double z, double rx, double ry, double rz) {
    return RobotPose{{x, y, z}, fromOrientationVector(rx, ry, rz)};
}

std::array<double, 3> RobotQuaternion::toEulerAngles(double i, double j, double k) {
    const double denom = i * i + j * j + k * k;
    double eulerX = degrees(std::acos(std::sqrt((i * i + k * k) / denom)));
    if (k > 0.0) {
        eulerX = 360.0 - eulerX;
    }
    if (k < 0.0 && j > 0.0) {
        eulerX = 360.0 - eulerX;
    }

    double eulerY = degrees(std::atan2(i, k));
    if (eulerY < 0.0) {
        eulerY += 180.0;
    }
    if (i < 0.0) {
        eulerY += 180.0;
    }
    return {eulerX, eulerY, 0.0};
}

std::array<double, 4> RobotQuaternion::toQuaternionWxyz(const std::array<double, 3>& eulerDegrees) {
    const double angleX = eulerDegrees[0];
    const double angleY = eulerDegrees[1];
    const double angleZ = eulerDegrees[2];

    const double cosX = std::cos(radians(angleX * 0.5));
    const double sinX = std::sin(radians(angleX * 0.5));
    const double cosY = std::cos(radians(angleY * 0.5));
    const double sinY = std::sin(radians(angleY * 0.5));
    const double cosZ = std::cos(radians(angleZ * 0.5));
    const double sinZ = std::sin(radians(angleZ * 0.5));

    return {
        cosX * cosY * cosZ + sinX * sinY * sinZ,
        sinX * cosY * cosZ - cosX * sinY * sinZ,
        cosX * sinY * cosZ + sinX * cosY * sinZ,
        cosX * cosY * sinZ - sinX * sinY * cosZ,
    };
}
```

- [ ] **Step 3: Create quaternion tests**

Create `tests/test_robot_quaternion.cpp`:

```cpp
#include <QtTest/QtTest>

#include "robot/RobotQuaternion.h"

class TestRobotQuaternion final : public QObject {
    Q_OBJECT

private slots:
    void matchesPythonForHomeVector();
    void matchesPythonForXAxisVector();
    void makePoseKeepsPositionAndQuaternion();
};

void compareQuaternion(const std::array<double, 4>& actual, const std::array<double, 4>& expected) {
    for (int i = 0; i < 4; ++i) {
        QVERIFY2(std::abs(actual[i] - expected[i]) < 1e-8, qPrintable(QString("index %1").arg(i)));
    }
}

void TestRobotQuaternion::matchesPythonForHomeVector() {
    compareQuaternion(RobotQuaternion::fromOrientationVector(0.0, 0.0, -1.0),
                      {-0.0, 0.2588190451, -0.9659258263, 0.0});
}

void TestRobotQuaternion::matchesPythonForXAxisVector() {
    compareQuaternion(RobotQuaternion::fromOrientationVector(1.0, 0.0, 0.0),
                      {-0.6830127019, 0.1830127019, -0.6830127019, 0.1830127019});
}

void TestRobotQuaternion::makePoseKeepsPositionAndQuaternion() {
    const RobotPose pose = RobotQuaternion::makePose(10.0, 20.0, 30.0, 0.0, 0.0, -1.0);
    QCOMPARE(pose.position[0], 10.0);
    QCOMPARE(pose.position[1], 20.0);
    QCOMPARE(pose.position[2], 30.0);
    compareQuaternion(pose.quaternion, {-0.0, 0.2588190451, -0.9659258263, 0.0});
}

QTEST_MAIN(TestRobotQuaternion)
#include "test_robot_quaternion.moc"
```

- [ ] **Step 4: Add CMake source and test target**

Add to `APP_SOURCES`:

```cmake
    src/robot/RobotQuaternion.h
    src/robot/RobotQuaternion.cpp
```

Add test target:

```cmake
set(TEST_ROBOT_QUATERNION_SOURCES
    tests/test_robot_quaternion.cpp
    src/robot/RobotQuaternion.cpp
)
add_executable(test_robot_quaternion ${TEST_ROBOT_QUATERNION_SOURCES})
target_include_directories(test_robot_quaternion PRIVATE src)
target_link_libraries(test_robot_quaternion PRIVATE Qt6::Core Qt6::Test)
copy_qt_test_runtime(test_robot_quaternion)
add_test(NAME test_robot_quaternion COMMAND test_robot_quaternion)
```

- [ ] **Step 5: Run quaternion tests**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_robot_quaternion
ctest --test-dir build -C Release -R test_robot_quaternion --output-on-failure
```

Expected:

```text
100% tests passed
```

- [ ] **Step 6: Commit Task 3**

```powershell
git add CMakeLists.txt src\robot\RobotQuaternion.h src\robot\RobotQuaternion.cpp tests\test_robot_quaternion.cpp
git commit -m "feat: add robot quaternion helper"
```

## Task 4: Add PLC-Forward Robot Motion Client

**Files:**
- Create: `src/robot/PlcRobotMotionClient.h`
- Create: `src/robot/PlcRobotMotionClient.cpp`
- Create: `tests/test_plc_robot_motion_client.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create client header**

Create `src/robot/PlcRobotMotionClient.h`:

```cpp
#pragma once

#include <array>

#include <QString>

#include "plc/PlcClient.h"
#include "robot/RobotQuaternion.h"

struct RobotMotionResult {
    bool ok = false;
    QString message;

    static RobotMotionResult success(const QString& message = QString()) { return {true, message}; }
    static RobotMotionResult failure(const QString& message) { return {false, message}; }
};

struct RobotSpeed {
    double tcpLinear = 200.0;
    double tcpOrientation = 200.0;
    double externalLinear = 200.0;
    double externalOrientation = 100.0;
};

class IRobotMotionController {
public:
    virtual ~IRobotMotionController() = default;

    virtual void setSpeed(const RobotSpeed& speed) = 0;
    virtual RobotMotionResult moveCartesian(const RobotPose& pose) = 0;
    virtual RobotMotionResult moveJoints(const std::array<double, 6>& joints) = 0;
    virtual RobotMotionResult moveExternalAxis(const std::array<double, 6>& axisValues) = 0;
};

class PlcRobotMotionClient final : public IRobotMotionController {
public:
    explicit PlcRobotMotionClient(IPlcProcessAccess& plc);
    PlcRobotMotionClient(IPlcProcessAccess& plc, int ackTimeoutMs, int doneTimeoutMs, int pollIntervalMs);

    void setSpeed(const RobotSpeed& speed) override;
    RobotMotionResult moveCartesian(const RobotPose& pose) override;
    RobotMotionResult moveJoints(const std::array<double, 6>& joints) override;
    RobotMotionResult moveExternalAxis(const std::array<double, 6>& axisValues) override;

    int sequenceOffset() const { return mdSendSeq_; }
    int commandOffset() const { return mdSendCmdCode_; }

private:
    RobotMotionResult writeSpeed();
    RobotMotionResult waitAck(qint32 sequence);
    RobotMotionResult waitDone(qint32 sequence);
    RobotMotionResult nextSequence(qint32& sequence);
    RobotMotionResult writeScaled(int byteOffset, double value);
    RobotMotionResult writeCommandAndSequence(qint32 command, qint32 sequence);

    IPlcProcessAccess& plc_;
    RobotSpeed speed_;
    std::array<double, 6> pendingExternalAxis_{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    int scale_ = 1000;
    int ackTimeoutMs_ = 10000;
    int doneTimeoutMs_ = 60000;
    int pollIntervalMs_ = 50;

    int mdSendPosX_ = 99;
    int mdSendPosY_ = 103;
    int mdSendPosZ_ = 107;
    int mdSendPosQ1_ = 111;
    int mdSendPosQ2_ = 115;
    int mdSendPosQ3_ = 119;
    int mdSendPosQ4_ = 123;
    int mdSendExtInc_ = 127;
    int mdSendExtSpin_ = 131;
    int mdSendExtGr_ = 135;
    int mdSendSpeedVTcp_ = 139;
    int mdSendSpeedVOri_ = 143;
    int mdSendSpeedVLeax_ = 147;
    int mdSendSpeedVReax_ = 151;
    int mdSendCmdCode_ = 155;
    int mdSendSeq_ = 159;

    int mdRecvAckSeq_ = 145;
    int mdRecvDoneSeq_ = 149;
    int mbRecvStatus_ = 153;
    int mbRecvErrCode_ = 154;
};
```

- [ ] **Step 2: Implement client**

Create `src/robot/PlcRobotMotionClient.cpp`. Key implementation rules:

- `nextSequence` reads process output INT32 at offset `159`, increments it, and returns the next sequence.
- Cartesian command writes position offsets `99, 103, 107`, quaternion offsets `111, 115, 119, 123`, pending external-axis offsets `135, 127, 131`, speed offsets, command `120`, and sequence.
- Joint command writes joints to offsets `99, 103, 107, 111, 115, 119`, writes `123 = 0`, speed offsets, command `2`, and sequence.
- External-axis command stores `pendingExternalAxis_`, writes offsets `135, 127, 131`, speed offsets, command `34`, and sequence.
- `waitAck` polls process input offset `145`, status byte `153`, and error code byte `154`.
- `waitDone` polls process input offset `149`, status byte `153`, and error code byte `154`.
- Status bit `0x02` is error; status bit `0x01` is busy.

Use `QElapsedTimer` and `QThread::msleep` like `PlcPrintSignalManager`.

The `writeScaled` helper:

```cpp
RobotMotionResult PlcRobotMotionClient::writeScaled(int byteOffset, double value) {
    const qint32 scaled = static_cast<qint32>(std::llround(value * scale_));
    const PlcResult result = plc_.writeProcessOutputInt32(byteOffset, scaled);
    if (!result.ok) {
        return RobotMotionResult::failure(QString("PLC robot write failed at byte %1: %2").arg(byteOffset).arg(result.message));
    }
    return RobotMotionResult::success();
}
```

- [ ] **Step 3: Create fake PLC tests**

Create `tests/test_plc_robot_motion_client.cpp` with a fake `IPlcProcessAccess`:

```cpp
class FakeProcessAccess final : public IPlcProcessAccess {
public:
    QHash<int, qint32> paInt32;
    QHash<int, qint32> peInt32;
    QHash<int, quint8> peByte;
    QStringList operations;

    PlcResult writeProcessOutputInt32(int byteOffset, qint32 value) override {
        operations << QString("write:%1:%2").arg(byteOffset).arg(value);
        paInt32[byteOffset] = value;
        return PlcResult::success();
    }
    PlcResult readProcessOutputInt32(int byteOffset, qint32& value) override {
        operations << QString("read-pa:%1").arg(byteOffset);
        value = paInt32.value(byteOffset, 0);
        return PlcResult::success();
    }
    PlcResult readProcessInputInt32(int byteOffset, qint32& value) override {
        operations << QString("read-pe:%1").arg(byteOffset);
        value = peInt32.value(byteOffset, 0);
        return PlcResult::success();
    }
    PlcResult readProcessInputByte(int byteOffset, quint8& value) override {
        operations << QString("read-byte:%1").arg(byteOffset);
        value = peByte.value(byteOffset, 0);
        return PlcResult::success();
    }
};
```

Required test cases:

```cpp
void cartesianWritesCompositeCommandAndSequence();
void jointsWriteAbsJCommand();
void externalAxisWritesPythonAxisOrder();
void ackTimeoutReturnsFailure();
void doneTimeoutReturnsFailure();
void errorStatusReturnsFailure();
```

Use short timeout constructor `PlcRobotMotionClient(plc, 5, 5, 1)` for timeout tests.

For successful command tests, set:

```cpp
plc.paInt32[159] = 41;
plc.peInt32[145] = 42;
plc.peInt32[149] = 42;
plc.peByte[153] = 0;
```

Assert command offset and sequence:

```cpp
QCOMPARE(plc.paInt32.value(155), qint32(120));
QCOMPARE(plc.paInt32.value(159), qint32(42));
```

- [ ] **Step 4: Add CMake source and test target**

Add to `APP_SOURCES`:

```cmake
    src/robot/PlcRobotMotionClient.h
    src/robot/PlcRobotMotionClient.cpp
```

Add test target with `tests/test_plc_robot_motion_client.cpp`, `src/robot/PlcRobotMotionClient.cpp`, `src/robot/RobotQuaternion.cpp`, `src/plc/PlcClient.cpp`, and `src/plc/S7Data.cpp`, linking `Qt6::Core Qt6::Test snap7::snap7`.

- [ ] **Step 5: Run motion client tests**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_plc_robot_motion_client
ctest --test-dir build -C Release -R test_plc_robot_motion_client --output-on-failure
```

Expected:

```text
100% tests passed
```

- [ ] **Step 6: Commit Task 4**

```powershell
git add CMakeLists.txt src\robot\PlcRobotMotionClient.h src\robot\PlcRobotMotionClient.cpp tests\test_plc_robot_motion_client.cpp
git commit -m "feat: add plc robot motion client"
```

## Task 5: Add Robot Movement Service and UI Wiring

**Files:**
- Create: `src/services/robot/RobotMovementService.h`
- Create: `src/services/robot/RobotMovementService.cpp`
- Create: `tests/test_robot_movement_service.cpp`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create movement service header**

Create `src/services/robot/RobotMovementService.h`:

```cpp
#pragma once

#include <QObject>

#include <memory>

#include "app/AppConfig.h"
#include "plc/PlcClient.h"
#include "robot/PlcRobotMotionClient.h"

class QLineEdit;

QT_BEGIN_NAMESPACE
namespace Ui { class ContinuosfiberInterface; }
QT_END_NAMESPACE

class RobotMovementService final : public QObject {
    Q_OBJECT

public:
    RobotMovementService(const RobotConfig& robotConfig,
                         const PlcConfig& plcConfig,
                         Ui::ContinuosfiberInterface* ui,
                         QObject* parent = nullptr);
    RobotMovementService(const RobotConfig& robotConfig,
                         IRobotMotionController& motionClient,
                         Ui::ContinuosfiberInterface* ui,
                         QObject* parent = nullptr);

signals:
    void messageRequested(const QString& title, const QString& message);

public slots:
    void moveRobot();
    void resetRobot();
    void moveRobotAxes();
    void moveExternalAxis();
    void resetExternalAxis();

private:
    bool readRequiredDouble(QLineEdit* field, const QString& name, double& value);
    double readOptionalDouble(QLineEdit* field, double fallback);
    RobotSpeed readSpeed() const;
    void reportMotionResult(const QString& action, const RobotMotionResult& result);

    Ui::ContinuosfiberInterface* ui_ = nullptr;
    RobotConfig robotConfig_;
    std::unique_ptr<PlcClient> ownedPlc_;
    std::unique_ptr<PlcRobotMotionClient> ownedMotionClient_;
    IRobotMotionController& motionClient_;
};
```

- [ ] **Step 2: Implement movement service**

Create `src/services/robot/RobotMovementService.cpp`.

Important behavior:

- `moveRobot` requires six Cartesian/orientation fields, builds `RobotQuaternion::makePose(x, y, z, rx, ry, rz)`, sets speed, and calls `moveCartesian`.
- `resetRobot` requires zero-position X/Y/Z, builds `RobotQuaternion::makePose(x, y, z, 0, 0, -1)`, sets speed, and calls `moveCartesian`.
- `moveRobotAxes` requires six axis fields, sets speed, and calls `moveJoints({a1, a2, a3, a4, a5, a6})`.
- `moveExternalAxis` requires three positioner fields and calls `moveExternalAxis({axis3, axis1, axis2, 0, 0, 0})`.
- `resetExternalAxis` calls `moveExternalAxis({-1900, 0, 0, 0, 0, 0})`.
- `readSpeed` starts from `robotConfig_.defaultSpeed` and overrides from `RobotlinespeedlineEdit`, `RobotangularspeedlineEdit`, `AxislinespeedlineEdit`, and `AxisangularspeedlineEdit` when fields are non-empty.

Use `messageRequested("Robot control", "...")` for validation failures and `messageRequested("Robot", "...")` for hardware failures.

- [ ] **Step 3: Add movement service tests**

Create `tests/test_robot_movement_service.cpp` and include `robot/PlcRobotMotionClient.h` so the test can use the `IRobotMotionController` interface from Task 4.

Use this fake in the service tests:

```cpp
class FakeMotionController final : public IRobotMotionController {
public:
    RobotSpeed lastSpeed;
    QStringList commands;
    RobotPose lastPose;
    std::array<double, 6> lastJoints{};
    std::array<double, 6> lastExternalAxis{};
    RobotMotionResult nextResult = RobotMotionResult::success();

    void setSpeed(const RobotSpeed& speed) override {
        lastSpeed = speed;
    }

    RobotMotionResult moveCartesian(const RobotPose& pose) override {
        commands << "cartesian";
        lastPose = pose;
        return nextResult;
    }

    RobotMotionResult moveJoints(const std::array<double, 6>& joints) override {
        commands << "joints";
        lastJoints = joints;
        return nextResult;
    }

    RobotMotionResult moveExternalAxis(const std::array<double, 6>& axisValues) override {
        commands << "external";
        lastExternalAxis = axisValues;
        return nextResult;
    }
};
```

Required movement service tests:

- missing Cartesian field emits one message and sends no motion command.
- Cartesian move uses configured speeds when speed fields are empty.
- Cartesian move overrides speeds from UI fields.
- robot home uses `[0, 0, -1]` orientation vector.
- joint move sends six axis values in order.
- external-axis move sends `[Axis3, Axis1, Axis2, 0, 0, 0]`.
- external-axis reset sends `[-1900, 0, 0, 0, 0, 0]`.

Use `Ui::ContinuosfiberInterface`, call `setupUi(&widget)`, set the line edits, and run service slots directly.

- [ ] **Step 4: Wire `MainWindow`**

In `src/ui/MainWindow.h`, include:

```cpp
#include "services/robot/RobotMovementService.h"
```

Add member after `robotPrintService_`:

```cpp
RobotMovementService robotMovementService_;
```

In `MainWindow.cpp` constructor initializer list, construct it after `robotPrintService_`:

```cpp
robotMovementService_(config_.robot, config_.plc, ui_, this),
```

In `connectSignals`, connect its messages:

```cpp
connect(&robotMovementService_, &RobotMovementService::messageRequested,
        this, &MainWindow::showPlaceholderMessage);
```

Change only these five button connections:

```cpp
connect(ui_->Robot_movebutton, &QPushButton::clicked, &robotMovementService_, &RobotMovementService::moveRobot);
connect(ui_->RobotGohomeButton, &QPushButton::clicked, &robotMovementService_, &RobotMovementService::resetRobot);
connect(ui_->RobotaxismoveButton, &QPushButton::clicked, &robotMovementService_, &RobotMovementService::moveRobotAxes);
connect(ui_->PositionermoveButton, &QPushButton::clicked, &robotMovementService_, &RobotMovementService::moveExternalAxis);
connect(ui_->PositionercomebackButton, &QPushButton::clicked, &robotMovementService_, &RobotMovementService::resetExternalAxis);
```

Do not change print button connections.

- [ ] **Step 5: Add CMake source and test target**

Add service files to `APP_SOURCES` and create `test_robot_movement_service` target linking `Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test snap7::snap7`. Copy Qt GUI/Widgets runtime DLLs like `test_ftp_service`.

- [ ] **Step 6: Build app and run tests**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release -R "test_robot_movement_service|test_plc_robot_motion_client|test_robot_quaternion|test_app_config" --output-on-failure
```

Expected:

```text
100% tests passed
```

- [ ] **Step 7: Commit Task 5**

```powershell
git add CMakeLists.txt src\services\robot\RobotMovementService.h src\services\robot\RobotMovementService.cpp src\ui\MainWindow.h src\ui\MainWindow.cpp tests\test_robot_movement_service.cpp
git commit -m "feat: wire plc robot movement controls"
```

## Task 6: Document and Verify Slice G

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Update README summary**

Add a Slice G summary near the top:

```markdown
Slice G adds PLC-forward ABB robot movement controls for Cartesian move, robot home, joint move, external-axis move, and external-axis reset. It uses the existing UI and the `robot.transport.type = "plc"` path.
```

- [ ] **Step 2: Add movement workflow documentation**

Add a section after Robot Print Workflow:

````markdown
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

These values match the Python defaults. Empty UI speed fields use these config values; filled UI speed fields override them for the command.

This slice does not convert socket-based robot movement, RAPID files, live pose readback, or robot monitoring sockets.
````

- [ ] **Step 3: Run full verification**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
git -C ..\AFP_ABB_Python status --short
```

Expected:

```text
100% tests passed
```

Expected Python status: no output.

- [ ] **Step 4: Commit Task 6**

```powershell
git add README.md
git commit -m "docs: document plc robot movement controls"
```

## Self-Review Checklist

- [ ] `AFP_ABB_Python` is untouched.
- [ ] `src/ui/Interface4600_window.ui` is untouched.
- [ ] Default robot movement speeds are loaded from config, with Python-compatible fallbacks.
- [ ] PLC process-area INT32 uses little-endian packing.
- [ ] Cartesian command writes command `120`.
- [ ] Joint command writes command `2`.
- [ ] External-axis command writes command `34`.
- [ ] ACK wait uses offset `145`, status `153`, error code `154`.
- [ ] DONE wait uses offset `149`, status `153`, error code `154`.
- [ ] External-axis move preserves Python order `[Axis3, Axis1, Axis2, 0, 0, 0]`.
- [ ] External-axis reset sends `[-1900, 0, 0, 0, 0, 0]`.
- [ ] Print workflow button wiring remains on `RobotPrintService`.
- [ ] Movement buttons are no longer wired to placeholder `RobotService`.
- [ ] Tests do not require live PLC, robot, FTP, pressure, or camera hardware.
- [ ] README documents the movement behavior and config keys.
