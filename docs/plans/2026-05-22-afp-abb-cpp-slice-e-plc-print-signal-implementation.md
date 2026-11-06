# AFP ABB C++ Slice E PLC Print Signal Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the DB18 PLC print command handshake used by the Python workflow, without wiring the print UI buttons yet.

**Architecture:** Extend the existing `PlcClient` with byte-level DB access and a small `IPlcByteAccess` interface. Build a focused `PlcPrintSignalManager` that writes DB18 command values, optionally waits for ACK, clears the command after ACK, and can be tested with a fake PLC byte interface.

**Tech Stack:** C++17, Qt 6 Core/Test, Snap7 C API through existing `PlcClient`, CMake, CTest.

---

## Scope Guardrails

- Do not edit anything under `AFP_ABB_Python`.
- Do not change `src/ui/Interface4600_window.ui`.
- Do not wire `Robot_movebutton_Gcode`, `PauseRobotButton`, `ContinueRobotButton`, or `StopRobotButton` in this slice.
- Do not add ABB robot socket behavior or `LOAD_MOD:<filename>#` sending.
- Do not modify FTP upload behavior.
- Keep comments generous in new C++ code.

## File Structure

- Modify `src/plc/PlcClient.h`: add `IPlcByteAccess` and DB byte read/write methods.
- Modify `src/plc/PlcClient.cpp`: implement DB byte read/write through Snap7.
- Create `src/plc/PlcPrintSignalManager.h`: declare print command enum and DB18 signal manager.
- Create `src/plc/PlcPrintSignalManager.cpp`: implement command write, ACK polling, timeout, and command clear.
- Create `tests/test_plc_print_signal_manager.cpp`: test the manager with a fake PLC byte interface.
- Modify `tests/test_plc_client_state.cpp`: add a compile/runtime check that `PlcClient` is usable through `IPlcByteAccess`.
- Modify `CMakeLists.txt`: add app sources and the new test target.
- Modify `README.md`: document the DB18 print command channel.

## Task 1: Add PLC DB Byte Access

**Files:**
- Modify: `src/plc/PlcClient.h`
- Modify: `src/plc/PlcClient.cpp`
- Modify: `tests/test_plc_client_state.cpp`

- [ ] **Step 1: Update the PLC client header**

In `src/plc/PlcClient.h`, add this interface after `PlcResult`:

```cpp
// Small byte-level PLC interface used by print signaling code.
//
// Tests can provide a fake implementation, while production code can use
// PlcClient directly. This keeps DB18 command handshakes independent from
// Snap7 network setup during automated tests.
class IPlcByteAccess {
public:
    virtual ~IPlcByteAccess() = default;

    virtual PlcResult writeDbByte(int dbNumber, int byteOffset, quint8 value) = 0;
    virtual PlcResult readDbByte(int dbNumber, int byteOffset, quint8& value) = 0;
};
```

Change the class declaration from:

```cpp
class PlcClient final {
```

to:

```cpp
class PlcClient final : public IPlcByteAccess {
```

Add the two methods to the public section near the existing DB methods:

```cpp
PlcResult writeDbByte(int dbNumber, int byteOffset, quint8 value) override;
PlcResult readDbByte(int dbNumber, int byteOffset, quint8& value) override;
```

- [ ] **Step 2: Implement DB byte access**

In `src/plc/PlcClient.cpp`, add these methods near the existing DB read/write methods:

```cpp
PlcResult PlcClient::writeDbByte(int dbNumber, int byteOffset, quint8 value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    std::array<std::uint8_t, 1> data{static_cast<std::uint8_t>(value)};
    int code = Cli_WriteArea(client_, s7AreaDb, dbNumber, byteOffset, 1, s7WordLenByte, data.data());
    if (code != 0) {
        connected_ = false;
        return snap7Failure("write DB byte", code);
    }

    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::readDbByte(int dbNumber, int byteOffset, quint8& value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    std::array<std::uint8_t, 1> data{};
    int code = Cli_ReadArea(client_, s7AreaDb, dbNumber, byteOffset, 1, s7WordLenByte, data.data());
    if (code != 0) {
        connected_ = false;
        return snap7Failure("read DB byte", code);
    }

    value = static_cast<quint8>(data[0]);
    lastError_.clear();
    return PlcResult::success();
}
```

- [ ] **Step 3: Add an interface compile check**

In `tests/test_plc_client_state.cpp`, add a private slot:

```cpp
void plcClientImplementsByteAccessInterface();
```

Add this test implementation:

```cpp
void TestPlcClientState::plcClientImplementsByteAccessInterface() {
    PlcConfig config;
    config.ip = "192.168.0.201";
    config.rack = 0;
    config.slot = 1;

    PlcClient client(config);
    IPlcByteAccess* byteAccess = &client;

    QVERIFY(byteAccess != nullptr);
    QVERIFY(!client.isConnected());
}
```

This test must not call `readDbByte` or `writeDbByte`, because that would contact real hardware.

- [ ] **Step 4: Build and run the PLC client state test**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_plc_client_state
ctest --test-dir build -C Release -R test_plc_client_state --output-on-failure
```

Expected:

```text
100% tests passed
```

- [ ] **Step 5: Commit Task 1**

```powershell
git add src\plc\PlcClient.h src\plc\PlcClient.cpp tests\test_plc_client_state.cpp
git commit -m "feat: add plc db byte access"
```

## Task 2: Add the DB18 Print Signal Manager

**Files:**
- Create: `src/plc/PlcPrintSignalManager.h`
- Create: `src/plc/PlcPrintSignalManager.cpp`
- Create: `tests/test_plc_print_signal_manager.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create the manager header**

Create `src/plc/PlcPrintSignalManager.h`:

```cpp
#pragma once

#include <QtGlobal>
#include <QString>

#include "plc/PlcClient.h"

// DB18 command values used by the Python PLCSignalManager.
enum class PlcPrintCommand : quint8 {
    None = 0,
    Start = 10,
    Pause = 11,
    Resume = 12,
    Stop = 13,
};

// Focused manager for the host-to-PLC print command channel.
//
// It writes command values to DB18.DBB0, optionally waits for DB18.DBB1 ACK,
// and clears DB18.DBB0 after a successful ACK. UI logic and robot socket
// messages are intentionally outside this class.
class PlcPrintSignalManager final {
public:
    explicit PlcPrintSignalManager(IPlcByteAccess& plc);

    PlcResult startPrint(bool waitAck = true);
    PlcResult pausePrint(bool waitAck = true);
    PlcResult resumePrint(bool waitAck = true);
    PlcResult stopPrint(bool waitAck = true);
    PlcResult sendCommand(PlcPrintCommand command, bool waitAck = true);

    int dbNumber() const;
    int commandOffset() const;
    int ackOffset() const;
    int stateOffset() const;
    int errorOffset() const;

private:
    PlcResult waitForAck();
    PlcResult clearCommand();
    QString commandName(PlcPrintCommand command) const;

    IPlcByteAccess& plc_;
    int dbNumber_ = 18;
    int commandOffset_ = 0;
    int ackOffset_ = 1;
    int stateOffset_ = 2;
    int errorOffset_ = 3;
    int timeoutMs_ = 3000;
    int pollIntervalMs_ = 50;
};
```

- [ ] **Step 2: Create a minimal failing stub implementation**

Create `src/plc/PlcPrintSignalManager.cpp` with this temporary implementation. It should compile but fail the behavior tests.

```cpp
#include "plc/PlcPrintSignalManager.h"

PlcPrintSignalManager::PlcPrintSignalManager(IPlcByteAccess& plc)
    : plc_(plc) {
}

PlcResult PlcPrintSignalManager::startPrint(bool waitAck) {
    return sendCommand(PlcPrintCommand::Start, waitAck);
}

PlcResult PlcPrintSignalManager::pausePrint(bool waitAck) {
    return sendCommand(PlcPrintCommand::Pause, waitAck);
}

PlcResult PlcPrintSignalManager::resumePrint(bool waitAck) {
    return sendCommand(PlcPrintCommand::Resume, waitAck);
}

PlcResult PlcPrintSignalManager::stopPrint(bool waitAck) {
    return sendCommand(PlcPrintCommand::Stop, waitAck);
}

PlcResult PlcPrintSignalManager::sendCommand(PlcPrintCommand command, bool waitAck) {
    Q_UNUSED(command);
    Q_UNUSED(waitAck);
    return PlcResult::failure("PLC print signal manager is not implemented.");
}

int PlcPrintSignalManager::dbNumber() const { return dbNumber_; }
int PlcPrintSignalManager::commandOffset() const { return commandOffset_; }
int PlcPrintSignalManager::ackOffset() const { return ackOffset_; }
int PlcPrintSignalManager::stateOffset() const { return stateOffset_; }
int PlcPrintSignalManager::errorOffset() const { return errorOffset_; }

PlcResult PlcPrintSignalManager::waitForAck() {
    return PlcResult::failure("PLC print ACK wait is not implemented.");
}

PlcResult PlcPrintSignalManager::clearCommand() {
    return PlcResult::failure("PLC print command clear is not implemented.");
}

QString PlcPrintSignalManager::commandName(PlcPrintCommand command) const {
    Q_UNUSED(command);
    return "UNKNOWN";
}
```

- [ ] **Step 3: Create manager tests**

Create `tests/test_plc_print_signal_manager.cpp`:

```cpp
#include <QtTest/QtTest>

#include "plc/PlcPrintSignalManager.h"

#include <QHash>
#include <QStringList>

class FakePlcByteAccess final : public IPlcByteAccess {
public:
    QHash<QString, quint8> bytes;
    QStringList operations;
    bool failNextWrite = false;
    bool failNextRead = false;
    int ackReadsBeforeSuccess = 0;

    PlcResult writeDbByte(int dbNumber, int byteOffset, quint8 value) override {
        operations << QString("write:%1:%2:%3").arg(dbNumber).arg(byteOffset).arg(value);
        if (failNextWrite) {
            failNextWrite = false;
            return PlcResult::failure("fake write failure");
        }
        bytes[key(dbNumber, byteOffset)] = value;
        return PlcResult::success();
    }

    PlcResult readDbByte(int dbNumber, int byteOffset, quint8& value) override {
        operations << QString("read:%1:%2").arg(dbNumber).arg(byteOffset);
        if (failNextRead) {
            failNextRead = false;
            return PlcResult::failure("fake read failure");
        }
        if (byteOffset == 1 && ackReadsBeforeSuccess > 0) {
            --ackReadsBeforeSuccess;
            value = 0;
            return PlcResult::success();
        }
        value = bytes.value(key(dbNumber, byteOffset), 0);
        return PlcResult::success();
    }

    static QString key(int dbNumber, int byteOffset) {
        return QString("%1:%2").arg(dbNumber).arg(byteOffset);
    }
};

class TestPlcPrintSignalManager final : public QObject {
    Q_OBJECT

private slots:
    void startWritesCommandAndClearsAfterAck();
    void pauseResumeStopUsePythonCommandValues();
    void waitAckFalseDoesNotReadAckOrClearCommand();
    void ackTimeoutReturnsFailure();
    void writeFailureReturnsFailure();
    void readFailureReturnsFailure();
};

void TestPlcPrintSignalManager::startWritesCommandAndClearsAfterAck() {
    FakePlcByteAccess plc;
    plc.bytes[FakePlcByteAccess::key(18, 1)] = 1;
    PlcPrintSignalManager manager(plc);

    const PlcResult result = manager.startPrint(true);

    QVERIFY2(result.ok, qPrintable(result.message));
    QCOMPARE(plc.bytes.value(FakePlcByteAccess::key(18, 0)), static_cast<quint8>(0));
    QVERIFY(plc.operations.contains("write:18:0:10"));
    QVERIFY(plc.operations.contains("read:18:1"));
    QVERIFY(plc.operations.contains("write:18:0:0"));
    QVERIFY(plc.operations.indexOf("write:18:0:10") < plc.operations.indexOf("read:18:1"));
    QVERIFY(plc.operations.indexOf("read:18:1") < plc.operations.indexOf("write:18:0:0"));
}

void TestPlcPrintSignalManager::pauseResumeStopUsePythonCommandValues() {
    FakePlcByteAccess plc;
    PlcPrintSignalManager manager(plc);

    QVERIFY(manager.pausePrint(false).ok);
    QCOMPARE(plc.bytes.value(FakePlcByteAccess::key(18, 0)), static_cast<quint8>(11));

    QVERIFY(manager.resumePrint(false).ok);
    QCOMPARE(plc.bytes.value(FakePlcByteAccess::key(18, 0)), static_cast<quint8>(12));

    QVERIFY(manager.stopPrint(false).ok);
    QCOMPARE(plc.bytes.value(FakePlcByteAccess::key(18, 0)), static_cast<quint8>(13));
}

void TestPlcPrintSignalManager::waitAckFalseDoesNotReadAckOrClearCommand() {
    FakePlcByteAccess plc;
    PlcPrintSignalManager manager(plc);

    const PlcResult result = manager.startPrint(false);

    QVERIFY(result.ok);
    QCOMPARE(plc.bytes.value(FakePlcByteAccess::key(18, 0)), static_cast<quint8>(10));
    QVERIFY(!plc.operations.contains("read:18:1"));
    QVERIFY(!plc.operations.contains("write:18:0:0"));
}

void TestPlcPrintSignalManager::ackTimeoutReturnsFailure() {
    FakePlcByteAccess plc;
    PlcPrintSignalManager manager(plc);

    const PlcResult result = manager.startPrint(true);

    QVERIFY(!result.ok);
    QVERIFY(result.message.contains("ACK", Qt::CaseInsensitive));
    QCOMPARE(plc.bytes.value(FakePlcByteAccess::key(18, 0)), static_cast<quint8>(10));
}

void TestPlcPrintSignalManager::writeFailureReturnsFailure() {
    FakePlcByteAccess plc;
    plc.failNextWrite = true;
    PlcPrintSignalManager manager(plc);

    const PlcResult result = manager.startPrint(true);

    QVERIFY(!result.ok);
    QVERIFY(result.message.contains("write", Qt::CaseInsensitive));
}

void TestPlcPrintSignalManager::readFailureReturnsFailure() {
    FakePlcByteAccess plc;
    plc.failNextRead = true;
    PlcPrintSignalManager manager(plc);

    const PlcResult result = manager.startPrint(true);

    QVERIFY(!result.ok);
    QVERIFY(result.message.contains("read", Qt::CaseInsensitive));
}

QTEST_MAIN(TestPlcPrintSignalManager)
#include "test_plc_print_signal_manager.moc"
```

- [ ] **Step 4: Add sources and test target to CMake**

Add these files to `APP_SOURCES` near the other PLC files:

```cmake
    src/plc/PlcPrintSignalManager.h
    src/plc/PlcPrintSignalManager.cpp
```

Add this test target block after `test_plc_client_state` or near PLC tests:

```cmake
set(TEST_PLC_PRINT_SIGNAL_MANAGER_SOURCES
    tests/test_plc_print_signal_manager.cpp
    src/plc/PlcPrintSignalManager.cpp
    src/plc/PlcClient.cpp
    src/plc/S7Data.cpp
)
set(TEST_PLC_PRINT_SIGNAL_MANAGER_READY TRUE)
set(MISSING_TEST_PLC_PRINT_SIGNAL_MANAGER_SOURCES)
foreach(TEST_SOURCE ${TEST_PLC_PRINT_SIGNAL_MANAGER_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        set(TEST_PLC_PRINT_SIGNAL_MANAGER_READY FALSE)
        list(APPEND MISSING_TEST_PLC_PRINT_SIGNAL_MANAGER_SOURCES ${TEST_SOURCE})
    endif()
endforeach()

if(TEST_PLC_PRINT_SIGNAL_MANAGER_READY)
    add_executable(test_plc_print_signal_manager ${TEST_PLC_PRINT_SIGNAL_MANAGER_SOURCES})
    target_include_directories(test_plc_print_signal_manager PRIVATE src)
    target_link_libraries(test_plc_print_signal_manager PRIVATE Qt6::Core Qt6::Test snap7::snap7)
    copy_qt_test_runtime(test_plc_print_signal_manager)
    add_custom_command(TARGET test_plc_print_signal_manager POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:snap7::snap7>
                $<TARGET_FILE_DIR:test_plc_print_signal_manager>
    )
    add_test(NAME test_plc_print_signal_manager COMMAND test_plc_print_signal_manager)
elseif(MISSING_TEST_PLC_PRINT_SIGNAL_MANAGER_SOURCES)
    string(REPLACE ";" "\n  " MISSING_TEST_PLC_PRINT_SIGNAL_MANAGER_TEXT "${MISSING_TEST_PLC_PRINT_SIGNAL_MANAGER_SOURCES}")
    message(STATUS "test_plc_print_signal_manager target deferred. Missing files:\n  ${MISSING_TEST_PLC_PRINT_SIGNAL_MANAGER_TEXT}")
endif()
```

- [ ] **Step 5: Run the failing test before full implementation**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_plc_print_signal_manager
ctest --test-dir build -C Release -R test_plc_print_signal_manager --output-on-failure
```

Expected before full implementation: the target builds, but the test fails because the stub returns `"PLC print signal manager is not implemented."`.

- [ ] **Step 6: Replace the stub with the full manager implementation**

Replace `src/plc/PlcPrintSignalManager.cpp` with:

```cpp
#include "plc/PlcPrintSignalManager.h"

#include <QElapsedTimer>
#include <QThread>

PlcPrintSignalManager::PlcPrintSignalManager(IPlcByteAccess& plc)
    : plc_(plc) {
}

PlcResult PlcPrintSignalManager::startPrint(bool waitAck) {
    return sendCommand(PlcPrintCommand::Start, waitAck);
}

PlcResult PlcPrintSignalManager::pausePrint(bool waitAck) {
    return sendCommand(PlcPrintCommand::Pause, waitAck);
}

PlcResult PlcPrintSignalManager::resumePrint(bool waitAck) {
    return sendCommand(PlcPrintCommand::Resume, waitAck);
}

PlcResult PlcPrintSignalManager::stopPrint(bool waitAck) {
    return sendCommand(PlcPrintCommand::Stop, waitAck);
}

PlcResult PlcPrintSignalManager::sendCommand(PlcPrintCommand command, bool waitAck) {
    if (command == PlcPrintCommand::None) {
        return PlcResult::failure("PLC print command is invalid.");
    }

    const auto value = static_cast<quint8>(command);
    PlcResult writeResult = plc_.writeDbByte(dbNumber_, commandOffset_, value);
    if (!writeResult.ok) {
        return PlcResult::failure(QString("PLC print command write failed (%1): %2")
                                  .arg(commandName(command), writeResult.message));
    }

    if (!waitAck) {
        return PlcResult::success();
    }

    PlcResult ackResult = waitForAck();
    if (!ackResult.ok) {
        return ackResult;
    }

    return clearCommand();
}

int PlcPrintSignalManager::dbNumber() const { return dbNumber_; }
int PlcPrintSignalManager::commandOffset() const { return commandOffset_; }
int PlcPrintSignalManager::ackOffset() const { return ackOffset_; }
int PlcPrintSignalManager::stateOffset() const { return stateOffset_; }
int PlcPrintSignalManager::errorOffset() const { return errorOffset_; }

PlcResult PlcPrintSignalManager::waitForAck() {
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < timeoutMs_) {
        quint8 ack = 0;
        PlcResult readResult = plc_.readDbByte(dbNumber_, ackOffset_, ack);
        if (!readResult.ok) {
            return PlcResult::failure(QString("PLC print ACK read failed: %1").arg(readResult.message));
        }

        if (ack == 1) {
            return PlcResult::success();
        }

        QThread::msleep(static_cast<unsigned long>(pollIntervalMs_));
    }

    return PlcResult::failure(QString("PLC print ACK timeout after %1 ms.").arg(timeoutMs_));
}

PlcResult PlcPrintSignalManager::clearCommand() {
    PlcResult clearResult = plc_.writeDbByte(dbNumber_, commandOffset_, 0);
    if (!clearResult.ok) {
        return PlcResult::failure(QString("PLC print command clear failed: %1").arg(clearResult.message));
    }
    return PlcResult::success();
}

QString PlcPrintSignalManager::commandName(PlcPrintCommand command) const {
    switch (command) {
    case PlcPrintCommand::Start:
        return "START";
    case PlcPrintCommand::Pause:
        return "PAUSE";
    case PlcPrintCommand::Resume:
        return "RESUME";
    case PlcPrintCommand::Stop:
        return "STOP";
    case PlcPrintCommand::None:
        return "NONE";
    }
    return "UNKNOWN";
}
```

- [ ] **Step 7: Run manager tests and full tests**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_plc_print_signal_manager
ctest --test-dir build -C Release -R test_plc_print_signal_manager --output-on-failure
ctest --test-dir build -C Release --output-on-failure
```

Expected:

```text
100% tests passed
```

- [ ] **Step 8: Commit Task 2**

```powershell
git add CMakeLists.txt src\plc\PlcPrintSignalManager.h src\plc\PlcPrintSignalManager.cpp tests\test_plc_print_signal_manager.cpp
git commit -m "feat: add plc print signal manager"
```

## Task 3: Document and Verify Slice E

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Add README notes**

Add this section near the PLC dependency or runtime configuration sections:

````markdown
## PLC DB18 Print Command Channel

Slice E adds the DB18 print command handshake used by the original Python workflow. The C++ code does not wire the print buttons yet, but the tested signal manager is ready for the next robot-print slice.

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

The UI print buttons will be connected in a later slice together with FTP upload state and robot socket filename sending.
````

Also update the top summary to mention Slice E.

- [ ] **Step 2: Configure, build, and run all tests**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Expected:

```text
100% tests passed
```

- [ ] **Step 3: Verify Python is untouched**

Run:

```powershell
git -C ..\AFP_ABB_Python status --short
```

Expected: no output.

- [ ] **Step 4: Commit README**

```powershell
git add README.md
git commit -m "docs: document plc print signal channel"
```

## Self-Review Checklist

- [ ] DB18 command values match Python: 10, 11, 12, 13.
- [ ] Command byte is DB18.DBB0 and ACK byte is DB18.DBB1.
- [ ] ACK timeout is 3000 ms.
- [ ] Poll interval is 50 ms.
- [ ] Command clear happens only after ACK when `waitAck=true`.
- [ ] `waitAck=false` does not poll ACK and does not clear the command.
- [ ] No UI button wiring is included.
- [ ] No ABB robot socket behavior is included.
- [ ] Tests do not require a live PLC.
- [ ] `AFP_ABB_Python` is untouched.
