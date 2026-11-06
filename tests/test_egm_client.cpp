#include <QtTest/QtTest>

#include "robot/EgmClient.h"

#include <QByteArray>
#include <QHostAddress>
#include <QNetworkDatagram>
#include <QUdpSocket>

#include <cstdint>
#include <cstring>

namespace {

// EGM 走 protobuf：控制器先发 EgmRobot，客户端才回 EgmSensor。
// 这里只做结构性校验 —— 在应答报文里找 pos.x 的 8 字节小端编码。
bool ContainsLittleEndianDouble(const QByteArray& data, double value) {
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));

    char needle[8];
    for (int i = 0; i < 8; ++i) {
        needle[i] = static_cast<char>(static_cast<std::uint8_t>(bits >> (8 * i)));
    }
    return data.indexOf(QByteArray(needle, 8)) >= 0;
}

quint16 PickFreeUdpPort() {
    QUdpSocket probe;
    probe.bind(QHostAddress::LocalHost, 0);
    const quint16 port = probe.localPort();
    probe.close();
    return port;
}

}  // namespace

class TestEgmClient final : public QObject {
    Q_OBJECT

private slots:
    void disabledConfigDoesNotStartStreaming();
    void noPacketsBeforeRobotContact();
    void repliesOnlyAfterRobotContact();
    void stopThreadIsGraceful();
};

void TestEgmClient::disabledConfigDoesNotStartStreaming() {
    EgmConfig cfg;
    cfg.enabled = false;
    EgmClient client(cfg);
    QVERIFY(!client.start());
    QVERIFY(!client.isStreaming());
}

void TestEgmClient::noPacketsBeforeRobotContact() {
    const quint16 port = PickFreeUdpPort();

    EgmConfig cfg;
    cfg.enabled = true;
    cfg.port = static_cast<int>(port);
    cfg.periodMs = 2;

    QUdpSocket robot;
    QVERIFY(robot.bind(QHostAddress::LocalHost, 0));

    EgmClient client(cfg);
    client.setTargetPose(EgmTcpPose{1.0, 2.0, 3.0, 1.0, 0.0, 0.0, 0.0});
    QVERIFY(client.start());
    // 绑定发生在收发线程里，等它真正完成再继续
    QTRY_VERIFY_WITH_TIMEOUT(client.boundPort() == port, 2000);

    // 控制器还没发包时，客户端不应该主动发任何东西
    QVERIFY(!robot.waitForReadyRead(300));
    QVERIFY(!client.isPeerConnected());

    client.stop();
}

void TestEgmClient::repliesOnlyAfterRobotContact() {
    const quint16 port = PickFreeUdpPort();

    EgmConfig cfg;
    cfg.enabled = true;
    cfg.port = static_cast<int>(port);
    cfg.periodMs = 2;

    QUdpSocket robot;
    QVERIFY(robot.bind(QHostAddress::LocalHost, 0));

    EgmClient client(cfg);
    client.setTargetPose(EgmTcpPose{123.5, -45.25, 880.125, 1.0, 0.0, 0.0, 0.0});
    QVERIFY(client.start());
    QTRY_VERIFY_WITH_TIMEOUT(client.boundPort() == port, 2000);

    // 模拟控制器发出的 EgmRobot（内容随意，客户端只取源地址）
    QVERIFY(robot.writeDatagram(QByteArrayLiteral("egm-robot-feedback"),
                                QHostAddress::LocalHost, port) > 0);

    QVERIFY(robot.waitForReadyRead(2000));
    const QByteArray reply = robot.receiveDatagram().data();

    QVERIFY(client.isPeerConnected());
    QVERIFY2(!reply.isEmpty(), "客户端应在收到 EgmRobot 后回发 EgmSensor");
    QVERIFY2(ContainsLittleEndianDouble(reply, 123.5), "应答里应包含 pos.x");
    QVERIFY2(ContainsLittleEndianDouble(reply, -45.25), "应答里应包含 pos.y");
    QVERIFY2(ContainsLittleEndianDouble(reply, 880.125), "应答里应包含 pos.z");

    client.stop();
    QVERIFY(!client.isPeerConnected());
}

void TestEgmClient::stopThreadIsGraceful() {
    EgmConfig cfg;
    cfg.enabled = true;
    cfg.port = 43210;
    cfg.periodMs = 2;
    EgmClient client(cfg);

    QVERIFY(client.start());
    QVERIFY(client.isStreaming());
    client.stop();
    QVERIFY(!client.isStreaming());

    QVERIFY(client.start());
    QVERIFY(client.isStreaming());
    client.stop();
    QVERIFY(!client.isStreaming());
}

QTEST_MAIN(TestEgmClient)
#include "test_egm_client.moc"
