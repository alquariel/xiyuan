#pragma once

#include <QFile>
#include <QString>

#include "robot_monitor/RobotMonitorSample.h"

// Writes robot monitor samples to plain files for first-stage hardware
// validation. This deliberately avoids DDS/InfluxDB so socket correctness can
// be confirmed with easy-to-inspect CSV and exact raw socket message logs.
class RobotMonitorRecorder final {
public:
    RobotMonitorRecorder(QString recordingDir,
                         bool recordCsv,
                         bool recordRawMessages,
                         bool flushEverySample);

    bool start();
    bool recordSample(const RobotMonitorSample& sample);
    bool recordRawMessage(const QString& rawMessage);
    void close();

    QString csvFilePath() const;
    QString rawLogFilePath() const;
    QString lastError() const;

private:
    bool writeCsvHeader();
    QString makeBasePath() const;

    QString recordingDir_;
    bool recordCsv_ = true;
    bool recordRawMessages_ = true;
    bool flushEverySample_ = true;

    QFile csvFile_;
    QFile rawFile_;
    QString lastError_;
};
