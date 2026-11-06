# Slice I Thermal Camera JPEG Cache Workflow Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Convert the Python thermal camera workflow to C++ using the approved JPEG cache pipeline while keeping the existing UI unchanged.

**Architecture:** Add a small thermal camera stack beside the existing high-speed camera stack: config values in `AppConfig`, filesystem cache handling in `ThermalFrameStore`, Hikvision SDK calls in `ThermalCameraClient`, UI behavior in `ThermalCameraService`, and main-window wiring in `MainWindow`. The SDK client is hidden behind `IThermalCameraClient` so tests can use fakes without camera hardware or vendor DLLs.

**Tech Stack:** C++17, Qt6 Core/Gui/Widgets/Test, dynamic DLL loading through `QLibrary`, existing CMake/CTest project structure, Hikvision `HCNetSDK.dll` and `PlayCtrl.dll` at runtime.

---

## File Structure

- Modify: `config/env_config.json`
  - Add thermal runtime/cache/display settings under `thermal_camera`.
- Modify: `src/app/AppConfig.h`
  - Extend `ThermalCameraConfig` with `sdkDir`, `portableSdkDir`, `cacheMaxFiles`, and `displayIntervalMs`.
- Modify: `src/app/AppConfig.cpp`
  - Parse and validate the new thermal camera config fields.
- Modify: `tests/test_app_config.cpp`
  - Cover valid and invalid thermal camera runtime/cache/display settings.
- Create: `src/thermal_camera/ThermalCameraTypes.h`
  - Define `ThermalCameraRuntimeConfig`-style helpers and SDK error formatting.
- Create: `src/thermal_camera/IThermalCameraClient.h`
  - Define the hardware-free client interface used by `ThermalCameraService`.
- Create: `src/thermal_camera/ThermalFrameStore.h`
  - Declare cache/saved/ROI directory and JPEG selection helper.
- Create: `src/thermal_camera/ThermalFrameStore.cpp`
  - Implement cache directory creation, clearing, pruning, newest-frame lookup, and ROI saving.
- Create: `src/thermal_camera/ThermalCameraClient.h`
  - Declare the Hikvision SDK wrapper.
- Create: `src/thermal_camera/ThermalCameraClient.cpp`
  - Implement dynamic DLL loading, SDK startup, login, preview callback, JPEG cache writing, and cleanup.
- Create: `src/services/thermal_camera/ThermalCameraService.h`
  - Declare the Qt UI service for `TIC_*` controls and ROI handling.
- Create: `src/services/thermal_camera/ThermalCameraService.cpp`
  - Implement thermal start/stop/cache/display/ROI UI behavior.
- Modify: `src/services/PlaceholderServices.h`
  - Remove the temporary `ThermalCameraService` declaration.
- Modify: `src/services/PlaceholderServices.cpp`
  - Remove the temporary thermal service methods.
- Modify: `src/ui/MainWindow.h`
  - Include the real thermal service and store it as a member.
- Modify: `src/ui/MainWindow.cpp`
  - Construct and connect the real thermal service, forward mouse presses, and stop capture on close.
- Modify: `CMakeLists.txt`
  - Add new production sources and new tests.
- Create: `tests/test_thermal_frame_store.cpp`
  - Test cache behavior without hardware.
- Create: `tests/test_thermal_camera_client.cpp`
  - Test missing runtime diagnostics without loading hardware.
- Create: `tests/test_thermal_camera_service.cpp`
  - Test UI behavior using fake client/store.
- Modify: `README.md`
  - Document thermal camera runtime placement, config, usage, and manual hardware checks.

---

### Task 1: Thermal Camera Config

**Files:**
- Modify: `src/app/AppConfig.h`
- Modify: `src/app/AppConfig.cpp`
- Modify: `config/env_config.json`
- Test: `tests/test_app_config.cpp`

- [ ] **Step 1: Write failing config tests**

Add two test slots to `TestAppConfig`:

```cpp
void loadsThermalCameraRuntimeConfig();
void invalidThermalCameraRuntimeConfigFallsBackToDefaults();
```

Add this test body:

```cpp
void TestAppConfig::loadsThermalCameraRuntimeConfig() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "thermal_camera": {
            "ip": "10.0.0.77",
            "port": 8100,
            "user": "operator",
            "password": "pass",
            "sdk_dir": "D:/Hikvision/Thermal",
            "portable_sdk_dir": "lib/sensors/thermal_camera/custom",
            "cache_max_files": 12,
            "display_interval_ms": 500
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.thermalCamera.ip, QString("10.0.0.77"));
    QCOMPARE(config.thermalCamera.port, 8100);
    QCOMPARE(config.thermalCamera.user, QString("operator"));
    QCOMPARE(config.thermalCamera.password, QString("pass"));
    QCOMPARE(config.thermalCamera.sdkDir, QString("D:/Hikvision/Thermal"));
    QCOMPARE(config.thermalCamera.portableSdkDir, QString("lib/sensors/thermal_camera/custom"));
    QCOMPARE(config.thermalCamera.cacheMaxFiles, 12);
    QCOMPARE(config.thermalCamera.displayIntervalMs, 500);
}
```

Add this invalid-value test body:

```cpp
void TestAppConfig::invalidThermalCameraRuntimeConfigFallsBackToDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "thermal_camera": {
            "sdk_dir": "",
            "portable_sdk_dir": "",
            "cache_max_files": 0,
            "display_interval_ms": -1
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.thermalCamera.sdkDir, QString(""));
    QCOMPARE(config.thermalCamera.portableSdkDir, QString("lib/sensors/thermal_camera/lib/win"));
    QCOMPARE(config.thermalCamera.cacheMaxFiles, 60);
    QCOMPARE(config.thermalCamera.displayIntervalMs, 1000);
    const QString warnings = config.warnings().join('\n');
    QVERIFY(warnings.contains("thermal_camera.portable_sdk_dir"));
    QVERIFY(warnings.contains("thermal_camera.cache_max_files"));
    QVERIFY(warnings.contains("thermal_camera.display_interval_ms"));
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake --build build --config Release --target test_app_config
ctest --test-dir build -C Release -R test_app_config --output-on-failure
```

Expected: build fails because `ThermalCameraConfig` does not yet have `sdkDir`, `portableSdkDir`, `cacheMaxFiles`, or `displayIntervalMs`.

- [ ] **Step 3: Implement config fields**

Update `ThermalCameraConfig` in `src/app/AppConfig.h`:

```cpp
struct ThermalCameraConfig {
    QString ip = "192.168.0.64";
    int port = 8000;
    QString user = "admin";
    QString password = "abcd1234";

    // Optional absolute folder containing HCNetSDK.dll, PlayCtrl.dll, and
    // related Hikvision runtime files. Empty means only the portable folder is
    // used.
    QString sdkDir;

    // Relative to the executable folder so a Release directory can be copied to
    // another Windows PC.
    QString portableSdkDir = "lib/sensors/thermal_camera/lib/win";

    // Matches thermal_camera.py and thermal_camera_display.py cache pruning.
    int cacheMaxFiles = 60;

    // Matches thermal_camera_display.py timer2.start(1000).
    int displayIntervalMs = 1000;
};
```

In `src/app/AppConfig.cpp`, after reading `password`, add:

```cpp
config.thermalCamera.sdkDir = readString(thermal, "sdk_dir", config.thermalCamera.sdkDir).trimmed();
config.thermalCamera.portableSdkDir = readNonEmptyString(
    config,
    thermal,
    "portable_sdk_dir",
    config.thermalCamera.portableSdkDir,
    "thermal_camera.portable_sdk_dir");
config.thermalCamera.cacheMaxFiles = readPositiveInt(
    config,
    thermal,
    "cache_max_files",
    config.thermalCamera.cacheMaxFiles,
    "thermal_camera.cache_max_files");
config.thermalCamera.displayIntervalMs = readPositiveInt(
    config,
    thermal,
    "display_interval_ms",
    config.thermalCamera.displayIntervalMs,
    "thermal_camera.display_interval_ms");
```

Update `config/env_config.json`:

```json
"thermal_camera": {
    "ip": "192.168.0.64",
    "port": 8000,
    "user": "admin",
    "password": "abcd1234",
    "sdk_dir": "",
    "portable_sdk_dir": "lib/sensors/thermal_camera/lib/win",
    "cache_max_files": 60,
    "display_interval_ms": 1000
}
```

- [ ] **Step 4: Run test to verify it passes**

Run:

```powershell
cmake --build build --config Release --target test_app_config
ctest --test-dir build -C Release -R test_app_config --output-on-failure
```

Expected: `test_app_config` passes.

- [ ] **Step 5: Commit**

```powershell
git add src/app/AppConfig.h src/app/AppConfig.cpp config/env_config.json tests/test_app_config.cpp
git commit -m "feat: add thermal camera runtime config"
```

---

### Task 2: Thermal Frame Store

**Files:**
- Create: `src/thermal_camera/ThermalFrameStore.h`
- Create: `src/thermal_camera/ThermalFrameStore.cpp`
- Test: `tests/test_thermal_frame_store.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing frame-store tests**

Create `tests/test_thermal_frame_store.cpp`:

```cpp
#include <QtTest/QtTest>

#include <QDir>
#include <QImage>
#include <QTemporaryDir>
#include <QThread>

#include "thermal_camera/ThermalFrameStore.h"

class TestThermalFrameStore final : public QObject {
    Q_OBJECT

private slots:
    void createsExpectedDirectories();
    void clearCacheKeepsCacheDirectory();
    void newestReadableJpegSkipsBrokenNewestFile();
    void pruneCacheKeepsNewestFiles();
    void saveRoiImageWritesExpectedFile();
};

void TestThermalFrameStore::createsExpectedDirectories() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalFrameStore store(QDir(tempDir.path()), 60);

    QVERIFY(store.ensureDirectories());

    QVERIFY(QDir(tempDir.filePath("logs/pic/cache")).exists());
    QVERIFY(QDir(tempDir.filePath("logs/pic/saved")).exists());
    QVERIFY(QDir(tempDir.filePath("logs/pic/roi")).exists());
}

void TestThermalFrameStore::clearCacheKeepsCacheDirectory() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalFrameStore store(QDir(tempDir.path()), 60);
    QVERIFY(store.ensureDirectories());
    QFile file(store.cacheDir().filePath("frame.jpg"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("not a jpeg");
    file.close();

    QVERIFY(store.clearCache());

    QVERIFY(store.cacheDir().exists());
    QVERIFY(store.cacheDir().entryList(QStringList{"*.jpg"}, QDir::Files).isEmpty());
}

void TestThermalFrameStore::newestReadableJpegSkipsBrokenNewestFile() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalFrameStore store(QDir(tempDir.path()), 60);
    QVERIFY(store.ensureDirectories());

    QImage valid(4, 4, QImage::Format_RGB888);
    valid.fill(Qt::red);
    const QString oldPath = store.cacheDir().filePath("old.jpg");
    QVERIFY(valid.save(oldPath, "JPG"));
    QThread::msleep(20);

    QFile broken(store.cacheDir().filePath("new.jpg"));
    QVERIFY(broken.open(QIODevice::WriteOnly));
    broken.write("broken");
    broken.close();

    QImage image;
    const QFileInfo newest = store.newestReadableJpeg(&image);

    QCOMPARE(newest.fileName(), QString("old.jpg"));
    QCOMPARE(image.size(), QSize(4, 4));
}

void TestThermalFrameStore::pruneCacheKeepsNewestFiles() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalFrameStore store(QDir(tempDir.path()), 2);
    QVERIFY(store.ensureDirectories());

    for (int i = 0; i < 4; ++i) {
        QImage image(2, 2, QImage::Format_RGB888);
        image.fill(QColor(10 * i, 0, 0));
        QVERIFY(image.save(store.cacheDir().filePath(QString("frame_%1.jpg").arg(i)), "JPG"));
        QThread::msleep(15);
    }

    store.pruneCache();

    const QStringList files = store.cacheDir().entryList(QStringList{"*.jpg"}, QDir::Files, QDir::Name);
    QCOMPARE(files, QStringList({"frame_2.jpg", "frame_3.jpg"}));
}

void TestThermalFrameStore::saveRoiImageWritesExpectedFile() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalFrameStore store(QDir(tempDir.path()), 60);
    QVERIFY(store.ensureDirectories());
    QImage roi(3, 3, QImage::Format_RGB888);
    roi.fill(Qt::blue);

    QVERIFY(store.saveRoiImage(roi));

    QVERIFY(QFileInfo::exists(store.roiFilePath()));
    QImage loaded(store.roiFilePath());
    QCOMPARE(loaded.size(), QSize(3, 3));
}

QTEST_MAIN(TestThermalFrameStore)
#include "test_thermal_frame_store.moc"
```

Add a test target in `CMakeLists.txt`:

```cmake
add_executable(test_thermal_frame_store
    tests/test_thermal_frame_store.cpp
    src/thermal_camera/ThermalFrameStore.cpp
)
target_include_directories(test_thermal_frame_store PRIVATE src)
target_link_libraries(test_thermal_frame_store PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
copy_qt_test_runtime(test_thermal_frame_store)
add_custom_command(TARGET test_thermal_frame_store POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Gui>
            $<TARGET_FILE_DIR:test_thermal_frame_store>
)
add_test(NAME test_thermal_frame_store COMMAND test_thermal_frame_store)
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake --build build --config Release --target test_thermal_frame_store
```

Expected: build fails because `ThermalFrameStore` does not exist.

- [ ] **Step 3: Implement `ThermalFrameStore`**

Create `src/thermal_camera/ThermalFrameStore.h`:

```cpp
#pragma once

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QString>

// Owns the filesystem layout used by the Python thermal camera workflow:
// logs/pic/cache for recent preview frames, logs/pic/saved for persistent
// copies, and logs/pic/roi for the cropped ROI image.
class ThermalFrameStore {
public:
    explicit ThermalFrameStore(QDir appRootDir, int cacheMaxFiles);

    QDir cacheDir() const;
    QDir savedDir() const;
    QDir roiDir() const;
    QString roiFilePath() const;

    bool ensureDirectories() const;
    bool clearCache() const;
    void pruneCache() const;
    QFileInfo newestReadableJpeg(QImage* image = nullptr) const;
    bool saveRoiImage(const QImage& image) const;

private:
    QList<QFileInfo> cacheJpegsNewestFirst() const;

    QDir rootDir_;
    int cacheMaxFiles_;
};
```

Create `src/thermal_camera/ThermalFrameStore.cpp`:

```cpp
#include "thermal_camera/ThermalFrameStore.h"

#include <algorithm>
#include <QFile>
#include <QFileInfoList>
#include <utility>

ThermalFrameStore::ThermalFrameStore(QDir appRootDir, int cacheMaxFiles)
    : rootDir_(std::move(appRootDir)),
      cacheMaxFiles_(cacheMaxFiles > 0 ? cacheMaxFiles : 60) {
}

QDir ThermalFrameStore::cacheDir() const {
    return QDir(rootDir_.absoluteFilePath("logs/pic/cache"));
}

QDir ThermalFrameStore::savedDir() const {
    return QDir(rootDir_.absoluteFilePath("logs/pic/saved"));
}

QDir ThermalFrameStore::roiDir() const {
    return QDir(rootDir_.absoluteFilePath("logs/pic/roi"));
}

QString ThermalFrameStore::roiFilePath() const {
    // Preserve the Python filename exactly: logs/pic/roi/uarter.png.
    return roiDir().absoluteFilePath("uarter.png");
}

bool ThermalFrameStore::ensureDirectories() const {
    return rootDir_.mkpath("logs/pic/cache")
        && rootDir_.mkpath("logs/pic/saved")
        && rootDir_.mkpath("logs/pic/roi");
}

bool ThermalFrameStore::clearCache() const {
    ensureDirectories();
    QDir dir = cacheDir();
    const QFileInfoList files = dir.entryInfoList(QStringList{"*.jpg", "*.jpeg"},
                                                  QDir::Files,
                                                  QDir::Name);
    bool ok = true;
    for (const QFileInfo& file : files) {
        ok = QFile::remove(file.absoluteFilePath()) && ok;
    }
    return ok && dir.exists();
}

QList<QFileInfo> ThermalFrameStore::cacheJpegsNewestFirst() const {
    QDir dir = cacheDir();
    QFileInfoList files = dir.entryInfoList(QStringList{"*.jpg", "*.jpeg"},
                                            QDir::Files,
                                            QDir::Time);
    return files;
}

void ThermalFrameStore::pruneCache() const {
    const QList<QFileInfo> files = cacheJpegsNewestFirst();
    for (int i = cacheMaxFiles_; i < files.size(); ++i) {
        QFile::remove(files.at(i).absoluteFilePath());
    }
}

QFileInfo ThermalFrameStore::newestReadableJpeg(QImage* image) const {
    const QList<QFileInfo> files = cacheJpegsNewestFirst();
    const int maxAttempts = std::min(files.size(), 5);
    for (int i = 0; i < maxAttempts; ++i) {
        QImage candidate(files.at(i).absoluteFilePath());
        if (!candidate.isNull()) {
            if (image != nullptr) {
                *image = candidate;
            }
            return files.at(i);
        }
    }
    return QFileInfo();
}

bool ThermalFrameStore::saveRoiImage(const QImage& image) const {
    ensureDirectories();
    return !image.isNull() && image.save(roiFilePath(), "PNG");
}
```

- [ ] **Step 4: Run test to verify it passes**

Run:

```powershell
cmake --build build --config Release --target test_thermal_frame_store
ctest --test-dir build -C Release -R test_thermal_frame_store --output-on-failure
```

Expected: `test_thermal_frame_store` passes.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt src/thermal_camera/ThermalFrameStore.h src/thermal_camera/ThermalFrameStore.cpp tests/test_thermal_frame_store.cpp
git commit -m "feat: add thermal frame cache store"
```

---

### Task 3: Thermal Camera Types And Client Interface

**Files:**
- Create: `src/thermal_camera/ThermalCameraTypes.h`
- Create: `src/thermal_camera/IThermalCameraClient.h`
- Test: `tests/test_thermal_camera_client.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing type/interface tests**

Create `tests/test_thermal_camera_client.cpp`:

```cpp
#include <QtTest/QtTest>

#include "thermal_camera/ThermalCameraTypes.h"

class TestThermalCameraClient final : public QObject {
    Q_OBJECT

private slots:
    void errorCodeFormatsAsDecimalAndHex();
};

void TestThermalCameraClient::errorCodeFormatsAsDecimalAndHex() {
    QCOMPARE(thermalCameraErrorText(7), QString("7 (0x00000007)"));
}

QTEST_MAIN(TestThermalCameraClient)
#include "test_thermal_camera_client.moc"
```

Add a temporary CMake target that only uses the type header:

```cmake
add_executable(test_thermal_camera_client
    tests/test_thermal_camera_client.cpp
)
target_include_directories(test_thermal_camera_client PRIVATE src)
target_link_libraries(test_thermal_camera_client PRIVATE Qt6::Core Qt6::Test)
copy_qt_test_runtime(test_thermal_camera_client)
add_test(NAME test_thermal_camera_client COMMAND test_thermal_camera_client)
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake --build build --config Release --target test_thermal_camera_client
```

Expected: build fails because `ThermalCameraTypes.h` does not exist.

- [ ] **Step 3: Implement types and interface**

Create `src/thermal_camera/ThermalCameraTypes.h`:

```cpp
#pragma once

#include <QString>

// Formats Hikvision SDK integer error values in a way that is useful during
// hardware testing. Hikvision documentation often lists decimal values, while
// Windows debugging often uses hexadecimal.
inline QString thermalCameraErrorText(unsigned long code) {
    const QString hex = QString::number(code, 16).rightJustified(8, QLatin1Char('0')).toUpper();
    return QString("%1 (0x%2)")
        .arg(code)
        .arg(hex);
}
```

Create `src/thermal_camera/IThermalCameraClient.h`:

```cpp
#pragma once

#include <QString>

// Small interface used by ThermalCameraService. Tests provide a fake
// implementation, while production uses ThermalCameraClient to call the
// Hikvision runtime DLLs.
class IThermalCameraClient {
public:
    virtual ~IThermalCameraClient() = default;

    virtual bool startCapture() = 0;
    virtual void stopCapture() = 0;
    virtual bool isCapturing() const = 0;
    virtual QString lastError() const = 0;
};
```

- [ ] **Step 4: Run test to verify it passes**

Run:

```powershell
cmake --build build --config Release --target test_thermal_camera_client
ctest --test-dir build -C Release -R test_thermal_camera_client --output-on-failure
```

Expected: `test_thermal_camera_client` passes.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt src/thermal_camera/ThermalCameraTypes.h src/thermal_camera/IThermalCameraClient.h tests/test_thermal_camera_client.cpp
git commit -m "feat: add thermal camera client interface"
```

---

### Task 4: Thermal Camera SDK Client Skeleton And Missing-Runtime Diagnostics

**Files:**
- Create: `src/thermal_camera/ThermalCameraClient.h`
- Create: `src/thermal_camera/ThermalCameraClient.cpp`
- Modify: `tests/test_thermal_camera_client.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Extend failing client tests**

Add these includes:

```cpp
#include <QTemporaryDir>
#include "app/AppConfig.h"
#include "app/AppPaths.h"
#include "thermal_camera/ThermalCameraClient.h"
#include "thermal_camera/ThermalFrameStore.h"
```

Add this test slot:

```cpp
void missingRuntimeFolderReportsUsefulError();
```

Add this test body:

```cpp
void TestThermalCameraClient::missingRuntimeFolderReportsUsefulError() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    ThermalCameraConfig config;
    config.sdkDir.clear();
    config.portableSdkDir = "missing/thermal/runtime";
    ThermalFrameStore store(QDir(tempDir.path()), config.cacheMaxFiles);
    ThermalCameraClient client(config, AppPaths(tempDir.path()), store);

    QVERIFY(!client.startCapture());
    QVERIFY(!client.lastError().isEmpty());
    QVERIFY(client.lastError().contains("HCNetSDK.dll"));
    QVERIFY(client.lastError().contains("PlayCtrl.dll"));
}
```

Update the target:

```cmake
add_executable(test_thermal_camera_client
    tests/test_thermal_camera_client.cpp
    src/thermal_camera/ThermalCameraClient.cpp
    src/thermal_camera/ThermalFrameStore.cpp
)
target_include_directories(test_thermal_camera_client PRIVATE src)
target_link_libraries(test_thermal_camera_client PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
copy_qt_test_runtime(test_thermal_camera_client)
add_custom_command(TARGET test_thermal_camera_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Gui>
            $<TARGET_FILE_DIR:test_thermal_camera_client>
)
add_test(NAME test_thermal_camera_client COMMAND test_thermal_camera_client)
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake --build build --config Release --target test_thermal_camera_client
```

Expected: build fails because `ThermalCameraClient` does not exist.

- [ ] **Step 3: Implement SDK client skeleton**

Create `src/thermal_camera/ThermalCameraClient.h`:

```cpp
#pragma once

#include "app/AppConfig.h"
#include "app/AppPaths.h"
#include "thermal_camera/IThermalCameraClient.h"
#include "thermal_camera/ThermalFrameStore.h"

#include <QDir>
#include <QLibrary>
#include <QString>
#include <QStringList>

// Production implementation for the Hikvision network thermal camera path.
// This class owns SDK initialization, login, preview, decode callbacks, and
// cleanup. The first implementation step focuses on safe runtime loading and
// diagnostics; later steps add the full preview callback path.
class ThermalCameraClient final : public IThermalCameraClient {
public:
    ThermalCameraClient(ThermalCameraConfig config, AppPaths paths, const ThermalFrameStore& frameStore);
    ~ThermalCameraClient() override;

    bool startCapture() override;
    void stopCapture() override;
    bool isCapturing() const override;
    QString lastError() const override;

private:
    QStringList runtimeDirectories() const;
    bool runtimeHasRequiredFiles(const QDir& dir) const;
    bool loadRuntime();
    void setError(QString error);

    ThermalCameraConfig config_;
    AppPaths paths_;
    ThermalFrameStore frameStore_;
    QLibrary hcNetSdk_;
    QLibrary playCtrl_;
    QString lastError_;
    bool capturing_ = false;
};
```

Create `src/thermal_camera/ThermalCameraClient.cpp` with the missing-runtime-safe skeleton:

```cpp
#include "thermal_camera/ThermalCameraClient.h"

#include <QFileInfo>

namespace {
constexpr const char* kHcNetSdkDll = "HCNetSDK.dll";
constexpr const char* kPlayCtrlDll = "PlayCtrl.dll";
}

ThermalCameraClient::ThermalCameraClient(ThermalCameraConfig config,
                                         AppPaths paths,
                                         const ThermalFrameStore& frameStore)
    : config_(std::move(config)),
      paths_(std::move(paths)),
      frameStore_(frameStore) {
}

ThermalCameraClient::~ThermalCameraClient() {
    stopCapture();
}

bool ThermalCameraClient::startCapture() {
    lastError_.clear();
    if (!frameStore_.ensureDirectories()) {
        setError("Could not create thermal camera image cache directories.");
        return false;
    }
    if (!loadRuntime()) {
        return false;
    }

    // Full SDK login and preview startup are added in the next task. Returning
    // false here prevents a misleading success when only runtime loading exists.
    setError("Thermal camera SDK runtime loaded, but preview startup is not implemented yet.");
    return false;
}

void ThermalCameraClient::stopCapture() {
    capturing_ = false;
    if (playCtrl_.isLoaded()) {
        playCtrl_.unload();
    }
    if (hcNetSdk_.isLoaded()) {
        hcNetSdk_.unload();
    }
}

bool ThermalCameraClient::isCapturing() const {
    return capturing_;
}

QString ThermalCameraClient::lastError() const {
    return lastError_;
}

QStringList ThermalCameraClient::runtimeDirectories() const {
    QStringList dirs;
    dirs << paths_.rootDir().absoluteFilePath(config_.portableSdkDir);
    if (!config_.sdkDir.trimmed().isEmpty()) {
        dirs << config_.sdkDir.trimmed();
    }
    dirs.removeDuplicates();
    return dirs;
}

bool ThermalCameraClient::runtimeHasRequiredFiles(const QDir& dir) const {
    return QFileInfo::exists(dir.absoluteFilePath(kHcNetSdkDll))
        && QFileInfo::exists(dir.absoluteFilePath(kPlayCtrlDll));
}

bool ThermalCameraClient::loadRuntime() {
    const QStringList dirs = runtimeDirectories();
    QStringList searched;
    for (const QString& path : dirs) {
        QDir dir(path);
        searched << dir.absolutePath();
        if (!runtimeHasRequiredFiles(dir)) {
            continue;
        }

        hcNetSdk_.setFileName(dir.absoluteFilePath(kHcNetSdkDll));
        playCtrl_.setFileName(dir.absoluteFilePath(kPlayCtrlDll));
        if (hcNetSdk_.load() && playCtrl_.load()) {
            return true;
        }

        setError(QString("Could not load thermal camera SDK DLLs from %1.\nHCNetSDK: %2\nPlayCtrl: %3")
                     .arg(dir.absolutePath(), hcNetSdk_.errorString(), playCtrl_.errorString()));
        return false;
    }

    setError(QString("Could not find thermal camera SDK runtime. Required files: %1, %2.\nSearched folders:\n%3")
                 .arg(kHcNetSdkDll, kPlayCtrlDll, searched.join('\n')));
    return false;
}

void ThermalCameraClient::setError(QString error) {
    lastError_ = std::move(error);
}
```

- [ ] **Step 4: Run tests**

Run:

```powershell
cmake --build build --config Release --target test_thermal_camera_client
ctest --test-dir build -C Release -R test_thermal_camera_client --output-on-failure
```

Expected: missing-runtime diagnostics pass.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt src/thermal_camera/ThermalCameraClient.h src/thermal_camera/ThermalCameraClient.cpp tests/test_thermal_camera_client.cpp
git commit -m "feat: add thermal camera runtime diagnostics"
```

---

### Task 5: Thermal Camera Service With Fake Client

**Files:**
- Create: `src/services/thermal_camera/ThermalCameraService.h`
- Create: `src/services/thermal_camera/ThermalCameraService.cpp`
- Create: `tests/test_thermal_camera_service.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Write failing service tests**

Create `tests/test_thermal_camera_service.cpp` with fake client and frame-store tests:

```cpp
#include <QtTest/QtTest>

#include <QLabel>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QWidget>

#include "thermal_camera/IThermalCameraClient.h"
#include "thermal_camera/ThermalFrameStore.h"
#include "services/thermal_camera/ThermalCameraService.h"
#include "ui_Interface4600_window.h"

class FakeThermalCameraClient final : public IThermalCameraClient {
public:
    bool startResult = true;
    int startCount = 0;
    int stopCount = 0;
    bool capturing = false;
    QString error = "fake thermal error";

    bool startCapture() override {
        ++startCount;
        capturing = startResult;
        return startResult;
    }

    void stopCapture() override {
        ++stopCount;
        capturing = false;
    }

    bool isCapturing() const override {
        return capturing;
    }

    QString lastError() const override {
        return error;
    }
};

class TestThermalCameraService final : public QObject {
    Q_OBJECT

private slots:
    void startCameraReportsClientFailure();
    void clearCacheRemovesCachedFrame();
    void showDataDisplaysNewestFrame();
    void stopCameraStopsClientAndClearsCache();
};

namespace {
std::unique_ptr<FakeThermalCameraClient> makeClient(FakeThermalCameraClient*& raw) {
    auto client = std::make_unique<FakeThermalCameraClient>();
    raw = client.get();
    return client;
}
}

void TestThermalCameraService::startCameraReportsClientFailure() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalFrameStore store(QDir(tempDir.path()), 60);
    FakeThermalCameraClient* client = nullptr;
    auto ownedClient = makeClient(client);
    client->startResult = false;
    ThermalCameraService service(&ui, store, std::move(ownedClient));
    QSignalSpy messages(&service, &ThermalCameraService::messageRequested);

    service.startCamera();

    QCOMPARE(client->startCount, 1);
    QCOMPARE(messages.count(), 1);
    QVERIFY(messages.at(0).at(1).toString().contains("fake thermal error"));
}

void TestThermalCameraService::clearCacheRemovesCachedFrame() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalFrameStore store(QDir(tempDir.path()), 60);
    QVERIFY(store.ensureDirectories());
    QImage image(2, 2, QImage::Format_RGB888);
    image.fill(Qt::green);
    QVERIFY(image.save(store.cacheDir().filePath("frame.jpg"), "JPG"));
    FakeThermalCameraClient* client = nullptr;
    ThermalCameraService service(&ui, store, makeClient(client));

    service.clearCache();

    QVERIFY(store.cacheDir().entryList(QStringList{"*.jpg"}, QDir::Files).isEmpty());
}

void TestThermalCameraService::showDataDisplaysNewestFrame() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalFrameStore store(QDir(tempDir.path()), 60);
    QVERIFY(store.ensureDirectories());
    QImage image(8, 6, QImage::Format_RGB888);
    image.fill(Qt::yellow);
    QVERIFY(image.save(store.cacheDir().filePath("frame.jpg"), "JPG"));
    FakeThermalCameraClient* client = nullptr;
    ThermalCameraService service(&ui, store, makeClient(client));

    service.showData();
    service.refreshDisplayOnceForTest();

    QVERIFY(!ui.TIC_RD->pixmap().isNull());
    QVERIFY(!ui.TIC_PD->pixmap().isNull());
}

void TestThermalCameraService::stopCameraStopsClientAndClearsCache() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalFrameStore store(QDir(tempDir.path()), 60);
    QVERIFY(store.ensureDirectories());
    QImage image(2, 2, QImage::Format_RGB888);
    image.fill(Qt::cyan);
    QVERIFY(image.save(store.cacheDir().filePath("frame.jpg"), "JPG"));
    FakeThermalCameraClient* client = nullptr;
    ThermalCameraService service(&ui, store, makeClient(client));

    service.stopCamera();

    QCOMPARE(client->stopCount, 1);
    QVERIFY(store.cacheDir().entryList(QStringList{"*.jpg"}, QDir::Files).isEmpty());
}

QTEST_MAIN(TestThermalCameraService)
#include "test_thermal_camera_service.moc"
```

Add target:

```cmake
add_executable(test_thermal_camera_service
    tests/test_thermal_camera_service.cpp
    src/services/thermal_camera/ThermalCameraService.cpp
    src/thermal_camera/ThermalFrameStore.cpp
    src/ui/Interface4600_window.ui
)
target_include_directories(test_thermal_camera_service PRIVATE
    src
    ${CMAKE_CURRENT_BINARY_DIR}/test_thermal_camera_service_autogen/include
)
target_link_libraries(test_thermal_camera_service PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
copy_qt_widget_test_runtime(test_thermal_camera_service)
add_test(NAME test_thermal_camera_service COMMAND test_thermal_camera_service)
```

- [ ] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake --build build --config Release --target test_thermal_camera_service
```

Expected: build fails because `ThermalCameraService` does not exist.

- [ ] **Step 3: Implement service**

Create `src/services/thermal_camera/ThermalCameraService.h`:

```cpp
#pragma once

#include "thermal_camera/IThermalCameraClient.h"
#include "thermal_camera/ThermalFrameStore.h"

#include <QObject>
#include <QPointF>
#include <QTimer>
#include <memory>

QT_BEGIN_NAMESPACE
namespace Ui { class ContinuosfiberInterface; }
QT_END_NAMESPACE

// Owns the thermal-camera UI workflow for the existing TIC_* controls. It keeps
// SDK calls behind IThermalCameraClient and keeps the Python JPEG-cache display
// behavior through ThermalFrameStore.
class ThermalCameraService final : public QObject {
    Q_OBJECT
public:
    ThermalCameraService(Ui::ContinuosfiberInterface* ui,
                         ThermalFrameStore frameStore,
                         std::unique_ptr<IThermalCameraClient> client,
                         int displayIntervalMs = 1000,
                         QObject* parent = nullptr);
    ~ThermalCameraService() override;

    bool handleMousePress(Qt::MouseButton button, const QPointF& localPos);

public slots:
    void startCamera();
    void stopCamera();
    void clearCache();
    void showData();
    void refreshDisplayOnceForTest();

signals:
    void messageRequested(const QString& title, const QString& message);

private:
    void refreshDisplay();
    QRect calculateSquareCrop(const QSize& size) const;
    QRect calculateRoiCrop(const QSize& size) const;
    void setLabelImage(QLabel* label, const QImage& image);
    void showMessage(const QString& message);

    Ui::ContinuosfiberInterface* ui_;
    ThermalFrameStore frameStore_;
    std::unique_ptr<IThermalCameraClient> client_;
    QTimer displayTimer_;
    int displayIntervalMs_;
    bool roiTracking_ = false;
    bool roiSet_ = false;
    QVector<QPointF> roiPoints_;
};
```

Create `src/services/thermal_camera/ThermalCameraService.cpp` with:

```cpp
#include "services/thermal_camera/ThermalCameraService.h"

#include "ui_Interface4600_window.h"

#include <algorithm>
#include <QFileInfo>
#include <QLabel>
#include <QMouseEvent>
#include <QPixmap>

namespace {
constexpr const char* kThermalTitle = "Thermal camera";
}

ThermalCameraService::ThermalCameraService(Ui::ContinuosfiberInterface* ui,
                                           ThermalFrameStore frameStore,
                                           std::unique_ptr<IThermalCameraClient> client,
                                           int displayIntervalMs,
                                           QObject* parent)
    : QObject(parent),
      ui_(ui),
      frameStore_(std::move(frameStore)),
      client_(std::move(client)),
      displayIntervalMs_(displayIntervalMs > 0 ? displayIntervalMs : 1000) {
    displayTimer_.setInterval(displayIntervalMs_);
    connect(&displayTimer_, &QTimer::timeout, this, &ThermalCameraService::refreshDisplay);
}

ThermalCameraService::~ThermalCameraService() {
    stopCamera();
}

void ThermalCameraService::startCamera() {
    if (client_ == nullptr) {
        showMessage("Thermal camera client is not available.");
        return;
    }
    if (!client_->startCapture()) {
        showMessage(client_->lastError().isEmpty() ? "Failed to start thermal camera." : client_->lastError());
    }
}

void ThermalCameraService::stopCamera() {
    displayTimer_.stop();
    if (client_ != nullptr) {
        client_->stopCapture();
    }
    frameStore_.clearCache();
}

void ThermalCameraService::clearCache() {
    frameStore_.clearCache();
}

void ThermalCameraService::showData() {
    roiTracking_ = true;
    displayTimer_.start();
}

void ThermalCameraService::refreshDisplayOnceForTest() {
    refreshDisplay();
}

void ThermalCameraService::refreshDisplay() {
    if (ui_ == nullptr) {
        return;
    }
    QImage full;
    const QFileInfo file = frameStore_.newestReadableJpeg(&full);
    if (!file.exists() || full.isNull()) {
        return;
    }

    frameStore_.pruneCache();

    setLabelImage(ui_->TIC_RD, full);
    const QRect crop = roiSet_ ? calculateRoiCrop(full.size()) : calculateSquareCrop(full.size());
    const QImage cropped = full.copy(crop);
    setLabelImage(ui_->TIC_PD, cropped);
    if (roiSet_) {
        frameStore_.saveRoiImage(cropped);
    }
}

bool ThermalCameraService::handleMousePress(Qt::MouseButton button, const QPointF& localPos) {
    if (!roiTracking_) {
        return false;
    }
    if (button == Qt::LeftButton) {
        roiPoints_.append(localPos);
        return true;
    }
    if (button == Qt::MiddleButton && roiPoints_.size() >= 2) {
        roiSet_ = true;
        while (roiPoints_.size() > 2) {
            roiPoints_.removeFirst();
        }
        return true;
    }
    if (button == Qt::RightButton) {
        roiSet_ = false;
        roiPoints_.clear();
        return true;
    }
    return false;
}

QRect ThermalCameraService::calculateSquareCrop(const QSize& size) const {
    const int side = std::min(size.width(), size.height());
    return QRect(0, 0, side, side);
}

QRect ThermalCameraService::calculateRoiCrop(const QSize& size) const {
    if (roiPoints_.size() < 2) {
        return calculateSquareCrop(size);
    }

    const QPointF a = roiPoints_.at(0);
    const QPointF b = roiPoints_.at(1);
    const double leftNorm = std::clamp(std::min(a.x(), b.x()) / 691.0, 0.0, 1.0);
    const double rightNorm = std::clamp(std::max(a.x(), b.x()) / 691.0, 0.0, 1.0);
    const double topNorm = std::clamp(std::min(a.y(), b.y()) / 518.0, 0.0, 1.0);
    const double bottomNorm = std::clamp(std::max(a.y(), b.y()) / 518.0, 0.0, 1.0);

    QRect rect(QPoint(static_cast<int>(leftNorm * size.width()), static_cast<int>(topNorm * size.height())),
               QPoint(static_cast<int>(rightNorm * size.width()), static_cast<int>(bottomNorm * size.height())));
    rect = rect.normalized().intersected(QRect(QPoint(0, 0), size));
    const int side = std::max(1, std::min(rect.width(), rect.height()));
    return QRect(rect.topLeft(), QSize(side, side)).intersected(QRect(QPoint(0, 0), size));
}

void ThermalCameraService::setLabelImage(QLabel* label, const QImage& image) {
    if (label == nullptr || image.isNull()) {
        return;
    }
    label->setScaledContents(true);
    label->setPixmap(QPixmap::fromImage(image));
}

void ThermalCameraService::showMessage(const QString& message) {
    emit messageRequested(kThermalTitle, message);
}
```

- [ ] **Step 4: Run test**

Run:

```powershell
cmake --build build --config Release --target test_thermal_camera_service
ctest --test-dir build -C Release -R test_thermal_camera_service --output-on-failure
```

Expected: service tests pass.

- [ ] **Step 5: Commit**

```powershell
git add CMakeLists.txt src/services/thermal_camera/ThermalCameraService.h src/services/thermal_camera/ThermalCameraService.cpp tests/test_thermal_camera_service.cpp
git commit -m "feat: add thermal camera UI service"
```

---

### Task 6: Full Hikvision Preview Callback Path

**Files:**
- Modify: `src/thermal_camera/ThermalCameraClient.h`
- Modify: `src/thermal_camera/ThermalCameraClient.cpp`
- Modify: `tests/test_thermal_camera_client.cpp`

- [ ] **Step 1: Add non-hardware cleanup test**

Add a test that verifies `stopCapture()` is safe before start:

```cpp
void stopCaptureBeforeStartIsSafe();
```

```cpp
void TestThermalCameraClient::stopCaptureBeforeStartIsSafe() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalCameraConfig config;
    ThermalFrameStore store(QDir(tempDir.path()), config.cacheMaxFiles);
    ThermalCameraClient client(config, AppPaths(tempDir.path()), store);

    client.stopCapture();

    QVERIFY(!client.isCapturing());
}
```

- [ ] **Step 2: Implement SDK function resolution and state fields**

Add narrow SDK struct definitions in `ThermalCameraClient.cpp`. Copy only fields required by the Python workflow:

```cpp
using LONG = long;
using DWORD = unsigned long;
using BOOL = int;
using BYTE = unsigned char;

struct NET_DVR_DEVICEINFO_V30 {
    BYTE sSerialNumber[48];
    BYTE byRes[196];
};

struct NET_DVR_LOCAL_SDK_PATH {
    char sPath[256];
    BYTE byRes[128];
};

struct NET_DVR_PREVIEWINFO {
    long lChannel;
    unsigned long dwStreamType;
    unsigned long dwLinkMode;
    void* hPlayWnd;
    int bBlocked;
    int bPassbackRecord;
    BYTE byPreviewMode;
    BYTE byStreamID[32];
    BYTE byProtoType;
    BYTE byRes1;
    BYTE byVideoCodingType;
    DWORD dwDisplayBufNum;
    BYTE byNPQMode;
    BYTE byRecvMetaData;
    BYTE byDataType;
    BYTE byRes[213];
};

struct FRAME_INFO {
    DWORD nWidth;
    DWORD nHeight;
    DWORD nStamp;
    DWORD nType;
    DWORD nFrameRate;
    DWORD dwFrameNum;
};
```

Add function pointer typedefs for only the used calls:

```cpp
using RealDataCallback = void(__stdcall*)(LONG, DWORD, BYTE*, DWORD, void*);
using DecodeCallback = void(__stdcall*)(LONG, char*, LONG, FRAME_INFO*, void*, void*);
using NET_DVR_InitFn = BOOL(__stdcall*)();
using NET_DVR_CleanupFn = BOOL(__stdcall*)();
using NET_DVR_SetSDKInitCfgFn = BOOL(__stdcall*)(int, void*);
using NET_DVR_SetLogToFileFn = BOOL(__stdcall*)(DWORD, const char*, BOOL);
using NET_DVR_Login_V30Fn = LONG(__stdcall*)(char*, unsigned short, char*, char*, NET_DVR_DEVICEINFO_V30*);
using NET_DVR_LogoutFn = BOOL(__stdcall*)(LONG);
using NET_DVR_RealPlay_V40Fn = LONG(__stdcall*)(LONG, NET_DVR_PREVIEWINFO*, RealDataCallback, void*);
using NET_DVR_StopRealPlayFn = BOOL(__stdcall*)(LONG);
using NET_DVR_GetLastErrorFn = DWORD(__stdcall*)();
using PlayM4_GetPortFn = BOOL(__stdcall*)(LONG*);
using PlayM4_FreePortFn = BOOL(__stdcall*)(LONG);
using PlayM4_SetStreamOpenModeFn = BOOL(__stdcall*)(LONG, DWORD);
using PlayM4_OpenStreamFn = BOOL(__stdcall*)(LONG, BYTE*, DWORD, DWORD);
using PlayM4_CloseStreamFn = BOOL(__stdcall*)(LONG);
using PlayM4_SetDecCallBackExMendFn = BOOL(__stdcall*)(LONG, DecodeCallback, void*, DWORD, void*);
using PlayM4_PlayFn = BOOL(__stdcall*)(LONG, void*);
using PlayM4_StopFn = BOOL(__stdcall*)(LONG);
using PlayM4_InputDataFn = BOOL(__stdcall*)(LONG, BYTE*, DWORD);
using PlayM4_ConvertToJpegFileFn = BOOL(__stdcall*)(char*, LONG, DWORD, DWORD, DWORD, const char*);
using PlayM4_GetLastErrorFn = DWORD(__stdcall*)(LONG);
```

Add state members in the header:

```cpp
long userId_ = -1;
long realPlayHandle_ = -1;
long playPort_ = -1;
bool sdkInitialized_ = false;
```

Keep callback function pointers as static member functions that receive `this` through `pUser`.

- [ ] **Step 3: Implement start flow**

In `startCapture()`:

1. Ensure frame directories exist.
2. Load runtime.
3. Resolve all function pointers.
4. Call `NET_DVR_SetSDKInitCfg` for SDK path, `libcrypto-1_1-x64.dll`, and `libssl-1_1-x64.dll`.
5. Call `NET_DVR_Init`.
6. Call `NET_DVR_SetLogToFile(3, "SdkLog_CPP", false)`.
7. Call `PlayM4_GetPort`.
8. Call `NET_DVR_Login_V30`.
9. Call `NET_DVR_RealPlay_V40`.
10. Set `capturing_ = true`.

Each failure must set `lastError_` with action text and SDK error code, then call `stopCapture()`.

- [ ] **Step 4: Implement callback flow**

The real-data callback should follow Python:

- For `NET_DVR_SYSHEAD`:
  - call `PlayM4_SetStreamOpenMode(playPort_, 0)`
  - call `PlayM4_OpenStream(playPort_, buffer, size, 1024 * 1024)`
  - call `PlayM4_SetDecCallBackExMend(playPort_, &ThermalCameraClient::decodeCallback, this, 0, nullptr)`
  - call `PlayM4_Play(playPort_, nullptr)`
- For `NET_DVR_STREAMDATA`:
  - call `PlayM4_InputData(playPort_, buffer, size)`

The decode callback should:

- Check `FRAME_INFO::nType == 3`.
- Write `logs/pic/cache/test_stamp[frameNumber].jpg`.
- Call `PlayM4_ConvertToJpegFile`.
- Copy the JPEG to `logs/pic/saved`.
- Call `frameStore_.pruneCache()`.

Add comments explaining that the callbacks must never touch Qt widgets directly.

- [ ] **Step 5: Implement stop flow**

`stopCapture()` must be safe when partially started:

1. Stop preview if `realPlayHandle_ >= 0`.
2. Logout if `userId_ >= 0`.
3. Stop and close PlayCtrl stream if `playPort_ >= 0`.
4. Free PlayCtrl port if `playPort_ >= 0`.
5. Cleanup SDK if initialized.
6. Unload libraries.
7. Reset handles and `capturing_`.

- [ ] **Step 6: Build and run non-hardware tests**

Run:

```powershell
cmake --build build --config Release --target test_thermal_camera_client
ctest --test-dir build -C Release -R test_thermal_camera_client --output-on-failure
```

Expected: non-hardware tests pass. Hardware startup cannot be fully verified until runtime DLLs and the camera are present.

- [ ] **Step 7: Commit**

```powershell
git add src/thermal_camera/ThermalCameraClient.h src/thermal_camera/ThermalCameraClient.cpp tests/test_thermal_camera_client.cpp
git commit -m "feat: add hikvision thermal preview client"
```

---

### Task 7: Main Window Integration

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Modify: `src/services/PlaceholderServices.h`
- Modify: `src/services/PlaceholderServices.cpp`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Remove temporary thermal service**

Delete `ThermalCameraService` from `PlaceholderServices.h` and its four methods from `PlaceholderServices.cpp`.

- [ ] **Step 2: Add real thermal service to app sources**

Add these to `APP_SOURCES`:

```cmake
src/thermal_camera/ThermalCameraTypes.h
src/thermal_camera/IThermalCameraClient.h
src/thermal_camera/ThermalFrameStore.h
src/thermal_camera/ThermalFrameStore.cpp
src/thermal_camera/ThermalCameraClient.h
src/thermal_camera/ThermalCameraClient.cpp
src/services/thermal_camera/ThermalCameraService.h
src/services/thermal_camera/ThermalCameraService.cpp
```

- [ ] **Step 3: Update `MainWindow.h`**

Replace the temporary-service include usage with:

```cpp
#include "services/thermal_camera/ThermalCameraService.h"
```

Add a mouse handler declaration:

```cpp
void mousePressEvent(QMouseEvent* event) override;
```

Keep the member:

```cpp
ThermalCameraService thermalCameraService_;
```

- [ ] **Step 4: Update `MainWindow.cpp` construction**

Add includes:

```cpp
#include "thermal_camera/ThermalCameraClient.h"
#include "thermal_camera/ThermalFrameStore.h"
#include <QMouseEvent>
#include <memory>
```

Construct the real service:

```cpp
thermalCameraService_(
    ui_,
    ThermalFrameStore(paths_.rootDir(), config_.thermalCamera.cacheMaxFiles),
    std::make_unique<ThermalCameraClient>(
        config_.thermalCamera,
        paths_,
        ThermalFrameStore(paths_.rootDir(), config_.thermalCamera.cacheMaxFiles)),
    config_.thermalCamera.displayIntervalMs,
    this)
```

Remove `connectPlaceholder(thermalCameraService_)`.

Add:

```cpp
connect(&thermalCameraService_, &ThermalCameraService::messageRequested,
        this, &MainWindow::showPlaceholderMessage);
```

Keep the existing `TIC_*` button connections, now targeting the real service.

- [ ] **Step 5: Forward mouse presses**

Add:

```cpp
void MainWindow::mousePressEvent(QMouseEvent* event) {
    if (event != nullptr && thermalCameraService_.handleMousePress(event->button(), event->position())) {
        return;
    }
    QWidget::mousePressEvent(event);
}
```

This preserves the Python behavior where `MyWindow.mousePressEvent` handles thermal ROI clicks globally.

- [ ] **Step 6: Build app target**

Run:

```powershell
cmake --build build --config Release --target AFP_ABB_CPP
```

Expected: `AFP_ABB_CPP.exe` builds.

- [ ] **Step 7: Commit**

```powershell
git add CMakeLists.txt src/ui/MainWindow.h src/ui/MainWindow.cpp src/services/PlaceholderServices.h src/services/PlaceholderServices.cpp
git commit -m "feat: wire thermal camera service into main window"
```

---

### Task 8: Portable Runtime Folder And Documentation

**Files:**
- Modify: `README.md`
- Optional create: `lib/sensors/thermal_camera/README.md`

- [ ] **Step 1: Confirm portable copy behavior**

The existing app post-build command copies the whole `lib` folder:

```cmake
COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
        $<TARGET_FILE_DIR:AFP_ABB_CPP>/lib
```

Therefore, any thermal runtime files placed under:

```text
lib/sensors/thermal_camera/lib/win
```

will be copied automatically into:

```text
build/Release/lib/sensors/thermal_camera/lib/win
```

- [ ] **Step 2: Document runtime placement**

Update `README.md` with a thermal camera section:

```markdown
## Thermal Camera Workflow

Slice I converts the original Hikvision network thermal camera workflow.

The C++ implementation uses the same JPEG cache behavior as Python:

1. `TIC_machine` starts the Hikvision SDK preview stream.
2. Decoded frames are written to `logs\pic\cache`.
3. Frames are copied to `logs\pic\saved`.
4. `TIC_data` displays the newest readable cache JPEG in `TIC_RD` and `TIC_PD`.
5. `TIC_start` clears the cache.
6. `TIC_stop` stops capture and clears the cache.

Portable thermal runtime files should be placed in:

```text
lib\sensors\thermal_camera\lib\win
```

At minimum this folder must include the matching Hikvision runtime files used by the original Python workflow, including `HCNetSDK.dll`, `PlayCtrl.dll`, `libcrypto-1_1-x64.dll`, `libssl-1_1-x64.dll`, and the `HCNetSDKCom` folder. Use only vendor files that are allowed to be redistributed for your deployment.

The thermal camera settings are in `config\env_config.json`:

```json
"thermal_camera": {
    "ip": "192.168.0.64",
    "port": 8000,
    "user": "admin",
    "password": "abcd1234",
    "sdk_dir": "",
    "portable_sdk_dir": "lib/sensors/thermal_camera/lib/win",
    "cache_max_files": 60,
    "display_interval_ms": 1000
}
```
```

- [ ] **Step 3: Add optional runtime-folder note**

If no vendor DLLs are committed, add `lib/sensors/thermal_camera/README.md`:

```markdown
# Thermal Camera Runtime Files

Place the Hikvision thermal camera runtime folder here:

`lib/sensors/thermal_camera/lib/win`

The application expects `HCNetSDK.dll`, `PlayCtrl.dll`, OpenSSL DLLs, and the SDK dependency folders in that runtime directory. These files are not generated by the C++ build.
```

- [ ] **Step 4: Commit**

```powershell
git add README.md lib/sensors/thermal_camera/README.md
git commit -m "docs: document thermal camera runtime setup"
```

If `lib/sensors/thermal_camera/README.md` is not created because runtime files are added instead, only add `README.md` and the permitted runtime files.

---

### Task 9: Full Verification

**Files:**
- All files touched by this slice.

- [ ] **Step 1: Configure**

Run:

```powershell
cmake -S . -B build
```

Expected: configure succeeds.

- [ ] **Step 2: Build app and all tests**

Run:

```powershell
cmake --build build --config Release
```

Expected: build succeeds.

- [ ] **Step 3: Run full test suite**

Run:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Expected: all tests pass.

- [ ] **Step 4: Check portable output**

Confirm these files/folders exist when runtime files are present in source:

```text
build/Release/AFP_ABB_CPP.exe
build/Release/config/env_config.json
build/Release/lib/sensors/thermal_camera/lib/win
```

If the source runtime folder is absent, confirm `AFP_ABB_CPP.exe` still builds and thermal button use reports missing `HCNetSDK.dll` and `PlayCtrl.dll` instead of crashing.

- [ ] **Step 5: Final commit if verification required fixes**

If verification required small fixes:

```powershell
git add <fixed files>
git commit -m "fix: stabilize thermal camera workflow"
```

---

## Manual Hardware Verification

Run this only on a Windows PC with the Hikvision thermal camera and runtime files available.

1. Put the Hikvision runtime folder under `build\Release\lib\sensors\thermal_camera\lib\win`, or set `thermal_camera.sdk_dir` to the installed runtime folder.
2. Edit `build\Release\config\env_config.json` so `thermal_camera.ip`, `thermal_camera.port`, `thermal_camera.user`, and `thermal_camera.password` match the camera.
3. Start `build\Release\AFP_ABB_CPP.exe`.
4. Click `TIC_machine`.
5. Confirm no error message appears.
6. Click `TIC_data`.
7. Confirm `TIC_RD` and `TIC_PD` update about once per second.
8. Confirm JPEGs appear in `build\Release\logs\pic\cache`.
9. Confirm JPEGs appear in `build\Release\logs\pic\saved`.
10. Click two ROI points with the left mouse button, then click the middle mouse button to apply ROI.
11. Confirm `TIC_PD` changes to the selected crop and `build\Release\logs\pic\roi\uarter.png` appears.
12. Right click and confirm ROI selection is cancelled.
13. Click `TIC_stop`.
14. Confirm capture stops and `build\Release\logs\pic\cache` is cleared.

## Open Item Kept Separate

The high-speed camera hardware issue with SDK code `0x80000006` remains unresolved and is not changed by this thermal camera slice.
