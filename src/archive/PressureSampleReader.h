#pragma once

#include <QString>

#include "app/AppConfig.h"
#include "archive/TimeSeriesPoint.h"
#include "pressure/generated/PressureSample.hpp"

namespace eprosima::fastdds::dds {
class DataReader;
class DomainParticipant;
class Subscriber;
class Topic;
class TypeSupport;
}

// Convert one generated DDS pressure sample into the backend-neutral point
// model consumed by the InfluxDB writer.
TimeSeriesPoint pressureSampleToPoint(const AfpPressureSample& sample, const PressureArchiveConfig& config);

// Owns the Fast DDS objects required to subscribe to AfpPressureSample data.
// The class hides participant/topic/reader lifetime details from the archive
// application coordinator.
class PressureSampleReader final {
public:
    PressureSampleReader(PressureArchiveConfig config, TimeSeriesPointHandler handler);
    ~PressureSampleReader();

    PressureSampleReader(const PressureSampleReader&) = delete;
    PressureSampleReader& operator=(const PressureSampleReader&) = delete;

    bool start();
    void stop();
    QString lastError() const;

private:
    class Listener;

    void setError(const QString& message);

    PressureArchiveConfig config_;
    TimeSeriesPointHandler handler_;
    eprosima::fastdds::dds::DomainParticipant* participant_ = nullptr;
    eprosima::fastdds::dds::Subscriber* subscriber_ = nullptr;
    eprosima::fastdds::dds::Topic* topic_ = nullptr;
    eprosima::fastdds::dds::DataReader* reader_ = nullptr;
    eprosima::fastdds::dds::TypeSupport* typeSupport_ = nullptr;
    Listener* listener_ = nullptr;
    QString lastError_;
};
