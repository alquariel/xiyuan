#pragma once

// PressureController — EGM 位姿流上的导纳式法向压力闭环
//
// 设计与约束：
//   1. 这是纯数学控制器，不依赖 Qt / 任何传感器库，便于单元测试和复用
//   2. 控制契约（统一数据口径，避免方向反转/轴向串扰）：
//        · 压力输入 p_meas / p_target    —— 单位 N
//        · 姿态输入 quat                  —— scalar-first [w,x,y,z]
//                                          与 EgmTcpPose 的 q1..q4 完全一致
//        · 输出 h_offset * n              —— 单位 mm，沿工具系 Z 轴（法向）
//                                          直接叠加到 TCP 的 x/y/z 上
//        · dt                             —— 秒，必须 = EGM 帧周期（4ms）
//   3. 每次 EGM 周期调用 step()，返回的 h_offset 会在内部累加上限，
//      单周期增量 max_step、累计高度 max_height 两级限幅
//   4. 纯 P 闭环（Ki=Kd=0）最安全，先跑通再说；加 I/D 时注意 dt 缩放
//   5. Config 定义在类外（全局 namespace），避免 GCC 对嵌套类默认构造
//      函数在类体内引用的挑剔

#include <algorithm>
#include <cmath>

struct PressureControllerConfig {
    double Kp = 0.1;        // mm/N  · 注意 dt=4ms 比参考实现 20ms 快 5×，
                            //        Kp 按 dt 比例缩小更稳，实测再调
    double Ki = 0.0;
    double Kd = 0.0;
    double dt = 0.004;      // 秒，EGM 250Hz 周期
    double max_step = 0.1;  // 单周期最大高度增量 mm（dt=4ms 下 0.1mm ≈ 25mm/s）
    double max_height = 3.0; // 累计高度硬限 mm（导纳"弹簧"的最大变形）
    double alpha = 0.3;     // 压力低通：p_f = α * p + (1-α) * p_prev
    double deadband = 2.0;  // 压力误差绝对值 < deadband 视为 0，避免抖振
};

class PressureController {
public:
    using Config = PressureControllerConfig;

    explicit PressureController(Config cfg = {}) : cfg_(cfg) {}

    void setConfig(const Config& c) { cfg_ = c; }
    const Config& config() const { return cfg_; }

    void reset() {
        h_offset_ = 0.0;
        integral_ = 0.0;
        prev_e_ = 0.0;
        prev_p_ = 0.0;
        prev_p_defined_ = false;
    }

    // ---- 四元数 → 工具系 Z 轴法向（手动矩阵旋转，无需第三方库）----
    // q 按 scalar-first [w,x,y,z] 传入，返回单位向量 n = R(q) * [0,0,1]
    static void quatToNormal(double w, double x, double y, double z,
                             double* nOutX, double* nOutY, double* nOutZ) {
        const double nrm = std::sqrt(w*w + x*x + y*y + z*z);
        if (nrm < 1e-12) {
            *nOutX = 0.0; *nOutY = 0.0; *nOutZ = 1.0;
            return;
        }
        w /= nrm;  x /= nrm;  y /= nrm;  z /= nrm;
        *nOutX = 2.0 * (x * z + w * y);
        *nOutY = 2.0 * (y * z - w * x);
        *nOutZ = 1.0 - 2.0 * (x * x + y * y);
        const double nzrm = std::sqrt(*nOutX * *nOutX + *nOutY * *nOutY + *nOutZ * *nOutZ);
        if (nzrm > 1e-12) {
            *nOutX /= nzrm;
            *nOutY /= nzrm;
            *nOutZ /= nzrm;
        }
    }

    // ---- 控制器每周期 step ----
    // quat: [w,x,y,z] scalar-first；out_xyz 是沿法向的高度修正量（mm）
    double step(double pMeas, double pTarget,
                double qw, double qx, double qy, double qz,
                double* outDx, double* outDy, double* outDz) {
        // 1) 法向
        double n[3] = {0, 0, 1};
        quatToNormal(qw, qx, qy, qz, &n[0], &n[1], &n[2]);

        // 2) 压力低通
        double pF = pMeas;
        if (prev_p_defined_) {
            pF = cfg_.alpha * pMeas + (1.0 - cfg_.alpha) * prev_p_;
        }
        prev_p_ = pF;
        prev_p_defined_ = true;

        // 3) 压力误差 + 死区
        double e = pTarget - pF;
        if (std::abs(e) < cfg_.deadband) {
            e = 0.0;
        }

        // 4) PID（输出增量 mm）
        const double P = cfg_.Kp * e;
        integral_ += cfg_.Ki * e * cfg_.dt;
        // Anti-windup：积分限幅到 ±max_height，避免 I 项把 h_offset 推到死循环
        integral_ = std::clamp(integral_, -cfg_.max_height, cfg_.max_height);
        const double D = cfg_.Kd * (e - prev_e_) / cfg_.dt;
        prev_e_ = e;

        double deltaH = P + integral_ + D;

        // 5) 单周期限幅
        deltaH = std::clamp(deltaH, -cfg_.max_step, cfg_.max_step);

        // 6) 累计高度（核心）+ 硬限幅
        h_offset_ += deltaH;
        h_offset_ = std::clamp(h_offset_, -cfg_.max_height, cfg_.max_height);

        // 7) 沿法向输出
        *outDx = h_offset_ * n[0];
        *outDy = h_offset_ * n[1];
        *outDz = h_offset_ * n[2];

        return e;
    }

    double hOffset() const { return h_offset_; }

private:
    Config cfg_;

    double h_offset_ = 0.0;
    double integral_ = 0.0;
    double prev_e_ = 0.0;
    double prev_p_ = 0.0;
    bool   prev_p_defined_ = false;
};
