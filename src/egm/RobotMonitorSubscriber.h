#pragma once

#include <QString>
#include <QMutex>

#include "egm/EgmSnapshots.h"

namespace eprosima::fastdds::dds {
class DataReader;
class DomainParticipant;
class Subscriber;
class Topic;
class TypeSupport;
}  // namespace eprosima::fastdds::dds

namespace egm {

// 订阅 afp.robot.monitor.sample，只在回调中写入最新位姿快照。
class RobotMonitorSubscriber final {
public:
    RobotMonitorSubscriber(int ddsDomainId, QString topicName);
    ~RobotMonitorSubscriber();

    RobotMonitorSubscriber(const RobotMonitorSubscriber&) = delete;
    RobotMonitorSubscriber& operator=(const RobotMonitorSubscriber&) = delete;

    bool start();
    void stop();

    RobotPoseSnapshot snapshot() const;
    QString lastError() const;

private:
    class Listener;

    void setError(const QString& message);

    int ddsDomainId_ = 0;
    QString topicName_;
    mutable QMutex mutex_;
    RobotPoseSnapshot snapshot_;

    eprosima::fastdds::dds::DomainParticipant* participant_ = nullptr;
    eprosima::fastdds::dds::Subscriber* subscriber_ = nullptr;
    eprosima::fastdds::dds::Topic* topic_ = nullptr;
    eprosima::fastdds::dds::DataReader* reader_ = nullptr;
    eprosima::fastdds::dds::TypeSupport* typeSupport_ = nullptr;
    Listener* listener_ = nullptr;
    QString lastError_;
};

}  // namespace egm
