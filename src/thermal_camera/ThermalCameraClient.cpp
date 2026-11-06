#include "thermal_camera/ThermalCameraClient.h"

#include "thermal_camera/ThermalCameraTypes.h"

#include <QByteArray>
#include <QFile>
#include <QFileInfo>

#include <algorithm>
#include <cstring>
#include <utility>

#if defined(Q_OS_WIN)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace {
constexpr const char* kHcNetSdkDll = "HCNetSDK.dll";
constexpr const char* kPlayCtrlDll = "PlayCtrl.dll";
constexpr const char* kOpenSslCryptoDll = "libcrypto-1_1-x64.dll";
constexpr const char* kOpenSslSslDll = "libssl-1_1-x64.dll";
constexpr unsigned long kNetDvrSysHead = 1;
constexpr unsigned long kNetDvrStreamData = 2;
constexpr unsigned long kPlayM4StreamRealtime = 0;
constexpr unsigned long kDecodedVideoFrameType = 3;
constexpr unsigned long kPlayCtrlBufferSize = 1024 * 1024;
constexpr long kInvalidHandle = -1;
constexpr int kSdkInitComPath = 2;
constexpr int kSdkInitCryptoPath = 3;
constexpr int kSdkInitSslPath = 4;

using Bool = int;
using Byte = unsigned char;

// Only the fields used by the original Python workflow are modelled here. The
// spare byte arrays keep the binary layout compatible with the vendor SDK while
// avoiding a dependency on Hikvision headers during normal C++ builds.
struct NetDvrDeviceInfoV30 {
    Byte serialNumber[48] = {};
    Byte reserved[196] = {};
};

struct NetDvrLocalSdkPath {
    char path[256] = {};
    Byte reserved[128] = {};
};

struct NetDvrPreviewInfo {
    long channel = 0;
    unsigned long streamType = 0;
    unsigned long linkMode = 0;
    void* playWindow = nullptr;
    int blocked = 0;
    int passbackRecord = 0;
    Byte previewMode = 0;
    Byte streamId[32] = {};
    Byte protoType = 0;
    Byte reserved1 = 0;
    Byte videoCodingType = 0;
    unsigned long displayBufferCount = 0;
    Byte npqMode = 0;
    Byte recvMetaData = 0;
    Byte dataType = 0;
    Byte reserved[213] = {};
};

using RealDataCallback = void(THERMAL_CAMERA_STDCALL*)(
    long,
    unsigned long,
    Byte*,
    unsigned long,
    void*);
using DecodeCallback = void(THERMAL_CAMERA_STDCALL*)(
    long,
    char*,
    long,
    ThermalCameraFrameInfo*,
    void*,
    void*);
using NetDvrInitFn = Bool(THERMAL_CAMERA_STDCALL*)();
using NetDvrCleanupFn = Bool(THERMAL_CAMERA_STDCALL*)();
using NetDvrSetSdkInitCfgFn = Bool(THERMAL_CAMERA_STDCALL*)(int, void*);
using NetDvrSetLogToFileFn = Bool(THERMAL_CAMERA_STDCALL*)(unsigned long, const char*, Bool);
using NetDvrLoginV30Fn = long(THERMAL_CAMERA_STDCALL*)(
    char*,
    unsigned short,
    char*,
    char*,
    NetDvrDeviceInfoV30*);
using NetDvrLogoutFn = Bool(THERMAL_CAMERA_STDCALL*)(long);
using NetDvrRealPlayV40Fn = long(THERMAL_CAMERA_STDCALL*)(
    long,
    NetDvrPreviewInfo*,
    RealDataCallback,
    void*);
using NetDvrStopRealPlayFn = Bool(THERMAL_CAMERA_STDCALL*)(long);
using NetDvrGetLastErrorFn = unsigned long(THERMAL_CAMERA_STDCALL*)();
using PlayM4GetPortFn = Bool(THERMAL_CAMERA_STDCALL*)(long*);
using PlayM4FreePortFn = Bool(THERMAL_CAMERA_STDCALL*)(long);
using PlayM4SetStreamOpenModeFn = Bool(THERMAL_CAMERA_STDCALL*)(long, unsigned long);
using PlayM4OpenStreamFn = Bool(THERMAL_CAMERA_STDCALL*)(long, Byte*, unsigned long, unsigned long);
using PlayM4CloseStreamFn = Bool(THERMAL_CAMERA_STDCALL*)(long);
using PlayM4SetDecCallBackExMendFn = Bool(THERMAL_CAMERA_STDCALL*)(
    long,
    DecodeCallback,
    void*,
    unsigned long,
    void*);
using PlayM4PlayFn = Bool(THERMAL_CAMERA_STDCALL*)(long, void*);
using PlayM4StopFn = Bool(THERMAL_CAMERA_STDCALL*)(long);
using PlayM4InputDataFn = Bool(THERMAL_CAMERA_STDCALL*)(long, Byte*, unsigned long);
using PlayM4ConvertToJpegFileFn = Bool(THERMAL_CAMERA_STDCALL*)(
    char*,
    long,
    long,
    long,
    long,
    const char*);
using PlayM4GetLastErrorFn = unsigned long(THERMAL_CAMERA_STDCALL*)(long);

template <typename Function>
Function resolveFunction(const QLibrary& library, const char* name) {
    // Qt exposes resolve() as non-const even though symbol lookup does not
    // change the library handle from the caller's point of view.
    return reinterpret_cast<Function>(const_cast<QLibrary&>(library).resolve(name));
}

QByteArray nativePathBytes(const QString& path) {
    return QDir::toNativeSeparators(path).toLocal8Bit();
}

void copyBytes(char* destination, size_t destinationSize, const QByteArray& source) {
    if (destinationSize == 0) {
        return;
    }

    const size_t bytesToCopy = std::min(destinationSize - 1, static_cast<size_t>(source.size()));
    std::memcpy(destination, source.constData(), bytesToCopy);
    destination[bytesToCopy] = '\0';
}

} // namespace

ThermalCameraClient::ThermalCameraClient(
    ThermalCameraConfig config,
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
    setError(QString());

    if (capturing_) {
        return true;
    }

    if (!frameStore_.ensureDirectories()) {
        setError("Could not create thermal camera frame directories.");
        return false;
    }

    if (!loadRuntime() ||
        !resolveRequiredFunctions() ||
        !configureSdkRuntimePath() ||
        !initializeSdk() ||
        !configureSdkLogging() ||
        !acquirePlayPort() ||
        !login()) {
        stopCapture();
        return false;
    }

    setCallbacksAccepted(true);
    if (!startPreview()) {
        stopCapture();
        return false;
    }

    capturing_ = true;
    return true;
}

void ThermalCameraClient::stopCapture() {
    capturing_ = false;
    setCallbacksAccepted(false);
    waitForCallbacksToFinish();

    if (realPlayHandle_ >= 0) {
        if (auto stopRealPlay = resolveFunction<NetDvrStopRealPlayFn>(hcNetSdk_, "NET_DVR_StopRealPlay")) {
            stopRealPlay(realPlayHandle_);
        }
        realPlayHandle_ = kInvalidHandle;
    }

    if (userId_ >= 0) {
        if (auto logout = resolveFunction<NetDvrLogoutFn>(hcNetSdk_, "NET_DVR_Logout")) {
            logout(userId_);
        }
        userId_ = kInvalidHandle;
    }

    if (playPort_ >= 0) {
        if (auto stop = resolveFunction<PlayM4StopFn>(playCtrl_, "PlayM4_Stop")) {
            stop(playPort_);
        }
        if (auto closeStream = resolveFunction<PlayM4CloseStreamFn>(playCtrl_, "PlayM4_CloseStream")) {
            closeStream(playPort_);
        }
        if (auto freePort = resolveFunction<PlayM4FreePortFn>(playCtrl_, "PlayM4_FreePort")) {
            freePort(playPort_);
        }
        playPort_ = kInvalidHandle;
    }

    if (sdkInitialized_) {
        if (auto cleanup = resolveFunction<NetDvrCleanupFn>(hcNetSdk_, "NET_DVR_Cleanup")) {
            cleanup();
        }
        sdkInitialized_ = false;
    }

    if (playCtrl_.isLoaded()) {
        playCtrl_.unload();
    }
    if (hcNetSdk_.isLoaded()) {
        hcNetSdk_.unload();
    }
    runtimeDirectory_.clear();
}

bool ThermalCameraClient::isCapturing() const {
    return capturing_;
}

QString ThermalCameraClient::lastError() const {
    std::lock_guard<std::mutex> lock(stateMutex_);
    return lastError_;
}

void THERMAL_CAMERA_STDCALL ThermalCameraClient::realDataCallback(
    long realHandle,
    unsigned long dataType,
    unsigned char* buffer,
    unsigned long bufferSize,
    void* userData) {
    Q_UNUSED(realHandle);
    if (userData == nullptr) {
        return;
    }

    // Hikvision invokes this callback from an SDK worker thread. It must not
    // touch Qt widgets; it only feeds stream bytes into PlayCtrl.
    auto* client = static_cast<ThermalCameraClient*>(userData);
    if (!client->enterCallback()) {
        return;
    }
    client->handleRealData(dataType, buffer, bufferSize);
    client->leaveCallback();
}

void THERMAL_CAMERA_STDCALL ThermalCameraClient::decodeCallback(
    long port,
    char* buffer,
    long bufferSize,
    ThermalCameraFrameInfo* frameInfo,
    void* userData,
    void* reserved) {
    Q_UNUSED(reserved);
    if (userData == nullptr || frameInfo == nullptr) {
        return;
    }

    // This callback is also owned by the SDK/PlayCtrl thread. It writes image
    // files only; the UI timer later reads those files on the Qt main thread.
    auto* client = static_cast<ThermalCameraClient*>(userData);
    if (!client->enterCallback()) {
        return;
    }
    client->handleDecodedFrame(port, buffer, bufferSize, *frameInfo);
    client->leaveCallback();
}

QStringList ThermalCameraClient::runtimeDirectories() const {
    QStringList directories;
    directories << QDir::cleanPath(paths_.rootDir().absoluteFilePath(config_.portableSdkDir));

    const QString configuredSdkDir = config_.sdkDir.trimmed();
    if (!configuredSdkDir.isEmpty()) {
        directories << QDir::cleanPath(configuredSdkDir);
    }

    directories.removeDuplicates();
    return directories;
}

bool ThermalCameraClient::runtimeHasRequiredFiles(const QDir& dir) const {
    return QFileInfo::exists(dir.absoluteFilePath(kHcNetSdkDll)) &&
           QFileInfo::exists(dir.absoluteFilePath(kPlayCtrlDll));
}

bool ThermalCameraClient::loadRuntime() {
    if (hcNetSdk_.isLoaded() && playCtrl_.isLoaded()) {
        return true;
    }

    const QStringList directories = runtimeDirectories();
    QStringList searchedDirectories;

    for (const QString& directoryPath : directories) {
        const QDir dir(directoryPath);
        searchedDirectories << dir.absolutePath();

        if (!runtimeHasRequiredFiles(dir)) {
            continue;
        }

        if (!configureDllSearchPath(dir)) {
            return false;
        }

        hcNetSdk_.setFileName(dir.absoluteFilePath(kHcNetSdkDll));
        playCtrl_.setFileName(dir.absoluteFilePath(kPlayCtrlDll));

        const bool loadedHcNetSdk = hcNetSdk_.load();
        const bool loadedPlayCtrl = playCtrl_.load();
#if defined(Q_OS_WIN)
        // SetDllDirectoryW is process-wide. It is needed only while Windows is
        // resolving Hikvision dependent DLLs from the selected runtime folder,
        // then it is cleared so unrelated Qt or hardware libraries are not
        // affected by this thermal-camera load attempt.
        SetDllDirectoryW(nullptr);
#endif
        if (loadedHcNetSdk && loadedPlayCtrl) {
            runtimeDirectory_ = dir.absolutePath();
            setError(QString());
            return true;
        }

        const QString hcNetSdkError = hcNetSdk_.errorString();
        const QString playCtrlError = playCtrl_.errorString();
        stopCapture();
        setError(QString("Could not load thermal camera SDK runtime from '%1'. %2: %3. %4: %5.")
                     .arg(dir.absolutePath(), kHcNetSdkDll, hcNetSdkError, kPlayCtrlDll, playCtrlError));
        return false;
    }

    setError(QString("Could not find thermal camera SDK runtime. Required files: %1, %2. Searched folders:\n%3")
                 .arg(kHcNetSdkDll, kPlayCtrlDll, searchedDirectories.join('\n')));
    return false;
}

bool ThermalCameraClient::resolveRequiredFunctions() {
    const auto requireSymbol = [this](QLibrary& library, const char* dllName, const char* functionName) {
        if (library.resolve(functionName) != nullptr) {
            return true;
        }

        setError(QString("Thermal camera SDK function %1 was not found in %2.")
                     .arg(functionName, dllName));
        return false;
    };

    const char* const hcNetSdkFunctions[] = {
        "NET_DVR_SetSDKInitCfg",
        "NET_DVR_Init",
        "NET_DVR_Cleanup",
        "NET_DVR_SetLogToFile",
        "NET_DVR_Login_V30",
        "NET_DVR_Logout",
        "NET_DVR_RealPlay_V40",
        "NET_DVR_StopRealPlay",
        "NET_DVR_GetLastError",
    };
    for (const char* functionName : hcNetSdkFunctions) {
        if (!requireSymbol(hcNetSdk_, kHcNetSdkDll, functionName)) {
            return false;
        }
    }

    const char* const playCtrlFunctions[] = {
        "PlayM4_GetPort",
        "PlayM4_FreePort",
        "PlayM4_SetStreamOpenMode",
        "PlayM4_OpenStream",
        "PlayM4_CloseStream",
        "PlayM4_SetDecCallBackExMend",
        "PlayM4_Play",
        "PlayM4_Stop",
        "PlayM4_InputData",
        "PlayM4_ConvertToJpegFile",
        "PlayM4_GetLastError",
    };
    for (const char* functionName : playCtrlFunctions) {
        if (!requireSymbol(playCtrl_, kPlayCtrlDll, functionName)) {
            return false;
        }
    }

    return true;
}

bool ThermalCameraClient::configureDllSearchPath(const QDir& dir) {
#if defined(Q_OS_WIN)
    const QString runtimeDirectory = dir.absolutePath();
    const std::wstring nativeDirectory = QDir::toNativeSeparators(runtimeDirectory).toStdWString();
    if (!SetDllDirectoryW(nativeDirectory.c_str())) {
        setError(QString("Could not add thermal camera SDK directory to the Windows DLL search path: %1")
                     .arg(runtimeDirectory));
        return false;
    }
#else
    Q_UNUSED(dir);
#endif
    return true;
}

bool ThermalCameraClient::configureSdkRuntimePath() {
    auto setInitCfg = resolveFunction<NetDvrSetSdkInitCfgFn>(hcNetSdk_, "NET_DVR_SetSDKInitCfg");
    if (setInitCfg == nullptr) {
        setError(QString("Thermal camera SDK function NET_DVR_SetSDKInitCfg was not found in %1.")
                     .arg(kHcNetSdkDll));
        return false;
    }

    NetDvrLocalSdkPath sdkPath;
    copyBytes(sdkPath.path, sizeof(sdkPath.path), nativePathBytes(runtimeDirectory_));
    if (!setInitCfg(kSdkInitComPath, &sdkPath)) {
        setError(hcNetSdkError("Could not set Hikvision thermal SDK component path"));
        return false;
    }

    QByteArray cryptoPath = nativePathBytes(QDir(runtimeDirectory_).absoluteFilePath(kOpenSslCryptoDll));
    if (!setInitCfg(kSdkInitCryptoPath, cryptoPath.data())) {
        setError(hcNetSdkError("Could not set Hikvision thermal SDK OpenSSL crypto path"));
        return false;
    }

    QByteArray sslPath = nativePathBytes(QDir(runtimeDirectory_).absoluteFilePath(kOpenSslSslDll));
    if (!setInitCfg(kSdkInitSslPath, sslPath.data())) {
        setError(hcNetSdkError("Could not set Hikvision thermal SDK OpenSSL SSL path"));
        return false;
    }

    return true;
}

bool ThermalCameraClient::initializeSdk() {
    auto init = resolveFunction<NetDvrInitFn>(hcNetSdk_, "NET_DVR_Init");
    if (init == nullptr) {
        setError(QString("Thermal camera SDK function NET_DVR_Init was not found in %1.").arg(kHcNetSdkDll));
        return false;
    }

    if (!init()) {
        setError(hcNetSdkError("Could not initialize Hikvision thermal SDK"));
        return false;
    }

    sdkInitialized_ = true;
    return true;
}

bool ThermalCameraClient::configureSdkLogging() {
    auto setLogToFile = resolveFunction<NetDvrSetLogToFileFn>(hcNetSdk_, "NET_DVR_SetLogToFile");
    if (setLogToFile == nullptr) {
        setError(QString("Thermal camera SDK function NET_DVR_SetLogToFile was not found in %1.")
                     .arg(kHcNetSdkDll));
        return false;
    }

    QDir root = paths_.rootDir();
    root.mkpath("SdkLog_CPP");
    QByteArray logPath = nativePathBytes(root.absoluteFilePath("SdkLog_CPP"));
    if (!setLogToFile(3, logPath.constData(), 0)) {
        setError(hcNetSdkError("Could not enable Hikvision thermal SDK log file"));
        return false;
    }

    return true;
}

bool ThermalCameraClient::acquirePlayPort() {
    auto getPort = resolveFunction<PlayM4GetPortFn>(playCtrl_, "PlayM4_GetPort");
    if (getPort == nullptr) {
        setError(QString("Thermal camera PlayCtrl function PlayM4_GetPort was not found in %1.")
                     .arg(kPlayCtrlDll));
        return false;
    }

    long port = kInvalidHandle;
    if (!getPort(&port) || port < 0) {
        setError(playCtrlError("Could not acquire Hikvision PlayCtrl decode port"));
        return false;
    }

    playPort_ = port;
    return true;
}

bool ThermalCameraClient::login() {
    auto loginV30 = resolveFunction<NetDvrLoginV30Fn>(hcNetSdk_, "NET_DVR_Login_V30");
    if (loginV30 == nullptr) {
        setError(QString("Thermal camera SDK function NET_DVR_Login_V30 was not found in %1.")
                     .arg(kHcNetSdkDll));
        return false;
    }

    QByteArray ip = config_.ip.toLatin1();
    QByteArray user = config_.user.toLocal8Bit();
    QByteArray password = config_.password.toLocal8Bit();
    NetDvrDeviceInfoV30 deviceInfo;
    userId_ = loginV30(
        ip.data(),
        static_cast<unsigned short>(config_.port),
        user.data(),
        password.data(),
        &deviceInfo);

    if (userId_ < 0) {
        setError(hcNetSdkError(QString("Could not log in to Hikvision thermal camera at %1:%2")
                                   .arg(config_.ip)
                                   .arg(config_.port)));
        return false;
    }

    return true;
}

bool ThermalCameraClient::startPreview() {
    auto realPlayV40 = resolveFunction<NetDvrRealPlayV40Fn>(hcNetSdk_, "NET_DVR_RealPlay_V40");
    if (realPlayV40 == nullptr) {
        setError(QString("Thermal camera SDK function NET_DVR_RealPlay_V40 was not found in %1.")
                     .arg(kHcNetSdkDll));
        return false;
    }

    NetDvrPreviewInfo previewInfo;
    previewInfo.channel = 1;
    previewInfo.streamType = 0;
    previewInfo.linkMode = 0;
    previewInfo.playWindow = nullptr;
    previewInfo.blocked = 1;

    realPlayHandle_ = realPlayV40(userId_, &previewInfo, &ThermalCameraClient::realDataCallback, this);
    if (realPlayHandle_ < 0) {
        setError(hcNetSdkError("Could not start Hikvision thermal preview stream"));
        return false;
    }

    return true;
}

void ThermalCameraClient::handleRealData(
    unsigned long dataType,
    unsigned char* buffer,
    unsigned long bufferSize) {
    if (playPort_ < 0 || buffer == nullptr || bufferSize == 0) {
        return;
    }

    if (dataType == kNetDvrSysHead) {
        auto setStreamOpenMode = resolveFunction<PlayM4SetStreamOpenModeFn>(playCtrl_, "PlayM4_SetStreamOpenMode");
        auto openStream = resolveFunction<PlayM4OpenStreamFn>(playCtrl_, "PlayM4_OpenStream");
        auto setDecodeCallback =
            resolveFunction<PlayM4SetDecCallBackExMendFn>(playCtrl_, "PlayM4_SetDecCallBackExMend");
        auto play = resolveFunction<PlayM4PlayFn>(playCtrl_, "PlayM4_Play");
        if (setStreamOpenMode == nullptr || openStream == nullptr || setDecodeCallback == nullptr || play == nullptr) {
            setError("One or more Hikvision PlayCtrl stream functions were not found.");
            return;
        }

        if (!setStreamOpenMode(playPort_, kPlayM4StreamRealtime) ||
            !openStream(playPort_, buffer, bufferSize, kPlayCtrlBufferSize) ||
            !setDecodeCallback(playPort_, &ThermalCameraClient::decodeCallback, nullptr, 0, this) ||
            !play(playPort_, nullptr)) {
            setError(playCtrlError("Could not open Hikvision PlayCtrl stream"));
        }
        return;
    }

    if (dataType == kNetDvrStreamData) {
        auto inputData = resolveFunction<PlayM4InputDataFn>(playCtrl_, "PlayM4_InputData");
        if (inputData == nullptr) {
            setError(QString("Thermal camera PlayCtrl function PlayM4_InputData was not found in %1.")
                         .arg(kPlayCtrlDll));
            return;
        }
        if (!inputData(playPort_, buffer, bufferSize)) {
            setError(playCtrlError("Could not pass Hikvision thermal stream data to PlayCtrl"));
        }
    }
}

void ThermalCameraClient::handleDecodedFrame(
    long port,
    char* buffer,
    long bufferSize,
    const ThermalCameraFrameInfo& frameInfo) {
    if (buffer == nullptr || bufferSize <= 0 || frameInfo.nType != kDecodedVideoFrameType) {
        return;
    }

    auto convertToJpeg =
        resolveFunction<PlayM4ConvertToJpegFileFn>(playCtrl_, "PlayM4_ConvertToJpegFile");
    if (convertToJpeg == nullptr) {
        setError(QString("Thermal camera PlayCtrl function PlayM4_ConvertToJpegFile was not found in %1.")
                     .arg(kPlayCtrlDll));
        return;
    }

    const QString cachePath = cacheFramePath(frameInfo.dwFrameNum);
    const QByteArray cachePathBytes = nativePathBytes(cachePath);
    const Bool converted = convertToJpeg(
        buffer,
        bufferSize,
        static_cast<long>(frameInfo.nWidth),
        static_cast<long>(frameInfo.nHeight),
        static_cast<long>(frameInfo.nType),
        cachePathBytes.constData());
    if (!converted) {
        setError(playCtrlError(QString("Could not convert Hikvision thermal frame %1 to JPEG")
                                   .arg(frameInfo.dwFrameNum)));
        return;
    }

    const QString savedPath = savedFramePath(cachePath);
    QFile::remove(savedPath);
    if (!QFile::copy(cachePath, savedPath)) {
        setError(QString("Could not copy thermal camera frame to saved image folder: %1").arg(savedPath));
        return;
    }

    Q_UNUSED(port);
    frameStore_.pruneCache();
}

QString ThermalCameraClient::cacheFramePath(unsigned long frameNumber) const {
    // Preserve the Python naming pattern to simplify comparing C++ and Python
    // capture folders during hardware checkout.
    return frameStore_.cacheDir().absoluteFilePath(QString("test_stamp[%1].jpg").arg(frameNumber));
}

QString ThermalCameraClient::savedFramePath(const QString& cacheFramePath) const {
    return frameStore_.savedDir().absoluteFilePath(QFileInfo(cacheFramePath).fileName());
}

QString ThermalCameraClient::hcNetSdkError(const QString& action) const {
    const auto lastError = resolveFunction<NetDvrGetLastErrorFn>(hcNetSdk_, "NET_DVR_GetLastError");
    if (lastError == nullptr) {
        return QString("%1. SDK error function NET_DVR_GetLastError is unavailable.").arg(action);
    }
    return QString("%1. SDK return code: %2").arg(action, thermalCameraErrorText(lastError()));
}

QString ThermalCameraClient::playCtrlError(const QString& action) const {
    const auto lastError = resolveFunction<PlayM4GetLastErrorFn>(playCtrl_, "PlayM4_GetLastError");
    if (lastError == nullptr) {
        return QString("%1. PlayCtrl error function PlayM4_GetLastError is unavailable.").arg(action);
    }
    return QString("%1. PlayCtrl return code: %2").arg(action, thermalCameraErrorText(lastError(playPort_)));
}

bool ThermalCameraClient::enterCallback() {
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (!acceptingCallbacks_) {
        return false;
    }

    ++activeCallbacks_;
    return true;
}

void ThermalCameraClient::leaveCallback() {
    std::lock_guard<std::mutex> lock(stateMutex_);
    if (activeCallbacks_ > 0) {
        --activeCallbacks_;
    }

    if (activeCallbacks_ == 0) {
        callbacksDrained_.notify_all();
    }
}

void ThermalCameraClient::setCallbacksAccepted(bool accepted) {
    {
        std::lock_guard<std::mutex> lock(stateMutex_);
        acceptingCallbacks_ = accepted;
    }

    if (!accepted) {
        callbacksDrained_.notify_all();
    }
}

void ThermalCameraClient::waitForCallbacksToFinish() {
    std::unique_lock<std::mutex> lock(stateMutex_);
    callbacksDrained_.wait(lock, [this]() { return activeCallbacks_ == 0; });
}

void ThermalCameraClient::setError(QString error) {
    std::lock_guard<std::mutex> lock(stateMutex_);
    lastError_ = std::move(error);
}
