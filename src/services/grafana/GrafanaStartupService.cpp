#include "services/grafana/GrafanaStartupService.h"

#include <QFileInfo>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QTimer>
#include <QUrl>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#endif

#include <memory>
#include <utility>

GrafanaStartupService::GrafanaStartupService(GrafanaConfig config, AppPaths paths, QObject* parent)
    : QObject(parent),
      config_(std::move(config)),
      paths_(std::move(paths)) {
}

bool GrafanaStartupService::start() {
    if (!config_.enabled || !config_.autostart) {
        setStatus("Grafana startup is disabled.");
        return false;
    }
    if (startupQueued_) {
        return true;
    }

    startupQueued_ = true;
    setStatus("Grafana startup queued.");
    if (isConfiguredProcessRunning()) {
        setStatus(QString("Grafana process is already running; not starting another server: %1")
                      .arg(QFileInfo(resolveExecutablePath(config_.exePath)).fileName()));
        return true;
    }

    QTimer::singleShot(0, this, &GrafanaStartupService::checkHostReachable);
    return true;
}

QString GrafanaStartupService::lastStatus() const {
    return lastStatus_;
}

void GrafanaStartupService::beginStartup() {
    const QString grafanaExecutable = resolveExecutablePath(config_.exePath);
    if (!QFileInfo::exists(grafanaExecutable)) {
        setStatus(QString("Grafana executable does not exist: %1").arg(config_.exePath));
        return;
    }

    const QString workingDirectory = resolveWorkingDirectory();
    if (!QFileInfo::exists(workingDirectory)) {
        setStatus(QString("Grafana working directory does not exist: %1").arg(config_.workingDir));
        return;
    }

    // Grafana is intentionally fire-and-forget. Operators can watch and stop it
    // from its own console, and it may keep running after AFP_ABB_CPP exits.
    QProcess process;
    process.setProgram(grafanaExecutable);
    process.setArguments({"server"});
    process.setWorkingDirectory(workingDirectory);
#ifdef Q_OS_WIN
    if (config_.showConsole) {
        process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments* arguments) {
            arguments->flags |= CREATE_NEW_CONSOLE;
            arguments->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES;
        });
    }
#endif

    qint64 processId = 0;
    if (!process.startDetached(&processId)) {
        setStatus(QString("Could not start Grafana server: %1").arg(grafanaExecutable));
        return;
    }

    setStatus(QString("Grafana server startup requested: %1").arg(processId));
}

void GrafanaStartupService::checkHostReachable() {
    const QUrl url(config_.hostUrl);
    if (!url.isValid()) {
        beginStartup();
        return;
    }

    QNetworkRequest request(url);
    request.setTransferTimeout(config_.startupTimeoutMs);
    QNetworkReply* reply = network_.get(request);
    QTimer* timer = new QTimer(reply);
    timer->setSingleShot(true);
    const auto completed = std::make_shared<bool>(false);

    const auto finish = [this, reply, timer, completed]() {
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

        if (reachable) {
            setStatus(QString("Grafana host is already reachable; not starting another server: %1").arg(config_.hostUrl));
            return;
        }
        beginStartup();
    };

    connect(reply, &QNetworkReply::finished, this, finish);
    connect(timer, &QTimer::timeout, this, finish);
    timer->start(config_.startupTimeoutMs);
}

bool GrafanaStartupService::isConfiguredProcessRunning() const {
#ifdef Q_OS_WIN
    const QString expectedName = QFileInfo(resolveExecutablePath(config_.exePath)).fileName();
    if (expectedName.isEmpty()) {
        return false;
    }

    // A Grafana console can exist before the HTTP endpoint is ready. Checking
    // the Windows process table prevents AFP_ABB_CPP from opening a duplicate
    // console during that warm-up window.
    const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(PROCESSENTRY32W);
    bool found = false;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            const QString runningName = QString::fromWCharArray(entry.szExeFile);
            if (runningName.compare(expectedName, Qt::CaseInsensitive) == 0) {
                found = true;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return found;
#else
    return false;
#endif
}

QString GrafanaStartupService::resolveExecutablePath(const QString& configuredPath) const {
    const QFileInfo info(configuredPath);
    if (info.isAbsolute()) {
        return info.absoluteFilePath();
    }
    return QFileInfo(paths_.rootDir().absoluteFilePath(configuredPath)).absoluteFilePath();
}

QString GrafanaStartupService::resolveWorkingDirectory() const {
    const QFileInfo info(config_.workingDir);
    if (info.isAbsolute()) {
        return info.absoluteFilePath();
    }
    return QFileInfo(paths_.rootDir().absoluteFilePath(config_.workingDir)).absoluteFilePath();
}

void GrafanaStartupService::setStatus(const QString& status) {
    lastStatus_ = status;
    emit statusChanged(status);
}
