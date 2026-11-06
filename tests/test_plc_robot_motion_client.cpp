#include <QtTest/QtTest>

#include <array>
#include <limits>

#include <QHash>
#include <QSet>
#include <QStringList>

#include "robot/PlcRobotMotionClient.h"

class FakeProcessAccess final : public IPlcProcessAccess {
public:
    QHash<int, qint32> paInt32;
    QHash<int, qint32> peInt32;
    QHash<int, quint8> peByte;
    QSet<int> failPaReads;
    QSet<int> failPaWrites;
    QSet<int> failPeInt32Reads;
    QSet<int> failPeByteReads;
    QStringList operations;
    int busyDoneReadsBeforeSuccess = 0;
    bool errorStatusAfterAck = false;
    bool ackReadSeen = false;

    PlcResult writeProcessOutputInt32(int byteOffset, qint32 value) override {
        operations << QString("write:%1:%2").arg(byteOffset).arg(value);
        if (failPaWrites.contains(byteOffset)) {
            return PlcResult::failure(QString("fake PA write failure at %1").arg(byteOffset));
        }
        paInt32[byteOffset] = value;
        return PlcResult::success();
    }

    PlcResult readProcessOutputInt32(int byteOffset, qint32& value) override {
        operations << QString("read-pa:%1").arg(byteOffset);
        if (failPaReads.contains(byteOffset)) {
            return PlcResult::failure(QString("fake PA read failure at %1").arg(byteOffset));
        }
        value = paInt32.value(byteOffset, 0);
        return PlcResult::success();
    }

    PlcResult readProcessInputInt32(int byteOffset, qint32& value) override {
        operations << QString("read-pe:%1").arg(byteOffset);
        if (failPeInt32Reads.contains(byteOffset)) {
            return PlcResult::failure(QString("fake PE int32 read failure at %1").arg(byteOffset));
        }
        if (byteOffset == 145) {
            ackReadSeen = true;
        }
        if (byteOffset == 149 && busyDoneReadsBeforeSuccess > 0) {
            --busyDoneReadsBeforeSuccess;
            if (busyDoneReadsBeforeSuccess == 0) {
                peByte[153] = 0;
                value = peInt32.value(byteOffset, 0);
            } else {
                value = 0;
            }
            return PlcResult::success();
        }
        value = peInt32.value(byteOffset, 0);
        return PlcResult::success();
    }

    PlcResult readProcessInputByte(int byteOffset, quint8& value) override {
        operations << QString("read-byte:%1").arg(byteOffset);
        if (failPeByteReads.contains(byteOffset)) {
            return PlcResult::failure(QString("fake PE byte read failure at %1").arg(byteOffset));
        }
        if (byteOffset == 153 && errorStatusAfterAck && ackReadSeen) {
            value = 0x02;
            return PlcResult::success();
        }
        value = peByte.value(byteOffset, 0);
        return PlcResult::success();
    }
};

class TestPlcRobotMotionClient final : public QObject {
    Q_OBJECT

private slots:
    void cartesianWritesCompositeCommandAndSequence();
    void jointsWriteAbsJCommand();
    void externalAxisWritesPythonAxisOrder();
    void ackTimeoutReturnsFailure();
    void doneTimeoutReturnsFailure();
    void errorStatusReturnsFailure();
    void sequenceReadFailureReturnsFailureAndWritesNoCommand();
    void payloadWriteFailureReturnsFailureAndWritesNoCommandOrSequence();
    void ackReadFailureReturnsFailure();
    void doneReadFailureReturnsFailure();
    void scaledConversionMatchesPythonTiesToEven_data();
    void scaledConversionMatchesPythonTiesToEven();
    void invalidScaledValueReturnsFailureAndWritesNoCommandOrSequence_data();
    void invalidScaledValueReturnsFailureAndWritesNoCommandOrSequence();
    void sequenceOverflowReturnsFailureAndWritesNoCommandOrSequence();
    void busyThenDoneSucceeds();
    void errorDuringDoneReturnsFailure();
};

void primeSuccessfulMotion(FakeProcessAccess& plc) {
    plc.paInt32[159] = 41;
    plc.peInt32[145] = 42;
    plc.peInt32[149] = 42;
    plc.peByte[153] = 0;
}

int firstWriteIndex(const FakeProcessAccess& plc, int byteOffset) {
    const QString prefix = QString("write:%1:").arg(byteOffset);
    for (int i = 0; i < plc.operations.size(); ++i) {
        if (plc.operations[i].startsWith(prefix)) {
            return i;
        }
    }
    return -1;
}

void verifyCommandBeforeSequenceAndAfterPayload(const FakeProcessAccess& plc,
                                                int commandValue,
                                                const QList<int>& payloadOffsets) {
    const int commandIndex = plc.operations.indexOf(QString("write:155:%1").arg(commandValue));
    const int sequenceIndex = plc.operations.indexOf("write:159:42");
    QVERIFY2(commandIndex >= 0, qPrintable(QString("missing command write %1").arg(commandValue)));
    QVERIFY2(sequenceIndex >= 0, "missing sequence write");
    QVERIFY(commandIndex < sequenceIndex);

    for (int offset : payloadOffsets) {
        const int payloadIndex = firstWriteIndex(plc, offset);
        QVERIFY2(payloadIndex >= 0, qPrintable(QString("missing payload/speed write at %1").arg(offset)));
        QVERIFY2(payloadIndex < commandIndex,
                 qPrintable(QString("write at %1 happened after command").arg(offset)));
    }
}

void verifyNoCommandOrSequenceWrites(const FakeProcessAccess& plc) {
    QCOMPARE(firstWriteIndex(plc, 155), -1);
    QCOMPARE(firstWriteIndex(plc, 159), -1);
}

void TestPlcRobotMotionClient::cartesianWritesCompositeCommandAndSequence() {
    FakeProcessAccess plc;
    primeSuccessfulMotion(plc);
    PlcRobotMotionClient client(plc);
    client.setSpeed({10.0, 20.0, 30.0, 40.0});
    client.moveExternalAxis({1.0, 2.0, 3.0, 0.0, 0.0, 0.0});
    primeSuccessfulMotion(plc);
    plc.operations.clear();

    const RobotPose pose{{100.1, 200.2, 300.3}, {0.1, 0.2, 0.3, 0.4}};
    const RobotMotionResult result = client.moveCartesian(pose);

    QVERIFY2(result.ok, qPrintable(result.message));
    QCOMPARE(plc.paInt32.value(99), qint32(100100));
    QCOMPARE(plc.paInt32.value(103), qint32(200200));
    QCOMPARE(plc.paInt32.value(107), qint32(300300));
    QCOMPARE(plc.paInt32.value(111), qint32(100));
    QCOMPARE(plc.paInt32.value(115), qint32(200));
    QCOMPARE(plc.paInt32.value(119), qint32(300));
    QCOMPARE(plc.paInt32.value(123), qint32(400));
    QCOMPARE(plc.paInt32.value(135), qint32(1000));
    QCOMPARE(plc.paInt32.value(127), qint32(2000));
    QCOMPARE(plc.paInt32.value(131), qint32(3000));
    QCOMPARE(plc.paInt32.value(139), qint32(10000));
    QCOMPARE(plc.paInt32.value(143), qint32(20000));
    QCOMPARE(plc.paInt32.value(147), qint32(30000));
    QCOMPARE(plc.paInt32.value(151), qint32(40000));
    QCOMPARE(plc.paInt32.value(155), qint32(120));
    QCOMPARE(plc.paInt32.value(159), qint32(42));
    verifyCommandBeforeSequenceAndAfterPayload(plc, 120, {99, 103, 107, 111, 115, 119, 123,
                                                          135, 127, 131, 139, 143, 147, 151});
}

void TestPlcRobotMotionClient::jointsWriteAbsJCommand() {
    FakeProcessAccess plc;
    primeSuccessfulMotion(plc);
    PlcRobotMotionClient client(plc);

    const RobotMotionResult result = client.moveJoints({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});

    QVERIFY2(result.ok, qPrintable(result.message));
    QCOMPARE(plc.paInt32.value(99), qint32(1000));
    QCOMPARE(plc.paInt32.value(103), qint32(2000));
    QCOMPARE(plc.paInt32.value(107), qint32(3000));
    QCOMPARE(plc.paInt32.value(111), qint32(4000));
    QCOMPARE(plc.paInt32.value(115), qint32(5000));
    QCOMPARE(plc.paInt32.value(119), qint32(6000));
    QCOMPARE(plc.paInt32.value(123), qint32(0));
    QCOMPARE(plc.paInt32.value(155), qint32(2));
    QCOMPARE(plc.paInt32.value(159), qint32(42));
    verifyCommandBeforeSequenceAndAfterPayload(plc, 2, {99, 103, 107, 111, 115, 119, 123,
                                                        139, 143, 147, 151});
}

void TestPlcRobotMotionClient::externalAxisWritesPythonAxisOrder() {
    FakeProcessAccess plc;
    primeSuccessfulMotion(plc);
    PlcRobotMotionClient client(plc);

    const RobotMotionResult result = client.moveExternalAxis({11.0, 22.0, 33.0, 44.0, 55.0, 66.0});

    QVERIFY2(result.ok, qPrintable(result.message));
    QCOMPARE(plc.paInt32.value(135), qint32(11000));
    QCOMPARE(plc.paInt32.value(127), qint32(22000));
    QCOMPARE(plc.paInt32.value(131), qint32(33000));
    QCOMPARE(plc.paInt32.value(155), qint32(34));
    QCOMPARE(plc.paInt32.value(159), qint32(42));
    verifyCommandBeforeSequenceAndAfterPayload(plc, 34, {135, 127, 131, 139, 143, 147, 151});
}

void TestPlcRobotMotionClient::ackTimeoutReturnsFailure() {
    FakeProcessAccess plc;
    plc.paInt32[159] = 41;
    plc.peByte[153] = 0;
    PlcRobotMotionClient client(plc, 5, 5, 1);

    const RobotMotionResult result = client.moveJoints({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});

    QVERIFY(!result.ok);
    QVERIFY(result.message.contains("ACK", Qt::CaseInsensitive));
}

void TestPlcRobotMotionClient::doneTimeoutReturnsFailure() {
    FakeProcessAccess plc;
    plc.paInt32[159] = 41;
    plc.peInt32[145] = 42;
    plc.peByte[153] = 0x01;
    PlcRobotMotionClient client(plc, 5, 5, 1);

    const RobotMotionResult result = client.moveJoints({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});

    QVERIFY(!result.ok);
    QVERIFY(result.message.contains("DONE", Qt::CaseInsensitive));
}

void TestPlcRobotMotionClient::errorStatusReturnsFailure() {
    FakeProcessAccess plc;
    plc.paInt32[159] = 41;
    plc.peInt32[145] = 42;
    plc.peByte[153] = 0x02;
    plc.peByte[154] = 7;
    PlcRobotMotionClient client(plc, 5, 5, 1);

    const RobotMotionResult result = client.moveJoints({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});

    QVERIFY(!result.ok);
    QVERIFY(result.message.contains("error", Qt::CaseInsensitive));
    QVERIFY(result.message.contains("7"));
}

void TestPlcRobotMotionClient::sequenceReadFailureReturnsFailureAndWritesNoCommand() {
    FakeProcessAccess plc;
    plc.failPaReads.insert(159);
    PlcRobotMotionClient client(plc, 5, 5, 1);

    const RobotMotionResult result = client.moveJoints({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});

    QVERIFY(!result.ok);
    QVERIFY(result.message.contains("sequence", Qt::CaseInsensitive));
    verifyNoCommandOrSequenceWrites(plc);
}

void TestPlcRobotMotionClient::payloadWriteFailureReturnsFailureAndWritesNoCommandOrSequence() {
    FakeProcessAccess plc;
    primeSuccessfulMotion(plc);
    plc.failPaWrites.insert(99);
    PlcRobotMotionClient client(plc, 5, 5, 1);

    const RobotMotionResult result = client.moveJoints({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});

    QVERIFY(!result.ok);
    QVERIFY(result.message.contains("write", Qt::CaseInsensitive));
    verifyNoCommandOrSequenceWrites(plc);
}

void TestPlcRobotMotionClient::ackReadFailureReturnsFailure() {
    FakeProcessAccess plc;
    primeSuccessfulMotion(plc);
    plc.failPeInt32Reads.insert(145);
    PlcRobotMotionClient client(plc, 5, 5, 1);

    const RobotMotionResult result = client.moveJoints({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});

    QVERIFY(!result.ok);
    QVERIFY(result.message.contains("ACK", Qt::CaseInsensitive));
    QVERIFY(result.message.contains("read", Qt::CaseInsensitive));
}

void TestPlcRobotMotionClient::doneReadFailureReturnsFailure() {
    FakeProcessAccess plc;
    primeSuccessfulMotion(plc);
    plc.failPeInt32Reads.insert(149);
    PlcRobotMotionClient client(plc, 5, 5, 1);

    const RobotMotionResult result = client.moveJoints({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});

    QVERIFY(!result.ok);
    QVERIFY(result.message.contains("DONE", Qt::CaseInsensitive));
    QVERIFY(result.message.contains("read", Qt::CaseInsensitive));
}

void TestPlcRobotMotionClient::scaledConversionMatchesPythonTiesToEven_data() {
    QTest::addColumn<double>("value");
    QTest::addColumn<qint32>("expectedScaled");

    QTest::newRow("positive-tie-rounds-to-even-down") << 1.0005 << qint32(1000);
    QTest::newRow("positive-tie-rounds-to-even-up") << 1.0015 << qint32(1002);
    QTest::newRow("negative-tie-rounds-to-even-up") << -1.0005 << qint32(-1000);
}

void TestPlcRobotMotionClient::scaledConversionMatchesPythonTiesToEven() {
    QFETCH(double, value);
    QFETCH(qint32, expectedScaled);

    FakeProcessAccess plc;
    primeSuccessfulMotion(plc);
    PlcRobotMotionClient client(plc, 5, 5, 1);

    const RobotMotionResult result = client.moveJoints({value, 2.0, 3.0, 4.0, 5.0, 6.0});

    QVERIFY2(result.ok, qPrintable(result.message));
    QCOMPARE(plc.paInt32.value(99), expectedScaled);
}

void TestPlcRobotMotionClient::invalidScaledValueReturnsFailureAndWritesNoCommandOrSequence_data() {
    QTest::addColumn<double>("invalidValue");

    QTest::newRow("nan") << std::numeric_limits<double>::quiet_NaN();
    QTest::newRow("infinity") << std::numeric_limits<double>::infinity();
    QTest::newRow("above-qint32-range-after-scaling") << 3000000.0;
}

void TestPlcRobotMotionClient::invalidScaledValueReturnsFailureAndWritesNoCommandOrSequence() {
    QFETCH(double, invalidValue);

    FakeProcessAccess plc;
    primeSuccessfulMotion(plc);
    PlcRobotMotionClient client(plc, 5, 5, 1);

    const RobotMotionResult result = client.moveJoints({invalidValue, 2.0, 3.0, 4.0, 5.0, 6.0});

    QVERIFY(!result.ok);
    QVERIFY(result.message.contains("finite", Qt::CaseInsensitive)
            || result.message.contains("range", Qt::CaseInsensitive));
    verifyNoCommandOrSequenceWrites(plc);
}

void TestPlcRobotMotionClient::sequenceOverflowReturnsFailureAndWritesNoCommandOrSequence() {
    FakeProcessAccess plc;
    plc.paInt32[159] = std::numeric_limits<qint32>::max();
    PlcRobotMotionClient client(plc, 5, 5, 1);

    const RobotMotionResult result = client.moveJoints({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});

    QVERIFY(!result.ok);
    QVERIFY(result.message.contains("sequence", Qt::CaseInsensitive));
    QVERIFY(result.message.contains("overflow", Qt::CaseInsensitive)
            || result.message.contains("maximum", Qt::CaseInsensitive));
    verifyNoCommandOrSequenceWrites(plc);
}

void TestPlcRobotMotionClient::busyThenDoneSucceeds() {
    FakeProcessAccess plc;
    primeSuccessfulMotion(plc);
    plc.peByte[153] = 0x01;
    plc.busyDoneReadsBeforeSuccess = 2;
    PlcRobotMotionClient client(plc, 50, 50, 1);

    const RobotMotionResult result = client.moveJoints({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});

    QVERIFY2(result.ok, qPrintable(result.message));
    QVERIFY(plc.operations.count("read-pe:149") >= 2);
}

void TestPlcRobotMotionClient::errorDuringDoneReturnsFailure() {
    FakeProcessAccess plc;
    primeSuccessfulMotion(plc);
    plc.errorStatusAfterAck = true;
    plc.peByte[154] = 9;
    PlcRobotMotionClient client(plc, 5, 5, 1);

    const RobotMotionResult result = client.moveJoints({1.0, 2.0, 3.0, 4.0, 5.0, 6.0});

    QVERIFY(!result.ok);
    QVERIFY(result.message.contains("DONE", Qt::CaseInsensitive));
    QVERIFY(result.message.contains("9"));
}

QTEST_MAIN(TestPlcRobotMotionClient)
#include "test_plc_robot_motion_client.moc"
