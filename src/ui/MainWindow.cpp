#include "ui/MainWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QGuiApplication>
#include <QIcon>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QScreen>
#include <memory>
#include "thermal_camera/ThermalCameraClient.h"
#include "thermal_camera/ThermalFrameStore.h"
#include "ui_Interface4600_window.h"

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
          this) {
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
    // Keep process orchestration out of the constructor so the main window can
    // be created, shown, and painted before slow external tools begin startup.
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
    connect(ui_->Robot_movebutton_Gcode, &QPushButton::clicked, &robotPrintService_, &RobotPrintService::startPrint);
    connect(ui_->PauseRobotButton, &QPushButton::clicked, &robotPrintService_, &RobotPrintService::pausePrint);
    connect(ui_->ContinueRobotButton, &QPushButton::clicked, &robotPrintService_, &RobotPrintService::continuePrint);
    connect(ui_->StopRobotButton, &QPushButton::clicked, &robotPrintService_, &RobotPrintService::stopPrint);

    connect(ui_->locusselectsetButton, &QPushButton::clicked, &ftpService_, &FtpService::openLocus);

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
}

void MainWindow::centerOnScreen() {
    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        const QRect area = screen->availableGeometry();
        move(area.center() - rect().center());
    }
}

void MainWindow::mousePressEvent(QMouseEvent* event) {
    // The Python main window handled thermal ROI clicks globally. Forwarding the
    // event here preserves that behavior without changing the .ui design. The
    // service expects coordinates local to the thermal preview label, not the
    // whole main window.
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
