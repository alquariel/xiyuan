#include "services/ftp/FtpService.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>

#include <utility>

FtpService::FtpService(FtpConfig config, QObject* parent)
    : QObject(parent),
      config_(std::move(config)),
      ownedUploader_(std::make_unique<FtpClient>()),
      uploader_(ownedUploader_.get()),
      enabled_(config_.enabled) {
}

FtpService::FtpService(FtpConfig config, IFtpUploader* uploader, QObject* parent)
    : QObject(parent),
      config_(std::move(config)),
      uploader_(uploader),
      enabled_(config_.enabled) {
}

QString FtpService::selectedLocalPath() const {
    return selectedLocalPath_;
}

QString FtpService::uploadedModFilename() const {
    return uploadedModFilename_;
}

bool FtpService::uploadSucceeded() const {
    return uploadSucceeded_;
}

QString FtpService::defaultModDirectory() {
    const QString appDir = QCoreApplication::applicationDirPath();
    // Legacy layout first so existing builds and tests continue to find the
    // directory they expect; fall back to build/path for new setups.
    const QString legacyDir = QDir(appDir).filePath("mod_files");
    if (QDir(legacyDir).exists()) return legacyDir;
    const QString preferredDir = QDir(appDir).filePath("path");
    return QDir(preferredDir).exists() ? preferredDir : appDir;
}

void FtpService::openLocus() {
    const QString selectedPath = QFileDialog::getOpenFileName(
        nullptr,
        QStringLiteral("Select MOD path file"),
        defaultModDirectory(),
        QStringLiteral("MOD files (*.mod)"));

    if (selectedPath.isEmpty()) {
        // Matching the Python behavior, canceling does not erase the previous
        // successful upload state. It only tells the operator no file was chosen.
        emit messageRequested(QStringLiteral("FTP Upload"),
                              QStringLiteral("No print path was selected; please select again."));
        return;
    }

    uploadSelectedFile(selectedPath);
}

bool FtpService::uploadSelectedFile(const QString& localFilePath) {
    if (!validateSelectedFile(localFilePath)) {
        clearUploadState();
        return false;
    }

    const QFileInfo fileInfo(localFilePath);
    const QString remoteFilename = fileInfo.fileName();

    if (!enabled_) {
        // Simulation mode: the station has no FTP server (RobotStudio virtual
        // controllers typically don't expose one). Treat the local file as
        // "uploaded" so the downstream robot-print workflow can still send
        // LOAD_MOD:<filename># over the socket channel.
        setUploadState(fileInfo.absoluteFilePath(), remoteFilename);
        emit messageRequested(QStringLiteral("FTP Upload"),
                              QStringLiteral("FTP disabled (simulation mode): path file %1 is marked as selected.").arg(remoteFilename));
        return true;
    }

    if (uploader_ == nullptr) {
        clearUploadState();
        emit messageRequested(QStringLiteral("FTP Upload"),
                              QStringLiteral("FTP uploader is not available."));
        return false;
    }

    FtpUploadRequest request;
    request.host = config_.ip;
    request.port = config_.port;
    request.user = config_.user;
    request.password = config_.password;
    request.localFilePath = fileInfo.absoluteFilePath();
    request.remoteFilename = remoteFilename;

    emit messageRequested(QStringLiteral("FTP Upload"),
                          QStringLiteral("Uploading file to FTP server..."));

    const FtpUploadResult result = uploader_->uploadFile(request);
    if (!result.success) {
        clearUploadState();
        emit messageRequested(QStringLiteral("FTP Upload"), result.message);
        return false;
    }

    setUploadState(fileInfo.absoluteFilePath(), remoteFilename);
    emit messageRequested(QStringLiteral("FTP Upload"),
                          QStringLiteral("Path file uploaded successfully: %1").arg(remoteFilename));
    return true;
}

bool FtpService::validateSelectedFile(const QString& localFilePath) {
    const QFileInfo fileInfo(localFilePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        emit messageRequested(
            QStringLiteral("FTP Upload"),
            QStringLiteral("Selected MOD file does not exist: %1").arg(localFilePath));
        return false;
    }

    if (fileInfo.suffix().compare(QStringLiteral("mod"), Qt::CaseInsensitive) != 0) {
        emit messageRequested(
            QStringLiteral("FTP Upload"),
            QStringLiteral("Selected file is not a .mod path file: %1").arg(localFilePath));
        return false;
    }

    if (enabled_ && (config_.ip.trimmed().isEmpty() || config_.port < 1 || config_.port > 65535)) {
        emit messageRequested(QStringLiteral("FTP Upload"),
                              QStringLiteral("FTP configuration is invalid."));
        return false;
    }

    return true;
}

void FtpService::clearUploadState() {
    selectedLocalPath_.clear();
    uploadedModFilename_.clear();
    uploadSucceeded_ = false;
}

void FtpService::setUploadState(const QString& localFilePath, const QString& remoteFilename) {
    selectedLocalPath_ = localFilePath;
    uploadedModFilename_ = remoteFilename;
    uploadSucceeded_ = true;
}
