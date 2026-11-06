#pragma once

#include <QString>

#include "app/AppConfig.h"
#include "robot_monitor/IRobotMonitorDdsPublisher.h"
#include "robot_monitor/RobotMonitorDdsTypes.h"

namespace eprosima::fastdds::dds {
class DataWriter;
class DomainParticipant;
class Publisher;
class Topic;
class TypeSupport;
}

// Owns the Fast DDS objects required to publish parsed robot monitor samples.
// RobotMonitoringService can use this small wrapper without knowing about DDS
// participants, topics, writers, or generated type-support lifetimes.
class RobotMonitorDdsPublisher final : public IRobotMonitorDdsPublisher {
public:
    explicit RobotMonitorDdsPublisher(RobotMonitorConfig config);
    ~RobotMonitorDdsPublisher();

    bool initialize();
    bool publish(const RobotMonitorSample& sample) override;
    void shutdown();

    bool initialized() const;
    QString topicName() const;
    QString lastError() const override;

private:
    void setError(const QString& message);

    RobotMonitorConfig config_;
    eprosima::fastdds::dds::DomainParticipant* participant_ = nullptr;
    eprosima::fastdds::dds::Publisher* publisher_ = nullptr;
    eprosima::fastdds::dds::Topic* topic_ = nullptr;
    eprosima::fastdds::dds::DataWriter* writer_ = nullptr;
    eprosima::fastdds::dds::TypeSupport* typeSupport_ = nullptr;
    QString lastError_;
};
