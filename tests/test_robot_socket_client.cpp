#include <QtTest/QtTest>

#include "robot/RobotSocketClient.h"

#include <QElapsedTimer>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>

#include <atomic>
#include <thread>

class TestRobotSocketClient final : public QObject {
    Q_OBJECT

private slots:
    void loadModMessageMatchesPythonProtocol();
    void connectFailsWhenIpMissing();
    void connectFailsWhenPortInvalid();
    void connectFailsWhenPortTooLarge();
    void connectFailsWhenPeerIgnoresPing();
    void connectSucceedsWhenPeerRepliesPing();
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

// Open a QTcpServer that accepts TCP connections but never writes anything
// back — simulates an orphaned process (RobVC.exe) that happens to hold
// the port. connectToRobot must fail the CASE 0 ping handshake.
void TestRobotSocketClient::connectFailsWhenPeerIgnoresPing() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    RobotConfig config;
    config.ip = "127.0.0.1";
    config.portMotion = static_cast<int>(server.serverPort());

    RobotSocketClient client(config);
    const RobotSocketResult result = client.connectToRobot();

    QVERIFY(!result.ok);
    QVERIFY(result.message.contains("ping", Qt::CaseInsensitive));
}

// Peer thread that replies CASE 0 Ping with "0 1" — this is the happy path
// that proves SERVER.mod is actually reachable.
void TestRobotSocketClient::connectSucceedsWhenPeerRepliesPing() {
    std::atomic_bool listeningStarted{false};
    std::atomic<quint16> assignedPort{0};

    std::thread mock([&]() {
        QTcpServer s;
        if (!s.listen(QHostAddress::LocalHost, 0)) return;
        assignedPort.store(s.serverPort());
        listeningStarted.store(true);
        if (!s.waitForNewConnection(3000)) return;
        QTcpSocket* peer = s.nextPendingConnection();
        if (!peer) return;
        // Read ping, reply "0 1"
        peer->waitForReadyRead(3000);
        peer->readAll();
        peer->write("0 1");
        peer->waitForBytesWritten(1000);
        // Keep alive a moment
        QThread::msleep(500);
        peer->close();
        peer->deleteLater();
    });
    mock.detach();

    QElapsedTimer t;
    t.start();
    while (!listeningStarted.load() && t.elapsed() < 2000) QTest::qWait(20);
    QVERIFY(listeningStarted.load());

    RobotConfig config;
    config.ip = "127.0.0.1";
    config.portMotion = static_cast<int>(assignedPort.load());

    RobotSocketClient client(config);
    const RobotSocketResult result = client.connectToRobot();

    QVERIFY2(result.ok, qPrintable(result.message));
    QVERIFY(result.message.contains("ping OK", Qt::CaseInsensitive));
}

QTEST_MAIN(TestRobotSocketClient)
#include "test_robot_socket_client.moc"
