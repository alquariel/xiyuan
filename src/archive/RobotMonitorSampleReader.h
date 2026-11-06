#pragma once

#include <QString>

#include "app/AppConfig.h"
#include "archive/TimeSeriesPoint.h"
#include "robot_monitor/generated/RobotMonitorSample.hpp"

namespace eprosima::fastdds::dds {
class DataReader;
class DomainParticipant;
class Subscriber;
class Topic;
class TypeSupport;
}

// Convert one generated DDS robot monitor sample into the backend-neutral point
// model consumed by the InfluxDB writer.
TimeSeriesPoint robotMonitorSampleToPoint(
    const AfpRobotMonitorSample& sample,
    const RobotMonitorArchiveConfig& config);

// Owns the Fast DDS objects required to subscribe to AfpRobotMonitorSample data.
// DdsInfluxSubscriberApp uses this the same way it uses PressureSampleReader,
// so additional data sources can share the archive writer path.
class RobotMonitorSampleReader final {
public:
    RobotMonitorSampleReader(RobotMonitorArchiveConfig config, TimeSeriesPointHandler handler);
    ~RobotMonitorSampleReader();

    RobotMonitorSampleReader(const RobotMonitorSampleReader&) = delete;
    RobotMonitorSampleReader& operator=(const RobotMonitorSampleReader&) = delete;

    bool start();
    void stop();
    QString lastError() const;

private:
    class Listener;

    void setError(const QString& message);

    RobotMonitorArchiveConfig config_;
    TimeSeriesPointHandler handler_;
    eprosima::fastdds::dds::DomainParticipant* participant_ = nullptr;
    eprosima::fastdds::dds::Subscriber* subscriber_ = nullptr;
    eprosima::fastdds::dds::Topic* topic_ = nullptr;
    eprosima::fastdds::dds::DataReader* reader_ = nullptr;
    eprosima::fastdds::dds::TypeSupport* typeSupport_ = nullptr;
    Listener* listener_ = nullptr;
    QString lastError_;
};
