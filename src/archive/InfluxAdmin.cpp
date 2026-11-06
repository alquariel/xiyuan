#include "archive/InfluxAdmin.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

QString readInfluxAdminToken(const QString& tokenFilePath, QString* error) {
    QFile file(tokenFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error != nullptr) {
            *error = QString("Could not open InfluxDB admin token file: %1").arg(tokenFilePath);
        }
        return {};
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error != nullptr) {
            *error = QString("Could not parse InfluxDB admin token JSON: %1").arg(parseError.errorString());
        }
        return {};
    }

    const QString token = document.object().value("token").toString().trimmed();
    if (token.isEmpty()) {
        if (error != nullptr) {
            *error = "InfluxDB admin token file is missing a non-empty 'token' field.";
        }
        return {};
    }

    if (error != nullptr) {
        error->clear();
    }
    return token;
}

QStringList buildInfluxServeArguments(const TimeseriesArchiveConfig& config) {
    // Keep process argument construction centralized so paths with spaces stay as single argv entries.
    return {"serve",
            "--node-id", config.nodeId,
            "--object-store", config.objectStore,
            "--data-dir", config.dataDir,
            "--admin-token-file", config.adminTokenFile};
}

QStringList buildInfluxShowDatabasesArguments() {
    return {"show", "databases", "--format", "json"};
}

QStringList buildInfluxCreateDatabaseArguments(const TimeseriesArchiveConfig& config) {
    return {"create", "database", "--retention-period", config.retentionPeriod, config.database};
}

QProcessEnvironment buildInfluxCliEnvironment(const TimeseriesArchiveConfig& config, const QString& token) {
    // Start from the inherited environment so the CLI can still find PATH, TLS, and OS-specific settings.
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("INFLUXDB3_HOST_URL", config.hostUrl);
    env.insert("INFLUXDB3_AUTH_TOKEN", token);
    return env;
}
