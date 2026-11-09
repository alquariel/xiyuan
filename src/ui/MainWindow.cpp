#include "ui/MainWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QMessageBox>
#include <QMouseEvent>
#include <QScreen>
#include <QStringList>
#include <QTextStream>
#include <QThread>
#include <algorithm>
#include <cmath>
#include <memory>

#include "thermal_camera/ThermalCameraClient.h"
#include "thermal_camera/ThermalFrameStore.h"
#include "ui_Interface4600_window.h"

namespace {

// Path txt default folder: prefer src/path inside the source tree.
// The executable normally runs from build/ (or build/Debug, build/Release),
// so walk up from the executable directory until src/path is found. A deployed
// layout may also place src/path beside the executable. Falls back to the
// executable directory when nothing matches.
QString defaultPathTxtDir() {
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + QStringLiteral("/src/path"),
        appDir + QStringLiteral("/../src/path"),
        appDir + QStringLiteral("/../../src/path"),
        appDir + QStringLiteral("/../../../src/path"),
    };
    for (const QString& candidate : candidates) {
        if (QDir(candidate).exists()) {
            return QDir(candidate).absolutePath();
        }
    }
    return appDir;
}

// data_save 目录：项目根下、与 src 文件夹同级。exe 通常在 build/ 下运行，
// 因此从可执行文件目录逐级向上找到含 src 的目录作为项目根，再在其下建 data_save。
// 部署环境（没有 src）时退回可执行文件同级的 data_save。
QString dataSaveDir() {
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidateRoots = {
        appDir,
        appDir + QStringLiteral("/.."),
        appDir + QStringLiteral("/../.."),
        appDir + QStringLiteral("/../../.."),
    };
    for (const QString& candidate : candidateRoots) {
        const QString rootPath = QDir(candidate).absolutePath();
        if (!QDir(rootPath + QStringLiteral("/src")).exists()) {
            continue;
        }
        const QString target = rootPath + QStringLiteral("/data_save");
        QDir().mkpath(target);
        return target;
    }
    const QString fallback = QDir(appDir).absoluteFilePath(QStringLiteral("data_save"));
    QDir().mkpath(fallback);
    return fallback;
}

// 位姿读数诊断：把每次启动 EGM 时 CASE 113 的原始应答与本地缓存的反馈位姿
// 追加到 data_save/pose_read.log。用于复盘「上一轮 EGM 结束后 113 返回错误
// 姿态」这类问题（正常时两者应只差几毫米、几十分之一度）。
void appendPoseReadLog(const QString& line) {
    QFile file(QDir(dataSaveDir()).absoluteFilePath(QStringLiteral("pose_read.log")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        return;
    }
    QTextStream stream(&file);
    stream << QDateTime::currentDateTime().toString(Qt::ISODate) << ' ' << line << '\n';
}

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

// 向 SERVER.mod 发一条无参数指令，并解析应答第三个 token 的整数值
// （CASE 112 = EGM 会话状态，CASE 114 = 上一次 EGM 会话的错误码）。
// 返回 -1 = 无应答；-2 = 应答了但状态位不是 OK（多半是模块还没重载，
// 不认识这条查询）；>=0 = 查询得到的值。超时故意设短，避免把界面卡住。
int queryRobotInt(IRobotCommandSocket* socket, const QString& frame, int timeoutMs) {
    if (socket == nullptr) {
        return -1;
    }
    QString reply;
    if (!socket->sendRawMessageWithReply(frame, reply, timeoutMs).ok) {
        return -1;
    }
    const QStringList tokens = reply.split(' ', Qt::SkipEmptyParts);
    if (tokens.size() >= 2 && tokens.at(1).toInt() == 0) {
        return -2;
    }
    if (tokens.size() < 3) {
        return -1;
    }
    bool ok = false;
    const int value = tokens.at(2).toInt(&ok);
    return ok ? value : -1;
}

// 把 queryRobotInt 的返回码转成给用户看的文字。
QString describeQueryResult(int value) {
    if (value == -1) {
        return QString::fromUtf8(u8"无应答");
    }
    if (value == -2) {
        return QString::fromUtf8(u8"命令未实现（模块可能未重载）");
    }
    return QString::number(value);
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
      egmTimer_(this),
      egmStartWatchdog_(this) {
    ui_->setupUi(this);
    egmStartWatchdog_.setSingleShot(true);
    initializeWindowChrome();
    initializeState();
    initializeUiDefaults();
    connectSignals();
    centerOnScreen();
}

MainWindow::~MainWindow() {
    stopEgmCsvRecording();
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
    connect(&egmClient_, &EgmClient::feedbackPoseReceived,
            this, &MainWindow::onEgmFeedbackPose);

    // 启动看门狗：EGM 启动后 2.5 s 内收不到任何 EgmRobot 反馈帧，说明会话
    // 根本没跑起来。这时必须主动停下并说明原因，否则就是「机器人不动、
    // 也没有报错」的静默失败。
    connect(&egmStartWatchdog_, &QTimer::timeout, this, [this]() {
        if (!egmFlowActive_ || egmClient_.isPeerConnected()) {
            return;
        }
        const quint16 udpPort = egmClient_.boundPort();
        const int egmState = queryRobotInt(robotPrintService_.socket(),
                                           QStringLiteral("112 #"), 800);
        const int lastEgmErrno = queryRobotInt(robotPrintService_.socket(),
                                               QStringLiteral("114 #"), 800);
        QString watchdogLine =
            QStringLiteral("EGM watchdog: no feedback frame | %1 | udpPort=%2 egmState=%3 lastErrno=%4");
        watchdogLine = watchdogLine.arg(egmStartContext_)
                                     .arg(static_cast<int>(udpPort))
                                     .arg(egmState)
                                     .arg(lastEgmErrno);
        appendPoseReadLog(watchdogLine);
        stopEgmFlow();
        ui_->robot_state_show->setText(
            QString::fromUtf8(u8"EGM 未启动：2.5 s 内未收到控制器反馈帧。"));

        QString detail = QString::fromUtf8(
            u8"EGM 流已经在发，但 2.5 s 内没有收到控制器的任何反馈帧，"
            u8"机器人不会运动。诊断信息：\n\n"
            u8"本地 UDP 端口：%1\n"
            u8"控制器 EGM 状态（CASE 112）：%2\n"
            u8"上次 EGM 会话错误码（CASE 114）：%3\n\n")
            .arg(udpPort)
            .arg(describeQueryResult(egmState))
            .arg(describeQueryResult(lastEgmErrno));

        if (udpPort == 0) {
            detail += QString::fromUtf8(
                u8"→ 6510 端口没有绑定成功：可能有别的程序（上一次残留的上位机进程、"
                u8"Python 旧版程序）还占着这个端口，请先关掉再试。");
        } else if (egmState == -1) {
            detail += QString::fromUtf8(
                u8"→ SERVER.mod 没有应答：T_ROB1 很可能已经停止（报警未清除/程序指针"
                u8"不在 main）。请到示教器清报警 → PP to main → 启动 T_ROB1，再点开始。");
        } else if (lastEgmErrno >= 0 && lastEgmErrno != 0) {
            detail += QString::fromUtf8(
                u8"→ 控制器侧 EGM 会话报错（错误码 %1）。上一次会话可能没有正常收尾，"
                u8"请在 RobotStudio 事件日志里核对这个错误码。")
                .arg(lastEgmErrno);
        } else if (lastEgmErrno == -2) {
            detail += QString::fromUtf8(
                u8"→ 控制器的 SERVER.mod 还是旧版本（不认识 CASE 114）。"
                u8"EGM 相关模块必须执行控制器完整重启（X-start）后才会生效，"
                u8"请先在 RobotStudio 里重启控制器，再重试。");
        } else {
            detail += QString::fromUtf8(
                u8"→ 控制器没有进入 EGM 会话。请检查 RAPID 事件日志里 "
                u8"「SERVER: EGM session start」之后是否有报错。");
        }
        QMessageBox::critical(this, QString::fromUtf8(u8"EGM 会话未启动"), detail);
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

    const QString defaultDir = defaultPathTxtDir();

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

    // 启动前先确认 SERVER.mod 真的在应答。T_ROB1 停机/报警后 TCP 仍然"连着"，
    // 写入的指令会静默丢失 → 表现为「点了开始、机器人不动、也没有报错」。
    // 顺带把缓冲区里残留的 ACK 读掉，避免后续查询拿到过期应答。
    {
        bool rapidAlive = false;
        for (int attempt = 0; attempt < 3 && !rapidAlive; ++attempt) {
            QString reply;
            if (!robotPrintService_.socket()
                     ->sendRawMessageWithReply(QStringLiteral("0 #"), reply, 800)
                     .ok) {
                continue;
            }
            const QStringList tokens = reply.split(' ', Qt::SkipEmptyParts);
            if (tokens.size() >= 2 && tokens.at(0).toInt() == 0 && tokens.at(1).toInt() == 1) {
                rapidAlive = true;
            }
        }
        if (!rapidAlive) {
            QMessageBox::critical(this, QString::fromUtf8(u8"SERVER.mod 无响应"),
                QString::fromUtf8(u8"TCP 连接还在，但 RAPID 侧的 SERVER.mod 没有应答心跳"
                                   u8"（CASE 0）。\n\n"
                                   u8"这通常表示 T_ROB1 已经停止（报警未清除或程序指针不在 "
                                   u8"main），此时发出去的 EGM 指令会被静默丢弃，机器人不会"
                                   u8"运动。\n\n"
                                   u8"请到示教器：清除报警 → PP to main → 启动 T_ROB1，再重试。"));
            ui_->robot_state_show->setText(
                QString::fromUtf8(u8"EGM 未启动：SERVER.mod 无响应（T_ROB1 可能已停止）。"));
            return;
        }
    }

    // 关节模式下 EGM 发送的是绝对关节角（单位：度），首帧必须等于机器人当前
    // 关节角，否则 EGMActJoint 的限幅会立刻判定超差、会话秒退。
    // 注意：txt 与 EGM 关节报文都用度，这里不做角度/弧度换算。
    QString poseLiveNote;
    QString poseOffsetNote;
    if (jointPath) {
        float startJoints[6] = {0, 0, 0, 0, 0, 0};
        pathLoader_.jointPositionAt(0, 0.0f, startJoints);
        double jointsDeg[6];
        for (int axis = 0; axis < 6; ++axis) {
            jointsDeg[axis] = static_cast<double>(startJoints[axis]);
        }
        egmClient_.setMode(EgmClient::Mode::Joint);
        egmClient_.setTargetJoints(jointsDeg);
        egmStartContext_ = QStringLiteral("file=%1 mode=joint")
                               .arg(QFileInfo(loadedPathFile_).fileName());
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
        egmPathOffsetMm_ = PathVector3{};

        // 当前真实位姿有两个来源：
        //   ① 本轮已经跑过 EGM —— 控制器反馈的真实 TCP 位姿（本地缓存）；
        //   ② SERVER.mod CASE 113 查询。
        // 实测：上一轮 EGM 会话结束后，113 会返回「位置相近、姿态偏 30°~40°」的
        // 错误读数；而查询本身还要等 TCP 超时（默认 3 s），连点几次就会把界面
        // 卡住十几秒。因此有缓存时直接用缓存，完全不查 113。
        QString liveReply;
        double liveX = 0.0;
        double liveY = 0.0;
        double liveZ = 0.0;
        PathQuaternion liveQuat{1, 0, 0, 0};
        bool haveLive = false;
        QString liveSource;

        if (hasLastFeedbackPose_) {
            liveX = lastFeedbackPose_.x;
            liveY = lastFeedbackPose_.y;
            liveZ = lastFeedbackPose_.z;
            liveQuat = PathQuaternion{static_cast<float>(lastFeedbackPose_.q1),
                                      static_cast<float>(lastFeedbackPose_.q2),
                                      static_cast<float>(lastFeedbackPose_.q3),
                                      static_cast<float>(lastFeedbackPose_.q4)};
            haveLive = true;
            liveSource = QString::fromUtf8(u8"EGM 反馈缓存");
        } else {
            // 仅软件刚启动、还没有任何反馈帧时需要查询。最多 2 次、每次 800 ms，
            // 失败就放弃，绝不用路径文件里的旧姿态顶替。
            ui_->robot_state_show->setText(QString::fromUtf8(u8"正在读取机器人当前位姿…"));
            QCoreApplication::processEvents();
            for (int attempt = 0; attempt < 2 && !haveLive; ++attempt) {
                if (attempt > 0) {
                    QThread::msleep(300);
                }
                QString reply;
                if (!robotPrintService_.socket()
                         ->sendRawMessageWithReply(QStringLiteral("113 #"), reply, 800)
                         .ok) {
                    continue;
                }
                const QStringList tokens = reply.split(' ', Qt::SkipEmptyParts);
                if (tokens.size() < 9) {
                    continue;  // 残留 ACK 之类的短应答，不是位姿
                }
                liveX = tokens.at(2).toDouble();
                liveY = tokens.at(3).toDouble();
                liveZ = tokens.at(4).toDouble();
                liveQuat = PathQuaternion{tokens.at(5).toFloat(), tokens.at(6).toFloat(),
                                          tokens.at(7).toFloat(), tokens.at(8).toFloat()};
                haveLive = true;
                liveSource = QStringLiteral("CASE113");
                liveReply = reply;
            }
        }

        appendPoseReadLog(QStringLiteral("CASE113 '%1' | cache '%2'")
                              .arg(liveReply.isEmpty() ? QString::fromUtf8(u8"<未查询>") : liveReply,
                                   hasLastFeedbackPose_
                                       ? QStringLiteral("%1 %2 %3 %4 %5 %6 %7")
                                             .arg(lastFeedbackPose_.x, 0, 'f', 2)
                                             .arg(lastFeedbackPose_.y, 0, 'f', 2)
                                             .arg(lastFeedbackPose_.z, 0, 'f', 2)
                                             .arg(lastFeedbackPose_.q1, 0, 'f', 4)
                                             .arg(lastFeedbackPose_.q2, 0, 'f', 4)
                                             .arg(lastFeedbackPose_.q3, 0, 'f', 4)
                                             .arg(lastFeedbackPose_.q4, 0, 'f', 4)
                                       : QStringLiteral("<none>")));

        // 位姿来源全都不可用就绝不启动：拿 txt 里的旧姿态或错误读数开流，会让
        // 机器人冲到一个完全错误的位置/姿态，撞关节限位（50027）。
        if (!haveLive) {
            QMessageBox::critical(this, QString::fromUtf8(u8"无法读取机器人当前位姿"),
                QString::fromUtf8(u8"SERVER.mod 未返回可信的 113 位姿读数（已重试 2 次），"
                                   u8"本地也没有上一轮 EGM 的反馈缓存。\n\n"
                                   u8"常见原因：T_ROB1 已停机、报警未清除，或程序指针不在 main。\n\n"
                                   u8"为避免在姿态未确认的情况下启动 EGM，本次不执行。"));
            return;
        }

        {
            const double dx = liveX - static_cast<double>(pathStart.x);
            const double dy = liveY - static_cast<double>(pathStart.y);
            const double dz = liveZ - static_cast<double>(pathStart.z);
            const double dist = std::sqrt(dx * dx + dy * dy + dz * dz);

            // 偏差超过 EGM 的 ±500 mm 补偿范围：这不是"起点微调"，多半是
            // 路径文件坐标系不对、或位姿读数异常。贸然补偿会让机器人猛冲，
            // 直接拒绝。
            if (dist > 500.0) {
                QMessageBox::critical(this,
                    QString::fromUtf8(u8"路径与机器人位置相差过大"),
                    QString::fromUtf8(u8"当前 TCP 与路径首点相差 %1 mm，超出 EGM 允许的 "
                                      u8"±500 mm 补偿范围，本次不执行。\n\n"
                                      u8"请依次确认：\n"
                                      u8"① 选的路径文件与此机器人坐标系（currentTool / "
                                      u8"currentWobj）匹配；\n"
                                      u8"② 机器人确实停在路径起点附近；\n"
                                      u8"③ 示教器上 T_ROB1 正在运行、无未清除报警。")
                        .arg(dist, 0, 'f', 1));
                return;
            }

            // 起始位姿与 txt 首点不一致时，征求确认后再把整条路径整体平移
            // 到机器人当前位置，避免 EGM 一启动就强制机器人“跳”到路径首点。
            if (dist > 0.5) {
                QString questionText = QString::fromUtf8(
                    u8"机器人当前 TCP 与 txt 路径首点不一致：\n\n"
                    u8"路径首点：X %1   Y %2   Z %3\n"
                    u8"当前 TCP：X %4   Y %5   Z %6\n"
                    u8"偏移量：ΔX %7   ΔY %8   ΔZ %9\n"
                    u8"位置偏差：%10 mm\n"
                    u8"位姿来源：%11\n\n"
                    u8"是否将整条路径整体平移该偏移量，"
                    u8"使路径从机器人当前位置开始？")
                    .arg(pathStart.x, 0, 'f', 2)
                    .arg(pathStart.y, 0, 'f', 2)
                    .arg(pathStart.z, 0, 'f', 2)
                    .arg(liveX, 0, 'f', 2)
                    .arg(liveY, 0, 'f', 2)
                    .arg(liveZ, 0, 'f', 2)
                    .arg(dx, 0, 'f', 2)
                    .arg(dy, 0, 'f', 2)
                    .arg(dz, 0, 'f', 2)
                    .arg(dist, 0, 'f', 2)
                    .arg(liveSource);

                // 偏差超过 50 mm 时给出额外安全提示，并改用警告图标。
                const bool largeOffset = dist > 50.0;
                if (largeOffset) {
                    questionText += QString::fromUtf8(
                        u8"\n\n【注意】位置偏差已超过 50 mm。补偿后整条路径会被"
                        u8"平移到机器人当前位置，请先确认机器人周边空间、"
                        u8"工具姿态和工件位置均安全，再继续执行。");
                }

                QMessageBox confirmDialog(this);
                confirmDialog.setWindowTitle(QString::fromUtf8(u8"路径起始位姿不一致"));
                confirmDialog.setIcon(largeOffset ? QMessageBox::Warning
                                                  : QMessageBox::Question);
                confirmDialog.setText(questionText);
                confirmDialog.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
                // 默认落在「No」：误按回车不会把机器人送出去。
                confirmDialog.setDefaultButton(QMessageBox::No);
                if (confirmDialog.exec() != QMessageBox::Yes) {
                    ui_->robot_state_show->setText(
                        QString::fromUtf8(u8"已取消：未对路径做偏移补偿，EGM 未启动。"));
                    return;
                }
                egmPathOffsetMm_ = PathVector3{static_cast<float>(dx),
                                               static_cast<float>(dy),
                                               static_cast<float>(dz)};
                poseOffsetNote = QString::fromUtf8(
                    u8"路径已整体偏移补偿：ΔX %1  ΔY %2  ΔZ %3 mm")
                    .arg(dx, 0, 'f', 2).arg(dy, 0, 'f', 2).arg(dz, 0, 'f', 2);
            }
        }

        // 姿态对齐：把整条路径的姿态旋转到机器人当前姿态。
        // 对齐角超过 10° 时说明路径文件与机器人姿态不匹配（或读数异常），此时
        // 强行对齐会让腕部大角度旋转、TCP 随长工具甩出去，宁可按文件姿态执行。
        egmStartContext_ = QStringLiteral("file=%1 offset=(%2,%3,%4) source=%5")
                               .arg(QFileInfo(loadedPathFile_).fileName())
                               .arg(egmPathOffsetMm_.x, 0, 'f', 2)
                               .arg(egmPathOffsetMm_.y, 0, 'f', 2)
                               .arg(egmPathOffsetMm_.z, 0, 'f', 2)
                               .arg(liveSource);
        const PathQuaternion alignQuat =
            QuatNormalize(QuatMultiply(liveQuat, QuatConjugate(pathStartQuat)));
        const double alignHalfW = std::min(1.0, std::fabs(static_cast<double>(alignQuat.q1)));
        const double alignAngleDeg = 2.0 * std::acos(alignHalfW) * 180.0 / 3.141592653589793;
        if (alignAngleDeg > 10.0) {
            egmAlignValid_ = false;
            egmAlignQuat_ = PathQuaternion{1, 0, 0, 0};
            poseLiveNote = QString::fromUtf8(
                u8"机器人姿态与路径首点相差 %1°（>10°），已忽略姿态对齐，"
                u8"按路径文件姿态执行（来源 %2）")
                .arg(alignAngleDeg, 0, 'f', 1)
                .arg(liveSource);
            appendPoseReadLog(QStringLiteral("skip attitude align: %1 deg, source %2")
                                  .arg(alignAngleDeg, 0, 'f', 2)
                                  .arg(liveSource));
        } else {
            egmAlignQuat_ = alignQuat;
            egmAlignValid_ = true;
            poseLiveNote = QString::fromUtf8(u8"已按真实姿态对齐（%1）：[%2, %3, %4, %5]，偏差 %6°")
                               .arg(liveSource)
                               .arg(liveQuat.q1, 0, 'f', 4)
                               .arg(liveQuat.q2, 0, 'f', 4)
                               .arg(liveQuat.q3, 0, 'f', 4)
                               .arg(liveQuat.q4, 0, 'f', 4)
                               .arg(alignAngleDeg, 0, 'f', 2);
        }

        // 位姿模式首帧不能是 EgmClient 内部的 (0,0,0)+单位四元数，否则与当前
        // TCP 相差上千米，EGM 偏移立刻超限 → 会话终止并急停。
        const PathQuaternion startQuat = egmAlignValid_
            ? QuatNormalize(QuatMultiply(egmAlignQuat_, pathStartQuat))
            : pathStartQuat;
        egmClient_.setTargetPose(
            EgmTcpPose{pathStart.x + egmPathOffsetMm_.x,
                       pathStart.y + egmPathOffsetMm_.y,
                       pathStart.z + egmPathOffsetMm_.z,
                       startQuat.q1, startQuat.q2,
                       startQuat.q3, startQuat.q4});
    }

    // 翻转 T_ROB1 的 EGM 启动标志（SERVER.mod CASE 110 / 120）；ACK 返回后
    // RAPID 才真正进入阻塞的 EGM 会话。这里要求拿到应答后再开流，否则指令
    // 可能根本没被处理。
    const QString startCommand = jointPath ? QStringLiteral("120 #") : QStringLiteral("110 #");
    const int startCode = jointPath ? 120 : 110;
    bool startAckOk = false;
    QString startAck;
    for (int attempt = 0; attempt < 3 && !startAckOk; ++attempt) {
        QString reply;
        if (!robotPrintService_.socket()
                 ->sendRawMessageWithReply(startCommand, reply, 1500)
                 .ok) {
            continue;  // 完全没应答，可以安全重发
        }
        // 收到应答后不再重发：RAPID 的 ACK 之后立刻就进了阻塞的 EGM 会话，
        // 会话期间再排一条 110 会在会话结束后被误当成新指令。
        startAck = reply;
        const QStringList tokens = reply.split(' ', Qt::SkipEmptyParts);
        if (tokens.size() >= 2 && tokens.at(0).toInt() == startCode && tokens.at(1).toInt() == 1) {
            startAckOk = true;
        }
        break;
    }
    if (!startAckOk) {
        QMessageBox::critical(this, QString::fromUtf8(u8"发送 EGM 启动指令失败"),
            QString::fromUtf8(u8"SERVER.mod 没有确认 EGM 启动指令（%1 %2），"
                               u8"机器人不会运动。%3")
                .arg(startCode)
                .arg(jointPath ? QStringLiteral("120") : QStringLiteral("110"))
                .arg(startAck.isEmpty() ? QString::fromUtf8(u8"（无应答）") : startAck));
        return;
    }

    // EGM 会话开始前开一个新 CSV，记录控制器实时反馈的 x/y/z/q1..q3。
    startEgmCsvRecording();

    if (!egmClient_.start()) {
        QMessageBox::critical(this, QString::fromUtf8(u8"EGM 客户端启动失败"),
            QString::fromUtf8(u8"本地 UDP 端口没有绑定成功（boundPort=%1）。\n\n"
                               u8"6510 可能被占用：上一次残留的上位机进程、Python 旧版程序、"
                               u8"或其它占用该端口的程序需要先关掉。")
                .arg(egmClient_.boundPort()));
        stopEgmCsvRecording();
        QString ack;
        robotPrintService_.socket()->sendRawMessageWithReply(QStringLiteral("111 #"), ack, 1000);
        return;
    }

    egmFlowActive_ = true;
    egmStartWatchdog_.start(2500);
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
        if (!poseOffsetNote.isEmpty()) {
            status += QString::fromUtf8(u8"\n%1").arg(poseOffsetNote);
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
    // EGM 的 planned.cartesian 是绝对位姿（工具/工件坐标系见 RAPID 端 EGMActPose）。
    // 用户确认过的整体偏移补偿在这里叠加：整条路径平移到机器人实际起点。
    egmClient_.setTargetPose(
        EgmTcpPose{pos.x + egmPathOffsetMm_.x,
                   pos.y + egmPathOffsetMm_.y,
                   pos.z + egmPathOffsetMm_.z,
                   quat.q1, quat.q2, quat.q3, quat.q4});
}

void MainWindow::stopEgmFlow() {
    egmStartWatchdog_.stop();
    egmTimer_.stop();
    egmClient_.stop();
    stopEgmCsvRecording();

    if (robotPrintService_.socket() != nullptr && robotPrintService_.socket()->isConnected()) {
        // 用带应答的版本把 ACK 读掉：残留的 "111 1" 留在缓冲区里会被后续查询
        // 当成 113 的应答，造成读数错位。
        QString ack;
        robotPrintService_.socket()->sendRawMessageWithReply(QStringLiteral("111 #"), ack, 1000);
    }

    egmFlowActive_ = false;
}

// ---- EGM 反馈位姿记录：data_save/data_年月日_时分秒.csv ----
void MainWindow::startEgmCsvRecording() {
    stopEgmCsvRecording();

    const QString name = QStringLiteral("data_%1.csv").arg(
        QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
    egmCsvFile_.setFileName(QDir(dataSaveDir()).absoluteFilePath(name));
    if (!egmCsvFile_.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        ui_->robot_state_show->setText(
            QString::fromUtf8(u8"EGM 反馈记录无法写入：%1").arg(egmCsvFile_.fileName()));
        return;
    }

    egmCsvStream_.setDevice(&egmCsvFile_);
    egmCsvStream_ << "x,y,z,q1,q2,q3\n";
    egmCsvStream_.flush();
    egmCsvRecording_ = true;
    egmCsvRowsSinceFlush_ = 0;
}

void MainWindow::stopEgmCsvRecording() {
    if (!egmCsvRecording_) return;

    egmCsvRecording_ = false;
    egmCsvStream_.flush();
    egmCsvStream_.setDevice(nullptr);
    egmCsvFile_.close();
}

void MainWindow::onEgmFeedbackPose(const EgmTcpPose& pose) {
    // 先缓存控制器反馈的真实位姿：即使 CSV 已经不记录（路径走完/手动停止），
    // 下一轮 EGM 启动也要用它做起点补偿与姿态对齐。
    lastFeedbackPose_ = pose;
    hasLastFeedbackPose_ = true;

    if (!egmCsvRecording_) return;

    egmCsvStream_ << QString::number(pose.x, 'f', 3) << ','
                  << QString::number(pose.y, 'f', 3) << ','
                  << QString::number(pose.z, 'f', 3) << ','
                  << QString::number(pose.q1, 'f', 6) << ','
                  << QString::number(pose.q2, 'f', 6) << ','
                  << QString::number(pose.q3, 'f', 6) << '\n';

    // 每 250 帧（约 1 秒）落一次盘，兼顾实时性与磁盘写入开销。
    if (++egmCsvRowsSinceFlush_ >= 250) {
        egmCsvStream_.flush();
        egmCsvRowsSinceFlush_ = 0;
    }
}
