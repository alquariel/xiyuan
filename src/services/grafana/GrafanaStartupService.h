#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>

#include "app/AppConfig.h"
#include "app/AppPaths.h"

// Starts the optional local Grafana OSS server. This service is deliberately
// independent from TimeseriesArchiveService: Grafana is a dashboard process,
// not part of the InfluxDB/DDS archive pipeline.
class GrafanaStartupService final : public QObject {
    Q_OBJECT

public:
    GrafanaStartupService(GrafanaConfig config, AppPaths paths, QObject* parent = nullptr);

    // Queues Grafana startup and returns immediately so the main GUI can show
    // without waiting for Grafana. Returns false only when Grafana is disabled.
    bool start();
    QString lastStatus() const;

signals:
    void statusChanged(const QString& status);

private:
    void beginStartup();
    void checkHostReachable();
    bool isConfiguredProcessRunning() const;
    QString resolveExecutablePath(const QString& configuredPath) const;
    QString resolveWorkingDirectory() const;
    void setStatus(const QString& status);

    GrafanaConfig config_;
    AppPaths paths_;
    QNetworkAccessManager network_;
    QString lastStatus_;
    bool startupQueued_ = false;
};
