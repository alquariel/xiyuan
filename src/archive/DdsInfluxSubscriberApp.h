#pragma once

#include "app/AppConfig.h"
#include "app/AppPaths.h"
#include "archive/TimeSeriesPoint.h"

#include <QObject>
#include <QTimer>

#include <memory>

class InfluxLineProtocolWriter;
class PressureSampleReader;
class RobotMonitorSampleReader;

// Coordinates the standalone DDS-to-InfluxDB subscriber process. It owns the
// archive writer, DDS reader, and periodic flush timer used by the executable.
class DdsInfluxSubscriberApp final : public QObject {
    Q_OBJECT

public:
    DdsInfluxSubscriberApp(AppConfig config, AppPaths paths, QObject* parent = nullptr);
    ~DdsInfluxSubscriberApp() override;

    // Loads the InfluxDB token, starts enabled DDS readers, and enables
    // periodic writes. Returns false with lastError() populated on setup errors.
    bool start();
    QString lastError() const;

public slots:
    // Stops DDS intake and performs one final writer flush. Safe to call from
    // aboutToQuit() and again from the destructor.
    void stop();

private:
    // Handles one normalized time-series point emitted by any DDS reader.
    void handlePoint(const TimeSeriesPoint& point);

    AppConfig config_;
    AppPaths paths_;
    QString lastError_;
    std::unique_ptr<InfluxLineProtocolWriter> writer_;
    std::unique_ptr<PressureSampleReader> pressureReader_;
    std::unique_ptr<RobotMonitorSampleReader> robotMonitorReader_;
    QTimer flushTimer_;
};
