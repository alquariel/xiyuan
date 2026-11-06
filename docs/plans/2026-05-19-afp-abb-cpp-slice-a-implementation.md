# AFP ABB C++ Slice A Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build the first C++ migration slice: a portable Windows Qt Widgets executable shell in `AFP_ABB_CPP` that preserves the original Python UI design and loads configuration without performing hardware I/O.

**Architecture:** Use a CMake + Qt Widgets project with the original `Interface4600_window.ui` copied unchanged into the C++ project. Keep startup, config loading, path resolution, window initialization, and placeholder service boundaries separate so later hardware slices can replace placeholders without reshaping the app.

**Tech Stack:** C++17, CMake, Qt 6 Widgets/Core/Gui/Test, Qt UIC/MOC, `windeployqt` for portable Windows packaging.

---

## File Structure

Create these files under `D:/OneDrive/NWPU/4600_BACKUP_ALL_26Apr2026/AFP_ABB_CPP`:

- `CMakeLists.txt`: top-level CMake project, Qt discovery, executable target, tests, asset copying, install/deploy helper targets.
- `README.md`: Windows build, run, and portable packaging instructions.
- `cmake/DeployQt.cmake`: helper script that calls `windeployqt` after the executable is built.
- `config/env_config.json`: copied from Python source, unchanged.
- `config/logging_config.json`: copied from Python source, unchanged.
- `ui/Interface4600_window.ui`: copied from Python source, unchanged.
- `assets/icons/icon.png`: copied from Python source, unchanged.
- `src/main.cpp`: Qt application entry point.
- `src/app/AppPaths.h` and `src/app/AppPaths.cpp`: runtime path resolver for executable-relative assets.
- `src/app/AppConfig.h` and `src/app/AppConfig.cpp`: typed config loader using `QJsonDocument`.
- `src/app/StatusLogger.h` and `src/app/StatusLogger.cpp`: lightweight Qt logging setup.
- `src/ui/MainWindow.h` and `src/ui/MainWindow.cpp`: owns the generated UI and wires safe placeholder slots.
- `src/services/PlaceholderServices.h` and `src/services/PlaceholderServices.cpp`: thin service classes that report "not converted in Slice A".
- `tests/test_app_paths.cpp`: Qt Test coverage for `AppPaths`.
- `tests/test_app_config.cpp`: Qt Test coverage for `AppConfig`.

Do not create, edit, or move files inside `AFP_ABB_Python`.

---

### Task 1: Create Project Skeleton And Copy Assets

**Files:**
- Create: `AFP_ABB_CPP/CMakeLists.txt`
- Create: `AFP_ABB_CPP/README.md`
- Create directories: `cmake`, `config`, `ui`, `assets/icons`, `src/app`, `src/ui`, `src/services`, `tests`
- Copy unchanged: `AFP_ABB_Python/config/env_config.json` to `AFP_ABB_CPP/config/env_config.json`
- Copy unchanged: `AFP_ABB_Python/config/logging_config.json` to `AFP_ABB_CPP/config/logging_config.json`
- Copy unchanged: `AFP_ABB_Python/src/ui/Interface4600_window.ui` to `AFP_ABB_CPP/ui/Interface4600_window.ui`
- Copy unchanged: `AFP_ABB_Python/src/ui/resources/icons/icon.png` to `AFP_ABB_CPP/assets/icons/icon.png`

- [ ] **Step 1: Create directories**

Run from `D:/OneDrive/NWPU/4600_BACKUP_ALL_26Apr2026/AFP_ABB_CPP`:

```powershell
New-Item -ItemType Directory -Force cmake,config,ui,assets\icons,src\app,src\ui,src\services,tests
```

Expected: all directories exist.

- [ ] **Step 2: Copy approved runtime assets without editing them**

Run from `D:/OneDrive/NWPU/4600_BACKUP_ALL_26Apr2026/AFP_ABB_CPP`:

```powershell
Copy-Item ..\AFP_ABB_Python\config\env_config.json config\env_config.json -Force
Copy-Item ..\AFP_ABB_Python\config\logging_config.json config\logging_config.json -Force
Copy-Item ..\AFP_ABB_Python\src\ui\Interface4600_window.ui ui\Interface4600_window.ui -Force
Copy-Item ..\AFP_ABB_Python\src\ui\resources\icons\icon.png assets\icons\icon.png -Force
```

Expected: copied files exist and the Python source files are unchanged.

- [ ] **Step 3: Add initial `CMakeLists.txt`**

Create `CMakeLists.txt` with:

```cmake
cmake_minimum_required(VERSION 3.20)

project(AFP_ABB_CPP VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)

set(APP_SOURCES
    src/main.cpp
    src/app/AppPaths.h
    src/app/AppPaths.cpp
    src/app/AppConfig.h
    src/app/AppConfig.cpp
    src/app/StatusLogger.h
    src/app/StatusLogger.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/services/PlaceholderServices.h
    src/services/PlaceholderServices.cpp
    ui/Interface4600_window.ui
)

qt_add_executable(AFP_ABB_CPP WIN32 ${APP_SOURCES})

target_include_directories(AFP_ABB_CPP PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR}/AFP_ABB_CPP_autogen/include
)

target_link_libraries(AFP_ABB_CPP PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

add_custom_command(TARGET AFP_ABB_CPP POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/config
            $<TARGET_FILE_DIR:AFP_ABB_CPP>/config
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:AFP_ABB_CPP>/assets
)

enable_testing()

add_executable(test_app_paths
    tests/test_app_paths.cpp
    src/app/AppPaths.cpp
)
target_include_directories(test_app_paths PRIVATE src)
target_link_libraries(test_app_paths PRIVATE Qt6::Core Qt6::Test)
add_test(NAME test_app_paths COMMAND test_app_paths)

add_executable(test_app_config
    tests/test_app_config.cpp
    src/app/AppConfig.cpp
    src/app/AppPaths.cpp
)
target_include_directories(test_app_config PRIVATE src)
target_link_libraries(test_app_config PRIVATE Qt6::Core Qt6::Test)
add_test(NAME test_app_config COMMAND test_app_config)

add_custom_target(deploy_portable
    COMMAND ${CMAKE_COMMAND}
            -DQT_DEPLOY_EXECUTABLE=$<TARGET_FILE:AFP_ABB_CPP>
            -DQT_DEPLOY_OUTPUT=$<TARGET_FILE_DIR:AFP_ABB_CPP>
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DeployQt.cmake
    DEPENDS AFP_ABB_CPP
    COMMENT "Deploying a portable Windows application folder"
)
```

- [ ] **Step 4: Add temporary `README.md` marker**

Create `README.md` with:

```markdown
# AFP_ABB_CPP

This folder contains the C++ migration of the AFP ABB Python software.

Slice A builds the Qt Widgets application shell and portable Windows runtime folder.
```

- [ ] **Step 5: Run CMake configure to confirm Qt is found**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.9.1\msvc2022_64
```

Expected: configure fails because source files are not created yet, but the Qt discovery lines show Qt 6 was found under `C:\Qt\6.9.1\msvc2022_64`. If Qt is installed elsewhere, use that Qt path and document it in `README.md`.

---

### Task 2: Implement `AppPaths`

**Files:**
- Create: `src/app/AppPaths.h`
- Create: `src/app/AppPaths.cpp`
- Test: `tests/test_app_paths.cpp`

- [ ] **Step 1: Write failing path tests**

Create `tests/test_app_paths.cpp` with:

```cpp
#include <QtTest/QtTest>
#include <QCoreApplication>
#include "app/AppPaths.h"

class TestAppPaths : public QObject {
    Q_OBJECT

private slots:
    void resolvesPathsFromApplicationDirectory() {
        const AppPaths paths(QCoreApplication::applicationDirPath());

        QVERIFY(paths.rootDir().isAbsolute());
        QCOMPARE(paths.configFilePath().fileName(), QString("env_config.json"));
        QVERIFY(paths.configFilePath().path().endsWith("config"));
        QCOMPARE(paths.loggingConfigFilePath().fileName(), QString("logging_config.json"));
        QCOMPARE(paths.iconFilePath().fileName(), QString("icon.png"));
    }

    void acceptsExplicitRootForTests() {
        const AppPaths paths("C:/portable/AFP_ABB_CPP");

        QCOMPARE(paths.rootDir().absolutePath(), QString("C:/portable/AFP_ABB_CPP"));
        QCOMPARE(paths.configDir().absolutePath(), QString("C:/portable/AFP_ABB_CPP/config"));
        QCOMPARE(paths.assetsDir().absolutePath(), QString("C:/portable/AFP_ABB_CPP/assets"));
    }
};

QTEST_MAIN(TestAppPaths)
#include "test_app_paths.moc"
```

- [ ] **Step 2: Run the test and verify it fails**

Run:

```powershell
cmake --build build --target test_app_paths --config Release
ctest --test-dir build -C Release -R test_app_paths --output-on-failure
```

Expected: build fails because `AppPaths.h` and `AppPaths.cpp` do not exist.

- [ ] **Step 3: Implement `AppPaths.h`**

Create `src/app/AppPaths.h` with:

```cpp
#pragma once

#include <QDir>
#include <QFileInfo>
#include <QString>

class AppPaths {
public:
    explicit AppPaths(QString applicationDirPath);

    QDir rootDir() const;
    QDir configDir() const;
    QDir assetsDir() const;
    QFileInfo configFilePath() const;
    QFileInfo loggingConfigFilePath() const;
    QFileInfo iconFilePath() const;

private:
    QDir rootDir_;
};
```

- [ ] **Step 4: Implement `AppPaths.cpp`**

Create `src/app/AppPaths.cpp` with:

```cpp
#include "app/AppPaths.h"

AppPaths::AppPaths(QString applicationDirPath)
    : rootDir_(QDir(applicationDirPath)) {
}

QDir AppPaths::rootDir() const {
    return rootDir_;
}

QDir AppPaths::configDir() const {
    return QDir(rootDir_.absoluteFilePath("config"));
}

QDir AppPaths::assetsDir() const {
    return QDir(rootDir_.absoluteFilePath("assets"));
}

QFileInfo AppPaths::configFilePath() const {
    return QFileInfo(configDir().absoluteFilePath("env_config.json"));
}

QFileInfo AppPaths::loggingConfigFilePath() const {
    return QFileInfo(configDir().absoluteFilePath("logging_config.json"));
}

QFileInfo AppPaths::iconFilePath() const {
    return QFileInfo(assetsDir().absoluteFilePath("icons/icon.png"));
}
```

- [ ] **Step 5: Run the test and verify it passes**

Run:

```powershell
cmake --build build --target test_app_paths --config Release
ctest --test-dir build -C Release -R test_app_paths --output-on-failure
```

Expected: `test_app_paths` passes.

- [ ] **Step 6: Commit the path resolver**

Run:

```powershell
git add CMakeLists.txt src/app/AppPaths.h src/app/AppPaths.cpp tests/test_app_paths.cpp
git commit -m "build: add app path resolver"
```

Expected: commit succeeds if `AFP_ABB_CPP` is a git repository. If it is not a git repository, skip the commit and note that in the final implementation summary.

---

### Task 3: Implement `AppConfig`

**Files:**
- Create: `src/app/AppConfig.h`
- Create: `src/app/AppConfig.cpp`
- Test: `tests/test_app_config.cpp`

- [ ] **Step 1: Write failing config tests**

Create `tests/test_app_config.cpp` with:

```cpp
#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include "app/AppConfig.h"

class TestAppConfig : public QObject {
    Q_OBJECT

private slots:
    void loadsExpectedSections() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        QDir root(tempDir.path());
        QVERIFY(root.mkpath("config"));

        QFile file(root.filePath("config/env_config.json"));
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        QTextStream stream(&file);
        stream << R"({
            "robot": {"ip": "192.168.0.104", "port_motion": 5010, "port_server": 5012, "transport": {"type": "plc"}},
            "ftp": {"ip": "192.168.0.104", "port": 21, "user": "Default User", "password": "robotics"},
            "plc": {"ip": "192.168.0.201", "rack": 0, "slot": 1},
            "thermal_camera": {"ip": "192.168.0.64", "port": 8000, "user": "admin", "password": "abcd1234"},
            "serial": {"general_baudrate": 9600, "fiber_laser": {"port": "COM3", "baudrate": 9600}}
        })";
        file.close();

        const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

        QVERIFY(config.isLoaded());
        QCOMPARE(config.robot.ip, QString("192.168.0.104"));
        QCOMPARE(config.robot.portMotion, 5010);
        QCOMPARE(config.robot.portServer, 5012);
        QCOMPARE(config.robot.transportType, QString("plc"));
        QCOMPARE(config.ftp.user, QString("Default User"));
        QCOMPARE(config.plc.ip, QString("192.168.0.201"));
        QCOMPARE(config.plc.rack, 0);
        QCOMPARE(config.plc.slot, 1);
        QCOMPARE(config.thermalCamera.user, QString("admin"));
        QCOMPARE(config.serial.generalBaudrate, 9600);
        QCOMPARE(config.serial.fiberLaserPort, QString("COM3"));
    }

    void missingConfigUsesDefaults() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const AppConfig config = AppConfig::load(AppPaths(tempDir.path()));

        QVERIFY(!config.isLoaded());
        QVERIFY(!config.warnings().isEmpty());
        QCOMPARE(config.robot.ip, QString("192.168.0.104"));
        QCOMPARE(config.plc.ip, QString("192.168.0.201"));
    }
};

QTEST_MAIN(TestAppConfig)
#include "test_app_config.moc"
```

- [ ] **Step 2: Run the test and verify it fails**

Run:

```powershell
cmake --build build --target test_app_config --config Release
ctest --test-dir build -C Release -R test_app_config --output-on-failure
```

Expected: build fails because `AppConfig.h` and `AppConfig.cpp` do not exist.

- [ ] **Step 3: Implement `AppConfig.h`**

Create `src/app/AppConfig.h` with:

```cpp
#pragma once

#include <QString>
#include <QStringList>
#include "app/AppPaths.h"

struct RobotConfig {
    QString ip = "192.168.0.104";
    int portMotion = 5010;
    int portServer = 5012;
    QString transportType = "plc";
};

struct FtpConfig {
    QString ip = "192.168.0.104";
    int port = 21;
    QString user = "Default User";
    QString password = "robotics";
};

struct PlcConfig {
    QString ip = "192.168.0.201";
    int rack = 0;
    int slot = 1;
};

struct ThermalCameraConfig {
    QString ip = "192.168.0.64";
    int port = 8000;
    QString user = "admin";
    QString password = "abcd1234";
};

struct SerialConfig {
    int generalBaudrate = 9600;
    QString fiberLaserPort = "COM3";
    int fiberLaserBaudrate = 9600;
};

class AppConfig {
public:
    RobotConfig robot;
    FtpConfig ftp;
    PlcConfig plc;
    ThermalCameraConfig thermalCamera;
    SerialConfig serial;

    static AppConfig load(const AppPaths& paths);

    bool isLoaded() const;
    QStringList warnings() const;

private:
    bool loaded_ = false;
    QStringList warnings_;
};
```

- [ ] **Step 4: Implement `AppConfig.cpp`**

Create `src/app/AppConfig.cpp` with:

```cpp
#include "app/AppConfig.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
QString readString(const QJsonObject& object, const QString& key, const QString& fallback) {
    const QJsonValue value = object.value(key);
    return value.isString() ? value.toString() : fallback;
}

int readInt(const QJsonObject& object, const QString& key, int fallback) {
    const QJsonValue value = object.value(key);
    return value.isDouble() ? value.toInt() : fallback;
}
}

AppConfig AppConfig::load(const AppPaths& paths) {
    AppConfig config;

    QFile file(paths.configFilePath().absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        config.warnings_ << QString("Could not open %1").arg(paths.configFilePath().absoluteFilePath());
        return config;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        config.warnings_ << QString("Could not parse env_config.json: %1").arg(error.errorString());
        return config;
    }

    const QJsonObject root = document.object();

    const QJsonObject robot = root.value("robot").toObject();
    config.robot.ip = readString(robot, "ip", config.robot.ip);
    config.robot.portMotion = readInt(robot, "port_motion", config.robot.portMotion);
    config.robot.portServer = readInt(robot, "port_server", config.robot.portServer);
    const QJsonValue transportValue = robot.value("transport");
    if (transportValue.isObject()) {
        config.robot.transportType = readString(transportValue.toObject(), "type", config.robot.transportType);
    } else if (transportValue.isString()) {
        config.robot.transportType = transportValue.toString();
    }

    const QJsonObject ftp = root.value("ftp").toObject();
    config.ftp.ip = readString(ftp, "ip", config.ftp.ip);
    config.ftp.port = readInt(ftp, "port", config.ftp.port);
    config.ftp.user = readString(ftp, "user", config.ftp.user);
    config.ftp.password = readString(ftp, "password", config.ftp.password);

    const QJsonObject plc = root.value("plc").toObject();
    config.plc.ip = readString(plc, "ip", config.plc.ip);
    config.plc.rack = readInt(plc, "rack", config.plc.rack);
    config.plc.slot = readInt(plc, "slot", config.plc.slot);

    const QJsonObject thermal = root.value("thermal_camera").toObject();
    config.thermalCamera.ip = readString(thermal, "ip", config.thermalCamera.ip);
    config.thermalCamera.port = readInt(thermal, "port", config.thermalCamera.port);
    config.thermalCamera.user = readString(thermal, "user", config.thermalCamera.user);
    config.thermalCamera.password = readString(thermal, "password", config.thermalCamera.password);

    const QJsonObject serial = root.value("serial").toObject();
    config.serial.generalBaudrate = readInt(serial, "general_baudrate", config.serial.generalBaudrate);
    const QJsonObject fiberLaser = serial.value("fiber_laser").toObject();
    config.serial.fiberLaserPort = readString(fiberLaser, "port", config.serial.fiberLaserPort);
    config.serial.fiberLaserBaudrate = readInt(fiberLaser, "baudrate", config.serial.fiberLaserBaudrate);

    config.loaded_ = true;
    return config;
}

bool AppConfig::isLoaded() const {
    return loaded_;
}

QStringList AppConfig::warnings() const {
    return warnings_;
}
```

- [ ] **Step 5: Run config tests and verify they pass**

Run:

```powershell
cmake --build build --target test_app_config --config Release
ctest --test-dir build -C Release -R test_app_config --output-on-failure
```

Expected: `test_app_config` passes.

- [ ] **Step 6: Commit config loader**

Run:

```powershell
git add src/app/AppConfig.h src/app/AppConfig.cpp tests/test_app_config.cpp
git commit -m "feat: load runtime config"
```

Expected: commit succeeds if `AFP_ABB_CPP` is a git repository. If it is not a git repository, skip the commit and note that in the final implementation summary.

---

### Task 4: Add Logging And Placeholder Services

**Files:**
- Create: `src/app/StatusLogger.h`
- Create: `src/app/StatusLogger.cpp`
- Create: `src/services/PlaceholderServices.h`
- Create: `src/services/PlaceholderServices.cpp`

- [ ] **Step 1: Implement `StatusLogger.h`**

Create `src/app/StatusLogger.h` with:

```cpp
#pragma once

class StatusLogger {
public:
    static void install();
};
```

- [ ] **Step 2: Implement `StatusLogger.cpp`**

Create `src/app/StatusLogger.cpp` with:

```cpp
#include "app/StatusLogger.h"

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QTextStream>

namespace {
QFile logFile;

void messageHandler(QtMsgType type, const QMessageLogContext&, const QString& message) {
    const QString level = [type]() {
        switch (type) {
        case QtDebugMsg: return QString("DEBUG");
        case QtInfoMsg: return QString("INFO");
        case QtWarningMsg: return QString("WARNING");
        case QtCriticalMsg: return QString("ERROR");
        case QtFatalMsg: return QString("FATAL");
        }
        return QString("INFO");
    }();

    const QString line = QString("%1 %2 %3\n")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
        .arg(level)
        .arg(message);

    if (logFile.isOpen()) {
        QTextStream stream(&logFile);
        stream << line;
        stream.flush();
    }

    QTextStream(stderr) << line;
}
}

void StatusLogger::install() {
    logFile.setFileName("afp_abb_cpp.log");
    logFile.open(QIODevice::Append | QIODevice::Text);
    qInstallMessageHandler(messageHandler);
}
```

- [ ] **Step 3: Implement placeholder service header**

Create `src/services/PlaceholderServices.h` with:

```cpp
#pragma once

#include <QObject>
#include <QString>

class PlaceholderService : public QObject {
    Q_OBJECT
public:
    explicit PlaceholderService(QString serviceName, QObject* parent = nullptr);

signals:
    void messageRequested(const QString& title, const QString& message);

protected:
    void reportNotConverted(const QString& action);

private:
    QString serviceName_;
};

class SerialService final : public PlaceholderService {
    Q_OBJECT
public:
    explicit SerialService(QObject* parent = nullptr);
public slots:
    void portCheck();
    void portOpen();
    void portClose();
};

class RobotService final : public PlaceholderService {
    Q_OBJECT
public:
    explicit RobotService(QObject* parent = nullptr);
public slots:
    void connectRobot();
    void moveRobot();
    void resetRobot();
    void moveRobotAxes();
    void moveExternalAxis();
    void resetExternalAxis();
    void startPrint();
    void pausePrint();
    void continuePrint();
    void stopPrint();
};

class FtpService final : public PlaceholderService {
    Q_OBJECT
public:
    explicit FtpService(QObject* parent = nullptr);
public slots:
    void openLocus();
};

class PLCService final : public PlaceholderService {
    Q_OBJECT
public:
    explicit PLCService(QObject* parent = nullptr);
public slots:
    void disconnectPlc();
    void cylinder1();
    void cylinder2();
    void cylinder3();
    void motorEnable();
    void motorReset();
    void motorActuation();
    void parameterInput();
    void laserConnect();
    void setTargetTemperature();
    void setLaserPower();
    void toggleTemperatureControl();
    void togglePowerControl();
    void openRedLight();
    void closeRedLight();
    void laserStart();
    void laserStop();
    void setHysteresisRatio();
};

class PressureService final : public PlaceholderService {
    Q_OBJECT
public:
    explicit PressureService(QObject* parent = nullptr);
public slots:
    void startCollection();
    void stopCollection();
    void showData();
};

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

class ThermalCameraService final : public PlaceholderService {
    Q_OBJECT
public:
    explicit ThermalCameraService(QObject* parent = nullptr);
public slots:
    void startCamera();
    void stopCamera();
    void showData();
    void clearCache();
};
```

- [ ] **Step 4: Implement placeholder service source**

Create `src/services/PlaceholderServices.cpp` with:

```cpp
#include "services/PlaceholderServices.h"

#include <utility>

PlaceholderService::PlaceholderService(QString serviceName, QObject* parent)
    : QObject(parent), serviceName_(std::move(serviceName)) {
}

void PlaceholderService::reportNotConverted(const QString& action) {
    emit messageRequested(serviceName_, QString("%1 is not converted in Slice A.").arg(action));
}

SerialService::SerialService(QObject* parent) : PlaceholderService("Serial", parent) {}
void SerialService::portCheck() { reportNotConverted("Serial port scan"); }
void SerialService::portOpen() { reportNotConverted("Serial port open"); }
void SerialService::portClose() { reportNotConverted("Serial port close"); }

RobotService::RobotService(QObject* parent) : PlaceholderService("Robot", parent) {}
void RobotService::connectRobot() { reportNotConverted("Robot connection"); }
void RobotService::moveRobot() { reportNotConverted("Robot move"); }
void RobotService::resetRobot() { reportNotConverted("Robot reset"); }
void RobotService::moveRobotAxes() { reportNotConverted("Robot axis move"); }
void RobotService::moveExternalAxis() { reportNotConverted("External axis move"); }
void RobotService::resetExternalAxis() { reportNotConverted("External axis reset"); }
void RobotService::startPrint() { reportNotConverted("Start print"); }
void RobotService::pausePrint() { reportNotConverted("Pause print"); }
void RobotService::continuePrint() { reportNotConverted("Continue print"); }
void RobotService::stopPrint() { reportNotConverted("Stop print"); }

FtpService::FtpService(QObject* parent) : PlaceholderService("FTP", parent) {}
void FtpService::openLocus() { reportNotConverted("Locus file selection"); }

PLCService::PLCService(QObject* parent) : PlaceholderService("PLC", parent) {}
void PLCService::disconnectPlc() { reportNotConverted("PLC disconnect"); }
void PLCService::cylinder1() { reportNotConverted("Cylinder 1 control"); }
void PLCService::cylinder2() { reportNotConverted("Cylinder 2 control"); }
void PLCService::cylinder3() { reportNotConverted("Cylinder 3 control"); }
void PLCService::motorEnable() { reportNotConverted("Motor enable"); }
void PLCService::motorReset() { reportNotConverted("Motor reset"); }
void PLCService::motorActuation() { reportNotConverted("Motor actuation"); }
void PLCService::parameterInput() { reportNotConverted("PLC parameter input"); }
void PLCService::laserConnect() { reportNotConverted("Laser connection"); }
void PLCService::setTargetTemperature() { reportNotConverted("Target temperature setting"); }
void PLCService::setLaserPower() { reportNotConverted("Laser power setting"); }
void PLCService::toggleTemperatureControl() { reportNotConverted("Temperature control toggle"); }
void PLCService::togglePowerControl() { reportNotConverted("Power control toggle"); }
void PLCService::openRedLight() { reportNotConverted("Red light open"); }
void PLCService::closeRedLight() { reportNotConverted("Red light close"); }
void PLCService::laserStart() { reportNotConverted("Laser start"); }
void PLCService::laserStop() { reportNotConverted("Laser stop"); }
void PLCService::setHysteresisRatio() { reportNotConverted("Hysteresis ratio setting"); }

PressureService::PressureService(QObject* parent) : PlaceholderService("Pressure", parent) {}
void PressureService::startCollection() { reportNotConverted("Pressure collection start"); }
void PressureService::stopCollection() { reportNotConverted("Pressure collection stop"); }
void PressureService::showData() { reportNotConverted("Pressure data display"); }

HighSpeedCameraService::HighSpeedCameraService(QObject* parent) : PlaceholderService("High-speed camera", parent) {}
void HighSpeedCameraService::enumDevices() { reportNotConverted("Camera enumeration"); }
void HighSpeedCameraService::openCamera() { reportNotConverted("Camera open"); }
void HighSpeedCameraService::flawPredict() { reportNotConverted("Flaw prediction"); }
void HighSpeedCameraService::startGrabbing() { reportNotConverted("Camera grabbing"); }
void HighSpeedCameraService::stopCamera() { reportNotConverted("Camera stop"); }
void HighSpeedCameraService::getParameter() { reportNotConverted("Camera parameter read"); }
void HighSpeedCameraService::setParameter() { reportNotConverted("Camera parameter write"); }

ThermalCameraService::ThermalCameraService(QObject* parent) : PlaceholderService("Thermal camera", parent) {}
void ThermalCameraService::startCamera() { reportNotConverted("Thermal camera start"); }
void ThermalCameraService::stopCamera() { reportNotConverted("Thermal camera stop"); }
void ThermalCameraService::showData() { reportNotConverted("Thermal data display"); }
void ThermalCameraService::clearCache() { reportNotConverted("Thermal cache clear"); }
```

- [ ] **Step 5: Build placeholder services**

Run:

```powershell
cmake --build build --target AFP_ABB_CPP --config Release
```

Expected: build still fails until `main.cpp` and `MainWindow` are created, but no syntax errors should be reported in placeholder service files when they are compiled later.

---

### Task 5: Implement Main Window Shell

**Files:**
- Create: `src/main.cpp`
- Create: `src/ui/MainWindow.h`
- Create: `src/ui/MainWindow.cpp`

- [ ] **Step 1: Create `MainWindow.h`**

Create `src/ui/MainWindow.h` with:

```cpp
#pragma once

#include <QWidget>
#include "app/AppConfig.h"
#include "app/AppPaths.h"
#include "services/PlaceholderServices.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ContinuosfiberInterface; }
QT_END_NAMESPACE

class MainWindow final : public QWidget {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void showPlaceholderMessage(const QString& title, const QString& message);
    void updateMotorStateFromJog();
    void updateMotorStateFromContinuous();

private:
    void initializeWindowChrome();
    void initializeState();
    void initializeUiDefaults();
    void connectSignals();
    void centerOnScreen();

    Ui::ContinuosfiberInterface* ui_;
    AppPaths paths_;
    AppConfig config_;

    bool linkState_ = false;
    bool pauseFlag_ = false;
    bool stopFlag_ = true;
    bool plcConnected_ = false;
    bool motorEnabled_ = false;
    bool redLight_ = false;
    bool laserState_ = false;
    bool targetSet_ = false;
    bool temperatureControl_ = false;
    bool powerControl_ = false;

    double robotLineSpeed_ = 200.0;
    double robotOrientationSpeed_ = 200.0;
    double externalAxisLineSpeed_ = 200.0;
    double externalAxisOrientationSpeed_ = 100.0;
    double pavingDistance_ = 1.0;
    double wireFeedRate_ = 10.0;
    int hysteresisRatio_ = 200;

    SerialService serialService_;
    RobotService robotService_;
    FtpService ftpService_;
    PLCService plcService_;
    PressureService pressureService_;
    HighSpeedCameraService highSpeedCameraService_;
    ThermalCameraService thermalCameraService_;
};
```

- [ ] **Step 2: Create `MainWindow.cpp`**

Create `src/ui/MainWindow.cpp` with:

```cpp
#include "ui/MainWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QGuiApplication>
#include <QIcon>
#include <QMessageBox>
#include <QPushButton>
#include <QScreen>
#include <QTextBrowser>
#include <QTimer>
#include "ui_Interface4600_window.h"

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent),
      ui_(new Ui::ContinuosfiberInterface),
      paths_(QApplication::applicationDirPath()),
      config_(AppConfig::load(paths_)),
      serialService_(this),
      robotService_(this),
      ftpService_(this),
      plcService_(this),
      pressureService_(this),
      highSpeedCameraService_(this),
      thermalCameraService_(this) {
    ui_->setupUi(this);
    initializeWindowChrome();
    initializeState();
    initializeUiDefaults();
    connectSignals();
    centerOnScreen();
}

MainWindow::~MainWindow() {
    delete ui_;
}

void MainWindow::initializeWindowChrome() {
    setWindowTitle(QStringLiteral("机器人面向铺带3D打印软件平台"));
    if (paths_.iconFilePath().exists()) {
        setWindowIcon(QIcon(paths_.iconFilePath().absoluteFilePath()));
    }
}

void MainWindow::initializeState() {
    linkState_ = false;
    pauseFlag_ = false;
    stopFlag_ = true;
    plcConnected_ = false;
    motorEnabled_ = false;
    redLight_ = false;
    laserState_ = false;
    targetSet_ = false;
    temperatureControl_ = false;
    powerControl_ = false;

    robotLineSpeed_ = 200.0;
    robotOrientationSpeed_ = 200.0;
    externalAxisLineSpeed_ = 200.0;
    externalAxisOrientationSpeed_ = 100.0;
    pavingDistance_ = 1.0;
    wireFeedRate_ = 10.0;
    hysteresisRatio_ = 200;
}

void MainWindow::initializeUiDefaults() {
    ui_->Other_tabWidget_Robotplatform->setCurrentIndex(0);

    ui_->ConnectionPLCButton->setVisible(false);
    ui_->ConnectionPLCButton->setEnabled(false);

    ui_->robot_state_show->setWordWrap(true);
    ui_->robot_state_show->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui_->robot_state_show->setText(config_.isLoaded()
        ? QStringLiteral("Config loaded. Hardware not connected in Slice A.")
        : QStringLiteral("Config missing or invalid. Hardware not connected in Slice A."));

    if (!config_.warnings().isEmpty()) {
        ui_->robot_state_show->setToolTip(config_.warnings().join('\n'));
    }
}

void MainWindow::connectSignals() {
    const auto connectPlaceholder = [this](PlaceholderService& service) {
        connect(&service, &PlaceholderService::messageRequested,
                this, &MainWindow::showPlaceholderMessage);
    };

    connectPlaceholder(serialService_);
    connectPlaceholder(robotService_);
    connectPlaceholder(ftpService_);
    connectPlaceholder(plcService_);
    connectPlaceholder(pressureService_);
    connectPlaceholder(highSpeedCameraService_);
    connectPlaceholder(thermalCameraService_);

    connect(ui_->ChoosepushButton, &QPushButton::clicked, &serialService_, &SerialService::portCheck);
    connect(ui_->ConmunicationConnectionButton, &QPushButton::clicked, &serialService_, &SerialService::portOpen);
    connect(ui_->CloseSerialpushButton, &QPushButton::clicked, &serialService_, &SerialService::portClose);

    connect(ui_->ConnectionRobotButton, &QPushButton::clicked, &robotService_, &RobotService::connectRobot);
    connect(ui_->Robot_movebutton, &QPushButton::clicked, &robotService_, &RobotService::moveRobot);
    connect(ui_->RobotGohomeButton, &QPushButton::clicked, &robotService_, &RobotService::resetRobot);
    connect(ui_->RobotaxismoveButton, &QPushButton::clicked, &robotService_, &RobotService::moveRobotAxes);
    connect(ui_->PositionermoveButton, &QPushButton::clicked, &robotService_, &RobotService::moveExternalAxis);
    connect(ui_->PositionercomebackButton, &QPushButton::clicked, &robotService_, &RobotService::resetExternalAxis);
    connect(ui_->Robot_movebutton_Gcode, &QPushButton::clicked, &robotService_, &RobotService::startPrint);
    connect(ui_->PauseRobotButton, &QPushButton::clicked, &robotService_, &RobotService::pausePrint);
    connect(ui_->ContinueRobotButton, &QPushButton::clicked, &robotService_, &RobotService::continuePrint);
    connect(ui_->StopRobotButton, &QPushButton::clicked, &robotService_, &RobotService::stopPrint);

    connect(ui_->locusselectsetButton, &QPushButton::clicked, &ftpService_, &FtpService::openLocus);

    connect(ui_->DisconnectPLCButton, &QPushButton::clicked, &plcService_, &PLCService::disconnectPlc);
    connect(ui_->cylinder1_button, &QPushButton::clicked, &plcService_, &PLCService::cylinder1);
    connect(ui_->cylinder2_button, &QPushButton::clicked, &plcService_, &PLCService::cylinder2);
    connect(ui_->cylinder3_button, &QPushButton::clicked, &plcService_, &PLCService::cylinder3);
    connect(ui_->motor_enable, &QPushButton::clicked, &plcService_, &PLCService::motorEnable);
    connect(ui_->motor_reset, &QPushButton::clicked, &plcService_, &PLCService::motorReset);
    connect(ui_->motor_actuation, &QPushButton::clicked, &plcService_, &PLCService::motorActuation);
    connect(ui_->parameter_input, &QPushButton::clicked, &plcService_, &PLCService::parameterInput);
    connect(ui_->laser_connect, &QPushButton::clicked, &plcService_, &PLCService::laserConnect);
    connect(ui_->set_target_temperature, &QPushButton::clicked, &plcService_, &PLCService::setTargetTemperature);
    connect(ui_->set_laser_power, &QPushButton::clicked, &plcService_, &PLCService::setLaserPower);
    connect(ui_->temperature_control, &QPushButton::clicked, &plcService_, &PLCService::toggleTemperatureControl);
    connect(ui_->set_laser_power_control, &QPushButton::clicked, &plcService_, &PLCService::togglePowerControl);
    connect(ui_->redlight_open, &QPushButton::clicked, &plcService_, &PLCService::openRedLight);
    connect(ui_->redlight_close, &QPushButton::clicked, &plcService_, &PLCService::closeRedLight);
    connect(ui_->laser_open_button, &QPushButton::clicked, &plcService_, &PLCService::laserStart);
    connect(ui_->laser_close_button, &QPushButton::clicked, &plcService_, &PLCService::laserStop);
    connect(ui_->Hysteresis_ratio_input, &QPushButton::clicked, &plcService_, &PLCService::setHysteresisRatio);

    connect(ui_->interruption, &QCheckBox::clicked, this, &MainWindow::updateMotorStateFromJog);
    connect(ui_->continuous, &QCheckBox::clicked, this, &MainWindow::updateMotorStateFromContinuous);

    connect(ui_->enum_cam, &QPushButton::clicked, &highSpeedCameraService_, &HighSpeedCameraService::enumDevices);
    connect(ui_->HC_machine, &QPushButton::clicked, &highSpeedCameraService_, &HighSpeedCameraService::openCamera);
    connect(ui_->HC_data, &QPushButton::clicked, &highSpeedCameraService_, &HighSpeedCameraService::flawPredict);
    connect(ui_->HC_start, &QPushButton::clicked, &highSpeedCameraService_, &HighSpeedCameraService::startGrabbing);
    connect(ui_->HC_stop, &QPushButton::clicked, &highSpeedCameraService_, &HighSpeedCameraService::stopCamera);
    connect(ui_->get_parameter, &QPushButton::clicked, &highSpeedCameraService_, &HighSpeedCameraService::getParameter);
    connect(ui_->set_parameter, &QPushButton::clicked, &highSpeedCameraService_, &HighSpeedCameraService::setParameter);

    connect(ui_->TIC_machine, &QPushButton::clicked, &thermalCameraService_, &ThermalCameraService::startCamera);
    connect(ui_->TIC_stop, &QPushButton::clicked, &thermalCameraService_, &ThermalCameraService::stopCamera);
    connect(ui_->TIC_start, &QPushButton::clicked, &thermalCameraService_, &ThermalCameraService::clearCache);
    connect(ui_->TIC_data, &QPushButton::clicked, &thermalCameraService_, &ThermalCameraService::showData);

    connect(ui_->FC_data, &QPushButton::clicked, &pressureService_, &PressureService::showData);
    connect(ui_->FC_start, &QPushButton::clicked, &pressureService_, &PressureService::startCollection);
    connect(ui_->FC_stop, &QPushButton::clicked, &pressureService_, &PressureService::stopCollection);
}

void MainWindow::centerOnScreen() {
    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        const QRect area = screen->availableGeometry();
        move(area.center() - rect().center());
    }
}

void MainWindow::showPlaceholderMessage(const QString& title, const QString& message) {
    QMessageBox::information(this, title, message);
}

void MainWindow::updateMotorStateFromJog() {
    if (ui_->interruption->isChecked()) {
        ui_->continuous->setChecked(false);
        ui_->state_show->setText(QStringLiteral("点动状态"));
    }
}

void MainWindow::updateMotorStateFromContinuous() {
    if (ui_->continuous->isChecked()) {
        ui_->interruption->setChecked(false);
        ui_->state_show->setText(QStringLiteral("连续状态"));
    }
}
```

- [ ] **Step 3: Create `main.cpp`**

Create `src/main.cpp` with:

```cpp
#include <QApplication>
#include "app/StatusLogger.h"
#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    #endif
    QApplication app(argc, argv);

    StatusLogger::install();

    MainWindow window;
    window.show();

    return app.exec();
}
```

- [ ] **Step 4: Build the executable**

Run:

```powershell
cmake --build build --target AFP_ABB_CPP --config Release
```

Expected: build succeeds and produces `build/Release/AFP_ABB_CPP.exe`.

- [ ] **Step 5: Run the executable locally**

Run:

```powershell
.\build\Release\AFP_ABB_CPP.exe
```

Expected: the original UI opens with the Python design, the window title is Chinese as in the Python app, and placeholder buttons show "not converted in Slice A" messages without crashing.

- [ ] **Step 6: Commit main-window shell**

Run:

```powershell
git add src/main.cpp src/ui/MainWindow.h src/ui/MainWindow.cpp src/app/StatusLogger.h src/app/StatusLogger.cpp src/services/PlaceholderServices.h src/services/PlaceholderServices.cpp
git commit -m "feat: add Qt main window shell"
```

Expected: commit succeeds if `AFP_ABB_CPP` is a git repository. If it is not a git repository, skip the commit and note that in the final implementation summary.

---

### Task 6: Add Portable Deployment Script

**Files:**
- Create: `cmake/DeployQt.cmake`
- Modify: `CMakeLists.txt`

- [ ] **Step 1: Create `DeployQt.cmake`**

Create `cmake/DeployQt.cmake` with:

```cmake
if(NOT WIN32)
    message(FATAL_ERROR "Portable deployment is currently defined only for Windows.")
endif()

if(NOT DEFINED QT_DEPLOY_EXECUTABLE)
    message(FATAL_ERROR "QT_DEPLOY_EXECUTABLE is required.")
endif()

if(NOT EXISTS "${QT_DEPLOY_EXECUTABLE}")
    message(FATAL_ERROR "Executable does not exist: ${QT_DEPLOY_EXECUTABLE}")
endif()

get_filename_component(_qt_bin_dir "${QT_DEPLOY_EXECUTABLE}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE
    NAMES windeployqt windeployqt.exe
    HINTS
        "$ENV{QTDIR}/bin"
        "C:/Qt/6.9.1/msvc2022_64/bin"
)

if(NOT WINDEPLOYQT_EXECUTABLE)
    message(FATAL_ERROR "windeployqt was not found. Add Qt bin to PATH or set QTDIR.")
endif()

execute_process(
    COMMAND "${WINDEPLOYQT_EXECUTABLE}" --release --compiler-runtime "${QT_DEPLOY_EXECUTABLE}"
    RESULT_VARIABLE _deploy_result
)

if(NOT _deploy_result EQUAL 0)
    message(FATAL_ERROR "windeployqt failed with exit code ${_deploy_result}")
endif()
```

- [ ] **Step 2: Reconfigure after adding the deployment helper**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.9.1\msvc2022_64
```

Expected: configure succeeds.

- [ ] **Step 3: Build deployment target**

Run:

```powershell
cmake --build build --target deploy_portable --config Release
```

Expected: `windeployqt` copies Qt DLLs and `platforms/qwindows.dll` into `build/Release`.

- [ ] **Step 4: Verify key portable files exist**

Run:

```powershell
Test-Path .\build\Release\AFP_ABB_CPP.exe
Test-Path .\build\Release\Qt6Core.dll
Test-Path .\build\Release\Qt6Widgets.dll
Test-Path .\build\Release\platforms\qwindows.dll
Test-Path .\build\Release\config\env_config.json
Test-Path .\build\Release\assets\icons\icon.png
```

Expected: every command prints `True`.

- [ ] **Step 5: Run executable from deployed folder**

Run:

```powershell
.\build\Release\AFP_ABB_CPP.exe
```

Expected: app opens directly from `build/Release` without Python.

---

### Task 7: Replace README With Full Build And Use Documentation

**Files:**
- Modify: `README.md`

- [ ] **Step 1: Write full README**

Replace `README.md` with:

```markdown
# AFP_ABB_CPP

This is the C++ migration of the AFP ABB Python PC software.

Slice A provides the Qt Widgets application shell, keeps the original UI design, loads runtime configuration, and builds a portable Windows application folder. It does not communicate with the PLC, ABB robot, cameras, FTP server, serial devices, or pressure sensor yet.

## What Slice A Contains

- C++17 Qt Widgets executable.
- Original UI copied from `AFP_ABB_Python/src/ui/Interface4600_window.ui`.
- Runtime config copied from `AFP_ABB_Python/config`.
- Placeholder service classes for future hardware slices.
- Portable deployment using Qt `windeployqt`.

## What Slice A Does Not Convert

- Siemens S7-1200 PLC communication.
- ABB IRB4600 robot communication.
- FTP upload/download.
- Serial port control.
- Pressure sensor DLL acquisition.
- High-speed camera SDK calls.
- Thermal camera SDK calls.
- RAPID files.

## Required Build Tools

Install these on the build computer:

- Windows 10 or Windows 11.
- Visual Studio 2022 Build Tools with C++ workload.
- CMake 3.20 or newer.
- Qt 6 for MSVC 2022 64-bit, for example `C:\Qt\6.9.1\msvc2022_64`.

The packaged application folder produced by this project does not require these tools on the target computer.

## Configure

From this folder:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.9.1\msvc2022_64
```

If Qt is installed elsewhere, replace `C:\Qt\6.9.1\msvc2022_64` with your Qt installation path.

## Build

```powershell
cmake --build build --config Release
```

The executable is created at:

```text
build\Release\AFP_ABB_CPP.exe
```

## Run On The Build Computer

```powershell
.\build\Release\AFP_ABB_CPP.exe
```

The app should open the original AFP ABB UI. Hardware buttons show placeholder messages because hardware behavior is intentionally not converted in Slice A.

## Run Tests

```powershell
ctest --test-dir build -C Release --output-on-failure
```

## Create Portable Windows Folder

Build the deployment target:

```powershell
cmake --build build --target deploy_portable --config Release
```

This runs Qt `windeployqt` and copies the required Qt runtime files into `build\Release`.

Before copying to another computer, confirm these files exist:

```text
build\Release\AFP_ABB_CPP.exe
build\Release\Qt6Core.dll
build\Release\Qt6Gui.dll
build\Release\Qt6Widgets.dll
build\Release\platforms\qwindows.dll
build\Release\config\env_config.json
build\Release\config\logging_config.json
build\Release\assets\icons\icon.png
```

Copy the entire `build\Release` folder to another compatible Windows computer. Run `AFP_ABB_CPP.exe` directly from that copied folder. Python, CMake, Visual Studio, and Qt do not need to be installed on the target computer.

## Runtime Configuration

The executable reads:

```text
config\env_config.json
```

relative to the executable folder. Edit the copied config file before running on a machine with different PLC, robot, camera, FTP, or serial settings.

## Future Conversion Slices

Recommended next slices:

1. PLC communication and PLC-controlled laser/motor/cylinder functions.
2. Pressure sensor acquisition and plotting.
3. ABB robot and FTP workflow.
4. High-speed camera workflow.
5. Thermal camera workflow.
```

- [ ] **Step 2: Verify README commands**

Run each README command on the build computer:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.9.1\msvc2022_64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --build build --target deploy_portable --config Release
```

Expected: all commands succeed.

- [ ] **Step 3: Commit README and deploy script**

Run:

```powershell
git add README.md cmake/DeployQt.cmake CMakeLists.txt
git commit -m "docs: document portable Windows build"
```

Expected: commit succeeds if `AFP_ABB_CPP` is a git repository. If it is not a git repository, skip the commit and note that in the final implementation summary.

---

### Task 8: Final Verification

**Files:**
- Verify: all Slice A files
- Do not modify: `AFP_ABB_Python/**`

- [ ] **Step 1: Confirm Python project was not modified**

Run from `AFP_ABB_Python`:

```powershell
git status --short
```

Expected: no output.

- [ ] **Step 2: Confirm C++ project files exist**

Run from `AFP_ABB_CPP`:

```powershell
Test-Path .\CMakeLists.txt
Test-Path .\src\main.cpp
Test-Path .\src\ui\MainWindow.cpp
Test-Path .\ui\Interface4600_window.ui
Test-Path .\config\env_config.json
Test-Path .\assets\icons\icon.png
Test-Path .\README.md
```

Expected: every command prints `True`.

- [ ] **Step 3: Run tests**

Run from `AFP_ABB_CPP`:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

Expected: `test_app_paths` and `test_app_config` pass.

- [ ] **Step 4: Build portable folder**

Run:

```powershell
cmake --build build --target deploy_portable --config Release
```

Expected: deployment succeeds.

- [ ] **Step 5: Check portable folder contents**

Run:

```powershell
Get-ChildItem .\build\Release
Get-ChildItem .\build\Release\platforms
Get-ChildItem .\build\Release\config
Get-ChildItem .\build\Release\assets\icons
```

Expected: the `.exe`, Qt DLLs, `platforms/qwindows.dll`, config files, and icon are present.

- [ ] **Step 6: Run the deployed executable**

Run:

```powershell
.\build\Release\AFP_ABB_CPP.exe
```

Expected:

- The original UI opens.
- The title is `机器人面向铺带3D打印软件平台`.
- No Python process is launched.
- Pressing placeholder-connected buttons shows a Slice A message.
- Closing the app exits cleanly.

- [ ] **Step 7: Final implementation summary**

Record:

- Build command output summary.
- Test output summary.
- Portable deployment folder path.
- Whether `AFP_ABB_CPP` is a git repository and whether commits were made.
- Confirmation that `AFP_ABB_Python` remained unchanged.
