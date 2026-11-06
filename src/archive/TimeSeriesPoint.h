#pragma once

#include <QMap>
#include <QString>
#include <QtGlobal>

#include <cstdint>
#include <functional>

// Backend-neutral time-series point. DDS readers produce this model, and the
// InfluxDB writer consumes it when converting samples to line protocol.
struct TimeSeriesPoint {
    QString table;
    QMap<QString, QString> tags;
    QMap<QString, double> floatFields;
    QMap<QString, qint64> integerFields;
    QMap<QString, bool> boolFields;
    QMap<QString, QString> stringFields;
    std::uint64_t timestampNs = 0;

    // Formats this point as one InfluxDB line-protocol record.
    QString toLineProtocol() const;
};

// Callback used by DDS readers to hand neutral time-series points to the
// archive writer. Pressure, robot monitor, and future readers share this shape.
using TimeSeriesPointHandler = std::function<void(const TimeSeriesPoint&)>;

// Escapes measurement names and field/tag keys for InfluxDB line protocol.
QString escapeMeasurementOrKey(QString value);

// Escapes tag values for InfluxDB line protocol.
QString escapeTagValue(QString value);

// Escapes string field values before surrounding them with quotes.
QString escapeStringField(QString value);
