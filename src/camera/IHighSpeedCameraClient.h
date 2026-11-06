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
