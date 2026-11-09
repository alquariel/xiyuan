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

    // TCP connect alone only proves *something* is listening on the port.
    // Actually talking to an orphaned RobVC.exe leaves us in a "connected but
    // useless" state, so verify the peer is SERVER.mod by running CASE 0 Ping.
    const RobotSocketResult ping = sendPing();
    if (!ping.ok) {
        socket_.disconnectFromHost();
        return RobotSocketResult::failure(
            QString("Connected to %1 but RAPID CASE 0 ping failed — SERVER.mod may not be running. %2")
                .arg(endpointText(), ping.message));
    }

    return RobotSocketResult::success(QString("Robot socket connected to %1 (RAPID ping OK).").arg(endpointText()));
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

RobotSocketResult RobotSocketClient::sendPing() {
    if (!isConnected()) {
        return RobotSocketResult::failure("Robot socket is not connected.");
    }

    // CASE 0 Ping in SERVER.mod: nParams=0 → replies "0 1" (SERVER_OK).
    const QByteArray payload = "0 #";
    if (socket_.write(payload) != payload.size()
        || !socket_.waitForBytesWritten(timeoutMs_)) {
        return RobotSocketResult::failure(
            QString("Failed to write RAPID CASE 0 ping: %1").arg(socket_.errorString()));
    }

    if (!socket_.waitForReadyRead(timeoutMs_)) {
        return RobotSocketResult::failure(
            QString("Timed out waiting for RAPID CASE 0 ping reply: %1").arg(socket_.errorString()));
    }

    const QByteArray reply = socket_.readAll().trimmed();
    const QStringList tokens = QString::fromUtf8(reply).split(QLatin1Char(' '), Qt::SkipEmptyParts);
    bool codeOk = false;
    bool statusOk = false;
    const int code = tokens.size() >= 1 ? tokens.first().toInt(&codeOk) : -1;
    const int status = tokens.size() >= 2 ? tokens.at(1).toInt(&statusOk) : 0;
    if (!codeOk || code != 0 || !statusOk || status != 1) {
        return RobotSocketResult::failure(
            QString("Unexpected RAPID CASE 0 ping reply '%1'.").arg(QString::fromUtf8(reply)));
    }

    return RobotSocketResult::success("RAPID CASE 0 ping OK.");
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

RobotSocketResult RobotSocketClient::sendRawMessageWithReply(const QString& frame, QString& reply) {
    return sendRawMessageWithReply(frame, reply, timeoutMs_);
}

RobotSocketResult RobotSocketClient::sendRawMessageWithReply(const QString& frame, QString& reply,
                                                             int timeoutMs) {
    reply.clear();
    if (!isConnected()) {
        return RobotSocketResult::failure("Robot socket is not connected.");
    }

    const QByteArray payload = frame.toUtf8();
    if (socket_.write(payload) != payload.size() || !socket_.waitForBytesWritten(timeoutMs)) {
        return RobotSocketResult::failure(
            QString("Failed to write robot socket message '%1': %2").arg(frame, socket_.errorString()));
    }

    // RAPID answers every command with one line of text (no terminator).
    if (!socket_.waitForReadyRead(timeoutMs)) {
        return RobotSocketResult::failure(
            QString("Timed out waiting for the reply to '%1': %2").arg(frame, socket_.errorString()));
    }

    reply = QString::fromUtf8(socket_.readAll()).trimmed();
    if (reply.isEmpty()) {
        return RobotSocketResult::failure(
            QString("Empty reply from the robot for '%1'.").arg(frame));
    }

    return RobotSocketResult::success(reply);
}

QString RobotSocketClient::endpointText() const {
    return QString("%1:%2").arg(config_.ip).arg(config_.portMotion);
}
