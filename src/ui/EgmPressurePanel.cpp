#include "ui/EgmPressurePanel.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QVector>
#include <QVBoxLayout>

#include <algorithm>

#include "egm/EgmPressureController.h"

class TrendPlot final : public QWidget {
public:
    TrendPlot(QString title, QColor color, QWidget* parent = nullptr)
        : QWidget(parent), title_(std::move(title)), color_(color) {
        setMinimumHeight(170);
    }

    void addSample(double timeSeconds, double value) {
        points_.append(QPointF(timeSeconds, value));
        while (!points_.isEmpty() && timeSeconds - points_.first().x() > kWindowSeconds) {
            points_.removeFirst();
        }
        update();
    }

    void setReference(double value, bool visible) {
        reference_ = value;
        hasReference_ = visible;
        update();
    }

    void clear() {
        points_.clear();
        update();
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.fillRect(rect(), QColor(30, 30, 30));

        painter.setPen(Qt::white);
        painter.drawText(QRectF(6, 2, width() - 12, 18), Qt::AlignLeft | Qt::AlignVCenter, title_);

        const QRectF area = QRectF(rect()).adjusted(42, 24, -10, -20);

        double yMin = 0.0;
        double yMax = 0.0;
        bool hasData = false;
        for (const QPointF& point : points_) {
            if (!hasData) {
                yMin = yMax = point.y();
                hasData = true;
            } else {
                yMin = std::min(yMin, point.y());
                yMax = std::max(yMax, point.y());
            }
        }
        if (hasReference_) {
            if (!hasData) {
                yMin = yMax = reference_;
                hasData = true;
            } else {
                yMin = std::min(yMin, reference_);
                yMax = std::max(yMax, reference_);
            }
        }
        if (!hasData) {
            yMin = -1.0;
            yMax = 1.0;
        }
        if (yMax - yMin < 1e-6) {
            yMin -= 0.5;
            yMax += 0.5;
        }
        const double padding = (yMax - yMin) * 0.1;
        yMin -= padding;
        yMax += padding;

        const double tMax = points_.isEmpty() ? 0.0 : points_.last().x();
        const double tMin = std::max(0.0, tMax - kWindowSeconds);
        const double tSpan = std::max(0.5, tMax - tMin);

        painter.setPen(QColor(58, 58, 58));
        for (int i = 0; i <= 4; ++i) {
            const double y = area.top() + area.height() * i / 4.0;
            painter.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
        }
        for (int i = 0; i <= 4; ++i) {
            const double x = area.left() + area.width() * i / 4.0;
            painter.drawLine(QPointF(x, area.top()), QPointF(x, area.bottom()));
        }

        const auto mapY = [&](double value) {
            return area.bottom() - (value - yMin) / (yMax - yMin) * area.height();
        };

        painter.setFont(QFont(font().family(), 8));
        painter.setPen(QColor(160, 160, 160));
        painter.drawText(QRectF(0, area.top() - 8, 38, 16), Qt::AlignRight,
                         QString::number(yMax, 'f', 2));
        painter.drawText(QRectF(0, area.bottom() - 8, 38, 16), Qt::AlignRight,
                         QString::number(yMin, 'f', 2));
        painter.drawText(QRectF(area.left(), area.bottom() + 2, 60, 16), Qt::AlignLeft,
                         QString("-60s"));
        painter.drawText(QRectF(area.right() - 60, area.bottom() + 2, 60, 16), Qt::AlignRight,
                         QString("now"));

        if (hasReference_) {
            QPen referencePen(QColor(150, 150, 150));
            referencePen.setStyle(Qt::DashLine);
            painter.setPen(referencePen);
            const double y = mapY(reference_);
            painter.drawLine(QPointF(area.left(), y), QPointF(area.right(), y));
        }

        if (points_.size() >= 2) {
            QPolygonF polyline;
            polyline.reserve(points_.size());
            for (const QPointF& point : points_) {
                polyline.append(QPointF(area.left() + (point.x() - tMin) / tSpan * area.width(),
                                        mapY(point.y())));
            }
            painter.setPen(QPen(color_, 2.0));
            painter.drawPolyline(polyline);
        }
    }

private:
    static constexpr double kWindowSeconds = 60.0;

    QString title_;
    QColor color_;
    QVector<QPointF> points_;
    double reference_ = 0.0;
    bool hasReference_ = false;
};

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
    clock_.start();

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
    addSpin(QStringLiteral("Z 限幅 (mm)"), 1, 0, &zMaxSpin_, config_.zMaxMm, 0.01, 20.0, 0.1, 2);
    addSpin(QStringLiteral("单周期限幅 (mm)"), 1, 1, &dzMaxSpin_, config_.dzMaxPerCycleMm, 0.001,
            5.0, 0.01, 3);

    correctionCheck_ = new QCheckBox(QStringLiteral("启用修正"), paramGroup);
    paramLayout->addWidget(correctionCheck_, 1, 4, 1, 2);

    startButton_ = new QPushButton(QStringLiteral("开始监听"), paramGroup);
    stopButton_ = new QPushButton(QStringLiteral("停止监听"), paramGroup);
    zeroButton_ = new QPushButton(QStringLiteral("一键回零"), paramGroup);
    paramLayout->addWidget(startButton_, 2, 0, 1, 2);
    paramLayout->addWidget(stopButton_, 2, 2, 1, 2);
    paramLayout->addWidget(zeroButton_, 2, 4, 1, 2);
    root->addWidget(paramGroup);

    auto* plotLayout = new QHBoxLayout();
    pressurePlot_ = new TrendPlot(QStringLiteral("压力均值"), QColor(80, 180, 255), this);
    correctionPlot_ = new TrendPlot(QStringLiteral("Z 修正 (mm)"), QColor(255, 170, 60), this);
    plotLayout->addWidget(pressurePlot_, 1);
    plotLayout->addWidget(correctionPlot_, 1);
    root->addLayout(plotLayout, 1);

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
        correctionCheck_->setChecked(false);
        updateButtonState();
    });
    connect(zeroButton_, &QPushButton::clicked, this, [this]() {
        controller_->requestZero();
        correctionCheck_->setChecked(false);
    });
    connect(correctionCheck_, &QCheckBox::toggled, this, [this](bool checked) {
        controller_->setCorrectionEnabled(checked);
    });

    connect(setpointSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &EgmPressurePanel::applyParameters);
    connect(kpSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            &EgmPressurePanel::applyParameters);
    connect(kiSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
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
                                      zMaxSpin_->value(), dzMaxSpin_->value());
    correctionPlot_->setReference(0.0, true);
    pressurePlot_->setReference(setpointSpin_->value(), true);
}

void EgmPressurePanel::updateButtonState() {
    const bool listening = controller_->isListening();
    startButton_->setEnabled(!listening);
    stopButton_->setEnabled(listening);
    zeroButton_->setEnabled(listening);
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

    if (status.listening && status.ddsPressureOk) {
        const double seconds = clock_.elapsed() / 1000.0;
        pressurePlot_->addSample(seconds, status.pressureMean);
        correctionPlot_->addSample(seconds, status.zCorrectionMm);
    }

    if (!status.lastError.isEmpty()) {
        errorLabel_->setText(status.lastError);
    }
    updateButtonState();
}
