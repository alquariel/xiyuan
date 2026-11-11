#pragma once

#include <QByteArray>
#include <QObject>

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>

#include "app/AppConfig.h"
#include "pressure/PressureController.h"

// ABB EGM (Externally Guided Motion) UDP 客户端。
//
// 协议：Google Protobuf（egm.proto），不是定长二进制帧。
//   · 控制器（robot）主动向 本机:6510 发 EgmRobot 反馈帧；
//   · 本机收到后，把 EgmSensor（MSGTYPE_CORRECTION）回发到该帧的源地址/源端口。
//   · 位置单位为 mm，姿态为四元数 [u0,u1,u2,u3] = [w,x,y,z]。
//
// 这是 UdpUc / EGMSetupUC 的标准时序，与 D:\pyqt5_2025_1012_WDY_pos_control
// 里的 ABBRobotEGM 参考实现一致。

// 纯 POD 的 TCP 位姿：x/y/z 单位 mm，四元数为 scalar-first（q1=w, q2..q4=x/y/z）。
// 不依赖 Qt GUI 几何类型，便于只用 Qt6::Core + Qt6::Network 做单元测试。
struct EgmTcpPose {
    double x = 0;
    double y = 0;
    double z = 0;
    double q1 = 1;
    double q2 = 0;
    double q3 = 0;
    double q4 = 0;
};

Q_DECLARE_METATYPE(EgmTcpPose)

class EgmClient : public QObject {
    Q_OBJECT

public:
    explicit EgmClient(EgmConfig config, QObject* parent = nullptr);
    ~EgmClient() override;

    EgmClient(const EgmClient&) = delete;
    EgmClient& operator=(const EgmClient&) = delete;

    // 流线程是否在跑。
    bool isStreaming() const;

    // 是否已经收到控制器发来的第一帧 EgmRobot（收到后才开始回发 EgmSensor）。
    bool isPeerConnected() const;

    // 实际监听的 UDP 端口（config.port 被占用时会绑定失败并返回 0）。
    quint16 boundPort() const;

    // 绑定 UDP 端口并启动 250 Hz 收发循环。config 未启用或端口绑定失败返回 false。
    bool start();

    // 请求停止；带短暂超时，避免线程卡死时调用方被永久阻塞。
    void stop();

    // 线程安全地写入下一帧要发给控制器的 TCP 目标位姿（绝对位姿，工具/工件坐标系见 RAPID 端）。
    void setTargetPose(const EgmTcpPose& pose);

    // EGM 有两种运动通道：位姿（EGMActPose）与关节（EGMActJoint），
    // 二者在协议里对应 EgmPlanned 的不同字段，必须与 RAPID 侧的会话模式一致。
    enum class Mode { Pose, Joint };

    void setMode(Mode mode);
    Mode mode() const;

    // 线程安全地写入下一帧要发给控制器的 6 个关节角。
    // 单位：度（DEG）。ABB EGM 关节通道（EGMActJoint/EGMRunJoint）的
    // EgmPlanned.joints 报文单位就是度，不是弧度，这里不做 deg->rad 换算。
    void setTargetJoints(const double jointsDeg[6]);

    // ---- 导纳式压力闭环 ----
    // 默认关闭；启用后每帧目标位姿会沿工具系 Z 轴叠加 PID 算出的高度修正。
    // 注意：仅 Pose 模式生效，Joint 模式不叠加（关节角无法表达法向位移）。
    void enablePressureControl(bool on);
    bool pressureControlEnabled() const;

    // 线程安全地注入当前测量压力 / 目标压力（N），主线程调用。
    void setPressureMeasured(double newton);
    void setPressureTarget(double newton);

    // 直接替换整套 PID 参数（随时可调，立即生效）。
    void setPressureControllerConfig(const PressureController::Config& cfg);

signals:
    // 收发线程启动/停止。
    void streamingStateChanged(bool running);

    // 首次收到 EgmRobot（true）/ 停止时断开（false）。
    void peerStateChanged(bool connected);

    // 每约 1 秒上报一次已发送帧数，供 UI 判断链路是否健康。
    void packetCounterTick(quint64 packetsSent);

    // 控制器每个反馈周期（EgmRobot.feedBack.cartesian）上报的实时 TCP 位姿。
    // 单位 mm，四元数顺序与各坐标轴定义见 RAPID 端 EGMActPose 的 Tool/WObj。
    // 由收包线程排队发到主线程，供 CSV 记录等使用。
    void feedbackPoseReceived(const EgmTcpPose& pose);

    // 每帧 PID 内部状态（调试/CSV 记录用）：h_offset 单位 mm、压力误差单位 N。
    void pressureControllerState(double hOffsetMm, double errorN);

private:
    void runStreamingLoop();

    EgmConfig config_;

    std::thread worker_;
    std::atomic_bool stopRequested_{false};
    std::atomic_bool running_{false};
    std::atomic_bool peerConnected_{false};
    std::atomic<quint64> packetsSent_{0};
    std::atomic<quint16> boundPort_{0};

    // 主线程写、UDP 线程读的目标位姿。
    mutable std::mutex poseMutex_;
    EgmTcpPose currentPose_{};
    Mode mode_ = Mode::Pose;
    double currentJoints_[6] = {0, 0, 0, 0, 0, 0};  // DEG

    // ---- 导纳压力闭环状态 ----
    // pressureMutex_ 与 poseMutex_ 分开，避免主线程调 setPressureMeasured()
    // 和 setTargetPose() 时互相阻塞。
    mutable std::mutex pressureMutex_;
    std::atomic_bool pressureEnabled_{false};
    PressureController pressureController_;  // runStreamingLoop 内部持有
    double pressureMeasured_ = 0.0;
    double pressureTarget_ = 80.0;
};
