#include <QtTest/QtTest>

#include <QTemporaryDir>

#include "app/AppConfig.h"
#include "app/AppPaths.h"
#include "thermal_camera/ThermalCameraClient.h"
#include "thermal_camera/ThermalFrameStore.h"
#include "thermal_camera/ThermalCameraTypes.h"

class TestThermalCameraClient final : public QObject {
    Q_OBJECT

private slots:
    void errorCodeFormatsAsDecimalAndHex();
    void missingRuntimeFolderReportsUsefulError();
    void stopCaptureBeforeStartIsSafe();
};

void TestThermalCameraClient::errorCodeFormatsAsDecimalAndHex() {
    QCOMPARE(thermalCameraErrorText(7), QString("7 (0x00000007)"));
}

void TestThermalCameraClient::missingRuntimeFolderReportsUsefulError() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    ThermalCameraConfig config;
    config.sdkDir.clear();
    config.portableSdkDir = "missing/thermal/runtime";

    ThermalFrameStore store(QDir(tempDir.path()), config.cacheMaxFiles);
    ThermalCameraClient client(config, AppPaths(tempDir.path()), store);

    QVERIFY(!client.startCapture());
    QVERIFY(!client.lastError().isEmpty());
    QVERIFY(client.lastError().contains("HCNetSDK.dll"));
    QVERIFY(client.lastError().contains("PlayCtrl.dll"));
}

void TestThermalCameraClient::stopCaptureBeforeStartIsSafe() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    ThermalCameraConfig config;
    ThermalFrameStore store(QDir(tempDir.path()), config.cacheMaxFiles);
    ThermalCameraClient client(config, AppPaths(tempDir.path()), store);

    client.stopCapture();

    QVERIFY(!client.isCapturing());
}

QTEST_GUILESS_MAIN(TestThermalCameraClient)
#include "test_thermal_camera_client.moc"
