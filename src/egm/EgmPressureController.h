#pragma once

#include <QObject>
#include <QMutex>
#include <QTimer>

#include <chrono>

#include "app/AppConfig.h"
#include "egm/EgmUdpServer.h"
#include "egm/PressurePiController.h"
#include "egm/PressureSampleSubscriber.h"
#include "egm/RobotMonitorSubscriber.h"

class QThread;

namespace egm {

// EGM 压力修正的主线程编排对象：
//   - 持有 DDS 订阅、UDP 服务端与 PI 控制律；
//   - UDP 收发与 PI 计算只在专用工作线程内执行；
//   - 通过 100 ms 定时器向界面发布只读状态快照。
class EgmPressureController final : public QObject {
    Q_OBJECT

public:
    struct Status {
        bool listening = false;
        bool ddsPressureOk = false;
        bool ddsRobotOk = false;
        bool egmConnected = false;
        bool linkTimeout = false;
        bool correctionEnabled = false;
        double pressureMean = 0.0;
        double pressureAgeMs = 0.0;
        double zCorrectionMm = 0.0;
        double robotTcpZ = 0.0;
        quint64 framesReceived = 0;
        quint64 framesSent = 0;
        QString lastError;
    };

    explicit EgmPressureController(EgmPressureConfig config, QObject* parent = nullptr);
    ~EgmPressureController() override;

    const EgmPressureConfig& config() const { return config_; }

    bool isListening() const;
    bool isCorrectionEnabled() const;

    // 绑定 UDP 端口并启动 DDS 订阅。返回 false 表示 DDS 订阅失败。
    bool startListening();

    // 先把修正平滑回零，再停止 UDP 线程与 DDS 订阅。
    void stopListening();

    // 参数热更新：不重置积分与当前输出。
    void setControlParameters(double setpoint, double kp, double ki, double kd, double zMaxMm,
                              double dzMaxPerCycleMm);
    void setCorrectionEnabled(bool enabled);

    // 请求平滑回零（只影响后续帧，链路保持）。
    void requestZero();

    Status status() const;

signals:
    void statusChanged(const egm::EgmPressureController::Status& status);

private:
    double computeCorrection(const EgmRobotFrame& frame, double dtSeconds);
    void refreshStatus();
    void stopWorkerNow();

    EgmPressureConfig config_;
    PressureSampleSubscriber pressureSubscriber_;
    RobotMonitorSubscriber monitorSubscriber_;
    EgmUdpServer server_;
    QThread* serverThread_ = nullptr;
    QTimer statusTimer_;

    mutable QMutex paramMutex_;
    PressurePiConfig piConfig_;
    bool correctionEnabled_ = false;
    bool zeroRequested_ = false;
    QString workerError_;

    // 只在 EGM 工作线程内访问。
    PressurePiController pi_;
    std::chrono::steady_clock::time_point lastFrameAt_{};

    bool listening_ = false;
    bool stopping_ = false;
};

}  // namespace egm
