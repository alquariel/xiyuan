#include "robot_monitor/RobotMonitorDdsPublisher.h"

#include "robot_monitor/generated/RobotMonitorSamplePubSubTypes.hpp"

#include <fastdds/dds/core/ReturnCode.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include <utility>

using eprosima::fastdds::dds::DATAWRITER_QOS_DEFAULT;
using eprosima::fastdds::dds::DomainParticipantFactory;
using eprosima::fastdds::dds::PARTICIPANT_QOS_DEFAULT;
using eprosima::fastdds::dds::PUBLISHER_QOS_DEFAULT;
using eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
using eprosima::fastdds::dds::RETCODE_OK;
using eprosima::fastdds::dds::TOPIC_QOS_DEFAULT;
using eprosima::fastdds::dds::TypeSupport;

namespace {
constexpr const char* kRobotMonitorTypeName = "AfpRobotMonitorSample";
}

RobotMonitorDdsPublisher::RobotMonitorDdsPublisher(RobotMonitorConfig config)
    : config_(std::move(config)) {
}

RobotMonitorDdsPublisher::~RobotMonitorDdsPublisher() {
    shutdown();
}

bool RobotMonitorDdsPublisher::initialize() {
    if (initialized()) {
        return true;
    }

    auto* factory = DomainParticipantFactory::get_instance();
    participant_ = factory->create_participant(config_.ddsDomainId, PARTICIPANT_QOS_DEFAULT);
    if (!participant_) {
        setError(QString("Could not create Fast DDS participant in domain %1.").arg(config_.ddsDomainId));
        return false;
    }

    // TypeSupport owns the generated PubSub type object and must outlive the
    // Fast DDS topic and writer that use its serializer.
    typeSupport_ = new TypeSupport(new AfpRobotMonitorSamplePubSubType());
    if (typeSupport_->register_type(participant_, kRobotMonitorTypeName) != RETCODE_OK) {
        setError("Could not register AfpRobotMonitorSample with Fast DDS.");
        shutdown();
        return false;
    }

    publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);
    if (!publisher_) {
        setError("Could not create Fast DDS robot monitor publisher.");
        shutdown();
        return false;
    }

    topic_ = participant_->create_topic(config_.ddsTopic.toStdString(), kRobotMonitorTypeName, TOPIC_QOS_DEFAULT);
    if (!topic_) {
        setError(QString("Could not create Fast DDS robot monitor topic '%1'.").arg(config_.ddsTopic));
        shutdown();
        return false;
    }

    auto writerQos = DATAWRITER_QOS_DEFAULT;
    writerQos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    writer_ = publisher_->create_datawriter(topic_, writerQos, nullptr);
    if (!writer_) {
        setError("Could not create Fast DDS robot monitor data writer.");
        shutdown();
        return false;
    }

    return true;
}

bool RobotMonitorDdsPublisher::publish(const RobotMonitorSample& sample) {
    if (!initialize()) {
        return false;
    }

    AfpRobotMonitorSample ddsSample = toDdsSample(sample);
    if (writer_->write(&ddsSample) != RETCODE_OK) {
        setError("Fast DDS failed to publish a robot monitor sample.");
        return false;
    }

    return true;
}

void RobotMonitorDdsPublisher::shutdown() {
    if (participant_) {
        if (publisher_ && writer_) {
            publisher_->delete_datawriter(writer_);
            writer_ = nullptr;
        }
        if (publisher_) {
            participant_->delete_publisher(publisher_);
            publisher_ = nullptr;
        }
        if (topic_) {
            participant_->delete_topic(topic_);
            topic_ = nullptr;
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
        participant_ = nullptr;
    }

    delete typeSupport_;
    typeSupport_ = nullptr;
}

bool RobotMonitorDdsPublisher::initialized() const {
    return participant_ && publisher_ && topic_ && writer_;
}

QString RobotMonitorDdsPublisher::topicName() const {
    return config_.ddsTopic;
}

QString RobotMonitorDdsPublisher::lastError() const {
    return lastError_;
}

void RobotMonitorDdsPublisher::setError(const QString& message) {
    lastError_ = message;
}
