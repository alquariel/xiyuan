#pragma once

#include <QFile>
#include <QObject>
#include <QString>
#include <QTextStream>
#include <QTimer>

#include "app/AppConfig.h"
#include "pressure/PressureSimulator.h"
#include "robot/EgmClient.h"
#include "robot/PathLoader.h"

class IRobotCommandSocket;
class PressureChartWindow;
class QWidget;

// EGM 会话编排器。
//
// 把原先散落在 MainWindow 里的 EGM 相关逻辑集中到这里：
//   路径加载 → 启动前校验（0 # 心跳、113 位姿、整体偏移确认、姿态对齐）
//   → 110/120 启动 → 4 ms 插值 tick（250 Hz UDP 由 EgmClient 负责）
//   → CSV / 实时曲线记录 → 111 停止 / 看门狗诊断。
//
// 本类不拥有 5000 端口的连接，只通过 IRobotCommandSocket* 发送文本指令；
// 状态文本通过 statusTextChanged 信号交给 UI 显示，自身不直接操作控件。
class EgmFlowController : public QObject {
    Q_OBJECT

public:
    EgmFlowController(const AppConfig& config,
                      IRobotCommandSocket* commandSocket,
                      QWidget* dialogParent,
                      QObject* parent = nullptr);
    ~EgmFlowController() override;

    bool isActive() const { return active_; }

public slots:
    void loadPathFromDialog();
    void startSession();
    void pause();
    void resume();
    void stop();

signals:
    void statusTextChanged(const QString& text);
    void statusToolTipChanged(const QString& text);

private slots:
    void tick();
    void onFeedbackPose(const EgmTcpPose& pose);
    void onStartWatchdogTimeout();

private:
    void startCsvRecording();
    void stopCsvRecording();

    AppConfig config_;
    IRobotCommandSocket* commandSocket_ = nullptr;
    QWidget* dialogParent_ = nullptr;

    // EGM motion layer — 4 ms interpolation tick feeding a 250 Hz UDP stream.
    EgmClient egmClient_;
    PathLoader pathLoader_;
    QTimer tickTimer_;
    QTimer startWatchdog_;

    // Currently-loaded txt path file (set by loadPathFromDialog, consumed by
    // startSession). Empty means "no path loaded".
    QString loadedPathFile_;

    // 速度取自路径文件首行的 v 列（笛卡尔 mm/s、关节 deg/s）。
    double traverseSpeedMmPerSec_ = 50.0;
    double jointSpeedDegPerSec_ = 1.0;

    // EGM path iterator state (driven by tickTimer_)
    int segmentIndex_ = 0;
    float segmentProgressMm_ = 0.0f;
    float jointProgressDeg_ = 0.0f;
    bool active_ = false;

    // Pose-mode attitude alignment, computed at "start" from the robot's live
    // TCP pose (CASE 113). Commanded orientation = alignQuat_ * pathQuat.
    PathQuaternion alignQuat_{1, 0, 0, 0};
    bool alignValid_ = false;

    // Pose-mode translation compensation: the whole path is shifted by
    // (live TCP - path start) so it begins exactly where the robot stands.
    PathVector3 pathOffsetMm_{};

    // 控制器最近一帧 EGM 反馈的真实 TCP 位姿，下一轮启动优先用它做起点补偿。
    EgmTcpPose lastFeedbackPose_{};
    bool hasLastFeedbackPose_ = false;

    // 启动看门狗上下文（写入诊断弹窗）。
    QString startContext_;

    // EGM 反馈位姿记录：每次 EGM 会话新建一个 data_save/data_*.csv。
    QFile csvFile_;
    QTextStream csvStream_;
    bool csvRecording_ = false;
    int csvRowsSinceFlush_ = 0;

    // 模拟法向压力 Fz 数据源：10 ms 采样，均值 80 N、波动 ±20 N。
    PressureSimulator pressureSim_{this};

    // EGM 运行时的实时曲线窗口（独立顶级窗口，EGM 启停时显隐）。
    PressureChartWindow* chartWindow_ = nullptr;

    // 图表窗口需要的缓存：目标 z（tick 里每次算完 setTargetPose 后更新）。
    double chartTargetZmm_ = 0.0;
    double chartPressureTargetN_ = 80.0;
};
