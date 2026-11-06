#include <QtTest/QtTest>

#include <QHash>
#include <QStringList>

#include "plc/PlcPrintSignalManager.h"

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
    void delayedAckPollsBeforeClear();
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

void TestPlcPrintSignalManager::delayedAckPollsBeforeClear() {
    FakePlcByteAccess plc;
    plc.ackReadsBeforeSuccess = 2;
    plc.bytes[FakePlcByteAccess::key(18, 1)] = 1;
    PlcPrintSignalManager manager(plc, 100, 1);

    const PlcResult result = manager.startPrint(true);

    QVERIFY2(result.ok, qPrintable(result.message));
    QCOMPARE(plc.bytes.value(FakePlcByteAccess::key(18, 0)), static_cast<quint8>(0));
    QVERIFY(plc.operations.count("read:18:1") >= 3);
    QVERIFY(plc.operations.indexOf("read:18:1") < plc.operations.indexOf("write:18:0:0"));
}

void TestPlcPrintSignalManager::ackTimeoutReturnsFailure() {
    FakePlcByteAccess plc;
    PlcPrintSignalManager manager(plc, 5, 1);

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
