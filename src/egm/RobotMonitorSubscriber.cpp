#include "egm/RobotMonitorSubscriber.h"

#include "robot_monitor/generated/RobotMonitorSample.hpp"
#include "robot_monitor/generated/RobotMonitorSamplePubSubTypes.hpp"

#include <fastdds/dds/core/ReturnCode.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include <QMutexLocker>
#include <utility>

using eprosima::fastdds::dds::DATAREADER_QOS_DEFAULT;
using eprosima::fastdds::dds::DomainParticipantFactory;
using eprosima::fastdds::dds::PARTICIPANT_QOS_DEFAULT;
using eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
using eprosima::fastdds::dds::RETCODE_OK;
using eprosima::fastdds::dds::SUBSCRIBER_QOS_DEFAULT;
using eprosima::fastdds::dds::TOPIC_QOS_DEFAULT;
using eprosima::fastdds::dds::TypeSupport;

namespace egm {
namespace {
constexpr const char* kRobotMonitorTypeName = "AfpRobotMonitorSample";
}

class RobotMonitorSubscriber::Listener final : public eprosima::fastdds::dds::DataReaderListener {
public:
    explicit Listener(RobotMonitorSubscriber* owner) : owner_(owner) {}

    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
        AfpRobotMonitorSample sample;
        eprosima::fastdds::dds::SampleInfo info;

        while (reader->take_next_sample(&sample, &info) == RETCODE_OK) {
            if (!info.valid_data || owner_ == nullptr) {
                continue;
            }
            RobotPoseSnapshot snapshot;
            snapshot.valid = true;
            snapshot.x = sample.x();
            snapshot.y = sample.y();
            snapshot.z = sample.z();
            snapshot.q1 = sample.q1();
            snapshot.q2 = sample.q2();
            snapshot.q3 = sample.q3();
            snapshot.q4 = sample.q4();
            snapshot.receivedAt = std::chrono::steady_clock::now();

            QMutexLocker locker(&owner_->mutex_);
            owner_->snapshot_ = snapshot;
        }
    }

private:
    RobotMonitorSubscriber* owner_ = nullptr;
};

RobotMonitorSubscriber::RobotMonitorSubscriber(int ddsDomainId, QString topicName)
    : ddsDomainId_(ddsDomainId), topicName_(std::move(topicName)) {
}

RobotMonitorSubscriber::~RobotMonitorSubscriber() {
    stop();
}

bool RobotMonitorSubscriber::start() {
    if (reader_ != nullptr) {
        return true;
    }

    auto* factory = DomainParticipantFactory::get_instance();
    participant_ = factory->create_participant(ddsDomainId_, PARTICIPANT_QOS_DEFAULT);
    if (participant_ == nullptr) {
        setError(QString("Could not create Fast DDS robot monitor subscriber participant in domain %1.")
                     .arg(ddsDomainId_));
        return false;
    }

    typeSupport_ = new TypeSupport(new AfpRobotMonitorSamplePubSubType());
    if (typeSupport_->register_type(participant_, kRobotMonitorTypeName) != RETCODE_OK) {
        setError("Could not register AfpRobotMonitorSample with Fast DDS robot monitor subscriber.");
        stop();
        return false;
    }

    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
    if (subscriber_ == nullptr) {
        setError("Could not create Fast DDS robot monitor subscriber.");
        stop();
        return false;
    }

    topic_ = participant_->create_topic(topicName_.toStdString(), kRobotMonitorTypeName, TOPIC_QOS_DEFAULT);
    if (topic_ == nullptr) {
        setError(QString("Could not create Fast DDS robot monitor subscriber topic '%1'.").arg(topicName_));
        stop();
        return false;
    }

    listener_ = new Listener(this);
    auto readerQos = DATAREADER_QOS_DEFAULT;
    readerQos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    reader_ = subscriber_->create_datareader(topic_, readerQos, listener_);
    if (reader_ == nullptr) {
        setError("Could not create Fast DDS robot monitor data reader.");
        stop();
        return false;
    }

    lastError_.clear();
    return true;
}

void RobotMonitorSubscriber::stop() {
    if (participant_ != nullptr) {
        if (subscriber_ != nullptr && reader_ != nullptr) {
            subscriber_->delete_datareader(reader_);
            reader_ = nullptr;
        }
        if (subscriber_ != nullptr) {
            participant_->delete_subscriber(subscriber_);
            subscriber_ = nullptr;
        }
        if (topic_ != nullptr) {
            participant_->delete_topic(topic_);
            topic_ = nullptr;
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
        participant_ = nullptr;
    }

    delete listener_;
    listener_ = nullptr;

    delete typeSupport_;
    typeSupport_ = nullptr;

    QMutexLocker locker(&mutex_);
    snapshot_ = RobotPoseSnapshot{};
}

RobotPoseSnapshot RobotMonitorSubscriber::snapshot() const {
    QMutexLocker locker(&mutex_);
    return snapshot_;
}

QString RobotMonitorSubscriber::lastError() const {
    return lastError_;
}

void RobotMonitorSubscriber::setError(const QString& message) {
    lastError_ = message;
}

}  // namespace egm
