#pragma once

#include <array>

#include <QTcpSocket>

#include "app/AppConfig.h"
#include "robot/PlcRobotMotionClient.h"

// Direct-socket robot motion client implementing the RAPID server protocol in
// rapid/RAPID/TASK1/PROGMOD/Motion.mod. Requests are "<code> <params...>#";
// every command is answered with "<code> <ok> [data]" where ok=1 means the
// RAPID TEST case ran with SERVER_OK. Motion cases execute synchronously on
// the controller, so each reply arrives after the movement finishes, matching
// the blocking ack/done behavior of PlcRobotMotionClient.
//
// The TCP socket is created lazily on the motion worker thread because
// RobotMovementService runs each command inside QThread::create, and Qt
// requires socket calls to stay on the object's own thread.
class SocketRobotMotionClient final : public IRobotMotionController {
public:
    explicit SocketRobotMotionClient(RobotConfig config);
    ~SocketRobotMotionClient() override;

    SocketRobotMotionClient(const SocketRobotMotionClient&) = delete;
    SocketRobotMotionClient& operator=(const SocketRobotMotionClient&) = delete;

    void setSpeed(const RobotSpeed& speed) override;
    RobotMotionResult moveCartesian(const RobotPose& pose) override;
    RobotMotionResult moveJoints(const std::array<double, 6>& joints) override;
    RobotMotionResult moveExternalAxis(const std::array<double, 6>& axisValues) override;

private:
    RobotMotionResult ensureConnected();
    RobotMotionResult sendCommand(int code, const QString& params, int responseTimeoutMs);
    RobotMotionResult readResponse(int code, int responseTimeoutMs);

    RobotConfig config_;
    QTcpSocket* socket_ = nullptr;
    RobotSpeed speed_;
};
