#include "services/archive/TimeseriesArchiveService.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcessEnvironment>
#include <QTimer>
#include <QUrl>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#endif

#include <utility>

#include "archive/InfluxAdmin.h"

namespace {
QString processErrorText(QProcess* process) {
    if (process == nullptr) {
        return {};
    }
    const QString errorOutput = QString::fromLocal8Bit(process->readAllStandardError()).trimmed();
    if (!errorOutput.isEmpty()) {
        return errorOutput;
    }
    return process->errorString();
}

QString describeWindowsExceptionExit(int exitCode, QProcess::ExitStatus exitStatus) {
    if (exitStatus != QProcess::CrashExit) {
        return {};
    }

    const auto code = static_cast<quint32>(exitCode);
    QString description;
    switch (code) {
    case 0xC0000005:
        description = "access violation";
        break;
    case 0xC00000FD:
        description = "stack overflow";
        break;
    case 0xC0000409:
        description = "stack buffer overrun / fast fail";
        break;
    case 0xC0000374:
        description = "heap corruption";
        break;
    default:
        description = "unhandled Windows exception";
        break;
    }

    // QProcess reports Windows exception codes as signed integers. Include the
    // unsigned hexadecimal form because it is the value used by Microsoft
    // debugger documentation and crash reports.
    const QString hexCode = QString("%1").arg(code, 8, 16, QLatin1Char('0')).toUpper();
    return QString(" windows_exception=0x%1 (%2)").arg(hexCode, description);
}
}

TimeseriesArchiveService::TimeseriesArchiveService(AppConfig config, AppPaths paths, QObject* parent)
    : QObject(parent),
      config_(std::move(config)),
      paths_(std::move(paths)) {
    connect(&influxProcess_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        setStatus(QString("InfluxDB process error: %1").arg(influxProcess_.errorString()));
    });
    connect(&subscriberProcess_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        setStatus(QString("DDS InfluxDB subscriber process error: %1").arg(subscriberProcess_.errorString()));
    });
    connect(&subscriberProcess_,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [this](int exitCode, QProcess::ExitStatus exitStatus) {
                const QString standardError = QString::fromLocal8Bit(subscriberProcess_.readAllStandardError()).trimmed();
                const QString standardOutput = QString::fromLocal8Bit(subscriberProcess_.readAllStandardOutput()).trimmed();
                QString message = QString("DDS InfluxDB subscriber exited. exitCode=%1 exitStatus=%2")
                                      .arg(exitCode)
                                      .arg(exitStatus == QProcess::NormalExit ? "normal" : "crashed");
                message += describeWindowsExceptionExit(exitCode, exitStatus);
                if (!standardError.isEmpty()) {
                    message += QString(" stderr_tail=\"%1\"").arg(standardError.right(500));
                }
                if (!standardOutput.isEmpty()) {
                    message += QString(" stdout_tail=\"%1\"").arg(standardOutput.right(500));
                }
                setStatus(message);
            });
}

TimeseriesArchiveService::~TimeseriesArchiveService() {
    stopSubscriber();
    stopInfluxdb();
}

bool TimeseriesArchiveService::start() {
    if (!config_.timeseriesArchive.enabled) {
        setStatus("Time-series archive is disabled.");
        return false;
    }
    if (startupQueued_) {
        return true;
    }

    startupQueued_ = true;
    setStatus("Time-series archive startup queued.");
    QTimer::singleShot(0, this, &TimeseriesArchiveService::beginStartup);
    return true;
}

void TimeseriesArchiveService::stopSubscriber() {
    if (subscriberProcess_.state() == QProcess::NotRunning) {
        return;
    }

    subscriberProcess_.terminate();
    if (!subscriberProcess_.waitForFinished(1000)) {
        subscriberProcess_.kill();
        subscriberProcess_.waitForFinished(1000);
    }
}

QString TimeseriesArchiveService::lastStatus() const {
    return lastStatus_;
}

void TimeseriesArchiveService::beginStartup() {
    checkInfluxdbThenContinue();
}

void TimeseriesArchiveService::checkInfluxdbThenContinue() {
    if (!config_.timeseriesArchive.startInfluxdb) {
        ensureDatabase();
        return;
    }

    checkHostReachable(
        config_.timeseriesArchive.hostUrl,
        config_.timeseriesArchive.reachabilityProbeTimeoutMs,
        [this](bool reachable) {
            if (reachable) {
#ifdef Q_OS_WIN
                if (config_.timeseriesArchive.stopInfluxdbOnExit) {
                    const int adoptedCount = adoptConfiguredInfluxdbProcesses();
                    setStatus(QString("InfluxDB host is already reachable; managing %1 existing server process(es).")
                                  .arg(adoptedCount));
                } else
#endif
                {
                    setStatus("InfluxDB host is already reachable; using existing server.");
                }
                ensureDatabase();
                return;
            }
            startInfluxdbProcess();
        });
}

void TimeseriesArchiveService::startInfluxdbProcess() {
    const QString influxExecutable = resolveExecutablePath(config_.timeseriesArchive.influxdbExePath);
    if (!QFileInfo::exists(influxExecutable)) {
        setStatus(QString("InfluxDB executable does not exist: %1").arg(config_.timeseriesArchive.influxdbExePath));
        return;
    }

    // The server process receives only the token-file path, never the token
    // value. Database CLI calls below pass the token through the environment.
#ifdef Q_OS_WIN
    createInfluxdbKillJob();
#endif
    influxProcess_.setProgram(influxExecutable);
    influxProcess_.setArguments(buildInfluxServeArguments(config_.timeseriesArchive));
    influxProcess_.setWorkingDirectory(QFileInfo(influxExecutable).absolutePath());
    influxProcess_.setProcessChannelMode(QProcess::SeparateChannels);
    applyVisibleConsole(influxProcess_, config_.timeseriesArchive.influxdbShowConsole);

    connect(&influxProcess_, &QProcess::started, this, [this]() {
        startedInfluxdb_ = true;
#ifdef Q_OS_WIN
        attachInfluxdbToKillJob();
#endif
        setStatus("InfluxDB server process started; waiting for HTTP endpoint.");
        pollInfluxdbUntilReady(std::make_shared<QElapsedTimer>());
    }, Qt::SingleShotConnection);

    influxProcess_.start();
}

void TimeseriesArchiveService::stopInfluxdb() {
    if (influxProcess_.state() != QProcess::NotRunning) {
        // First ask InfluxDB to exit cleanly. If it ignores the request, force
        // the process to stop so the server console does not outlive the app.
        influxProcess_.terminate();
        if (!influxProcess_.waitForFinished(3000)) {
            influxProcess_.kill();
            influxProcess_.waitForFinished(3000);
        }
    }

#ifdef Q_OS_WIN
    // Closing the job handle with KILL_ON_JOB_CLOSE also cleans up any child
    // process created by the InfluxDB server process we started.
    closeInfluxdbKillJob();
#endif
}

void TimeseriesArchiveService::pollInfluxdbUntilReady(const std::shared_ptr<QElapsedTimer>& elapsed) {
    if (!elapsed->isValid()) {
        elapsed->start();
    }
    if (elapsed->elapsed() > config_.timeseriesArchive.influxdbStartupTimeoutMs) {
        setStatus(QString("InfluxDB host did not become reachable: %1").arg(config_.timeseriesArchive.hostUrl));
        return;
    }

    checkHostReachable(
        config_.timeseriesArchive.hostUrl,
        config_.timeseriesArchive.reachabilityProbeTimeoutMs,
        [this, elapsed](bool reachable) {
            if (reachable) {
                setStatus("InfluxDB server is reachable.");
                ensureDatabase();
                return;
            }
            QTimer::singleShot(200, this, [this, elapsed]() {
                pollInfluxdbUntilReady(elapsed);
            });
        });
}

void TimeseriesArchiveService::ensureDatabase() {
    if (!config_.timeseriesArchive.ensureDatabaseOnStart) {
        startSubscriber();
        return;
    }

    auto elapsed = std::make_shared<QElapsedTimer>();
    elapsed->start();
    ensureDatabaseAttempt(elapsed);
}

void TimeseriesArchiveService::ensureDatabaseAttempt(const std::shared_ptr<QElapsedTimer>& elapsed) {
    // InfluxDB may accept HTTP connections before its CLI/database layer is
    // fully ready. Retry transient CLI failures so a cold server start does not
    // silently skip the DDS subscriber.

    const QString influxExecutable = resolveExecutablePath(config_.timeseriesArchive.influxdbExePath);
    if (!QFileInfo::exists(influxExecutable)) {
        setStatus(QString("InfluxDB executable does not exist: %1").arg(config_.timeseriesArchive.influxdbExePath));
        return;
    }

    QString tokenError;
    const QString token = readInfluxAdminToken(config_.timeseriesArchive.adminTokenFile, &tokenError);
    if (token.isEmpty()) {
        setStatus(tokenError);
        return;
    }

    const QProcessEnvironment environment = buildInfluxCliEnvironment(config_.timeseriesArchive, token);
    runInfluxCli(
        buildInfluxShowDatabasesArguments(),
        environment,
        config_.timeseriesArchive.cliTimeoutMs,
        [this, environment, elapsed](bool ok, const QString& databaseList) {
            if (!ok) {
                retryEnsureDatabase(QString("InfluxDB database check failed: %1").arg(databaseList), elapsed);
                return;
            }
            if (databaseListContains(databaseList.toUtf8())) {
                setStatus(QString("InfluxDB database already exists: %1").arg(config_.timeseriesArchive.database));
                startSubscriber();
                return;
            }

            runInfluxCli(
                buildInfluxCreateDatabaseArguments(config_.timeseriesArchive),
                environment,
                config_.timeseriesArchive.cliTimeoutMs,
                [this, elapsed](bool createOk, const QString& createOutput) {
                    if (!createOk) {
                        retryEnsureDatabase(QString("InfluxDB database creation failed: %1").arg(createOutput), elapsed);
                        return;
                    }
                    setStatus(QString("InfluxDB database created: %1").arg(config_.timeseriesArchive.database));
                    startSubscriber();
                });
        });
}

void TimeseriesArchiveService::retryEnsureDatabase(const QString& reason, const std::shared_ptr<QElapsedTimer>& elapsed) {
    if (elapsed != nullptr &&
        elapsed->isValid() &&
        elapsed->elapsed() < config_.timeseriesArchive.influxdbStartupTimeoutMs) {
        setStatus(QString("%1 Retrying InfluxDB database setup.").arg(reason));
        QTimer::singleShot(500, this, [this, elapsed]() {
            ensureDatabaseAttempt(elapsed);
        });
        return;
    }

    setStatus(reason);
}

void TimeseriesArchiveService::startSubscriber() {
    if (!config_.timeseriesArchive.subscriberAutostart) {
        setStatus("DDS InfluxDB subscriber autostart is disabled.");
        return;
    }
    if (!config_.pressureArchive.enabled) {
        setStatus("Pressure archive is disabled; DDS InfluxDB subscriber not started.");
        return;
    }

    const QString subscriberExecutable = resolveExecutablePath(config_.timeseriesArchive.subscriberExecutable);
    if (!QFileInfo::exists(subscriberExecutable)) {
        setStatus(QString("DDS InfluxDB subscriber executable does not exist: %1")
                      .arg(config_.timeseriesArchive.subscriberExecutable));
        return;
    }

    // The subscriber reads the shared config and token file itself. The main app
    // deliberately passes no token or database credentials to this child.
    subscriberProcess_.setProgram(subscriberExecutable);
    subscriberProcess_.setWorkingDirectory(paths_.rootDir().absolutePath());
    subscriberProcess_.setProcessChannelMode(QProcess::SeparateChannels);
    applyVisibleConsole(subscriberProcess_, config_.timeseriesArchive.subscriberShowConsole);
    connect(&subscriberProcess_, &QProcess::started, this, [this]() {
        setStatus(config_.timeseriesArchive.subscriberShowConsole
                      ? "DDS InfluxDB subscriber started with live console."
                      : "DDS InfluxDB subscriber started.");
    }, Qt::SingleShotConnection);
    subscriberProcess_.start();
}

void TimeseriesArchiveService::checkHostReachable(const QString& urlText,
                                                  int timeoutMs,
                                                  const std::function<void(bool)>& callback) {
    const QUrl url(urlText);
    if (!url.isValid()) {
        callback(false);
        return;
    }

    QNetworkRequest request(url);
    request.setTransferTimeout(timeoutMs);
    QNetworkReply* reply = network_.get(request);
    QTimer* timer = new QTimer(reply);
    timer->setSingleShot(true);
    const auto completed = std::make_shared<bool>(false);

    const auto finish = [reply, timer, callback, completed]() {
        if (*completed) {
            return;
        }
        *completed = true;
        if (reply->isRunning()) {
            reply->abort();
        }
        const bool hasHttpResponse = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).isValid();
        const bool reachable = hasHttpResponse || reply->error() == QNetworkReply::NoError;
        timer->stop();
        reply->deleteLater();
        callback(reachable);
    };

    connect(reply, &QNetworkReply::finished, this, finish);
    connect(timer, &QTimer::timeout, this, finish);
    timer->start(timeoutMs);
}

void TimeseriesArchiveService::runInfluxCli(const QStringList& arguments,
                                            const QProcessEnvironment& environment,
                                            int timeoutMs,
                                            const std::function<void(bool, const QString&)>& callback) {
    const QString influxExecutable = resolveExecutablePath(config_.timeseriesArchive.influxdbExePath);
    QProcess* process = new QProcess(this);
    QTimer* timer = new QTimer(process);
    timer->setSingleShot(true);
    const auto completed = std::make_shared<bool>(false);

    process->setProgram(influxExecutable);
    process->setArguments(arguments);
    process->setProcessEnvironment(environment);
    process->setWorkingDirectory(QFileInfo(influxExecutable).absolutePath());
    process->setProcessChannelMode(QProcess::SeparateChannels);

    connect(timer, &QTimer::timeout, this, [process, callback, completed]() {
        if (*completed) {
            return;
        }
        *completed = true;
        process->kill();
        callback(false, "InfluxDB CLI timed out.");
        process->deleteLater();
    });
    connect(process, &QProcess::errorOccurred, this, [process, timer, callback, completed](QProcess::ProcessError) {
        if (*completed) {
            return;
        }
        *completed = true;
        timer->stop();
        callback(false, process->errorString());
        process->deleteLater();
    });
    connect(process,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [process, timer, callback, completed](int exitCode, QProcess::ExitStatus exitStatus) {
                if (*completed) {
                    return;
                }
                *completed = true;
                timer->stop();
                if (exitStatus != QProcess::NormalExit || exitCode != 0) {
                    callback(false, processErrorText(process));
                } else {
                    callback(true, QString::fromLocal8Bit(process->readAllStandardOutput()));
                }
                process->deleteLater();
            });

    timer->start(timeoutMs);
    process->start();
}

QString TimeseriesArchiveService::resolveExecutablePath(const QString& configuredPath) const {
    const QFileInfo info(configuredPath);
    if (info.isAbsolute()) {
        return info.absoluteFilePath();
    }
    return QFileInfo(paths_.rootDir().absoluteFilePath(configuredPath)).absoluteFilePath();
}

void TimeseriesArchiveService::applyVisibleConsole(QProcess& process, bool visible) const {
#ifdef Q_OS_WIN
    if (visible) {
        process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* arguments) {
            arguments->flags |= CREATE_NEW_CONSOLE;
            arguments->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES;
        });
    }
#else
    Q_UNUSED(process);
    Q_UNUSED(visible);
#endif
}

bool TimeseriesArchiveService::databaseListContains(const QByteArray& output) const {
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(output, &error);
    if (error.error != QJsonParseError::NoError) {
        return QString::fromUtf8(output).contains(config_.timeseriesArchive.database);
    }

    const auto objectMatches = [this](const QJsonObject& object) {
        for (const QJsonValue& value : object) {
            if (value.toString() == config_.timeseriesArchive.database) {
                return true;
            }
        }
        return false;
    };

    if (document.isArray()) {
        for (const QJsonValue& value : document.array()) {
            if (value.toString() == config_.timeseriesArchive.database) {
                return true;
            }
            if (value.isObject() && objectMatches(value.toObject())) {
                return true;
            }
        }
    } else if (document.isObject()) {
        const QJsonObject root = document.object();
        if (objectMatches(root)) {
            return true;
        }
        for (const QJsonValue& value : root) {
            if (value.isArray()) {
                for (const QJsonValue& item : value.toArray()) {
                    if (item.toString() == config_.timeseriesArchive.database) {
                        return true;
                    }
                    if (item.isObject() && objectMatches(item.toObject())) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void TimeseriesArchiveService::setStatus(const QString& status) {
    lastStatus_ = status;
    qInfo().noquote() << status;
    emit statusChanged(status);
}

#ifdef Q_OS_WIN
void TimeseriesArchiveService::createInfluxdbKillJob() {
    closeInfluxdbKillJob();

    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (job == nullptr) {
        return;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits))) {
        CloseHandle(job);
        return;
    }

    influxJob_ = job;
}

void TimeseriesArchiveService::attachInfluxdbToKillJob() {
    if (influxJob_ == nullptr || influxProcess_.processId() <= 0) {
        return;
    }

    HANDLE process = OpenProcess(PROCESS_SET_QUOTA | PROCESS_TERMINATE, FALSE, static_cast<DWORD>(influxProcess_.processId()));
    if (process == nullptr) {
        return;
    }

    AssignProcessToJobObject(static_cast<HANDLE>(influxJob_), process);
    CloseHandle(process);
}

int TimeseriesArchiveService::adoptConfiguredInfluxdbProcesses() {
    createInfluxdbKillJob();
    if (influxJob_ == nullptr) {
        return 0;
    }

    const QFileInfo expectedInfo(resolveExecutablePath(config_.timeseriesArchive.influxdbExePath));
    QString expectedPath = expectedInfo.canonicalFilePath();
    if (expectedPath.isEmpty()) {
        expectedPath = expectedInfo.absoluteFilePath();
    }
    if (expectedPath.isEmpty()) {
        return 0;
    }
    expectedPath = QDir::cleanPath(QDir::fromNativeSeparators(expectedPath));

    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    int adoptedCount = 0;
    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(PROCESSENTRY32W);
    if (Process32FirstW(snapshot, &entry)) {
        do {
            HANDLE process = OpenProcess(
                PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_SET_QUOTA | PROCESS_TERMINATE,
                FALSE,
                entry.th32ProcessID);
            if (process == nullptr) {
                continue;
            }

            wchar_t imagePath[MAX_PATH] = {};
            DWORD imagePathSize = MAX_PATH;
            const bool imageOk = QueryFullProcessImageNameW(process, 0, imagePath, &imagePathSize);
            const QString runningPath = imageOk
                ? QDir::cleanPath(QDir::fromNativeSeparators(QFileInfo(QString::fromWCharArray(imagePath)).absoluteFilePath()))
                : QString();
            if (!runningPath.isEmpty() && runningPath.compare(expectedPath, Qt::CaseInsensitive) == 0) {
                if (AssignProcessToJobObject(static_cast<HANDLE>(influxJob_), process)) {
                    ++adoptedCount;
                }
            }

            CloseHandle(process);
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return adoptedCount;
}

void TimeseriesArchiveService::closeInfluxdbKillJob() {
    if (influxJob_ == nullptr) {
        return;
    }

    CloseHandle(static_cast<HANDLE>(influxJob_));
    influxJob_ = nullptr;
}
#endif
