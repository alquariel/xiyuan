#include "plc/PlcPrintSignalManager.h"

#include <QElapsedTimer>
#include <QThread>

PlcPrintSignalManager::PlcPrintSignalManager(IPlcByteAccess& plc)
    : PlcPrintSignalManager(plc, 3000, 50) {
}

PlcPrintSignalManager::PlcPrintSignalManager(IPlcByteAccess& plc,
                                             int timeoutMs,
                                             int pollIntervalMs)
    : plc_(plc),
      timeoutMs_(timeoutMs < 1 ? 3000 : timeoutMs),
      pollIntervalMs_(pollIntervalMs < 1 ? 50 : pollIntervalMs) {
}

PlcResult PlcPrintSignalManager::startPrint(bool waitAck) {
    return sendCommand(PlcPrintCommand::Start, waitAck);
}

PlcResult PlcPrintSignalManager::pausePrint(bool waitAck) {
    return sendCommand(PlcPrintCommand::Pause, waitAck);
}

PlcResult PlcPrintSignalManager::resumePrint(bool waitAck) {
    return sendCommand(PlcPrintCommand::Resume, waitAck);
}

PlcResult PlcPrintSignalManager::stopPrint(bool waitAck) {
    return sendCommand(PlcPrintCommand::Stop, waitAck);
}

PlcResult PlcPrintSignalManager::sendCommand(PlcPrintCommand command, bool waitAck) {
    if (command == PlcPrintCommand::None) {
        return PlcResult::failure("PLC print command is invalid.");
    }

    const auto value = static_cast<quint8>(command);
    PlcResult writeResult = plc_.writeDbByte(dbNumber_, commandOffset_, value);
    if (!writeResult.ok) {
        return PlcResult::failure(QString("PLC print command write failed (%1): %2")
                                  .arg(commandName(command), writeResult.message));
    }

    if (!waitAck) {
        return PlcResult::success();
    }

    PlcResult ackResult = waitForAck();
    if (!ackResult.ok) {
        return ackResult;
    }

    return clearCommand();
}

int PlcPrintSignalManager::dbNumber() const {
    return dbNumber_;
}

int PlcPrintSignalManager::commandOffset() const {
    return commandOffset_;
}

int PlcPrintSignalManager::ackOffset() const {
    return ackOffset_;
}

int PlcPrintSignalManager::stateOffset() const {
    return stateOffset_;
}

int PlcPrintSignalManager::errorOffset() const {
    return errorOffset_;
}

PlcResult PlcPrintSignalManager::waitForAck() {
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() < timeoutMs_) {
        quint8 ack = 0;
        PlcResult readResult = plc_.readDbByte(dbNumber_, ackOffset_, ack);
        if (!readResult.ok) {
            return PlcResult::failure(QString("PLC print ACK read failed: %1").arg(readResult.message));
        }

        if (ack == 1) {
            return PlcResult::success();
        }

        QThread::msleep(static_cast<unsigned long>(pollIntervalMs_));
    }

    return PlcResult::failure(QString("PLC print ACK timeout after %1 ms.").arg(timeoutMs_));
}

PlcResult PlcPrintSignalManager::clearCommand() {
    PlcResult clearResult = plc_.writeDbByte(dbNumber_, commandOffset_, 0);
    if (!clearResult.ok) {
        return PlcResult::failure(QString("PLC print command clear failed: %1").arg(clearResult.message));
    }
    return PlcResult::success();
}

QString PlcPrintSignalManager::commandName(PlcPrintCommand command) const {
    switch (command) {
    case PlcPrintCommand::Start:
        return "START";
    case PlcPrintCommand::Pause:
        return "PAUSE";
    case PlcPrintCommand::Resume:
        return "RESUME";
    case PlcPrintCommand::Stop:
        return "STOP";
    case PlcPrintCommand::None:
        return "NONE";
    }
    return "UNKNOWN";
}
