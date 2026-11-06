#include "robot/SocketRobotMotionClient.h"

#include <QAbstractSocket>
#include <QElapsedTimer>

namespace {

constexpr int connectTimeoutMs = 3000;

// CASE 8 (set speed) replies immediately; motion cases reply only after the
// movement finishes, so they get the same generous budget as the PLC DONE
// wait in PlcRobotMotionClient.
constexpr int speedResponseTimeoutMs = 10000;
constexpr int motionResponseTimeoutMs = 60000;

// RAPID instruction codes from Motion.mod: CASE 8 updates currentSpeed,
// CASE 1 runs MoveL to the Cartesian target, CASE 2 runs MoveAbsJ to the
// joint target, and CASE 34 runs MoveAbsJ to the external-axis target.
constexpr int codeSetSpeed = 8;
constexpr int codeMoveCartesian = 1;
constexpr int codeMoveJoints = 2;
constexpr int codeMoveExternalAxis = 34;

// SERVER_OK:=1 in Motion.mod; the reply's second token must carry it.
constexpr int rapidServerOk = 1;

QString formatNumber(double value) {
    return QString::number(value, 'f', 6);
}

QString formatList(const std::array<double, 6>& values) {
    QString text;
    for (double value : values) {
        if (!text.isEmpty()) {
            text += QLatin1Char(' ');
        }
        text += formatNumber(value);
    }
    return text;
}

}  // namespace

SocketRobotMotionClient::SocketRobotMotionClient(RobotConfig config)
    : config_(std::move(config)) {
}

SocketRobotMotionClient::~SocketRobotMotionClient() {
    // The motion worker thread has already been joined by
    // RobotMovementService before its members are destroyed, so deleting the
    // lazily created socket here cannot race a running command.
    if (socket_ != nullptr) {
        if (socket_->state() != QAbstractSocket::UnconnectedState) {
            socket_->disconnectFromHost();
        }
        delete socket_;
        socket_ = nullptr;
    }
}

void SocketRobotMotionClient::setSpeed(const RobotSpeed& speed) {
    // Stored like PlcRobotMotionClient and transmitted before each motion so
    // the controller uses the operator speed for the upcoming command.
    speed_ = speed;
}

RobotMotionResult SocketRobotMotionClient::moveCartesian(const RobotPose& pose) {
    RobotMotionResult connected = ensureConnected();
    if (!connected.ok) {
        return connected;
    }

    const QString speedParams = QString("%1 %2 %3 %4")
                                    .arg(formatNumber(speed_.tcpLinear),
                                         formatNumber(speed_.tcpOrientation),
                                         formatNumber(speed_.externalLinear),
                                         formatNumber(speed_.externalOrientation));
    RobotMotionResult speedResult =
        sendCommand(codeSetSpeed, speedParams, speedResponseTimeoutMs);
    if (!speedResult.ok) {
        return speedResult;
    }

    const QString params = QString("%1 %2 %3 %4 %5 %6 %7")
                               .arg(formatNumber(pose.position[0]),
                                    formatNumber(pose.position[1]),
                                    formatNumber(pose.position[2]),
                                    formatNumber(pose.quaternion[0]),
                                    formatNumber(pose.quaternion[1]),
                                    formatNumber(pose.quaternion[2]),
                                    formatNumber(pose.quaternion[3]));
    return sendCommand(codeMoveCartesian, params, motionResponseTimeoutMs);
}

RobotMotionResult SocketRobotMotionClient::moveJoints(const std::array<double, 6>& joints) {
    RobotMotionResult connected = ensureConnected();
    if (!connected.ok) {
        return connected;
    }

    RobotMotionResult speedResult = sendCommand(codeSetSpeed, formatList({speed_.tcpLinear,
                                                                              speed_.tcpOrientation,
                                                                              speed_.externalLinear,
                                                                              speed_.externalOrientation}),
                                                speedResponseTimeoutMs);
    if (!speedResult.ok) {
        return speedResult;
    }

    return sendCommand(codeMoveJoints, formatList(joints), motionResponseTimeoutMs);
}

RobotMotionResult SocketRobotMotionClient::moveExternalAxis(const std::array<double, 6>& axisValues) {
    RobotMotionResult connected = ensureConnected();
    if (!connected.ok) {
        return connected;
    }

    RobotMotionResult speedResult = sendCommand(codeSetSpeed, formatList({speed_.tcpLinear,
                                                                              speed_.tcpOrientation,
                                                                              speed_.externalLinear,
                                                                              speed_.externalOrientation}),
                                                speedResponseTimeoutMs);
    if (!speedResult.ok) {
        return speedResult;
    }

    return sendCommand(codeMoveExternalAxis, formatList(axisValues), motionResponseTimeoutMs);
}

RobotMotionResult SocketRobotMotionClient::ensureConnected() {
    if (config_.ip.trimmed().isEmpty()) {
        return RobotMotionResult::failure("Robot motion socket IP is empty.");
    }
    if (config_.portMotion <= 0 || config_.portMotion > 65535) {
        return RobotMotionResult::failure("Robot motion socket port is invalid.");
    }

    if (socket_ != nullptr && socket_->state() == QAbstractSocket::ConnectedState) {
        return RobotMotionResult::success();
    }

    // Lazily create the socket on the calling (motion worker) thread so every
    // later blocking wait stays on the socket's own thread.
    if (socket_ == nullptr) {
        socket_ = new QTcpSocket();
    }

    socket_->abort();
    socket_->connectToHost(config_.ip, static_cast<quint16>(config_.portMotion));
    if (!socket_->waitForConnected(connectTimeoutMs)) {
        return RobotMotionResult::failure(
            QString("Failed to connect robot motion socket %1:%2: %3")
                .arg(config_.ip)
                .arg(config_.portMotion)
                .arg(socket_->errorString()));
    }

    return RobotMotionResult::success(
        QString("Robot motion socket connected to %1:%2.")
            .arg(config_.ip)
            .arg(config_.portMotion));
}

RobotMotionResult SocketRobotMotionClient::sendCommand(int code,
                                                       const QString& params,
                                                       int responseTimeoutMs) {
    if (socket_ == nullptr || socket_->state() != QAbstractSocket::ConnectedState) {
        return RobotMotionResult::failure("Robot motion socket is not connected.");
    }

    // Drop any stale bytes from a previous exchange whose response timed out;
    // the RAPID server sends exactly one reply per request, so clearing here
    // keeps request/response pairing aligned.
    socket_->readAll();

    const QByteArray payload = QString("%1 %2#").arg(code).arg(params).toUtf8();
    if (socket_->write(payload) != payload.size() || !socket_->waitForBytesWritten(connectTimeoutMs)) {
        return RobotMotionResult::failure(
            QString("Failed to send robot motion socket command '%1': %2")
                .arg(QString::fromUtf8(payload), socket_->errorString()));
    }

    return readResponse(code, responseTimeoutMs);
}

RobotMotionResult SocketRobotMotionClient::readResponse(int code, int responseTimeoutMs) {
    // The RAPID reply has no terminator: "<code> <ok> [data]". Motion cases
    // finish their movement before the reply is sent, so waiting for the first
    // ready-read yields the complete response in practice.
    QElapsedTimer timer;
    timer.start();
    QByteArray response;
    while (timer.elapsed() < responseTimeoutMs) {
        if (!socket_->waitForReadyRead(responseTimeoutMs - static_cast<int>(timer.elapsed()))) {
            if (socket_->state() != QAbstractSocket::ConnectedState) {
                return RobotMotionResult::failure(
                    QString("Robot motion socket disconnected while waiting for reply to command %1: %2")
                        .arg(code)
                        .arg(socket_->errorString()));
            }
            continue;
        }

        response += socket_->readAll();
        if (!response.trimmed().isEmpty()) {
            break;
        }
    }

    const QString responseText = QString::fromUtf8(response).trimmed();
    if (responseText.isEmpty()) {
        return RobotMotionResult::failure(
            QString("Robot motion socket reply timeout after %1 ms for command %2.")
                .arg(responseTimeoutMs)
                .arg(code));
    }

    const QStringList tokens = responseText.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (tokens.size() < 2) {
        return RobotMotionResult::failure(
            QString("Robot motion socket sent an unexpected reply '%1' for command %2.")
                .arg(responseText)
                .arg(code));
    }

    bool codeOk = false;
    const int replyCode = tokens.first().toInt(&codeOk);
    bool statusOk = false;
    const int replyStatus = tokens.at(1).toInt(&statusOk);
    if (!codeOk || replyCode != code) {
        return RobotMotionResult::failure(
            QString("Robot motion socket reply '%1' does not match command %2.")
                .arg(responseText)
                .arg(code));
    }
    if (!statusOk || replyStatus != rapidServerOk) {
        return RobotMotionResult::failure(
            QString("RAPID rejected command %1: reply '%2' is not SERVER_OK.")
                .arg(code)
                .arg(responseText));
    }

    return RobotMotionResult::success();
}
