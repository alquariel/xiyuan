#include "robot_monitor/RobotMonitorClient.h"

#include "robot_monitor/RobotMonitorMessageParser.h"

namespace {
constexpr char kHeartbeatMessage[] = "#ROBOT_MONITOR_CLIENT_ALIVE#";
}

RobotMonitorClient::RobotMonitorClient(QObject* parent)
    : QObject(parent) {
    connect(&socket_, &QTcpSocket::connected, this, &RobotMonitorClient::connected);
    connect(&socket_, &QTcpSocket::disconnected, this, &RobotMonitorClient::disconnected);
    connect(&socket_, &QTcpSocket::readyRead, this, &RobotMonitorClient::onReadyRead);
    connect(&socket_, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit errorReported(socket_.errorString());
    });
}

void RobotMonitorClient::connectToRobot(const QString& ip, int port) {
    pendingBytes_.clear();
    socket_.abort();
    socket_.connectToHost(ip, static_cast<quint16>(port));
}

void RobotMonitorClient::disconnectFromRobot() {
    pendingBytes_.clear();
    socket_.disconnectFromHost();
}

bool RobotMonitorClient::isConnected() const {
    return socket_.state() == QAbstractSocket::ConnectedState;
}

bool RobotMonitorClient::sendHeartbeat() {
    if (!isConnected()) {
        return false;
    }

    // This is not a robot command. RAPID uses it only to detect whether the
    // accepted PC monitor client is still alive after AFP_ABB_CPP is restarted.
    const qint64 bytesWritten = socket_.write(kHeartbeatMessage);
    socket_.flush();
    return bytesWritten == static_cast<qint64>(sizeof(kHeartbeatMessage) - 1);
}

void RobotMonitorClient::onReadyRead() {
    pendingBytes_.append(socket_.readAll());

    while (true) {
        const int startIndex = pendingBytes_.indexOf('#');
        if (startIndex < 0) {
            pendingBytes_.clear();
            return;
        }

        if (startIndex > 0) {
            pendingBytes_.remove(0, startIndex);
        }

        const int endIndex = pendingBytes_.indexOf('#', 1);
        if (endIndex < 0) {
            return;
        }

        const QByteArray frame = pendingBytes_.left(endIndex + 1);
        pendingBytes_.remove(0, endIndex + 1);
        processFrame(frame);
    }
}

void RobotMonitorClient::processFrame(const QByteArray& frame) {
    const QString raw = QString::fromUtf8(frame).trimmed();
    emit rawMessageReceived(raw);

    if (!raw.startsWith("#ROBOT_MONITOR")) {
        emit parseErrorReported(raw, "Frame does not start with ROBOT_MONITOR.");
        return;
    }

    const RobotMonitorParseResult parsed = RobotMonitorMessageParser::parse(raw);
    if (parsed.ok) {
        emit sampleReceived(parsed.sample);
    } else {
        emit parseErrorReported(raw, parsed.error);
    }
}
