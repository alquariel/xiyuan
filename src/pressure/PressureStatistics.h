#pragma once

#include <deque>

class PressureStatistics final {
public:
    explicit PressureStatistics(int windowSamples);

    void addSample(double value);
    void clear();

    bool windowReady() const;
    int sampleCount() const;
    int windowSamples() const;

    double mean() const;
    double variance() const;
    double standardDeviation() const;
    double peak() const;

private:
    double computeMean() const;

    int windowSamples_;
    std::deque<double> samples_;
};
