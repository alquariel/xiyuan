#pragma once

// High-speed camera UI service.
//
// This file declares the QObject boundary between the generated main-window
// widgets and the IHighSpeedCameraClient abstraction. MainWindow wiring is left
// for the next slice; this service is independently testable through an
// injected fake client.

#include "app/AppConfig.h"
#include "camera/IHighSpeedCameraClient.h"

#include <QImage>
#include <QObject>
#include <QPointer>
#include <QThread>

#include <memory>

namespace Ui { class ContinuosfiberInterface; }
class QLineEdit;
class HighSpeedCameraFrameWorker;

// Owns the high-speed camera workflow for the existing UI widgets: SDK loading,
// device selection, opening/closing, parameter transfer, and preview display.
// The concrete camera client is hidden behind IHighSpeedCameraClient so tests
// can exercise UI behavior without camera hardware or vendor DLLs.
class HighSpeedCameraService final : public QObject {
    Q_OBJECT

public:
    HighSpeedCameraService(const HighSpeedCameraConfig& config,
                           Ui::ContinuosfiberInterface* ui,
                           QObject* parent = nullptr);
    HighSpeedCameraService(const HighSpeedCameraConfig& config,
                           Ui::ContinuosfiberInterface* ui,
                           std::unique_ptr<IHighSpeedCameraClient> client,
                           QObject* parent = nullptr);
    ~HighSpeedCameraService() override;

public slots:
    void enumDevices();
    void openCamera();
    void flawPredict();
    void startGrabbing();
    void stopCamera();
    void getParameter();
    void setParameter();

signals:
    void messageRequested(const QString& title, const QString& message);

private slots:
    void displayFrame(const QImage& image);
    void handleWorkerError(const QString& message);

private:
    bool ensureLoaded();
    bool ensureOpen(const QString& action);
    bool isGrabbing() const;
    bool readPositiveDouble(QLineEdit* edit, const QString& fieldName, double& value);
    void stopWorker();
    void clearUiAfterClose();
    void showMessage(const QString& title, const QString& message);

    HighSpeedCameraConfig config_;
    Ui::ContinuosfiberInterface* ui_ = nullptr;
    std::unique_ptr<IHighSpeedCameraClient> client_;
    QThread workerThread_;
    // The worker belongs to workerThread_ and deletes itself when acquisition
    // ends. QPointer clears automatically if Qt destroys the worker before the
    // service asks it to stop, avoiding a stale cross-thread pointer.
    QPointer<HighSpeedCameraFrameWorker> worker_;
    bool grabbingActive_ = false;
    // Each preview run gets a generation number. Queued frame signals from an
    // older run are ignored after stop/close, so a stale frame cannot repaint
    // the preview after the UI has been cleared.
    quint64 frameGeneration_ = 0;
};
