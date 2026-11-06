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
    explicit FakeSocket(QStringList* eventLog = nullptr) : eventLog(eventLog) {}

    bool connected = false;
    bool failLoadMod = false;
    bool failSideMessage = false;
    QStringList operations;
    QStringList* eventLog = nullptr;

    RobotSocketResult connectToRobot() override {
        record("socket:connect");
        connected = true;
        return RobotSocketResult::success("connected");
    }

    void disconnectFromRobot() override {
        record("socket:disconnect");
        connected = false;
    }

    bool isConnected() const override { return connected; }

    RobotSocketResult sendLoadMod(const QString& modFilename) override {
        record(QString("socket:load:%1").arg(modFilename));
        if (failLoadMod) {
            return RobotSocketResult::failure("load mod failed");
        }
        return RobotSocketResult::success();
    }

    RobotSocketResult sendPause() override {
        record("socket:pause");
        return failSideMessage ? RobotSocketResult::failure("pause side message failed") : RobotSocketResult::success();
    }

    RobotSocketResult sendContinue() override {
        record("socket:continue");
        return failSideMessage ? RobotSocketResult::failure("continue side message failed") : RobotSocketResult::success();
    }

    RobotSocketResult sendStop() override {
        record("socket:stop");
        return failSideMessage ? RobotSocketResult::failure("stop side message failed") : RobotSocketResult::success();
    }

    RobotSocketResult sendRawMessage(const QString& frame) override {
        record(QStringLiteral("socket:raw:%1").arg(frame));
        return failSideMessage ? RobotSocketResult::failure(QStringLiteral("raw message failed")) : RobotSocketResult::success();
    }

private:
    void record(const QString& operation) {
        operations << operation;
        if (eventLog != nullptr) {
            *eventLog << operation;
        }
    }
};

class FakePlcPrint final : public IPlcPrintController {
public:
    explicit FakePlcPrint(QStringList* eventLog = nullptr) : eventLog(eventLog) {}

    bool failNext = false;
    QStringList operations;
    QStringList* eventLog = nullptr;

    PlcResult startPrint(bool waitAck = true) override {
        Q_UNUSED(waitAck);
        record("plc:start");
        return result();
    }

    PlcResult pausePrint(bool waitAck = true) override {
        Q_UNUSED(waitAck);
        record("plc:pause");
        return result();
    }

    PlcResult resumePrint(bool waitAck = true) override {
        Q_UNUSED(waitAck);
        record("plc:resume");
        return result();
    }

    PlcResult stopPrint(bool waitAck = true) override {
        Q_UNUSED(waitAck);
        record("plc:stop");
        return result();
    }

private:
    void record(const QString& operation) {
        operations << operation;
        if (eventLog != nullptr) {
            *eventLog << operation;
        }
    }

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
    void startKeepsStoppedWhenPlcStartFails();
    void pauseContinueStopUsePlcAndOptionalSocketMessages();
    void pauseContinueStopUsePlcWhenSocketDisconnected();
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
    QStringList events;
    FakeUploadState upload;
    upload.success = true;
    upload.filename = "path_a.mod";
    FakeSocket socket(&events);
    socket.connected = true;
    FakePlcPrint plc(&events);
    RobotPrintService service(upload, socket, plc);

    service.startPrint();

    QCOMPARE(events, QStringList({"socket:load:path_a.mod", "plc:start"}));
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
    QVERIFY(service.stopFlag());
    QVERIFY(!service.pauseFlag());
}

void TestRobotPrintService::startKeepsStoppedWhenPlcStartFails() {
    FakeUploadState upload;
    upload.success = true;
    upload.filename = "path_a.mod";
    FakeSocket socket;
    socket.connected = true;
    FakePlcPrint plc;
    plc.failNext = true;
    RobotPrintService service(upload, socket, plc);

    service.startPrint();

    QCOMPARE(socket.operations, QStringList({"socket:load:path_a.mod"}));
    QCOMPARE(plc.operations, QStringList({"plc:start"}));
    QVERIFY(service.stopFlag());
    QVERIFY(!service.pauseFlag());
}

void TestRobotPrintService::pauseContinueStopUsePlcAndOptionalSocketMessages() {
    QStringList events;
    FakeUploadState upload;
    FakeSocket socket(&events);
    socket.connected = true;
    FakePlcPrint plc(&events);
    RobotPrintService service(upload, socket, plc);

    service.pausePrint();
    service.continuePrint();
    service.stopPrint();

    QCOMPARE(events, QStringList({
        "plc:pause", "socket:pause",
        "plc:resume", "socket:continue",
        "plc:stop", "socket:stop",
    }));
    QVERIFY(service.stopFlag());
    QVERIFY(!service.pauseFlag());
}

void TestRobotPrintService::pauseContinueStopUsePlcWhenSocketDisconnected() {
    QStringList events;
    FakeUploadState upload;
    FakeSocket socket(&events);
    FakePlcPrint plc(&events);
    RobotPrintService service(upload, socket, plc);

    service.pausePrint();
    service.continuePrint();
    service.stopPrint();

    QCOMPARE(events, QStringList({"plc:pause", "plc:resume", "plc:stop"}));
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
