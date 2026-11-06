#pragma once

#include <QString>

// Tests use fake implementations of this boundary; production will use
// ThermalCameraClient to talk to the Hikvision SDK.
class IThermalCameraClient {
public:
    virtual ~IThermalCameraClient() = default;

    virtual bool startCapture() = 0;
    virtual void stopCapture() = 0;
    virtual bool isCapturing() const = 0;
    virtual QString lastError() const = 0;
};
