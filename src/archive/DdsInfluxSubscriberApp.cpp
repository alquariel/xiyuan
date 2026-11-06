#include "archive/DdsInfluxSubscriberApp.h"

#include "archive/InfluxAdmin.h"
#include "archive/InfluxLineProtocolWriter.h"
#include "archive/PressureSampleReader.h"
#include "archive/RobotMonitorSampleReader.h"

#include <QByteArray>
#include <QDebug>
#include <QMetaObject>

#include <algorithm>
#include <cstdio>
#include <memory>
#include <utility>

namespace {
// Live sample printing is intentionally kept out of qInfo(). The Qt message
// handler is for diagnostic events, while time-series samples can arrive
// continuously; writing samples straight to stdout keeps the live console
// visible without also flooding dds_influx_subscriber.log.
void printLivePointToConsole(const TimeSeriesPoint& point) {
    const QByteArray line = point.toLineProtocol().toLocal8Bit();
    std::fwrite(line.constData(), 1, static_cast<std::size_t>(line.size()), stdout);
    std::fwrite("\n", 1, 1, stdout);
    std::fflush(stdout);
}
}

DdsInfluxSubscriberApp::DdsInfluxSubscriberApp(AppConfig config, AppPaths paths, QObject* parent)
    : QObject(parent),
      config_(std::move(config)),
      paths_(std::move(paths)) {
    flushTimer_.setSingleShot(false);
}

DdsInfluxSubscriberApp::~DdsInfluxSubscriberApp() {
    stop();
}

bool DdsInfluxSubscriberApp::start() {
    qInfo().noquote() << "DDS InfluxDB subscriber initializing.";
    if (!config_.timeseriesArchive.enabled) {
        lastError_ = "Timeseries archive is disabled.";
        return false;
    }
    if (!config_.pressureArchive.enabled && !config_.robotMonitorArchive.enabled) {
        lastError_ = "No DDS archive readers are enabled.";
        return false;
    }

    QString tokenError;
    const QString token = readInfluxAdminToken(config_.timeseriesArchive.adminTokenFile, &tokenError);
    if (token.isEmpty()) {
        lastError_ = tokenError.isEmpty() ? QString("InfluxDB admin token is empty.") : tokenError;
        return false;
    }

    writer_ = std::make_unique<InfluxLineProtocolWriter>(
        config_.timeseriesArchive,
        token,
        std::make_unique<QtInfluxHttpClient>());

    QObject::connect(&flushTimer_, &QTimer::timeout, this, [this]() {
        if (writer_ && !writer_->flush()) {
            qWarning().noquote() << QString("InfluxDB flush failed: %1").arg(writer_->lastError());
        }
    });
    flushTimer_.start(std::max(1, config_.timeseriesArchive.writeFlushIntervalMs));
    qInfo().noquote() << QString("DDS InfluxDB subscriber flush interval: %1 ms.")
                             .arg(std::max(1, config_.timeseriesArchive.writeFlushIntervalMs));

    const auto pointHandler = [this](const TimeSeriesPoint& point) {
        // Fast DDS invokes listener callbacks from its own thread. Queue point
        // handling onto this QObject's Qt thread so enqueue() and flushTimer_
        // cannot touch the writer concurrently.
        QMetaObject::invokeMethod(
            this,
            [this, point]() {
                handlePoint(point);
            },
            Qt::QueuedConnection);
    };

    if (config_.pressureArchive.enabled) {
        pressureReader_ = std::make_unique<PressureSampleReader>(config_.pressureArchive, pointHandler);
        if (!pressureReader_->start()) {
            flushTimer_.stop();
            lastError_ = pressureReader_->lastError();
            if (lastError_.isEmpty()) {
                lastError_ = "Could not start pressure DDS reader.";
            }
            pressureReader_.reset();
            writer_.reset();
            return false;
        }
    }

    if (config_.robotMonitorArchive.enabled) {
        robotMonitorReader_ = std::make_unique<RobotMonitorSampleReader>(config_.robotMonitorArchive, pointHandler);
        if (!robotMonitorReader_->start()) {
            flushTimer_.stop();
            lastError_ = robotMonitorReader_->lastError();
            if (lastError_.isEmpty()) {
                lastError_ = "Could not start robot monitor DDS reader.";
            }
            robotMonitorReader_.reset();
            pressureReader_.reset();
            writer_.reset();
            return false;
        }
    }

    lastError_.clear();
    qInfo().noquote() << QString("DDS InfluxDB subscriber started. pressure=%1 robot_monitor=%2")
                             .arg(config_.pressureArchive.enabled ? "enabled" : "disabled")
                             .arg(config_.robotMonitorArchive.enabled ? "enabled" : "disabled");
    return true;
}

QString DdsInfluxSubscriberApp::lastError() const {
    return lastError_;
}

void DdsInfluxSubscriberApp::stop() {
    qInfo().noquote() << "DDS InfluxDB subscriber stopping.";
    flushTimer_.stop();

    if (pressureReader_) {
        pressureReader_->stop();
        pressureReader_.reset();
    }
    if (robotMonitorReader_) {
        robotMonitorReader_->stop();
        robotMonitorReader_.reset();
    }

    if (writer_) {
        if (!writer_->flush()) {
            qWarning().noquote() << QString("Final InfluxDB flush failed: %1").arg(writer_->lastError());
        } else {
            qInfo().noquote() << "Final InfluxDB flush completed.";
        }
    }
    writer_.reset();
    qInfo().noquote() << "DDS InfluxDB subscriber stopped.";
}

void DdsInfluxSubscriberApp::handlePoint(const TimeSeriesPoint& point) {
    if (!writer_) {
        return;
    }

    if (config_.timeseriesArchive.subscriberPrintLiveData) {
        printLivePointToConsole(point);
    }

    if (!writer_->enqueue(point)) {
        qWarning().noquote() << QString("Could not enqueue InfluxDB point: %1").arg(writer_->lastError());
    }
}
