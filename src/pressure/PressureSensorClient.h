#pragma once

#include <QLibrary>
#include <QString>
#include <QtGlobal>

#if defined(Q_OS_WIN)
#define AFP_ABB_PRESSURE_CALL __stdcall
#else
#define AFP_ABB_PRESSURE_CALL
#endif

// Thin wrapper around the pressure vendor DLL used by pressure_reader.py.
// It deliberately does not know about Qt widgets, DDS, or acquisition timing;
// its only job is loading the DLL, resolving the three exports, and reading one
// raw pressure value.
class PressureSensorClient final {
public:
    explicit PressureSensorClient(QString dllPath);
    ~PressureSensorClient();

    bool load();
    bool start();
    bool read(double& value);
    void stop();

    bool loaded() const;
    bool running() const;
    QString lastError() const;

private:
    // Python uses ctypes.WinDLL, which means stdcall on Windows. Matching that
    // calling convention avoids stack corruption when calling vendor exports.
    using WorkStartFn = int(AFP_ABB_PRESSURE_CALL*)();
    using WorkStopFn = int(AFP_ABB_PRESSURE_CALL*)();
    using CatchDataFn = double(AFP_ABB_PRESSURE_CALL*)();

    bool configureDllSearchPath();
    bool resolveFunctions();
    void setError(const QString& message);

    QString dllPath_;
    QLibrary library_;
    WorkStartFn workStart_ = nullptr;
    WorkStopFn workStop_ = nullptr;
    CatchDataFn catchData_ = nullptr;
    bool running_ = false;
    QString lastError_;
};

#undef AFP_ABB_PRESSURE_CALL
