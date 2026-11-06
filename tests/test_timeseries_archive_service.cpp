#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "app/AppConfig.h"
#include "app/AppPaths.h"
#include "services/archive/TimeseriesArchiveService.h"

class TestTimeseriesArchiveService : public QObject {
    Q_OBJECT

private slots:
    void disabledConfigDoesNotStart();
    void missingInfluxExecutableReportsError();
    void startupIsQueuedAndReportsMissingInfluxLater();
    void transientInfluxCliFailureIsRetried();
    void ownedInfluxdbProcessTreeStopsWhenServiceIsDestroyed();
    void reachableExistingInfluxdbProcessTreeStopsWhenServiceIsDestroyed();
    void subscriberExitIsReported();
};

namespace {
bool isProcessRunning(qint64 processId) {
#ifdef Q_OS_WIN
    const HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(processId));
    if (process == nullptr) {
        return false;
    }
    const bool running = WaitForSingleObject(process, 0) == WAIT_TIMEOUT;
    CloseHandle(process);
    return running;
#else
    Q_UNUSED(processId);
    return false;
#endif
}

qint64 readPidFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return 0;
    }
    return QString::fromUtf8(file.readAll()).trimmed().toLongLong();
}
}

void TestTimeseriesArchiveService::disabledConfigDoesNotStart() {
    AppConfig config;
    config.timeseriesArchive.enabled = false;
    TimeseriesArchiveService service(config, AppPaths(QDir::tempPath()));

    QVERIFY(!service.start());
    QVERIFY(service.lastStatus().contains("disabled", Qt::CaseInsensitive));
}

void TestTimeseriesArchiveService::missingInfluxExecutableReportsError() {
    AppConfig config;
    config.timeseriesArchive.enabled = true;
    config.timeseriesArchive.startInfluxdb = true;
    config.timeseriesArchive.hostUrl = "http://127.0.0.1:9";
    config.timeseriesArchive.influxdbExePath = "Z:/missing/influxdb3.exe";
    config.timeseriesArchive.ensureDatabaseOnStart = false;
    config.timeseriesArchive.subscriberAutostart = false;

    TimeseriesArchiveService service(config, AppPaths(QDir::tempPath()));

    QSignalSpy spy(&service, &TimeseriesArchiveService::statusChanged);

    QVERIFY(service.start());
    QVERIFY(service.lastStatus().contains("queued", Qt::CaseInsensitive));
    QVERIFY(spy.wait(2000));
    QVERIFY(service.lastStatus().contains("InfluxDB executable"));
    QVERIFY(service.lastStatus().contains(config.timeseriesArchive.influxdbExePath));
}

void TestTimeseriesArchiveService::startupIsQueuedAndReportsMissingInfluxLater() {
    AppConfig config;
    config.timeseriesArchive.enabled = true;
    config.timeseriesArchive.startInfluxdb = true;
    config.timeseriesArchive.hostUrl = "http://127.0.0.1:9";
    config.timeseriesArchive.influxdbExePath = "Z:/missing/influxdb3.exe";
    config.timeseriesArchive.ensureDatabaseOnStart = false;
    config.timeseriesArchive.subscriberAutostart = false;

    TimeseriesArchiveService service(config, AppPaths(QDir::tempPath()));

    QElapsedTimer elapsed;
    elapsed.start();
    QVERIFY(service.start());
    QVERIFY(elapsed.elapsed() < 100);
    QVERIFY(!service.lastStatus().contains("InfluxDB executable"));
}

void TestTimeseriesArchiveService::transientInfluxCliFailureIsRetried() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QFile tokenFile(tempDir.filePath("afp-admin-token.json"));
    QVERIFY(tokenFile.open(QIODevice::WriteOnly | QIODevice::Text));
    QVERIFY(tokenFile.write(R"({"token":"test-token"})") > 0);
    tokenFile.close();

    const QString failingCli = QStandardPaths::findExecutable("where.exe");
    if (failingCli.isEmpty()) {
        QSKIP("where.exe is required for this Windows-specific CLI retry test.");
    }

    AppConfig config;
    config.timeseriesArchive.enabled = true;
    config.timeseriesArchive.startInfluxdb = false;
    config.timeseriesArchive.influxdbExePath = failingCli;
    config.timeseriesArchive.adminTokenFile = tokenFile.fileName();
    config.timeseriesArchive.ensureDatabaseOnStart = true;
    config.timeseriesArchive.subscriberAutostart = false;
    config.timeseriesArchive.cliTimeoutMs = 200;
    config.timeseriesArchive.influxdbStartupTimeoutMs = 1000;

    TimeseriesArchiveService service(config, AppPaths(QDir::tempPath()));

    QVERIFY(service.start());
    QTRY_VERIFY_WITH_TIMEOUT(service.lastStatus().contains("retrying", Qt::CaseInsensitive), 2000);
    QVERIFY(!service.lastStatus().contains("subscriber started", Qt::CaseInsensitive));
}

void TestTimeseriesArchiveService::ownedInfluxdbProcessTreeStopsWhenServiceIsDestroyed() {
#ifndef Q_OS_WIN
    QSKIP("Owned InfluxDB process-tree cleanup is Windows-specific.");
#else
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString childPidFile = tempDir.filePath("child.pid");

    AppConfig config;
    config.timeseriesArchive.enabled = true;
    config.timeseriesArchive.startInfluxdb = true;
    config.timeseriesArchive.influxdbExePath = QString::fromUtf8(TEST_HELPER_LONG_RUNNING_PROCESS);
    config.timeseriesArchive.hostUrl = "http://127.0.0.1:9";
    config.timeseriesArchive.dataDir = childPidFile;
    config.timeseriesArchive.ensureDatabaseOnStart = false;
    config.timeseriesArchive.subscriberAutostart = false;
    config.timeseriesArchive.reachabilityProbeTimeoutMs = 100;
    config.timeseriesArchive.influxdbStartupTimeoutMs = 3000;

    qint64 childProcessId = 0;
    {
        TimeseriesArchiveService service(config, AppPaths(QDir::tempPath()));
        QVERIFY(service.start());
        QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(childPidFile), 3000);
        childProcessId = readPidFile(childPidFile);
        QVERIFY(childProcessId > 0);
        QVERIFY(isProcessRunning(childProcessId));
    }

    QTRY_VERIFY_WITH_TIMEOUT(!isProcessRunning(childProcessId), 3000);
#endif
}

void TestTimeseriesArchiveService::reachableExistingInfluxdbProcessTreeStopsWhenServiceIsDestroyed() {
#ifndef Q_OS_WIN
    QSKIP("Existing InfluxDB process adoption is Windows-specific.");
#else
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString childPidFile = tempDir.filePath("existing-child.pid");

    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost));
    connect(&server, &QTcpServer::newConnection, this, [&server]() {
        QTcpSocket* socket = server.nextPendingConnection();
        socket->setParent(&server);
        connect(socket, &QTcpSocket::readyRead, socket, [socket]() {
            socket->readAll();
            socket->write("HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 2\r\n\r\nOK");
            socket->flush();
            socket->disconnectFromHost();
        });
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    });

    QProcess existingInfluxdb;
    existingInfluxdb.setProgram(QString::fromUtf8(TEST_HELPER_LONG_RUNNING_PROCESS));
    existingInfluxdb.setArguments({"serve", "--data-dir", childPidFile});
    existingInfluxdb.start();
    QVERIFY(existingInfluxdb.waitForStarted(2000));
    QTRY_VERIFY_WITH_TIMEOUT(QFileInfo::exists(childPidFile), 3000);
    const qint64 childProcessId = readPidFile(childPidFile);
    QVERIFY(childProcessId > 0);
    QVERIFY(isProcessRunning(childProcessId));

    AppConfig config;
    config.timeseriesArchive.enabled = true;
    config.timeseriesArchive.startInfluxdb = true;
    config.timeseriesArchive.stopInfluxdbOnExit = true;
    config.timeseriesArchive.influxdbExePath = QString::fromUtf8(TEST_HELPER_LONG_RUNNING_PROCESS);
    config.timeseriesArchive.hostUrl = QString("http://127.0.0.1:%1").arg(server.serverPort());
    config.timeseriesArchive.ensureDatabaseOnStart = false;
    config.timeseriesArchive.subscriberAutostart = false;
    config.timeseriesArchive.reachabilityProbeTimeoutMs = 100;

    {
        TimeseriesArchiveService service(config, AppPaths(QDir::tempPath()));
        QVERIFY(service.start());
        QTRY_VERIFY_WITH_TIMEOUT(service.lastStatus().contains("disabled", Qt::CaseInsensitive), 2000);
    }

    QTRY_VERIFY_WITH_TIMEOUT(!isProcessRunning(childProcessId), 3000);
    if (existingInfluxdb.state() != QProcess::NotRunning) {
        existingInfluxdb.kill();
        existingInfluxdb.waitForFinished(1000);
    }
#endif
}

void TestTimeseriesArchiveService::subscriberExitIsReported() {
    AppConfig config;
    config.timeseriesArchive.enabled = true;
    config.timeseriesArchive.startInfluxdb = false;
    config.timeseriesArchive.ensureDatabaseOnStart = false;
    config.timeseriesArchive.subscriberAutostart = true;
    config.timeseriesArchive.subscriberExecutable = QString::fromUtf8(TEST_HELPER_EXIT_PROCESS);
    config.timeseriesArchive.subscriberShowConsole = false;
    config.pressureArchive.enabled = true;

    TimeseriesArchiveService service(config, AppPaths(QDir::tempPath()));

    QVERIFY(service.start());
    QTRY_VERIFY_WITH_TIMEOUT(service.lastStatus().contains("exited", Qt::CaseInsensitive), 2000);
    QVERIFY(service.lastStatus().contains("exitCode=7"));
}

QTEST_MAIN(TestTimeseriesArchiveService)
#include "test_timeseries_archive_service.moc"
