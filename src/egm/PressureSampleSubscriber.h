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

// 订阅 afp.pressure.sample，只在回调中写入最新快照。
// 与控制律、UI 完全解耦：控制线程通过 snapshot() 读取原子化副本。
class PressureSampleSubscriber final {
public:
    PressureSampleSubscriber(int ddsDomainId, QString topicName);
    ~PressureSampleSubscriber();

    PressureSampleSubscriber(const PressureSampleSubscriber&) = delete;
    PressureSampleSubscriber& operator=(const PressureSampleSubscriber&) = delete;

    bool start();
    void stop();

    PressureSnapshot snapshot() const;
    QString lastError() const;

private:
    class Listener;

    void setError(const QString& message);

    int ddsDomainId_ = 0;
    QString topicName_;
    mutable QMutex mutex_;
    PressureSnapshot snapshot_;

    eprosima::fastdds::dds::DomainParticipant* participant_ = nullptr;
    eprosima::fastdds::dds::Subscriber* subscriber_ = nullptr;
    eprosima::fastdds::dds::Topic* topic_ = nullptr;
    eprosima::fastdds::dds::DataReader* reader_ = nullptr;
    eprosima::fastdds::dds::TypeSupport* typeSupport_ = nullptr;
    Listener* listener_ = nullptr;
    QString lastError_;
};

}  // namespace egm
