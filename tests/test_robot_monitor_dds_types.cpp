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
    QCOMPARE(ddsSample.sample_sequence(), 7ULL);
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
