#include "archive/RobotMonitorSampleReader.h"

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

#include <limits>
#include <utility>

using eprosima::fastdds::dds::DATAREADER_QOS_DEFAULT;
using eprosima::fastdds::dds::DomainParticipantFactory;
using eprosima::fastdds::dds::PARTICIPANT_QOS_DEFAULT;
using eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
using eprosima::fastdds::dds::RETCODE_OK;
using eprosima::fastdds::dds::SUBSCRIBER_QOS_DEFAULT;
using eprosima::fastdds::dds::TOPIC_QOS_DEFAULT;
using eprosima::fastdds::dds::TypeSupport;

namespace {
constexpr const char* kRobotMonitorTypeName = "AfpRobotMonitorSample";
}

class RobotMonitorSampleReader::Listener final : public eprosima::fastdds::dds::DataReaderListener {
public:
    Listener(RobotMonitorArchiveConfig config, TimeSeriesPointHandler handler)
        : config_(std::move(config)),
          handler_(std::move(handler)) {
    }

    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
        AfpRobotMonitorSample sample;
        eprosima::fastdds::dds::SampleInfo info;

        // Drain all available samples in one callback so the archive queue does
        // not lag behind when DDS delivers a burst.
        while (reader->take_next_sample(&sample, &info) == RETCODE_OK) {
            if (info.valid_data && handler_) {
                try {
                    handler_(robotMonitorSampleToPoint(sample, config_));
                } catch (...) {
                    // DDS listener callbacks must not let application
                    // exceptions escape into Fast DDS internals.
                }
            }
        }
    }

private:
    RobotMonitorArchiveConfig config_;
    TimeSeriesPointHandler handler_;
};

TimeSeriesPoint robotMonitorSampleToPoint(
    const AfpRobotMonitorSample& sample,
    const RobotMonitorArchiveConfig& config) {
    TimeSeriesPoint point;
    point.table = config.table;
    point.tags.insert("source", config.sourceTag);
    point.integerFields.insert(
        "sequence",
        sample.sample_sequence() > static_cast<std::uint64_t>(std::numeric_limits<qint64>::max())
            ? std::numeric_limits<qint64>::max()
            : static_cast<qint64>(sample.sample_sequence()));
    point.floatFields.insert("x", sample.x());
    point.floatFields.insert("y", sample.y());
    point.floatFields.insert("z", sample.z());
    point.floatFields.insert("q1", sample.q1());
    point.floatFields.insert("q2", sample.q2());
    point.floatFields.insert("q3", sample.q3());
    point.floatFields.insert("q4", sample.q4());
    point.floatFields.insert("j1", sample.j1());
    point.floatFields.insert("j2", sample.j2());
    point.floatFields.insert("j3", sample.j3());
    point.floatFields.insert("j4", sample.j4());
    point.floatFields.insert("j5", sample.j5());
    point.floatFields.insert("j6", sample.j6());
    point.floatFields.insert("eax1", sample.eax1());
    point.floatFields.insert("eax2", sample.eax2());
    point.floatFields.insert("eax3", sample.eax3());
    point.floatFields.insert("eax4", sample.eax4());
    point.floatFields.insert("eax5", sample.eax5());
    point.floatFields.insert("eax6", sample.eax6());
    point.floatFields.insert("commanded_tcp_speed", sample.commanded_tcp_speed());
    point.floatFields.insert("commanded_orientation_speed", sample.commanded_orientation_speed());
    point.floatFields.insert("commanded_external_linear_speed", sample.commanded_external_linear_speed());
    point.floatFields.insert("commanded_external_orientation_speed", sample.commanded_external_orientation_speed());
    point.floatFields.insert("estimated_tcp_speed", sample.estimated_tcp_speed());
    constexpr std::uint64_t kNsPerMs = 1000000ULL;
    point.timestampNs = sample.timestamp_ms() <= std::numeric_limits<std::uint64_t>::max() / kNsPerMs
        ? sample.timestamp_ms() * kNsPerMs
        : 0;
    return point;
}

RobotMonitorSampleReader::RobotMonitorSampleReader(
    RobotMonitorArchiveConfig config,
    TimeSeriesPointHandler handler)
    : config_(std::move(config)),
      handler_(std::move(handler)) {
}

RobotMonitorSampleReader::~RobotMonitorSampleReader() {
    stop();
}

bool RobotMonitorSampleReader::start() {
    if (reader_ != nullptr) {
        return true;
    }

    auto* factory = DomainParticipantFactory::get_instance();
    participant_ = factory->create_participant(config_.ddsDomainId, PARTICIPANT_QOS_DEFAULT);
    if (participant_ == nullptr) {
        setError(QString("Could not create Fast DDS robot monitor subscriber participant in domain %1.")
                     .arg(config_.ddsDomainId));
        return false;
    }

    // The generated PubSub type must remain alive for the whole reader lifetime.
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

    topic_ = participant_->create_topic(config_.ddsTopic.toStdString(), kRobotMonitorTypeName, TOPIC_QOS_DEFAULT);
    if (topic_ == nullptr) {
        setError(QString("Could not create Fast DDS robot monitor subscriber topic '%1'.").arg(config_.ddsTopic));
        stop();
        return false;
    }

    listener_ = new Listener(config_, handler_);
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

void RobotMonitorSampleReader::stop() {
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
}

QString RobotMonitorSampleReader::lastError() const {
    return lastError_;
}

void RobotMonitorSampleReader::setError(const QString& message) {
    lastError_ = message;
}
