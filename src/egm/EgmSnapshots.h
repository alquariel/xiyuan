#pragma once

#include <chrono>

namespace egm {

// 压力 DDS 最新样本快照。DDS 回调只负责写快照，不做任何控制计算。
struct PressureSnapshot {
    bool valid = false;
    bool windowReady = false;        // 统计窗口已满（PressureStatistics window_ready）
    bool sensorRunning = false;      // 传感器在线
    double rawValue = 0.0;
    double meanValue = 0.0;          // 窗口均值，PI 的反馈量
    double standardDeviationValue = 0.0;
    std::chrono::steady_clock::time_point receivedAt{};
};

// 机器人监控 DDS 最新位姿快照（TCP + 四元数），用于界面显示与软限位判断。
struct RobotPoseSnapshot {
    bool valid = false;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double q1 = 1.0;
    double q2 = 0.0;
    double q3 = 0.0;
    double q4 = 0.0;
    std::chrono::steady_clock::time_point receivedAt{};
};

// 快照年龄（毫秒）。未收到过任何样本时返回 +inf，调用方必须先判断 valid。
inline double snapshotAgeMs(
    std::chrono::steady_clock::time_point receivedAt,
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now()) {
    if (receivedAt.time_since_epoch().count() == 0) {
        return 1e18;
    }
    const auto age = std::chrono::duration_cast<std::chrono::microseconds>(now - receivedAt).count();
    return age < 0 ? 0.0 : static_cast<double>(age) / 1000.0;
}

}  // namespace egm
