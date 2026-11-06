#include "archive/TimeSeriesPoint.h"

#include <QStringList>

#include <cmath>

namespace {
QString normalizeLineProtocolToken(QString value) {
    value.replace('\r', ' ');
    value.replace('\n', ' ');
    return value;
}

// Keeps floating-point output deterministic while preserving useful precision.
QString formatDouble(double value) {
    return QString::number(value, 'g', 15);
}
}

QString escapeMeasurementOrKey(QString value) {
    value = normalizeLineProtocolToken(std::move(value));
    value.replace("\\", "\\\\");
    value.replace(" ", "\\ ");
    value.replace(",", "\\,");
    value.replace("=", "\\=");
    return value;
}

QString escapeTagValue(QString value) {
    value = normalizeLineProtocolToken(std::move(value));
    value.replace("\\", "\\\\");
    value.replace(" ", "\\ ");
    value.replace(",", "\\,");
    value.replace("=", "\\=");
    return value;
}

QString escapeStringField(QString value) {
    value = normalizeLineProtocolToken(std::move(value));
    value.replace("\\", "\\\\");
    value.replace("\"", "\\\"");
    return value;
}

QString TimeSeriesPoint::toLineProtocol() const {
    QString line = escapeMeasurementOrKey(table);
    for (auto it = tags.cbegin(); it != tags.cend(); ++it) {
        line += "," + escapeMeasurementOrKey(it.key()) + "=" + escapeTagValue(it.value());
    }

    QStringList fields;
    for (auto it = floatFields.cbegin(); it != floatFields.cend(); ++it) {
        if (!std::isfinite(it.value())) {
            continue;
        }
        fields << escapeMeasurementOrKey(it.key()) + "=" + formatDouble(it.value());
    }
    for (auto it = integerFields.cbegin(); it != integerFields.cend(); ++it) {
        fields << escapeMeasurementOrKey(it.key()) + "=" + QString::number(it.value()) + "i";
    }
    for (auto it = boolFields.cbegin(); it != boolFields.cend(); ++it) {
        fields << escapeMeasurementOrKey(it.key()) + "=" + (it.value() ? "true" : "false");
    }
    for (auto it = stringFields.cbegin(); it != stringFields.cend(); ++it) {
        fields << escapeMeasurementOrKey(it.key()) + "=\"" + escapeStringField(it.value()) + "\"";
    }

    // InfluxDB line protocol requires at least one field. An empty string tells
    // the writer to skip this point instead of sending an invalid row.
    if (fields.isEmpty()) {
        return {};
    }

    line += " " + fields.join(',');
    if (timestampNs > 0) {
        line += " " + QString::number(static_cast<qulonglong>(timestampNs));
    }
    return line;
}
