#include <QtTest/QtTest>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryDir>

#include "pressure/PressureSensorClient.h"

class TestPressureSensorClient final : public QObject {
    Q_OBJECT

private slots:
    void reportsMissingDll();
    void readFailsBeforeStart();
    void loadsBundledPressureDllWithAppWorkingDirectory();
};

void TestPressureSensorClient::reportsMissingDll() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    PressureSensorClient client(tempDir.filePath("missing-pressure.dll"));

    QVERIFY(!client.load());
    QVERIFY(!client.lastError().isEmpty());
    QVERIFY(client.lastError().contains("pressure", Qt::CaseInsensitive));
    QVERIFY(!client.loaded());
    QVERIFY(!client.running());
}

void TestPressureSensorClient::readFailsBeforeStart() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    PressureSensorClient client(tempDir.filePath("missing-pressure.dll"));
    double value = 0.0;

    QVERIFY(!client.read(value));
    QVERIFY(client.lastError().contains("not running", Qt::CaseInsensitive));
}

void TestPressureSensorClient::loadsBundledPressureDllWithAppWorkingDirectory() {
    const QDir appDir(QCoreApplication::applicationDirPath());
    const QFileInfo dll(appDir.filePath("lib/sensors/pressure_sensor/pressure.dll"));
    if (!dll.exists()) {
        QSKIP("Bundled pressure sensor runtime is not present in this build output.");
    }

    QVERIFY(QDir::setCurrent(appDir.absolutePath()));

    PressureSensorClient client(dll.absoluteFilePath());

    QVERIFY2(client.load(), qPrintable(client.lastError()));
    QVERIFY(client.loaded());
    QVERIFY(!client.running());
}

QTEST_MAIN(TestPressureSensorClient)
#include "test_pressure_sensor_client.moc"
