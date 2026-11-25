#pragma once

namespace egm {

// PID 控制律参数（kd 默认 0 时退化为 PI）。压力单位为传感器原始单位，输出单位为 mm。
struct PressurePiConfig {
    double setpoint = 0.0;            // 目标压力
    double kp = 0.0;                  // mm / 压力单位
    double ki = 0.0;                  // mm / (压力单位 * s)
    double kd = 0.0;                  // mm / (压力单位 / s)
    double zMaxMm = 3.0;              // 输出限幅（单轴）
    double dzMaxPerCycleMm = 0.1;     // 每周期最大变化量
    double staleTimeoutMs = 300.0;    // 压力样本失效阈值
    double rampRateMmPerS = 5.0;      // 失效/关闭时回零速率
};

// 单个控制周期的输入快照。
struct PressurePiInput {
    double pressureMean = 0.0;        // 压力窗口均值
    bool sampleValid = false;         // window_ready && sensor_running
    double sampleAgeMs = 0.0;         // 距最近一次样本的时长
    bool correctionEnabled = false;   // 页签上的修正使能
};

// 纯逻辑 PI 控制器：无 Qt / 无网络依赖，便于单元测试。
//
//   工具 +Z = 离开工件表面（抬刀）
//   pressureMean > setpoint（压力过大）-> z_corr > 0 -> 抬刀
//   pressureMean < setpoint（压力不足）-> z_corr < 0 -> 下压
class PressurePiController {
public:
    PressurePiController() = default;
    explicit PressurePiController(const PressurePiConfig& config);

    // 热更新参数：不重置积分与输出，避免整定过程中产生扰动。
    void setConfig(const PressurePiConfig& config);
    const PressurePiConfig& config() const;

    // 清零积分与输出（停机、回滚时使用）。
    void reset();

    // 执行一个控制周期，返回本周期输出 z_corr（mm）。
    double update(const PressurePiInput& input, double dtSeconds);

    double outputMm() const;
    double integralTerm() const;
    bool saturated() const;

private:
    double rampTowardZero(double dtSeconds);

    PressurePiConfig config_;
    double outputMm_ = 0.0;
    double integralTerm_ = 0.0;
    double prevPressure_ = 0.0;
    bool hasPrevPressure_ = false;
    bool saturated_ = false;
};

}  // namespace egm
