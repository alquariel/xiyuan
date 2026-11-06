#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTcpServer>
#include <QTcpSocket>

#include "app/AppConfig.h"
#include "app/AppPaths.h"
#include "robot_monitor/IRobotMonitorDdsPublisher.h"
#include "services/robot_monitor/RobotMonitoringService.h"

namespace {
QStringList* capturedLogMessages = nullptr;

void captureLogMessage(QtMsgType, const QMessageLogContext&, const QString& message) {
    if (capturedLogMessages != nullptr) {
        capturedLogMessages->append(message);
    }
}

class ScopedLogCapture final {
public:
    ScopedLogCapture()
        : previousHandler_(qInstallMessageHandler(captureLogMessage)) {
        capturedLogMessages = &messages_;
    }

    ~ScopedLogCapture() {
        capturedLogMessages = nullptr;
        qInstallMessageHandler(previousHandler_);
    }

    QStringList messages() const {
        return messages_;
    }

private:
    QStringList messages_;
    QtMessageHandler previousHandler_ = nullptr;
};

class FakeRobotMonitorDdsPublisher final : public IRobotMonitorDdsPublisher {
public:
    bool publish(const RobotMonitorSample& sample) override {
        publishedSamples.append(sample);
        return publishResult;
    }

    QString lastError() const override {
        return error;
    }

    QList<RobotMonitorSample> publishedSamples;
    bool publishResult = true;
    QString error;
};
}

// Covers the config-owned monitor service boundary. The service must never be
// started by the robot connection button, and it must record socket data when
// startup config enables the independent monitoring workflow.
class TestRobotMonitoringService final : public QObject {
    Q_OBJECT

private slots:
    void disabledServiceDoesNotConnect();
    void autostartRecordsIncomingSample();
    void autostartSendsHeartbeat();
    void publishesIncomingSampleToDds();
    void retriesUntilRobotMonitorServerStarts();
    void reportsConnectedButNoData();
    void writesStatusMessagesToQtLog();
};

void TestRobotMonitoringService::disabledServiceDoesNotConnect() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    AppConfig config;
    config.robot.ip = "127.0.0.1";
    config.robot.monitor.enabled = false;
    config.robot.monitor.autostart = true;
    config.robot.monitor.port = 9;
    config.robot.monitor.ddsPublishEnabled = false;

    RobotMonitoringService service(config, AppPaths(dir.path()));
    service.start();

    QVERIFY(!service.isRunning());
    QTest::qWait(50);
    QVERIFY(!service.isRunning());
}

void TestRobotMonitoringService::autostartRecordsIncomingSample() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    AppConfig config;
    config.robot.ip = "127.0.0.1";
    config.robot.monitor.enabled = true;
    config.robot.monitor.autostart = true;
    config.robot.monitor.port = server.serverPort();
    config.robot.monitor.recordingDir = "logs/robot_monitor";
    config.robot.monitor.recordCsv = true;
    config.robot.monitor.recordRawMessages = true;
    config.robot.monitor.flushEverySample = true;
    config.robot.monitor.ddsPublishEnabled = false;

    RobotMonitoringService service(config, AppPaths(dir.path()));
    service.start();

    QVERIFY(QTest::qWaitFor([&]() { return server.hasPendingConnections(); }, 1000));
    QTcpSocket* socket = server.nextPendingConnection();
    QVERIFY(socket != nullptr);
    socket->setParent(&server);

    socket->write(
        "#ROBOT_MONITOR seq=1 x=1 y=2 z=3 q1=1 q2=0 q3=0 q4=0 "
        "j1=1 j2=2 j3=3 j4=4 j5=5 j6=6#");
    socket->flush();

    const QString recordingDir = QDir(dir.path()).filePath("logs/robot_monitor");
    const bool recorded = QTest::qWaitFor([&]() {
        const QStringList csvFiles = QDir(recordingDir).entryList(QStringList() << "*.csv", QDir::Files);
        const QStringList rawFiles = QDir(recordingDir).entryList(QStringList() << "*.raw.log", QDir::Files);
        if (csvFiles.isEmpty() || rawFiles.isEmpty()) {
            return false;
        }

        QFile csv(QDir(recordingDir).filePath(csvFiles.first()));
        QFile raw(QDir(recordingDir).filePath(rawFiles.first()));
        if (!csv.open(QIODevice::ReadOnly | QIODevice::Text)
            || !raw.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }

        const QString csvContent = QString::fromUtf8(csv.readAll());
        const QString rawContent = QString::fromUtf8(raw.readAll());
        return csvContent.contains(",1,1,2,3,")
            && rawContent.contains("#ROBOT_MONITOR seq=1");
    }, 1000);
    QVERIFY2(recorded, qPrintable(QString("recordingDir=%1").arg(recordingDir)));

    const QString csvName = QDir(recordingDir).entryList(QStringList() << "*.csv", QDir::Files).first();
    QFile csv(QDir(recordingDir).filePath(csvName));
    QVERIFY(csv.open(QIODevice::ReadOnly | QIODevice::Text));
    const QString csvContent = QString::fromUtf8(csv.readAll());
    QVERIFY(csvContent.contains("received_at_utc,seq,x,y,z"));
    QVERIFY(csvContent.contains(",1,1,2,3,"));

    const QString rawName = QDir(recordingDir).entryList(QStringList() << "*.raw.log", QDir::Files).first();
    QFile raw(QDir(recordingDir).filePath(rawName));
    QVERIFY(raw.open(QIODevice::ReadOnly | QIODevice::Text));
    QVERIFY(QString::fromUtf8(raw.readAll()).contains("#ROBOT_MONITOR seq=1"));
}

void TestRobotMonitoringService::autostartSendsHeartbeat() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    AppConfig config;
    config.robot.ip = "127.0.0.1";
    config.robot.monitor.enabled = true;
    config.robot.monitor.autostart = true;
    config.robot.monitor.port = server.serverPort();
    config.robot.monitor.recordingDir = "logs/robot_monitor";
    config.robot.monitor.ddsPublishEnabled = false;

    RobotMonitoringService service(config, AppPaths(dir.path()));
    service.start();

    QVERIFY(QTest::qWaitFor([&]() { return server.hasPendingConnections(); }, 1000));
    QTcpSocket* socket = server.nextPendingConnection();
    QVERIFY(socket != nullptr);
    socket->setParent(&server);

    QByteArray receivedBytes;
    QVERIFY(QTest::qWaitFor([&]() {
        receivedBytes.append(socket->readAll());
        return QString::fromUtf8(receivedBytes).contains("#ROBOT_MONITOR_CLIENT_ALIVE#");
    }, 2000));
}

void TestRobotMonitoringService::retriesUntilRobotMonitorServerStarts() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    const quint16 port = server.serverPort();
    server.close();

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    AppConfig config;
    config.robot.ip = "127.0.0.1";
    config.robot.monitor.enabled = true;
    config.robot.monitor.autostart = true;
    config.robot.monitor.port = port;
    config.robot.monitor.recordingDir = "logs/robot_monitor";
    config.robot.monitor.recordCsv = true;
    config.robot.monitor.recordRawMessages = true;
    config.robot.monitor.flushEverySample = true;
    config.robot.monitor.ddsPublishEnabled = false;

    RobotMonitoringService service(config, AppPaths(dir.path()));
    service.start();

    QTest::qWait(300);
    QVERIFY(!service.isRunning());

    QVERIFY(server.listen(QHostAddress::LocalHost, port));
    QVERIFY(QTest::qWaitFor([&]() { return server.hasPendingConnections(); }, 3000));

    QTcpSocket* socket = server.nextPendingConnection();
    QVERIFY(socket != nullptr);
    socket->setParent(&server);
    socket->write("#ROBOT_MONITOR s=2 x=4 y=5 z=6 v=7#");
    socket->flush();

    const QString recordingDir = QDir(dir.path()).filePath("logs/robot_monitor");
    const bool recorded = QTest::qWaitFor([&]() {
        const QStringList csvFiles = QDir(recordingDir).entryList(QStringList() << "*.csv", QDir::Files);
        if (csvFiles.isEmpty()) {
            return false;
        }
        QFile csv(QDir(recordingDir).filePath(csvFiles.first()));
        if (!csv.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return false;
        }
        return QString::fromUtf8(csv.readAll()).contains(",2,4,5,6,");
    }, 1000);
    QVERIFY2(recorded, qPrintable(QString("recordingDir=%1").arg(recordingDir)));
}

void TestRobotMonitoringService::publishesIncomingSampleToDds() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    AppConfig config;
    config.robot.ip = "127.0.0.1";
    config.robot.monitor.enabled = true;
    config.robot.monitor.autostart = true;
    config.robot.monitor.port = server.serverPort();
    config.robot.monitor.recordingDir = "logs/robot_monitor";
    config.robot.monitor.recordCsv = false;
    config.robot.monitor.recordRawMessages = false;
    config.robot.monitor.ddsPublishEnabled = true;

    auto fakePublisher = std::make_unique<FakeRobotMonitorDdsPublisher>();
    FakeRobotMonitorDdsPublisher* fakePublisherPtr = fakePublisher.get();
    RobotMonitoringService service(config, AppPaths(dir.path()), nullptr, std::move(fakePublisher));
    service.start();

    QVERIFY(QTest::qWaitFor([&]() { return server.hasPendingConnections(); }, 1000));
    QTcpSocket* socket = server.nextPendingConnection();
    QVERIFY(socket != nullptr);
    socket->setParent(&server);

    socket->write("#ROBOT_MONITOR seq=9 x=10 y=20 z=30 q1=1 q2=0 q3=0 q4=0#");
    socket->flush();

    QVERIFY(QTest::qWaitFor([&]() { return fakePublisherPtr->publishedSamples.size() == 1; }, 1000));
    const RobotMonitorSample sample = fakePublisherPtr->publishedSamples.first();
    QCOMPARE(sample.sequence, 9);
    QCOMPARE(sample.x, 10.0);
    QCOMPARE(sample.y, 20.0);
    QCOMPARE(sample.z, 30.0);
    QCOMPARE(sample.estimatedTcpSpeed, 0.0);
}

void TestRobotMonitoringService::reportsConnectedButNoData() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    AppConfig config;
    config.robot.ip = "127.0.0.1";
    config.robot.monitor.enabled = true;
    config.robot.monitor.autostart = true;
    config.robot.monitor.port = server.serverPort();
    config.robot.monitor.recordingDir = "logs/robot_monitor";
    config.robot.monitor.ddsPublishEnabled = false;

    RobotMonitoringService service(config, AppPaths(dir.path()));
    QStringList statuses;
    connect(&service, &RobotMonitoringService::statusChanged, this, [&statuses](const QString& status) {
        statuses.append(status);
    });

    service.start();

    QVERIFY(QTest::qWaitFor([&]() { return server.hasPendingConnections(); }, 1000));
    QTcpSocket* socket = server.nextPendingConnection();
    QVERIFY(socket != nullptr);
    socket->setParent(&server);

    QVERIFY(QTest::qWaitFor([&]() {
        return statuses.join('\n').contains("no monitor data received");
    }, 4000));
}

void TestRobotMonitoringService::writesStatusMessagesToQtLog() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    AppConfig config;
    config.robot.ip = "";
    config.robot.monitor.enabled = true;
    config.robot.monitor.autostart = true;
    config.robot.monitor.ddsPublishEnabled = false;

    ScopedLogCapture capture;
    RobotMonitoringService service(config, AppPaths(dir.path()));
    service.start();

    QVERIFY(capture.messages().join('\n').contains("Robot monitor disabled: robot.ip is empty."));
}

QTEST_MAIN(TestRobotMonitoringService)
#include "test_robot_monitoring_service.moc"
