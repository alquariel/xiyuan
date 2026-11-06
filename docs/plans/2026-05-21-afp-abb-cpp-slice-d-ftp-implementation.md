# AFP ABB C++ Slice D FTP Upload Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the FTP placeholder behind `locusselectsetButton` with the original MOD-file upload behavior, using a portable C++ runtime folder instead of the old Python `pythonLIb` path.

**Architecture:** Add a small WinINet-based `FtpClient` with no UI dependencies, then add a Qt `FtpService` that owns file selection, upload state, validation, and user messages. Wire `MainWindow` to the new service while leaving the `.ui` design unchanged.

**Tech Stack:** C++17, Qt 6 Widgets/Core/Test, Windows WinINet, CMake, CTest.

---

## Scope Guardrails

- Do not edit anything under `AFP_ABB_Python`.
- Do not change `src/ui/Interface4600_window.ui`.
- Do not add robot motion, print start, pause, continue, stop, or socket behavior in this slice.
- Do not introduce libcurl or another FTP runtime dependency.
- Keep comments generous in new C++ code because this is migration code that future slices will build on.
- Use `mod_files` beside `AFP_ABB_CPP.exe` as the file picker start folder. Do not use `./pythonLIb/printpath/` in new runtime code.

## File Structure

- Create `src/ftp/FtpClient.h`: declares the upload request/result structs, `IFtpUploader`, and the WinINet-backed `FtpClient`.
- Create `src/ftp/FtpClient.cpp`: implements FTP connect, upload, listing verification, WinINet handle cleanup, and readable Windows errors.
- Create `src/services/ftp/FtpService.h`: declares the Qt service that validates selected MOD files and stores upload state.
- Create `src/services/ftp/FtpService.cpp`: implements file picker behavior, upload-state transitions, and user-message signals.
- Modify `src/services/PlaceholderServices.h`: remove the placeholder `FtpService` class declaration.
- Modify `src/services/PlaceholderServices.cpp`: remove the placeholder `FtpService` constructor and slot implementation.
- Modify `src/ui/MainWindow.h`: include the real FTP service header and keep `ftpService_` as a concrete member.
- Modify `src/ui/MainWindow.cpp`: construct `ftpService_` with `config_.ftp`, connect messages, and keep `locusselectsetButton` wired to `openLocus()`.
- Modify `CMakeLists.txt`: add FTP sources to `APP_SOURCES`, link `wininet`, add FTP tests, and deploy an empty `mod_files` folder.
- Create `tests/test_ftp_service.cpp`: tests validation and upload-state behavior with a fake uploader, no network required.
- Create `mod_files/.gitkeep`: ensures the portable MOD-file folder exists in source and deployed output.
- Modify `README.md`: document FTP config, MOD-file folder, and manual smoke-test steps.

## Build Command Notes

When running CMake/MSBuild commands from PowerShell, use the existing cleaned environment pattern if duplicate `PATH`/`Path` causes build issues:

```powershell
$pathValue = [Environment]::GetEnvironmentVariable('Path','Process')
[Environment]::SetEnvironmentVariable('PATH',$null,'Process')
[Environment]::SetEnvironmentVariable('Path',$pathValue,'Process')
cmake --build build --config Release
```

## Task 1: Add the Low-Level FTP Client

**Files:**
- Create: `src/ftp/FtpClient.h`
- Create: `src/ftp/FtpClient.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create the FTP client header**

Create `src/ftp/FtpClient.h`:

```cpp
#pragma once

#include <QString>

// FtpUploadRequest contains one complete upload operation.
// The caller supplies the local MOD file and the remote filename that should
// appear on the ABB controller FTP server.
struct FtpUploadRequest {
    QString host;
    int port = 21;
    QString user;
    QString password;
    QString localFilePath;
    QString remoteFilename;
    int timeoutMs = 5000;
};

// FtpUploadResult keeps the low-level FTP client free of QMessageBox calls.
// UI services can display the message using the same message path as the rest
// of the C++ application.
struct FtpUploadResult {
    bool success = false;
    QString message;
};

// IFtpUploader lets FtpService be tested without talking to a live ABB
// controller or RobotStudio FTP server.
class IFtpUploader {
public:
    virtual ~IFtpUploader() = default;
    virtual FtpUploadResult uploadFile(const FtpUploadRequest& request) = 0;
};

// FtpClient is the production uploader. It uses WinINet because the target
// application is Windows-only and WinINet does not add another deployed DLL.
class FtpClient final : public IFtpUploader {
public:
    FtpUploadResult uploadFile(const FtpUploadRequest& request) override;
};
```

- [ ] **Step 2: Create the WinINet implementation**

Create `src/ftp/FtpClient.cpp`:

```cpp
#include "ftp/FtpClient.h"

#include <QFileInfo>
#include <QtGlobal>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wininet.h>
#endif

namespace {

#ifdef Q_OS_WIN
class InternetHandle {
public:
    explicit InternetHandle(HINTERNET handle = nullptr) : handle_(handle) {}
    ~InternetHandle()
    {
        if (handle_) {
            InternetCloseHandle(handle_);
        }
    }

    InternetHandle(const InternetHandle&) = delete;
    InternetHandle& operator=(const InternetHandle&) = delete;

    HINTERNET get() const { return handle_; }
    explicit operator bool() const { return handle_ != nullptr; }

private:
    HINTERNET handle_ = nullptr;
};

QString windowsErrorMessage(DWORD errorCode)
{
    wchar_t* buffer = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER
        | FORMAT_MESSAGE_FROM_SYSTEM
        | FORMAT_MESSAGE_IGNORE_INSERTS;

    const DWORD length = FormatMessageW(flags,
                                        nullptr,
                                        errorCode,
                                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                        reinterpret_cast<LPWSTR>(&buffer),
                                        0,
                                        nullptr);
    if (length == 0 || buffer == nullptr) {
        return QString("Windows error %1").arg(errorCode);
    }

    QString message = QString::fromWCharArray(buffer, static_cast<int>(length)).trimmed();
    LocalFree(buffer);
    return message.isEmpty() ? QString("Windows error %1").arg(errorCode) : message;
}

FtpUploadResult failWithLastError(const QString& context)
{
    const DWORD error = GetLastError();
    return {false, QString("%1: %2").arg(context, windowsErrorMessage(error))};
}
#endif

FtpUploadResult validateRequest(const FtpUploadRequest& request)
{
    if (request.host.trimmed().isEmpty()) {
        return {false, "FTP host is empty."};
    }
    if (request.port <= 0 || request.port > 65535) {
        return {false, "FTP port is invalid."};
    }
    if (request.localFilePath.trimmed().isEmpty()) {
        return {false, "No local MOD file was selected."};
    }
    const QFileInfo fileInfo(request.localFilePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return {false, QString("Selected MOD file does not exist: %1").arg(request.localFilePath)};
    }
    if (request.remoteFilename.trimmed().isEmpty()) {
        return {false, "Remote MOD filename is empty."};
    }
    return {true, QString()};
}

} // namespace

FtpUploadResult FtpClient::uploadFile(const FtpUploadRequest& request)
{
    const FtpUploadResult validation = validateRequest(request);
    if (!validation.success) {
        return validation;
    }

#ifndef Q_OS_WIN
    Q_UNUSED(request);
    return {false, "WinINet FTP upload is only available on Windows."};
#else
    InternetHandle internet(InternetOpenW(L"AFP_ABB_CPP",
                                          INTERNET_OPEN_TYPE_PRECONFIG,
                                          nullptr,
                                          nullptr,
                                          0));
    if (!internet) {
        return failWithLastError("Could not open WinINet session");
    }

    DWORD timeout = static_cast<DWORD>(request.timeoutMs);
    InternetSetOptionW(internet.get(), INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionW(internet.get(), INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionW(internet.get(), INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));

    InternetHandle connection(InternetConnectW(internet.get(),
                                               reinterpret_cast<LPCWSTR>(request.host.utf16()),
                                               static_cast<INTERNET_PORT>(request.port),
                                               reinterpret_cast<LPCWSTR>(request.user.utf16()),
                                               reinterpret_cast<LPCWSTR>(request.password.utf16()),
                                               INTERNET_SERVICE_FTP,
                                               INTERNET_FLAG_PASSIVE,
                                               0));
    if (!connection) {
        return failWithLastError("Could not connect to FTP server");
    }

    const BOOL uploaded = FtpPutFileW(connection.get(),
                                      reinterpret_cast<LPCWSTR>(request.localFilePath.utf16()),
                                      reinterpret_cast<LPCWSTR>(request.remoteFilename.utf16()),
                                      FTP_TRANSFER_TYPE_BINARY,
                                      0);
    if (!uploaded) {
        return failWithLastError("Could not upload MOD file");
    }

    WIN32_FIND_DATAW findData = {};
    InternetHandle listing(FtpFindFirstFileW(connection.get(),
                                             reinterpret_cast<LPCWSTR>(request.remoteFilename.utf16()),
                                             &findData,
                                             INTERNET_FLAG_RELOAD,
                                             0));
    if (!listing) {
        return failWithLastError("Uploaded MOD file was not found in FTP listing");
    }

    return {true, QString("Uploaded %1 to FTP server as %2.")
                      .arg(request.localFilePath, request.remoteFilename)};
#endif
}
```

- [ ] **Step 3: Add FTP sources and WinINet link to CMake**

In `CMakeLists.txt`, add these entries to `APP_SOURCES` near the other source groups:

```cmake
    src/ftp/FtpClient.h
    src/ftp/FtpClient.cpp
```

In the `target_link_libraries(AFP_ABB_CPP PRIVATE ...)` block, add:

```cmake
        wininet
```

- [ ] **Step 4: Configure and build**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64;C:\vcpkg\installed\x64-windows" -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Release
```

Expected: the project configures and `AFP_ABB_CPP.exe` still builds. Any failure at this point should be limited to syntax/link issues in `FtpClient`.

- [ ] **Step 5: Commit the FTP client**

```powershell
git add CMakeLists.txt src\ftp\FtpClient.h src\ftp\FtpClient.cpp
git commit -m "feat: add wininet ftp client"
```

## Task 2: Add the Qt FTP Service With Tests

**Files:**
- Create: `tests/test_ftp_service.cpp`
- Create: `src/services/ftp/FtpService.h`
- Create: `src/services/ftp/FtpService.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write the failing FTP service tests**

Create `tests/test_ftp_service.cpp`:

```cpp
#include <QtTest/QtTest>

#include "app/AppConfig.h"
#include "ftp/FtpClient.h"
#include "services/ftp/FtpService.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>

class FakeFtpUploader final : public IFtpUploader {
public:
    FtpUploadResult result{true, "fake upload success"};
    int callCount = 0;
    FtpUploadRequest lastRequest;

    FtpUploadResult uploadFile(const FtpUploadRequest& request) override
    {
        ++callCount;
        lastRequest = request;
        return result;
    }
};

class FtpServiceTests final : public QObject {
    Q_OBJECT

private slots:
    void successfulUploadStoresStateAndRemoteBasename();
    void failedUploadClearsState();
    void missingFileFailsBeforeNetwork();
    void nonModFileFailsBeforeNetwork();
    void defaultDirectoryUsesModFilesBesideApplicationWhenPresent();
};

namespace {

QString createFile(QTemporaryDir& dir, const QString& name)
{
    const QString path = dir.filePath(name);
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("MODULE TestPath\nENDMODULE\n");
    file.close();
    return path;
}

FtpConfig testConfig()
{
    FtpConfig config;
    config.ip = "192.168.0.104";
    config.port = 21;
    config.user = "Default User";
    config.password = "robotics";
    return config;
}

} // namespace

void FtpServiceTests::successfulUploadStoresStateAndRemoteBasename()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString modPath = createFile(dir, "path_a.mod");

    FakeFtpUploader uploader;
    FtpService service(testConfig(), &uploader);
    QSignalSpy messages(&service, &FtpService::messageRequested);

    QVERIFY(service.uploadSelectedFile(modPath));

    QCOMPARE(uploader.callCount, 1);
    QCOMPARE(uploader.lastRequest.localFilePath, modPath);
    QCOMPARE(uploader.lastRequest.remoteFilename, QString("path_a.mod"));
    QCOMPARE(service.selectedLocalPath(), modPath);
    QCOMPARE(service.uploadedModFilename(), QString("path_a.mod"));
    QVERIFY(service.uploadSucceeded());
    QVERIFY(messages.count() >= 1);
}

void FtpServiceTests::failedUploadClearsState()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString modPath = createFile(dir, "path_b.mod");

    FakeFtpUploader uploader;
    uploader.result = {false, "upload rejected"};
    FtpService service(testConfig(), &uploader);

    QVERIFY(!service.uploadSelectedFile(modPath));

    QCOMPARE(uploader.callCount, 1);
    QVERIFY(service.selectedLocalPath().isEmpty());
    QVERIFY(service.uploadedModFilename().isEmpty());
    QVERIFY(!service.uploadSucceeded());
}

void FtpServiceTests::missingFileFailsBeforeNetwork()
{
    FakeFtpUploader uploader;
    FtpService service(testConfig(), &uploader);

    QVERIFY(!service.uploadSelectedFile("D:/missing/path/not_here.mod"));

    QCOMPARE(uploader.callCount, 0);
    QVERIFY(!service.uploadSucceeded());
}

void FtpServiceTests::nonModFileFailsBeforeNetwork()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString txtPath = createFile(dir, "path.txt");

    FakeFtpUploader uploader;
    FtpService service(testConfig(), &uploader);

    QVERIFY(!service.uploadSelectedFile(txtPath));

    QCOMPARE(uploader.callCount, 0);
    QVERIFY(!service.uploadSucceeded());
}

void FtpServiceTests::defaultDirectoryUsesModFilesBesideApplicationWhenPresent()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString modDir = QDir(appDir).filePath("mod_files");
    QDir().mkpath(modDir);

    QCOMPARE(QDir::cleanPath(FtpService::defaultModDirectory()),
             QDir::cleanPath(modDir));
}

QTEST_MAIN(FtpServiceTests)
#include "test_ftp_service.moc"
```

- [ ] **Step 2: Add the service header**

Create `src/services/ftp/FtpService.h`:

```cpp
#pragma once

#include "app/AppConfig.h"
#include "ftp/FtpClient.h"

#include <QObject>
#include <QString>
#include <memory>

// FtpService is the Qt-facing migration of the Python ftp_service.py workflow.
// It owns the selected MOD-file state that a later robot-print slice will read.
class FtpService final : public QObject {
    Q_OBJECT

public:
    explicit FtpService(FtpConfig config, QObject* parent = nullptr);
    FtpService(FtpConfig config, IFtpUploader* uploader, QObject* parent = nullptr);

    QString selectedLocalPath() const;
    QString uploadedModFilename() const;
    bool uploadSucceeded() const;

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
```

- [ ] **Step 3: Add the service implementation**

Create `src/services/ftp/FtpService.cpp`:

```cpp
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
      uploader_(ownedUploader_.get())
{
}

FtpService::FtpService(FtpConfig config, IFtpUploader* uploader, QObject* parent)
    : QObject(parent),
      config_(std::move(config)),
      uploader_(uploader)
{
}

QString FtpService::selectedLocalPath() const { return selectedLocalPath_; }
QString FtpService::uploadedModFilename() const { return uploadedModFilename_; }
bool FtpService::uploadSucceeded() const { return uploadSucceeded_; }

QString FtpService::defaultModDirectory()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString modDir = QDir(appDir).filePath("mod_files");
    return QDir(modDir).exists() ? modDir : appDir;
}

void FtpService::openLocus()
{
    const QString selectedPath = QFileDialog::getOpenFileName(
        nullptr,
        QStringLiteral("Select MOD path file"),
        defaultModDirectory(),
        QStringLiteral("MOD files (*.mod)"));

    if (selectedPath.isEmpty()) {
        emit messageRequested(QStringLiteral("FTP Upload"),
                              QStringLiteral("No print path was selected; please select again."));
        return;
    }

    uploadSelectedFile(selectedPath);
}

bool FtpService::uploadSelectedFile(const QString& localFilePath)
{
    if (!validateSelectedFile(localFilePath)) {
        clearUploadState();
        return false;
    }

    if (uploader_ == nullptr) {
        clearUploadState();
        emit messageRequested(QStringLiteral("FTP Upload"),
                              QStringLiteral("FTP uploader is not available."));
        return false;
    }

    const QFileInfo fileInfo(localFilePath);
    const QString remoteFilename = fileInfo.fileName();

    FtpUploadRequest request;
    request.host = config_.ip;
    request.port = config_.port;
    request.user = config_.user;
    request.password = config_.password;
    request.localFilePath = localFilePath;
    request.remoteFilename = remoteFilename;

    emit messageRequested(QStringLiteral("FTP Upload"),
                          QStringLiteral("Uploading file to FTP server..."));

    const FtpUploadResult result = uploader_->uploadFile(request);
    if (!result.success) {
        clearUploadState();
        emit messageRequested(QStringLiteral("FTP Upload"), result.message);
        return false;
    }

    setUploadState(localFilePath, remoteFilename);
    emit messageRequested(QStringLiteral("FTP Upload"),
                          QStringLiteral("Path file uploaded successfully: %1").arg(remoteFilename));
    return true;
}

bool FtpService::validateSelectedFile(const QString& localFilePath)
{
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
    if (config_.ip.trimmed().isEmpty() || config_.port <= 0) {
        emit messageRequested(
            QStringLiteral("FTP Upload"),
            QStringLiteral("FTP configuration is invalid."));
        return false;
    }
    return true;
}

void FtpService::clearUploadState()
{
    selectedLocalPath_.clear();
    uploadedModFilename_.clear();
    uploadSucceeded_ = false;
}

void FtpService::setUploadState(const QString& localFilePath, const QString& remoteFilename)
{
    selectedLocalPath_ = localFilePath;
    uploadedModFilename_ = remoteFilename;
    uploadSucceeded_ = true;
}
```

- [ ] **Step 4: Add the test target**

In `CMakeLists.txt`, add a test block after the pressure or PLC tests:

```cmake
set(TEST_FTP_SERVICE_SOURCES
    tests/test_ftp_service.cpp
    src/services/ftp/FtpService.cpp
    src/ftp/FtpClient.cpp
)
set(TEST_FTP_SERVICE_READY TRUE)
set(MISSING_TEST_FTP_SERVICE_SOURCES)
foreach(TEST_SOURCE ${TEST_FTP_SERVICE_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        set(TEST_FTP_SERVICE_READY FALSE)
        list(APPEND MISSING_TEST_FTP_SERVICE_SOURCES ${TEST_SOURCE})
    endif()
endforeach()

if(TEST_FTP_SERVICE_READY)
    add_executable(test_ftp_service ${TEST_FTP_SERVICE_SOURCES})
    target_include_directories(test_ftp_service PRIVATE src)
    target_link_libraries(test_ftp_service PRIVATE Qt6::Core Qt6::Widgets Qt6::Test wininet)
    copy_qt_test_runtime(test_ftp_service)
    add_test(NAME test_ftp_service COMMAND test_ftp_service)
elseif(MISSING_TEST_FTP_SERVICE_SOURCES)
    string(REPLACE ";" "\n  " MISSING_TEST_FTP_SERVICE_TEXT "${MISSING_TEST_FTP_SERVICE_SOURCES}")
    message(STATUS "test_ftp_service target deferred. Missing files:\n  ${MISSING_TEST_FTP_SERVICE_TEXT}")
endif()
```

- [ ] **Step 5: Run the new test and fix only compile/test issues**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64;C:\vcpkg\installed\x64-windows" -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Release --target test_ftp_service
ctest --test-dir build -C Release -R test_ftp_service --output-on-failure
```

Expected: `test_ftp_service` passes. The success test asserts state and only checks that at least one user-facing message was emitted, because the service may emit both an upload-start message and a success message.

- [ ] **Step 6: Commit the FTP service**

```powershell
git add CMakeLists.txt tests\test_ftp_service.cpp src\services\ftp\FtpService.h src\services\ftp\FtpService.cpp
git commit -m "feat: add ftp upload service"
```

## Task 3: Replace the FTP Placeholder in MainWindow

**Files:**
- Modify: `src/services/PlaceholderServices.h`
- Modify: `src/services/PlaceholderServices.cpp`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Remove the FTP placeholder declaration**

In `src/services/PlaceholderServices.h`, delete only this class:

```cpp
class FtpService final : public PlaceholderService {
    Q_OBJECT
public:
    explicit FtpService(QObject* parent = nullptr);
public slots:
    void openLocus();
};
```

Keep the serial, robot, camera, and thermal placeholders unchanged.

- [ ] **Step 2: Remove the FTP placeholder implementation**

In `src/services/PlaceholderServices.cpp`, delete only the FTP constructor and slot implementation:

```cpp
FtpService::FtpService(QObject* parent)
    : PlaceholderService(QStringLiteral("FTP"), parent)
{
}

void FtpService::openLocus()
{
    reportNotConverted(QStringLiteral("Select and upload MOD path file"));
}
```

- [ ] **Step 3: Include the real FTP service in MainWindow**

In `src/ui/MainWindow.h`, add:

```cpp
#include "services/ftp/FtpService.h"
```

Keep the existing member name:

```cpp
FtpService ftpService_;
```

- [ ] **Step 4: Construct and connect the real service**

In `src/ui/MainWindow.cpp`, update the constructor initializer for `ftpService_` from:

```cpp
ftpService_(this),
```

to:

```cpp
ftpService_(config_.ftp, this),
```

In `MainWindow::connectSignals()`, remove:

```cpp
connectPlaceholder(ftpService_);
```

Add the real message connection near the PLC and pressure service message connections:

```cpp
connect(&ftpService_, &FtpService::messageRequested,
        this, &MainWindow::showPlaceholderMessage);
```

Keep the existing button connection:

```cpp
connect(ui_->locusselectsetButton, &QPushButton::clicked,
        &ftpService_, &FtpService::openLocus);
```

- [ ] **Step 5: Make the application target use the real FTP files**

In `CMakeLists.txt`, ensure `APP_SOURCES` includes:

```cmake
    src/ftp/FtpClient.h
    src/ftp/FtpClient.cpp
    src/services/ftp/FtpService.h
    src/services/ftp/FtpService.cpp
```

Ensure `target_link_libraries(AFP_ABB_CPP PRIVATE ...)` includes:

```cmake
        wininet
```

- [ ] **Step 6: Build the full app**

Run:

```powershell
cmake --build build --config Release --target AFP_ABB_CPP
```

Expected: `build\Release\AFP_ABB_CPP.exe` builds, and there is no duplicate `FtpService` symbol or include error.

- [ ] **Step 7: Commit the UI wiring**

```powershell
git add CMakeLists.txt src\services\PlaceholderServices.h src\services\PlaceholderServices.cpp src\ui\MainWindow.h src\ui\MainWindow.cpp
git commit -m "feat: wire ftp upload service"
```

## Task 4: Add the Portable `mod_files` Folder

**Files:**
- Create: `mod_files/.gitkeep`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create the source folder marker**

Create `mod_files/.gitkeep` as an empty file. This keeps the folder in git and gives operators a visible place for MOD path files in the portable package.

- [ ] **Step 2: Deploy the folder beside the executable**

In the `add_custom_command(TARGET AFP_ABB_CPP POST_BUILD ...)` block in `CMakeLists.txt`, add:

```cmake
        COMMAND ${CMAKE_COMMAND} -E make_directory
                $<TARGET_FILE_DIR:AFP_ABB_CPP>/mod_files
```

Place it after the existing `copy_directory` commands. The folder must be created even if it is empty.

- [ ] **Step 3: Build and check the deployed folder**

Run:

```powershell
cmake --build build --config Release --target AFP_ABB_CPP
Test-Path build\Release\mod_files
```

Expected: `Test-Path` prints `True`.

- [ ] **Step 4: Commit the folder deployment**

```powershell
git add CMakeLists.txt mod_files\.gitkeep
git commit -m "build: add portable mod files folder"
```

## Task 5: Document and Verify Slice D

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Add README FTP instructions**

Add a section to `README.md` near the existing configuration/runtime sections:

````markdown
## FTP MOD File Upload

The `locusselectsetButton` control uploads one selected `.mod` path file to the FTP server configured in `config/env_config.json`.

FTP configuration:

```json
"ftp": {
  "ip": "192.168.0.104",
  "port": 21,
  "user": "Default User",
  "password": "robotics"
}
```

The file picker starts in the portable `mod_files` folder beside `AFP_ABB_CPP.exe`. This replaces the old Python-only `pythonLIb/printpath` start folder. The folder is only a convenience location; operators can browse to any valid `.mod` file.

To test with a real controller or RobotStudio FTP server:

1. Confirm the FTP server IP, port, user, and password in `config/env_config.json`.
2. Put a `.mod` file in `mod_files` or browse to another `.mod` file.
3. Start `AFP_ABB_CPP.exe`.
4. Click the existing MOD path selection button.
5. Select the `.mod` file.
6. Confirm the upload success message.
7. Confirm the remote FTP directory contains the selected filename.
````

- [ ] **Step 2: Configure, build, and run all tests**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.9.1\msvc2022_64;C:\vcpkg\installed\x64-windows" -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Expected:

```text
100% tests passed
```

If unrelated hardware-dependent behavior prevents a test from running, record the exact failing test and reason in the final response.

- [ ] **Step 3: Deploy the portable folder**

Run:

```powershell
cmake --build build --config Release --target deploy_portable
```

Expected:

- `build\Release\AFP_ABB_CPP.exe` exists.
- `build\Release\config\env_config.json` exists.
- `build\Release\lib\sensors\pressure_sensor\pressure.dll` exists.
- `build\Release\mod_files` exists.

- [ ] **Step 4: Confirm the Python project is untouched**

Run from `D:\OneDrive\NWPU\4600_BACKUP_ALL_26Apr2026\AFP_ABB_Python`:

```powershell
git status --short
```

Expected: no changes caused by Slice D. If the Python folder is not a git repo, inspect file timestamps only if there is a specific concern; do not write anything there.

- [ ] **Step 5: Commit README and final verification updates**

```powershell
git add README.md
git commit -m "docs: document ftp mod upload"
```

## Self-Review Checklist

- [ ] The plan implements every Slice D design requirement, including the corrected `mod_files` start directory.
- [ ] No new runtime dependency is introduced beyond Windows WinINet.
- [ ] The original `.ui` design is not modified.
- [ ] FTP state is preserved for a future robot-print slice through `selectedLocalPath`, `uploadedModFilename`, and `uploadSucceeded`.
- [ ] Automated FTP service tests do not require a live FTP server.
- [ ] README explains the portable MOD folder and manual FTP smoke test.
- [ ] `AFP_ABB_Python` remains read-only.
