#include "services/camera/HighSpeedCameraService.h"

// High-speed camera service implementation.
//
// The service deliberately wraps the generated widgets instead of changing the
// MainWindow in this slice. It provides a thin, testable controller over
// IHighSpeedCameraClient and keeps frame acquisition off the UI thread.

#include <atomic>
#include <cmath>
#include <memory>

#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QPixmap>
#include <QString>
#include <QThread>

#include "camera/HighSpeedCameraClient.h"
#include "ui_Interface4600_window.h"

namespace {

constexpr const char* kCameraTitle = "High-speed camera";

QString cameraClientError(const IHighSpeedCameraClient& client, const QString& fallback) {
    const QString error = client.lastError().trimmed();
    return error.isEmpty() ? fallback : error;
}

QString formatParameter(double value) {
    return QString::number(value, 'g', 12);
}

} // namespace

// Worker object for the blocking acquisition loop.
//
// readFrame can wait for the camera SDK timeout, so it must not run on the Qt
// main thread. The worker emits immutable QImage values; HighSpeedCameraService
// receives them through a queued connection and performs QLabel updates on the
// UI thread, which is the only thread allowed to mutate Qt widgets.
class HighSpeedCameraFrameWorker final : public QObject {
    Q_OBJECT

public:
    HighSpeedCameraFrameWorker(IHighSpeedCameraClient* client, int timeoutMs)
        : client_(client), timeoutMs_(timeoutMs) {
    }

public slots:
    void run() {
        // requestStop can be called immediately after the thread is started
        // and before this slot gets CPU time. The loop therefore tracks a
        // monotonic "stop requested" flag instead of resetting state here.
        while (!stopRequested_.load()) {
            HighSpeedCameraFrame frame;
            if (!client_->readFrame(frame, timeoutMs_)) {
                if (!stopRequested_.load()) {
                    emit error(cameraClientError(*client_, "Failed to read a high-speed camera frame."));
                }
                break;
            }

            if (!frame.image.isNull()) {
                emit frameReady(frame.image);
            }
        }

        emit finished();
    }

    void requestStop() {
        stopRequested_.store(true);
    }

signals:
    void frameReady(const QImage& image);
    void error(const QString& message);
    void finished();

private:
    IHighSpeedCameraClient* client_ = nullptr;
    int timeoutMs_ = 1000;
    std::atomic_bool stopRequested_ = false;
};

HighSpeedCameraService::HighSpeedCameraService(const HighSpeedCameraConfig& config,
                                               Ui::ContinuosfiberInterface* ui,
                                               QObject* parent)
    : HighSpeedCameraService(config,
                             ui,
                             std::make_unique<HighSpeedCameraClient>(config, QApplication::applicationDirPath()),
                             parent) {
}

HighSpeedCameraService::HighSpeedCameraService(const HighSpeedCameraConfig& config,
                                               Ui::ContinuosfiberInterface* ui,
                                               std::unique_ptr<IHighSpeedCameraClient> client,
                                               QObject* parent)
    : QObject(parent),
      config_(config),
      ui_(ui),
      client_(std::move(client)) {
}

HighSpeedCameraService::~HighSpeedCameraService() {
    stopWorker();
}

void HighSpeedCameraService::enumDevices() {
    if (ui_ == nullptr || ui_->camera_list == nullptr || !ensureLoaded()) {
        return;
    }

    ui_->camera_list->clear();
    const QVector<HighSpeedCameraDevice> devices = client_->enumerateDevices();
    for (const HighSpeedCameraDevice& device : devices) {
        ui_->camera_list->addItem(device.displayText(), device.index);
    }

    if (devices.isEmpty()) {
        showMessage(kCameraTitle, cameraClientError(*client_, "No high-speed cameras were found."));
    }
}

void HighSpeedCameraService::openCamera() {
    if (ui_ == nullptr || ui_->camera_list == nullptr || !ensureLoaded()) {
        return;
    }

    // The MVS SDK handle is shared by parameter, frame, and open/close calls.
    // Keep the service simple by allowing only one kind of camera operation at
    // a time; users must stop preview before switching to another device.
    if (isGrabbing()) {
        showMessage(kCameraTitle, "Stop high-speed camera grabbing before opening another camera.");
        return;
    }

    if (ui_->camera_list->currentIndex() < 0) {
        showMessage(kCameraTitle, "Select a high-speed camera before opening.");
        return;
    }

    bool ok = false;
    const int deviceIndex = ui_->camera_list->currentData().toInt(&ok);
    if (!ok) {
        showMessage(kCameraTitle, "The selected high-speed camera has no device index.");
        return;
    }

    if (!client_->open(deviceIndex)) {
        showMessage(kCameraTitle, cameraClientError(*client_, "Failed to open the selected high-speed camera."));
        return;
    }

    showMessage(kCameraTitle, "High-speed camera opened successfully.");
}

void HighSpeedCameraService::flawPredict() {
    showMessage(kCameraTitle, "High-speed camera flaw prediction is not converted for Slice H yet.");
}

void HighSpeedCameraService::startGrabbing() {
    if (!ensureOpen("start grabbing")) {
        return;
    }

    if (isGrabbing()) {
        showMessage(kCameraTitle, "High-speed camera grabbing is already running.");
        return;
    }

    if (!client_->startGrabbing()) {
        showMessage(kCameraTitle, cameraClientError(*client_, "Failed to start high-speed camera grabbing."));
        return;
    }

    grabbingActive_ = true;
    const quint64 generation = ++frameGeneration_;
    worker_ = new HighSpeedCameraFrameWorker(client_.get(), config_.frameTimeoutMs);
    worker_->moveToThread(&workerThread_);
    connect(&workerThread_, &QThread::started, worker_, &HighSpeedCameraFrameWorker::run);
    connect(worker_, &HighSpeedCameraFrameWorker::finished, &workerThread_, &QThread::quit, Qt::DirectConnection);
    connect(worker_, &HighSpeedCameraFrameWorker::finished, worker_, &QObject::deleteLater);
    connect(worker_, &HighSpeedCameraFrameWorker::finished, this, [this]() {
        grabbingActive_ = false;
    }, Qt::QueuedConnection);
    connect(worker_, &QObject::destroyed, this, [this]() {
        worker_ = nullptr;
    });
    connect(worker_, &HighSpeedCameraFrameWorker::frameReady, this, [this, generation](const QImage& image) {
        if (generation != frameGeneration_ || !isGrabbing()) {
            return;
        }
        displayFrame(image);
    }, Qt::QueuedConnection);
    connect(worker_, &HighSpeedCameraFrameWorker::error, this, &HighSpeedCameraService::handleWorkerError, Qt::QueuedConnection);
    workerThread_.start();
}

void HighSpeedCameraService::stopCamera() {
    stopWorker();
    if (client_) {
        client_->stopGrabbing();
        client_->close();
    }
    clearUiAfterClose();
}

void HighSpeedCameraService::getParameter() {
    if (!ensureOpen("read parameters")) {
        return;
    }

    if (isGrabbing()) {
        showMessage(kCameraTitle, "Stop high-speed camera grabbing before reading camera parameters.");
        return;
    }

    HighSpeedCameraParameters parameters;
    if (!client_->readParameters(parameters)) {
        showMessage(kCameraTitle, cameraClientError(*client_, "Failed to read high-speed camera parameters."));
        return;
    }

    ui_->FrameRate->setText(formatParameter(parameters.frameRate));
    ui_->ExposureTime->setText(formatParameter(parameters.exposureTime));
    ui_->Gain->setText(formatParameter(parameters.gain));
}

void HighSpeedCameraService::setParameter() {
    if (!ensureOpen("write parameters")) {
        return;
    }

    if (isGrabbing()) {
        showMessage(kCameraTitle, "Stop high-speed camera grabbing before writing camera parameters.");
        return;
    }

    HighSpeedCameraParameters parameters;
    if (!readPositiveDouble(ui_->FrameRate, "Frame rate", parameters.frameRate)
        || !readPositiveDouble(ui_->ExposureTime, "Exposure time", parameters.exposureTime)
        || !readPositiveDouble(ui_->Gain, "Gain", parameters.gain)) {
        return;
    }

    if (!client_->writeParameters(parameters)) {
        showMessage(kCameraTitle, cameraClientError(*client_, "Failed to write high-speed camera parameters."));
    }
}

void HighSpeedCameraService::displayFrame(const QImage& image) {
    if (ui_ == nullptr || ui_->HC_RD == nullptr) {
        return;
    }

    // Qt widgets are not thread-safe. Even though the worker captures frames on
    // a background thread, this slot belongs to the service on the Qt main
    // thread and is reached via Qt::QueuedConnection before touching HC_RD.
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, [this, image]() {
            displayFrame(image);
        }, Qt::QueuedConnection);
        return;
    }

    if (image.isNull()) {
        ui_->HC_RD->clear();
        return;
    }

    const QSize targetSize = ui_->HC_RD->size();
    QPixmap pixmap = QPixmap::fromImage(image);
    if (targetSize.isValid() && targetSize.width() > 0 && targetSize.height() > 0) {
        pixmap = pixmap.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    ui_->HC_RD->setPixmap(pixmap);
}

void HighSpeedCameraService::handleWorkerError(const QString& message) {
    showMessage(kCameraTitle, message);
    // A frame-read failure means the live preview loop can no longer make
    // progress. Close the camera path cleanly so later UI commands start from a
    // known idle state instead of leaving the SDK in "grabbing" mode.
    stopCamera();
}

bool HighSpeedCameraService::ensureLoaded() {
    if (!client_) {
        showMessage(kCameraTitle, "High-speed camera client is not available.");
        return false;
    }

    if (client_->loaded()) {
        return true;
    }

    if (client_->load()) {
        return true;
    }

    showMessage(kCameraTitle, cameraClientError(*client_, "Failed to load the high-speed camera SDK."));
    return false;
}

bool HighSpeedCameraService::ensureOpen(const QString& action) {
    if (!client_ || !client_->isOpen()) {
        showMessage(kCameraTitle, QString("Open a high-speed camera before trying to %1.").arg(action));
        return false;
    }

    return true;
}

bool HighSpeedCameraService::isGrabbing() const {
    return grabbingActive_ || workerThread_.isRunning();
}

bool HighSpeedCameraService::readPositiveDouble(QLineEdit* edit, const QString& fieldName, double& value) {
    const QString text = edit != nullptr ? edit->text().trimmed() : QString();
    bool ok = false;
    const double parsed = text.toDouble(&ok);
    if (text.isEmpty() || !ok || !std::isfinite(parsed) || parsed <= 0.0) {
        showMessage(kCameraTitle, QString("%1 must be a positive number.").arg(fieldName));
        return false;
    }

    value = parsed;
    return true;
}

void HighSpeedCameraService::stopWorker() {
    ++frameGeneration_;

    if (!worker_.isNull()) {
        // requestStop only flips an atomic flag, so calling it directly from the
        // UI thread is safe and avoids waiting for the worker event loop while
        // readFrame is blocked in the SDK timeout.
        worker_->requestStop();
    }

    if (workerThread_.isRunning()) {
        workerThread_.quit();
        workerThread_.wait();
    }

    worker_ = nullptr;
    grabbingActive_ = false;
}

void HighSpeedCameraService::clearUiAfterClose() {
    if (ui_ == nullptr) {
        return;
    }

    ui_->FrameRate->clear();
    ui_->ExposureTime->clear();
    ui_->Gain->clear();
    ui_->HC_RD->clear();
    ui_->HC_PData->clear();
}

void HighSpeedCameraService::showMessage(const QString& title, const QString& message) {
    emit messageRequested(title, message);
}

#include "HighSpeedCameraService.moc"
