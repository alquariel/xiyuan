#pragma once

#include <QWidget>
#include "app/AppConfig.h"
#include "app/AppPaths.h"
#include "services/PlaceholderServices.h"
#include "services/ftp/FtpService.h"
#include "services/plc/PlcService.h"
#include "services/pressure/PressureService.h"
#include "services/archive/TimeseriesArchiveService.h"
#include "services/grafana/GrafanaStartupService.h"
#include "services/camera/HighSpeedCameraService.h"
#include "services/thermal_camera/ThermalCameraService.h"
#include "services/robot/RobotMovementService.h"
#include "services/robot_monitor/RobotMonitoringService.h"
#include "services/robot/RobotPrintService.h"

class QMouseEvent;

QT_BEGIN_NAMESPACE
namespace Ui { class ContinuosfiberInterface; }
QT_END_NAMESPACE

class MainWindow final : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    void startBackgroundServices();

private slots:
    void showPlaceholderMessage(const QString& title, const QString& message);
    void updateMotorStateFromJog();
    void updateMotorStateFromContinuous();

private:
    void mousePressEvent(QMouseEvent* event) override;
    void initializeWindowChrome();
    void initializeState();
    void initializeUiDefaults();
    void connectSignals();
    void centerOnScreen();

    Ui::ContinuosfiberInterface* ui_;
    AppPaths paths_;
    AppConfig config_;

    bool linkState_ = false;
    bool pauseFlag_ = false;
    bool stopFlag_ = true;
    bool plcConnected_ = false;
    bool motorEnabled_ = false;
    bool redLight_ = false;
    bool laserState_ = false;
    bool targetSet_ = false;
    bool temperatureControl_ = false;
    bool powerControl_ = false;

    double robotLineSpeed_ = 200.0;
    double robotOrientationSpeed_ = 200.0;
    double externalAxisLineSpeed_ = 200.0;
    double externalAxisOrientationSpeed_ = 100.0;
    double pavingDistance_ = 1.0;
    double wireFeedRate_ = 10.0;
    int hysteresisRatio_ = 200;

    SerialService serialService_;
    RobotService robotService_;
    FtpService ftpService_;
    RobotPrintService robotPrintService_;
    RobotMovementService robotMovementService_;
    RobotMonitoringService robotMonitoringService_;
    PlcService plcService_;
    PressureService pressureService_;
    TimeseriesArchiveService timeseriesArchiveService_;
    GrafanaStartupService grafanaStartupService_;
    HighSpeedCameraService highSpeedCameraService_;
    ThermalCameraService thermalCameraService_;
};
