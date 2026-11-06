#include <QtTest/QtTest>

#include <QDir>
#include <QImage>
#include <QLabel>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QWidget>

#include "services/thermal_camera/ThermalCameraService.h"
#include "thermal_camera/IThermalCameraClient.h"
#include "thermal_camera/ThermalFrameStore.h"
#include "ui_Interface4600_window.h"

class FakeThermalCameraClient final : public IThermalCameraClient {
public:
    bool startResult = true;
    int startCount = 0;
    int stopCount = 0;
    bool capturing = false;
    QString error = "fake thermal error";

    bool startCapture() override {
        ++startCount;
        capturing = startResult;
        return startResult;
    }

    void stopCapture() override {
        ++stopCount;
        capturing = false;
    }

    bool isCapturing() const override { return capturing; }

    QString lastError() const override { return error; }
};

class TestThermalCameraService final : public QObject {
    Q_OBJECT

private slots:
    void startCameraReportsClientFailure();
    void startCameraRejectsRepeatedStart();
    void clearCacheRemovesCachedFrame();
    void showDataDisplaysNewestFrame();
    void stopCameraStopsClientAndClearsCache();
};

namespace {

std::unique_ptr<FakeThermalCameraClient> makeClient(FakeThermalCameraClient*& rawClient) {
    auto client = std::make_unique<FakeThermalCameraClient>();
    rawClient = client.get();
    return client;
}

void writeJpeg(const QString& path, const QSize& size, const QColor& color) {
    QImage image(size, QImage::Format_RGB888);
    image.fill(color);
    QVERIFY2(image.save(path, "JPG"), qPrintable(QString("Could not save test JPEG %1").arg(path)));
}

} // namespace

void TestThermalCameraService::startCameraReportsClientFailure() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalFrameStore store(QDir(tempDir.path()), 60);
    FakeThermalCameraClient* client = nullptr;
    auto ownedClient = makeClient(client);
    client->startResult = false;
    ThermalCameraService service(&ui, store, std::move(ownedClient));
    QSignalSpy messages(&service, &ThermalCameraService::messageRequested);

    service.startCamera();

    QCOMPARE(client->startCount, 1);
    QCOMPARE(messages.count(), 1);
    QVERIFY(messages.at(0).at(1).toString().contains("fake thermal error"));
}

void TestThermalCameraService::startCameraRejectsRepeatedStart() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalFrameStore store(QDir(tempDir.path()), 60);
    FakeThermalCameraClient* client = nullptr;
    auto ownedClient = makeClient(client);
    client->capturing = true;
    ThermalCameraService service(&ui, store, std::move(ownedClient));
    QSignalSpy messages(&service, &ThermalCameraService::messageRequested);

    service.startCamera();

    QCOMPARE(client->startCount, 0);
    QCOMPARE(messages.count(), 1);
    QVERIFY(messages.at(0).at(1).toString().contains("already running"));
}

void TestThermalCameraService::clearCacheRemovesCachedFrame() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalFrameStore store(QDir(tempDir.path()), 60);
    QVERIFY(store.ensureDirectories());
    writeJpeg(store.cacheDir().filePath("frame.jpg"), QSize(2, 2), Qt::green);
    FakeThermalCameraClient* client = nullptr;
    ThermalCameraService service(&ui, store, makeClient(client));

    service.clearCache();

    QVERIFY(store.cacheDir().entryList({"*.jpg"}, QDir::Files).isEmpty());
}

void TestThermalCameraService::showDataDisplaysNewestFrame() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalFrameStore store(QDir(tempDir.path()), 60);
    QVERIFY(store.ensureDirectories());
    writeJpeg(store.cacheDir().filePath("frame.jpg"), QSize(8, 6), Qt::yellow);
    FakeThermalCameraClient* client = nullptr;
    ThermalCameraService service(&ui, store, makeClient(client));

    service.showData();
    service.refreshDisplayOnceForTest();

    QVERIFY(!ui.TIC_RD->pixmap().isNull());
    QVERIFY(!ui.TIC_PD->pixmap().isNull());
}

void TestThermalCameraService::stopCameraStopsClientAndClearsCache() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalFrameStore store(QDir(tempDir.path()), 60);
    QVERIFY(store.ensureDirectories());
    writeJpeg(store.cacheDir().filePath("frame.jpg"), QSize(2, 2), Qt::cyan);
    FakeThermalCameraClient* client = nullptr;
    ThermalCameraService service(&ui, store, makeClient(client));

    service.stopCamera();

    QCOMPARE(client->stopCount, 1);
    QVERIFY(store.cacheDir().entryList({"*.jpg"}, QDir::Files).isEmpty());
}

QTEST_MAIN(TestThermalCameraService)
#include "test_thermal_camera_service.moc"
