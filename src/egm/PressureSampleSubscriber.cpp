#include "egm/PressureSampleSubscriber.h"

#include "pressure/generated/PressureSample.hpp"
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
constexpr const char* kPressureTypeName = "AfpPressureSample";
}

//定义监听器类
class PressureSampleSubscriber::Listener final : public eprosima::fastdds::dds::DataReaderListener {
public:
    explicit Listener(PressureSampleSubscriber* owner) : owner_(owner) {}

    // 数据可用回调
    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override {
        AfpPressureSample sample;
        eprosima::fastdds::dds::SampleInfo info;

        // 一次回调排空所有可用样本；只保留最后一个，控制回路使用最新值。
        while (reader->take_next_sample(&sample, &info) == RETCODE_OK) {
            if (!info.valid_data || owner_ == nullptr) {
                continue;
            }
            // 处理样本数据，更新快照
            PressureSnapshot snapshot;
            snapshot.valid = true;
            snapshot.windowReady = sample.window_ready();
            snapshot.sensorRunning = sample.sensor_running();
            snapshot.rawValue = sample.raw_value();
            snapshot.meanValue = sample.mean_value();
            snapshot.standardDeviationValue = sample.standard_deviation_value();
            snapshot.receivedAt = std::chrono::steady_clock::now();

            QMutexLocker locker(&owner_->mutex_);
            owner_->snapshot_ = snapshot;
        }
    }

private:
    PressureSampleSubscriber* owner_ = nullptr;
};

PressureSampleSubscriber::PressureSampleSubscriber(int ddsDomainId, QString topicName)
    : ddsDomainId_(ddsDomainId), topicName_(std::move(topicName)) {
}

PressureSampleSubscriber::~PressureSampleSubscriber() {
    stop();
}

bool PressureSampleSubscriber::start() {
    if (reader_ != nullptr) {
        return true;
    }

    auto* factory = DomainParticipantFactory::get_instance();
    participant_ = factory->create_participant(ddsDomainId_, PARTICIPANT_QOS_DEFAULT);
    if (participant_ == nullptr) {
        setError(QString("Could not create Fast DDS pressure subscriber participant in domain %1.")
                     .arg(ddsDomainId_));
        return false;
    }

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

    topic_ = participant_->create_topic(topicName_.toStdString(), kPressureTypeName, TOPIC_QOS_DEFAULT);
    if (topic_ == nullptr) {
        setError(QString("Could not create Fast DDS pressure subscriber topic '%1'.").arg(topicName_));
        stop();
        return false;
    }

    listener_ = new Listener(this);
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

void PressureSampleSubscriber::stop() {
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
    snapshot_ = PressureSnapshot{};
}

PressureSnapshot PressureSampleSubscriber::snapshot() const {
    QMutexLocker locker(&mutex_);
    return snapshot_;
}

QString PressureSampleSubscriber::lastError() const {
    return lastError_;
}

void PressureSampleSubscriber::setError(const QString& message) {
    lastError_ = message;
}

}  // namespace egm
