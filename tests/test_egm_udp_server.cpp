#include <QtTest>

#include <QByteArray>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QThread>
#include <QUdpSocket>

#include <cstring>
#include <thread>

#include "egm/EgmUdpServer.h"
#include "egm/EgmWireCodec.h"

namespace {

// 由参考工程 Python egm_pb2 生成的黄金字节（EgmSensorPathCorr：
// seqno=7, tm=12345, mtype=MSGTYPE_PATH_CORRECTION, pos=(0,0,1.25), age=48）。
constexpr const char* kGoldenSensorHex =
    "0a07080710b9601804121f0a1b09000000000000000011000000000000000019000000000000f43f1030";

// EgmRobot（seqno=7, tm=336, MSGTYPE_DATA, feedBack cartesian=(500,10,100) +
// 四元数(1,0,0,0), MOTORS_ON, MCI_RUNNING）。
constexpr const char* kGoldenRobotHex =
    "0a07080710d0021802124512430a1b090000000000407f401100000000000024401900000000000059401224"
    "09000000000000f03f110000000000000000190000000000000000210000000000000000220208012a020803";

QByteArray goldenRobotBytes() {
    return QByteArray::fromHex(kGoldenRobotHex);
}

QString toHex(const std::string& bytes) {
    return QString::fromLatin1(QByteArray(bytes.data(), static_cast<int>(bytes.size())).toHex());
}

bool waitFor(const std::function<bool()>& predicate, int timeoutMs = 3000) {
    QElapsedTimer timer;
    timer.start();
    while (!predicate()) {
        if (timer.elapsed() > timeoutMs) {
            return false;
        }
        QThread::msleep(5);
    }
    return true;
}

struct RunningServer {
    egm::EgmUdpServer server;
    std::thread thread;
    bool runResult = false;
    QString error;

    explicit RunningServer(egm::EgmUdpServerConfig config,
                           egm::EgmUdpServer::CorrectionHandler handler)
        : server(config) {
        thread = std::thread([this, handler = std::move(handler)]() {
            runResult = server.run(handler, &error);
        });
    }

    ~RunningServer() {
        server.requestStop();
        if (thread.joinable()) {
            thread.join();
        }
    }
};

}  // namespace

class TestEgmUdpServer : public QObject {
    Q_OBJECT

private slots:
    void codecEncodesGoldenSensorBytes();
    void codecDecodesGoldenRobotFrame();
    void noReplyBeforeFirstRobotFrame();
    void repliesWithPathCorrectionMessage();
    void seqnoIncrementsOncePerFrame();
    void clampsCorrectionToHardLimit();
    void reportsLinkTimeoutAndStopsReplying();
};

void TestEgmUdpServer::codecEncodesGoldenSensorBytes() {
    // seqno / tm / pos.z / age 与 Python egm_pb2 完全一致。
    const std::string encoded = egm::encodeSensorPathCorr(7, 12345, 1.25, 48);
    QCOMPARE(toHex(encoded), QString::fromLatin1(kGoldenSensorHex));
}

void TestEgmUdpServer::codecDecodesGoldenRobotFrame() {
    const QByteArray datagram = goldenRobotBytes();
    egm::EgmRobotFrame frame;
    QVERIFY(egm::decodeEgmRobot(datagram.constData(), static_cast<std::size_t>(datagram.size()), &frame));

    QCOMPARE(frame.seqno, static_cast<quint32>(7));
    QCOMPARE(frame.controllerTimeMs, static_cast<quint32>(336));
    QVERIFY(frame.motorsOn);
    QCOMPARE(frame.mciState, 3);
    QVERIFY(frame.feedbackValid);
    QCOMPARE(frame.fbX, 500.0);
    QCOMPARE(frame.fbY, 10.0);
    QCOMPARE(frame.fbZ, 100.0);
    QCOMPARE(frame.fbQ1, 1.0);
    QCOMPARE(frame.fbQ2, 0.0);
    QCOMPARE(frame.fbQ3, 0.0);
    QCOMPARE(frame.fbQ4, 0.0);
}

void TestEgmUdpServer::noReplyBeforeFirstRobotFrame() {
    egm::EgmUdpServerConfig config;
    config.listenPort = 36510;
    RunningServer running(config, [](const egm::EgmRobotFrame&, double) { return 0.0; });

    QVERIFY(waitFor([&] { return running.server.status().listening; }));

    QUdpSocket client;
    QVERIFY(client.bind(QHostAddress::LocalHost, 0));

    QByteArray buffer;
    buffer.resize(256);
    // 控制器尚未发过任何 EgmRobot，服务端不得主动发送。
    QVERIFY(!client.waitForReadyRead(200));
    QCOMPARE(client.readDatagram(buffer.data(), buffer.size()), qint64(-1));
}

void TestEgmUdpServer::repliesWithPathCorrectionMessage() {
    egm::EgmUdpServerConfig config;
    config.listenPort = 36511;
    config.correctionAgeMs = 48;
    RunningServer running(config, [](const egm::EgmRobotFrame&, double) { return 1.25; });

    QVERIFY(waitFor([&] { return running.server.status().listening; }));

    QUdpSocket client;
    QVERIFY(client.bind(QHostAddress::LocalHost, 0));
    const QByteArray datagram = goldenRobotBytes();
    QCOMPARE(client.writeDatagram(datagram.constData(), datagram.size(), QHostAddress::LocalHost,
                                  config.listenPort),
             qint64(datagram.size()));

    QVERIFY(client.waitForReadyRead(2000));
    QByteArray buffer;
    buffer.resize(256);
    const qint64 received = client.readDatagram(buffer.data(), buffer.size());
    QVERIFY(received > 0);

    egm::SensorPathCorrFrame sensor;
    QVERIFY(egm::decodeSensorPathCorr(buffer.constData(), static_cast<std::size_t>(received), &sensor));
    QCOMPARE(sensor.seqno, static_cast<quint32>(0));
    QCOMPARE(sensor.mtype, static_cast<quint32>(4));
    QCOMPARE(sensor.ageMs, static_cast<quint32>(48));
    QCOMPARE(sensor.x, 0.0);
    QCOMPARE(sensor.y, 0.0);
    QCOMPARE(sensor.z, 1.25);

    // 服务端状态记录了最后收到的机器人序号。
    const egm::EgmUdpServerStatus status = running.server.status();
    QCOMPARE(status.lastRobotSeqno, static_cast<quint32>(7));
    QCOMPARE(status.framesReceived, static_cast<quint64>(1));
    QCOMPARE(status.framesSent, static_cast<quint64>(1));
    QVERIFY(status.lastRobot.feedbackValid);
    QCOMPARE(status.lastRobot.fbZ, 100.0);
}

void TestEgmUdpServer::seqnoIncrementsOncePerFrame() {
    egm::EgmUdpServerConfig config;
    config.listenPort = 36512;
    RunningServer running(config, [](const egm::EgmRobotFrame&, double) { return 0.0; });
    QVERIFY(waitFor([&] { return running.server.status().listening; }));

    QUdpSocket client;
    QVERIFY(client.bind(QHostAddress::LocalHost, 0));

    QByteArray buffer;
    buffer.resize(256);
    for (quint32 frame = 0; frame < 3; ++frame) {
        const QByteArray datagram = goldenRobotBytes();
        QCOMPARE(client.writeDatagram(datagram.constData(), datagram.size(),
                                      QHostAddress::LocalHost, config.listenPort),
                 qint64(datagram.size()));
        QVERIFY(client.waitForReadyRead(2000));
        const qint64 received = client.readDatagram(buffer.data(), buffer.size());
        QVERIFY(received > 0);
        egm::SensorPathCorrFrame sensor;
        QVERIFY(egm::decodeSensorPathCorr(buffer.constData(), static_cast<std::size_t>(received), &sensor));
        QCOMPARE(sensor.seqno, frame);
    }
    QCOMPARE(running.server.status().framesSent, static_cast<quint64>(3));
}

void TestEgmUdpServer::clampsCorrectionToHardLimit() {
    egm::EgmUdpServerConfig config;
    config.listenPort = 36513;
    config.hardZLimitMm = 3.0;
    RunningServer running(config, [](const egm::EgmRobotFrame&, double) { return 99.0; });
    QVERIFY(waitFor([&] { return running.server.status().listening; }));

    QUdpSocket client;
    QVERIFY(client.bind(QHostAddress::LocalHost, 0));
    const QByteArray datagram = goldenRobotBytes();
    QCOMPARE(client.writeDatagram(datagram.constData(), datagram.size(), QHostAddress::LocalHost,
                                  config.listenPort),
             qint64(datagram.size()));
    QVERIFY(client.waitForReadyRead(2000));
    QByteArray buffer;
    buffer.resize(256);
    const qint64 received = client.readDatagram(buffer.data(), buffer.size());
    QVERIFY(received > 0);
    egm::SensorPathCorrFrame sensor;
    QVERIFY(egm::decodeSensorPathCorr(buffer.constData(), static_cast<std::size_t>(received), &sensor));
    QCOMPARE(sensor.z, 3.0);
}

void TestEgmUdpServer::reportsLinkTimeoutAndStopsReplying() {
    egm::EgmUdpServerConfig config;
    config.listenPort = 36514;
    config.receiveTimeoutMs = 100;
    RunningServer running(config, [](const egm::EgmRobotFrame&, double) { return 0.5; });
    QVERIFY(waitFor([&] { return running.server.status().listening; }));

    QUdpSocket client;
    QVERIFY(client.bind(QHostAddress::LocalHost, 0));
    const QByteArray datagram = goldenRobotBytes();
    QCOMPARE(client.writeDatagram(datagram.constData(), datagram.size(), QHostAddress::LocalHost,
                                  config.listenPort),
             qint64(datagram.size()));
    QVERIFY(client.waitForReadyRead(2000));
    QByteArray buffer;
    buffer.resize(256);
    QVERIFY(client.readDatagram(buffer.data(), buffer.size()) > 0);

    // 之后不再发送 EgmRobot：服务端应标记链路超时，且不会继续回包。
    QVERIFY(waitFor([&] { return running.server.status().linkTimeout; }, 2000));
    QCOMPARE(running.server.status().framesSent, static_cast<quint64>(1));
    QVERIFY(!client.waitForReadyRead(200));
    QCOMPARE(running.server.status().framesSent, static_cast<quint64>(1));
}

QTEST_MAIN(TestEgmUdpServer)
#include "test_egm_udp_server.moc"
