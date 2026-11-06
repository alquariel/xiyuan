#pragma once

#include <memory>
#include <optional>

#include <QObject>
#include <QTimer>

#include "app/AppConfig.h"
#include "app/AppPaths.h"
#include "robot_monitor/IRobotMonitorDdsPublisher.h"
#include "robot_monitor/RobotMonitorClient.h"
#include "robot_monitor/RobotMonitorRecorder.h"

// Owns the optional ABB robot monitoring workflow. This service is deliberately
// separate from ConnectionRobotButton, RobotPrintService, and RobotMovementService
// so live monitoring can be enabled from config without changing command sockets
// or robot motion/print behavior.
class RobotMonitoringService final : public QObject {
    Q_OBJECT

public:
    RobotMonitoringService(
        const AppConfig& config,
        const AppPaths& paths,
        QObject* parent = nullptr,
        std::unique_ptr<IRobotMonitorDdsPublisher> ddsPublisher = nullptr);
    ~RobotMonitoringService() override;

    bool isRunning() const;

signals:
    void statusChanged(const QString& status);

public slots:
    void start();
    void stop();

private:
    QString recordingDirectoryPath() const;
    void connectToConfiguredRobot();
    void reportStatus(const QString& status);
    void scheduleReconnect();
    void updateEstimatedSpeed(RobotMonitorSample& sample);

    AppConfig config_;
    AppPaths paths_;
    RobotMonitorClient client_;
    std::unique_ptr<RobotMonitorRecorder> recorder_;
    std::unique_ptr<IRobotMonitorDdsPublisher> ddsPublisher_;
    std::optional<RobotMonitorSample> previousSample_;
    QTimer reconnectTimer_;
    QTimer noDataTimer_;
    QTimer heartbeatTimer_;
    bool running_ = false;
    bool active_ = false;
    bool waitingForFirstFrame_ = false;
};
