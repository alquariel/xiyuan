#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#include <cstdint>

#include "app/AppConfig.h"
#include "pressure/PressureDdsPublisher.h"
#include "pressure/PressureSensorClient.h"
#include "pressure/PressureStatistics.h"

// Runs pressure acquisition in a worker thread. It mirrors the Python reader's
// 100 ms polling loop, computes the same rolling statistics, and publishes a
// structured DDS sample for every raw reading.
class PressureAcquisitionWorker final : public QObject {
    Q_OBJECT

public:
    PressureAcquisitionWorker(QString dllPath, PressureConfig config, QObject* parent = nullptr);

public slots:
    void start();
    void stop();

signals:
    void started();
    void stopped();
    void errorOccurred(const QString& message);
    void samplePublished(const PressureSampleData& sample);

private slots:
    void acquireOneSample();

private:
    QString dllPath_;
    PressureConfig config_;
    PressureSensorClient sensor_;
    PressureStatistics statistics_;
    PressureDdsPublisher publisher_;
    QTimer* timer_ = nullptr;
    std::uint64_t sequence_ = 0;
    // TEMP PRESSURE RAW DIAGNOSTIC: BEGIN removable raw catchdata() logging state.
    // TEMP PRESSURE RAW DIAGNOSTIC: delete the next counter after the zero-reading investigation is complete.
    int diagnosticRawReadLogs_ = 0;
    // TEMP PRESSURE RAW DIAGNOSTIC: END removable raw catchdata() logging state.
    bool running_ = false;
};
