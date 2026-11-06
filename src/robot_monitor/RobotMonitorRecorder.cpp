#include "robot_monitor/RobotMonitorRecorder.h"

#include <utility>

#include <QDateTime>
#include <QDir>
#include <QLocale>
#include <QTextStream>

// File writer for the robot monitor validation path. CSV gives engineers a
// typed sample history, while the raw log preserves the socket frames that
// arrived before parsing so RAPID/C++ boundary issues are easy to diagnose.

RobotMonitorRecorder::RobotMonitorRecorder(QString recordingDir,
                                           bool recordCsv,
                                           bool recordRawMessages,
                                           bool flushEverySample)
    : recordingDir_(std::move(recordingDir)),
      recordCsv_(recordCsv),
      recordRawMessages_(recordRawMessages),
      flushEverySample_(flushEverySample) {
}

bool RobotMonitorRecorder::start() {
    QDir dir(recordingDir_);
    if (!dir.exists() && !dir.mkpath(".")) {
        lastError_ = QString("Could not create robot monitor recording directory: %1").arg(recordingDir_);
        return false;
    }

    const QString base = makeBasePath();
    if (recordCsv_) {
        csvFile_.setFileName(base + ".csv");
        if (!csvFile_.open(QIODevice::WriteOnly | QIODevice::Text)) {
            lastError_ = QString("Could not open robot monitor CSV file: %1").arg(csvFile_.errorString());
            return false;
        }
        if (!writeCsvHeader()) {
            return false;
        }
    }

    if (recordRawMessages_) {
        rawFile_.setFileName(base + ".raw.log");
        if (!rawFile_.open(QIODevice::WriteOnly | QIODevice::Text)) {
            lastError_ = QString("Could not open robot monitor raw log file: %1").arg(rawFile_.errorString());
            return false;
        }
    }

    return true;
}

bool RobotMonitorRecorder::recordSample(const RobotMonitorSample& sample) {
    if (!recordCsv_ || !csvFile_.isOpen()) {
        return true;
    }

    const QLocale locale = QLocale::c();
    QTextStream out(&csvFile_);
    out << sample.receivedAtUtc.toUTC().toString(Qt::ISODateWithMs) << ','
        << sample.sequence << ','
        << locale.toString(sample.x, 'g', 15) << ','
        << locale.toString(sample.y, 'g', 15) << ','
        << locale.toString(sample.z, 'g', 15) << ','
        << locale.toString(sample.q1, 'g', 15) << ','
        << locale.toString(sample.q2, 'g', 15) << ','
        << locale.toString(sample.q3, 'g', 15) << ','
        << locale.toString(sample.q4, 'g', 15);

    for (double joint : sample.joints) {
        out << ',' << locale.toString(joint, 'g', 15);
    }
    for (double axis : sample.externalAxis) {
        out << ',' << locale.toString(axis, 'g', 15);
    }

    out << ',' << locale.toString(sample.commandedTcpSpeed, 'g', 15)
        << ',' << locale.toString(sample.commandedOrientationSpeed, 'g', 15)
        << ',' << locale.toString(sample.commandedExternalLinearSpeed, 'g', 15)
        << ',' << locale.toString(sample.commandedExternalOrientationSpeed, 'g', 15)
        << ',' << locale.toString(sample.estimatedTcpSpeed, 'g', 15)
        << '\n';

    if (flushEverySample_) {
        out.flush();
        csvFile_.flush();
    }
    return true;
}

bool RobotMonitorRecorder::recordRawMessage(const QString& rawMessage) {
    if (!recordRawMessages_ || !rawFile_.isOpen()) {
        return true;
    }

    QTextStream out(&rawFile_);
    out << rawMessage << '\n';
    if (flushEverySample_) {
        out.flush();
        rawFile_.flush();
    }
    return true;
}

void RobotMonitorRecorder::close() {
    if (csvFile_.isOpen()) {
        csvFile_.close();
    }
    if (rawFile_.isOpen()) {
        rawFile_.close();
    }
}

QString RobotMonitorRecorder::csvFilePath() const {
    return csvFile_.fileName();
}

QString RobotMonitorRecorder::rawLogFilePath() const {
    return rawFile_.fileName();
}

QString RobotMonitorRecorder::lastError() const {
    return lastError_;
}

bool RobotMonitorRecorder::writeCsvHeader() {
    QTextStream out(&csvFile_);
    out << "received_at_utc,seq,x,y,z,q1,q2,q3,q4,"
        << "j1,j2,j3,j4,j5,j6,"
        << "eax1,eax2,eax3,eax4,eax5,eax6,"
        << "speed_tcp,speed_ori,speed_leax,speed_reax,estimated_tcp_speed\n";
    if (flushEverySample_) {
        out.flush();
        csvFile_.flush();
    }
    return true;
}

QString RobotMonitorRecorder::makeBasePath() const {
    const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    return QDir(recordingDir_).filePath("robot_monitor_" + stamp);
}
