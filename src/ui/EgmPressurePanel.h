#pragma once

#include <QWidget>

#include "app/AppConfig.h"
#include "egm/EgmPressureController.h"

class QDoubleSpinBox;
class QLabel;
class QPushButton;

// 主界面「EGM 压力修正」页签：参数整定、启停监听与状态显示。
class EgmPressurePanel final : public QWidget {
    Q_OBJECT

public:
    explicit EgmPressurePanel(const EgmPressureConfig& config, QWidget* parent = nullptr);
    ~EgmPressurePanel() override;

private:
    void buildUi();
    void applyParameters();
    void updateButtonState();
    void updateLabels(const egm::EgmPressureController::Status& status);

    EgmPressureConfig config_;
    egm::EgmPressureController* controller_ = nullptr;

    QDoubleSpinBox* setpointSpin_ = nullptr;
    QDoubleSpinBox* kpSpin_ = nullptr;
    QDoubleSpinBox* kiSpin_ = nullptr;
    QDoubleSpinBox* kdSpin_ = nullptr;
    QDoubleSpinBox* zMaxSpin_ = nullptr;
    QDoubleSpinBox* dzMaxSpin_ = nullptr;

    QPushButton* enableCorrectionButton_ = nullptr;
    QPushButton* stopCorrectionButton_ = nullptr;
    QPushButton* startButton_ = nullptr;
    QPushButton* stopButton_ = nullptr;
    QPushButton* zeroButton_ = nullptr;

    QLabel* pressureStateLabel_ = nullptr;
    QLabel* robotStateLabel_ = nullptr;
    QLabel* egmStateLabel_ = nullptr;
    QLabel* pressureValueLabel_ = nullptr;
    QLabel* correctionValueLabel_ = nullptr;
    QLabel* frameCountLabel_ = nullptr;
    QLabel* errorLabel_ = nullptr;
};
