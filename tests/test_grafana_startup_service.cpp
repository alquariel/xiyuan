#include <QtTest/QtTest>

#include <QCoreApplication>
#include <QDir>
#include <QTcpServer>
#include <QTcpSocket>

#include "app/AppConfig.h"
#include "app/AppPaths.h"
#include "services/grafana/GrafanaStartupService.h"

class TestGrafanaStartupService : public QObject {
    Q_OBJECT

private slots:
    void disabledConfigDoesNotStart();
    void missingGrafanaExecutableReportsErrorLater();
    void reachableGrafanaHostDoesNotStartAnotherServer();
    void runningConfiguredProcessDoesNotStartAnotherServer();
};

void TestGrafanaStartupService::disabledConfigDoesNotStart() {
    AppConfig config;
    config.grafana.enabled = false;
    GrafanaStartupService service(config.grafana, AppPaths(QDir::tempPath()));

    QVERIFY(!service.start());
    QVERIFY(service.lastStatus().contains("disabled", Qt::CaseInsensitive));
}

void TestGrafanaStartupService::missingGrafanaExecutableReportsErrorLater() {
    AppConfig config;
    config.grafana.enabled = true;
    config.grafana.autostart = true;
    config.grafana.exePath = "Z:/missing/definitely_missing_grafana_test.exe";
    config.grafana.hostUrl = "http://127.0.0.1:9";
    config.grafana.startupTimeoutMs = 100;

    GrafanaStartupService service(config.grafana, AppPaths(QDir::tempPath()));

    QElapsedTimer elapsed;
    elapsed.start();
    QVERIFY(service.start());
    QVERIFY(elapsed.elapsed() < 100);
    QVERIFY(service.lastStatus().contains("queued", Qt::CaseInsensitive));
    QTRY_VERIFY_WITH_TIMEOUT(service.lastStatus().contains("Grafana executable"), 2000);
    QVERIFY(service.lastStatus().contains("Grafana executable"));
    QVERIFY(service.lastStatus().contains(config.grafana.exePath));
}

void TestGrafanaStartupService::reachableGrafanaHostDoesNotStartAnotherServer() {
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

    AppConfig config;
    config.grafana.enabled = true;
    config.grafana.autostart = true;
    config.grafana.exePath = "Z:/missing/definitely_missing_grafana_test.exe";
    config.grafana.hostUrl = QString("http://127.0.0.1:%1").arg(server.serverPort());
    config.grafana.startupTimeoutMs = 200;

    GrafanaStartupService service(config.grafana, AppPaths(QDir::tempPath()));

    QVERIFY(service.start());
    QTRY_VERIFY_WITH_TIMEOUT(service.lastStatus().contains("already reachable", Qt::CaseInsensitive), 2000);
    QVERIFY(!service.lastStatus().contains("executable", Qt::CaseInsensitive));
}

void TestGrafanaStartupService::runningConfiguredProcessDoesNotStartAnotherServer() {
#ifndef Q_OS_WIN
    QSKIP("Process-table duplicate Grafana detection is only implemented on Windows.");
#else
    AppConfig config;
    config.grafana.enabled = true;
    config.grafana.autostart = true;
    config.grafana.exePath = QCoreApplication::applicationFilePath();
    config.grafana.hostUrl = "http://127.0.0.1:9";
    config.grafana.startupTimeoutMs = 100;

    GrafanaStartupService service(config.grafana, AppPaths(QDir::tempPath()));

    QVERIFY(service.start());
    QVERIFY(service.lastStatus().contains("already running", Qt::CaseInsensitive));
    QVERIFY(!service.lastStatus().contains("executable does not exist", Qt::CaseInsensitive));
#endif
}

QTEST_MAIN(TestGrafanaStartupService)
#include "test_grafana_startup_service.moc"
