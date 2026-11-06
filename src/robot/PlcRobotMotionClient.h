#pragma once

#include <array>

#include <QString>

#include "plc/PlcClient.h"
#include "robot/RobotQuaternion.h"

struct RobotMotionResult {
    bool ok = false;
    QString message;

    static RobotMotionResult success(const QString& message = QString()) { return {true, message}; }
    static RobotMotionResult failure(const QString& message) { return {false, message}; }
};

struct RobotSpeed {
    // Fallbacks mirror config/env_config.json and Python defaults. UI/config
    // layers should pass user-configured values through setSpeed().
    double tcpLinear = 200.0;
    double tcpOrientation = 200.0;
    double externalLinear = 200.0;
    double externalOrientation = 100.0;
};

class IRobotMotionController {
public:
    virtual ~IRobotMotionController() = default;

    virtual void setSpeed(const RobotSpeed& speed) = 0;
    virtual RobotMotionResult moveCartesian(const RobotPose& pose) = 0;
    virtual RobotMotionResult moveJoints(const std::array<double, 6>& joints) = 0;
    virtual RobotMotionResult moveExternalAxis(const std::array<double, 6>& axisValues) = 0;
};

class PlcRobotMotionClient final : public IRobotMotionController {
public:
    explicit PlcRobotMotionClient(IPlcProcessAccess& plc);
    PlcRobotMotionClient(IPlcProcessAccess& plc, int ackTimeoutMs, int doneTimeoutMs, int pollIntervalMs);

    void setSpeed(const RobotSpeed& speed) override;
    RobotMotionResult moveCartesian(const RobotPose& pose) override;
    RobotMotionResult moveJoints(const std::array<double, 6>& joints) override;
    RobotMotionResult moveExternalAxis(const std::array<double, 6>& axisValues) override;

    int sequenceOffset() const { return mdSendSeq_; }
    int commandOffset() const { return mdSendCmdCode_; }

private:
    RobotMotionResult writeSpeed();
    RobotMotionResult waitAck(qint32 sequence);
    RobotMotionResult waitDone(qint32 sequence);
    RobotMotionResult nextSequence(qint32& sequence);
    RobotMotionResult writeScaled(int byteOffset, double value);
    RobotMotionResult writeCommandAndSequence(qint32 command, qint32 sequence);

    IPlcProcessAccess& plc_;
    RobotSpeed speed_;
    std::array<double, 6> pendingExternalAxis_{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    int scale_ = 1000;
    int ackTimeoutMs_ = 10000;
    int doneTimeoutMs_ = 60000;
    int pollIntervalMs_ = 50;

    int mdSendPosX_ = 99;
    int mdSendPosY_ = 103;
    int mdSendPosZ_ = 107;
    int mdSendPosQ1_ = 111;
    int mdSendPosQ2_ = 115;
    int mdSendPosQ3_ = 119;
    int mdSendPosQ4_ = 123;
    int mdSendExtInc_ = 127;
    int mdSendExtSpin_ = 131;
    int mdSendExtGr_ = 135;
    int mdSendSpeedVTcp_ = 139;
    int mdSendSpeedVOri_ = 143;
    int mdSendSpeedVLeax_ = 147;
    int mdSendSpeedVReax_ = 151;
    int mdSendCmdCode_ = 155;
    int mdSendSeq_ = 159;

    int mdRecvAckSeq_ = 145;
    int mdRecvDoneSeq_ = 149;
    int mbRecvStatus_ = 153;
    int mbRecvErrCode_ = 154;
};
