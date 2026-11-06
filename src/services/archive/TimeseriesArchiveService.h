#pragma once

#include <QObject>
#include <QProcess>
#include <QNetworkAccessManager>
#include <QElapsedTimer>
#include <QString>

#include <functional>
#include <memory>

#include "app/AppConfig.h"
#include "app/AppPaths.h"

// Owns startup coordination for the optional InfluxDB pressure archive path.
// The main UI remains responsible for operator workflows; this service only
// starts supporting processes and reports concise status text.
class TimeseriesArchiveService final : public QObject {
    Q_OBJECT

public:
    TimeseriesArchiveService(AppConfig config, AppPaths paths, QObject* parent = nullptr);
    ~TimeseriesArchiveService() override;

    bool start();
    void stopSubscriber();
    QString lastStatus() const;

signals:
    void statusChanged(const QString& status);

private:
    void beginStartup();
    void checkInfluxdbThenContinue();
    void startInfluxdbProcess();
    void stopInfluxdb();
    void pollInfluxdbUntilReady(const std::shared_ptr<QElapsedTimer>& elapsed);
    void ensureDatabase();
    void ensureDatabaseAttempt(const std::shared_ptr<QElapsedTimer>& elapsed);
    void retryEnsureDatabase(const QString& reason, const std::shared_ptr<QElapsedTimer>& elapsed);
    void startSubscriber();
    void checkHostReachable(const QString& url,
                            int timeoutMs,
                            const std::function<void(bool)>& callback);
    void runInfluxCli(const QStringList& arguments,
                      const QProcessEnvironment& environment,
                      int timeoutMs,
                      const std::function<void(bool, const QString&)>& callback);
    QString resolveExecutablePath(const QString& configuredPath) const;
    void applyVisibleConsole(QProcess& process, bool visible) const;
    bool databaseListContains(const QByteArray& output) const;
    void setStatus(const QString& status);
#ifdef Q_OS_WIN
    void createInfluxdbKillJob();
    void attachInfluxdbToKillJob();
    int adoptConfiguredInfluxdbProcesses();
    void closeInfluxdbKillJob();
#endif

    AppConfig config_;
    AppPaths paths_;
    QNetworkAccessManager network_;
    QProcess influxProcess_;
    QProcess subscriberProcess_;
    QString lastStatus_;
    bool startupQueued_ = false;
    bool startedInfluxdb_ = false;
#ifdef Q_OS_WIN
    void* influxJob_ = nullptr;
#endif
};
