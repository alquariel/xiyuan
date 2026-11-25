#include "egm/EgmPressureController.h"

#include <QThread>

#include <algorithm>
#include <chrono>

namespace egm {

EgmPressureController::EgmPressureController(EgmPressureConfig config, QObject* parent)
    : QObject(parent),
      config_(std::move(config)),
      pressureSubscriber_(config_.ddsDomainId, config_.pressureTopic),
      monitorSubscriber_(config_.ddsDomainId, config_.robotMonitorTopic),
      server_(EgmUdpServerConfig{
          static_cast<quint16>(config_.udpPort),
          config_.controlPeriodMs,
          config_.correctionAgeMs,
          config_.receiveTimeoutMs,
          config_.zMaxMm}) {
    piConfig_.setpoint = config_.pressureSetpoint;
    piConfig_.kp = config_.kp;
    piConfig_.ki = config_.ki;
    piConfig_.kd = config_.kd;
    piConfig_.zMaxMm = config_.zMaxMm;
    piConfig_.dzMaxPerCycleMm = config_.dzMaxPerCycleMm;
    piConfig_.staleTimeoutMs = config_.staleTimeoutMs;
    piConfig_.rampRateMmPerS = config_.rampRateMmPerS;

    statusTimer_.setInterval(100);
    connect(&statusTimer_, &QTimer::timeout, this, &EgmPressureController::refreshStatus);
    statusTimer_.start();
}

EgmPressureController::~EgmPressureController() {
    statusTimer_.stop();
    stopWorkerNow();
}

bool EgmPressureController::isListening() const {
    return listening_;
}

bool EgmPressureController::isCorrectionEnabled() const {
    QMutexLocker locker(&paramMutex_);
    return correctionEnabled_;
}

bool EgmPressureController::startListening() {
    if (listening_) {
        return true;
    }

    {
        QMutexLocker locker(&paramMutex_);
        workerError_.clear();
    }

    if (!pressureSubscriber_.start()) {
        QMutexLocker locker(&paramMutex_);
        workerError_ = pressureSubscriber_.lastError();
        return false;
    }
    if (!monitorSubscriber_.start()) {
        const QString monitorError = monitorSubscriber_.lastError();
        pressureSubscriber_.stop();
        QMutexLocker locker(&paramMutex_);
        workerError_ = monitorError;
        return false;
    }

    pi_.reset();
    lastFrameAt_ = {};

    // UDP 收发与 PI 计算必须在本进程的专用线程内完成，绝不能在 Qt 主线程
    // （主线程可能被 PLC 打印确认流程阻塞数秒）。
    serverThread_ = QThread::create([this]() {
        QString error;
        const bool ok = server_.run(
            [this](const EgmRobotFrame& frame, double dtSeconds) {
                return computeCorrection(frame, dtSeconds);
            },
            &error);
        if (!ok) {
            QMutexLocker locker(&paramMutex_);
            workerError_ = error;
        }
    });
    serverThread_->setObjectName("EgmUdpServer");
    serverThread_->start();

    listening_ = true;
    stopping_ = false;
    refreshStatus();
    return true;
}

void EgmPressureController::stopListening() {
    if (!listening_ || stopping_) {
        return;
    }
    stopping_ = true;
    {
        QMutexLocker locker(&paramMutex_);
        correctionEnabled_ = false;
    }

    // 先让 PI 按 rampRate 平滑回零，再断开 UDP，避免控制器因 EGM 超时急停。
    const double rate = std::max(0.1, config_.rampRateMmPerS);
    const double rampMs = std::min(2000.0, config_.zMaxMm / rate * 1000.0) + 100.0;
    QTimer::singleShot(static_cast<int>(rampMs), this, &EgmPressureController::stopWorkerNow);
}

void EgmPressureController::setControlParameters(double setpoint, double kp, double ki,
                                                 double kd, double zMaxMm,
                                                 double dzMaxPerCycleMm) {
    QMutexLocker locker(&paramMutex_);
    piConfig_.setpoint = setpoint;
    piConfig_.kp = kp;
    piConfig_.ki = ki;
    piConfig_.kd = kd;
    piConfig_.zMaxMm = std::max(0.01, zMaxMm);
    piConfig_.dzMaxPerCycleMm = std::max(0.001, dzMaxPerCycleMm);
}

void EgmPressureController::setCorrectionEnabled(bool enabled) {
    QMutexLocker locker(&paramMutex_);
    correctionEnabled_ = enabled;
}

void EgmPressureController::requestZero() {
    QMutexLocker locker(&paramMutex_);
    zeroRequested_ = true;
    correctionEnabled_ = false;
}

EgmPressureController::Status EgmPressureController::status() const {
    Status result;
    result.listening = listening_ && !stopping_;

    const PressureSnapshot pressure = pressureSubscriber_.snapshot();
    const double pressureAgeMs = snapshotAgeMs(pressure.receivedAt);
    result.pressureMean = pressure.meanValue;
    result.pressureAgeMs = pressureAgeMs;
    result.ddsPressureOk = pressure.valid && pressure.windowReady && pressure.sensorRunning &&
                           pressureAgeMs <= config_.staleTimeoutMs;

    const RobotPoseSnapshot robot = monitorSubscriber_.snapshot();
    const double robotAgeMs = snapshotAgeMs(robot.receivedAt);
    result.ddsRobotOk = robot.valid && robotAgeMs <= config_.staleTimeoutMs;
    result.robotTcpZ = robot.z;

    const EgmUdpServerStatus server = server_.status();
    result.egmConnected = server.robotSeen && !server.linkTimeout;
    result.linkTimeout = server.linkTimeout;
    result.framesReceived = server.framesReceived;
    result.framesSent = server.framesSent;
    result.zCorrectionMm = server.lastSentZMm;

    {
        QMutexLocker locker(&paramMutex_);
        result.correctionEnabled = correctionEnabled_;
        result.lastError = workerError_;
    }

    if (result.lastError.isEmpty()) {
        if (!pressureSubscriber_.lastError().isEmpty()) {
            result.lastError = pressureSubscriber_.lastError();
        } else if (!monitorSubscriber_.lastError().isEmpty()) {
            result.lastError = monitorSubscriber_.lastError();
        }
    }
    return result;
}

void EgmPressureController::refreshStatus() {
    emit statusChanged(status());
}

void EgmPressureController::stopWorkerNow() {
    if (!listening_ && serverThread_ == nullptr) {
        return;
    }

    server_.requestStop();
    if (serverThread_ != nullptr) {
        serverThread_->wait(3000);
        delete serverThread_;
        serverThread_ = nullptr;
    }

    pressureSubscriber_.stop();
    monitorSubscriber_.stop();
    listening_ = false;
    stopping_ = false;
    emit statusChanged(status());
}

double EgmPressureController::computeCorrection(const EgmRobotFrame& frame, double dtSeconds) {
    Q_UNUSED(frame);
    const auto now = std::chrono::steady_clock::now();
    if (lastFrameAt_.time_since_epoch().count() != 0) {
        const double gapMs =
            std::chrono::duration<double, std::milli>(now - lastFrameAt_).count();
        // 链路中断后恢复：清空积分与上次输出，避免用旧压力状态继续压。
        if (gapMs > static_cast<double>(config_.receiveTimeoutMs)) {
            pi_.reset();
        }
    }
    lastFrameAt_ = now;

    PressurePiConfig piConfig;
    bool enabled = false;
    bool zero = false;
    {
        QMutexLocker locker(&paramMutex_);
        piConfig = piConfig_;
        enabled = correctionEnabled_;
        zero = zeroRequested_;
        zeroRequested_ = false;
    }

    pi_.setConfig(piConfig);
    if (zero) {
        pi_.reset();
    }

    const PressureSnapshot pressure = pressureSubscriber_.snapshot();
    PressurePiInput input;
    input.pressureMean = pressure.meanValue;
    input.sampleValid = pressure.valid && pressure.windowReady && pressure.sensorRunning;
    input.sampleAgeMs = snapshotAgeMs(pressure.receivedAt, now);
    input.correctionEnabled = enabled;
    return pi_.update(input, dtSeconds);
}

}  // namespace egm
