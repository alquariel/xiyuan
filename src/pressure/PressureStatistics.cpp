#include "pressure/PressureStatistics.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

PressureStatistics::PressureStatistics(int windowSamples)
    : windowSamples_(windowSamples) {
    if (windowSamples_ <= 0) {
        throw std::invalid_argument("Pressure statistics window must be greater than 0.");
    }
}

void PressureStatistics::addSample(double value) {
    samples_.push_back(value);
    while (static_cast<int>(samples_.size()) > windowSamples_) {
        samples_.pop_front();
    }
}

void PressureStatistics::clear() {
    samples_.clear();
}

bool PressureStatistics::windowReady() const {
    return static_cast<int>(samples_.size()) >= windowSamples_;
}

int PressureStatistics::sampleCount() const {
    return static_cast<int>(samples_.size());
}

int PressureStatistics::windowSamples() const {
    return windowSamples_;
}

double PressureStatistics::mean() const {
    return windowReady() ? computeMean() : 0.0;
}

double PressureStatistics::variance() const {
    if (!windowReady()) {
        return 0.0;
    }

    // Match pressure_monitor.py: population variance over the newest N samples.
    const double currentMean = computeMean();
    double sum = 0.0;
    for (double sample : samples_) {
        const double diff = sample - currentMean;
        sum += diff * diff;
    }
    return sum / static_cast<double>(samples_.size());
}

double PressureStatistics::standardDeviation() const {
    return std::sqrt(variance());
}

double PressureStatistics::peak() const {
    if (!windowReady()) {
        return 0.0;
    }
    return *std::max_element(samples_.begin(), samples_.end());
}

double PressureStatistics::computeMean() const {
    // Match pressure_monitor.py: statistics are reported only after the window fills.
    const double sum = std::accumulate(samples_.begin(), samples_.end(), 0.0);
    return sum / static_cast<double>(samples_.size());
}
