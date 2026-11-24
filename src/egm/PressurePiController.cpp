#include "egm/PressurePiController.h"

#include <algorithm>

namespace egm {

namespace {
double clampValue(double value, double low, double high) {
    return std::max(low, std::min(high, value));
}
}  // namespace

PressurePiController::PressurePiController(const PressurePiConfig& config) : config_(config) {}

void PressurePiController::setConfig(const PressurePiConfig& config) {
    config_ = config;
    // 不重置 integralTerm_ / outputMm_：整定过程中保持连续。
}

const PressurePiConfig& PressurePiController::config() const {
    return config_;
}

void PressurePiController::reset() {
    outputMm_ = 0.0;
    integralTerm_ = 0.0;
    saturated_ = false;
}

double PressurePiController::update(const PressurePiInput& input, double dtSeconds) {
    if (dtSeconds <= 0.0) {
        return outputMm_;
    }

    const bool sampleFresh =
        input.sampleValid && input.sampleAgeMs <= config_.staleTimeoutMs;

    // 未使能、样本无效或超时：按 rampRate 平滑回零，并清掉积分，避免恢复时突然发力。
    if (!input.correctionEnabled || !sampleFresh) {
        integralTerm_ = 0.0;
        saturated_ = false;
        outputMm_ = rampTowardZero(dtSeconds);
        return outputMm_;
    }

    const double error = input.pressureMean - config_.setpoint;

    // 条件积分：先试算，再由饱和判断决定是否提交。
    const double integralNext = clampValue(
        integralTerm_ + config_.ki * error * dtSeconds,
        -config_.zMaxMm,
        config_.zMaxMm);

    double raw = config_.kp * error + integralNext;
    const bool saturatedPositive = raw > config_.zMaxMm;
    const bool saturatedNegative = raw < -config_.zMaxMm;

    // 饱和且误差方向会加剧饱和 -> 冻结积分（抗 windup）。
    const bool freezeIntegral =
        (saturatedPositive && error > 0.0) || (saturatedNegative && error < 0.0);

    if (!freezeIntegral) {
        integralTerm_ = integralNext;
        raw = config_.kp * error + integralTerm_;
    } else {
        raw = config_.kp * error + integralTerm_;
    }

    saturated_ = raw > config_.zMaxMm || raw < -config_.zMaxMm;

    const double clamped = clampValue(raw, -config_.zMaxMm, config_.zMaxMm);

    // 速率限制：每周期变化量不超过 dzMaxPerCycleMm。
    const double lower = std::max(outputMm_ - config_.dzMaxPerCycleMm, -config_.zMaxMm);
    const double upper = std::min(outputMm_ + config_.dzMaxPerCycleMm, config_.zMaxMm);
    outputMm_ = clampValue(clamped, lower, upper);

    return outputMm_;
}

double PressurePiController::rampTowardZero(double dtSeconds) {
    const double step = config_.rampRateMmPerS * dtSeconds;
    if (outputMm_ > 0.0) {
        return std::max(0.0, outputMm_ - step);
    }
    if (outputMm_ < 0.0) {
        return std::min(0.0, outputMm_ + step);
    }
    return 0.0;
}

double PressurePiController::outputMm() const {
    return outputMm_;
}

double PressurePiController::integralTerm() const {
    return integralTerm_;
}

bool PressurePiController::saturated() const {
    return saturated_;
}

}  // namespace egm
