#include "pressure/PressureSensorClient.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QThread>

#if defined(Q_OS_WIN)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <iostream>

namespace {
constexpr int kDefaultSamples = 80;
constexpr int kDefaultIntervalMs = 100;

struct ProbeOptions {
    int samples = kDefaultSamples;
    int intervalMs = kDefaultIntervalMs;
};

QString readDaq1Value(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString("not readable: %1").arg(file.errorString());
    }

    const QString text = QString::fromLocal8Bit(file.readAll());
    const QRegularExpression expression("DAQ-1\\s*=\\s*\"([^\"]*)\"");
    const QRegularExpressionMatch match = expression.match(text);
    if (!match.hasMatch()) {
        return "DAQ-1 not found";
    }
    return match.captured(1);
}

void printEnvironment(const QString& appDir, const QString& dllPath) {
    const QString rootConfig = QDir(appDir).absoluteFilePath("vk70xmcdllconfig.ini");
    const QString nestedConfig = QDir(appDir).absoluteFilePath("lib/sensors/pressure_sensor/vk70xmcdllconfig.ini");

    std::cout << "=== Pressure probe environment ===\n";
    std::cout << "application_dir: " << appDir.toLocal8Bit().constData() << "\n";
    std::cout << "current_dir: " << QDir::currentPath().toLocal8Bit().constData() << "\n";
    std::cout << "pressure_dll: " << dllPath.toLocal8Bit().constData() << "\n";
    std::cout << "pressure_dll_exists: " << (QFileInfo::exists(dllPath) ? "true" : "false") << "\n";
    std::cout << "root_vk70_DAQ-1: " << readDaq1Value(rootConfig).toLocal8Bit().constData() << "\n";
    std::cout << "nested_vk70_DAQ-1: " << readDaq1Value(nestedConfig).toLocal8Bit().constData() << "\n";
}

ProbeOptions parseOptions(const QStringList& arguments) {
    ProbeOptions options;
    for (int i = 1; i < arguments.size(); ++i) {
        if (arguments[i] == "--samples" && i + 1 < arguments.size()) {
            bool ok = false;
            const int value = arguments[++i].toInt(&ok);
            if (ok && value > 0) {
                options.samples = value;
            }
        } else if (arguments[i] == "--interval-ms" && i + 1 < arguments.size()) {
            bool ok = false;
            const int value = arguments[++i].toInt(&ok);
            if (ok && value > 0) {
                options.intervalMs = value;
            }
        }
    }
    return options;
}

void printUsage() {
    std::cout << "\nOptional arguments:\n";
    std::cout << "  --samples N       Number of catchdata() samples per method. Default: 80\n";
    std::cout << "  --interval-ms N   Delay between samples. Default: 100\n";
}

void runWrapperMethod(const QString& dllPath, const ProbeOptions& options) {
    std::cout << "\n=== Method A: PressureSensorClient / QLibrary wrapper ===\n";
    PressureSensorClient client(dllPath);
    if (!client.start()) {
        std::cout << "start_failed: " << client.lastError().toLocal8Bit().constData() << "\n";
        return;
    }

    for (int i = 0; i < options.samples; ++i) {
        double value = 0.0;
        if (!client.read(value)) {
            std::cout << i << " read_failed: " << client.lastError().toLocal8Bit().constData() << "\n";
            break;
        }
        std::cout << i << " " << value << "\n";
        QThread::msleep(static_cast<unsigned long>(options.intervalMs));
    }

    client.stop();
}

#if defined(Q_OS_WIN)
void runWinApiMethod(const QString& appDir, const ProbeOptions& options) {
    std::cout << "\n=== Method B: Python-like WinAPI LoadLibraryW from release root ===\n";

    using WorkStartFn = int(__stdcall*)();
    using WorkStopFn = int(__stdcall*)();
    using CatchDataFn = double(__stdcall*)();

    const QString oldCurrentDir = QDir::currentPath();
    QDir::setCurrent(appDir);

    const std::wstring relativeDllPath =
        QDir::toNativeSeparators(QString("lib/sensors/pressure_sensor/pressure.dll")).toStdWString();
    HMODULE library = LoadLibraryW(relativeDllPath.c_str());
    if (!library) {
        std::cout << "load_failed_GetLastError: " << GetLastError() << "\n";
        QDir::setCurrent(oldCurrentDir);
        return;
    }

    auto workStart = reinterpret_cast<WorkStartFn>(GetProcAddress(library, "workstart"));
    auto workStop = reinterpret_cast<WorkStopFn>(GetProcAddress(library, "workstop"));
    auto catchData = reinterpret_cast<CatchDataFn>(GetProcAddress(library, "catchdata"));
    if (!workStart || !workStop || !catchData) {
        std::cout << "resolve_failed: workstart/workstop/catchdata export missing\n";
        FreeLibrary(library);
        QDir::setCurrent(oldCurrentDir);
        return;
    }

    const int startResult = workStart();
    std::cout << "workstart_return: " << startResult << "\n";
    for (int i = 0; i < options.samples; ++i) {
        std::cout << i << " " << catchData() << "\n";
        QThread::msleep(static_cast<unsigned long>(options.intervalMs));
    }

    workStop();
    FreeLibrary(library);
    QDir::setCurrent(oldCurrentDir);
}
#endif
}

// Diagnostic-only executable for comparing C++ and Python-style pressure DLL
// loading. It is intentionally separate from AFP_ABB_CPP.exe so hardware tests
// can isolate pressure.dll behavior without DDS, InfluxDB, or the GUI.
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    const QString appDir = QCoreApplication::applicationDirPath();
    const QString dllPath = QDir(appDir).absoluteFilePath("lib/sensors/pressure_sensor/pressure.dll");
    const ProbeOptions options = parseOptions(QCoreApplication::arguments());

    printEnvironment(appDir, dllPath);
    printUsage();
    runWrapperMethod(dllPath, options);
#if defined(Q_OS_WIN)
    runWinApiMethod(appDir, options);
#else
    std::cout << "\nMethod B is only available on Windows.\n";
#endif

    return 0;
}
