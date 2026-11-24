#include "services/pressure/PressureService.h"

#include "pressure/PressureAcquisitionWorker.h"
#include "ui_Interface4600_window.h"

#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QPushButton>

PressureService::PressureService(const AppConfig& config,
                                 const AppPaths& paths,
                                 Ui::ContinuosfiberInterface* ui,
                                 QObject* parent)
    : QObject(parent), config_(config.pressure), paths_(paths), ui_(ui) {
}

PressureService::~PressureService() {
    stopCollection();
}

void PressureService::initializeMachine() {
    const QFileInfo dll(pressureDllPath());
    if (!dll.exists()) {
        emit messageRequested("Pressure", QString("Pressure sensor DLL was not found: %1").arg(dll.absoluteFilePath()));
        return;
    }

    emit messageRequested(
        "Pressure",
        QString("Pressure sensor runtime found.\nDDS topic: %1\nDDS domain: %2\nSample interval: %3 ms\nProcessing window: %4 samples")
            .arg(config_.ddsTopic)
            .arg(config_.ddsDomainId)
            .arg(config_.sampleIntervalMs)
            .arg(config_.processingWindowSamples));
}

void PressureService::startCollection() {
    if (running_ || workerThread_) {
        emit messageRequested("Pressure", "Pressure acquisition is already running.");
        return;
    }

    workerThread_ = std::make_unique<QThread>();
    worker_ = new PressureAcquisitionWorker(pressureDllPath(), config_);
    worker_->moveToThread(workerThread_.get());

    connect(workerThread_.get(), &QThread::started, worker_, &PressureAcquisitionWorker::start);
    connect(worker_, &PressureAcquisitionWorker::started, this, &PressureService::handleStarted);
    connect(worker_, &PressureAcquisitionWorker::stopped, this, &PressureService::handleStopped);
    connect(worker_, &PressureAcquisitionWorker::errorOccurred, this, &PressureService::handleError);
    connect(worker_, &PressureAcquisitionWorker::stopped, workerThread_.get(), &QThread::quit);
    connect(workerThread_.get(), &QThread::finished, worker_, &QObject::deleteLater);
    connect(workerThread_.get(), &QThread::finished, this, &PressureService::cleanupWorkerThread);

    workerThread_->start();
}

void PressureService::stopCollection() {
    if (!workerThread_) {
        running_ = false;
        return;
    }

    if (worker_) {
        QMetaObject::invokeMethod(worker_, "stop", Qt::QueuedConnection);
    }

    workerThread_->wait(2000);
    cleanupWorkerThread();
}

void PressureService::showData() {
    // Slice C publishes the pressure stream through DDS. The original labels
    // and plot widgets remain in the UI file, but this slice does not draw
    // curves in the main window.
    emit messageRequested(
        "Pressure DDS",
        QString("Publish topic: %1\nType: AfpPressureSample\nDomain ID: %2\nUse PlotJuggler eProsima Edition or a Fast DDS subscriber to view pressure data.")
            .arg(config_.ddsTopic)
            .arg(config_.ddsDomainId));
}

void PressureService::handleStarted() {
    running_ = true;
    if (ui_ && ui_->FC_start) {
        ui_->FC_start->setEnabled(false);
    }
    if (ui_ && ui_->FC_stop) {
        ui_->FC_stop->setEnabled(true);
    }
}

void PressureService::handleStopped() {
    running_ = false;
    if (ui_ && ui_->FC_start) {
        ui_->FC_start->setEnabled(true);
    }
    if (ui_ && ui_->FC_stop) {
        ui_->FC_stop->setEnabled(true);
    }
}

void PressureService::handleError(const QString& message) {
    emit messageRequested("Pressure", message);
    handleStopped();
}

QString PressureService::pressureDllPath() const {
    return paths_.rootDir().absoluteFilePath("lib/sensors/pressure_sensor/pressure.dll");
}

void PressureService::cleanupWorkerThread() {
    running_ = false;
    worker_ = nullptr;
    if (workerThread_ && !workerThread_->isRunning()) {
        workerThread_.reset();
    }
}
