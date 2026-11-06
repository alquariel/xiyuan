#include <QtTest/QtTest>

#include "robot_monitor/RobotMonitorMessageParser.h"

// Exercises conversion of raw robot monitor socket frames into typed samples.
class TestRobotMonitorMessageParser final : public QObject {
    Q_OBJECT

private slots:
    void parsesValidMonitorMessage();
    void parsesCompactRapidFriendlyMessage();
    void rejectsWrongPrefix();
    void rejectsMissingRequiredField();
};

void TestRobotMonitorMessageParser::parsesValidMonitorMessage() {
    const QString raw =
        "#ROBOT_MONITOR seq=12 x=100.20 y=50.10 z=300.00 "
        "q1=0.707 q2=0 q3=0 q4=0 "
        "j1=1.0 j2=2.0 j3=3.0 j4=4.0 j5=5.0 j6=6.0 "
        "eax1=10 eax2=20 eax3=30 speed_tcp=120.5 speed_ori=40 speed_leax=20 speed_reax=10#";

    const RobotMonitorParseResult result = RobotMonitorMessageParser::parse(raw);

    QVERIFY(result.ok);
    QCOMPARE(result.sample.sequence, 12);
    QCOMPARE(result.sample.x, 100.20);
    QCOMPARE(result.sample.y, 50.10);
    QCOMPARE(result.sample.z, 300.00);
    QCOMPARE(result.sample.q1, 0.707);
    QCOMPARE(result.sample.q2, 0.0);
    QCOMPARE(result.sample.q3, 0.0);
    QCOMPARE(result.sample.q4, 0.0);
    QCOMPARE(result.sample.joints[0], 1.0);
    QCOMPARE(result.sample.joints[5], 6.0);
    QCOMPARE(result.sample.externalAxis[0], 10.0);
    QCOMPARE(result.sample.externalAxis[1], 20.0);
    QCOMPARE(result.sample.externalAxis[2], 30.0);
    QCOMPARE(result.sample.externalAxis[3], 0.0);
    QCOMPARE(result.sample.externalAxis[4], 0.0);
    QCOMPARE(result.sample.externalAxis[5], 0.0);
    QCOMPARE(result.sample.commandedTcpSpeed, 120.5);
    QCOMPARE(result.sample.commandedOrientationSpeed, 40.0);
    QCOMPARE(result.sample.commandedExternalLinearSpeed, 20.0);
    QCOMPARE(result.sample.commandedExternalOrientationSpeed, 10.0);
}

void TestRobotMonitorMessageParser::parsesCompactRapidFriendlyMessage() {
    const RobotMonitorParseResult result =
        RobotMonitorMessageParser::parse("#ROBOT_MONITOR s=7 x=10.5 y=20.5 z=30.5 v=80#");

    QVERIFY(result.ok);
    QCOMPARE(result.sample.sequence, 7);
    QCOMPARE(result.sample.x, 10.5);
    QCOMPARE(result.sample.y, 20.5);
    QCOMPARE(result.sample.z, 30.5);
    QCOMPARE(result.sample.commandedTcpSpeed, 80.0);
    QCOMPARE(result.sample.q1, 0.0);
    QCOMPARE(result.sample.joints[0], 0.0);
}

void TestRobotMonitorMessageParser::rejectsWrongPrefix() {
    const RobotMonitorParseResult result =
        RobotMonitorMessageParser::parse("#OTHER seq=1 x=1 y=2 z=3#");

    QVERIFY(!result.ok);
    QVERIFY(result.error.contains("ROBOT_MONITOR"));
}

void TestRobotMonitorMessageParser::rejectsMissingRequiredField() {
    const RobotMonitorParseResult result =
        RobotMonitorMessageParser::parse("#ROBOT_MONITOR seq=1 x=1 y=2#");

    QVERIFY(!result.ok);
    QVERIFY(result.error.contains("z"));
}

QTEST_MAIN(TestRobotMonitorMessageParser)
#include "test_robot_monitor_message_parser.moc"
