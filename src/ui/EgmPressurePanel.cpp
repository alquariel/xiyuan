#include "ui/EgmPressurePanel.h"

#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "egm/EgmPressureController.h"

QLabel* makeStateLabel() {
    auto* label = new QLabel(QStringLiteral("--"));
    label->setMinimumWidth(70);
    return label;
}

void setStateStyle(QLabel* label, const QString& text, bool ok) {
    label->setText(text);
    label->setStyleSheet(ok ? "color: #2e7d32; font-weight: bold;"
                            : "color: #c62828; font-weight: bold;");
}

EgmPressurePanel::EgmPressurePanel(const EgmPressureConfig& config, QWidget* parent)
    : QWidget(parent), config_(config) {
    controller_ = new egm::EgmPressureController(config_, this);

    buildUi();
    applyParameters();

    connect(controller_, &egm::EgmPressureController::statusChanged, this,
            [this](const egm::EgmPressureController::Status& status) {
                updateLabels(status);
            });
    updateButtonState();
}

EgmPressurePanel::~EgmPressurePanel() = default;

void EgmPressurePanel::buildUi() {
    auto* root = new QVBoxLayout(this);

    auto* statusGroup = new QGroupBox(QStringLiteral("连接状态"), this);
    auto* statusLayout = new QGridLayout(statusGroup);
    statusLayout->addWidget(new QLabel(QStringLiteral("压力 DDS"), this), 0, 0);
    pressureStateLabel_ = makeStateLabel();
    statusLayout->addWidget(pressureStateLabel_, 0, 1);
    statusLayout->addWidget(new QLabel(QStringLiteral("位姿 DDS"), this), 0, 2);
    robotStateLabel_ = makeStateLabel();
    statusLayout->addWidget(robotStateLabel_, 0, 3);
    statusLayout->addWidget(new QLabel(QStringLiteral("EGM 链路"), this), 0, 4);
    egmStateLabel_ = makeStateLabel();
    statusLayout->addWidget(egmStateLabel_, 0, 5);

    pressureValueLabel_ = new QLabel(QStringLiteral("压力均值 --"), this);
    correctionValueLabel_ = new QLabel(QStringLiteral("修正输出 -- mm"), this);
    frameCountLabel_ = new QLabel(QStringLiteral("收 0 / 发 0"), this);
    statusLayout->addWidget(pressureValueLabel_, 1, 0, 1, 2);
    statusLayout->addWidget(correctionValueLabel_, 1, 2, 1, 2);
    statusLayout->addWidget(frameCountLabel_, 1, 4, 1, 2);
    root->addWidget(statusGroup);

    auto* paramGroup = new QGroupBox(QStringLiteral("控制参数（压力为传感器原始单位）"), this);
    auto* paramLayout = new QGridLayout(paramGroup);

    const auto addSpin = [this, paramLayout, paramGroup](const QString& text, int row, int column,
                                                         QDoubleSpinBox** target, double value,
                                                         double minimum, double maximum,
                                                         double step, int decimals) {
        paramLayout->addWidget(new QLabel(text, paramGroup), row, column * 2);
        auto* spin = new QDoubleSpinBox(paramGroup);
        spin->setRange(minimum, maximum);
        spin->setSingleStep(step);
        spin->setDecimals(decimals);
        spin->setValue(value);
        paramLayout->addWidget(spin, row, column * 2 + 1);
        *target = spin;
    };

    addSpin(QStringLiteral("目标压力"), 0, 0, &setpointSpin_, config_.pressureSetpoint,
            -1.0e6, 1.0e6, 0.5, 3);
    addSpin(QStringLiteral("Kp"), 0, 1, &kpSpin_, config_.kp, 0.0, 1000.0, 0.001, 4);
    addSpin(QStringLiteral("Ki"), 0, 2, &kiSpin_, config_.ki, 0.0, 10000.0, 0.001, 4);
    addSpin(QStringLiteral("Kd"), 0, 3, &kdSpin_, config_.kd, 0.0, 10000.0, 0.001, 4);
    addSpin(QStringLiteral("Z 限幅 (mm)"), 1, 0, &zMaxSpin_, config_.zMaxMm, 0.01, 20.0, 0.1, 2);
    addSpin(QStringLiteral("单周期限幅 (mm)"), 1, 1, &dzMaxSpin_, config_.dzMaxPerCycleMm, 0.001,
            5.0, 0.01, 3);

    enableCorrectionButton_ = new QPushButton(QStringLiteral("启用修正"), paramGroup);
    stopCorrectionButton_ = new QPushButton(QStringLiteral("停止修正"), paramGroup);
    paramLayout->addWidget(enableCorrectionButton_, 1, 4, 1, 2);
    paramLayout->addWidget(stopCorrectionButton_, 1, 6, 1, 2);

    startButton_ = new QPushButton(QStringLiteral("开始监听"), paramGroup);
    stopButton_ = new QPushButton(QStringLiteral("停止监听"), paramGroup);
    zeroButton_ = new QPushButton(QStringLiteral("一键回零"), paramGroup);
    paramLayout->addWidget(startButton_, 2, 0, 1, 2);
    paramLayout->addWidget(stopButton_, 2, 2, 1, 2);
    paramLayout->addWidget(zeroButton_, 2, 4, 1, 2);
    root->addWidget(paramGroup);

    errorLabel_ = new QLabel(this);
    errorLabel_->setWordWrap(true);
    errorLabel_->setStyleSheet("color: #c62828;");
    root->addWidget(errorLabel_);

    connect(startButton_, &QPushButton::clicked, this, [this]() {
        if (!controller_->startListening()) {
            const egm::EgmPressureController::Status status = controller_->status();
            errorLabel_->setText(status.lastError.isEmpty()
                                     ? QStringLiteral("启动失败：无法启动 DDS 订阅。")
                                     : status.lastError);
        } else {
            errorLabel_->clear();
        }
        updateButtonState();
    });
    connect(stopButton_, &QPushButton::clicked, this, [this]() {
        controller_->stopListening();
        updateButtonState();
    });
    connect(zeroButton_, &QPushButton::clicked, this, [this]() {
        controller_->requestZero();
        updateButtonState();
    });
    connect(enableCorrectionButton_, &QPushButton::clicked, this, [this]() {
        controller_->setCorrectionEnabled(true);
        updateButtonState();
    });
    connect(stopCorrectionButton_, &QPushButton::clicked, this, [this]() {
        controller_->setCorrectionEnabled(false);
        updateButtonState();
    });

    connect(setpointSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &EgmPressurePanel::applyParameters);
    connect(kpSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &EgmPressurePanel::applyParameters);
    connect(kiSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &EgmPressurePanel::applyParameters);
    connect(kdSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &EgmPressurePanel::applyParameters);
    connect(zMaxSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &EgmPressurePanel::applyParameters);
    connect(dzMaxSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &EgmPressurePanel::applyParameters);
}

void EgmPressurePanel::applyParameters() {
    if (controller_ == nullptr) {
        return;
    }
    controller_->setControlParameters(setpointSpin_->value(), kpSpin_->value(), kiSpin_->value(),
                                      kdSpin_->value(), zMaxSpin_->value(),
                                      dzMaxSpin_->value());
}

void EgmPressurePanel::updateButtonState() {
    const bool listening = controller_->isListening();
    const bool correcting = controller_->isCorrectionEnabled();
    startButton_->setEnabled(!listening);
    stopButton_->setEnabled(listening);
    zeroButton_->setEnabled(listening);
    enableCorrectionButton_->setEnabled(listening && !correcting);
    stopCorrectionButton_->setEnabled(listening && correcting);
}

void EgmPressurePanel::updateLabels(const egm::EgmPressureController::Status& status) {
    setStateStyle(pressureStateLabel_, status.ddsPressureOk ? QStringLiteral("正常")
                                                           : QStringLiteral("无数据"),
                  status.ddsPressureOk);
    setStateStyle(robotStateLabel_,
                  status.ddsRobotOk ? QStringLiteral("正常") : QStringLiteral("无数据"),
                  status.ddsRobotOk);
    if (status.linkTimeout) {
        setStateStyle(egmStateLabel_, QStringLiteral("超时"), false);
    } else if (status.egmConnected) {
        setStateStyle(egmStateLabel_, QStringLiteral("已连接"), true);
    } else {
        setStateStyle(egmStateLabel_, QStringLiteral("等待"), false);
    }

    if (status.ddsPressureOk) {
        pressureValueLabel_->setText(
            QStringLiteral("压力均值 %1").arg(status.pressureMean, 0, 'f', 3));
    } else if (status.pressureAgeMs < 1e17) {
        pressureValueLabel_->setText(
            QStringLiteral("压力均值 --（样本 %1 ms）").arg(status.pressureAgeMs, 0, 'f', 0));
    } else {
        pressureValueLabel_->setText(QStringLiteral("压力均值 --"));
    }
    correctionValueLabel_->setText(
        QStringLiteral("修正输出 %1 mm").arg(status.zCorrectionMm, 0, 'f', 3));
    frameCountLabel_->setText(
        QStringLiteral("收 %1 / 发 %2").arg(status.framesReceived).arg(status.framesSent));

    if (!status.lastError.isEmpty()) {
        errorLabel_->setText(status.lastError);
    }
    updateButtonState();
}
