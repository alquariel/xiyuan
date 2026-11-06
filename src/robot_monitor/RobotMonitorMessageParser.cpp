#include "robot_monitor/RobotMonitorMessageParser.h"

#include <QDateTime>
#include <QMap>
#include <QStringList>

// Implements the text-frame parser used by the robot monitoring socket path.
namespace {
bool readDouble(const QMap<QString, QString>& fields, const QString& key, double& value, QString& error) {
    if (!fields.contains(key)) {
        error = QString("Missing required field '%1'.").arg(key);
        return false;
    }

    bool ok = false;
    const double parsed = fields.value(key).toDouble(&ok);
    if (!ok) {
        error = QString("Field '%1' is not a valid number.").arg(key);
        return false;
    }

    value = parsed;
    return true;
}

bool readInt(const QMap<QString, QString>& fields, const QString& key, int& value, QString& error) {
    if (!fields.contains(key)) {
        error = QString("Missing required field '%1'.").arg(key);
        return false;
    }

    bool ok = false;
    const int parsed = fields.value(key).toInt(&ok);
    if (!ok) {
        error = QString("Field '%1' is not a valid integer.").arg(key);
        return false;
    }

    value = parsed;
    return true;
}

double optionalDouble(const QMap<QString, QString>& fields, const QString& key) {
    bool ok = false;
    const double parsed = fields.value(key).toDouble(&ok);
    return ok ? parsed : 0.0;
}

double optionalDoubleAlias(const QMap<QString, QString>& fields, const QString& primaryKey, const QString& aliasKey) {
    if (fields.contains(primaryKey)) {
        return optionalDouble(fields, primaryKey);
    }
    return optionalDouble(fields, aliasKey);
}

bool readIntAlias(const QMap<QString, QString>& fields,
                  const QString& primaryKey,
                  const QString& aliasKey,
                  int& value,
                  QString& error) {
    if (fields.contains(primaryKey)) {
        return readInt(fields, primaryKey, value, error);
    }
    return readInt(fields, aliasKey, value, error);
}
}

RobotMonitorParseResult RobotMonitorMessageParser::parse(const QString& rawMessage) {
    RobotMonitorParseResult result;

    QString message = rawMessage.trimmed();
    if (message.startsWith('#')) {
        message.remove(0, 1);
    }
    if (message.endsWith('#')) {
        message.chop(1);
    }

    const QStringList parts = message.split(' ', Qt::SkipEmptyParts);
    if (parts.isEmpty() || parts.first() != "ROBOT_MONITOR") {
        result.error = "Message does not start with ROBOT_MONITOR.";
        return result;
    }

    QMap<QString, QString> fields;
    for (int i = 1; i < parts.size(); ++i) {
        const QString part = parts.at(i);
        const int equalsIndex = part.indexOf('=');
        if (equalsIndex <= 0 || equalsIndex == part.size() - 1) {
            result.error = QString("Malformed key=value field '%1'.").arg(part);
            return result;
        }

        fields.insert(part.left(equalsIndex), part.mid(equalsIndex + 1));
    }

    RobotMonitorSample sample;
    sample.receivedAtUtc = QDateTime::currentDateTimeUtc();

    QString error;
    if (!readIntAlias(fields, "seq", "s", sample.sequence, error)
        || !readDouble(fields, "x", sample.x, error)
        || !readDouble(fields, "y", sample.y, error)
        || !readDouble(fields, "z", sample.z, error)) {
        result.error = error;
        return result;
    }

    // RAPID strings are intentionally kept short in the first hardware-facing
    // monitor stream. Orientation and joint fields are therefore optional:
    // they are recorded when present and left at zero when a compact frame is
    // used only to confirm live Cartesian position and speed.
    sample.q1 = optionalDouble(fields, "q1");
    sample.q2 = optionalDouble(fields, "q2");
    sample.q3 = optionalDouble(fields, "q3");
    sample.q4 = optionalDouble(fields, "q4");

    for (int i = 0; i < 6; ++i) {
        sample.joints[i] = optionalDouble(fields, QString("j%1").arg(i + 1));
        sample.externalAxis[i] = optionalDouble(fields, QString("eax%1").arg(i + 1));
    }

    sample.commandedTcpSpeed = optionalDoubleAlias(fields, "speed_tcp", "v");
    sample.commandedOrientationSpeed = optionalDouble(fields, "speed_ori");
    sample.commandedExternalLinearSpeed = optionalDouble(fields, "speed_leax");
    sample.commandedExternalOrientationSpeed = optionalDouble(fields, "speed_reax");

    result.ok = true;
    result.sample = sample;
    return result;
}
