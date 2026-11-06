#include "robot/PlcRobotMotionClient.h"

#include <cmath>
#include <limits>

#include <QElapsedTimer>
#include <QThread>

namespace {

constexpr quint8 statusBusy = 0x01;
constexpr quint8 statusError = 0x02;

double roundTiesToEven(double value) {
    const double sign = value < 0.0 ? -1.0 : 1.0;
    const double absoluteValue = std::fabs(value);
    const double lower = std::floor(absoluteValue);
    const double fraction = absoluteValue - lower;

    if (fraction < 0.5) {
        return sign * lower;
    }
    if (fraction > 0.5) {
        return sign * (lower + 1.0);
    }

    // Python round() uses banker rounding for exact .5 ties. Match
    // int(round(value * scale)) from abb_plc.py before writing PLC DINTs.
    if (std::fmod(lower, 2.0) == 0.0) {
        return sign * lower;
    }
    return sign * (lower + 1.0);
}

}

PlcRobotMotionClient::PlcRobotMotionClient(IPlcProcessAccess& plc)
    : PlcRobotMotionClient(plc, 10000, 60000, 50) {
}

PlcRobotMotionClient::PlcRobotMotionClient(IPlcProcessAccess& plc,
                                           int ackTimeoutMs,
                                           int doneTimeoutMs,
                                           int pollIntervalMs)
    : plc_(plc),
      ackTimeoutMs_(ackTimeoutMs < 1 ? 10000 : ackTimeoutMs),
      doneTimeoutMs_(doneTimeoutMs < 1 ? 60000 : doneTimeoutMs),
      pollIntervalMs_(pollIntervalMs < 1 ? 50 : pollIntervalMs) {
}

void PlcRobotMotionClient::setSpeed(const RobotSpeed& speed) {
    speed_ = speed;
}

RobotMotionResult PlcRobotMotionClient::moveCartesian(const RobotPose& pose) {
    qint32 sequence = 0;
    RobotMotionResult seqResult = nextSequence(sequence);
    if (!seqResult.ok) {
        return seqResult;
    }

    // PLC PA byte map for composite Cartesian command 120:
    // position X/Y/Z at MD99/103/107, quaternion Q1..Q4 at MD111..123,
    // and pending external axes in Python order GR/INC/SPIN at MD135/127/131.
    const std::array<std::pair<int, double>, 10> writes{{
        {mdSendPosX_, pose.position[0]},
        {mdSendPosY_, pose.position[1]},
        {mdSendPosZ_, pose.position[2]},
        {mdSendPosQ1_, pose.quaternion[0]},
        {mdSendPosQ2_, pose.quaternion[1]},
        {mdSendPosQ3_, pose.quaternion[2]},
        {mdSendPosQ4_, pose.quaternion[3]},
        {mdSendExtGr_, pendingExternalAxis_[0]},
        {mdSendExtInc_, pendingExternalAxis_[1]},
        {mdSendExtSpin_, pendingExternalAxis_[2]},
    }};

    for (const auto& write : writes) {
        RobotMotionResult result = writeScaled(write.first, write.second);
        if (!result.ok) {
            return result;
        }
    }

    RobotMotionResult speedResult = writeSpeed();
    if (!speedResult.ok) {
        return speedResult;
    }

    RobotMotionResult commandResult = writeCommandAndSequence(120, sequence);
    if (!commandResult.ok) {
        return commandResult;
    }

    RobotMotionResult ackResult = waitAck(sequence);
    if (!ackResult.ok) {
        return ackResult;
    }
    return waitDone(sequence);
}

RobotMotionResult PlcRobotMotionClient::moveJoints(const std::array<double, 6>& joints) {
    qint32 sequence = 0;
    RobotMotionResult seqResult = nextSequence(sequence);
    if (!seqResult.ok) {
        return seqResult;
    }

    // PLC PA byte map for AbsJ command 2: six joint values use MD99..119.
    // MD123 is cleared because this slot is Q4 for Cartesian commands.
    const std::array<int, 6> jointOffsets{mdSendPosX_, mdSendPosY_, mdSendPosZ_,
                                          mdSendPosQ1_, mdSendPosQ2_, mdSendPosQ3_};
    for (int i = 0; i < static_cast<int>(joints.size()); ++i) {
        RobotMotionResult result = writeScaled(jointOffsets[i], joints[i]);
        if (!result.ok) {
            return result;
        }
    }

    RobotMotionResult clearResult = writeScaled(mdSendPosQ4_, 0.0);
    if (!clearResult.ok) {
        return clearResult;
    }

    RobotMotionResult speedResult = writeSpeed();
    if (!speedResult.ok) {
        return speedResult;
    }

    RobotMotionResult commandResult = writeCommandAndSequence(2, sequence);
    if (!commandResult.ok) {
        return commandResult;
    }

    RobotMotionResult ackResult = waitAck(sequence);
    if (!ackResult.ok) {
        return ackResult;
    }
    return waitDone(sequence);
}

RobotMotionResult PlcRobotMotionClient::moveExternalAxis(const std::array<double, 6>& axisValues) {
    qint32 sequence = 0;
    RobotMotionResult seqResult = nextSequence(sequence);
    if (!seqResult.ok) {
        return seqResult;
    }

    pendingExternalAxis_ = axisValues;

    // PLC PA byte map for external-axis command 34 follows abb_plc.py order:
    // Python axis[0] -> GR MD135, axis[1] -> INC MD127, axis[2] -> SPIN MD131.
    const std::array<std::pair<int, double>, 3> writes{{
        {mdSendExtGr_, pendingExternalAxis_[0]},
        {mdSendExtInc_, pendingExternalAxis_[1]},
        {mdSendExtSpin_, pendingExternalAxis_[2]},
    }};

    for (const auto& write : writes) {
        RobotMotionResult result = writeScaled(write.first, write.second);
        if (!result.ok) {
            return result;
        }
    }

    RobotMotionResult speedResult = writeSpeed();
    if (!speedResult.ok) {
        return speedResult;
    }

    RobotMotionResult commandResult = writeCommandAndSequence(34, sequence);
    if (!commandResult.ok) {
        return commandResult;
    }

    RobotMotionResult ackResult = waitAck(sequence);
    if (!ackResult.ok) {
        return ackResult;
    }
    return waitDone(sequence);
}

RobotMotionResult PlcRobotMotionClient::writeSpeed() {
    const std::array<std::pair<int, double>, 4> writes{{
        {mdSendSpeedVTcp_, speed_.tcpLinear},
        {mdSendSpeedVOri_, speed_.tcpOrientation},
        {mdSendSpeedVLeax_, speed_.externalLinear},
        {mdSendSpeedVReax_, speed_.externalOrientation},
    }};

    for (const auto& write : writes) {
        RobotMotionResult result = writeScaled(write.first, write.second);
        if (!result.ok) {
            return result;
        }
    }
    return RobotMotionResult::success();
}

RobotMotionResult PlcRobotMotionClient::waitAck(qint32 sequence) {
    QElapsedTimer timer;
    timer.start();

    // ACK wait: PLC copies the accepted command sequence to PE MD145. While
    // polling, PE MB153 bit 0x02 reports robot/PLC error and MB154 carries the
    // PLC-side error code.
    while (timer.elapsed() < ackTimeoutMs_) {
        quint8 status = 0;
        PlcResult statusResult = plc_.readProcessInputByte(mbRecvStatus_, status);
        if (!statusResult.ok) {
            return RobotMotionResult::failure(QString("PLC robot ACK status read failed: %1").arg(statusResult.message));
        }

        quint8 errorCode = 0;
        PlcResult errorResult = plc_.readProcessInputByte(mbRecvErrCode_, errorCode);
        if (!errorResult.ok) {
            return RobotMotionResult::failure(QString("PLC robot ACK error-code read failed: %1").arg(errorResult.message));
        }
        if ((status & statusError) != 0) {
            return RobotMotionResult::failure(QString("PLC robot ACK error status 0x%1, error code %2.")
                                                  .arg(status, 2, 16, QLatin1Char('0'))
                                                  .arg(errorCode));
        }

        qint32 ackSequence = 0;
        PlcResult ackResult = plc_.readProcessInputInt32(mdRecvAckSeq_, ackSequence);
        if (!ackResult.ok) {
            return RobotMotionResult::failure(QString("PLC robot ACK sequence read failed: %1").arg(ackResult.message));
        }
        if (ackSequence == sequence) {
            return RobotMotionResult::success();
        }

        QThread::msleep(static_cast<unsigned long>(pollIntervalMs_));
    }

    return RobotMotionResult::failure(QString("PLC robot ACK timeout after %1 ms for sequence %2.")
                                          .arg(ackTimeoutMs_)
                                          .arg(sequence));
}

RobotMotionResult PlcRobotMotionClient::waitDone(qint32 sequence) {
    QElapsedTimer timer;
    timer.start();

    // DONE wait: after ACK, PLC writes the completed sequence to PE MD149.
    // Status bit 0x01 means the PLC still considers motion busy; bit 0x02
    // aborts the wait immediately with MB154 as the PLC error code.
    while (timer.elapsed() < doneTimeoutMs_) {
        quint8 status = 0;
        PlcResult statusResult = plc_.readProcessInputByte(mbRecvStatus_, status);
        if (!statusResult.ok) {
            return RobotMotionResult::failure(QString("PLC robot DONE status read failed: %1").arg(statusResult.message));
        }

        quint8 errorCode = 0;
        PlcResult errorResult = plc_.readProcessInputByte(mbRecvErrCode_, errorCode);
        if (!errorResult.ok) {
            return RobotMotionResult::failure(QString("PLC robot DONE error-code read failed: %1").arg(errorResult.message));
        }
        if ((status & statusError) != 0) {
            return RobotMotionResult::failure(QString("PLC robot DONE error status 0x%1, error code %2.")
                                                  .arg(status, 2, 16, QLatin1Char('0'))
                                                  .arg(errorCode));
        }

        qint32 doneSequence = 0;
        PlcResult doneResult = plc_.readProcessInputInt32(mdRecvDoneSeq_, doneSequence);
        if (!doneResult.ok) {
            return RobotMotionResult::failure(QString("PLC robot DONE sequence read failed: %1").arg(doneResult.message));
        }
        if (doneSequence == sequence && (status & statusBusy) == 0) {
            return RobotMotionResult::success();
        }

        QThread::msleep(static_cast<unsigned long>(pollIntervalMs_));
    }

    return RobotMotionResult::failure(QString("PLC robot DONE timeout after %1 ms for sequence %2.")
                                          .arg(doneTimeoutMs_)
                                          .arg(sequence));
}

RobotMotionResult PlcRobotMotionClient::nextSequence(qint32& sequence) {
    qint32 current = 0;
    const PlcResult readResult = plc_.readProcessOutputInt32(mdSendSeq_, current);
    if (!readResult.ok) {
        return RobotMotionResult::failure(QString("PLC robot sequence read failed at byte %1: %2")
                                              .arg(mdSendSeq_)
                                              .arg(readResult.message));
    }

    if (current == std::numeric_limits<qint32>::max()) {
        return RobotMotionResult::failure(QString("PLC robot sequence is already at maximum qint32 value %1; refusing to overflow.")
                                              .arg(current));
    }

    sequence = current + 1;
    return RobotMotionResult::success();
}

RobotMotionResult PlcRobotMotionClient::writeScaled(int byteOffset, double value) {
    if (!std::isfinite(value)) {
        return RobotMotionResult::failure(QString("PLC robot write rejected at byte %1: value must be finite.").arg(byteOffset));
    }

    const double scaledValue = value * scale_;
    if (!std::isfinite(scaledValue)) {
        return RobotMotionResult::failure(QString("PLC robot write rejected at byte %1: scaled value must be finite.")
                                              .arg(byteOffset));
    }

    const double rounded = roundTiesToEven(scaledValue);
    const double minValue = static_cast<double>(std::numeric_limits<qint32>::min());
    const double maxValue = static_cast<double>(std::numeric_limits<qint32>::max());
    if (rounded < minValue || rounded > maxValue) {
        return RobotMotionResult::failure(QString("PLC robot write rejected at byte %1: scaled value %2 is outside qint32 range.")
                                              .arg(byteOffset)
                                              .arg(rounded));
    }

    const qint32 scaled = static_cast<qint32>(rounded);
    const PlcResult result = plc_.writeProcessOutputInt32(byteOffset, scaled);
    if (!result.ok) {
        return RobotMotionResult::failure(QString("PLC robot write failed at byte %1: %2").arg(byteOffset).arg(result.message));
    }
    return RobotMotionResult::success();
}

RobotMotionResult PlcRobotMotionClient::writeCommandAndSequence(qint32 command, qint32 sequence) {
    const PlcResult commandResult = plc_.writeProcessOutputInt32(mdSendCmdCode_, command);
    if (!commandResult.ok) {
        return RobotMotionResult::failure(QString("PLC robot command write failed at byte %1: %2")
                                              .arg(mdSendCmdCode_)
                                              .arg(commandResult.message));
    }

    const PlcResult sequenceResult = plc_.writeProcessOutputInt32(mdSendSeq_, sequence);
    if (!sequenceResult.ok) {
        return RobotMotionResult::failure(QString("PLC robot sequence write failed at byte %1: %2")
                                              .arg(mdSendSeq_)
                                              .arg(sequenceResult.message));
    }

    return RobotMotionResult::success();
}
