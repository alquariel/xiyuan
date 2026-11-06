#include <QtTest/QtTest>

#include <QComboBox>
#include <QCoreApplication>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QSignalSpy>
#include <QThread>
#include <QWidget>

#include "camera/IHighSpeedCameraClient.h"
#include "services/camera/HighSpeedCameraService.h"
#include "ui_Interface4600_window.h"

class FakeHighSpeedCameraClient final : public IHighSpeedCameraClient {
public:
    bool loadResult = true;
    bool loadedState = false;
    QVector<HighSpeedCameraDevice> devices;
    int openCallCount = 0;
    int openedDeviceIndex = -1;
    bool openResult = true;
    bool openState = false;
    bool startGrabbingResult = true;
    bool readFrameResult = false;
    int startGrabbingCallCount = 0;
    int readFrameCallCount = 0;
    int readFrameDelayMs = 0;
    QImage frameToRead;
    HighSpeedCameraParameters parametersToRead;
    HighSpeedCameraParameters writtenParameters;
    int readParametersCallCount = 0;
    int writeParametersCallCount = 0;
    int stopGrabbingCallCount = 0;
    int closeCallCount = 0;
    QString error = "fake camera error";

    bool load() override {
        loadedState = loadResult;
        return loadResult;
    }

    bool loaded() const override { return loadedState; }
    QStringList searchedDllPaths() const override { return {"fake/MvCameraControl.dll"}; }
    QVector<HighSpeedCameraDevice> enumerateDevices() override { return devices; }

    bool open(int deviceIndex) override {
        ++openCallCount;
        openedDeviceIndex = deviceIndex;
        openState = openResult;
        return openResult;
    }

    bool isOpen() const override { return openState; }
    bool startGrabbing() override {
        ++startGrabbingCallCount;
        return startGrabbingResult;
    }

    bool readFrame(HighSpeedCameraFrame& frame, int) override {
        ++readFrameCallCount;
        if (readFrameDelayMs > 0) {
            QThread::msleep(static_cast<unsigned long>(readFrameDelayMs));
        }

        if (!readFrameResult) {
            return false;
        }

        frame.image = frameToRead.isNull() ? QImage(2, 2, QImage::Format_RGB888) : frameToRead;
        return true;
    }

    void stopGrabbing() override {
        ++stopGrabbingCallCount;
    }

    void close() override {
        ++closeCallCount;
        openState = false;
    }

    bool readParameters(HighSpeedCameraParameters& parameters) override {
        ++readParametersCallCount;
        parameters = parametersToRead;
        return true;
    }

    bool writeParameters(const HighSpeedCameraParameters& parameters) override {
        ++writeParametersCallCount;
        writtenParameters = parameters;
        return true;
    }

    QString lastError() const override { return error; }
};

class TestHighSpeedCameraService final : public QObject {
    Q_OBJECT

private slots:
    void enumerateDevicesPopulatesComboBox();
    void enumerateDevicesShowsClientDiagnosticWhenEmpty();
    void openCameraRequiresSelection();
    void openCameraUsesSelectedDeviceIndex();
    void getParameterWritesUiFields();
    void setParameterRejectsInvalidInput();
    void setParameterWritesClientValues();
    void startGrabbingDisplaysFramesAndStopsWorker();
    void startGrabbingRejectsAlreadyRunning();
    void stopCameraImmediatelyAfterStartIsSafe();
    void openCameraRejectsWhileGrabbing();
    void parametersRejectWhileGrabbing();
    void workerErrorClosesCamera();
    void flawPredictReportsNotConverted();
    void stopCameraClosesClientAndClearsPreview();
};

namespace {

HighSpeedCameraConfig testCameraConfig() {
    HighSpeedCameraConfig config;
    config.frameTimeoutMs = 10;
    return config;
}

HighSpeedCameraDevice makeDevice(int index, HighSpeedCameraTransport transport, const QString& name, const QString& endpoint) {
    HighSpeedCameraDevice device;
    device.index = index;
    device.transport = transport;
    device.userName = name;
    device.endpoint = endpoint;
    return device;
}

std::unique_ptr<FakeHighSpeedCameraClient> makeClient(FakeHighSpeedCameraClient*& rawClient) {
    auto client = std::make_unique<FakeHighSpeedCameraClient>();
    rawClient = client.get();
    return client;
}

} // namespace

void TestHighSpeedCameraService::enumerateDevicesPopulatesComboBox() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    FakeHighSpeedCameraClient* client = nullptr;
    auto ownedClient = makeClient(client);
    client->devices = {
        makeDevice(4, HighSpeedCameraTransport::GigE, "LineCam", "192.168.0.88"),
        makeDevice(9, HighSpeedCameraTransport::Usb, "BenchCam", "ABC123"),
    };
    HighSpeedCameraService service(testCameraConfig(), &ui, std::move(ownedClient));

    service.enumDevices();

    QCOMPARE(ui.camera_list->count(), 2);
    QCOMPARE(ui.camera_list->itemText(0), QString("[4]GigE: LineCam(192.168.0.88)"));
    QCOMPARE(ui.camera_list->itemData(0).toInt(), 4);
    QCOMPARE(ui.camera_list->itemText(1), QString("[9]USB: BenchCam(ABC123)"));
    QCOMPARE(ui.camera_list->itemData(1).toInt(), 9);
}

void TestHighSpeedCameraService::enumerateDevicesShowsClientDiagnosticWhenEmpty() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    FakeHighSpeedCameraClient* client = nullptr;
    auto ownedClient = makeClient(client);
    client->error = "MVS enumeration succeeded, but returned 0 devices.";
    HighSpeedCameraService service(testCameraConfig(), &ui, std::move(ownedClient));
    QSignalSpy messages(&service, &HighSpeedCameraService::messageRequested);

    service.enumDevices();

    QCOMPARE(ui.camera_list->count(), 0);
    QCOMPARE(messages.count(), 1);
    QVERIFY(messages.at(0).at(1).toString().contains("returned 0 devices"));
}

void TestHighSpeedCameraService::openCameraRequiresSelection() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    FakeHighSpeedCameraClient* client = nullptr;
    auto ownedClient = makeClient(client);
    client->loadedState = true;
    HighSpeedCameraService service(testCameraConfig(), &ui, std::move(ownedClient));
    QSignalSpy messages(&service, &HighSpeedCameraService::messageRequested);

    service.openCamera();

    QCOMPARE(client->openCallCount, 0);
    QCOMPARE(messages.count(), 1);
    QVERIFY(messages.at(0).at(1).toString().contains("Select"));
}

void TestHighSpeedCameraService::openCameraUsesSelectedDeviceIndex() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    FakeHighSpeedCameraClient* client = nullptr;
    auto ownedClient = makeClient(client);
    client->devices = {
        makeDevice(2, HighSpeedCameraTransport::GigE, "First", "10.0.0.2"),
        makeDevice(7, HighSpeedCameraTransport::Usb, "Second", "XYZ"),
    };
    HighSpeedCameraService service(testCameraConfig(), &ui, std::move(ownedClient));

    service.enumDevices();
    ui.camera_list->setCurrentIndex(1);
    service.openCamera();

    QCOMPARE(client->openCallCount, 1);
    QCOMPARE(client->openedDeviceIndex, 7);
}

void TestHighSpeedCameraService::getParameterWritesUiFields() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    FakeHighSpeedCameraClient* client = nullptr;
    auto ownedClient = makeClient(client);
    client->openState = true;
    client->parametersToRead = {120.5, 2500.25, 3.5};
    HighSpeedCameraService service(testCameraConfig(), &ui, std::move(ownedClient));

    service.getParameter();

    QCOMPARE(client->readParametersCallCount, 1);
    QCOMPARE(ui.FrameRate->text(), QString("120.5"));
    QCOMPARE(ui.ExposureTime->text(), QString("2500.25"));
    QCOMPARE(ui.Gain->text(), QString("3.5"));
}

void TestHighSpeedCameraService::setParameterRejectsInvalidInput() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    FakeHighSpeedCameraClient* client = nullptr;
    auto ownedClient = makeClient(client);
    client->openState = true;
    HighSpeedCameraService service(testCameraConfig(), &ui, std::move(ownedClient));
    QSignalSpy messages(&service, &HighSpeedCameraService::messageRequested);
    ui.FrameRate->setText("120");
    ui.ExposureTime->setText("not numeric");
    ui.Gain->setText("4");

    service.setParameter();

    QCOMPARE(client->writeParametersCallCount, 0);
    QCOMPARE(messages.count(), 1);
    QVERIFY(messages.at(0).at(1).toString().contains("Exposure time"));
}

void TestHighSpeedCameraService::setParameterWritesClientValues() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    FakeHighSpeedCameraClient* client = nullptr;
    auto ownedClient = makeClient(client);
    client->openState = true;
    HighSpeedCameraService service(testCameraConfig(), &ui, std::move(ownedClient));
    ui.FrameRate->setText("88.25");
    ui.ExposureTime->setText("1000.5");
    ui.Gain->setText("2.25");

    service.setParameter();

    QCOMPARE(client->writeParametersCallCount, 1);
    QCOMPARE(client->writtenParameters.frameRate, 88.25);
    QCOMPARE(client->writtenParameters.exposureTime, 1000.5);
    QCOMPARE(client->writtenParameters.gain, 2.25);
}

void TestHighSpeedCameraService::startGrabbingDisplaysFramesAndStopsWorker() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    FakeHighSpeedCameraClient* client = nullptr;
    auto ownedClient = makeClient(client);
    client->openState = true;
    client->readFrameResult = true;
    client->readFrameDelayMs = 1;
    client->frameToRead = QImage(4, 4, QImage::Format_RGB888);
    client->frameToRead.fill(Qt::red);
    HighSpeedCameraService service(testCameraConfig(), &ui, std::move(ownedClient));

    service.startGrabbing();

    QTRY_VERIFY(!ui.HC_RD->pixmap().isNull());
    service.stopCamera();

    QVERIFY(client->readFrameCallCount > 0);
    QCOMPARE(client->startGrabbingCallCount, 1);
    QCOMPARE(client->stopGrabbingCallCount, 1);
    QCOMPARE(client->closeCallCount, 1);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    QVERIFY(ui.HC_RD->pixmap().isNull());
}

void TestHighSpeedCameraService::startGrabbingRejectsAlreadyRunning() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    FakeHighSpeedCameraClient* client = nullptr;
    auto ownedClient = makeClient(client);
    client->openState = true;
    client->readFrameResult = true;
    client->readFrameDelayMs = 25;
    HighSpeedCameraService service(testCameraConfig(), &ui, std::move(ownedClient));
    QSignalSpy messages(&service, &HighSpeedCameraService::messageRequested);

    service.startGrabbing();
    service.startGrabbing();
    service.stopCamera();

    QCOMPARE(client->startGrabbingCallCount, 1);
    QVERIFY(messages.count() >= 1);
    QVERIFY(messages.takeLast().at(1).toString().contains("already running"));
}

void TestHighSpeedCameraService::stopCameraImmediatelyAfterStartIsSafe() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    FakeHighSpeedCameraClient* client = nullptr;
    auto ownedClient = makeClient(client);
    client->openState = true;
    client->readFrameResult = true;
    client->readFrameDelayMs = 50;
    HighSpeedCameraService service(testCameraConfig(), &ui, std::move(ownedClient));

    service.startGrabbing();
    service.stopCamera();

    QCOMPARE(client->startGrabbingCallCount, 1);
    QCOMPARE(client->stopGrabbingCallCount, 1);
    QCOMPARE(client->closeCallCount, 1);
    QVERIFY(ui.HC_RD->pixmap().isNull());
}

void TestHighSpeedCameraService::openCameraRejectsWhileGrabbing() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    FakeHighSpeedCameraClient* client = nullptr;
    auto ownedClient = makeClient(client);
    client->loadedState = true;
    client->openState = true;
    client->readFrameResult = true;
    client->readFrameDelayMs = 25;
    HighSpeedCameraService service(testCameraConfig(), &ui, std::move(ownedClient));
    QSignalSpy messages(&service, &HighSpeedCameraService::messageRequested);
    ui.camera_list->addItem("Camera", 4);

    service.startGrabbing();
    service.openCamera();
    service.stopCamera();

    QCOMPARE(client->openCallCount, 0);
    QVERIFY(messages.count() >= 1);
    QVERIFY(messages.at(0).at(1).toString().contains("Stop"));
}

void TestHighSpeedCameraService::parametersRejectWhileGrabbing() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    FakeHighSpeedCameraClient* client = nullptr;
    auto ownedClient = makeClient(client);
    client->openState = true;
    client->readFrameResult = true;
    client->readFrameDelayMs = 25;
    HighSpeedCameraService service(testCameraConfig(), &ui, std::move(ownedClient));
    QSignalSpy messages(&service, &HighSpeedCameraService::messageRequested);
    ui.FrameRate->setText("100");
    ui.ExposureTime->setText("2000");
    ui.Gain->setText("2");

    service.startGrabbing();
    service.getParameter();
    service.setParameter();
    service.stopCamera();

    QCOMPARE(client->readParametersCallCount, 0);
    QCOMPARE(client->writeParametersCallCount, 0);
    QVERIFY(messages.count() >= 2);
    QVERIFY(messages.at(0).at(1).toString().contains("Stop"));
    QVERIFY(messages.at(1).at(1).toString().contains("Stop"));
}

void TestHighSpeedCameraService::workerErrorClosesCamera() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    FakeHighSpeedCameraClient* client = nullptr;
    auto ownedClient = makeClient(client);
    client->openState = true;
    client->readFrameResult = false;
    client->error = "frame read failed";
    HighSpeedCameraService service(testCameraConfig(), &ui, std::move(ownedClient));
    QSignalSpy messages(&service, &HighSpeedCameraService::messageRequested);

    service.startGrabbing();

    QTRY_COMPARE(client->closeCallCount, 1);
    QCOMPARE(client->stopGrabbingCallCount, 1);
    QVERIFY(messages.count() >= 1);
    QVERIFY(messages.at(0).at(1).toString().contains("frame read failed"));
}

void TestHighSpeedCameraService::flawPredictReportsNotConverted() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    FakeHighSpeedCameraClient* client = nullptr;
    auto ownedClient = makeClient(client);
    HighSpeedCameraService service(testCameraConfig(), &ui, std::move(ownedClient));
    QSignalSpy messages(&service, &HighSpeedCameraService::messageRequested);

    service.flawPredict();

    QCOMPARE(messages.count(), 1);
    QVERIFY(messages.at(0).at(1).toString().contains("not converted"));
    QVERIFY(messages.at(0).at(1).toString().contains("Slice H"));
}

void TestHighSpeedCameraService::stopCameraClosesClientAndClearsPreview() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    FakeHighSpeedCameraClient* client = nullptr;
    auto ownedClient = makeClient(client);
    client->openState = true;
    HighSpeedCameraService service(testCameraConfig(), &ui, std::move(ownedClient));
    ui.FrameRate->setText("120");
    ui.ExposureTime->setText("2000");
    ui.Gain->setText("3");
    ui.HC_RD->setText("preview");
    ui.HC_PData->setText("prediction");

    service.stopCamera();

    QCOMPARE(client->stopGrabbingCallCount, 1);
    QCOMPARE(client->closeCallCount, 1);
    QVERIFY(ui.FrameRate->text().isEmpty());
    QVERIFY(ui.ExposureTime->text().isEmpty());
    QVERIFY(ui.Gain->text().isEmpty());
    QVERIFY(ui.HC_RD->text().isEmpty());
    QVERIFY(ui.HC_PData->text().isEmpty());
}

QTEST_MAIN(TestHighSpeedCameraService)
#include "test_high_speed_camera_service.moc"
