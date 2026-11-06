#include <QtTest/QtTest>

#include "app/AppConfig.h"
#include "plc/PlcClient.h"

class TestPlcClientState final : public QObject {
    Q_OBJECT

private slots:
    void newClientStartsDisconnected();
    void plcClientImplementsByteAccessInterface();
    void plcClientImplementsProcessAccessInterface();
};

void TestPlcClientState::newClientStartsDisconnected() {
    PlcConfig config;
    config.ip = "192.168.0.201";
    config.rack = 0;
    config.slot = 1;

    // This test must not contact real hardware. A new wrapper should be inert
    // until a UI action asks it to connect.
    PlcClient client(config);

    QVERIFY(!client.isConnected());
    QVERIFY(client.lastError().isEmpty());
}

void TestPlcClientState::plcClientImplementsByteAccessInterface() {
    PlcConfig config;
    config.ip = "192.168.0.201";
    config.rack = 0;
    config.slot = 1;

    PlcClient client(config);
    IPlcByteAccess* byteAccess = &client;

    QVERIFY(byteAccess != nullptr);
    QVERIFY(!client.isConnected());
}

void TestPlcClientState::plcClientImplementsProcessAccessInterface() {
    PlcConfig config;
    PlcClient client(config);
    IPlcProcessAccess* processAccess = &client;

    QVERIFY(processAccess != nullptr);
    QVERIFY(!client.isConnected());
}

QTEST_MAIN(TestPlcClientState)
#include "test_plc_client_state.moc"
