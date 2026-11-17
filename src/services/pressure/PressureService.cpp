#include "services/pressure/PressureService.h"

#include "pressure/DebugPressureTcp.h"
#include "pressure/PressureAcquisitionWorker.h"
#include "ui_Interface4600_window.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMetaObject>
#include <QPushButton>

#include <iostream>

PressureService::PressureService(const AppConfig& config,
                                 const AppPaths& paths,
                                 Ui::ContinuosfiberInterface* ui,
                                 QObject* parent)
    : QObject(parent), config_(config.pressure), paths_(paths), ui_(ui) {
    qRegisterMetaType<PressureSampleData>("PressureSampleData");
}

PressureService::~PressureService() {
    stopCollection();
}

void PressureService::initializeMachine() {
    if (config_.source == PressureSource::DebugTcp) {
        emit messageRequested(
            "Pressure (Debug TCP)",
            QString("调试 TCP 模式 — 监听端口 %1\n手机/PC TCP 调试助手连接到 <PC IP>:%1\n发送格式：P,80.0  或  80.0")
                .arg(config_.debugTcpPort));
        return;
    }

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
    std::cerr << "[DIAG] PressureService::startCollection BEGIN" << std::endl;
    qInfo() << "[PressureService] startCollection() source="
            << static_cast<int>(config_.source)
            << "running_=" << running_;
    std::cerr << "[DIAG] log after qInfo" << std::endl;
    if (running_) {
        emit messageRequested("Pressure", "Pressure acquisition is already running.");
        return;
    }

    if (config_.source == PressureSource::DebugTcp) {
        // ---- Debug TCP 源 ----
        qInfo() << "[PressureService] creating DebugPressureTcp";
        // parent 传 nullptr：我们用 unique_ptr 手动管理生命周期，
        // 不要让 QObject parent 机制同时跟踪它，避免 double-delete。
        debugTcp_ = std::make_unique<DebugPressureTcp>(config_.debugTcpPort, nullptr);
        connect(debugTcp_.get(), &DebugPressureTcp::pressureSampleReady,
                this, &PressureService::pressureSampleReady, Qt::QueuedConnection);
        connect(debugTcp_.get(), &DebugPressureTcp::logMessage,
                this, &PressureService::handleDebugTcpMessage);
        connect(debugTcp_.get(), &DebugPressureTcp::listeningStateChanged,
                this, [this](bool listening) {
                    qInfo() << "[PressureService] listeningStateChanged =" << listening;
                    if (listening) handleStarted();
                    else handleStopped();
                });

        qInfo() << "[PressureService] calling debugTcp_->start()";
        if (!debugTcp_->start()) {
            qWarning() << "[PressureService] debugTcp_->start() FAILED";
            debugTcp_.reset();
            return;
        }
        qInfo() << "[PressureService] debugTcp started OK, handleStarted";
        running_ = true;
        handleStarted();
        qInfo() << "[PressureService] startCollection() done (DebugTcp)";
        return;
    }

    // ---- Real DLL 源 ----
    workerThread_ = std::make_unique<QThread>();
    worker_ = new PressureAcquisitionWorker(pressureDllPath(), config_);
    worker_->moveToThread(workerThread_.get());

    connect(workerThread_.get(), &QThread::started, worker_, &PressureAcquisitionWorker::start);
    connect(worker_, &PressureAcquisitionWorker::started, this, &PressureService::handleStarted);
    connect(worker_, &PressureAcquisitionWorker::stopped, this, &PressureService::handleStopped);
    connect(worker_, &PressureAcquisitionWorker::errorOccurred, this, &PressureService::handleError);
    // 采集线程只负责取样；通过队列连接切回主线程后再向图表和 EGM 转发。
    connect(worker_, &PressureAcquisitionWorker::samplePublished,
            this, [this](const PressureSampleData& sample) {
                emit pressureSampleReady(sample.rawValue);
            }, Qt::QueuedConnection);
    connect(worker_, &PressureAcquisitionWorker::stopped, workerThread_.get(), &QThread::quit);
    connect(workerThread_.get(), &QThread::finished, worker_, &QObject::deleteLater);
    connect(workerThread_.get(), &QThread::finished, this, &PressureService::cleanupWorkerThread);

    workerThread_->start();
}

void PressureService::stopCollection() {
    if (config_.source == PressureSource::DebugTcp) {
        cleanupDebugTcp();
        return;
    }

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

void PressureService::handleDebugTcpMessage(const QString& message) {
    emit messageRequested("Pressure (Debug TCP)", message);
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

void PressureService::cleanupDebugTcp() {
    if (debugTcp_) {
        debugTcp_->stop();
        debugTcp_.reset();
    }
    running_ = false;
    handleStopped();
}
