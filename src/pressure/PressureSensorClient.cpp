#include "pressure/PressureSensorClient.h"

#include <QDir>
#include <QFileInfo>

#include <utility>

#if defined(Q_OS_WIN)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

PressureSensorClient::PressureSensorClient(QString dllPath)
    : dllPath_(std::move(dllPath)), library_(dllPath_) {
}

PressureSensorClient::~PressureSensorClient() {
    // The Python reader calls workstop() during shutdown. The C++ wrapper does
    // the same defensively so hardware is not left in acquisition mode if a
    // service object is destroyed while running.
    stop();
}

bool PressureSensorClient::load() {
    if (library_.isLoaded()) {
        return true;
    }

    if (!configureDllSearchPath()) {
        return false;
    }

    if (!library_.load()) {
        setError(QString("Could not load pressure sensor DLL '%1': %2").arg(dllPath_, library_.errorString()));
        return false;
    }

    return resolveFunctions();
}

bool PressureSensorClient::start() {
    if (!load()) {
        return false;
    }

    if (running_) {
        return true;
    }

    // ctypes would see a non-zero integer return as data from the vendor
    // function. Treating non-zero as failure gives the UI a clear diagnostic.
    if (workStart_ && workStart_() != 0) {
        setError("pressure.dll workstart() reported failure.");
        return false;
    }

    running_ = true;
    return true;
}

bool PressureSensorClient::read(double& value) {
    if (!running_ || !catchData_) {
        setError("Pressure sensor is not running.");
        return false;
    }

    // Match pressure_reader.py: catchdata() is interpreted as a double.
    value = catchData_();
    return true;
}

void PressureSensorClient::stop() {
    if (running_ && workStop_) {
        workStop_();
    }
    running_ = false;
}

bool PressureSensorClient::loaded() const {
    return library_.isLoaded();
}

bool PressureSensorClient::running() const {
    return running_;
}

QString PressureSensorClient::lastError() const {
    return lastError_;
}

bool PressureSensorClient::configureDllSearchPath() {
#if defined(Q_OS_WIN)
    const QString dllDirectory = QFileInfo(dllPath_).absoluteDir().absolutePath();
    const std::wstring nativeDirectory = QDir::toNativeSeparators(dllDirectory).toStdWString();

    // pressure.dll imports BD802NHMC_DAQ.dll. Windows does not automatically
    // search the loaded DLL's own folder when QLibrary loads pressure.dll by
    // absolute path, so add the vendor folder to the process DLL search path
    // before calling LoadLibrary through QLibrary.
    if (!SetDllDirectoryW(nativeDirectory.c_str())) {
        setError(QString("Could not add pressure sensor DLL directory to the Windows search path: %1")
                     .arg(dllDirectory));
        return false;
    }
#endif
    return true;
}

bool PressureSensorClient::resolveFunctions() {
    workStart_ = reinterpret_cast<WorkStartFn>(library_.resolve("workstart"));
    workStop_ = reinterpret_cast<WorkStopFn>(library_.resolve("workstop"));
    catchData_ = reinterpret_cast<CatchDataFn>(library_.resolve("catchdata"));

    if (!workStart_ || !workStop_ || !catchData_) {
        setError("pressure.dll is missing the workstart, workstop, or catchdata export.");
        library_.unload();
        return false;
    }

    return true;
}

void PressureSensorClient::setError(const QString& message) {
    lastError_ = message;
}
