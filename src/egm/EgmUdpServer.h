#pragma once

#include <atomic>
#include <functional>
#include <QMutex>
#include <QString>

#include "egm/EgmWireCodec.h"

namespace egm {

struct EgmUdpServerConfig {
    quint16 listenPort = 6510;
    int controlPeriodMs = 48;         // 首帧的 dt 回退值（EGM 周期）
    int correctionAgeMs = 48;         // EgmPathCorr.age，单位毫秒
    int receiveTimeoutMs = 500;       // 超过该时长未收到 EgmRobot 视为链路中断
    double hardZLimitMm = 3.0;        // 服务端最后一道输出硬限幅
};

struct EgmUdpServerStatus {
    bool listening = false;
    bool robotSeen = false;
    bool linkTimeout = false;
    quint64 framesReceived = 0;
    quint64 framesSent = 0;
    quint32 lastRobotSeqno = 0;
    double lastSentZMm = 0.0;
    double lastReceiveAgeMs = 0.0;
    EgmRobotFrame lastRobot;
};

// EGM Path Correction 的 UDP 服务端：
//   - PC 绑定 6510，控制器作为 UDP 客户端发 EgmRobot；
//   - 每收到一帧 EgmRobot，立即回一帧 EgmSensorPathCorr（先收后发）；
//   - 未收到首帧前绝不发送；
//   - seqno 每帧只自增一次；
//   - 超过 receiveTimeoutMs 未收到新帧时停止发送并置 linkTimeout。
class EgmUdpServer final {
public:
    // 返回本周期 z 修正量（mm）。dtSeconds 为相邻两帧的实际间隔。
    using CorrectionHandler = std::function<double(const EgmRobotFrame& frame, double dtSeconds)>;

    explicit EgmUdpServer(EgmUdpServerConfig config);
    ~EgmUdpServer();

    EgmUdpServer(const EgmUdpServer&) = delete;
    EgmUdpServer& operator=(const EgmUdpServer&) = delete;

    // 绑定端口并阻塞收发，直到 requestStop()。必须在工作线程中调用。
    bool run(const CorrectionHandler& handler, QString* errorMessage);

    // 线程安全；请求 run() 退出（最多一个轮询周期内生效）。
    void requestStop();

    // 线程安全快照。
    EgmUdpServerStatus status() const;

    bool isRunning() const;

private:
    void setStatus(const EgmUdpServerStatus& status);

    EgmUdpServerConfig config_;
    std::atomic<bool> stopRequested_{false};
    std::atomic<bool> running_{false};

    mutable QMutex mutex_;
    EgmUdpServerStatus status_;
};

}  // namespace egm
