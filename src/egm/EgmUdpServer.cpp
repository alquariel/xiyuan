#include "egm/EgmUdpServer.h"

#include <QHostAddress>
#include <QMutexLocker>
#include <QUdpSocket>

#include <chrono>
#include <cmath>
#include <string>

namespace egm {
namespace {

constexpr int kPollIntervalMs = 25;
constexpr int kMaxDatagramBytes = 65535;

double clampValue(double value, double limit) {
    if (!std::isfinite(value)) {
        return 0.0;
    }
    if (value > limit) {
        return limit;
    }
    if (value < -limit) {
        return -limit;
    }
    return value;
}

}  // namespace

EgmUdpServer::EgmUdpServer(EgmUdpServerConfig config) : config_(config) {
}

EgmUdpServer::~EgmUdpServer() {
    requestStop();
}

bool EgmUdpServer::run(const CorrectionHandler& handler, QString* errorMessage) {
    stopRequested_ = false;

    QUdpSocket socket;
    if (!socket.bind(QHostAddress::AnyIPv4, config_.listenPort)) {
        if (errorMessage != nullptr) {
            *errorMessage = QString("Could not bind EGM UDP port %1: %2")
                                .arg(config_.listenPort)
                                .arg(socket.errorString());
        }
        EgmUdpServerStatus failed;
        failed.listening = false;
        setStatus(failed);
        return false;
    }

    EgmUdpServerStatus initial;
    initial.listening = true;
    setStatus(initial);
    running_ = true;

    QByteArray buffer;
    buffer.resize(kMaxDatagramBytes);

    const auto startedAt = std::chrono::steady_clock::now();
    auto lastReceiveAt = startedAt;
    bool robotSeen = false;
    bool linkTimeout = false;
    quint32 nextSeqno = 0;
    quint64 framesReceived = 0;
    quint64 framesSent = 0;
    EgmRobotFrame lastRobot;
    double lastSentZ = 0.0;

    const double fallbackDt = static_cast<double>(config_.controlPeriodMs) / 1000.0;

    while (!stopRequested_) {
        if (!socket.waitForReadyRead(kPollIntervalMs)) {
            // 没有新报文：检查链路超时。超时后不再发送（本实现本来就是收包驱动）。
            const double ageMs = std::chrono::duration<double, std::milli>(
                                     std::chrono::steady_clock::now() - lastReceiveAt)
                                     .count();
            const bool timeoutNow = robotSeen && ageMs > config_.receiveTimeoutMs;
            if (robotSeen) {
                linkTimeout = timeoutNow;
                EgmUdpServerStatus status;
                status.listening = true;
                status.robotSeen = true;
                status.linkTimeout = linkTimeout;
                status.framesReceived = framesReceived;
                status.framesSent = framesSent;
                status.lastRobotSeqno = lastRobot.seqno;
                status.lastSentZMm = lastSentZ;
                status.lastReceiveAgeMs = ageMs;
                status.lastRobot = lastRobot;
                setStatus(status);
            }
            continue;
        }

        while (socket.hasPendingDatagrams()) {
            QHostAddress peerAddress;
            quint16 peerPort = 0;
            const qint64 bytesRead = socket.readDatagram(
                buffer.data(), buffer.size(), &peerAddress, &peerPort);
            if (bytesRead <= 0) {
                break;
            }

            EgmRobotFrame frame;
            if (!decodeEgmRobot(buffer.constData(), static_cast<std::size_t>(bytesRead), &frame)) {
                continue;
            }

            const auto now = std::chrono::steady_clock::now();
            double dtSeconds = robotSeen
                ? std::chrono::duration<double>(now - lastReceiveAt).count()
                : fallbackDt;
            if (!std::isfinite(dtSeconds) || dtSeconds < 0.001) {
                dtSeconds = 0.001;
            } else if (dtSeconds > 0.5) {
                dtSeconds = 0.5;
            }
            lastReceiveAt = now;
            robotSeen = true;
            linkTimeout = false;
            ++framesReceived;

            lastRobot = frame;

            double zCorr = handler != nullptr ? handler(lastRobot, dtSeconds) : 0.0;
            zCorr = clampValue(zCorr, config_.hardZLimitMm);

            const quint32 tmMs = static_cast<quint32>(
                std::chrono::duration_cast<std::chrono::milliseconds>(now - startedAt).count());
            // seqno 每成功发送一帧只自增一次。
            const std::string encoded = encodeSensorPathCorr(
                nextSeqno, tmMs, zCorr, static_cast<quint32>(config_.correctionAgeMs));
            ++nextSeqno;

            socket.writeDatagram(
                encoded.data(), static_cast<qint64>(encoded.size()), peerAddress, peerPort);
            ++framesSent;
            lastSentZ = zCorr;

            EgmUdpServerStatus status;
            status.listening = true;
            status.robotSeen = true;
            status.linkTimeout = false;
            status.framesReceived = framesReceived;
            status.framesSent = framesSent;
            status.lastRobotSeqno = lastRobot.seqno;
            status.lastSentZMm = lastSentZ;
            status.lastReceiveAgeMs = 0.0;
            status.lastRobot = lastRobot;
            setStatus(status);
        }
    }

    running_ = false;
    EgmUdpServerStatus stopped;
    stopped.listening = false;
    stopped.robotSeen = robotSeen;
    stopped.linkTimeout = linkTimeout;
    stopped.framesReceived = framesReceived;
    stopped.framesSent = framesSent;
    stopped.lastRobotSeqno = lastRobot.seqno;
    stopped.lastSentZMm = lastSentZ;
    stopped.lastRobot = lastRobot;
    setStatus(stopped);
    return true;
}

void EgmUdpServer::requestStop() {
    stopRequested_ = true;
}

EgmUdpServerStatus EgmUdpServer::status() const {
    QMutexLocker locker(&mutex_);
    return status_;
}

bool EgmUdpServer::isRunning() const {
    return running_.load();
}

void EgmUdpServer::setStatus(const EgmUdpServerStatus& status) {
    QMutexLocker locker(&mutex_);
    status_ = status;
}

}  // namespace egm
