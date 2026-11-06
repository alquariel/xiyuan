#include <QtTest/QtTest>

#include <QList>
#include <QTcpServer>
#include <QTcpSocket>

#include "robot_monitor/RobotMonitorClient.h"

// Verifies that the monitor client reassembles socket chunks into complete frames.
class TestRobotMonitorClient final : public QObject {
    Q_OBJECT

private slots:
    void receivesSplitFrames();
    void sendsHeartbeatMarker();
};

void TestRobotMonitorClient::receivesSplitFrames() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    RobotMonitorClient client;
    QList<QString> rawMessages;
    QList<RobotMonitorSample> samples;
    QStringList errors;

    connect(&client, &RobotMonitorClient::rawMessageReceived, this, [&](const QString& raw) {
        rawMessages << raw;
    });
    connect(&client, &RobotMonitorClient::sampleReceived, this, [&](const RobotMonitorSample& sample) {
        samples << sample;
    });
    connect(&client, &RobotMonitorClient::errorReported, this, [&](const QString& error) {
        errors << error;
    });

    client.connectToRobot("127.0.0.1", server.serverPort());
    QVERIFY(QTest::qWaitFor([&]() { return server.hasPendingConnections(); }, 1000));

    QTcpSocket* socket = server.nextPendingConnection();
    QVERIFY(socket != nullptr);
    QVERIFY(QTest::qWaitFor([&]() { return client.isConnected(); }, 1000));

    socket->write("#ROBOT_MONITOR seq=1 x=1 y=2 z=3 q1=1 q2=0 ");
    socket->flush();
    QTest::qWait(20);
    QCOMPARE(samples.size(), 0);

    socket->write("q3=0 q4=0 j1=1 j2=2 j3=3 j4=4 j5=5 j6=6#");
    socket->flush();

    QVERIFY(QTest::qWaitFor([&]() { return samples.size() == 1; }, 1000));
    QCOMPARE(rawMessages.size(), 1);
    QCOMPARE(samples.first().sequence, 1);
    QCOMPARE(errors.size(), 0);
}

void TestRobotMonitorClient::sendsHeartbeatMarker() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    RobotMonitorClient client;
    client.connectToRobot("127.0.0.1", server.serverPort());

    QVERIFY(QTest::qWaitFor([&]() { return server.hasPendingConnections(); }, 1000));
    QTcpSocket* socket = server.nextPendingConnection();
    QVERIFY(socket != nullptr);
    socket->setParent(&server);
    QVERIFY(QTest::qWaitFor([&]() { return client.isConnected(); }, 1000));

    QVERIFY(client.sendHeartbeat());
    QVERIFY(QTest::qWaitFor([&]() { return socket->bytesAvailable() > 0; }, 1000));
    QCOMPARE(QString::fromUtf8(socket->readAll()), QString("#ROBOT_MONITOR_CLIENT_ALIVE#"));
}

QTEST_MAIN(TestRobotMonitorClient)
#include "test_robot_monitor_client.moc"
