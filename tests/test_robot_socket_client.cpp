#include <QtTest/QtTest>

#include "robot/RobotSocketClient.h"

#include <QElapsedTimer>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>

class TestRobotSocketClient final : public QObject {
    Q_OBJECT

private slots:
    void loadModMessageMatchesPythonProtocol();
    void connectFailsWhenIpMissing();
    void connectFailsWhenPortInvalid();
    void connectFailsWhenPortTooLarge();
    void sendsExpectedBytesToLocalSocket();
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

void TestRobotSocketClient::connectFailsWhenPortTooLarge() {
    RobotConfig config;
    config.ip = "127.0.0.1";
    config.portMotion = 70000;

    RobotSocketClient client(config);
    const RobotSocketResult result = client.connectToRobot();

    QVERIFY(!result.ok);
    QVERIFY(result.message.contains("port", Qt::CaseInsensitive));
}

void TestRobotSocketClient::sendsExpectedBytesToLocalSocket() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    RobotConfig config;
    config.ip = "127.0.0.1";
    config.portMotion = static_cast<int>(server.serverPort());

    RobotSocketClient client(config);
    const RobotSocketResult connectResult = client.connectToRobot();

    QVERIFY2(connectResult.ok, qPrintable(connectResult.message));
    QVERIFY(server.waitForNewConnection(1000));

    QTcpSocket* acceptedSocket = server.nextPendingConnection();
    QVERIFY(acceptedSocket != nullptr);

    QVERIFY(client.sendLoadMod("path_a.mod").ok);
    QVERIFY(client.sendPause().ok);
    QVERIFY(client.sendContinue().ok);
    QVERIFY(client.sendStop().ok);

    const QByteArray expected = "LOAD_MOD:path_a.mod#PAUSE#CONTINUE#STOP#";
    QByteArray received;
    QElapsedTimer timer;
    timer.start();

    while (received.size() < expected.size() && timer.elapsed() < 1000) {
        if (acceptedSocket->bytesAvailable() == 0) {
            acceptedSocket->waitForReadyRead(50);
        }
        received += acceptedSocket->readAll();
    }

    QCOMPARE(received, expected);
}

QTEST_MAIN(TestRobotSocketClient)
#include "test_robot_socket_client.moc"
