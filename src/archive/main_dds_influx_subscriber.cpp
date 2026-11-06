#include "app/AppConfig.h"
#include "app/AppPaths.h"
#include "app/StatusLogger.h"
#include "archive/DdsInfluxSubscriberApp.h"

#include <QCoreApplication>
#include <QDebug>
#include <QMetaObject>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#ifdef Q_OS_WIN
namespace {
// Converts Windows console shutdown notifications into a queued Qt quit
// request, allowing aboutToQuit() cleanup to flush pending archive rows.
BOOL WINAPI handleConsoleControl(DWORD controlType) {
    switch (controlType) {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        qInfo().noquote() << QString("DDS InfluxDB subscriber received console control event: %1").arg(controlType);
        QMetaObject::invokeMethod(qApp, "quit", Qt::QueuedConnection);
        return TRUE;
    default:
        return FALSE;
    }
}
}
#endif

// Entry point for the standalone DDS pressure subscriber that writes InfluxDB
// line protocol without starting the full AFP GUI application.
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    StatusLogger::install("dds_influx_subscriber.log");
    qInfo().noquote() << "DDS InfluxDB subscriber process starting.";

#ifdef Q_OS_WIN
    SetConsoleCtrlHandler(handleConsoleControl, TRUE);
#endif

    AppPaths paths(QCoreApplication::applicationDirPath());
    AppConfig config = AppConfig::load(paths);

    if (!config.isLoaded()) {
        qWarning().noquote() << "Application config was not loaded; startup will use built-in defaults.";
    }
    for (const QString& warning : config.warnings()) {
        qWarning().noquote() << warning;
    }

    DdsInfluxSubscriberApp subscriber(config, paths);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &subscriber, [&subscriber]() {
        qInfo().noquote() << "DDS InfluxDB subscriber aboutToQuit received.";
        subscriber.stop();
    });
    if (!subscriber.start()) {
        qCritical().noquote() << subscriber.lastError();
        return 1;
    }

    const int exitCode = app.exec();
    qInfo().noquote() << QString("DDS InfluxDB subscriber event loop exited with code %1.").arg(exitCode);
    return exitCode;
}
