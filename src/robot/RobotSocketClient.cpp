#include "robot/RobotSocketClient.h"

#include <QAbstractSocket>

#include <utility>

RobotSocketClient::RobotSocketClient(RobotConfig config)
    : config_(std::move(config)) {
}

RobotSocketResult RobotSocketClient::connectToRobot() {
    if (config_.ip.trimmed().isEmpty()) {
        return RobotSocketResult::failure("Robot socket IP is empty.");
    }
    if (config_.portMotion <= 0 || config_.portMotion > 65535) {
        return RobotSocketResult::failure("Robot motion socket port is invalid.");
    }

    if (isConnected()) {
        return RobotSocketResult::success(QString("Robot socket already connected to %1.").arg(endpointText()));
    }

    socket_.abort();
    socket_.connectToHost(config_.ip, static_cast<quint16>(config_.portMotion));
    if (!socket_.waitForConnected(timeoutMs_)) {
        return RobotSocketResult::failure(
            QString("Failed to connect robot socket %1: %2").arg(endpointText(), socket_.errorString()));
    }

    return RobotSocketResult::success(QString("Robot socket connected to %1.").arg(endpointText()));
}

void RobotSocketClient::disconnectFromRobot() {
    socket_.disconnectFromHost();
    if (socket_.state() != QAbstractSocket::UnconnectedState) {
        socket_.waitForDisconnected(timeoutMs_);
    }
}

bool RobotSocketClient::isConnected() const {
    return socket_.state() == QAbstractSocket::ConnectedState;
}

RobotSocketResult RobotSocketClient::sendLoadMod(const QString& modFilename) {
    return sendRawMessage(loadModMessage(modFilename));
}

RobotSocketResult RobotSocketClient::sendPause() {
    return sendRawMessage("PAUSE#");
}

RobotSocketResult RobotSocketClient::sendContinue() {
    return sendRawMessage("CONTINUE#");
}

RobotSocketResult RobotSocketClient::sendStop() {
    return sendRawMessage("STOP#");
}

QString RobotSocketClient::loadModMessage(const QString& modFilename) {
    return QString("LOAD_MOD:%1#").arg(modFilename);
}

RobotSocketResult RobotSocketClient::sendRawMessage(const QString& message) {
    if (!isConnected()) {
        return RobotSocketResult::failure("Robot socket is not connected.");
    }

    const QByteArray payload = message.toUtf8();
    const qint64 written = socket_.write(payload);
    if (written != payload.size()) {
        return RobotSocketResult::failure(
            QString("Failed to write robot socket message '%1': %2").arg(message, socket_.errorString()));
    }

    if (!socket_.waitForBytesWritten(timeoutMs_)) {
        return RobotSocketResult::failure(
            QString("Timed out sending robot socket message '%1': %2").arg(message, socket_.errorString()));
    }

    return RobotSocketResult::success(QString("Sent robot socket message: %1").arg(message));
}

QString RobotSocketClient::endpointText() const {
    return QString("%1:%2").arg(config_.ip).arg(config_.portMotion);
}
