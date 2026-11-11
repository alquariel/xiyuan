#pragma once

#include <QObject>
#include <QRandomGenerator>
#include <QTimer>

// -----------------------------------------------------------------------------
// PressureSimulator — 模拟法向压力 Fz 数据源
//
// 每 10 ms 采样一次（独立于 EGM 位姿流 ~4 ms 的发送节奏），内部做
// random-walk + clamp，让压力曲线有真实传感器的"惯性"感而不是纯随机
// 跳变。长期均值 80 N、波动范围 ±20 N（即 [60, 100] N）。
//
// CSV 写入端不需要关心采样相位，每次写行时调用 currentValue() 取
// "此刻" 的压力值直接追加即可——用户明确不需要时间戳对齐。
// -----------------------------------------------------------------------------
class PressureSimulator final : public QObject {
    Q_OBJECT

public:
    explicit PressureSimulator(QObject* parent = nullptr);

    // ---- 参数（默认即按本次需求设定，也允许外部覆盖） ----
    void setMean(double newton) { meanN_ = newton; }
    void setAmplitude(double newton) { ampN_ = newton; }   // ±newton 波动
    void setStepPerTick(double newton) { stepN_ = newton; } // 每 10 ms 最大跳变
    void setTickIntervalMs(int ms);                          // 默认 10

    // ---- 生命周期 ----
    void start();
    void stop();
    bool isRunning() const { return timer_.isActive(); }

    // ---- 读数 ----
    // 取当前压力值（单位 N）。多线程场景下调用方需自行保证串行（上位机
    // 目前只在主线程读写，所以直接取即可）。
    double currentValue() const { return currentN_; }

signals:
    // 每次 tick 都 emit，方便以后如果有实时显示压力曲线的 UI 直接挂。
    void ticked(double valueN);

private slots:
    void onTick();

private:
    QTimer timer_;
    QRandomGenerator rng_;

    // 配置
    int tickIntervalMs_ = 10;       // 10 ms 采样一次
    double meanN_ = 80.0;           // 均值 80 N
    double ampN_ = 20.0;            // 波动 ±20 N
    double stepN_ = 2.0;            // 每 tick 最大跳变 2 N → 约 200 N/s 变化率

    // 状态
    double currentN_ = 80.0;
};
