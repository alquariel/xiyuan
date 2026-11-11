#include "pressure/PressureSimulator.h"

#include <QRandomGenerator>
#include <algorithm>

PressureSimulator::PressureSimulator(QObject* parent)
    : QObject(parent)
    , rng_(QRandomGenerator::global()->generate())
{
    timer_.setInterval(tickIntervalMs_);
    connect(&timer_, &QTimer::timeout, this, &PressureSimulator::onTick);
}

void PressureSimulator::setTickIntervalMs(int ms) {
    tickIntervalMs_ = std::max(1, ms);
    timer_.setInterval(tickIntervalMs_);
}

void PressureSimulator::start() {
    // 每次启动都从均值重新出发，避免上一轮会话的尾部值带到新一轮。
    currentN_ = meanN_;
    timer_.start();
}

void PressureSimulator::stop() {
    timer_.stop();
}

void PressureSimulator::onTick() {
    // 均匀随机步长 ∈ [-stepN_, +stepN_]
    const double delta = (rng_.generateDouble() * 2.0 - 1.0) * stepN_;
    double next = currentN_ + delta;

    // 均值软回归：越远离 meanN_，被拉回的力度越大。
    // 这让分布自然收敛到 meanN_ 附近，而不是在 clamp 边界贴边徘徊。
    constexpr double kPull = 0.02;   // 每 tick 拉回 2% 的偏离量
    next += (meanN_ - next) * kPull;

    // 硬边界 [mean - amp, mean + amp]
    const double lo = meanN_ - ampN_;
    const double hi = meanN_ + ampN_;
    next = std::clamp(next, lo, hi);

    currentN_ = next;
    emit ticked(currentN_);
}
