#pragma once

#include <QString>

#include "app/AppConfig.h"
#include "pressure/PressureDdsTypes.h"

namespace eprosima::fastdds::dds {
class DataWriter;
class DomainParticipant;
class Publisher;
class Topic;
class TypeSupport;
}

// Owns the Fast DDS objects required to publish AfpPressureSample data. The
// rest of the pressure code talks to this class instead of directly managing
// participants, topics, writers, and generated type support.
class PressureDdsPublisher final {
public:
    explicit PressureDdsPublisher(PressureConfig config);
    ~PressureDdsPublisher();

    bool initialize();
    bool publish(const PressureSampleData& sample);
    void shutdown();

    bool initialized() const;
    QString topicName() const;
    QString lastError() const;

private:
    void setError(const QString& message);

    PressureConfig config_;
    eprosima::fastdds::dds::DomainParticipant* participant_ = nullptr;
    eprosima::fastdds::dds::Publisher* publisher_ = nullptr;
    eprosima::fastdds::dds::Topic* topic_ = nullptr;
    eprosima::fastdds::dds::DataWriter* writer_ = nullptr;
    eprosima::fastdds::dds::TypeSupport* typeSupport_ = nullptr;
    QString lastError_;
};
