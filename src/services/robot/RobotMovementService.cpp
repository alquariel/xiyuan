#include "services/robot/RobotMovementService.h"

#include <array>
#include <cmath>
#include <exception>

#include <QLineEdit>
#include <QMetaObject>
#include <QString>
#include <QThread>

#include "robot/RobotQuaternion.h"
#include "ui_Interface4600_window.h"

RobotMovementService::RobotMovementService(const RobotConfig& robotConfig,
                                           const PlcConfig& plcConfig,
                                           Ui::ContinuosfiberInterface* ui,
                                           QObject* parent)
    : QObject(parent),
      ui_(ui),
      robotConfig_(robotConfig),
      ownedPlc_(std::make_unique<PlcClient>(plcConfig)),
      ownedMotionClient_(std::make_unique<PlcRobotMotionClient>(*ownedPlc_)),
      motionClient_(*ownedMotionClient_) {
}

RobotMovementService::RobotMovementService(const RobotConfig& robotConfig,
                                           IRobotMotionController& motionClient,
                                           Ui::ContinuosfiberInterface* ui,
                                           QObject* parent)
    : QObject(parent),
      ui_(ui),
      robotConfig_(robotConfig),
      motionClient_(motionClient) {
}

RobotMovementService::~RobotMovementService() {
    if (motionThread_ != nullptr) {
        QThread* thread = motionThread_;
        motionThread_ = nullptr;
        disconnect(thread, nullptr, this, nullptr);
        thread->wait();
        delete thread;
    }
}

// Reads the six Cartesian/orientation fields, converts the Python-compatible
// orientation vector into an ABB quaternion, applies UI/config speeds, and
// sends the pose through the injected PLC motion controller.
void RobotMovementService::moveRobot() {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double rx = 0.0;
    double ry = 0.0;
    double rz = 0.0;
    if (!readRequiredDouble(ui_->RobotXlineEdit, "Robot X", x)
        || !readRequiredDouble(ui_->RobotYlineEdit, "Robot Y", y)
        || !readRequiredDouble(ui_->RobotZlineEdit, "Robot Z", z)
        || !readRequiredDouble(ui_->RobotRXlineEdit, "Robot RX", rx)
        || !readRequiredDouble(ui_->RobotRYlineEdit, "Robot RY", ry)
        || !readRequiredDouble(ui_->RobotRZlineEdit, "Robot RZ", rz)) {
        return;
    }

    RobotPose pose;
    try {
        pose = RobotQuaternion::makePose(x, y, z, rx, ry, rz);
    } catch (const std::exception& error) {
        emit messageRequested("Robot control", QString("Invalid robot orientation: %1").arg(error.what()));
        return;
    }

    RobotSpeed speed;
    if (!readSpeed(speed)) {
        return;
    }

    runMotionCommand("move robot", speed, [this, pose]() {
        return motionClient_.moveCartesian(pose);
    });
}

// Sends the robot to the UI zero position. The [0, 0, -1] orientation vector
// matches the original Python reset/home behavior before quaternion conversion.
void RobotMovementService::resetRobot() {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    if (!readRequiredDouble(ui_->zero_position_x, "Zero position X", x)
        || !readRequiredDouble(ui_->zero_position_y, "Zero position Y", y)
        || !readRequiredDouble(ui_->zero_position_z, "Zero position Z", z)) {
        return;
    }

    RobotPose pose;
    try {
        pose = RobotQuaternion::makePose(x, y, z, 0.0, 0.0, -1.0);
    } catch (const std::exception& error) {
        emit messageRequested("Robot control", QString("Invalid robot home orientation: %1").arg(error.what()));
        return;
    }

    RobotSpeed speed;
    if (!readSpeed(speed)) {
        return;
    }

    runMotionCommand("reset robot", speed, [this, pose]() {
        return motionClient_.moveCartesian(pose);
    });
}

// Reads the six joint axis fields in UI order and forwards them unchanged to
// the PLC motion client as the robot absolute-joint command payload.
void RobotMovementService::moveRobotAxes() {
    std::array<double, 6> axes{};
    if (!readRequiredDouble(ui_->axis1lineEdit, "Axis 1", axes[0])
        || !readRequiredDouble(ui_->axis2lineEdit, "Axis 2", axes[1])
        || !readRequiredDouble(ui_->axis3lineEdit, "Axis 3", axes[2])
        || !readRequiredDouble(ui_->axis4lineEdit, "Axis 4", axes[3])
        || !readRequiredDouble(ui_->axis5lineEdit, "Axis 5", axes[4])
        || !readRequiredDouble(ui_->axis6lineEdit, "Axis 6", axes[5])) {
        return;
    }

    RobotSpeed speed;
    if (!readSpeed(speed)) {
        return;
    }

    runMotionCommand("move robot axes", speed, [this, axes]() {
        return motionClient_.moveJoints(axes);
    });
}

// Preserves the Python positioner ordering: UI fields Axis1/Axis2/Axis3 are
// sent to the PLC as [Axis3, Axis1, Axis2, 0, 0, 0].
void RobotMovementService::moveExternalAxis() {
    double axis1 = 0.0;
    double axis2 = 0.0;
    double axis3 = 0.0;
    if (!readRequiredDouble(ui_->Positioner1lineEdit, "Positioner axis 1", axis1)
        || !readRequiredDouble(ui_->Positioner2lineEdit, "Positioner axis 2", axis2)
        || !readRequiredDouble(ui_->Positioner3lineEdit, "Positioner axis 3", axis3)) {
        return;
    }

    RobotSpeed speed;
    if (!readSpeed(speed)) {
        return;
    }

    runMotionCommand("move external axis", speed, [this, axis1, axis2, axis3]() {
        return motionClient_.moveExternalAxis({axis3, axis1, axis2, 0.0, 0.0, 0.0});
    });
}

// Returns the external positioner to the same Python-compatible reset payload
// used by the legacy UI workflow: [-1900, 0, 0, 0, 0, 0].
void RobotMovementService::resetExternalAxis() {
    RobotSpeed speed;
    if (!readSpeed(speed)) {
        return;
    }

    runMotionCommand("reset external axis", speed, [this]() {
        return motionClient_.moveExternalAxis({-1900.0, 0.0, 0.0, 0.0, 0.0, 0.0});
    });
}

bool RobotMovementService::readRequiredDouble(QLineEdit* field, const QString& name, double& value) {
    const QString text = field != nullptr ? field->text().trimmed() : QString();
    bool ok = false;
    const double parsed = text.toDouble(&ok);
    if (text.isEmpty() || !ok || !std::isfinite(parsed)) {
        emit messageRequested("Robot control", QString("%1 must be a valid number.").arg(name));
        return false;
    }

    value = parsed;
    return true;
}

bool RobotMovementService::readOptionalSpeedField(QLineEdit* field, const QString& name, double& value) {
    const QString text = field != nullptr ? field->text().trimmed() : QString();
    if (text.isEmpty()) {
        return true;
    }

    bool ok = false;
    const double parsed = text.toDouble(&ok);
    if (!ok || !std::isfinite(parsed) || parsed <= 0.0) {
        emit messageRequested("Robot control", QString("%1 must be a positive speed number.").arg(name));
        return false;
    }

    value = parsed;
    return true;
}

bool RobotMovementService::readSpeed(RobotSpeed& speed) {
    speed.tcpLinear = robotConfig_.defaultSpeed.tcpLinear;
    speed.tcpOrientation = robotConfig_.defaultSpeed.tcpOrientation;
    speed.externalLinear = robotConfig_.defaultSpeed.externalLinear;
    speed.externalOrientation = robotConfig_.defaultSpeed.externalOrientation;

    // Empty speed fields intentionally preserve the configured defaults, as in
    // the Python UI. Non-empty invalid values abort so the robot never moves at
    // a speed different from what the operator attempted to enter.
    return readOptionalSpeedField(ui_->RobotlinespeedlineEdit, "Robot linear speed", speed.tcpLinear)
        && readOptionalSpeedField(ui_->RobotangularspeedlineEdit, "Robot angular speed", speed.tcpOrientation)
        && readOptionalSpeedField(ui_->AxislinespeedlineEdit, "External axis linear speed", speed.externalLinear)
        && readOptionalSpeedField(ui_->AxisangularspeedlineEdit, "External axis angular speed", speed.externalOrientation);
}

void RobotMovementService::runMotionCommand(const QString& action,
                                            const RobotSpeed& speed,
                                            std::function<RobotMotionResult()> motionTask) {
    if (motionRunning_) {
        emit messageRequested("Robot control", "A robot movement command is already running.");
        return;
    }

    motionRunning_ = true;
    QThread* thread = QThread::create([this, action, speed, motionTask = std::move(motionTask)]() mutable {
        try {
            motionClient_.setSpeed(speed);
            const RobotMotionResult result = motionTask();
            QMetaObject::invokeMethod(this, [this, action, result]() {
                reportMotionResult(action, result);
                motionRunning_ = false;
            }, Qt::QueuedConnection);
        } catch (const std::exception& error) {
            const QString message = error.what();
            QMetaObject::invokeMethod(this, [this, action, message]() {
                emit messageRequested("Robot", QString("Failed to %1: %2").arg(action, message));
                motionRunning_ = false;
            }, Qt::QueuedConnection);
        } catch (...) {
            QMetaObject::invokeMethod(this, [this, action]() {
                emit messageRequested("Robot", QString("Failed to %1: unknown robot motion error").arg(action));
                motionRunning_ = false;
            }, Qt::QueuedConnection);
        }
    });

    motionThread_ = thread;
    connect(thread, &QThread::finished, this, [this, thread]() {
        if (motionThread_ == thread) {
            motionThread_ = nullptr;
        }
        thread->deleteLater();
    });
    thread->start();
}

void RobotMovementService::reportMotionResult(const QString& action, const RobotMotionResult& result) {
    if (result.ok) {
        const QString message = result.message.trimmed();
        if (!message.isEmpty()) {
            emit messageRequested("Robot", message);
        }
        return;
    }

    if (!result.ok) {
        const QString message = result.message.trimmed().isEmpty()
            ? "motion controller reported failure without details"
            : result.message;
        emit messageRequested("Robot", QString("Failed to %1: %2").arg(action, message));
    }
}

void RobotMovementService::reportMotionException(const QString& action, const std::exception& error) {
    emit messageRequested("Robot", QString("Failed to %1: %2").arg(action, error.what()));
}

void RobotMovementService::reportUnknownMotionException(const QString& action) {
    emit messageRequested("Robot", QString("Failed to %1: unknown robot motion error").arg(action));
}
