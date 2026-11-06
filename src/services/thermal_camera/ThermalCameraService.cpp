#include "services/thermal_camera/ThermalCameraService.h"

#include "ui_Interface4600_window.h"

#include <algorithm>
#include <utility>

#include <QFileInfo>
#include <QLabel>
#include <QPixmap>

namespace {
constexpr const char* kThermalTitle = "Thermal camera";
constexpr double kPythonPreviewWidth = 691.0;
constexpr double kPythonPreviewHeight = 518.0;
}

ThermalCameraService::ThermalCameraService(Ui::ContinuosfiberInterface* ui,
                                           ThermalFrameStore frameStore,
                                           std::unique_ptr<IThermalCameraClient> client,
                                           int displayIntervalMs,
                                           QObject* parent)
    : QObject(parent),
      ui_(ui),
      frameStore_(std::move(frameStore)),
      client_(std::move(client)),
      displayIntervalMs_(displayIntervalMs > 0 ? displayIntervalMs : 1000) {
    displayTimer_.setInterval(displayIntervalMs_);
    connect(&displayTimer_, &QTimer::timeout, this, &ThermalCameraService::refreshDisplay);
}

ThermalCameraService::~ThermalCameraService() {
    stopCamera();
}

void ThermalCameraService::startCamera() {
    if (client_ == nullptr) {
        showMessage("Thermal camera client is not available.");
        return;
    }

    // Ignore repeated clicks while the preview is already active. The Python UI
    // exposed separate start/stop buttons, so a second start should not reopen
    // the SDK session or disturb the current frame cache.
    if (client_->isCapturing()) {
        showMessage("Thermal camera capture is already running.");
        return;
    }

    if (!client_->startCapture()) {
        const QString error = client_->lastError().trimmed();
        showMessage(error.isEmpty() ? "Failed to start thermal camera." : error);
    }
}

void ThermalCameraService::stopCamera() {
    displayTimer_.stop();
    if (client_ != nullptr) {
        client_->stopCapture();
    }
    frameStore_.clearCache();
}

void ThermalCameraService::clearCache() {
    frameStore_.clearCache();
}

void ThermalCameraService::showData() {
    roiTracking_ = true;
    displayTimer_.start(displayIntervalMs_);
}

void ThermalCameraService::refreshDisplayOnceForTest() {
    refreshDisplay();
}

void ThermalCameraService::refreshDisplay() {
    if (ui_ == nullptr) {
        return;
    }

    QImage full;
    const QFileInfo newestFrame = frameStore_.newestReadableJpeg(&full);
    if (!newestFrame.exists() || full.isNull()) {
        return;
    }

    frameStore_.pruneCache();
    setLabelImage(ui_->TIC_RD, full);

    const QRect crop = roiSet_ ? calculateRoiCrop(full.size()) : calculateSquareCrop(full.size());
    const QImage cropped = full.copy(crop);
    setLabelImage(ui_->TIC_PD, cropped);
    if (roiSet_) {
        frameStore_.saveRoiImage(cropped);
    }
}

bool ThermalCameraService::handleMousePress(Qt::MouseButton button, const QPointF& localPos) {
    if (!roiTracking_) {
        return false;
    }

    if (button == Qt::LeftButton) {
        roiPoints_.append(localPos);
        return true;
    }

    if (button == Qt::MiddleButton && roiPoints_.size() >= 2) {
        roiSet_ = true;
        while (roiPoints_.size() > 2) {
            roiPoints_.removeFirst();
        }
        return true;
    }

    if (button == Qt::RightButton) {
        roiSet_ = false;
        roiPoints_.clear();
        return true;
    }

    return false;
}

QRect ThermalCameraService::calculateSquareCrop(const QSize& size) const {
    const int side = std::min(size.width(), size.height());
    return QRect(0, 0, side, side);
}

QRect ThermalCameraService::calculateRoiCrop(const QSize& size) const {
    if (roiPoints_.size() < 2) {
        return calculateSquareCrop(size);
    }

    const QPointF first = roiPoints_.at(0);
    const QPointF second = roiPoints_.at(1);
    const double leftNorm = std::clamp(std::min(first.x(), second.x()) / kPythonPreviewWidth, 0.0, 1.0);
    const double rightNorm = std::clamp(std::max(first.x(), second.x()) / kPythonPreviewWidth, 0.0, 1.0);
    const double topNorm = std::clamp(std::min(first.y(), second.y()) / kPythonPreviewHeight, 0.0, 1.0);
    const double bottomNorm = std::clamp(std::max(first.y(), second.y()) / kPythonPreviewHeight, 0.0, 1.0);

    QRect crop(
        QPoint(static_cast<int>(leftNorm * size.width()), static_cast<int>(topNorm * size.height())),
        QPoint(static_cast<int>(rightNorm * size.width()), static_cast<int>(bottomNorm * size.height())));
    crop = crop.normalized().intersected(QRect(QPoint(0, 0), size));

    const int side = std::max(1, std::min(crop.width(), crop.height()));
    return QRect(crop.topLeft(), QSize(side, side)).intersected(QRect(QPoint(0, 0), size));
}

void ThermalCameraService::setLabelImage(QLabel* label, const QImage& image) {
    if (label == nullptr || image.isNull()) {
        return;
    }

    label->setScaledContents(true);
    label->setPixmap(QPixmap::fromImage(image));
}

void ThermalCameraService::showMessage(const QString& message) {
    emit messageRequested(kThermalTitle, message);
}
