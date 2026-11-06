#pragma once

#include "app/AppConfig.h"
#include "app/AppPaths.h"
#include "thermal_camera/IThermalCameraClient.h"
#include "thermal_camera/ThermalFrameStore.h"

#include <QDir>
#include <QLibrary>
#include <QString>
#include <QStringList>

#include <condition_variable>
#include <mutex>

#if defined(Q_OS_WIN)
#define THERMAL_CAMERA_STDCALL __stdcall
#else
#define THERMAL_CAMERA_STDCALL
#endif

// Minimal PlayCtrl frame metadata used by the decode callback. This mirrors the
// Hikvision SDK layout fields that the Python workflow reads before converting a
// decoded frame to a JPEG file.
struct ThermalCameraFrameInfo {
    unsigned long nWidth = 0;
    unsigned long nHeight = 0;
    unsigned long nStamp = 0;
    unsigned long nType = 0;
    unsigned long nFrameRate = 0;
    unsigned long dwFrameNum = 0;
};

// Production client boundary for the Hikvision network thermal camera path. It
// owns the SDK login, preview stream, PlayCtrl decode port, and JPEG cache
// writes. UI code talks only to IThermalCameraClient so tests can replace this
// hardware client with a fake.
class ThermalCameraClient final : public IThermalCameraClient {
public:
    ThermalCameraClient(ThermalCameraConfig config, AppPaths paths, const ThermalFrameStore& frameStore);
    ~ThermalCameraClient() override;

    bool startCapture() override;
    void stopCapture() override;
    bool isCapturing() const override;
    QString lastError() const override;

private:
    static void THERMAL_CAMERA_STDCALL realDataCallback(
        long realHandle,
        unsigned long dataType,
        unsigned char* buffer,
        unsigned long bufferSize,
        void* userData);
    static void THERMAL_CAMERA_STDCALL decodeCallback(
        long port,
        char* buffer,
        long bufferSize,
        ThermalCameraFrameInfo* frameInfo,
        void* userData,
        void* reserved);

    QStringList runtimeDirectories() const;
    bool runtimeHasRequiredFiles(const QDir& dir) const;
    bool loadRuntime();
    bool resolveRequiredFunctions();
    bool configureDllSearchPath(const QDir& dir);
    bool configureSdkRuntimePath();
    bool initializeSdk();
    bool configureSdkLogging();
    bool acquirePlayPort();
    bool login();
    bool startPreview();
    void handleRealData(unsigned long dataType, unsigned char* buffer, unsigned long bufferSize);
    void handleDecodedFrame(long port, char* buffer, long bufferSize, const ThermalCameraFrameInfo& frameInfo);
    QString cacheFramePath(unsigned long frameNumber) const;
    QString savedFramePath(const QString& cacheFramePath) const;
    QString hcNetSdkError(const QString& action) const;
    QString playCtrlError(const QString& action) const;
    bool enterCallback();
    void leaveCallback();
    void setCallbacksAccepted(bool accepted);
    void waitForCallbacksToFinish();
    void setError(QString error);

    ThermalCameraConfig config_;
    AppPaths paths_;
    ThermalFrameStore frameStore_;
    QLibrary hcNetSdk_;
    QLibrary playCtrl_;
    QString runtimeDirectory_;
    QString lastError_;
    bool capturing_ = false;
    bool sdkInitialized_ = false;
    long userId_ = -1;
    long realPlayHandle_ = -1;
    long playPort_ = -1;
    mutable std::mutex stateMutex_;
    std::condition_variable callbacksDrained_;
    int activeCallbacks_ = 0;
    bool acceptingCallbacks_ = false;
};
