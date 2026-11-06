# AFP ABB C++ Slice H High-Speed Camera Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Convert the core Hikrobot/MVS high-speed camera workflow: enumerate, open, live preview, stop/close, and frame-rate/exposure/gain read-write.

**Architecture:** Add a small camera layer that hides the MVS SDK behind an interface, then replace the current high-speed camera placeholder with a Qt service that owns UI state and a preview worker. The SDK DLL is loaded dynamically so the application still starts without camera hardware or vendor runtime files.

**Tech Stack:** C++17, Qt6 Core/Gui/Widgets/Test, Hikrobot/MVS DLL via `QLibrary`, existing generated Qt UI `Interface4600_window.ui`, CMake/CTest.

---

## File Structure

- Create `src/camera/HighSpeedCameraTypes.h`
  - Value types shared by the driver, service, worker, and tests.
- Create `src/camera/IHighSpeedCameraClient.h`
  - Hardware-facing interface used by the service and fakes.
- Create `src/camera/HighSpeedCameraClient.h`
  - Dynamic MVS SDK wrapper declaration.
- Create `src/camera/HighSpeedCameraClient.cpp`
  - Dynamic DLL loading, required MVS function resolution, enumeration/open/grab/parameter operations.
- Create `src/services/camera/HighSpeedCameraService.h`
  - Qt service declaration for the existing UI buttons.
- Create `src/services/camera/HighSpeedCameraService.cpp`
  - UI behavior, worker ownership, validation, message emission, and preview label updates.
- Modify `src/services/PlaceholderServices.h`
  - Remove the placeholder `HighSpeedCameraService` class.
- Modify `src/services/PlaceholderServices.cpp`
  - Remove the placeholder high-speed camera methods.
- Modify `src/ui/MainWindow.h`
  - Include and store the real `HighSpeedCameraService`.
- Modify `src/ui/MainWindow.cpp`
  - Construct the real service with config, paths, and UI pointer; remove high-speed camera placeholder wiring.
- Modify `src/app/AppConfig.h`
  - Add `HighSpeedCameraConfig`.
- Modify `src/app/AppConfig.cpp`
  - Parse high-speed camera config fields.
- Modify `config/env_config.json`
  - Add defaults for portable DLL path, installed DLL path, and frame timeout.
- Modify `CMakeLists.txt`
  - Add camera files to the app target and add test targets.
- Create `tests/test_high_speed_camera_config.cpp`
  - Focused AppConfig coverage for camera settings, or extend `tests/test_app_config.cpp` if the change is tiny.
- Create `tests/test_high_speed_camera_client.cpp`
  - Hardware-free tests for missing DLL and static display helpers.
- Create `tests/test_high_speed_camera_service.cpp`
  - Hardware-free UI service tests using a fake camera client.
- Modify `README.md`
  - Document Slice H behavior, camera DLL deployment, and manual hardware verification.

## MVS Function Surface

The implementation should resolve only these SDK functions because they are the functions used by the original Python workflow or needed to avoid OpenCV for preview:

```cpp
MV_CC_EnumDevices
MV_CC_CreateHandle
MV_CC_DestroyHandle
MV_CC_OpenDevice
MV_CC_CloseDevice
MV_CC_StartGrabbing
MV_CC_StopGrabbing
MV_CC_GetOptimalPacketSize
MV_CC_GetIntValue
MV_CC_GetFloatValue
MV_CC_SetIntValue
MV_CC_SetFloatValue
MV_CC_SetEnumValue
MV_CC_GetImageForRGB
```

Use `MV_CC_GetImageForRGB` first so live preview does not need OpenCV. If the resolved DLL does not expose it, return a clear error: `MvCameraControl.dll does not expose MV_CC_GetImageForRGB; live preview cannot start without RGB frame support.` A local Bayer conversion helper can be added in a separate small follow-up if this real SDK version requires it.

---

### Task 1: Add High-Speed Camera Config

**Files:**
- Modify: `src/app/AppConfig.h`
- Modify: `src/app/AppConfig.cpp`
- Modify: `config/env_config.json`
- Modify: `tests/test_app_config.cpp`

- [ ] **Step 1: Write failing config tests**

Add two private slots to `tests/test_app_config.cpp`:

```cpp
void loadsHighSpeedCameraConfig();
void invalidHighSpeedCameraConfigFallsBackToDefaults();
```

Add these test bodies before `missingConfigUsesDefaults()`:

```cpp
void TestAppConfig::loadsHighSpeedCameraConfig() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "high_speed_camera": {
            "sdk_dll_path": "D:/Camera/MvCameraControl.dll",
            "portable_sdk_dll_path": "lib/sensors/high_speed_camera/custom.dll",
            "installed_sdk_dll_path": "C:/MVS/MvCameraControl.dll",
            "frame_timeout_ms": 250
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.highSpeedCamera.sdkDllPath, QString("D:/Camera/MvCameraControl.dll"));
    QCOMPARE(config.highSpeedCamera.portableSdkDllPath, QString("lib/sensors/high_speed_camera/custom.dll"));
    QCOMPARE(config.highSpeedCamera.installedSdkDllPath, QString("C:/MVS/MvCameraControl.dll"));
    QCOMPARE(config.highSpeedCamera.frameTimeoutMs, 250);
}

void TestAppConfig::invalidHighSpeedCameraConfigFallsBackToDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "high_speed_camera": {
            "sdk_dll_path": "",
            "portable_sdk_dll_path": "",
            "installed_sdk_dll_path": "",
            "frame_timeout_ms": 0
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.highSpeedCamera.sdkDllPath, QString(""));
    QCOMPARE(config.highSpeedCamera.portableSdkDllPath, QString("lib/sensors/high_speed_camera/MvCameraControl.dll"));
    QCOMPARE(config.highSpeedCamera.installedSdkDllPath, QString("C:/Program Files (x86)/Common Files/MVS/Runtime/Win64_x64/MvCameraControl.dll"));
    QCOMPARE(config.highSpeedCamera.frameTimeoutMs, 1000);
    const QString warnings = config.warnings().join('\n');
    QVERIFY(warnings.contains("high_speed_camera.portable_sdk_dll_path"));
    QVERIFY(warnings.contains("high_speed_camera.installed_sdk_dll_path"));
    QVERIFY(warnings.contains("high_speed_camera.frame_timeout_ms"));
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake --build build --config Release --target test_app_config
ctest --test-dir build -C Release -R test_app_config --output-on-failure
```

Expected: compile failure because `AppConfig` has no `highSpeedCamera` member.

- [ ] **Step 3: Add config types**

Add this struct to `src/app/AppConfig.h` after `ThermalCameraConfig`:

```cpp
struct HighSpeedCameraConfig {
    // Empty means "try the portable path, then the installed MVS runtime path";
    // a non-empty value is tried between those two defaults.
    QString sdkDllPath;

    // Relative to the application directory. This supports portable releases.
    QString portableSdkDllPath = "lib/sensors/high_speed_camera/MvCameraControl.dll";

    // Fallback used on development PCs where the MVS runtime is installed.
    QString installedSdkDllPath =
        "C:/Program Files (x86)/Common Files/MVS/Runtime/Win64_x64/MvCameraControl.dll";

    // Matches highspeed_camera_service.py MV_CC_GetOneFrameTimeout(..., 1000).
    int frameTimeoutMs = 1000;
};
```

Add this member to `class AppConfig` after `thermalCamera`:

```cpp
HighSpeedCameraConfig highSpeedCamera;
```

- [ ] **Step 4: Parse config**

In `src/app/AppConfig.cpp`, after the thermal camera block, add:

```cpp
const QJsonObject highSpeedCamera = root.value("high_speed_camera").toObject();
config.highSpeedCamera.sdkDllPath = readString(
    highSpeedCamera, "sdk_dll_path", config.highSpeedCamera.sdkDllPath).trimmed();
config.highSpeedCamera.portableSdkDllPath = readNonEmptyString(
    config,
    highSpeedCamera,
    "portable_sdk_dll_path",
    config.highSpeedCamera.portableSdkDllPath,
    "high_speed_camera.portable_sdk_dll_path");
config.highSpeedCamera.installedSdkDllPath = readNonEmptyString(
    config,
    highSpeedCamera,
    "installed_sdk_dll_path",
    config.highSpeedCamera.installedSdkDllPath,
    "high_speed_camera.installed_sdk_dll_path");
config.highSpeedCamera.frameTimeoutMs = readPositiveInt(
    config,
    highSpeedCamera,
    "frame_timeout_ms",
    config.highSpeedCamera.frameTimeoutMs,
    "high_speed_camera.frame_timeout_ms");
```

- [ ] **Step 5: Update default config file**

Add this section to `config/env_config.json` after `thermal_camera`:

```json
"high_speed_camera": {
    "_source_note": "Defaults match Python highspeed_camera_service.py and prefer a portable MVS SDK DLL beside the executable.",
    "sdk_dll_path": "",
    "portable_sdk_dll_path": "lib/sensors/high_speed_camera/MvCameraControl.dll",
    "installed_sdk_dll_path": "C:/Program Files (x86)/Common Files/MVS/Runtime/Win64_x64/MvCameraControl.dll",
    "frame_timeout_ms": 1000
},
```

- [ ] **Step 6: Run test to verify it passes**

Run:

```powershell
cmake --build build --config Release --target test_app_config
ctest --test-dir build -C Release -R test_app_config --output-on-failure
```

Expected: `test_app_config` passes.

- [ ] **Step 7: Commit**

```powershell
git add src/app/AppConfig.h src/app/AppConfig.cpp config/env_config.json tests/test_app_config.cpp
git commit -m "feat: add high-speed camera config"
```

---

### Task 2: Add Camera Value Types And Driver Interface

**Files:**
- Create: `src/camera/HighSpeedCameraTypes.h`
- Create: `src/camera/IHighSpeedCameraClient.h`
- Create: `tests/test_high_speed_camera_client.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing type tests**

Create `tests/test_high_speed_camera_client.cpp`:

```cpp
#include <QtTest/QtTest>

#include "camera/HighSpeedCameraTypes.h"

class TestHighSpeedCameraClient final : public QObject {
    Q_OBJECT

private slots:
    void deviceDisplayTextMatchesPythonStyle();
    void failureCodeFormatsAsHex();
};

void TestHighSpeedCameraClient::deviceDisplayTextMatchesPythonStyle() {
    HighSpeedCameraDevice gige;
    gige.index = 2;
    gige.transport = HighSpeedCameraTransport::GigE;
    gige.userName = "CamA";
    gige.endpoint = "192.168.0.88";

    HighSpeedCameraDevice usb;
    usb.index = 3;
    usb.transport = HighSpeedCameraTransport::Usb;
    usb.userName = "CamB";
    usb.endpoint = "ABC123";

    QCOMPARE(gige.displayText(), QString("[2]GigE: CamA(192.168.0.88)"));
    QCOMPARE(usb.displayText(), QString("[3]USB: CamB(ABC123)"));
}

void TestHighSpeedCameraClient::failureCodeFormatsAsHex() {
    QCOMPARE(highSpeedCameraErrorHex(0), QString("0x00000000"));
    QCOMPARE(highSpeedCameraErrorHex(-1), QString("0xFFFFFFFF"));
    QCOMPARE(highSpeedCameraErrorHex(0x80000001), QString("0x80000001"));
}

QTEST_MAIN(TestHighSpeedCameraClient)
#include "test_high_speed_camera_client.moc"
```

Add this test target to `CMakeLists.txt` near the other hardware-free targets:

```cmake
add_executable(test_high_speed_camera_client
    tests/test_high_speed_camera_client.cpp
)
target_include_directories(test_high_speed_camera_client PRIVATE src)
target_link_libraries(test_high_speed_camera_client PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
copy_qt_test_runtime(test_high_speed_camera_client)
add_custom_command(TARGET test_high_speed_camera_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Gui>
            $<TARGET_FILE_DIR:test_high_speed_camera_client>
)
add_test(NAME test_high_speed_camera_client COMMAND test_high_speed_camera_client)
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_high_speed_camera_client
```

Expected: compile failure because `camera/HighSpeedCameraTypes.h` does not exist.

- [ ] **Step 3: Add value types**

Create `src/camera/HighSpeedCameraTypes.h`:

```cpp
#pragma once

#include <QImage>
#include <QString>

#include <cstdint>

enum class HighSpeedCameraTransport {
    GigE,
    Usb,
    Unknown,
};

struct HighSpeedCameraDevice {
    int index = -1;
    HighSpeedCameraTransport transport = HighSpeedCameraTransport::Unknown;
    QString userName;
    QString endpoint;

    QString displayText() const {
        const QString transportName = transport == HighSpeedCameraTransport::GigE
            ? "GigE"
            : (transport == HighSpeedCameraTransport::Usb ? "USB" : "Unknown");
        return QString("[%1]%2: %3(%4)").arg(index).arg(transportName, userName, endpoint);
    }
};

struct HighSpeedCameraParameters {
    double frameRate = 0.0;
    double exposureTime = 0.0;
    double gain = 0.0;
};

struct HighSpeedCameraFrame {
    QImage image;
};

inline QString highSpeedCameraErrorHex(int code) {
    const quint32 unsignedCode = static_cast<quint32>(code);
    return QString("0x%1").arg(unsignedCode, 8, 16, QLatin1Char('0')).toUpper();
}
```

- [ ] **Step 4: Add driver interface**

Create `src/camera/IHighSpeedCameraClient.h`:

```cpp
#pragma once

#include "camera/HighSpeedCameraTypes.h"

#include <QString>
#include <QStringList>
#include <QVector>

class IHighSpeedCameraClient {
public:
    virtual ~IHighSpeedCameraClient() = default;

    virtual bool load() = 0;
    virtual bool loaded() const = 0;
    virtual QStringList searchedDllPaths() const = 0;
    virtual QVector<HighSpeedCameraDevice> enumerateDevices() = 0;
    virtual bool open(int deviceIndex) = 0;
    virtual bool isOpen() const = 0;
    virtual bool startGrabbing() = 0;
    virtual bool readFrame(HighSpeedCameraFrame& frame, int timeoutMs) = 0;
    virtual void stopGrabbing() = 0;
    virtual void close() = 0;
    virtual bool readParameters(HighSpeedCameraParameters& parameters) = 0;
    virtual bool writeParameters(const HighSpeedCameraParameters& parameters) = 0;
    virtual QString lastError() const = 0;
};
```

- [ ] **Step 5: Run test to verify it passes**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_high_speed_camera_client
ctest --test-dir build -C Release -R test_high_speed_camera_client --output-on-failure
```

Expected: `test_high_speed_camera_client` passes.

- [ ] **Step 6: Commit**

```powershell
git add CMakeLists.txt src/camera/HighSpeedCameraTypes.h src/camera/IHighSpeedCameraClient.h tests/test_high_speed_camera_client.cpp
git commit -m "feat: add high-speed camera driver interface"
```

---

### Task 3: Add Dynamic MVS Camera Client

**Files:**
- Create: `src/camera/HighSpeedCameraClient.h`
- Create: `src/camera/HighSpeedCameraClient.cpp`
- Modify: `tests/test_high_speed_camera_client.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Add failing missing-DLL test**

Extend `tests/test_high_speed_camera_client.cpp`:

```cpp
#include <QTemporaryDir>

#include "camera/HighSpeedCameraClient.h"
```

Add a private slot:

```cpp
void missingDllReportsSearchedPaths();
```

Add the test body:

```cpp
void TestHighSpeedCameraClient::missingDllReportsSearchedPaths() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    HighSpeedCameraConfig config;
    config.sdkDllPath = tempDir.filePath("explicit/MvCameraControl.dll");
    config.portableSdkDllPath = "portable/MvCameraControl.dll";
    config.installedSdkDllPath = tempDir.filePath("installed/MvCameraControl.dll");

    HighSpeedCameraClient client(config, tempDir.path());

    QVERIFY(!client.load());
    QVERIFY(!client.loaded());
    QVERIFY(client.lastError().contains("Could not load high-speed camera SDK DLL"));
    QVERIFY(client.searchedDllPaths().join('\n').contains("explicit"));
    QVERIFY(client.searchedDllPaths().join('\n').contains("portable"));
    QVERIFY(client.searchedDllPaths().join('\n').contains("installed"));
}
```

Update the test target in `CMakeLists.txt`:

```cmake
add_executable(test_high_speed_camera_client
    tests/test_high_speed_camera_client.cpp
    src/camera/HighSpeedCameraClient.cpp
)
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_high_speed_camera_client
```

Expected: compile failure because `HighSpeedCameraClient` does not exist.

- [ ] **Step 3: Add client declaration**

Create `src/camera/HighSpeedCameraClient.h`:

```cpp
#pragma once

#include "app/AppConfig.h"
#include "camera/IHighSpeedCameraClient.h"

#include <QLibrary>
#include <QVector>

// Dynamic wrapper around the Hikrobot/MVS SDK. It keeps vendor DLL loading and
// SDK return-code handling out of the Qt UI service.
class HighSpeedCameraClient final : public IHighSpeedCameraClient {
public:
    HighSpeedCameraClient(HighSpeedCameraConfig config, QString appDirPath);
    ~HighSpeedCameraClient() override;

    bool load() override;
    bool loaded() const override;
    QStringList searchedDllPaths() const override;
    QVector<HighSpeedCameraDevice> enumerateDevices() override;
    bool open(int deviceIndex) override;
    bool isOpen() const override;
    bool startGrabbing() override;
    bool readFrame(HighSpeedCameraFrame& frame, int timeoutMs) override;
    void stopGrabbing() override;
    void close() override;
    bool readParameters(HighSpeedCameraParameters& parameters) override;
    bool writeParameters(const HighSpeedCameraParameters& parameters) override;
    QString lastError() const override;

private:
    void buildDllSearchList();
    bool loadFromPath(const QString& path);
    bool resolveFunctions();
    void setError(const QString& message);

    HighSpeedCameraConfig config_;
    QString appDirPath_;
    QStringList searchedDllPaths_;
    QLibrary library_;
    bool open_ = false;
    bool grabbing_ = false;
    QString lastError_;
};
```

- [ ] **Step 4: Add minimal client implementation for DLL loading**

Create `src/camera/HighSpeedCameraClient.cpp`:

```cpp
#include "camera/HighSpeedCameraClient.h"

#include <QDir>
#include <QFileInfo>

#if defined(Q_OS_WIN)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <utility>

HighSpeedCameraClient::HighSpeedCameraClient(HighSpeedCameraConfig config, QString appDirPath)
    : config_(std::move(config)), appDirPath_(std::move(appDirPath)) {
    buildDllSearchList();
}

HighSpeedCameraClient::~HighSpeedCameraClient() {
    close();
}

bool HighSpeedCameraClient::load() {
    if (library_.isLoaded()) {
        return true;
    }

    for (const QString& path : searchedDllPaths_) {
        if (QFileInfo::exists(path) && loadFromPath(path)) {
            return true;
        }
    }

    setError(QString("Could not load high-speed camera SDK DLL. Searched paths:\n%1")
                 .arg(searchedDllPaths_.join('\n')));
    return false;
}

bool HighSpeedCameraClient::loaded() const {
    return library_.isLoaded();
}

QStringList HighSpeedCameraClient::searchedDllPaths() const {
    return searchedDllPaths_;
}

QVector<HighSpeedCameraDevice> HighSpeedCameraClient::enumerateDevices() {
    setError("Camera enumeration is not available until MVS functions are resolved.");
    return {};
}

bool HighSpeedCameraClient::open(int) {
    setError("Camera open is not available until MVS functions are resolved.");
    return false;
}

bool HighSpeedCameraClient::isOpen() const {
    return open_;
}

bool HighSpeedCameraClient::startGrabbing() {
    setError("Camera grabbing is not available until MVS functions are resolved.");
    return false;
}

bool HighSpeedCameraClient::readFrame(HighSpeedCameraFrame&, int) {
    setError("Camera frame reading is not available until MVS functions are resolved.");
    return false;
}

void HighSpeedCameraClient::stopGrabbing() {
    grabbing_ = false;
}

void HighSpeedCameraClient::close() {
    stopGrabbing();
    open_ = false;
}

bool HighSpeedCameraClient::readParameters(HighSpeedCameraParameters&) {
    setError("Camera parameter reading is not available until MVS functions are resolved.");
    return false;
}

bool HighSpeedCameraClient::writeParameters(const HighSpeedCameraParameters&) {
    setError("Camera parameter writing is not available until MVS functions are resolved.");
    return false;
}

QString HighSpeedCameraClient::lastError() const {
    return lastError_;
}

void HighSpeedCameraClient::buildDllSearchList() {
    searchedDllPaths_.clear();
    searchedDllPaths_ << QDir(appDirPath_).filePath(config_.portableSdkDllPath);
    if (!config_.sdkDllPath.trimmed().isEmpty()) {
        searchedDllPaths_ << QDir::cleanPath(config_.sdkDllPath.trimmed());
    }
    searchedDllPaths_ << config_.installedSdkDllPath;
    searchedDllPaths_.removeDuplicates();
}

bool HighSpeedCameraClient::loadFromPath(const QString& path) {
#if defined(Q_OS_WIN)
    const QString dllDirectory = QFileInfo(path).absoluteDir().absolutePath();
    const std::wstring nativeDirectory = QDir::toNativeSeparators(dllDirectory).toStdWString();
    if (!SetDllDirectoryW(nativeDirectory.c_str())) {
        setError(QString("Could not add high-speed camera DLL directory to the Windows search path: %1")
                     .arg(dllDirectory));
        return false;
    }
#endif

    library_.setFileName(path);
    if (!library_.load()) {
        setError(QString("Could not load high-speed camera SDK DLL '%1': %2").arg(path, library_.errorString()));
        return false;
    }
    return resolveFunctions();
}

bool HighSpeedCameraClient::resolveFunctions() {
    return true;
}

void HighSpeedCameraClient::setError(const QString& message) {
    lastError_ = message;
}
```

- [ ] **Step 5: Run missing-DLL test**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_high_speed_camera_client
ctest --test-dir build -C Release -R test_high_speed_camera_client --output-on-failure
```

Expected: test passes.

- [ ] **Step 6: Implement MVS function resolution and operations**

Replace the stub operation bodies in `src/camera/HighSpeedCameraClient.cpp` with a minimal dynamic MVS implementation. Keep all MVS-only structs and function pointer typedefs inside the `.cpp` file. Do not expose vendor structs through headers.

Use comments like this above the local structs:

```cpp
// These local declarations mirror only the MVS SDK fields used by the original
// Python ctypes wrapper. Keeping them private avoids leaking vendor SDK details
// into the service layer.
```

The implementation must:

- Resolve every function listed in the "MVS Function Surface" section.
- Return a missing-export error if `MV_CC_GetImageForRGB` is not exported.
- Build GigE combo text as `[i]GigE: name(ip)`.
- Build USB combo text as `[i]USB: name(serial)`.
- Open selected devices in exclusive access mode.
- Set GigE packet size when `MV_CC_GetOptimalPacketSize` returns a positive value.
- Set `TriggerMode` to `0` after opening, matching Python continuous acquisition behavior.
- Read `AcquisitionFrameRate`, `ExposureTime`, and `Gain`.
- Write `AcquisitionFrameRate`, `ExposureTime`, and `Gain`.
- Convert RGB frame data to `QImage::Format_RGB888` and call `.copy()` before returning.

- [ ] **Step 7: Add optional bundled-DLL smoke test**

Add this test to `tests/test_high_speed_camera_client.cpp`:

```cpp
void bundledMvsDllLoadsWhenPresent();
```

Body:

```cpp
void TestHighSpeedCameraClient::bundledMvsDllLoadsWhenPresent() {
    const QString appDir = QCoreApplication::applicationDirPath();
    const QFileInfo dll(QDir(appDir).filePath("lib/sensors/high_speed_camera/MvCameraControl.dll"));
    if (!dll.exists()) {
        QSKIP("Bundled MVS runtime is not present in this build output.");
    }

    HighSpeedCameraConfig config;
    config.portableSdkDllPath = "lib/sensors/high_speed_camera/MvCameraControl.dll";
    HighSpeedCameraClient client(config, appDir);

    QVERIFY2(client.load(), qPrintable(client.lastError()));
    QVERIFY(client.loaded());
}
```

Add includes:

```cpp
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
```

- [ ] **Step 8: Run client tests**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_high_speed_camera_client
ctest --test-dir build -C Release -R test_high_speed_camera_client --output-on-failure
```

Expected: tests pass. The bundled-DLL test may skip when the MVS runtime is absent.

- [ ] **Step 9: Commit**

```powershell
git add CMakeLists.txt src/camera/HighSpeedCameraClient.h src/camera/HighSpeedCameraClient.cpp tests/test_high_speed_camera_client.cpp
git commit -m "feat: add high-speed camera sdk client"
```

---

### Task 4: Add High-Speed Camera Service With Fake-Client Tests

**Files:**
- Create: `src/services/camera/HighSpeedCameraService.h`
- Create: `src/services/camera/HighSpeedCameraService.cpp`
- Create: `tests/test_high_speed_camera_service.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing service tests**

Create `tests/test_high_speed_camera_service.cpp`:

```cpp
#include <QtTest/QtTest>

#include <QSignalSpy>
#include <QWidget>

#include "camera/IHighSpeedCameraClient.h"
#include "services/camera/HighSpeedCameraService.h"
#include "ui_Interface4600_window.h"

class FakeHighSpeedCameraClient final : public IHighSpeedCameraClient {
public:
    bool loadResult = true;
    bool openResult = true;
    bool startResult = true;
    bool readParametersResult = true;
    bool writeParametersResult = true;
    bool loadedValue = true;
    bool openValue = false;
    QString error = "fake camera error";
    QVector<HighSpeedCameraDevice> devices;
    HighSpeedCameraParameters parameters;
    HighSpeedCameraParameters writtenParameters;
    int openedIndex = -1;
    int startCalls = 0;
    int stopCalls = 0;
    int closeCalls = 0;

    bool load() override { return loadResult; }
    bool loaded() const override { return loadedValue; }
    QStringList searchedDllPaths() const override { return {"fake/path/MvCameraControl.dll"}; }
    QVector<HighSpeedCameraDevice> enumerateDevices() override { return devices; }
    bool open(int deviceIndex) override {
        openedIndex = deviceIndex;
        openValue = openResult;
        return openResult;
    }
    bool isOpen() const override { return openValue; }
    bool startGrabbing() override {
        ++startCalls;
        return startResult;
    }
    bool readFrame(HighSpeedCameraFrame&, int) override { return false; }
    void stopGrabbing() override {
        ++stopCalls;
    }
    void close() override {
        ++closeCalls;
        openValue = false;
    }
    bool readParameters(HighSpeedCameraParameters& output) override {
        output = parameters;
        return readParametersResult;
    }
    bool writeParameters(const HighSpeedCameraParameters& input) override {
        writtenParameters = input;
        return writeParametersResult;
    }
    QString lastError() const override { return error; }
};

class TestHighSpeedCameraService final : public QObject {
    Q_OBJECT

private slots:
    void enumerateDevicesPopulatesComboBox();
    void openCameraRequiresSelection();
    void openCameraUsesSelectedDeviceIndex();
    void getParameterWritesUiFields();
    void setParameterRejectsInvalidInput();
    void setParameterWritesClientValues();
    void flawPredictReportsNotConverted();
    void stopCameraClosesClientAndClearsPreview();
};
```

Add helper and first tests:

```cpp
namespace {
HighSpeedCameraConfig testConfig() {
    HighSpeedCameraConfig config;
    config.frameTimeoutMs = 10;
    return config;
}
}

void TestHighSpeedCameraService::enumerateDevicesPopulatesComboBox() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    auto fake = std::make_unique<FakeHighSpeedCameraClient>();
    HighSpeedCameraDevice device;
    device.index = 1;
    device.transport = HighSpeedCameraTransport::Usb;
    device.userName = "USB Cam";
    device.endpoint = "SER123";
    fake->devices = {device};
    auto* fakeRaw = fake.get();
    HighSpeedCameraService service(testConfig(), &ui, std::move(fake));

    service.enumDevices();

    QCOMPARE(ui.camera_list->count(), 1);
    QCOMPARE(ui.camera_list->itemText(0), QString("[1]USB: USB Cam(SER123)"));
    QVERIFY(fakeRaw->loadedValue);
}

void TestHighSpeedCameraService::openCameraRequiresSelection() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    auto fake = std::make_unique<FakeHighSpeedCameraClient>();
    auto* fakeRaw = fake.get();
    HighSpeedCameraService service(testConfig(), &ui, std::move(fake));
    QSignalSpy messages(&service, &HighSpeedCameraService::messageRequested);

    service.openCamera();

    QCOMPARE(messages.count(), 1);
    QCOMPARE(fakeRaw->openedIndex, -1);
}

void TestHighSpeedCameraService::openCameraUsesSelectedDeviceIndex() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    auto fake = std::make_unique<FakeHighSpeedCameraClient>();
    HighSpeedCameraDevice device;
    device.index = 7;
    device.transport = HighSpeedCameraTransport::GigE;
    device.userName = "GigE Cam";
    device.endpoint = "192.168.0.99";
    fake->devices = {device};
    auto* fakeRaw = fake.get();
    HighSpeedCameraService service(testConfig(), &ui, std::move(fake));

    service.enumDevices();
    service.openCamera();

    QCOMPARE(fakeRaw->openedIndex, 7);
}
```

Add the remaining test bodies:

```cpp
void TestHighSpeedCameraService::getParameterWritesUiFields() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    auto fake = std::make_unique<FakeHighSpeedCameraClient>();
    fake->openValue = true;
    fake->parameters = {12.5, 3456.0, 7.5};
    HighSpeedCameraService service(testConfig(), &ui, std::move(fake));

    service.getParameter();

    QCOMPARE(ui.FrameRate->text(), QString("12.5"));
    QCOMPARE(ui.ExposureTime->text(), QString("3456"));
    QCOMPARE(ui.Gain->text(), QString("7.5"));
}

void TestHighSpeedCameraService::setParameterRejectsInvalidInput() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    auto fake = std::make_unique<FakeHighSpeedCameraClient>();
    fake->openValue = true;
    auto* fakeRaw = fake.get();
    HighSpeedCameraService service(testConfig(), &ui, std::move(fake));
    QSignalSpy messages(&service, &HighSpeedCameraService::messageRequested);
    ui.FrameRate->setText("fast");
    ui.ExposureTime->setText("100");
    ui.Gain->setText("2");

    service.setParameter();

    QCOMPARE(messages.count(), 1);
    QCOMPARE(fakeRaw->writtenParameters.exposureTime, 0.0);
}

void TestHighSpeedCameraService::setParameterWritesClientValues() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    auto fake = std::make_unique<FakeHighSpeedCameraClient>();
    fake->openValue = true;
    auto* fakeRaw = fake.get();
    HighSpeedCameraService service(testConfig(), &ui, std::move(fake));
    ui.FrameRate->setText("30");
    ui.ExposureTime->setText("4000");
    ui.Gain->setText("8");

    service.setParameter();

    QCOMPARE(fakeRaw->writtenParameters.frameRate, 30.0);
    QCOMPARE(fakeRaw->writtenParameters.exposureTime, 4000.0);
    QCOMPARE(fakeRaw->writtenParameters.gain, 8.0);
}

void TestHighSpeedCameraService::flawPredictReportsNotConverted() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    HighSpeedCameraService service(testConfig(), &ui, std::make_unique<FakeHighSpeedCameraClient>());
    QSignalSpy messages(&service, &HighSpeedCameraService::messageRequested);

    service.flawPredict();

    QCOMPARE(messages.count(), 1);
    QVERIFY(messages.takeFirst().at(1).toString().contains("not converted"));
}

void TestHighSpeedCameraService::stopCameraClosesClientAndClearsPreview() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    auto fake = std::make_unique<FakeHighSpeedCameraClient>();
    fake->openValue = true;
    auto* fakeRaw = fake.get();
    HighSpeedCameraService service(testConfig(), &ui, std::move(fake));
    ui.FrameRate->setText("30");
    ui.ExposureTime->setText("4000");
    ui.Gain->setText("8");
    ui.HC_RD->setText("preview");

    service.stopCamera();

    QCOMPARE(fakeRaw->stopCalls, 1);
    QCOMPARE(fakeRaw->closeCalls, 1);
    QVERIFY(ui.FrameRate->text().isEmpty());
    QVERIFY(ui.ExposureTime->text().isEmpty());
    QVERIFY(ui.Gain->text().isEmpty());
    QVERIFY(ui.HC_RD->text().isEmpty());
}
```

Add:

```cpp
QTEST_MAIN(TestHighSpeedCameraService)
#include "test_high_speed_camera_service.moc"
```

Add the test target to `CMakeLists.txt`:

```cmake
add_executable(test_high_speed_camera_service
    tests/test_high_speed_camera_service.cpp
    src/services/camera/HighSpeedCameraService.cpp
    src/ui/Interface4600_window.ui
)
target_include_directories(test_high_speed_camera_service PRIVATE
    src
    ${CMAKE_CURRENT_BINARY_DIR}/test_high_speed_camera_service_autogen/include
)
target_link_libraries(test_high_speed_camera_service PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
copy_qt_widget_test_runtime(test_high_speed_camera_service)
add_test(NAME test_high_speed_camera_service COMMAND test_high_speed_camera_service)
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_high_speed_camera_service
```

Expected: compile failure because `services/camera/HighSpeedCameraService.h` does not exist.

- [ ] **Step 3: Add service declaration**

Create `src/services/camera/HighSpeedCameraService.h`:

```cpp
#pragma once

#include "app/AppConfig.h"
#include "camera/IHighSpeedCameraClient.h"

#include <QObject>
#include <QThread>

#include <memory>

namespace Ui { class ContinuosfiberInterface; }
class QLineEdit;

class HighSpeedCameraFrameWorker;

class HighSpeedCameraService final : public QObject {
    Q_OBJECT

public:
    HighSpeedCameraService(const HighSpeedCameraConfig& config,
                           Ui::ContinuosfiberInterface* ui,
                           QObject* parent = nullptr);
    HighSpeedCameraService(const HighSpeedCameraConfig& config,
                           Ui::ContinuosfiberInterface* ui,
                           std::unique_ptr<IHighSpeedCameraClient> client,
                           QObject* parent = nullptr);
    ~HighSpeedCameraService() override;

public slots:
    void enumDevices();
    void openCamera();
    void flawPredict();
    void startGrabbing();
    void stopCamera();
    void getParameter();
    void setParameter();

signals:
    void messageRequested(const QString& title, const QString& message);

private slots:
    void displayFrame(const QImage& image);
    void handleWorkerError(const QString& message);

private:
    bool ensureLoaded();
    bool ensureOpen(const QString& action);
    bool readPositiveDouble(QLineEdit* edit, const QString& fieldName, double& value);
    void stopWorker();
    void clearUiAfterClose();
    void showMessage(const QString& title, const QString& message);

    HighSpeedCameraConfig config_;
    Ui::ContinuosfiberInterface* ui_ = nullptr;
    std::unique_ptr<IHighSpeedCameraClient> client_;
    QThread workerThread_;
    HighSpeedCameraFrameWorker* worker_ = nullptr;
};
```

- [ ] **Step 4: Add service implementation**

Create `src/services/camera/HighSpeedCameraService.cpp` with fully commented UI logic:

```cpp
#include "services/camera/HighSpeedCameraService.h"

#include "camera/HighSpeedCameraClient.h"
#include "ui_Interface4600_window.h"

#include <QApplication>
#include <QImage>
#include <QLineEdit>
#include <QPixmap>
#include <QTimer>

#include <utility>

class HighSpeedCameraFrameWorker final : public QObject {
    Q_OBJECT
public:
    HighSpeedCameraFrameWorker(IHighSpeedCameraClient& client, int timeoutMs)
        : client_(client), timeoutMs_(timeoutMs) {
    }

public slots:
    void run() {
        running_ = true;
        while (running_) {
            HighSpeedCameraFrame frame;
            if (!client_.readFrame(frame, timeoutMs_)) {
                emit error(client_.lastError());
                break;
            }
            if (!frame.image.isNull()) {
                emit frameReady(frame.image);
            }
        }
        emit finished();
    }

    void stop() {
        running_ = false;
    }

signals:
    void frameReady(const QImage& image);
    void error(const QString& message);
    void finished();

private:
    IHighSpeedCameraClient& client_;
    int timeoutMs_ = 1000;
    bool running_ = false;
};

HighSpeedCameraService::HighSpeedCameraService(const HighSpeedCameraConfig& config,
                                               Ui::ContinuosfiberInterface* ui,
                                               QObject* parent)
    : HighSpeedCameraService(
          config,
          ui,
          std::make_unique<HighSpeedCameraClient>(config, QApplication::applicationDirPath()),
          parent) {
}

HighSpeedCameraService::HighSpeedCameraService(const HighSpeedCameraConfig& config,
                                               Ui::ContinuosfiberInterface* ui,
                                               std::unique_ptr<IHighSpeedCameraClient> client,
                                               QObject* parent)
    : QObject(parent), config_(config), ui_(ui), client_(std::move(client)) {
}

HighSpeedCameraService::~HighSpeedCameraService() {
    stopCamera();
}

void HighSpeedCameraService::enumDevices() {
    if (!ensureLoaded()) {
        return;
    }

    ui_->camera_list->clear();
    const QVector<HighSpeedCameraDevice> devices = client_->enumerateDevices();
    if (devices.isEmpty()) {
        showMessage("High-speed camera", "No available camera devices found.");
        return;
    }

    for (const HighSpeedCameraDevice& device : devices) {
        ui_->camera_list->addItem(device.displayText(), device.index);
    }
}

void HighSpeedCameraService::openCamera() {
    if (!ensureLoaded()) {
        return;
    }
    if (ui_->camera_list->currentIndex() < 0) {
        showMessage("High-speed camera", "No enumerated camera is selected.");
        return;
    }

    const int deviceIndex = ui_->camera_list->currentData().toInt();
    if (!client_->open(deviceIndex)) {
        showMessage("High-speed camera", client_->lastError());
        return;
    }

    showMessage("High-speed camera", "Camera connected successfully.");
}

void HighSpeedCameraService::flawPredict() {
    showMessage("High-speed camera", "Flaw prediction is not converted in Slice H.");
}

void HighSpeedCameraService::startGrabbing() {
    if (!ensureOpen("start camera preview")) {
        return;
    }
    if (worker_) {
        showMessage("High-speed camera", "Camera preview is already running.");
        return;
    }
    if (!client_->startGrabbing()) {
        showMessage("High-speed camera", client_->lastError());
        return;
    }

    worker_ = new HighSpeedCameraFrameWorker(*client_, config_.frameTimeoutMs);
    worker_->moveToThread(&workerThread_);
    connect(&workerThread_, &QThread::started, worker_, &HighSpeedCameraFrameWorker::run);
    connect(worker_, &HighSpeedCameraFrameWorker::frameReady, this, &HighSpeedCameraService::displayFrame, Qt::QueuedConnection);
    connect(worker_, &HighSpeedCameraFrameWorker::error, this, &HighSpeedCameraService::handleWorkerError, Qt::QueuedConnection);
    connect(worker_, &HighSpeedCameraFrameWorker::finished, &workerThread_, &QThread::quit);
    connect(worker_, &HighSpeedCameraFrameWorker::finished, worker_, &QObject::deleteLater);
    connect(&workerThread_, &QThread::finished, this, [this]() { worker_ = nullptr; });
    workerThread_.start();
}

void HighSpeedCameraService::stopCamera() {
    stopWorker();
    client_->stopGrabbing();
    client_->close();
    clearUiAfterClose();
}

void HighSpeedCameraService::getParameter() {
    if (!ensureOpen("read camera parameters")) {
        return;
    }

    HighSpeedCameraParameters parameters;
    if (!client_->readParameters(parameters)) {
        showMessage("High-speed camera", client_->lastError());
        return;
    }

    ui_->FrameRate->setText(QString::number(parameters.frameRate));
    ui_->ExposureTime->setText(QString::number(parameters.exposureTime));
    ui_->Gain->setText(QString::number(parameters.gain));
}

void HighSpeedCameraService::setParameter() {
    if (!ensureOpen("write camera parameters")) {
        return;
    }

    HighSpeedCameraParameters parameters;
    if (!readPositiveDouble(ui_->FrameRate, "FrameRate", parameters.frameRate) ||
        !readPositiveDouble(ui_->ExposureTime, "ExposureTime", parameters.exposureTime) ||
        !readPositiveDouble(ui_->Gain, "Gain", parameters.gain)) {
        return;
    }

    if (!client_->writeParameters(parameters)) {
        showMessage("High-speed camera", client_->lastError());
    }
}

void HighSpeedCameraService::displayFrame(const QImage& image) {
    ui_->HC_RD->setScaledContents(true);
    ui_->HC_RD->setPixmap(QPixmap::fromImage(image));
}

void HighSpeedCameraService::handleWorkerError(const QString& message) {
    showMessage("High-speed camera", message);
    stopCamera();
}

bool HighSpeedCameraService::ensureLoaded() {
    if (client_->load()) {
        return true;
    }
    showMessage("High-speed camera", client_->lastError());
    return false;
}

bool HighSpeedCameraService::ensureOpen(const QString& action) {
    if (client_->isOpen()) {
        return true;
    }
    showMessage("High-speed camera", QString("Cannot %1 because the camera is not open.").arg(action));
    return false;
}

bool HighSpeedCameraService::readPositiveDouble(QLineEdit* edit, const QString& fieldName, double& value) {
    bool ok = false;
    value = edit->text().trimmed().toDouble(&ok);
    if (!ok || value <= 0.0) {
        showMessage("High-speed camera", QString("%1 must be a positive number.").arg(fieldName));
        return false;
    }
    return true;
}

void HighSpeedCameraService::stopWorker() {
    if (worker_) {
        QMetaObject::invokeMethod(worker_, "stop", Qt::QueuedConnection);
        workerThread_.quit();
        workerThread_.wait();
        worker_ = nullptr;
    }
}

void HighSpeedCameraService::clearUiAfterClose() {
    ui_->FrameRate->clear();
    ui_->ExposureTime->clear();
    ui_->Gain->clear();
    ui_->HC_RD->clear();
    ui_->HC_PData->clear();
}

void HighSpeedCameraService::showMessage(const QString& title, const QString& message) {
    emit messageRequested(title, message);
}

#include "HighSpeedCameraService.moc"
```

- [ ] **Step 5: Run service tests**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release --target test_high_speed_camera_service
ctest --test-dir build -C Release -R test_high_speed_camera_service --output-on-failure
```

Expected: `test_high_speed_camera_service` passes.

- [ ] **Step 6: Commit**

```powershell
git add CMakeLists.txt src/services/camera/HighSpeedCameraService.h src/services/camera/HighSpeedCameraService.cpp tests/test_high_speed_camera_service.cpp
git commit -m "feat: add high-speed camera service"
```

---

### Task 5: Wire Service Into Main Window And Remove Placeholder

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Modify: `src/services/PlaceholderServices.h`
- Modify: `src/services/PlaceholderServices.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Update app source list**

In `CMakeLists.txt`, add these files to `APP_SOURCES`:

```cmake
src/camera/HighSpeedCameraTypes.h
src/camera/IHighSpeedCameraClient.h
src/camera/HighSpeedCameraClient.h
src/camera/HighSpeedCameraClient.cpp
src/services/camera/HighSpeedCameraService.h
src/services/camera/HighSpeedCameraService.cpp
```

- [ ] **Step 2: Update `MainWindow.h` includes and member**

Replace:

```cpp
#include "services/PlaceholderServices.h"
```

with:

```cpp
#include "services/PlaceholderServices.h"
#include "services/camera/HighSpeedCameraService.h"
```

Keep the existing member name:

```cpp
HighSpeedCameraService highSpeedCameraService_;
```

- [ ] **Step 3: Update service construction**

In `MainWindow::MainWindow`, replace:

```cpp
highSpeedCameraService_(this),
```

with:

```cpp
highSpeedCameraService_(config_.highSpeedCamera, ui_, this),
```

- [ ] **Step 4: Remove placeholder connection**

In `MainWindow::connectSignals`, remove:

```cpp
connectPlaceholder(highSpeedCameraService_);
```

Add real message wiring near the other real services:

```cpp
connect(&highSpeedCameraService_, &HighSpeedCameraService::messageRequested,
        this, &MainWindow::showPlaceholderMessage);
```

- [ ] **Step 5: Remove placeholder class**

Delete this class from `src/services/PlaceholderServices.h`:

```cpp
class HighSpeedCameraService final : public PlaceholderService {
    Q_OBJECT
public:
    explicit HighSpeedCameraService(QObject* parent = nullptr);
public slots:
    void enumDevices();
    void openCamera();
    void flawPredict();
    void startGrabbing();
    void stopCamera();
    void getParameter();
    void setParameter();
};
```

Delete these definitions from `src/services/PlaceholderServices.cpp`:

```cpp
HighSpeedCameraService::HighSpeedCameraService(QObject* parent) : PlaceholderService("High-speed camera", parent) {}
void HighSpeedCameraService::enumDevices() { reportNotConverted("Camera enumeration"); }
void HighSpeedCameraService::openCamera() { reportNotConverted("Camera open"); }
void HighSpeedCameraService::flawPredict() { reportNotConverted("Flaw prediction"); }
void HighSpeedCameraService::startGrabbing() { reportNotConverted("Camera grabbing"); }
void HighSpeedCameraService::stopCamera() { reportNotConverted("Camera stop"); }
void HighSpeedCameraService::getParameter() { reportNotConverted("Camera parameter read"); }
void HighSpeedCameraService::setParameter() { reportNotConverted("Camera parameter write"); }
```

- [ ] **Step 6: Build app and tests**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Expected: the Release app and all tests pass.

- [ ] **Step 7: Commit**

```powershell
git add CMakeLists.txt src/ui/MainWindow.h src/ui/MainWindow.cpp src/services/PlaceholderServices.h src/services/PlaceholderServices.cpp
git commit -m "feat: wire high-speed camera service"
```

---

### Task 6: Update README And Portable Runtime Notes

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Update feature summary**

Add this paragraph after the Slice G paragraph:

```markdown
Slice H adds the core high-speed visible-light camera workflow using the Hikrobot/MVS SDK: camera enumeration, opening the selected camera, live preview in the existing `HC_RD` label, stop/close behavior, and frame-rate/exposure/gain read-write. It does not convert YOLO flaw prediction, video recording, abrasion/misalignment plotting, or DDS camera-frame publishing.
```

- [ ] **Step 2: Add camera runtime deployment section**

Add:

```markdown
## High-Speed Camera Runtime

Slice H loads the Hikrobot/MVS SDK DLL dynamically. The default search order is:

1. `lib\sensors\high_speed_camera\MvCameraControl.dll` beside the executable.
2. `high_speed_camera.sdk_dll_path` from `config\env_config.json`, when set.
3. `C:\Program Files (x86)\Common Files\MVS\Runtime\Win64_x64\MvCameraControl.dll`.

For a portable release, place the vendor runtime files allowed by the Hikrobot/MVS license under:

```text
build\Release\lib\sensors\high_speed_camera\
```

If the DLL is absent, the application still starts. The high-speed camera buttons show a clear missing-DLL message when used.
```

- [ ] **Step 3: Add manual hardware verification**

Add:

```markdown
## Manual High-Speed Camera Check

With a Hikrobot/MVS-compatible camera and runtime available:

1. Start `AFP_ABB_CPP.exe`.
2. Click `enum_cam` and confirm the camera appears in `camera_list`.
3. Click `HC_machine` and confirm the success message appears.
4. Click `get_parameter` and confirm `FrameRate`, `ExposureTime`, and `Gain` are populated.
5. Click `HC_start` and confirm live preview appears in `HC_RD`.
6. Click `HC_stop` and confirm preview and parameter fields are cleared.
```

- [ ] **Step 4: Run markdown and build verification**

Run:

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 5: Commit**

```powershell
git add README.md
git commit -m "docs: document high-speed camera workflow"
```

---

### Task 7: Final Verification

**Files:**
- Verify all changed files.

- [ ] **Step 1: Check repository status**

Run:

```powershell
git status --short --branch
```

Expected: only generated `build/` output is untracked or modified. No source/config/doc files should be unstaged.

- [ ] **Step 2: Configure**

Run:

```powershell
cmake -S . -B build
```

Expected: configure completes without missing-source messages for the app target or new tests.

- [ ] **Step 3: Build Release**

Run:

```powershell
cmake --build build --config Release
```

Expected: build completes successfully.

- [ ] **Step 4: Run all Release tests**

Run:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Expected: all tests pass. Camera hardware tests either pass when the MVS runtime is present or skip when it is absent.

- [ ] **Step 5: Verify Python folder was not changed**

Run:

```powershell
git -C ..\AFP_ABB_Python status --short
```

Expected: no tracked Python source changes. Pre-existing untracked generated files can remain untracked.

---

## Self-Review

- Spec coverage: Tasks cover config, dynamic SDK loading, camera enumeration/open/grab/parameter behavior, Qt service wiring, error messages, tests, README notes, and portable runtime documentation.
- Scope check: The plan does not implement YOLO flaw prediction, video recording, abrasion/misalignment plots, or DDS image streaming.
- Type consistency: The plan uses `HighSpeedCameraConfig`, `HighSpeedCameraDevice`, `HighSpeedCameraParameters`, `HighSpeedCameraFrame`, `IHighSpeedCameraClient`, `HighSpeedCameraClient`, and `HighSpeedCameraService` consistently across tasks.
