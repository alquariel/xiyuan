#pragma once

#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QWidget>

#include "app/AppConfig.h"
#include "app/AppPaths.h"
#include "robot/EgmClient.h"
#include "robot/PathLoader.h"
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

    // Existing button reroutes — EGM is the default motion path now.
    void onStartPrintClicked();          // Robot_movebutton_Gcode
    void loadPathFromTxtDialog();        // locusselectsetButton ("获取路径")

    // EGM tick + manual stop (StopRobotButton side-effect)
    void tickEgmFlow();
    void stopEgmFlow();

    // 控制器 EGM 反馈位姿 → data_save/data_yyyyMMdd_HHmmss.csv
    void onEgmFeedbackPose(const EgmTcpPose& pose);

private:
    void mousePressEvent(QMouseEvent* event) override;
    void initializeWindowChrome();
    void initializeState();
    void initializeUiDefaults();
    void connectSignals();
    void centerOnScreen();

    void startEgmCsvRecording();
    void stopEgmCsvRecording();

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

    // EGM traverse speed (mm/s) — read from the legacy path-control v column
    // or fall back to this default.
    double egmTraverseSpeedMmPerSec_ = 50.0;

    // EGM joint-space traverse speed (deg/s) — read from the joint path v column.
    double egmJointSpeedDegPerSec_ = 1.0;

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

    // EGM motion layer — 4 ms interpolation tick feeding a 250 Hz UDP stream.
    EgmClient egmClient_;
    PathLoader pathLoader_;
    QTimer egmTimer_;

    // Currently-loaded txt path file (set by loadPathFromTxtDialog, consumed
    // by onStartPrintClicked). Empty means "no path loaded".
    QString loadedPathFile_;

    // EGM path iterator state (driven by egmTimer_)
    int egmSegmentIndex_ = 0;
    float egmSegmentProgressMm_ = 0.0f;
    float egmJointProgressDeg_ = 0.0f;
    bool egmFlowActive_ = false;

    // Pose-mode attitude alignment, computed at "start" from the robot's live
    // TCP pose (CASE 113). Commanded orientation = egmAlignQuat_ * pathQuat,
    // so the very first frame equals the robot's real attitude and the wrist
    // never rotates just to "enter" the path.
    PathQuaternion egmAlignQuat_{1, 0, 0, 0};
    bool egmAlignValid_ = false;

    // EGM 反馈位姿记录：每次 EGM 会话新建一个 data_save/data_*.csv。
    // 只在主线程读写，暂停/结束时关闭。
    QFile egmCsvFile_;
    QTextStream egmCsvStream_;
    bool egmCsvRecording_ = false;
    int egmCsvRowsSinceFlush_ = 0;
};
