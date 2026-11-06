#pragma once

#include <QObject>

#include <exception>
#include <functional>
#include <memory>

#include "app/AppConfig.h"
#include "plc/PlcClient.h"
#include "robot/PlcRobotMotionClient.h"
#include "robot/SocketRobotMotionClient.h"

class QLineEdit;
class QThread;

QT_BEGIN_NAMESPACE
namespace Ui { class ContinuosfiberInterface; }
QT_END_NAMESPACE

class RobotMovementService final : public QObject {
    Q_OBJECT

public:
    RobotMovementService(const RobotConfig& robotConfig,
                         const PlcConfig& plcConfig,
                         Ui::ContinuosfiberInterface* ui,
                         QObject* parent = nullptr);
    RobotMovementService(const RobotConfig& robotConfig,
                         IRobotMotionController& motionClient,
                         Ui::ContinuosfiberInterface* ui,
                         QObject* parent = nullptr);
    ~RobotMovementService() override;

signals:
    void messageRequested(const QString& title, const QString& message);

public slots:
    void moveRobot();
    void resetRobot();
    void moveRobotAxes();
    void moveExternalAxis();
    void resetExternalAxis();

private:
    bool readRequiredDouble(QLineEdit* field, const QString& name, double& value);
    bool readOptionalSpeedField(QLineEdit* field, const QString& name, double& value);
    bool readSpeed(RobotSpeed& speed);
    void runMotionCommand(const QString& action,
                          const RobotSpeed& speed,
                          std::function<RobotMotionResult()> motionTask);
    void reportMotionResult(const QString& action, const RobotMotionResult& result);
    void reportMotionException(const QString& action, const std::exception& error);
    void reportUnknownMotionException(const QString& action);

    Ui::ContinuosfiberInterface* ui_ = nullptr;
    RobotConfig robotConfig_;
    std::unique_ptr<PlcClient> ownedPlc_;
    std::unique_ptr<PlcRobotMotionClient> ownedMotionClient_;
    std::unique_ptr<SocketRobotMotionClient> ownedSocketMotionClient_;
    IRobotMotionController* motionClient_ = nullptr;
    QThread* motionThread_ = nullptr;
    bool motionRunning_ = false;
};
