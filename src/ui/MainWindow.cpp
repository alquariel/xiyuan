#include "ui/MainWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QMessageBox>
#include <QMouseEvent>
#include <QScreen>
#include <QStringList>
#include <cmath>
#include <memory>

#include "thermal_camera/ThermalCameraClient.h"
#include "thermal_camera/ThermalFrameStore.h"
#include "ui_Interface4600_window.h"

namespace {

// Quaternion helpers (ABB order: q1 = scalar / w). Used only to align the
// path's attitude onto the robot's real attitude at session start.
PathQuaternion QuatNormalize(const PathQuaternion& q) {
    const float len = std::sqrt(q.q1 * q.q1 + q.q2 * q.q2 + q.q3 * q.q3 + q.q4 * q.q4);
    if (len < 1e-6f) return PathQuaternion{1, 0, 0, 0};
    return PathQuaternion{q.q1 / len, q.q2 / len, q.q3 / len, q.q4 / len};
}

PathQuaternion QuatMultiply(const PathQuaternion& a, const PathQuaternion& b) {
    return PathQuaternion{
        a.q1 * b.q1 - a.q2 * b.q2 - a.q3 * b.q3 - a.q4 * b.q4,
        a.q1 * b.q2 + a.q2 * b.q1 + a.q3 * b.q4 - a.q4 * b.q3,
        a.q1 * b.q3 - a.q2 * b.q4 + a.q3 * b.q1 + a.q4 * b.q2,
        a.q1 * b.q4 + a.q2 * b.q3 - a.q3 * b.q2 + a.q4 * b.q1};
}

PathQuaternion QuatConjugate(const PathQuaternion& q) {
    return PathQuaternion{q.q1, -q.q2, -q.q3, -q.q4};
}

}  // namespace

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent),
      ui_(new Ui::ContinuosfiberInterface),
      paths_(QApplication::applicationDirPath()),
      config_(AppConfig::load(paths_)),
      serialService_(this),
      robotService_(this),
      ftpService_(config_.ftp, this),
      robotPrintService_(config_.robot, config_.plc, ftpService_, this),
      robotMovementService_(config_.robot, config_.plc, ui_, this),
      robotMonitoringService_(config_, paths_, this),
      plcService_(config_.plc, ui_, this),
      pressureService_(config_, paths_, ui_, this),
      timeseriesArchiveService_(config_, paths_, this),
      grafanaStartupService_(config_.grafana, paths_, this),
      highSpeedCameraService_(config_.highSpeedCamera, ui_, this),
      thermalCameraService_(
          ui_,
          ThermalFrameStore(paths_.rootDir(), config_.thermalCamera.cacheMaxFiles),
          std::make_unique<ThermalCameraClient>(
              config_.thermalCamera,
              paths_,
              ThermalFrameStore(paths_.rootDir(), config_.thermalCamera.cacheMaxFiles)),
          config_.thermalCamera.displayIntervalMs,
          this),
      egmClient_(config_.egm, this),
      pathLoader_(this),
      egmTimer_(this) {
    ui_->setupUi(this);
    initializeWindowChrome();
    initializeState();
    initializeUiDefaults();
    connectSignals();
    centerOnScreen();
}

MainWindow::~MainWindow() {
    delete ui_;
}

void MainWindow::startBackgroundServices() {
    if (config_.timeseriesArchive.enabled) {
        timeseriesArchiveService_.start();
    }
    if (config_.grafana.enabled) {
        grafanaStartupService_.start();
    }
    if (config_.robot.monitor.enabled && config_.robot.monitor.autostart) {
        robotMonitoringService_.start();
    }
}

void MainWindow::initializeWindowChrome() {
    setWindowTitle(QString::fromUtf8(u8"机器人面向铺带3D打印软件平台"));
    if (paths_.iconFilePath().exists()) {
        setWindowIcon(QIcon(paths_.iconFilePath().absoluteFilePath()));
    }
}

void MainWindow::initializeState() {
    linkState_ = false;
    pauseFlag_ = false;
    stopFlag_ = true;
    plcConnected_ = false;
    motorEnabled_ = false;
    redLight_ = false;
    laserState_ = false;
    targetSet_ = false;
    temperatureControl_ = false;
    powerControl_ = false;

    robotLineSpeed_ = 200.0;
    robotOrientationSpeed_ = 200.0;
    externalAxisLineSpeed_ = 200.0;
    externalAxisOrientationSpeed_ = 100.0;
    pavingDistance_ = 1.0;
    wireFeedRate_ = 10.0;
    hysteresisRatio_ = 200;
}

void MainWindow::initializeUiDefaults() {
    ui_->Other_tabWidget_Robotplatform->setCurrentIndex(0);

    ui_->ConnectionPLCButton->setVisible(false);
    ui_->ConnectionPLCButton->setEnabled(false);

    ui_->robot_state_show->setWordWrap(true);
    ui_->robot_state_show->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui_->robot_state_show->setText(config_.isLoaded()
        ? "Config loaded. Hardware not connected in Slice A."
        : "Config missing or invalid. Hardware not connected in Slice A.");

    if (!config_.warnings().isEmpty()) {
        ui_->robot_state_show->setToolTip(config_.warnings().join('\n'));
    }

    if (!config_.plc.enabled) {
        QWidget* const plcWidgets[] = {
            ui_->DisconnectPLCButton,
            ui_->cylinder1_button, ui_->cylinder2_button, ui_->cylinder3_button,
            ui_->motor_enable, ui_->motor_reset, ui_->motor_actuation,
            ui_->parameter_input, ui_->laser_connect,
            ui_->set_target_temperature, ui_->set_laser_power,
            ui_->temperature_control, ui_->set_laser_power_control,
            ui_->redlight_open, ui_->redlight_close,
            ui_->laser_open_button, ui_->laser_close_button,
            ui_->Hysteresis_ratio_input, ui_->interruption, ui_->continuous };
        for (QWidget* widget : plcWidgets) {
            widget->setEnabled(false);
        }

        const QString plcNote = "PLC disabled (simulation mode): laser, motor and cylinder controls are read-only.";
        const QString existingTip = ui_->robot_state_show->toolTip();
        ui_->robot_state_show->setToolTip(
            existingTip.isEmpty() ? plcNote : existingTip + "\n" + plcNote);
    }

    if (!config_.egm.enabled) {
        const QString egmNote = "EGM disabled (needs 689-1 External Guided Motion on the controller).";
        const QString existingTip = ui_->robot_state_show->toolTip();
        ui_->robot_state_show->setToolTip(
            existingTip.isEmpty() ? egmNote : existingTip + "\n" + egmNote);
    }
}

void MainWindow::connectSignals() {
    const auto connectPlaceholder = [this](PlaceholderService& service) {
        connect(&service, &PlaceholderService::messageRequested,
                this, &MainWindow::showPlaceholderMessage);
    };

    connectPlaceholder(serialService_);
    connectPlaceholder(robotService_);

    connect(&ftpService_, &FtpService::messageRequested,
            this, &MainWindow::showPlaceholderMessage);
    connect(&robotPrintService_, &RobotPrintService::messageRequested,
            this, &MainWindow::showPlaceholderMessage);
    connect(&robotMovementService_, &RobotMovementService::messageRequested,
            this, &MainWindow::showPlaceholderMessage);
    connect(&robotMonitoringService_, &RobotMonitoringService::statusChanged,
            this, [this](const QString& status) {
                const QString existingTip = ui_->robot_state_show->toolTip();
                ui_->robot_state_show->setToolTip(
                    existingTip.isEmpty() ? status : existingTip + "\n" + status);
            });
    connect(&robotPrintService_, &RobotPrintService::robotStatusRequested,
            this, [this](const QString& text, bool connected) {
                ui_->robot_state_show->setText(text);
                ui_->robot_state_show->setStyleSheet(
                    connected ? "background-color: lightgreen; font-size: 12px;"
                              : "background-color: pink; font-size: 12px;");
            });
    connect(&plcService_, &PlcService::messageRequested,
            this, &MainWindow::showPlaceholderMessage);
    connect(&pressureService_, &PressureService::messageRequested,
            this, &MainWindow::showPlaceholderMessage);
    connect(&timeseriesArchiveService_, &TimeseriesArchiveService::statusChanged,
            this, [this](const QString& status) {
                const QString existingTip = ui_->robot_state_show->toolTip();
                ui_->robot_state_show->setToolTip(
                    existingTip.isEmpty() ? status : existingTip + "\n" + status);
            });
    connect(&grafanaStartupService_, &GrafanaStartupService::statusChanged,
            this, [this](const QString& status) {
                const QString existingTip = ui_->robot_state_show->toolTip();
                ui_->robot_state_show->setToolTip(
                    existingTip.isEmpty() ? status : existingTip + "\n" + status);
            });
    connect(&highSpeedCameraService_, &HighSpeedCameraService::messageRequested,
            this, &MainWindow::showPlaceholderMessage);
    connect(&thermalCameraService_, &ThermalCameraService::messageRequested,
            this, &MainWindow::showPlaceholderMessage);

    connect(ui_->ChoosepushButton, &QPushButton::clicked, &serialService_, &SerialService::portCheck);
    connect(ui_->ConmunicationConnectionButton, &QPushButton::clicked, &serialService_, &SerialService::portOpen);
    connect(ui_->CloseSerialpushButton, &QPushButton::clicked, &serialService_, &SerialService::portClose);

    connect(ui_->ConnectionRobotButton, &QPushButton::clicked, &robotPrintService_, &RobotPrintService::connectRobot);
    connect(ui_->Robot_movebutton, &QPushButton::clicked, &robotMovementService_, &RobotMovementService::moveRobot);
    connect(ui_->RobotGohomeButton, &QPushButton::clicked, &robotMovementService_, &RobotMovementService::resetRobot);
    connect(ui_->RobotaxismoveButton, &QPushButton::clicked, &robotMovementService_, &RobotMovementService::moveRobotAxes);
    connect(ui_->PositionermoveButton, &QPushButton::clicked, &robotMovementService_, &RobotMovementService::moveExternalAxis);
    connect(ui_->PositionercomebackButton, &QPushButton::clicked, &robotMovementService_, &RobotMovementService::resetExternalAxis);

    // "开始打印" → EGM motion flow (was: robotPrintService_.startPrint)
    connect(ui_->Robot_movebutton_Gcode, &QPushButton::clicked,
            this, &MainWindow::onStartPrintClicked);

    // Pause / Continue / Stop — PLC side stays (so UI is symmetric), but when
    // EGM is the active motion layer we also pause/resume/terminate the tick.
    connect(ui_->PauseRobotButton, &QPushButton::clicked,
            &robotPrintService_, &RobotPrintService::pausePrint);
    connect(ui_->PauseRobotButton, &QPushButton::clicked, this, [this]() {
        if (egmFlowActive_) egmTimer_.stop();
    });
    connect(ui_->ContinueRobotButton, &QPushButton::clicked,
            &robotPrintService_, &RobotPrintService::continuePrint);
    connect(ui_->ContinueRobotButton, &QPushButton::clicked, this, [this]() {
        if (egmFlowActive_) egmTimer_.start(config_.egm.periodMs);
    });
    connect(ui_->StopRobotButton, &QPushButton::clicked,
            &robotPrintService_, &RobotPrintService::stopPrint);
    connect(ui_->StopRobotButton, &QPushButton::clicked, this, [this]() {
        if (egmFlowActive_) stopEgmFlow();
    });

    // "获取路径" → open file dialog on the legacy Python path folder
    connect(ui_->locusselectsetButton, &QPushButton::clicked,
            this, &MainWindow::loadPathFromTxtDialog);

    connect(ui_->DisconnectPLCButton, &QPushButton::clicked, &plcService_, &PlcService::disconnectPlc);
    connect(ui_->cylinder1_button, &QPushButton::clicked, &plcService_, &PlcService::cylinder1);
    connect(ui_->cylinder2_button, &QPushButton::clicked, &plcService_, &PlcService::cylinder2);
    connect(ui_->cylinder3_button, &QPushButton::clicked, &plcService_, &PlcService::cylinder3);
    connect(ui_->motor_enable, &QPushButton::clicked, &plcService_, &PlcService::motorEnable);
    connect(ui_->motor_reset, &QPushButton::clicked, &plcService_, &PlcService::motorReset);
    connect(ui_->motor_actuation, &QPushButton::clicked, &plcService_, &PlcService::motorActuation);
    connect(ui_->parameter_input, &QPushButton::clicked, &plcService_, &PlcService::parameterInput);
    connect(ui_->laser_connect, &QPushButton::clicked, &plcService_, &PlcService::laserConnect);
    connect(ui_->set_target_temperature, &QPushButton::clicked, &plcService_, &PlcService::setTargetTemperature);
    connect(ui_->set_laser_power, &QPushButton::clicked, &plcService_, &PlcService::setLaserPower);
    connect(ui_->temperature_control, &QPushButton::clicked, &plcService_, &PlcService::toggleTemperatureControl);
    connect(ui_->set_laser_power_control, &QPushButton::clicked, &plcService_, &PlcService::togglePowerControl);
    connect(ui_->redlight_open, &QPushButton::clicked, &plcService_, &PlcService::openRedLight);
    connect(ui_->redlight_close, &QPushButton::clicked, &plcService_, &PlcService::closeRedLight);
    connect(ui_->laser_open_button, &QPushButton::clicked, &plcService_, &PlcService::laserStart);
    connect(ui_->laser_close_button, &QPushButton::clicked, &plcService_, &PlcService::laserStop);
    connect(ui_->Hysteresis_ratio_input, &QPushButton::clicked, &plcService_, &PlcService::setHysteresisRatio);

    connect(ui_->interruption, &QCheckBox::clicked, this, &MainWindow::updateMotorStateFromJog);
    connect(ui_->continuous, &QCheckBox::clicked, this, &MainWindow::updateMotorStateFromContinuous);

    connect(ui_->enum_cam, &QPushButton::clicked, &highSpeedCameraService_, &HighSpeedCameraService::enumDevices);
    connect(ui_->HC_machine, &QPushButton::clicked, &highSpeedCameraService_, &HighSpeedCameraService::openCamera);
    connect(ui_->HC_data, &QPushButton::clicked, &highSpeedCameraService_, &HighSpeedCameraService::flawPredict);
    connect(ui_->HC_start, &QPushButton::clicked, &highSpeedCameraService_, &HighSpeedCameraService::startGrabbing);
    connect(ui_->HC_stop, &QPushButton::clicked, &highSpeedCameraService_, &HighSpeedCameraService::stopCamera);
    connect(ui_->get_parameter, &QPushButton::clicked, &highSpeedCameraService_, &HighSpeedCameraService::getParameter);
    connect(ui_->set_parameter, &QPushButton::clicked, &highSpeedCameraService_, &HighSpeedCameraService::setParameter);

    connect(ui_->TIC_machine, &QPushButton::clicked, &thermalCameraService_, &ThermalCameraService::startCamera);
    connect(ui_->TIC_stop, &QPushButton::clicked, &thermalCameraService_, &ThermalCameraService::stopCamera);
    connect(ui_->TIC_start, &QPushButton::clicked, &thermalCameraService_, &ThermalCameraService::clearCache);
    connect(ui_->TIC_data, &QPushButton::clicked, &thermalCameraService_, &ThermalCameraService::showData);

    connect(ui_->FC_machine, &QPushButton::clicked, &pressureService_, &PressureService::initializeMachine);
    connect(ui_->FC_data, &QPushButton::clicked, &pressureService_, &PressureService::showData);
    connect(ui_->FC_start, &QPushButton::clicked, &pressureService_, &PressureService::startCollection);
    connect(ui_->FC_stop, &QPushButton::clicked, &pressureService_, &PressureService::stopCollection);

    // EGM tick runs regardless of which .ui buttons started it — pause/continue
    // toggle the timer via lambdas above, stopEgmFlow is called explicitly.
    connect(&egmTimer_, &QTimer::timeout, this, &MainWindow::tickEgmFlow);
    connect(&egmClient_, &EgmClient::packetCounterTick,
            this, [this](quint64 packetsSent) {
                ui_->robot_state_show->setToolTip(
                    QString("EGM: %1 packets sent").arg(packetsSent));
            });
}

void MainWindow::centerOnScreen() {
    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        const QRect area = screen->availableGeometry();
        move(area.center() - rect().center());
    }
}

void MainWindow::mousePressEvent(QMouseEvent* event) {
    const QPoint labelPos = ui_->TIC_RD->mapFrom(this, event != nullptr ? event->pos() : QPoint());
    if (event != nullptr &&
        ui_->TIC_RD->rect().contains(labelPos) &&
        thermalCameraService_.handleMousePress(event->button(), QPointF(labelPos))) {
        return;
    }

    QWidget::mousePressEvent(event);
}

void MainWindow::showPlaceholderMessage(const QString& title, const QString& message) {
    QMessageBox::information(this, title, message);
}

void MainWindow::updateMotorStateFromJog() {
    if (ui_->interruption->isChecked()) {
        ui_->continuous->setChecked(false);
        plcService_.setMotorJogMode();
        ui_->state_show->setText(QString::fromUtf8(u8"点动状态"));
    }
}

void MainWindow::updateMotorStateFromContinuous() {
    if (ui_->continuous->isChecked()) {
        ui_->interruption->setChecked(false);
        plcService_.setMotorContinuousMode();
        ui_->state_show->setText(QString::fromUtf8(u8"连续状态"));
    }
}

// -----------------------------------------------------------------------------
// "获取路径" (locusselectsetButton) — pick a legacy Python path txt.
// -----------------------------------------------------------------------------
void MainWindow::loadPathFromTxtDialog() {
    if (!config_.egm.enabled) {
        QMessageBox::information(this, QString::fromUtf8(u8"路径加载"),
            QString::fromUtf8(u8"EGM 未启用，当前只是预加载路径。安装 689-1 并启用 egm.enabled 后点 \"开始打印\" 生效。"));
    }

    const QString defaultDir = QDir(QCoreApplication::applicationDirPath() + "/path").exists()
        ? QCoreApplication::applicationDirPath() + "/path"
        : QCoreApplication::applicationDirPath();

    const QString filePath = QFileDialog::getOpenFileName(
        this,
        QString::fromUtf8(u8"选择 7-10 列 txt 路径文件"),
        defaultDir,
        QStringLiteral("Path files (*.txt)"));
    if (filePath.isEmpty()) return;

    if (!pathLoader_.loadFromTxt(filePath)) {
        QMessageBox::critical(this, QString::fromUtf8(u8"路径加载失败"),
            QString::fromUtf8(u8"%1 解析失败。笛卡尔文件需要 ≥7 列 space-separated；\n"
                              u8"关节文件需要一行 \"# MODE=JOINT\" 注释 + 7 列 j1..j6(deg) v(deg/s)。").arg(filePath));
        return;
    }
    const bool jointPath = pathLoader_.isJointPath();
    const int segCount = jointPath ? pathLoader_.jointSegmentCount() : pathLoader_.segmentCount();
    if (segCount <= 0) {
        QMessageBox::critical(this, QString::fromUtf8(u8"路径文件为空"),
            QString::fromUtf8(u8"文件里没有有效行（至少 2 行）。"));
        return;
    }

    loadedPathFile_ = filePath;

    if (jointPath) {
        // 关节模式：v 列是 deg/s
        const float firstV = pathLoader_.jointPoints().isEmpty()
            ? 0.0f : pathLoader_.jointPoints().first().v;
        if (firstV > 0.0f) {
            egmJointSpeedDegPerSec_ = static_cast<double>(firstV);
        }
        ui_->robot_state_show->setText(
            QString::fromUtf8(u8"关节路径已加载：%1  [%2 段, 首段速度 %3 °/s, 关节模式]")
                .arg(QFileInfo(filePath).fileName())
                .arg(segCount)
                .arg(egmJointSpeedDegPerSec_, 0, 'f', 1));
        return;
    }

    // Use the v column of the first raw point as a hint for traverse speed
    // (legacy Python convention: column 7 is mm/s). Fall back to the default
    // we read from env_config.json / hard-coded otherwise.
    const float firstV = pathLoader_.rawPoints().isEmpty()
        ? 0.0f : pathLoader_.rawPoints().first().v;
    if (firstV > 1.0f) {
        egmTraverseSpeedMmPerSec_ = static_cast<double>(firstV);
    }

    ui_->robot_state_show->setText(
        QString::fromUtf8(u8"路径已加载：%1  [%2 段, 首段速度 %3 mm/s]")
            .arg(QFileInfo(filePath).fileName())
            .arg(pathLoader_.segmentCount())
            .arg(egmTraverseSpeedMmPerSec_, 0, 'f', 1));
}

// -----------------------------------------------------------------------------
// "开始打印" (Robot_movebutton_Gcode) — now EGM flow by default.
// -----------------------------------------------------------------------------
void MainWindow::onStartPrintClicked() {
    if (!config_.egm.enabled) {
        QMessageBox::warning(this, QString::fromUtf8(u8"EGM 不可用"),
            QString::fromUtf8(u8"EGM 未启用。RobotStudio 需先安装 689-1 External Guided Motion，\n"
                               u8"然后在 env_config.json 里把 egm.enabled 改为 true。"));
        return;
    }

    const bool jointPath = pathLoader_.isJointPath();
    const int segCount = jointPath ? pathLoader_.jointSegmentCount() : pathLoader_.segmentCount();
    if (loadedPathFile_.isEmpty() || segCount <= 0) {
        QMessageBox::information(this, QString::fromUtf8(u8"请先加载路径"),
            QString::fromUtf8(u8"先点 \"获取路径\" 选一个 .txt，再点 \"开始打印\" 启动 EGM 流。"));
        return;
    }

    if (!robotPrintService_.linkState() || robotPrintService_.socket() == nullptr ||
        !robotPrintService_.socket()->isConnected()) {
        QMessageBox::information(this, QString::fromUtf8(u8"请先连接机器人"),
            QString::fromUtf8(u8"EGM 使用与打印相同的 socket (5000) 发送 CASE 110 启动信号。\n"
                               u8"请先点 \"连接机器人\" 让 SERVER.mod 接收命令。"));
        return;
    }

    // Reset iterator state
    egmSegmentIndex_ = 0;
    egmSegmentProgressMm_ = 0.0f;
    egmJointProgressDeg_ = 0.0f;
    egmFlowActive_ = false;

    // 关节模式下 EGM 发送的是绝对关节角（单位：度），首帧必须等于机器人当前
    // 关节角，否则 EGMActJoint 的限幅会立刻判定超差、会话秒退。
    // 注意：txt 与 EGM 关节报文都用度，这里不做角度/弧度换算。
    QString poseLiveNote;
    if (jointPath) {
        float startJoints[6] = {0, 0, 0, 0, 0, 0};
        pathLoader_.jointPositionAt(0, 0.0f, startJoints);
        double jointsDeg[6];
        for (int axis = 0; axis < 6; ++axis) {
            jointsDeg[axis] = static_cast<double>(startJoints[axis]);
        }
        egmClient_.setMode(EgmClient::Mode::Joint);
        egmClient_.setTargetJoints(jointsDeg);
    } else {
        egmClient_.setMode(EgmClient::Mode::Pose);

        const PathVector3 pathStart = pathLoader_.positionAt(0, 0.0f);
        const PathQuaternion pathStartQuat = pathLoader_.orientationAt(0, 0.0f);

        // 先读机器人真实位姿（SERVER.mod CASE 113，currentTool + currentWobj）。
        // 路径里的姿态列可能已经过期（例如跑过 j6 关节测试后姿态变了 5°），
        // 直接下发会让腕部为了“进入路径姿态”而转动。这里把整条路径的姿态
        // 旋转对齐到当前姿态：
        //     q_cmd(t) = q_live ⊗ q_pathStart⁻¹ ⊗ q_path(t)
        // t=0 时 q_cmd 就是机器人当前姿态，入场零转动；路径内部的相对姿态
        // 变化（若有）仍然保留。
        egmAlignValid_ = false;
        egmAlignQuat_ = PathQuaternion{1, 0, 0, 0};

        QString liveReply;
        if (robotPrintService_.socket()->sendRawMessageWithReply(
                QStringLiteral("113 #"), liveReply).ok) {
            const QStringList tokens = liveReply.split(' ', Qt::SkipEmptyParts);
            if (tokens.size() >= 9) {
                const double liveX = tokens.at(2).toDouble();
                const double liveY = tokens.at(3).toDouble();
                const double liveZ = tokens.at(4).toDouble();
                const double dx = liveX - static_cast<double>(pathStart.x);
                const double dy = liveY - static_cast<double>(pathStart.y);
                const double dz = liveZ - static_cast<double>(pathStart.z);
                const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
                if (dist > 100.0) {
                    QMessageBox::critical(this, QString::fromUtf8(u8"机器人不在路径起点"),
                        QString::fromUtf8(u8"当前 TCP 与路径首点相差 %1 mm"
                                          u8"（EGM 偏移限幅 ±500 mm）。\n"
                                          u8"请先把机器人移到起点，或重新生成路径。")
                            .arg(dist, 0, 'f', 1));
                    return;
                }
                const PathQuaternion liveQuat{tokens.at(5).toFloat(), tokens.at(6).toFloat(),
                                              tokens.at(7).toFloat(), tokens.at(8).toFloat()};
                egmAlignQuat_ = QuatNormalize(
                    QuatMultiply(liveQuat, QuatConjugate(pathStartQuat)));
                egmAlignValid_ = true;
                poseLiveNote = QString::fromUtf8(u8"机器人当前姿态 [%1, %2, %3, %4]")
                                   .arg(tokens.at(5)).arg(tokens.at(6))
                                   .arg(tokens.at(7)).arg(tokens.at(8));
            }
        }

        // 位姿模式首帧不能是 EgmClient 内部的 (0,0,0)+单位四元数，否则与当前
        // TCP 相差上千米，EGM 偏移立刻超限 → 会话终止并急停。
        const PathQuaternion startQuat = egmAlignValid_
            ? QuatNormalize(QuatMultiply(egmAlignQuat_, pathStartQuat))
            : pathStartQuat;
        egmClient_.setTargetPose(EgmTcpPose{pathStart.x, pathStart.y, pathStart.z,
                                            startQuat.q1, startQuat.q2,
                                            startQuat.q3, startQuat.q4});
    }

    // 翻转 T_ROB1 的 EGM 启动标志（SERVER.mod CASE 110 / 120）；ACK 返回后
    // RAPID 才真正进入阻塞的 EGM 会话
    const QString startCommand = jointPath ? QStringLiteral("120 #") : QStringLiteral("110 #");
    const auto startResult = robotPrintService_.socket()->sendRawMessage(startCommand);
    if (!startResult.ok) {
        QMessageBox::critical(this, QString::fromUtf8(u8"发送 EGM 启动指令失败"),
            QString::fromUtf8(u8"SERVER.mod 未回复：%1").arg(startResult.message));
        return;
    }

    if (!egmClient_.start()) {
        QMessageBox::critical(this, QString::fromUtf8(u8"EGM 客户端启动失败"),
            QString::fromUtf8(u8"EgmClient::start() 返回 false。检查 egm.enabled / period_ms。"));
        robotPrintService_.socket()->sendRawMessage(QStringLiteral("111 #"));
        return;
    }

    egmFlowActive_ = true;
    egmTimer_.start(config_.egm.periodMs);

    if (jointPath) {
        ui_->robot_state_show->setText(
            QString::fromUtf8(u8"EGM 关节流启动：%1  [%2 段, %3 °/s, %4 ms 周期]")
                .arg(QFileInfo(loadedPathFile_).fileName())
                .arg(segCount)
                .arg(egmJointSpeedDegPerSec_, 0, 'f', 1)
                .arg(config_.egm.periodMs));
    } else {
        QString status = QString::fromUtf8(u8"EGM 流启动：%1  [%2 段, %3 mm/s, %4 ms 周期]")
                .arg(QFileInfo(loadedPathFile_).fileName())
                .arg(segCount)
                .arg(egmTraverseSpeedMmPerSec_, 0, 'f', 1)
                .arg(config_.egm.periodMs);
        if (!poseLiveNote.isEmpty()) {
            status += QString::fromUtf8(u8"\n已按真实姿态对齐 —— %1").arg(poseLiveNote);
        }
        ui_->robot_state_show->setText(status);
    }
}

void MainWindow::tickEgmFlow() {
    if (!egmFlowActive_) return;

    const double dtSec = config_.egm.periodMs / 1000.0;

    // ---- 关节模式：txt 与 EGM 报文都用「度」，不做角度/弧度换算 ----
    if (pathLoader_.isJointPath()) {
        egmJointProgressDeg_ += static_cast<float>(egmJointSpeedDegPerSec_ * dtSec);
        const auto& jointSegs = pathLoader_.jointSegments();

        while (egmSegmentIndex_ < jointSegs.size() &&
               egmJointProgressDeg_ >= jointSegs[egmSegmentIndex_].lengthDeg) {
            egmJointProgressDeg_ -= jointSegs[egmSegmentIndex_].lengthDeg;
            ++egmSegmentIndex_;
        }

        if (egmSegmentIndex_ >= jointSegs.size()) {
            stopEgmFlow();
            ui_->robot_state_show->setText(
                QString::fromUtf8(u8"EGM 关节流结束：已走完 %1 段").arg(jointSegs.size()));
            return;
        }

        float jointsDeg[6] = {0, 0, 0, 0, 0, 0};
        pathLoader_.jointPositionAt(egmSegmentIndex_, egmJointProgressDeg_, jointsDeg);
        double target[6];
        for (int axis = 0; axis < 6; ++axis) {
            target[axis] = static_cast<double>(jointsDeg[axis]);
        }
        egmClient_.setTargetJoints(target);
        return;
    }

    const float advanceMm = static_cast<float>(egmTraverseSpeedMmPerSec_ * dtSec);

    egmSegmentProgressMm_ += advanceMm;

    while (egmSegmentIndex_ < pathLoader_.segmentCount() &&
           egmSegmentProgressMm_ >= pathLoader_.segments()[egmSegmentIndex_].lengthMm) {
        egmSegmentProgressMm_ -= pathLoader_.segments()[egmSegmentIndex_].lengthMm;
        ++egmSegmentIndex_;
    }

    if (egmSegmentIndex_ >= pathLoader_.segmentCount()) {
        stopEgmFlow();
        ui_->robot_state_show->setText(
            QString::fromUtf8(u8"EGM 流结束：已走完 %1 段").arg(pathLoader_.segmentCount()));
        return;
    }

    const PathVector3 pos = pathLoader_.positionAt(egmSegmentIndex_, egmSegmentProgressMm_);
    PathQuaternion quat = pathLoader_.orientationAt(egmSegmentIndex_, egmSegmentProgressMm_);
    if (egmAlignValid_) {
        quat = QuatNormalize(QuatMultiply(egmAlignQuat_, quat));
    }
    // EGM 的 planned.cartesian 是绝对位姿（工具/工件坐标系见 RAPID 端 EGMActPose）
    egmClient_.setTargetPose(
        EgmTcpPose{pos.x, pos.y, pos.z, quat.q1, quat.q2, quat.q3, quat.q4});
}

void MainWindow::stopEgmFlow() {
    egmTimer_.stop();
    egmClient_.stop();

    if (robotPrintService_.socket() != nullptr && robotPrintService_.socket()->isConnected()) {
        robotPrintService_.socket()->sendRawMessage(QStringLiteral("111 #"));
    }

    egmFlowActive_ = false;
}
