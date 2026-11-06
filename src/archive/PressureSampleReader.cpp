#include "archive/PressureSampleReader.h"

#include "pressure/generated/PressureSamplePubSubTypes.hpp"

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
constexpr const char* kPressureTypeName = "AfpPressureSample";
}

class PressureSampleReader::Listener final : public eprosima::fastdds::dds::DataReaderListener {
public:
    Listener(PressureArchiveConfig config, TimeSeriesPointHandler handler)
        : config_(std::move(config)),
          handler_(std::move(handler)) {
    }

    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
        AfpPressureSample sample;
        eprosima::fastdds::dds::SampleInfo info;

        // Drain all available samples in one callback so the archive queue does
        // not lag behind when DDS delivers a burst.
        while (reader->take_next_sample(&sample, &info) == RETCODE_OK) {
            if (info.valid_data && handler_) {
                try {
                    handler_(pressureSampleToPoint(sample, config_));
                } catch (...) {
                    // DDS listener callbacks must not let application
                    // exceptions escape into Fast DDS internals.
                }
            }
        }
    }

private:
    PressureArchiveConfig config_;
    TimeSeriesPointHandler handler_;
};

TimeSeriesPoint pressureSampleToPoint(const AfpPressureSample& sample, const PressureArchiveConfig& config) {
    TimeSeriesPoint point;
    point.table = config.table;
    point.tags.insert("source", config.sourceTag);
    point.floatFields.insert("raw_value", sample.raw_value());
    point.floatFields.insert("mean_value", sample.mean_value());
    point.floatFields.insert("variance_value", sample.variance_value());
    point.floatFields.insert("standard_deviation_value", sample.standard_deviation_value());
    point.floatFields.insert("peak_value", sample.peak_value());
    point.integerFields.insert(
        "sequence",
        sample.sample_sequence() > static_cast<std::uint64_t>(std::numeric_limits<qint64>::max())
            ? std::numeric_limits<qint64>::max()
            : static_cast<qint64>(sample.sample_sequence()));
    point.boolFields.insert("window_ready", sample.window_ready());
    point.boolFields.insert("sensor_running", sample.sensor_running());
    // A zero timestamp asks InfluxDB to apply server time. Use the same
    // fallback if a malformed DDS timestamp would overflow nanoseconds.
    constexpr std::uint64_t kNsPerMs = 1000000ULL;
    point.timestampNs = sample.timestamp_ms() <= std::numeric_limits<std::uint64_t>::max() / kNsPerMs
        ? sample.timestamp_ms() * kNsPerMs
        : 0;
    return point;
}

PressureSampleReader::PressureSampleReader(PressureArchiveConfig config, TimeSeriesPointHandler handler)
    : config_(std::move(config)),
      handler_(std::move(handler)) {
}

PressureSampleReader::~PressureSampleReader() {
    stop();
}

bool PressureSampleReader::start() {
    if (reader_ != nullptr) {
        return true;
    }

    auto* factory = DomainParticipantFactory::get_instance();
    participant_ = factory->create_participant(config_.ddsDomainId, PARTICIPANT_QOS_DEFAULT);
    if (participant_ == nullptr) {
        setError(QString("Could not create Fast DDS pressure subscriber participant in domain %1.")
                     .arg(config_.ddsDomainId));
        return false;
    }

    // The generated PubSub type must remain alive for the whole reader lifetime.
    typeSupport_ = new TypeSupport(new AfpPressureSamplePubSubType());
    if (typeSupport_->register_type(participant_, kPressureTypeName) != RETCODE_OK) {
        setError("Could not register AfpPressureSample with Fast DDS pressure subscriber.");
        stop();
        return false;
    }

    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
    if (subscriber_ == nullptr) {
        setError("Could not create Fast DDS pressure subscriber.");
        stop();
        return false;
    }

    topic_ = participant_->create_topic(config_.ddsTopic.toStdString(), kPressureTypeName, TOPIC_QOS_DEFAULT);
    if (topic_ == nullptr) {
        setError(QString("Could not create Fast DDS pressure subscriber topic '%1'.").arg(config_.ddsTopic));
        stop();
        return false;
    }

    listener_ = new Listener(config_, handler_);
    auto readerQos = DATAREADER_QOS_DEFAULT;
    readerQos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    reader_ = subscriber_->create_datareader(topic_, readerQos, listener_);
    if (reader_ == nullptr) {
        setError("Could not create Fast DDS pressure data reader.");
        stop();
        return false;
    }

    lastError_.clear();
    return true;
}

void PressureSampleReader::stop() {
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

QString PressureSampleReader::lastError() const {
    return lastError_;
}

void PressureSampleReader::setError(const QString& message) {
    lastError_ = message;
}
