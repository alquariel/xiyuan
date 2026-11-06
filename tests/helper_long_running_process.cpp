#include <chrono>
#include <fstream>
#include <string>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {
unsigned long currentProcessId() {
#ifdef _WIN32
    return static_cast<unsigned long>(GetCurrentProcessId());
#else
    return 0;
#endif
}

void writePidFile(const std::string& path) {
    std::ofstream file(path, std::ios::trunc);
    file << currentProcessId();
}

std::string valueAfter(int argc, char* argv[], const std::string& key) {
    for (int index = 1; index + 1 < argc; ++index) {
        if (argv[index] == key) {
            return argv[index + 1];
        }
    }
    return {};
}

void spawnChildProcess(const char* executablePath, const std::string& pidFile) {
#ifdef _WIN32
    std::string commandLine = "\"";
    commandLine += executablePath;
    commandLine += "\" child \"";
    commandLine += pidFile;
    commandLine += "\"";

    STARTUPINFOA startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo = {};
    if (CreateProcessA(
            nullptr,
            commandLine.data(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo)) {
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
    }
#else
    (void)executablePath;
    (void)pidFile;
#endif
}
}

int main(int argc, char* argv[]) {
    if (argc >= 3 && std::string(argv[1]) == "child") {
        writePidFile(argv[2]);
        std::this_thread::sleep_for(std::chrono::seconds(60));
        return 0;
    }

    const std::string pidFile = valueAfter(argc, argv, "--data-dir");
    if (!pidFile.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        spawnChildProcess(argv[0], pidFile);
    }

    std::this_thread::sleep_for(std::chrono::seconds(60));
    return 0;
}
