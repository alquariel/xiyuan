#pragma once

#include "app/AppConfig.h"
#include "camera/IHighSpeedCameraClient.h"

#include <QLibrary>
#include <QString>
#include <QStringList>

// Concrete camera client for the Hikrobot/MVS visible-light camera SDK.
//
// The rest of the application talks to IHighSpeedCameraClient, not directly to
// this class. That keeps UI/service code testable without camera hardware and
// keeps all vendor DLL details contained in one small implementation unit.
class HighSpeedCameraClient final : public IHighSpeedCameraClient {
public:
    // appDirPath is the directory beside AFP_ABB_CPP.exe. It is used to find
    // the portable runtime folder copied into the release package.
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
    // Opaque implementation storage. The concrete structs contain MVS SDK
    // layouts and function pointers, so they stay out of this public header.
    struct MvsFunctions;
    struct MvsDeviceInfoList;

    void buildDllSearchList();
    bool loadFromPath(const QString& path);
    bool configureDllSearchPath(const QString& dllPath);
    bool resolveFunctions();
    bool requireLoaded(const QString& action);
    bool failWithSdkCode(const QString& action, quint32 code);
    void setError(const QString& message);

    HighSpeedCameraConfig config_;
    QString appDirPath_;
    QStringList searchedDllPaths_;
    QLibrary library_;
    MvsFunctions* functions_ = nullptr;
    MvsDeviceInfoList* deviceList_ = nullptr;
    void* handle_ = nullptr;
    bool open_ = false;
    bool grabbing_ = false;
    QString lastError_;
};
