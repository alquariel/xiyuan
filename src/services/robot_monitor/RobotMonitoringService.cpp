#include "services/robot_monitor/RobotMonitoringService.h"

#include "robot_monitor/RobotMonitorDdsPublisher.h"

#include <cmath>
#include <utility>

#include <QDebug>
#include <QDir>

namespace {
constexpr int kReconnectDelayMs = 1000;
constexpr int kNoDataWarningDelayMs = 3000;
constexpr int kHeartbeatIntervalMs = 1000;
}

RobotMonitoringService::RobotMonitoringService(
    const AppConfig& config,
    const AppPaths& paths,
    QObject* parent,
    std::unique_ptr<IRobotMonitorDdsPublisher> ddsPublisher)
    : QObject(parent),
      config_(config),
      paths_(paths),
      client_(this),
      ddsPublisher_(std::move(ddsPublisher)) {
    if (config_.robot.monitor.ddsPublishEnabled && !ddsPublisher_) {
        ddsPublisher_ = std::make_unique<RobotMonitorDdsPublisher>(config_.robot.monitor);
    }

    reconnectTimer_.setSingleShot(true);
    reconnectTimer_.setInterval(kReconnectDelayMs);
    connect(&reconnectTimer_, &QTimer::timeout, this, [this]() {
        if (!active_ || client_.isConnected()) {
            return;
        }
        reportStatus(QString("Robot monitor retrying connection to %1:%2.")
                         .arg(config_.robot.ip)
                         .arg(config_.robot.monitor.port));
        connectToConfiguredRobot();
    });

    noDataTimer_.setSingleShot(true);
    noDataTimer_.setInterval(kNoDataWarningDelayMs);
    connect(&noDataTimer_, &QTimer::timeout, this, [this]() {
        if (active_ && running_ && waitingForFirstFrame_) {
            reportStatus("Robot monitor connected, but no monitor data received yet.");
        }
    });

    heartbeatTimer_.setSingleShot(false);
    heartbeatTimer_.setInterval(kHeartbeatIntervalMs);
    connect(&heartbeatTimer_, &QTimer::timeout, this, [this]() {
        if (!active_ || !running_ || !client_.isConnected()) {
            heartbeatTimer_.stop();
            return;
        }
        if (!client_.sendHeartbeat()) {
            reportStatus("Robot monitor heartbeat send failed.");
        }
    });

    connect(&client_, &RobotMonitorClient::connected, this, [this]() {
        running_ = true;
        waitingForFirstFrame_ = true;
        reconnectTimer_.stop();
        noDataTimer_.start();
        heartbeatTimer_.start();
        reportStatus("Robot monitor connected.");
    });
    connect(&client_, &RobotMonitorClient::disconnected, this, [this]() {
        running_ = false;
        waitingForFirstFrame_ = false;
        noDataTimer_.stop();
        heartbeatTimer_.stop();
        reportStatus("Robot monitor disconnected.");
        scheduleReconnect();
    });
    connect(&client_, &RobotMonitorClient::errorReported, this, [this](const QString& error) {
        running_ = false;
        waitingForFirstFrame_ = false;
        noDataTimer_.stop();
        heartbeatTimer_.stop();
        reportStatus(QString("Robot monitor socket error: %1").arg(error));
        scheduleReconnect();
    });
    connect(&client_, &RobotMonitorClient::rawMessageReceived, this, [this](const QString& raw) {
        if (waitingForFirstFrame_) {
            waitingForFirstFrame_ = false;
            noDataTimer_.stop();
            reportStatus("Robot monitor first frame received.");
        }
        if (recorder_ && !recorder_->recordRawMessage(raw)) {
            reportStatus(QString("Robot monitor recorder warning: %1").arg(recorder_->lastError()));
        }
    });
    connect(&client_, &RobotMonitorClient::parseErrorReported, this, [this](const QString& raw, const QString& error) {
        reportStatus(QString("Robot monitor parse error: %1 | raw=%2").arg(error, raw));
    });
    connect(&client_, &RobotMonitorClient::sampleReceived, this, [this](const RobotMonitorSample& receivedSample) {
        RobotMonitorSample sample = receivedSample;
        updateEstimatedSpeed(sample);
        if (config_.robot.monitor.ddsPublishEnabled && ddsPublisher_ && !ddsPublisher_->publish(sample)) {
            reportStatus(QString("Robot monitor DDS publish warning: %1").arg(ddsPublisher_->lastError()));
        }
        if (recorder_ && !recorder_->recordSample(sample)) {
            reportStatus(QString("Robot monitor recorder warning: %1").arg(recorder_->lastError()));
        }
    });
}

RobotMonitoringService::~RobotMonitoringService() {
    stop();
}

bool RobotMonitoringService::isRunning() const {
    return running_;
}

void RobotMonitoringService::start() {
    if (!config_.robot.monitor.enabled || !config_.robot.monitor.autostart) {
        return;
    }
    active_ = true;
    if (config_.robot.ip.trimmed().isEmpty()) {
        active_ = false;
        reportStatus("Robot monitor disabled: robot.ip is empty.");
        return;
    }

    previousSample_.reset();
    recorder_ = std::make_unique<RobotMonitorRecorder>(
        recordingDirectoryPath(),
        config_.robot.monitor.recordCsv,
        config_.robot.monitor.recordRawMessages,
        config_.robot.monitor.flushEverySample);

    if (!recorder_->start()) {
        reportStatus(QString("Robot monitor recorder warning: %1").arg(recorder_->lastError()));
        recorder_.reset();
    }

    reportStatus(QString("Robot monitor connecting to %1:%2.")
                     .arg(config_.robot.ip)
                     .arg(config_.robot.monitor.port));
    connectToConfiguredRobot();
}

void RobotMonitoringService::stop() {
    active_ = false;
    reconnectTimer_.stop();
    noDataTimer_.stop();
    heartbeatTimer_.stop();
    client_.disconnectFromRobot();
    if (recorder_) {
        recorder_->close();
        recorder_.reset();
    }
    previousSample_.reset();
    running_ = false;
    waitingForFirstFrame_ = false;
}

QString RobotMonitoringService::recordingDirectoryPath() const {
    const QDir configured(config_.robot.monitor.recordingDir);
    if (configured.isAbsolute()) {
        return configured.absolutePath();
    }
    return QDir(paths_.rootDir().absolutePath()).filePath(config_.robot.monitor.recordingDir);
}

void RobotMonitoringService::connectToConfiguredRobot() {
    if (!active_) {
        return;
    }
    client_.connectToRobot(config_.robot.ip, config_.robot.monitor.port);
}

void RobotMonitoringService::reportStatus(const QString& status) {
    qInfo().noquote() << status;
    emit statusChanged(status);
}

void RobotMonitoringService::scheduleReconnect() {
    if (!active_ || reconnectTimer_.isActive()) {
        return;
    }
    reconnectTimer_.start();
}

void RobotMonitoringService::updateEstimatedSpeed(RobotMonitorSample& sample) {
    if (!previousSample_) {
        sample.estimatedTcpSpeed = 0.0;
        previousSample_ = sample;
        return;
    }

    const double elapsedSeconds = previousSample_->receivedAtUtc.msecsTo(sample.receivedAtUtc) / 1000.0;
    if (elapsedSeconds > 0.0) {
        const double dx = sample.x - previousSample_->x;
        const double dy = sample.y - previousSample_->y;
        const double dz = sample.z - previousSample_->z;
        sample.estimatedTcpSpeed = std::sqrt(dx * dx + dy * dy + dz * dz) / elapsedSeconds;
    } else {
        sample.estimatedTcpSpeed = 0.0;
    }
    previousSample_ = sample;
}
