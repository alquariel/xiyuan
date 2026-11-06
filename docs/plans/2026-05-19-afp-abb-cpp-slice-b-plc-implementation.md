# Slice B PLC Direct UI Controls Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Convert the Python direct Siemens S7-1200 PLC UI controls into the C++ Qt application while keeping `AFP_ABB_Python` read-only.

**Architecture:** Add a small PLC layer under `src/plc` for S7 data encoding and Snap7 access, then add a Qt-facing PLC service under `src/services/plc`. `MainWindow` continues to own UI wiring and passes the generated UI plus `AppConfig::plc` into the real PLC service.

**Tech Stack:** C++17, Qt 6 Widgets/Test, Snap7 from vcpkg target `snap7::snap7`, CMake, CTest, Windows portable deployment through `windeployqt`.

---

## File Structure

- Create `src/plc/S7Data.h`: inline declarations for Siemens byte/bit and big-endian numeric helpers.
- Create `src/plc/S7Data.cpp`: helper implementations with comments for S7 memory layout.
- Create `tests/test_s7_data.cpp`: Qt unit tests for bit, INT, and REAL encoding.
- Create `src/plc/PlcClient.h`: low-level Snap7 client interface and result type.
- Create `src/plc/PlcClient.cpp`: Snap7 connect, disconnect, DB bit/int/real, and M bit operations.
- Create `tests/test_plc_client_state.cpp`: hardware-free tests for client construction and disconnected state.
- Create `src/services/plc/PlcService.h`: Qt slots for the approved direct PLC UI controls.
- Create `src/services/plc/PlcService.cpp`: PLC UI behavior migrated from `plc_service.py`.
- Modify `src/services/PlaceholderServices.h`: remove the old PLC service class from the generic temporary services file.
- Modify `src/services/PlaceholderServices.cpp`: remove old PLC service method definitions.
- Modify `src/ui/MainWindow.h`: include the real PLC service header.
- Modify `src/ui/MainWindow.cpp`: connect existing PLC buttons to the real PLC service and route service messages to the existing message box slot.
- Modify `CMakeLists.txt`: add Snap7, PLC sources, and PLC tests.
- Modify `cmake/DeployQt.cmake`: copy `snap7.dll` into the portable output folder.
- Modify `README.md`: document the Snap7 dependency and portable PLC deployment check.

The generated UI file `src/ui/Interface4600_window.ui` remains unchanged.

---

### Task 1: Add S7 Data Encoding Helpers

**Files:**
- Create: `src/plc/S7Data.h`
- Create: `src/plc/S7Data.cpp`
- Create: `tests/test_s7_data.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing S7 data tests**

Create `tests/test_s7_data.cpp`:

```cpp
#include <QtTest/QtTest>

#include <array>
#include <cstdint>

#include "plc/S7Data.h"

class TestS7Data final : public QObject {
    Q_OBJECT

private slots:
    void setBitSetsOnlyRequestedBit();
    void encodeDecodeInt16UsesBigEndianOrder();
    void encodeDecodeReal32UsesBigEndianOrder();
};

void TestS7Data::setBitSetsOnlyRequestedBit() {
    quint8 value = 0x00;
    S7Data::setBit(value, 5, true);
    QCOMPARE(value, static_cast<quint8>(0x20));

    S7Data::setBit(value, 1, true);
    QCOMPARE(value, static_cast<quint8>(0x22));

    S7Data::setBit(value, 5, false);
    QCOMPARE(value, static_cast<quint8>(0x02));
}

void TestS7Data::encodeDecodeInt16UsesBigEndianOrder() {
    const auto bytes = S7Data::encodeInt16(1025);
    QCOMPARE(bytes[0], static_cast<std::uint8_t>(0x04));
    QCOMPARE(bytes[1], static_cast<std::uint8_t>(0x01));
    QCOMPARE(S7Data::decodeInt16(bytes), static_cast<std::int16_t>(1025));
}

void TestS7Data::encodeDecodeReal32UsesBigEndianOrder() {
    const auto bytes = S7Data::encodeReal32(12.5f);
    QCOMPARE(bytes[0], static_cast<std::uint8_t>(0x41));
    QCOMPARE(bytes[1], static_cast<std::uint8_t>(0x48));
    QCOMPARE(bytes[2], static_cast<std::uint8_t>(0x00));
    QCOMPARE(bytes[3], static_cast<std::uint8_t>(0x00));
    QCOMPARE(S7Data::decodeReal32(bytes), 12.5f);
}

QTEST_MAIN(TestS7Data)
#include "test_s7_data.moc"
```

- [ ] **Step 2: Add the test target before the helpers exist**

Modify `CMakeLists.txt` by adding this block after the `test_app_config` block:

```cmake
set(TEST_S7_DATA_SOURCES
    tests/test_s7_data.cpp
    src/plc/S7Data.cpp
)
set(TEST_S7_DATA_READY TRUE)
set(MISSING_TEST_S7_DATA_SOURCES)
foreach(TEST_SOURCE ${TEST_S7_DATA_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        set(TEST_S7_DATA_READY FALSE)
        list(APPEND MISSING_TEST_S7_DATA_SOURCES ${TEST_SOURCE})
    endif()
endforeach()

if(TEST_S7_DATA_READY)
    add_executable(test_s7_data
        ${TEST_S7_DATA_SOURCES}
    )
    target_include_directories(test_s7_data PRIVATE src)
    target_link_libraries(test_s7_data PRIVATE Qt6::Core Qt6::Test)
    copy_qt_test_runtime(test_s7_data)
    add_test(NAME test_s7_data COMMAND test_s7_data)
elseif(MISSING_TEST_S7_DATA_SOURCES)
    string(REPLACE ";" "\n  " MISSING_TEST_S7_DATA_TEXT "${MISSING_TEST_S7_DATA_SOURCES}")
    message(STATUS "test_s7_data target deferred. Missing files:\n  ${MISSING_TEST_S7_DATA_TEXT}")
endif()
```

- [ ] **Step 3: Run configure and confirm the new target is deferred**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64;C:\vcpkg\installed\x64-windows"
```

Expected: configure succeeds and reports `test_s7_data target deferred` because `src/plc/S7Data.cpp` does not exist yet.

- [ ] **Step 4: Add the S7 data helper header**

Create `src/plc/S7Data.h`:

```cpp
#pragma once

#include <array>
#include <cstdint>

#include <QtGlobal>

namespace S7Data {

bool bitIsSet(quint8 value, int bitIndex);
void setBit(quint8& value, int bitIndex, bool enabled);

std::array<std::uint8_t, 2> encodeInt16(std::int16_t value);
std::int16_t decodeInt16(const std::array<std::uint8_t, 2>& bytes);

std::array<std::uint8_t, 4> encodeReal32(float value);
float decodeReal32(const std::array<std::uint8_t, 4>& bytes);

}
```

- [ ] **Step 5: Add the S7 data helper implementation**

Create `src/plc/S7Data.cpp`:

```cpp
#include "plc/S7Data.h"

#include <cstring>

namespace {
quint8 bitMask(int bitIndex) {
    return static_cast<quint8>(1u << bitIndex);
}
}

namespace S7Data {

bool bitIsSet(quint8 value, int bitIndex) {
    return (value & bitMask(bitIndex)) != 0;
}

void setBit(quint8& value, int bitIndex, bool enabled) {
    if (enabled) {
        value = static_cast<quint8>(value | bitMask(bitIndex));
    } else {
        value = static_cast<quint8>(value & ~bitMask(bitIndex));
    }
}

std::array<std::uint8_t, 2> encodeInt16(std::int16_t value) {
    const auto unsignedValue = static_cast<std::uint16_t>(value);
    return {
        static_cast<std::uint8_t>((unsignedValue >> 8) & 0xFF),
        static_cast<std::uint8_t>(unsignedValue & 0xFF)
    };
}

std::int16_t decodeInt16(const std::array<std::uint8_t, 2>& bytes) {
    const std::uint16_t combined = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(bytes[0]) << 8) |
        static_cast<std::uint16_t>(bytes[1]));
    return static_cast<std::int16_t>(combined);
}

std::array<std::uint8_t, 4> encodeReal32(float value) {
    std::uint32_t raw = 0;
    std::memcpy(&raw, &value, sizeof(value));
    return {
        static_cast<std::uint8_t>((raw >> 24) & 0xFF),
        static_cast<std::uint8_t>((raw >> 16) & 0xFF),
        static_cast<std::uint8_t>((raw >> 8) & 0xFF),
        static_cast<std::uint8_t>(raw & 0xFF)
    };
}

float decodeReal32(const std::array<std::uint8_t, 4>& bytes) {
    const std::uint32_t raw =
        (static_cast<std::uint32_t>(bytes[0]) << 24) |
        (static_cast<std::uint32_t>(bytes[1]) << 16) |
        (static_cast<std::uint32_t>(bytes[2]) << 8) |
        static_cast<std::uint32_t>(bytes[3]);

    float value = 0.0f;
    std::memcpy(&value, &raw, sizeof(value));
    return value;
}

}
```

- [ ] **Step 6: Run the S7 data test**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64;C:\vcpkg\installed\x64-windows"
cmake --build build --config Release --target test_s7_data
ctest --test-dir build -C Release -R test_s7_data --output-on-failure
```

Expected: `100% tests passed, 0 tests failed out of 1`.

- [ ] **Step 7: Commit Task 1**

Run:

```powershell
git add CMakeLists.txt src/plc/S7Data.h src/plc/S7Data.cpp tests/test_s7_data.cpp
git commit -m "test: add s7 data helpers"
```

---

### Task 2: Add Snap7 Client Wrapper

**Files:**
- Create: `src/plc/PlcClient.h`
- Create: `src/plc/PlcClient.cpp`
- Create: `tests/test_plc_client_state.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the hardware-free client state test**

Create `tests/test_plc_client_state.cpp`:

```cpp
#include <QtTest/QtTest>

#include "app/AppConfig.h"
#include "plc/PlcClient.h"

class TestPlcClientState final : public QObject {
    Q_OBJECT

private slots:
    void newClientStartsDisconnected();
};

void TestPlcClientState::newClientStartsDisconnected() {
    PlcConfig config;
    config.ip = "192.168.0.201";
    config.rack = 0;
    config.slot = 1;

    PlcClient client(config);

    QVERIFY(!client.isConnected());
    QVERIFY(client.lastError().isEmpty());
}

QTEST_MAIN(TestPlcClientState)
#include "test_plc_client_state.moc"
```

- [ ] **Step 2: Add Snap7 and the client test target before the client exists**

Modify `CMakeLists.txt` near the Qt package line:

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)
find_package(snap7 CONFIG REQUIRED)
```

Add these client sources to `APP_SOURCES`:

```cmake
    src/plc/S7Data.h
    src/plc/S7Data.cpp
    src/plc/PlcClient.h
    src/plc/PlcClient.cpp
```

Add `snap7::snap7` to `target_link_libraries(AFP_ABB_CPP PRIVATE ...)`:

```cmake
        snap7::snap7
```

Add this test block after the `test_s7_data` block:

```cmake
set(TEST_PLC_CLIENT_STATE_SOURCES
    tests/test_plc_client_state.cpp
    src/plc/PlcClient.cpp
    src/plc/S7Data.cpp
)
set(TEST_PLC_CLIENT_STATE_READY TRUE)
set(MISSING_TEST_PLC_CLIENT_STATE_SOURCES)
foreach(TEST_SOURCE ${TEST_PLC_CLIENT_STATE_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        set(TEST_PLC_CLIENT_STATE_READY FALSE)
        list(APPEND MISSING_TEST_PLC_CLIENT_STATE_SOURCES ${TEST_SOURCE})
    endif()
endforeach()

if(TEST_PLC_CLIENT_STATE_READY)
    add_executable(test_plc_client_state
        ${TEST_PLC_CLIENT_STATE_SOURCES}
    )
    target_include_directories(test_plc_client_state PRIVATE src)
    target_link_libraries(test_plc_client_state PRIVATE Qt6::Core Qt6::Test snap7::snap7)
    copy_qt_test_runtime(test_plc_client_state)
    add_custom_command(TARGET test_plc_client_state POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:snap7::snap7>
                $<TARGET_FILE_DIR:test_plc_client_state>
    )
    add_test(NAME test_plc_client_state COMMAND test_plc_client_state)
elseif(MISSING_TEST_PLC_CLIENT_STATE_SOURCES)
    string(REPLACE ";" "\n  " MISSING_TEST_PLC_CLIENT_STATE_TEXT "${MISSING_TEST_PLC_CLIENT_STATE_SOURCES}")
    message(STATUS "test_plc_client_state target deferred. Missing files:\n  ${MISSING_TEST_PLC_CLIENT_STATE_TEXT}")
endif()
```

- [ ] **Step 3: Add the PLC client header**

Create `src/plc/PlcClient.h`:

```cpp
#pragma once

#include <QString>

#include "app/AppConfig.h"

class TS7Client;

struct PlcResult {
    bool ok = false;
    QString message;

    static PlcResult success();
    static PlcResult failure(const QString& message);
};

class PlcClient final {
public:
    explicit PlcClient(PlcConfig config);
    ~PlcClient();

    PlcClient(const PlcClient&) = delete;
    PlcClient& operator=(const PlcClient&) = delete;

    bool isConnected() const;
    QString lastError() const;

    PlcResult connect();
    void disconnect();

    PlcResult writeDbBit(int dbNumber, int byteOffset, int bitIndex, bool value);
    PlcResult readDbBit(int dbNumber, int byteOffset, int bitIndex, bool& value);
    PlcResult writeDbInt16(int dbNumber, int byteOffset, qint16 value);
    PlcResult writeDbReal32(int dbNumber, int byteOffset, float value);
    PlcResult writeMBit(int byteOffset, int bitIndex, bool value);

private:
    PlcResult ensureConnected();
    PlcResult snap7Failure(const QString& action, int code);

    PlcConfig config_;
    TS7Client* client_ = nullptr;
    QString lastError_;
};
```

- [ ] **Step 4: Add the PLC client implementation**

Create `src/plc/PlcClient.cpp`:

```cpp
#include "plc/PlcClient.h"

#include <array>
#include <utility>

#include <QByteArray>
#include <snap7/snap7.h>

#include "plc/S7Data.h"

namespace {
constexpr int s7AreaDb = S7AreaDB;
constexpr int s7AreaMk = S7AreaMK;
constexpr int s7WordLenByte = S7WLByte;
}

PlcResult PlcResult::success() {
    return {true, QString()};
}

PlcResult PlcResult::failure(const QString& message) {
    return {false, message};
}

PlcClient::PlcClient(PlcConfig config)
    : config_(std::move(config)),
      client_(new TS7Client) {
}

PlcClient::~PlcClient() {
    disconnect();
    delete client_;
}

bool PlcClient::isConnected() const {
    return client_ != nullptr && client_->Connected();
}

QString PlcClient::lastError() const {
    return lastError_;
}

PlcResult PlcClient::connect() {
    if (isConnected()) {
        return PlcResult::success();
    }

    const QByteArray ip = config_.ip.toUtf8();
    const int result = client_->ConnectTo(ip.constData(), config_.rack, config_.slot);
    if (result != 0) {
        return snap7Failure("connect", result);
    }

    lastError_.clear();
    return PlcResult::success();
}

void PlcClient::disconnect() {
    if (client_ != nullptr && client_->Connected()) {
        client_->Disconnect();
    }
}

PlcResult PlcClient::writeDbBit(int dbNumber, int byteOffset, int bitIndex, bool value) {
    bool currentBit = false;
    PlcResult readResult = readDbBit(dbNumber, byteOffset, bitIndex, currentBit);
    if (!readResult.ok) {
        return readResult;
    }

    std::array<std::uint8_t, 1> data{};
    int readCode = client_->ReadArea(s7AreaDb, dbNumber, byteOffset, 1, s7WordLenByte, data.data());
    if (readCode != 0) {
        return snap7Failure("read DB byte", readCode);
    }

    quint8 byteValue = static_cast<quint8>(data[0]);
    S7Data::setBit(byteValue, bitIndex, value);
    data[0] = static_cast<std::uint8_t>(byteValue);

    int writeCode = client_->WriteArea(s7AreaDb, dbNumber, byteOffset, 1, s7WordLenByte, data.data());
    if (writeCode != 0) {
        return snap7Failure("write DB bit", writeCode);
    }

    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::readDbBit(int dbNumber, int byteOffset, int bitIndex, bool& value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    std::array<std::uint8_t, 1> data{};
    int code = client_->ReadArea(s7AreaDb, dbNumber, byteOffset, 1, s7WordLenByte, data.data());
    if (code != 0) {
        return snap7Failure("read DB bit", code);
    }

    value = S7Data::bitIsSet(static_cast<quint8>(data[0]), bitIndex);
    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::writeDbInt16(int dbNumber, int byteOffset, qint16 value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    auto data = S7Data::encodeInt16(value);
    int code = client_->WriteArea(s7AreaDb, dbNumber, byteOffset, static_cast<int>(data.size()), s7WordLenByte, data.data());
    if (code != 0) {
        return snap7Failure("write DB int", code);
    }

    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::writeDbReal32(int dbNumber, int byteOffset, float value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    auto data = S7Data::encodeReal32(value);
    int code = client_->WriteArea(s7AreaDb, dbNumber, byteOffset, static_cast<int>(data.size()), s7WordLenByte, data.data());
    if (code != 0) {
        return snap7Failure("write DB real", code);
    }

    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::writeMBit(int byteOffset, int bitIndex, bool value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    std::array<std::uint8_t, 1> data{};
    int readCode = client_->ReadArea(s7AreaMk, 0, byteOffset, 1, s7WordLenByte, data.data());
    if (readCode != 0) {
        return snap7Failure("read M byte", readCode);
    }

    quint8 byteValue = static_cast<quint8>(data[0]);
    S7Data::setBit(byteValue, bitIndex, value);
    data[0] = static_cast<std::uint8_t>(byteValue);

    int writeCode = client_->WriteArea(s7AreaMk, 0, byteOffset, 1, s7WordLenByte, data.data());
    if (writeCode != 0) {
        return snap7Failure("write M bit", writeCode);
    }

    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::ensureConnected() {
    if (isConnected()) {
        return PlcResult::success();
    }
    return connect();
}

PlcResult PlcClient::snap7Failure(const QString& action, int code) {
    lastError_ = QString("PLC %1 failed with Snap7 error %2").arg(action).arg(code);
    return PlcResult::failure(lastError_);
}
```

- [ ] **Step 5: Run the client state test**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64;C:\vcpkg\installed\x64-windows"
cmake --build build --config Release --target test_plc_client_state
ctest --test-dir build -C Release -R test_plc_client_state --output-on-failure
```

Expected: `100% tests passed, 0 tests failed out of 1`.

- [ ] **Step 6: Commit Task 2**

Run:

```powershell
git add CMakeLists.txt src/plc/PlcClient.h src/plc/PlcClient.cpp tests/test_plc_client_state.cpp
git commit -m "feat: add snap7 plc client"
```

---

### Task 3: Replace the Temporary PLC Service With a Real Qt Service Skeleton

**Files:**
- Create: `src/services/plc/PlcService.h`
- Create: `src/services/plc/PlcService.cpp`
- Modify: `src/services/PlaceholderServices.h`
- Modify: `src/services/PlaceholderServices.cpp`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add the real PLC service header**

Create `src/services/plc/PlcService.h`:

```cpp
#pragma once

#include <QObject>
#include <QString>

#include "app/AppConfig.h"
#include "plc/PlcClient.h"

class QLineEdit;

QT_BEGIN_NAMESPACE
namespace Ui { class ContinuosfiberInterface; }
QT_END_NAMESPACE

class PlcService final : public QObject {
    Q_OBJECT

public:
    PlcService(const PlcConfig& config, Ui::ContinuosfiberInterface* ui, QObject* parent = nullptr);

signals:
    void messageRequested(const QString& title, const QString& message);

public slots:
    void disconnectPlc();
    void cylinder1();
    void cylinder2();
    void cylinder3();
    void motorEnable();
    void motorReset();
    void motorActuation();
    void parameterInput();
    void laserConnect();
    void setTargetTemperature();
    void setLaserPower();
    void toggleTemperatureControl();
    void togglePowerControl();
    void openRedLight();
    void closeRedLight();
    void laserStart();
    void laserStop();
    void setHysteresisRatio();

private:
    bool ensureConnected();
    bool runCommand(const QString& action, const PlcResult& result);
    bool writeDbBitConfirmed(const QString& action, int byteOffset, int bitIndex, bool value);
    double readDoubleField(const QLineEdit* lineEdit, double fallback) const;
    int readIntField(const QLineEdit* lineEdit, int fallback) const;
    void showInfo(const QString& message);

    Ui::ContinuosfiberInterface* ui_ = nullptr;
    PlcClient client_;

    bool motorEnabled_ = false;
    bool redLight_ = false;
    bool laserState_ = false;
    bool targetSet_ = false;
    bool temperatureControl_ = false;
    bool powerSet_ = false;
    bool powerControl_ = false;
    bool cylinder2On_ = false;
    bool cylinder3On_ = true;
};
```

- [ ] **Step 2: Add a compiling service skeleton**

Create `src/services/plc/PlcService.cpp`:

```cpp
#include "services/plc/PlcService.h"

#include <QAbstractButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBrowser>

#include "ui_Interface4600_window.h"

namespace {
constexpr int kDb5 = 5;
constexpr int kConfirmRetries = 3;
}

PlcService::PlcService(const PlcConfig& config, Ui::ContinuosfiberInterface* ui, QObject* parent)
    : QObject(parent),
      ui_(ui),
      client_(config) {
}

void PlcService::disconnectPlc() {
    client_.disconnect();
    showInfo("PLC disconnected.");
}

void PlcService::cylinder1() { showInfo("Cylinder 1 PLC control will be active after Task 4."); }
void PlcService::cylinder2() { showInfo("Cylinder 2 PLC control will be active after Task 4."); }
void PlcService::cylinder3() { showInfo("Cylinder 3 PLC control will be active after Task 4."); }
void PlcService::motorEnable() { showInfo("Motor enable PLC control will be active after Task 4."); }
void PlcService::motorReset() { showInfo("Motor reset PLC control will be active after Task 4."); }
void PlcService::motorActuation() { showInfo("Motor actuation PLC control will be active after Task 4."); }
void PlcService::parameterInput() { showInfo("Motor parameter PLC control will be active after Task 4."); }
void PlcService::laserConnect() { showInfo("Laser PLC control will be active after Task 5."); }
void PlcService::setTargetTemperature() { showInfo("Temperature target PLC control will be active after Task 5."); }
void PlcService::setLaserPower() { showInfo("Laser power PLC control will be active after Task 5."); }
void PlcService::toggleTemperatureControl() { showInfo("Temperature control PLC toggle will be active after Task 5."); }
void PlcService::togglePowerControl() { showInfo("Power control PLC toggle will be active after Task 5."); }
void PlcService::openRedLight() { showInfo("Red light PLC control will be active after Task 5."); }
void PlcService::closeRedLight() { showInfo("Red light PLC control will be active after Task 5."); }
void PlcService::laserStart() { showInfo("Laser start PLC control will be active after Task 5."); }
void PlcService::laserStop() { showInfo("Laser stop PLC control will be active after Task 5."); }
void PlcService::setHysteresisRatio() { showInfo("Hysteresis ratio PLC control will be active after Task 4."); }

bool PlcService::ensureConnected() {
    if (client_.isConnected()) {
        return true;
    }
    return runCommand("PLC connect", client_.connect());
}

bool PlcService::runCommand(const QString& action, const PlcResult& result) {
    if (result.ok) {
        return true;
    }
    emit messageRequested("PLC", QString("%1 failed: %2").arg(action, result.message));
    return false;
}

bool PlcService::writeDbBitConfirmed(const QString& action, int byteOffset, int bitIndex, bool value) {
    for (int attempt = 0; attempt < kConfirmRetries; ++attempt) {
        if (!runCommand(action, client_.writeDbBit(kDb5, byteOffset, bitIndex, value))) {
            return false;
        }

        bool confirmed = false;
        if (!runCommand(action, client_.readDbBit(kDb5, byteOffset, bitIndex, confirmed))) {
            return false;
        }
        if (confirmed == value) {
            return true;
        }
    }

    emit messageRequested("PLC", QString("%1 was written but did not confirm.").arg(action));
    return false;
}

double PlcService::readDoubleField(const QLineEdit* lineEdit, double fallback) const {
    if (lineEdit == nullptr) {
        return fallback;
    }

    bool ok = false;
    const double value = lineEdit->text().toDouble(&ok);
    return ok ? value : fallback;
}

int PlcService::readIntField(const QLineEdit* lineEdit, int fallback) const {
    if (lineEdit == nullptr) {
        return fallback;
    }

    bool ok = false;
    const int value = lineEdit->text().toInt(&ok);
    return ok ? value : fallback;
}

void PlcService::showInfo(const QString& message) {
    emit messageRequested("PLC", message);
}
```

- [ ] **Step 3: Remove the old PLC class from generic services**

In `src/services/PlaceholderServices.h`, delete the `PLCService` class declaration. Keep all other service declarations unchanged.

In `src/services/PlaceholderServices.cpp`, delete the `PLCService::PLCService` constructor and every `PLCService::...` method definition. Keep all other service definitions unchanged.

- [ ] **Step 4: Wire `MainWindow` to the new service type**

Modify `src/ui/MainWindow.h`:

```cpp
#include "services/PlaceholderServices.h"
#include "services/plc/PlcService.h"
```

Keep the member name unchanged:

```cpp
    PlcService plcService_;
```

Modify the constructor initializer in `src/ui/MainWindow.cpp`:

```cpp
      plcService_(config_.plc, ui_, this),
```

Modify `connectSignals()` so the PLC service uses the existing message box handler:

```cpp
    connect(&plcService_, &PlcService::messageRequested,
            this, &MainWindow::showPlaceholderMessage);
```

Leave the existing PLC button `connect(...)` calls in place because the slot names are preserved.

- [ ] **Step 5: Add the new service files to CMake**

Add to `APP_SOURCES`:

```cmake
    src/services/plc/PlcService.h
    src/services/plc/PlcService.cpp
```

- [ ] **Step 6: Build the app skeleton**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64;C:\vcpkg\installed\x64-windows"
cmake --build build --config Release --target AFP_ABB_CPP
```

Expected: build succeeds and `build/Release/AFP_ABB_CPP.exe` exists.

- [ ] **Step 7: Commit Task 3**

Run:

```powershell
git add CMakeLists.txt src/services/PlaceholderServices.h src/services/PlaceholderServices.cpp src/services/plc/PlcService.h src/services/plc/PlcService.cpp src/ui/MainWindow.h src/ui/MainWindow.cpp
git commit -m "feat: add plc qt service skeleton"
```

---

### Task 4: Implement Motor, Cylinder, Parameter, and Hysteresis Controls

**Files:**
- Modify: `src/services/plc/PlcService.cpp`

- [ ] **Step 1: Replace cylinder methods**

Replace the three cylinder methods with:

```cpp
void PlcService::cylinder1() {
    if (!ensureConnected()) {
        return;
    }
    writeDbBitConfirmed("Cylinder 1", 14, 5, true);
}

void PlcService::cylinder2() {
    if (!ensureConnected()) {
        return;
    }

    const bool nextState = !cylinder2On_;
    if (writeDbBitConfirmed("Cylinder 2", 14, 6, nextState)) {
        cylinder2On_ = nextState;
        if (ui_ != nullptr) {
            ui_->cylinder2_state->setChecked(cylinder2On_);
        }
    }
}

void PlcService::cylinder3() {
    if (!ensureConnected()) {
        return;
    }

    const bool nextState = !cylinder3On_;
    if (writeDbBitConfirmed("Cylinder 3", 14, 7, nextState)) {
        cylinder3On_ = nextState;
        if (ui_ != nullptr) {
            ui_->cylinder3_state->setChecked(!cylinder3On_);
        }
    }
}
```

- [ ] **Step 2: Replace motor methods**

Replace `motorEnable`, `motorReset`, and `motorActuation` with:

```cpp
void PlcService::motorEnable() {
    if (!ensureConnected()) {
        return;
    }

    const bool nextState = !motorEnabled_;
    if (writeDbBitConfirmed("Motor enable", 15, 1, nextState)) {
        motorEnabled_ = nextState;
        if (ui_ != nullptr) {
            ui_->state_show->setText(motorEnabled_ ? "Motor enabled" : "Motor disabled");
        }
    }
}

void PlcService::motorReset() {
    if (!motorEnabled_) {
        showInfo("Motor is not enabled.");
        return;
    }
    if (!ensureConnected()) {
        return;
    }

    runCommand("Motor reset", client_.writeMBit(12, 7, true));
}

void PlcService::motorActuation() {
    if (!ensureConnected()) {
        return;
    }

    const bool jogMode = ui_ != nullptr && ui_->interruption->isChecked();
    const bool continuousMode = ui_ != nullptr && ui_->continuous->isChecked();

    if (jogMode) {
        if (!runCommand("Motor jog mode", client_.writeDbBit(kDb5, 15, 0, true))) {
            return;
        }
        runCommand("Motor actuation", client_.writeDbBit(kDb5, 40, 0, true));
        return;
    }

    if (continuousMode) {
        if (!runCommand("Motor continuous mode bit 0", client_.writeDbBit(kDb5, 15, 0, false))) {
            return;
        }
        if (!runCommand("Motor continuous mode bit 1", client_.writeDbBit(kDb5, 15, 1, true))) {
            return;
        }
        runCommand("Motor actuation", client_.writeDbBit(kDb5, 40, 0, true));
        return;
    }

    showInfo("Select jog or continuous motor mode first.");
}
```

- [ ] **Step 3: Update motor mode checkbox handlers to write PLC mode values**

Modify `MainWindow::updateMotorStateFromJog()` in `src/ui/MainWindow.cpp`:

```cpp
void MainWindow::updateMotorStateFromJog() {
    if (ui_->interruption->isChecked()) {
        ui_->continuous->setChecked(false);
        ui_->state_show->setText(QString::fromUtf8(u8"点动状态"));
        plcService_.setMotorJogMode();
    }
}
```

Modify `MainWindow::updateMotorStateFromContinuous()`:

```cpp
void MainWindow::updateMotorStateFromContinuous() {
    if (ui_->continuous->isChecked()) {
        ui_->interruption->setChecked(false);
        ui_->state_show->setText(QString::fromUtf8(u8"连续状态"));
        plcService_.setMotorContinuousMode();
    }
}
```

Add these slots to `src/services/plc/PlcService.h` under `public slots`:

```cpp
    void setMotorJogMode();
    void setMotorContinuousMode();
```

Add these methods to `src/services/plc/PlcService.cpp`:

```cpp
void PlcService::setMotorJogMode() {
    if (!ensureConnected()) {
        return;
    }
    runCommand("Motor jog mode", client_.writeDbInt16(kDb5, 38, 1));
}

void PlcService::setMotorContinuousMode() {
    if (!ensureConnected()) {
        return;
    }
    runCommand("Motor continuous mode", client_.writeDbInt16(kDb5, 38, 3));
}
```

- [ ] **Step 4: Replace parameter and hysteresis methods**

Replace `parameterInput` and `setHysteresisRatio` with:

```cpp
void PlcService::parameterInput() {
    if (!motorEnabled_) {
        showInfo("Motor is not enabled.");
        return;
    }
    if (!ensureConnected()) {
        return;
    }

    const double wireFeedRate = readDoubleField(ui_ != nullptr ? ui_->Wire_feed_rate : nullptr, 10.0);
    const double pavingDistance = readDoubleField(ui_ != nullptr ? ui_->Paving_distance : nullptr, 1.0);

    if (!runCommand("Paving distance", client_.writeDbReal32(kDb5, 16, static_cast<float>(1000.0 * pavingDistance)))) {
        return;
    }
    runCommand("Wire feed rate", client_.writeDbReal32(kDb5, 20, static_cast<float>(60.0 * wireFeedRate)));
}

void PlcService::setHysteresisRatio() {
    if (!ensureConnected()) {
        return;
    }

    const int hysteresisRatio = readIntField(ui_ != nullptr ? ui_->Hysteresis_ratio_text : nullptr, 200);
    if (!runCommand("Hysteresis ratio", client_.writeDbReal32(kDb5, 34, static_cast<float>(hysteresisRatio)))) {
        return;
    }
    runCommand("Hysteresis apply flag", client_.writeDbBit(kDb5, 40, 1, true));
}
```

- [ ] **Step 5: Build and run existing tests**

Run:

```powershell
cmake --build build --config Release --target AFP_ABB_CPP
ctest --test-dir build -C Release --output-on-failure
```

Expected: app builds and all configured automated tests pass.

- [ ] **Step 6: Commit Task 4**

Run:

```powershell
git add src/services/plc/PlcService.h src/services/plc/PlcService.cpp src/ui/MainWindow.cpp
git commit -m "feat: implement plc motor and cylinder controls"
```

---

### Task 5: Implement Laser, Temperature, and Power Controls

**Files:**
- Modify: `src/services/plc/PlcService.cpp`

- [ ] **Step 1: Replace laser connection and red light methods**

Replace `laserConnect`, `openRedLight`, and `closeRedLight` with:

```cpp
void PlcService::laserConnect() {
    if (!ensureConnected()) {
        return;
    }

    if (writeDbBitConfirmed("Laser connect", 14, 4, true)) {
        laserState_ = true;
        if (ui_ != nullptr) {
            ui_->laser_state_display->append("Laser connected.");
        }
    }
}

void PlcService::openRedLight() {
    if (!laserState_) {
        showInfo("Laser is not connected.");
        return;
    }
    if (!ensureConnected()) {
        return;
    }

    if (!writeDbBitConfirmed("Red light open step 1", 14, 3, true)) {
        return;
    }
    if (writeDbBitConfirmed("Red light open step 2", 14, 0, true)) {
        redLight_ = true;
        if (ui_ != nullptr) {
            ui_->laser_state_display->append("Red light open.");
        }
    }
}

void PlcService::closeRedLight() {
    if (!ensureConnected()) {
        return;
    }

    if (writeDbBitConfirmed("Red light close", 14, 1, true)) {
        redLight_ = false;
        if (ui_ != nullptr) {
            ui_->laser_state_display->append("Red light closed.");
        }
    }
}
```

- [ ] **Step 2: Replace laser start and stop methods**

Replace `laserStart` and `laserStop` with:

```cpp
void PlcService::laserStart() {
    if (!targetSet_) {
        showInfo("Set target temperature before starting the laser.");
        return;
    }
    if (!ensureConnected()) {
        return;
    }

    if (!writeDbBitConfirmed("Laser start red light close", 14, 1, true)) {
        return;
    }
    if (writeDbBitConfirmed("Laser start", 14, 2, true)) {
        redLight_ = false;
        if (ui_ != nullptr) {
            ui_->laser_state_display->append("Laser started.");
        }
    }
}

void PlcService::laserStop() {
    if (!ensureConnected()) {
        return;
    }

    if (writeDbBitConfirmed("Laser stop", 14, 3, true)) {
        redLight_ = true;
        if (ui_ != nullptr) {
            ui_->laser_state_display->append("Laser stopped.");
        }
    }
}
```

- [ ] **Step 3: Replace temperature and power methods**

Replace `setTargetTemperature`, `setLaserPower`, `toggleTemperatureControl`, and `togglePowerControl` with:

```cpp
void PlcService::setTargetTemperature() {
    if (!ensureConnected()) {
        return;
    }

    const double targetTemperature = readDoubleField(ui_ != nullptr ? ui_->target_temperature_input : nullptr, 0.0);
    if (runCommand("Target temperature", client_.writeDbReal32(kDb5, 0, static_cast<float>(targetTemperature)))) {
        targetSet_ = true;
    }
}

void PlcService::setLaserPower() {
    if (temperatureControl_) {
        showInfo("Disable temperature control before setting laser power.");
        return;
    }
    if (!ensureConnected()) {
        return;
    }

    const double laserPower = readDoubleField(ui_ != nullptr ? ui_->laser_power_input : nullptr, 0.0);
    if (runCommand("Laser power", client_.writeDbReal32(kDb5, 6, static_cast<float>(laserPower)))) {
        powerSet_ = true;
    }
}

void PlcService::toggleTemperatureControl() {
    if (!targetSet_) {
        showInfo("Set target temperature before enabling temperature control.");
        return;
    }
    if (!ensureConnected()) {
        return;
    }

    const bool nextState = !temperatureControl_;
    if (writeDbBitConfirmed("Temperature control", 4, 0, nextState)) {
        temperatureControl_ = nextState;
        if (ui_ != nullptr) {
            ui_->temperature_control->setText(temperatureControl_ ? "Temperature control on" : "Temperature control off");
        }
    }
}

void PlcService::togglePowerControl() {
    if (!powerSet_) {
        showInfo("Set laser power before enabling power control.");
        return;
    }
    if (!ensureConnected()) {
        return;
    }

    const bool nextState = !powerControl_;
    if (writeDbBitConfirmed("Power control", 4, 1, nextState)) {
        powerControl_ = nextState;
        if (ui_ != nullptr) {
            ui_->set_laser_power_control->setText(powerControl_ ? "Power control on" : "Power control off");
        }
    }
}
```

- [ ] **Step 4: Build and run existing tests**

Run:

```powershell
cmake --build build --config Release --target AFP_ABB_CPP
ctest --test-dir build -C Release --output-on-failure
```

Expected: app builds and all configured automated tests pass.

- [ ] **Step 5: Commit Task 5**

Run:

```powershell
git add src/services/plc/PlcService.cpp
git commit -m "feat: implement plc laser controls"
```

---

### Task 6: Deploy Snap7 Runtime and Update README

**Files:**
- Modify: `cmake/DeployQt.cmake`
- Modify: `README.md`

- [ ] **Step 1: Copy Snap7 runtime during portable deployment**

Modify `cmake/DeployQt.cmake` after the `windeployqt` `execute_process` block:

```cmake
find_file(SNAP7_RUNTIME_DLL
    NAMES snap7.dll
    HINTS
        "C:/vcpkg/installed/x64-windows/bin"
        "$ENV{VCPKG_ROOT}/installed/x64-windows/bin"
)

if(NOT SNAP7_RUNTIME_DLL)
    message(FATAL_ERROR "snap7.dll was not found. Install snap7 with vcpkg or set VCPKG_ROOT.")
endif()

get_filename_component(QT_DEPLOY_OUTPUT_DIR "${QT_DEPLOY_EXECUTABLE}" DIRECTORY)
file(COPY "${SNAP7_RUNTIME_DLL}" DESTINATION "${QT_DEPLOY_OUTPUT_DIR}")
message(STATUS "Copied Snap7 runtime: ${SNAP7_RUNTIME_DLL}")
```

- [ ] **Step 2: Update README build dependency section**

Add this section to `README.md`:

```markdown
## PLC dependency

Slice B uses Snap7 for Siemens S7-1200 communication. On the development PC this project expects the vcpkg package:

```powershell
C:\vcpkg\vcpkg.exe install snap7:x64-windows
```

The CMake configure command must include the vcpkg installed prefix:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64;C:\vcpkg\installed\x64-windows"
```

The portable folder must contain `snap7.dll` beside `AFP_ABB_CPP.exe`. The `deploy_portable` target copies it from `C:\vcpkg\installed\x64-windows\bin`.
```

- [ ] **Step 3: Build, test, and deploy the portable folder**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64;C:\vcpkg\installed\x64-windows"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Release --target deploy_portable
Test-Path build\Release\snap7.dll
```

Expected:

```text
100% tests passed
True
```

- [ ] **Step 4: Confirm Python stayed untouched**

Run:

```powershell
git -C ..\AFP_ABB_Python status --short
```

Expected: no output.

- [ ] **Step 5: Commit Task 6**

Run:

```powershell
git add cmake/DeployQt.cmake README.md
git commit -m "docs: document plc portable deployment"
```

---

### Task 7: Final Verification and Hardware Checklist

**Files:**
- No source files changed in this task.

- [ ] **Step 1: Run final automated verification**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64;C:\vcpkg\installed\x64-windows"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Release --target deploy_portable
Test-Path build\Release\AFP_ABB_CPP.exe
Test-Path build\Release\snap7.dll
git status --short
git -C ..\AFP_ABB_Python status --short
```

Expected:

```text
All build commands complete successfully.
CTest reports 100% tests passed.
The two Test-Path commands print True.
AFP_ABB_CPP shows only generated build output as untracked.
AFP_ABB_Python prints no status output.
```

- [ ] **Step 2: Record manual PLC verification items**

Use this checklist when the development PC is connected to the PLC network:

```text
[ ] Launch build\Release\AFP_ABB_CPP.exe.
[ ] Verify PLC disconnect button does not crash the app.
[ ] Select jog mode and confirm DB5 byte 38 writes INT 1.
[ ] Select continuous mode and confirm DB5 byte 38 writes INT 3.
[ ] Toggle motor enable and confirm DB5 byte 15 bit 1.
[ ] Press motor reset and confirm M12.7 when motor is enabled.
[ ] Press motor actuation in jog mode and confirm DB5 byte 15 bit 0 plus DB5 byte 40 bit 0.
[ ] Press motor actuation in continuous mode and confirm DB5 byte 15 bit 0 false, DB5 byte 15 bit 1 true, and DB5 byte 40 bit 0 true.
[ ] Press cylinder buttons and confirm DB5 byte 14 bits 5, 6, and 7.
[ ] Set paving distance and wire feed rate and confirm DB5 REAL writes at byte 16 and byte 20.
[ ] Set hysteresis ratio and confirm DB5 REAL byte 34 plus DB5 byte 40 bit 1.
[ ] Connect laser and confirm DB5 byte 14 bit 4.
[ ] Open and close red light and confirm DB5 byte 14 bits 3, 0, and 1.
[ ] Set target temperature and confirm DB5 REAL byte 0.
[ ] Toggle temperature control and confirm DB5 byte 4 bit 0.
[ ] Set laser power and confirm DB5 REAL byte 6.
[ ] Toggle power control and confirm DB5 byte 4 bit 1.
[ ] Start and stop laser and confirm DB5 byte 14 bits 1, 2, and 3.
```

- [ ] **Step 3: Commit a verification note only if manual hardware verification is recorded**

If manual PLC verification is completed, append the checklist result to a new file:

```text
docs/verification/2026-05-19-slice-b-plc-manual-verification.md
```

Use this exact content shape:

```markdown
# Slice B PLC Manual Verification

Date: 2026-05-19

PLC IP: 192.168.0.201
Application: build/Release/AFP_ABB_CPP.exe

## Result

Manual PLC checks completed on the development PC connected to the Siemens S7-1200 network.

## Notes

- All approved Slice B direct PLC controls were exercised.
- DB18 print-command signaling was not tested because it is outside Slice B.
```

Commit it:

```powershell
git add docs/verification/2026-05-19-slice-b-plc-manual-verification.md
git commit -m "docs: record slice b plc verification"
```

Skip this commit when manual PLC hardware verification has not been performed.
