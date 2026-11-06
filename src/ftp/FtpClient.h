#pragma once

#include <QString>

// Plain request object for one FTP upload. Keeping this separate from the
// implementation makes it easy for later UI/service code to validate and pass
// operator-entered settings without learning any WinINet details.
struct FtpUploadRequest {
    QString host;
    int port = 21;
    QString user;
    QString password;
    QString localFilePath;
    QString remoteFilename;
    int timeoutMs = 5000;
};

// Structured result returned to callers. The FTP layer never shows UI; callers
// decide whether to log, retry, or present the message in a dialog.
struct FtpUploadResult {
    bool success = false;
    QString message;
};

// Small seam for the migration: production code can depend on IFtpUploader,
// while tests or higher-level services may provide their own uploader later.
class IFtpUploader {
public:
    virtual ~IFtpUploader() = default;

    virtual FtpUploadResult uploadFile(const FtpUploadRequest& request) = 0;
};

// Low-level WinINet FTP uploader. This class owns only transport mechanics;
// user interaction and application workflow stay above this layer.
class FtpClient final : public IFtpUploader {
public:
    FtpUploadResult uploadFile(const FtpUploadRequest& request) override;
};
