#pragma once

#include "app/AppConfig.h"
#include "ftp/FtpClient.h"
#include "services/robot/RobotPrintInterfaces.h"

#include <QObject>
#include <QString>

#include <memory>

// FtpService owns the UI-facing part of the MOD path upload workflow.
// It keeps the selected local path, uploaded remote filename, and success flag
// so a later robot-print slice can reuse the same state the Python software
// stored on its main window object.
class FtpService final : public QObject, public IPrintUploadState {
    Q_OBJECT

public:
    explicit FtpService(FtpConfig config, QObject* parent = nullptr);
    FtpService(FtpConfig config, IFtpUploader* uploader, QObject* parent = nullptr);

    QString selectedLocalPath() const;
    QString uploadedModFilename() const override;
    bool uploadSucceeded() const override;

    // Returns the portable C++ MOD folder beside the executable when present.
    // This intentionally replaces the old Python-only pythonLIb/printpath path.
    static QString defaultModDirectory();

signals:
    void messageRequested(const QString& title, const QString& message);

public slots:
    void openLocus();

public:
    bool uploadSelectedFile(const QString& localFilePath);

private:
    bool validateSelectedFile(const QString& localFilePath);
    void clearUploadState();
    void setUploadState(const QString& localFilePath, const QString& remoteFilename);

    FtpConfig config_;
    std::unique_ptr<IFtpUploader> ownedUploader_;
    IFtpUploader* uploader_ = nullptr;
    QString selectedLocalPath_;
    QString uploadedModFilename_;
    bool uploadSucceeded_ = false;
};
