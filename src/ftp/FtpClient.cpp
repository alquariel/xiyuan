#include "ftp/FtpClient.h"

#include <string>

#include <QFileInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wininet.h>
#endif

namespace {

constexpr int kMaxTimeoutMs = 300000;

FtpUploadResult failure(const QString& message) {
    return {false, message};
}

FtpUploadResult success(const QString& message) {
    return {true, message};
}

FtpUploadResult validateRequest(const FtpUploadRequest& request) {
    if (request.host.trimmed().isEmpty()) {
        return failure("FTP host is required.");
    }

    if (request.port < 1 || request.port > 65535) {
        return failure("FTP port must be between 1 and 65535.");
    }

    if (request.timeoutMs < 1 || request.timeoutMs > kMaxTimeoutMs) {
        return failure(QString("FTP timeout must be between 1 and %1 ms.").arg(kMaxTimeoutMs));
    }

    if (request.localFilePath.trimmed().isEmpty()) {
        return failure("FTP local file path is required.");
    }

    const QFileInfo localFile(request.localFilePath);
    if (!localFile.exists() || !localFile.isFile()) {
        return failure(QString("FTP local file does not exist: %1").arg(request.localFilePath));
    }

    if (request.remoteFilename.trimmed().isEmpty()) {
        return failure("FTP remote filename is required.");
    }

    return success(QString());
}

#ifdef Q_OS_WIN

// WinINet returns raw HINTERNET handles. This tiny owner keeps every early
// return leak-free while still making the upload steps read in WinINet order.
class InternetHandle final {
public:
    explicit InternetHandle(HINTERNET handle = nullptr)
        : handle_(handle) {
    }

    ~InternetHandle() {
        reset();
    }

    InternetHandle(const InternetHandle&) = delete;
    InternetHandle& operator=(const InternetHandle&) = delete;

    HINTERNET get() const {
        return handle_;
    }

    explicit operator bool() const {
        return handle_ != nullptr;
    }

    void reset(HINTERNET handle = nullptr) {
        if (handle_ != nullptr) {
            InternetCloseHandle(handle_);
        }
        handle_ = handle;
    }

private:
    HINTERNET handle_ = nullptr;
};

QString windowsErrorMessage(DWORD errorCode) {
    LPWSTR buffer = nullptr;
    const DWORD length = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        0,
        reinterpret_cast<LPWSTR>(&buffer),
        0,
        nullptr);

    QString message;
    if (length > 0 && buffer != nullptr) {
        message = QString::fromWCharArray(buffer, static_cast<int>(length)).trimmed();
        LocalFree(buffer);
    }

    if (message.isEmpty()) {
        message = QString("Windows error %1").arg(errorCode);
    }

    return message;
}

QString lastWinInetErrorMessage(const QString& action) {
    const DWORD errorCode = GetLastError();
    return QString("FTP %1 failed: %2").arg(action, windowsErrorMessage(errorCode));
}

std::wstring toStdWString(const QString& value) {
    return value.toStdWString();
}

bool setInternetTimeout(HINTERNET handle, DWORD option, DWORD timeoutMs) {
    return InternetSetOptionW(handle, option, &timeoutMs, sizeof(timeoutMs)) == TRUE;
}

#endif

}

FtpUploadResult FtpClient::uploadFile(const FtpUploadRequest& request) {
    const FtpUploadResult validation = validateRequest(request);
    if (!validation.success) {
        return validation;
    }

#ifndef Q_OS_WIN
    Q_UNUSED(request);
    return failure("FTP upload is only implemented on Windows with WinINet.");
#else
    const DWORD timeoutMs = static_cast<DWORD>(request.timeoutMs);

    // InternetOpen starts a WinINet session for this one upload. No global
    // session is kept because the migrated Python flow uploads small files on
    // demand rather than maintaining a long-lived FTP connection.
    InternetHandle internet(InternetOpenW(
        L"AFP_ABB_CPP",
        INTERNET_OPEN_TYPE_PRECONFIG,
        nullptr,
        nullptr,
        0));
    if (!internet) {
        return failure(lastWinInetErrorMessage("session open"));
    }

    if (!setInternetTimeout(internet.get(), INTERNET_OPTION_CONNECT_TIMEOUT, timeoutMs) ||
        !setInternetTimeout(internet.get(), INTERNET_OPTION_SEND_TIMEOUT, timeoutMs) ||
        !setInternetTimeout(internet.get(), INTERNET_OPTION_RECEIVE_TIMEOUT, timeoutMs)) {
        return failure(lastWinInetErrorMessage("timeout configuration"));
    }

    const std::wstring host = toStdWString(request.host.trimmed());
    const std::wstring user = toStdWString(request.user);
    const std::wstring password = toStdWString(request.password);

    // Passive FTP matches the Python deployment expectations better in plant
    // networks where the client may sit behind NAT or a strict firewall.
    InternetHandle connection(InternetConnectW(
        internet.get(),
        host.c_str(),
        static_cast<INTERNET_PORT>(request.port),
        user.c_str(),
        password.c_str(),
        INTERNET_SERVICE_FTP,
        INTERNET_FLAG_PASSIVE,
        0));
    if (!connection) {
        return failure(lastWinInetErrorMessage("connect"));
    }

    const std::wstring localPath = toStdWString(QFileInfo(request.localFilePath).absoluteFilePath());
    const std::wstring remoteFilename = toStdWString(request.remoteFilename.trimmed());

    if (FtpPutFileW(
            connection.get(),
            localPath.c_str(),
            remoteFilename.c_str(),
            FTP_TRANSFER_TYPE_BINARY,
            0) != TRUE) {
        return failure(lastWinInetErrorMessage("upload"));
    }

    WIN32_FIND_DATAW findData{};
    InternetHandle remoteFile(FtpFindFirstFileW(
        connection.get(),
        remoteFilename.c_str(),
        &findData,
        INTERNET_FLAG_RELOAD,
        0));
    if (!remoteFile) {
        return failure(lastWinInetErrorMessage("upload verification"));
    }

    return success(QString("FTP uploaded %1 to %2:%3/%4")
                       .arg(QFileInfo(request.localFilePath).fileName())
                       .arg(request.host.trimmed())
                       .arg(request.port)
                       .arg(request.remoteFilename.trimmed()));
#endif
}
