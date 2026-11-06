# AFP ABB C++ Slice F Robot Print Workflow Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Wire the original print start/pause/continue/stop workflow through FTP upload state, ABB robot socket messages, and the DB18 PLC print command manager.

**Architecture:** Add small test interfaces for MOD upload state, robot command socket sending, and PLC print commands. Implement `RobotSocketClient` with Qt `QTcpSocket`, then build a Qt-facing `RobotPrintService` that preserves the Python print-control order while leaving non-print robot motion controls as placeholders.

**Tech Stack:** C++17, Qt 6 Core/Network/Widgets/Test, Snap7 through existing `PlcClient`, CMake, CTest.

---

## Scope Guardrails

- Do not edit anything under `AFP_ABB_Python`.
- Do not change `src/ui/Interface4600_window.ui`.
- Do not convert robot move, robot reset, robot jog, axis move, or external-axis commands.
- Do not convert RAPID files.
- Do not use `pythonLIb` or any Python runtime path.
- Do not send PLC START unless `LOAD_MOD:<filename>#` was sent successfully.
- Do not reintroduce command-value validation that the user rejected for `PlcPrintSignalManager`.
- Keep comments generous in new C++ files.

## File Structure

- Create `src/services/robot/RobotPrintInterfaces.h`: shared test interfaces for upload state, socket command sending, and PLC print commands.
- Modify `src/services/ftp/FtpService.h`: make `FtpService` implement `IPrintUploadState`.
- Modify `src/plc/PlcPrintSignalManager.h`: make `PlcPrintSignalManager` implement `IPlcPrintController`.
- Create `src/robot/RobotSocketClient.h`: declare lightweight ABB robot socket wrapper and `RobotSocketResult`.
- Create `src/robot/RobotSocketClient.cpp`: implement synchronous connect/send/disconnect using `QTcpSocket`.
- Create `tests/test_robot_socket_client.cpp`: verify message formatting and no-hardware failure paths.
- Create `src/services/robot/RobotPrintService.h`: declare the Qt-facing print workflow service.
- Create `src/services/robot/RobotPrintService.cpp`: implement connection, start, pause, continue, and stop workflow.
- Create `tests/test_robot_print_service.cpp`: verify upload checks, command ordering, PLC failure handling, and non-fatal side-message failures.
- Modify `src/ui/MainWindow.h`: add `RobotPrintService` member.
- Modify `src/ui/MainWindow.cpp`: wire print buttons to `RobotPrintService` and keep non-print robot buttons on placeholders.
- Modify `CMakeLists.txt`: add Qt Network, new app sources, and new tests.
- Modify `README.md`: document robot print workflow configuration and current limits.

## Task 1: Add Print Workflow Interfaces

**Files:**
- Create: `src/services/robot/RobotPrintInterfaces.h`
- Modify: `src/services/ftp/FtpService.h`
- Modify: `src/plc/PlcPrintSignalManager.h`

- [ ] **Step 1: Create the interfaces header**

Create `src/services/robot/RobotPrintInterfaces.h`:

```cpp
#pragma once

#include <QString>

#include "plc/PlcClient.h"

// Read-only view of the MOD upload state needed before print can start.
//
// FtpService implements this in production. Tests use a tiny fake so the print
// workflow can be verified without opening file dialogs or contacting FTP.
class IPrintUploadState {
public:
    virtual ~IPrintUploadState() = default;

    virtual bool uploadSucceeded() const = 0;
    virtual QString uploadedModFilename() const = 0;
};

// Result returned by robot socket operations. It mirrors PlcResult so service
// code can treat socket and PLC failures in the same simple way.
struct RobotSocketResult {
    bool ok = false;
    QString message;

    static RobotSocketResult success(const QString& message = QString());
    static RobotSocketResult failure(const QString& message);
};

// Narrow command-socket interface used by RobotPrintService.
//
// The real implementation is RobotSocketClient. Tests use a fake that records
// command order, which is important for proving LOAD_MOD is sent before PLC START.
class IRobotCommandSocket {
public:
    virtual ~IRobotCommandSocket() = default;

    virtual RobotSocketResult connectToRobot() = 0;
    virtual void disconnectFromRobot() = 0;
    virtual bool isConnected() const = 0;
    virtual RobotSocketResult sendLoadMod(const QString& modFilename) = 0;
    virtual RobotSocketResult sendPause() = 0;
    virtual RobotSocketResult sendContinue() = 0;
    virtual RobotSocketResult sendStop() = 0;
};

// Interface for the DB18 print commands needed by the robot print workflow.
//
// PlcPrintSignalManager implements this in production. Tests use a fake so the
// workflow can be verified without PLC hardware.
class IPlcPrintController {
public:
    virtual ~IPlcPrintController() = default;

    virtual PlcResult startPrint(bool waitAck = true) = 0;
    virtual PlcResult pausePrint(bool waitAck = true) = 0;
    virtual PlcResult resumePrint(bool waitAck = true) = 0;
    virtual PlcResult stopPrint(bool waitAck = true) = 0;
};
```

- [ ] **Step 2: Implement `RobotSocketResult` in the socket client task**

Do not create a separate `.cpp` file for the interfaces yet. Task 2 will implement `RobotSocketResult::success` and `RobotSocketResult::failure` in `src/robot/RobotSocketClient.cpp` so the result type is linked wherever the socket client is linked.

- [ ] **Step 3: Make `FtpService` implement upload state**

In `src/services/ftp/FtpService.h`, add:

```cpp
#include "services/robot/RobotPrintInterfaces.h"
```

Change:

```cpp
class FtpService final : public QObject {
```

to:

```cpp
class FtpService final : public QObject, public IPrintUploadState {
```

Change these declarations:

```cpp
QString uploadedModFilename() const;
bool uploadSucceeded() const;
```

to:

```cpp
QString uploadedModFilename() const override;
bool uploadSucceeded() const override;
```

No `.cpp` change is needed because the existing methods already have the correct signatures.

- [ ] **Step 4: Make `PlcPrintSignalManager` implement print command interface**

In `src/plc/PlcPrintSignalManager.h`, add:

```cpp
#include "services/robot/RobotPrintInterfaces.h"
```

Change:

```cpp
class PlcPrintSignalManager final {
```

to:

```cpp
class PlcPrintSignalManager final : public IPlcPrintController {
```

Change the public command declarations to:

```cpp
PlcResult startPrint(bool waitAck = true) override;
PlcResult pausePrint(bool waitAck = true) override;
PlcResult resumePrint(bool waitAck = true) override;
PlcResult stopPrint(bool waitAck = true) override;
```

- [ ] **Step 5: Configure and build the existing tests**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_ftp_service
cmake --build build --config Release --target test_plc_print_signal_manager
ctest --test-dir build -C Release -R "test_ftp_service|test_plc_print_signal_manager" --output-on-failure
```

Expected:

```text
100% tests passed
```

- [ ] **Step 6: Commit Task 1**

```powershell
git add src\services\robot\RobotPrintInterfaces.h src\services\ftp\FtpService.h src\plc\PlcPrintSignalManager.h
git commit -m "feat: add robot print workflow interfaces"
```

## Task 2: Add `RobotSocketClient`

**Files:**
- Create: `src/robot/RobotSocketClient.h`
- Create: `src/robot/RobotSocketClient.cpp`
- Create: `tests/test_robot_socket_client.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create the socket client header**

Create `src/robot/RobotSocketClient.h`:

```cpp
#pragma once

#include <QString>
#include <QTcpSocket>

#include "app/AppConfig.h"
#include "services/robot/RobotPrintInterfaces.h"

// Lightweight ABB robot socket wrapper for the print workflow.
//
// It only sends the exact string commands used by the Python print path:
// LOAD_MOD:<filename>#, PAUSE#, CONTINUE#, and STOP#. It does not implement
// robot motion, pose conversion, or RAPID handling.
class RobotSocketClient final : public IRobotCommandSocket {
public:
    explicit RobotSocketClient(RobotConfig config);

    RobotSocketResult connectToRobot() override;
    void disconnectFromRobot() override;
    bool isConnected() const override;
    RobotSocketResult sendLoadMod(const QString& modFilename) override;
    RobotSocketResult sendPause() override;
    RobotSocketResult sendContinue() override;
    RobotSocketResult sendStop() override;

    static QString loadModMessage(const QString& modFilename);

private:
    RobotSocketResult sendRawMessage(const QString& message);
    QString endpointText() const;

    RobotConfig config_;
    QTcpSocket socket_;
    int timeoutMs_ = 3000;
};
```

- [ ] **Step 2: Create failing socket client tests**

Create `tests/test_robot_socket_client.cpp`:

```cpp
#include <QtTest/QtTest>

#include "robot/RobotSocketClient.h"

class TestRobotSocketClient final : public QObject {
    Q_OBJECT

private slots:
    void loadModMessageMatchesPythonProtocol();
    void connectFailsWhenIpMissing();
    void connectFailsWhenPortInvalid();
};

void TestRobotSocketClient::loadModMessageMatchesPythonProtocol() {
    QCOMPARE(RobotSocketClient::loadModMessage("path_a.mod"), QString("LOAD_MOD:path_a.mod#"));
    QCOMPARE(RobotSocketClient::loadModMessage("folder/path_b.mod"), QString("LOAD_MOD:folder/path_b.mod#"));
}

void TestRobotSocketClient::connectFailsWhenIpMissing() {
    RobotConfig config;
    config.ip.clear();
    config.portMotion = 5010;

    RobotSocketClient client(config);
    const RobotSocketResult result = client.connectToRobot();

    QVERIFY(!result.ok);
    QVERIFY(result.message.contains("IP", Qt::CaseInsensitive));
}

void TestRobotSocketClient::connectFailsWhenPortInvalid() {
    RobotConfig config;
    config.ip = "127.0.0.1";
    config.portMotion = 0;

    RobotSocketClient client(config);
    const RobotSocketResult result = client.connectToRobot();

    QVERIFY(!result.ok);
    QVERIFY(result.message.contains("port", Qt::CaseInsensitive));
}

QTEST_MAIN(TestRobotSocketClient)
#include "test_robot_socket_client.moc"
```

- [ ] **Step 3: Add Qt Network and test target to CMake**

In `CMakeLists.txt`, change:

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)
```

to:

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Widgets Test)
```

Add to `APP_SOURCES` near other low-level communication sources:

```cmake
    src/robot/RobotSocketClient.h
    src/robot/RobotSocketClient.cpp
```

Add `Qt6::Network` to the `AFP_ABB_CPP` target link libraries:

```cmake
        Qt6::Network
```

Add this test block near the other unit tests:

```cmake
set(TEST_ROBOT_SOCKET_CLIENT_SOURCES
    tests/test_robot_socket_client.cpp
    src/robot/RobotSocketClient.cpp
)
set(TEST_ROBOT_SOCKET_CLIENT_READY TRUE)
set(MISSING_TEST_ROBOT_SOCKET_CLIENT_SOURCES)
foreach(TEST_SOURCE ${TEST_ROBOT_SOCKET_CLIENT_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        set(TEST_ROBOT_SOCKET_CLIENT_READY FALSE)
        list(APPEND MISSING_TEST_ROBOT_SOCKET_CLIENT_SOURCES ${TEST_SOURCE})
    endif()
endforeach()

if(TEST_ROBOT_SOCKET_CLIENT_READY)
    add_executable(test_robot_socket_client ${TEST_ROBOT_SOCKET_CLIENT_SOURCES})
    target_include_directories(test_robot_socket_client PRIVATE src)
    target_link_libraries(test_robot_socket_client PRIVATE Qt6::Core Qt6::Network Qt6::Test)
    copy_qt_test_runtime(test_robot_socket_client)
    add_custom_command(TARGET test_robot_socket_client POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt6::Network>
                $<TARGET_FILE_DIR:test_robot_socket_client>
    )
    add_test(NAME test_robot_socket_client COMMAND test_robot_socket_client)
elseif(MISSING_TEST_ROBOT_SOCKET_CLIENT_SOURCES)
    string(REPLACE ";" "\n  " MISSING_TEST_ROBOT_SOCKET_CLIENT_TEXT "${MISSING_TEST_ROBOT_SOCKET_CLIENT_SOURCES}")
    message(STATUS "test_robot_socket_client target deferred. Missing files:\n  ${MISSING_TEST_ROBOT_SOCKET_CLIENT_TEXT}")
endif()
```

- [ ] **Step 4: Run the failing socket tests**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_robot_socket_client
ctest --test-dir build -C Release -R test_robot_socket_client --output-on-failure
```

Expected before implementation: build fails because `RobotSocketClient.cpp` does not exist or link fails because methods are not implemented.

- [ ] **Step 5: Implement the socket client**

Create `src/robot/RobotSocketClient.cpp`:

```cpp
#include "robot/RobotSocketClient.h"

#include <QAbstractSocket>

#include <utility>

RobotSocketResult RobotSocketResult::success(const QString& message) {
    return RobotSocketResult{true, message};
}

RobotSocketResult RobotSocketResult::failure(const QString& message) {
    return RobotSocketResult{false, message};
}

RobotSocketClient::RobotSocketClient(RobotConfig config)
    : config_(std::move(config)) {
}

RobotSocketResult RobotSocketClient::connectToRobot() {
    if (config_.ip.trimmed().isEmpty()) {
        return RobotSocketResult::failure("Robot socket IP is empty.");
    }
    if (config_.portMotion <= 0) {
        return RobotSocketResult::failure("Robot motion socket port is invalid.");
    }

    if (isConnected()) {
        return RobotSocketResult::success(QString("Robot socket already connected to %1.").arg(endpointText()));
    }

    socket_.abort();
    socket_.connectToHost(config_.ip, static_cast<quint16>(config_.portMotion));
    if (!socket_.waitForConnected(timeoutMs_)) {
        return RobotSocketResult::failure(
            QString("Failed to connect robot socket %1: %2").arg(endpointText(), socket_.errorString()));
    }

    return RobotSocketResult::success(QString("Robot socket connected to %1.").arg(endpointText()));
}

void RobotSocketClient::disconnectFromRobot() {
    socket_.disconnectFromHost();
    if (socket_.state() != QAbstractSocket::UnconnectedState) {
        socket_.waitForDisconnected(timeoutMs_);
    }
}

bool RobotSocketClient::isConnected() const {
    return socket_.state() == QAbstractSocket::ConnectedState;
}

RobotSocketResult RobotSocketClient::sendLoadMod(const QString& modFilename) {
    return sendRawMessage(loadModMessage(modFilename));
}

RobotSocketResult RobotSocketClient::sendPause() {
    return sendRawMessage("PAUSE#");
}

RobotSocketResult RobotSocketClient::sendContinue() {
    return sendRawMessage("CONTINUE#");
}

RobotSocketResult RobotSocketClient::sendStop() {
    return sendRawMessage("STOP#");
}

QString RobotSocketClient::loadModMessage(const QString& modFilename) {
    return QString("LOAD_MOD:%1#").arg(modFilename);
}

RobotSocketResult RobotSocketClient::sendRawMessage(const QString& message) {
    if (!isConnected()) {
        return RobotSocketResult::failure("Robot socket is not connected.");
    }

    const QByteArray payload = message.toUtf8();
    const qint64 written = socket_.write(payload);
    if (written != payload.size()) {
        return RobotSocketResult::failure(
            QString("Failed to write robot socket message '%1': %2").arg(message, socket_.errorString()));
    }

    if (!socket_.waitForBytesWritten(timeoutMs_)) {
        return RobotSocketResult::failure(
            QString("Timed out sending robot socket message '%1': %2").arg(message, socket_.errorString()));
    }

    return RobotSocketResult::success(QString("Sent robot socket message: %1").arg(message));
}

QString RobotSocketClient::endpointText() const {
    return QString("%1:%2").arg(config_.ip).arg(config_.portMotion);
}
```

- [ ] **Step 6: Run socket tests**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_robot_socket_client
ctest --test-dir build -C Release -R test_robot_socket_client --output-on-failure
```

Expected:

```text
100% tests passed
```

- [ ] **Step 7: Commit Task 2**

```powershell
git add CMakeLists.txt src\robot\RobotSocketClient.h src\robot\RobotSocketClient.cpp tests\test_robot_socket_client.cpp
git commit -m "feat: add robot socket client"
```

## Task 3: Add `RobotPrintService`

**Files:**
- Create: `src/services/robot/RobotPrintService.h`
- Create: `src/services/robot/RobotPrintService.cpp`
- Create: `tests/test_robot_print_service.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create the service header**

Create `src/services/robot/RobotPrintService.h`:

```cpp
#pragma once

#include <QObject>
#include <QString>

#include <memory>

#include "app/AppConfig.h"
#include "plc/PlcClient.h"
#include "plc/PlcPrintSignalManager.h"
#include "robot/RobotSocketClient.h"
#include "services/robot/RobotPrintInterfaces.h"

// Qt-facing service for the original print control buttons.
//
// This service intentionally covers only the print workflow. General robot
// movement remains in the placeholder service until a later migration slice.
class RobotPrintService final : public QObject {
    Q_OBJECT

public:
    RobotPrintService(const RobotConfig& robotConfig,
                      const PlcConfig& plcConfig,
                      IPrintUploadState& uploadState,
                      QObject* parent = nullptr);
    RobotPrintService(IPrintUploadState& uploadState,
                      IRobotCommandSocket& socket,
                      IPlcPrintController& plcPrint,
                      QObject* parent = nullptr);

    bool linkState() const;
    bool pauseFlag() const;
    bool stopFlag() const;

signals:
    void messageRequested(const QString& title, const QString& message);
    void robotStatusRequested(const QString& text, bool connected);

public slots:
    void connectRobot();
    void startPrint();
    void pausePrint();
    void continuePrint();
    void stopPrint();

private:
    bool requireConnected();
    void reportPlcFailure(const QString& action, const PlcResult& result);
    void reportOptionalSocketFailure(const QString& action, const RobotSocketResult& result);

    std::unique_ptr<RobotSocketClient> ownedSocket_;
    std::unique_ptr<PlcClient> ownedPlc_;
    std::unique_ptr<PlcPrintSignalManager> ownedPrintSignals_;

    IPrintUploadState& uploadState_;
    IRobotCommandSocket& socket_;
    IPlcPrintController& plcPrint_;

    bool linkState_ = false;
    bool pauseFlag_ = false;
    bool stopFlag_ = true;
};
```

- [ ] **Step 2: Create workflow tests**

Create `tests/test_robot_print_service.cpp`:

```cpp
#include <QtTest/QtTest>

#include "services/robot/RobotPrintService.h"

class FakeUploadState final : public IPrintUploadState {
public:
    bool success = false;
    QString filename;

    bool uploadSucceeded() const override { return success; }
    QString uploadedModFilename() const override { return filename; }
};

class FakeSocket final : public IRobotCommandSocket {
public:
    bool connected = false;
    bool failLoadMod = false;
    bool failSideMessage = false;
    QStringList operations;

    RobotSocketResult connectToRobot() override {
        operations << "socket:connect";
        connected = true;
        return RobotSocketResult::success("connected");
    }

    void disconnectFromRobot() override {
        operations << "socket:disconnect";
        connected = false;
    }

    bool isConnected() const override { return connected; }

    RobotSocketResult sendLoadMod(const QString& modFilename) override {
        operations << QString("socket:load:%1").arg(modFilename);
        if (failLoadMod) {
            return RobotSocketResult::failure("load mod failed");
        }
        return RobotSocketResult::success();
    }

    RobotSocketResult sendPause() override {
        operations << "socket:pause";
        return failSideMessage ? RobotSocketResult::failure("pause side message failed") : RobotSocketResult::success();
    }

    RobotSocketResult sendContinue() override {
        operations << "socket:continue";
        return failSideMessage ? RobotSocketResult::failure("continue side message failed") : RobotSocketResult::success();
    }

    RobotSocketResult sendStop() override {
        operations << "socket:stop";
        return failSideMessage ? RobotSocketResult::failure("stop side message failed") : RobotSocketResult::success();
    }
};

class FakePlcPrint final : public IPlcPrintController {
public:
    bool failNext = false;
    QStringList operations;

    PlcResult startPrint(bool waitAck = true) override {
        Q_UNUSED(waitAck);
        operations << "plc:start";
        return result();
    }

    PlcResult pausePrint(bool waitAck = true) override {
        Q_UNUSED(waitAck);
        operations << "plc:pause";
        return result();
    }

    PlcResult resumePrint(bool waitAck = true) override {
        Q_UNUSED(waitAck);
        operations << "plc:resume";
        return result();
    }

    PlcResult stopPrint(bool waitAck = true) override {
        Q_UNUSED(waitAck);
        operations << "plc:stop";
        return result();
    }

private:
    PlcResult result() {
        if (failNext) {
            failNext = false;
            return PlcResult::failure("plc failed");
        }
        return PlcResult::success();
    }
};

class TestRobotPrintService final : public QObject {
    Q_OBJECT

private slots:
    void startFailsWithoutUpload();
    void startFailsWithoutFilename();
    void startFailsWhenSocketNotConnected();
    void startSendsLoadModBeforePlcStart();
    void startStopsWhenLoadModFails();
    void pauseContinueStopUsePlcAndOptionalSocketMessages();
    void sideMessageFailureDoesNotUndoPlcSuccess();
};

void TestRobotPrintService::startFailsWithoutUpload() {
    FakeUploadState upload;
    FakeSocket socket;
    FakePlcPrint plc;
    RobotPrintService service(upload, socket, plc);
    QSignalSpy messages(&service, &RobotPrintService::messageRequested);

    service.startPrint();

    QCOMPARE(messages.count(), 1);
    QVERIFY(socket.operations.isEmpty());
    QVERIFY(plc.operations.isEmpty());
}

void TestRobotPrintService::startFailsWithoutFilename() {
    FakeUploadState upload;
    upload.success = true;
    FakeSocket socket;
    socket.connected = true;
    FakePlcPrint plc;
    RobotPrintService service(upload, socket, plc);

    service.startPrint();

    QVERIFY(socket.operations.isEmpty());
    QVERIFY(plc.operations.isEmpty());
}

void TestRobotPrintService::startFailsWhenSocketNotConnected() {
    FakeUploadState upload;
    upload.success = true;
    upload.filename = "path_a.mod";
    FakeSocket socket;
    FakePlcPrint plc;
    RobotPrintService service(upload, socket, plc);

    service.startPrint();

    QVERIFY(socket.operations.isEmpty());
    QVERIFY(plc.operations.isEmpty());
}

void TestRobotPrintService::startSendsLoadModBeforePlcStart() {
    FakeUploadState upload;
    upload.success = true;
    upload.filename = "path_a.mod";
    FakeSocket socket;
    socket.connected = true;
    FakePlcPrint plc;
    RobotPrintService service(upload, socket, plc);

    service.startPrint();

    QCOMPARE(socket.operations, QStringList({"socket:load:path_a.mod"}));
    QCOMPARE(plc.operations, QStringList({"plc:start"}));
    QVERIFY(!service.stopFlag());
    QVERIFY(!service.pauseFlag());
}

void TestRobotPrintService::startStopsWhenLoadModFails() {
    FakeUploadState upload;
    upload.success = true;
    upload.filename = "path_a.mod";
    FakeSocket socket;
    socket.connected = true;
    socket.failLoadMod = true;
    FakePlcPrint plc;
    RobotPrintService service(upload, socket, plc);

    service.startPrint();

    QCOMPARE(socket.operations, QStringList({"socket:load:path_a.mod"}));
    QVERIFY(plc.operations.isEmpty());
}

void TestRobotPrintService::pauseContinueStopUsePlcAndOptionalSocketMessages() {
    FakeUploadState upload;
    FakeSocket socket;
    socket.connected = true;
    FakePlcPrint plc;
    RobotPrintService service(upload, socket, plc);

    service.pausePrint();
    service.continuePrint();
    service.stopPrint();

    QCOMPARE(plc.operations, QStringList({"plc:pause", "plc:resume", "plc:stop"}));
    QCOMPARE(socket.operations, QStringList({"socket:pause", "socket:continue", "socket:stop"}));
    QVERIFY(service.stopFlag());
    QVERIFY(!service.pauseFlag());
}

void TestRobotPrintService::sideMessageFailureDoesNotUndoPlcSuccess() {
    FakeUploadState upload;
    FakeSocket socket;
    socket.connected = true;
    socket.failSideMessage = true;
    FakePlcPrint plc;
    RobotPrintService service(upload, socket, plc);
    QSignalSpy messages(&service, &RobotPrintService::messageRequested);

    service.pausePrint();

    QCOMPARE(plc.operations, QStringList({"plc:pause"}));
    QCOMPARE(socket.operations, QStringList({"socket:pause"}));
    QVERIFY(service.pauseFlag());
    QVERIFY(messages.count() >= 1);
}

QTEST_MAIN(TestRobotPrintService)
#include "test_robot_print_service.moc"
```

- [ ] **Step 3: Add the test target to CMake**

Add to `APP_SOURCES`:

```cmake
    src/services/robot/RobotPrintInterfaces.h
    src/services/robot/RobotPrintService.h
    src/services/robot/RobotPrintService.cpp
```

Add this test target:

```cmake
set(TEST_ROBOT_PRINT_SERVICE_SOURCES
    tests/test_robot_print_service.cpp
    src/services/robot/RobotPrintService.cpp
    src/robot/RobotSocketClient.cpp
    src/plc/PlcClient.cpp
    src/plc/PlcPrintSignalManager.cpp
    src/plc/S7Data.cpp
)
set(TEST_ROBOT_PRINT_SERVICE_READY TRUE)
set(MISSING_TEST_ROBOT_PRINT_SERVICE_SOURCES)
foreach(TEST_SOURCE ${TEST_ROBOT_PRINT_SERVICE_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        set(TEST_ROBOT_PRINT_SERVICE_READY FALSE)
        list(APPEND MISSING_TEST_ROBOT_PRINT_SERVICE_SOURCES ${TEST_SOURCE})
    endif()
endforeach()

if(TEST_ROBOT_PRINT_SERVICE_READY)
    add_executable(test_robot_print_service ${TEST_ROBOT_PRINT_SERVICE_SOURCES})
    target_include_directories(test_robot_print_service PRIVATE src)
    target_link_libraries(test_robot_print_service PRIVATE Qt6::Core Qt6::Network Qt6::Test snap7::snap7)
    copy_qt_test_runtime(test_robot_print_service)
    add_custom_command(TARGET test_robot_print_service POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt6::Network>
                $<TARGET_FILE:snap7::snap7>
                $<TARGET_FILE_DIR:test_robot_print_service>
    )
    add_test(NAME test_robot_print_service COMMAND test_robot_print_service)
elseif(MISSING_TEST_ROBOT_PRINT_SERVICE_SOURCES)
    string(REPLACE ";" "\n  " MISSING_TEST_ROBOT_PRINT_SERVICE_TEXT "${MISSING_TEST_ROBOT_PRINT_SERVICE_SOURCES}")
    message(STATUS "test_robot_print_service target deferred. Missing files:\n  ${MISSING_TEST_ROBOT_PRINT_SERVICE_TEXT}")
endif()
```

- [ ] **Step 4: Run the failing service tests**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_robot_print_service
ctest --test-dir build -C Release -R test_robot_print_service --output-on-failure
```

Expected before implementation: build or tests fail because `RobotPrintService.cpp` is missing or empty.

- [ ] **Step 5: Implement `RobotPrintService`**

Create `src/services/robot/RobotPrintService.cpp`:

```cpp
#include "services/robot/RobotPrintService.h"

RobotPrintService::RobotPrintService(const RobotConfig& robotConfig,
                                     const PlcConfig& plcConfig,
                                     IPrintUploadState& uploadState,
                                     QObject* parent)
    : QObject(parent),
      ownedSocket_(std::make_unique<RobotSocketClient>(robotConfig)),
      ownedPlc_(std::make_unique<PlcClient>(plcConfig)),
      ownedPrintSignals_(std::make_unique<PlcPrintSignalManager>(*ownedPlc_)),
      uploadState_(uploadState),
      socket_(*ownedSocket_),
      plcPrint_(*ownedPrintSignals_) {
}

RobotPrintService::RobotPrintService(IPrintUploadState& uploadState,
                                     IRobotCommandSocket& socket,
                                     IPlcPrintController& plcPrint,
                                     QObject* parent)
    : QObject(parent),
      uploadState_(uploadState),
      socket_(socket),
      plcPrint_(plcPrint) {
}

bool RobotPrintService::linkState() const {
    return linkState_;
}

bool RobotPrintService::pauseFlag() const {
    return pauseFlag_;
}

bool RobotPrintService::stopFlag() const {
    return stopFlag_;
}

void RobotPrintService::connectRobot() {
    const RobotSocketResult result = socket_.connectToRobot();
    linkState_ = result.ok && socket_.isConnected();

    emit robotStatusRequested(result.message, linkState_);
    emit messageRequested(linkState_ ? "Robot" : "Robot connection", result.message);
}

void RobotPrintService::startPrint() {
    if (!uploadState_.uploadSucceeded()) {
        emit messageRequested("Warning", "Please upload a MOD file before starting print.");
        return;
    }

    const QString modFilename = uploadState_.uploadedModFilename().trimmed();
    if (modFilename.isEmpty()) {
        emit messageRequested("Warning", "Uploaded MOD filename is empty. Please upload again.");
        return;
    }

    if (!requireConnected()) {
        return;
    }

    stopFlag_ = false;
    pauseFlag_ = false;

    const RobotSocketResult loadResult = socket_.sendLoadMod(modFilename);
    if (!loadResult.ok) {
        emit messageRequested("Robot", QString("Failed to send MOD filename to robot: %1").arg(loadResult.message));
        return;
    }

    const PlcResult plcResult = plcPrint_.startPrint(true);
    if (!plcResult.ok) {
        reportPlcFailure("start print", plcResult);
        return;
    }

    emit messageRequested("Success", "MOD filename sent. Print task started.");
}

void RobotPrintService::pausePrint() {
    if (!requireConnected()) {
        return;
    }

    const PlcResult plcResult = plcPrint_.pausePrint(true);
    if (!plcResult.ok) {
        reportPlcFailure("pause print", plcResult);
        return;
    }

    pauseFlag_ = true;
    reportOptionalSocketFailure("pause side message", socket_.sendPause());
    emit messageRequested("Success", "Print paused.");
}

void RobotPrintService::continuePrint() {
    if (!requireConnected()) {
        return;
    }

    const PlcResult plcResult = plcPrint_.resumePrint(true);
    if (!plcResult.ok) {
        reportPlcFailure("continue print", plcResult);
        return;
    }

    pauseFlag_ = false;
    reportOptionalSocketFailure("continue side message", socket_.sendContinue());
    emit messageRequested("Success", "Print continued.");
}

void RobotPrintService::stopPrint() {
    if (!requireConnected()) {
        return;
    }

    const PlcResult plcResult = plcPrint_.stopPrint(true);
    if (!plcResult.ok) {
        reportPlcFailure("stop print", plcResult);
        return;
    }

    stopFlag_ = true;
    pauseFlag_ = false;
    reportOptionalSocketFailure("stop side message", socket_.sendStop());
    emit messageRequested("Success", "Print stopped.");
}

bool RobotPrintService::requireConnected() {
    linkState_ = socket_.isConnected();
    if (!linkState_) {
        emit messageRequested("Robot control", "Robot is not connected. Please connect the robot first.");
        emit robotStatusRequested("Robot socket not connected.", false);
        return false;
    }
    return true;
}

void RobotPrintService::reportPlcFailure(const QString& action, const PlcResult& result) {
    emit messageRequested("PLC", QString("PLC handshake failed during %1: %2").arg(action, result.message));
}

void RobotPrintService::reportOptionalSocketFailure(const QString& action, const RobotSocketResult& result) {
    if (!result.ok) {
        emit messageRequested("Robot", QString("Non-fatal %1 failed: %2").arg(action, result.message));
    }
}
```

- [ ] **Step 6: Run service tests**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_robot_print_service
ctest --test-dir build -C Release -R test_robot_print_service --output-on-failure
```

Expected:

```text
100% tests passed
```

- [ ] **Step 7: Commit Task 3**

```powershell
git add CMakeLists.txt src\services\robot\RobotPrintService.h src\services\robot\RobotPrintService.cpp tests\test_robot_print_service.cpp
git commit -m "feat: add robot print workflow service"
```

## Task 4: Wire the Existing UI Buttons

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add `RobotPrintService` to `MainWindow`**

In `src/ui/MainWindow.h`, add:

```cpp
#include "services/robot/RobotPrintService.h"
```

Add a member next to `robotService_`:

```cpp
RobotPrintService robotPrintService_;
```

- [ ] **Step 2: Construct `RobotPrintService`**

In `src/ui/MainWindow.cpp`, update the constructor initializer list so `robotPrintService_` is built after `ftpService_`:

```cpp
      robotService_(this),
      ftpService_(config_.ftp, this),
      robotPrintService_(config_.robot, config_.plc, ftpService_, this),
      plcService_(config_.plc, ui_, this),
```

Keep existing service construction order if class member order requires a different initializer order. The important rule is that `ftpService_` must be constructed before `robotPrintService_`.

- [ ] **Step 3: Connect robot print service messages**

In `MainWindow::connectSignals()`, add:

```cpp
    connect(&robotPrintService_, &RobotPrintService::messageRequested,
            this, &MainWindow::showPlaceholderMessage);
    connect(&robotPrintService_, &RobotPrintService::robotStatusRequested,
            this, [this](const QString& text, bool connected) {
                ui_->robot_state_show->setText(text);
                ui_->robot_state_show->setStyleSheet(
                    connected ? "background-color: lightgreen; font-size: 12px;"
                              : "background-color: pink; font-size: 12px;");
            });
```

- [ ] **Step 4: Rewire only the print-related buttons**

Change these existing connections:

```cpp
    connect(ui_->ConnectionRobotButton, &QPushButton::clicked, &robotService_, &RobotService::connectRobot);
    connect(ui_->Robot_movebutton_Gcode, &QPushButton::clicked, &robotService_, &RobotService::startPrint);
    connect(ui_->PauseRobotButton, &QPushButton::clicked, &robotService_, &RobotService::pausePrint);
    connect(ui_->ContinueRobotButton, &QPushButton::clicked, &robotService_, &RobotService::continuePrint);
    connect(ui_->StopRobotButton, &QPushButton::clicked, &robotService_, &RobotService::stopPrint);
```

to:

```cpp
    connect(ui_->ConnectionRobotButton, &QPushButton::clicked, &robotPrintService_, &RobotPrintService::connectRobot);
    connect(ui_->Robot_movebutton_Gcode, &QPushButton::clicked, &robotPrintService_, &RobotPrintService::startPrint);
    connect(ui_->PauseRobotButton, &QPushButton::clicked, &robotPrintService_, &RobotPrintService::pausePrint);
    connect(ui_->ContinueRobotButton, &QPushButton::clicked, &robotPrintService_, &RobotPrintService::continuePrint);
    connect(ui_->StopRobotButton, &QPushButton::clicked, &robotPrintService_, &RobotPrintService::stopPrint);
```

Leave `Robot_movebutton`, `RobotGohomeButton`, `RobotaxismoveButton`, `PositionermoveButton`, and `PositionercomebackButton` connected to the placeholder `RobotService`.

- [ ] **Step 5: Build the app**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Expected: Release build completes and links `AFP_ABB_CPP.exe`.

- [ ] **Step 6: Run all tests**

Run:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Expected:

```text
100% tests passed
```

- [ ] **Step 7: Commit Task 4**

```powershell
git add src\ui\MainWindow.h src\ui\MainWindow.cpp CMakeLists.txt
git commit -m "feat: wire robot print controls"
```

## Task 5: Document and Verify Slice F

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Update README summary**

In `README.md`, update the top slice summary to mention Slice F:

```markdown
Slice F adds the robot print workflow around the existing UI buttons: MOD upload state is checked, `LOAD_MOD:<filename>#` is sent to the ABB robot socket, and DB18 PLC print commands are used for start, pause, continue, and stop.
```

- [ ] **Step 2: Add robot print workflow documentation**

Add this section near the FTP and PLC DB18 documentation:

````markdown
## Robot Print Workflow

Slice F wires the existing print buttons without changing the UI design.

Before starting print:

1. Upload a `.mod` file with `locusselectsetButton`.
2. Connect the robot socket with `ConnectionRobotButton`.
3. Click `Robot_movebutton_Gcode`.

The C++ workflow preserves the Python order:

```text
check FTP upload success
check uploaded MOD filename
send LOAD_MOD:<filename># to robot.ip:robot.port_motion
write DB18 START and wait for ACK
clear DB18 command after ACK
```

Pause, continue, and stop use the DB18 PLC command manager first. After a successful PLC ACK, the app also attempts the matching socket side message:

```text
PAUSE#
CONTINUE#
STOP#
```

The side message is non-fatal for pause, continue, and stop because the Python workflow already treats the PLC handshake as the main control path.

This slice does not convert robot jog/move/reset commands, external-axis movement, RAPID files, or robot monitoring sockets.
````

- [ ] **Step 3: Add configuration note**

Add or update the robot configuration example:

````markdown
Robot socket settings are read from `config/env_config.json`:

```json
"robot": {
  "ip": "192.168.0.104",
  "port_motion": 5010,
  "port_server": 5012,
  "transport": {
    "type": "plc"
  }
}
```

Slice F uses `robot.ip` and `robot.port_motion` for the print socket. `robot.port_server` remains parsed for compatibility with the original config, but the broader monitor/server socket behavior is not converted in this slice.
````

- [ ] **Step 4: Run full verification**

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

- [ ] **Step 5: Commit Task 5**

```powershell
git add README.md
git commit -m "docs: document robot print workflow"
```

## Self-Review Checklist

- [ ] `AFP_ABB_Python` is untouched.
- [ ] `src/ui/Interface4600_window.ui` is untouched.
- [ ] `Robot_movebutton_Gcode` checks FTP upload state before print start.
- [ ] Start print refuses an empty uploaded MOD filename.
- [ ] Start print refuses to run when robot socket is not connected.
- [ ] `LOAD_MOD:<filename>#` is sent before PLC START.
- [ ] PLC START is not sent if `LOAD_MOD` fails.
- [ ] Pause, continue, and stop require the robot link state.
- [ ] Pause, continue, and stop use DB18 PLC commands.
- [ ] Optional `PAUSE#`, `CONTINUE#`, and `STOP#` socket side-message failure is non-fatal after PLC ACK.
- [ ] Robot movement buttons remain placeholders.
- [ ] Qt Network is deployed for the app and tests.
- [ ] Automated tests do not require a live PLC, robot, FTP server, or pressure sensor.
- [ ] README explains what Slice F does and does not convert.
