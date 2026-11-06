#pragma once

#include <cstdint>

#include "pressure/generated/PressureSample.hpp"

// Hand-written pressure code should not depend on the generated DDS class in
// every file. This small struct is the application's own pressure sample
// shape; the inline conversion below is the only place that maps it into the
// Fast DDS generated type.
struct PressureSampleData {
    std::uint64_t timestampMs = 0;
    std::uint64_t sequence = 0;
    double rawValue = 0.0;
    double meanValue = 0.0;
    double varianceValue = 0.0;
    double standardDeviationValue = 0.0;
    double peakValue = 0.0;
    bool windowReady = false;
    bool sensorRunning = false;
};

// Build the generated DDS sample. The generated class uses IDL field names
// such as timestamp_ms(), while the hand-written C++ code uses normal C++
// member names. Keeping this mapping explicit makes subscriber-facing fields
// easy to audit against PressureSample.idl.
inline AfpPressureSample toDdsSample(const PressureSampleData& sample) {
    AfpPressureSample ddsSample;
    ddsSample.timestamp_ms(sample.timestampMs);
    ddsSample.sample_sequence(sample.sequence);
    ddsSample.raw_value(sample.rawValue);
    ddsSample.mean_value(sample.meanValue);
    ddsSample.variance_value(sample.varianceValue);
    ddsSample.standard_deviation_value(sample.standardDeviationValue);
    ddsSample.peak_value(sample.peakValue);
    ddsSample.window_ready(sample.windowReady);
    ddsSample.sensor_running(sample.sensorRunning);
    return ddsSample;
}
