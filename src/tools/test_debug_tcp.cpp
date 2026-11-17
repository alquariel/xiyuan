#include <QApplication>
#include "pressure/DebugPressureTcp.h"
#include <QDebug>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    qInfo() << "[test] Creating DebugPressureTcp on 9100...";
    DebugPressureTcp server(9100);

    QObject::connect(&server, &DebugPressureTcp::logMessage,
                     [](const QString& m) { qInfo() << "[tcp-log]" << m; });
    QObject::connect(&server, &DebugPressureTcp::listeningStateChanged,
                     [](bool b) { qInfo() << "[tcp-state]" << b; });
    QObject::connect(&server, &DebugPressureTcp::pressureSampleReady,
                     [](double v) { qInfo() << "[tcp-pressure]" << v; });

    qInfo() << "[test] Calling start()...";
    bool ok = server.start();
    qInfo() << "[test] start() returned" << ok << "boundPort =" << server.boundPort();

    if (!ok) {
        qInfo() << "[test] FAIL - server did not start";
        return 1;
    }

    qInfo() << "[test] Server running, waiting 30s for connections...";
    return app.exec();
}
