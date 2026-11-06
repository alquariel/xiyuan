#include "pressure/PressureAcquisitionWorker.h"

#include <QDateTime>
// TEMP PRESSURE RAW DIAGNOSTIC: delete this include after removing the raw catchdata() logging block.
#include <QDebug>

#include <utility>

// TEMP PRESSURE RAW DIAGNOSTIC: BEGIN removable raw catchdata() logging settings.
namespace {
// TEMP PRESSURE RAW DIAGNOSTIC: set this false, or delete this block, after the zero-reading investigation.
constexpr bool kEnableRawReadDiagnostic = true;
// TEMP PRESSURE RAW DIAGNOSTIC: limits log volume to the first 80 samples, about 8 seconds at 100 ms/sample.
constexpr int kRawReadDiagnosticSampleLimit = 80;
}
// TEMP PRESSURE RAW DIAGNOSTIC: END removable raw catchdata() logging settings.

PressureAcquisitionWorker::PressureAcquisitionWorker(QString dllPath, PressureConfig config, QObject* parent)
    : QObject(parent),
      dllPath_(std::move(dllPath)),
      config_(std::move(config)),
      sensor_(dllPath_),
      statistics_(config_.processingWindowSamples),
      publisher_(config_) {
}

void PressureAcquisitionWorker::start() {
    if (running_) {
        return;
    }

    if (!publisher_.initialize()) {
        emit errorOccurred(publisher_.lastError());
        emit stopped();
        return;
    }

    if (!sensor_.start()) {
        emit errorOccurred(sensor_.lastError());
        emit stopped();
        return;
    }

    statistics_.clear();
    sequence_ = 0;
    // TEMP PRESSURE RAW DIAGNOSTIC: reset the removable raw-read log limiter when acquisition starts.
    diagnosticRawReadLogs_ = 0;
    running_ = true;

    // TEMP PRESSURE RAW DIAGNOSTIC: announce the temporary producer-side raw-value probe in afp_abb_cpp.log.
    if (kEnableRawReadDiagnostic) {
        // TEMP PRESSURE RAW DIAGNOSTIC: include the sample limit and DLL path to confirm what is being tested.
        qInfo().noquote() << QString("[pressure-raw-diagnostic] enabled; logging first %1 raw catchdata() values from %2")
                                 .arg(kRawReadDiagnosticSampleLimit)
                                 .arg(dllPath_);
    }

    timer_ = new QTimer(this);
    timer_->setTimerType(Qt::PreciseTimer);
    timer_->setInterval(config_.sampleIntervalMs);
    connect(timer_, &QTimer::timeout, this, &PressureAcquisitionWorker::acquireOneSample);
    timer_->start();

    emit started();
}

void PressureAcquisitionWorker::stop() {
    if (timer_) {
        timer_->stop();
        timer_->deleteLater();
        timer_ = nullptr;
    }

    if (running_) {
        sensor_.stop();
    }

    running_ = false;
    emit stopped();
}

void PressureAcquisitionWorker::acquireOneSample() {
    double raw = 0.0;
    if (!sensor_.read(raw)) {
        emit errorOccurred(sensor_.lastError());
        stop();
        return;
    }

    // TEMP PRESSURE RAW DIAGNOSTIC: log the raw value immediately after pressure.dll::catchdata() returns.
    if (kEnableRawReadDiagnostic && diagnosticRawReadLogs_ < kRawReadDiagnosticSampleLimit) {
        // TEMP PRESSURE RAW DIAGNOSTIC: use sequence_ before it is incremented so the log matches the DDS sample.
        qInfo().noquote() << QString("[pressure-raw-diagnostic] sequence=%1 raw_value=%2")
                                 .arg(sequence_)
                                 .arg(raw, 0, 'g', 17);
        // TEMP PRESSURE RAW DIAGNOSTIC: advance only the diagnostic limiter, not the DDS sequence counter.
        ++diagnosticRawReadLogs_;
    }

    statistics_.addSample(raw);

    PressureSampleData sample;
    sample.timestampMs = static_cast<std::uint64_t>(QDateTime::currentMSecsSinceEpoch());
    sample.sequence = sequence_++;
    sample.rawValue = raw;
    sample.windowReady = statistics_.windowReady();
    sample.sensorRunning = running_;

    // Match pressure_monitor.py: processed values appear only after the
    // configured rolling window is full. Before that they remain zero.
    if (sample.windowReady) {
        sample.meanValue = statistics_.mean();
        sample.varianceValue = statistics_.variance();
        sample.standardDeviationValue = statistics_.standardDeviation();
        sample.peakValue = statistics_.peak();
    }

    if (!publisher_.publish(sample)) {
        emit errorOccurred(publisher_.lastError());
        stop();
        return;
    }

    emit samplePublished(sample);
}
