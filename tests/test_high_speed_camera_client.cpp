#include <QtTest/QtTest>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "camera/HighSpeedCameraClient.h"
#include "camera/IHighSpeedCameraClient.h"
#include "camera/HighSpeedCameraTypes.h"

class FakeHighSpeedCameraClient final : public IHighSpeedCameraClient {
public:
    bool load() override { return true; }
    bool loaded() const override { return true; }
    QStringList searchedDllPaths() const override { return {"fake/MvCameraControl.dll"}; }
    QVector<HighSpeedCameraDevice> enumerateDevices() override { return {}; }
    bool open(int) override { return true; }
    bool isOpen() const override { return true; }
    bool startGrabbing() override { return true; }
    bool readFrame(HighSpeedCameraFrame&, int) override { return false; }
    void stopGrabbing() override {}
    void close() override {}
    bool readParameters(HighSpeedCameraParameters&) override { return false; }
    bool writeParameters(const HighSpeedCameraParameters&) override { return false; }
    QString lastError() const override { return "fake"; }
};

class TestHighSpeedCameraClient final : public QObject {
    Q_OBJECT

private slots:
    void deviceDisplayTextMatchesPythonStyle();
    void failureCodeFormatsAsHex();
    void interfaceCanBeImplementedByFakeClient();
    void missingDllReportsSearchedPaths();
    void invalidExistingDllDoesNotHideSearchedPaths();
    void bundledMvsDllLoadsWhenPresent();
};

void TestHighSpeedCameraClient::deviceDisplayTextMatchesPythonStyle() {
    HighSpeedCameraDevice gige;
    gige.index = 2;
    gige.transport = HighSpeedCameraTransport::GigE;
    gige.userName = "CamA";
    gige.endpoint = "192.168.0.88";

    HighSpeedCameraDevice usb;
    usb.index = 3;
    usb.transport = HighSpeedCameraTransport::Usb;
    usb.userName = "CamB";
    usb.endpoint = "ABC123";

    QCOMPARE(gige.displayText(), QString("[2]GigE: CamA(192.168.0.88)"));
    QCOMPARE(usb.displayText(), QString("[3]USB: CamB(ABC123)"));
}

void TestHighSpeedCameraClient::failureCodeFormatsAsHex() {
    QCOMPARE(highSpeedCameraErrorHex(0), QString("0x00000000"));
    QCOMPARE(highSpeedCameraErrorHex(-1), QString("0xFFFFFFFF"));
    QCOMPARE(highSpeedCameraErrorHex(0x80000001u), QString("0x80000001"));
}

void TestHighSpeedCameraClient::interfaceCanBeImplementedByFakeClient() {
    FakeHighSpeedCameraClient fake;
    IHighSpeedCameraClient& client = fake;

    QVERIFY(client.load());
    QVERIFY(client.loaded());
    QCOMPARE(client.searchedDllPaths(), QStringList({"fake/MvCameraControl.dll"}));
}

void TestHighSpeedCameraClient::missingDllReportsSearchedPaths() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    HighSpeedCameraConfig config;
    config.sdkDllPath = tempDir.filePath("explicit/MvCameraControl.dll");
    config.portableSdkDllPath = "portable/MvCameraControl.dll";
    config.installedSdkDllPath = tempDir.filePath("installed/MvCameraControl.dll");

    HighSpeedCameraClient client(config, tempDir.path());

    QVERIFY(!client.load());
    QVERIFY(!client.loaded());
    QVERIFY(client.lastError().contains("Could not load high-speed camera SDK DLL"));
    const QStringList searchedPaths = client.searchedDllPaths();
    QCOMPARE(searchedPaths.size(), 3);
    QVERIFY(searchedPaths.at(0).contains("portable"));
    QVERIFY(searchedPaths.at(1).contains("explicit"));
    QVERIFY(searchedPaths.at(2).contains("installed"));
}

void TestHighSpeedCameraClient::invalidExistingDllDoesNotHideSearchedPaths() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QVERIFY(QDir(tempDir.path()).mkpath("portable"));

    QFile invalidDll(tempDir.filePath("portable/MvCameraControl.dll"));
    QVERIFY(invalidDll.open(QIODevice::WriteOnly));
    invalidDll.write("not a real camera dll");
    invalidDll.close();

    HighSpeedCameraConfig config;
    config.portableSdkDllPath = "portable/MvCameraControl.dll";
    config.sdkDllPath = tempDir.filePath("explicit/MvCameraControl.dll");
    config.installedSdkDllPath = tempDir.filePath("installed/MvCameraControl.dll");
    HighSpeedCameraClient client(config, tempDir.path());

    QVERIFY(!client.load());
    QVERIFY(client.lastError().contains("Could not load high-speed camera SDK DLL"));
    QVERIFY(client.lastError().contains("Attempted existing DLLs"));
    QVERIFY(client.lastError().contains("portable"));
    QVERIFY(client.lastError().contains("explicit"));
    QVERIFY(client.lastError().contains("installed"));
}

void TestHighSpeedCameraClient::bundledMvsDllLoadsWhenPresent() {
    const QString appDir = QCoreApplication::applicationDirPath();
    const QFileInfo dll(QDir(appDir).filePath("lib/sensors/high_speed_camera/MvCameraControl.dll"));
    if (!dll.exists()) {
        QSKIP("Bundled MVS runtime is not present in this build output.");
    }

    HighSpeedCameraConfig config;
    config.portableSdkDllPath = "lib/sensors/high_speed_camera/MvCameraControl.dll";
    HighSpeedCameraClient client(config, appDir);

    if (!client.load()) {
        const QString error = client.lastError();
        // A copied MvCameraControl.dll can still be unloadable when Hikrobot's
        // dependent runtime DLLs are not beside it or installed on the test PC.
        // That is an environment/deployment gap, not a unit-test failure in the
        // wrapper's search logic, so keep this optional smoke test non-blocking.
        if (error.contains("specified module could not be found", Qt::CaseInsensitive)) {
            QSKIP(qPrintable(QString("Bundled MVS DLL is present, but dependent MVS runtime DLLs are missing: %1").arg(error)));
        }
        QFAIL(qPrintable(error));
    }
    QVERIFY(client.loaded());
}

QTEST_GUILESS_MAIN(TestHighSpeedCameraClient)
#include "test_high_speed_camera_client.moc"
