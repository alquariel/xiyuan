#include <QtTest/QtTest>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include "robot_monitor/RobotMonitorRecorder.h"

// Verifies that the monitor recorder produces operator-readable validation files.
class TestRobotMonitorRecorder final : public QObject {
    Q_OBJECT

private slots:
    void writesCsvHeaderAndSample();
    void writesRawMessagesWhenEnabled();
};

void TestRobotMonitorRecorder::writesCsvHeaderAndSample() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    RobotMonitorRecorder recorder(dir.path(), true, false, true);
    QVERIFY2(recorder.start(), qPrintable(recorder.lastError()));

    RobotMonitorSample sample;
    sample.receivedAtUtc = QDateTime::fromString("2026-06-03T01:02:03.004Z", Qt::ISODateWithMs);
    sample.sequence = 7;
    sample.x = 1.1;
    sample.y = 2.2;
    sample.z = 3.3;
    sample.q1 = 0.1;
    sample.q2 = 0.2;
    sample.q3 = 0.3;
    sample.q4 = 0.4;
    sample.joints = {1, 2, 3, 4, 5, 6};
    sample.externalAxis = {10, 20, 30, 40, 50, 60};
    sample.commandedTcpSpeed = 120.5;
    sample.estimatedTcpSpeed = 99.9;

    QVERIFY(recorder.recordSample(sample));
    recorder.close();

    const QStringList files = QDir(dir.path()).entryList(QStringList() << "*.csv", QDir::Files);
    QCOMPARE(files.size(), 1);

    QFile csv(QDir(dir.path()).filePath(files.first()));
    QVERIFY(csv.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString content = QString::fromUtf8(csv.readAll());
    QVERIFY(content.contains("received_at_utc,seq,x,y,z,q1,q2,q3,q4,j1,j2,j3,j4,j5,j6"));
    QVERIFY(content.contains("2026-06-03T01:02:03.004Z,7,1.1,2.2,3.3"));
}

void TestRobotMonitorRecorder::writesRawMessagesWhenEnabled() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    RobotMonitorRecorder recorder(dir.path(), false, true, true);
    QVERIFY2(recorder.start(), qPrintable(recorder.lastError()));
    const QString rawMessage = "  #ROBOT_MONITOR seq=1#  ";
    QVERIFY(recorder.recordRawMessage(rawMessage));
    recorder.close();

    const QStringList files = QDir(dir.path()).entryList(QStringList() << "*.raw.log", QDir::Files);
    QCOMPARE(files.size(), 1);

    QFile raw(QDir(dir.path()).filePath(files.first()));
    QVERIFY(raw.open(QIODevice::ReadOnly | QIODevice::Text));
    QCOMPARE(QString::fromUtf8(raw.readAll()), rawMessage + "\n");
}

QTEST_MAIN(TestRobotMonitorRecorder)
#include "test_robot_monitor_recorder.moc"
