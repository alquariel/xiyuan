#pragma once

#include <cstdint>

#include <QString>
#include <QtGlobal>

#include "app/AppConfig.h"

// Result object returned by PLC operations. This keeps Snap7 error handling out
// of UI slots and lets callers decide how to show failures to the operator.
struct PlcResult {
    bool ok = false;
    QString message;

    static PlcResult success();
    static PlcResult failure(const QString& message);
};

class IPlcByteAccess {
public:
    virtual ~IPlcByteAccess() = default;
    virtual PlcResult writeDbByte(int dbNumber, int byteOffset, quint8 value) = 0;
    virtual PlcResult readDbByte(int dbNumber, int byteOffset, quint8& value) = 0;
};

class IPlcProcessAccess {
public:
    virtual ~IPlcProcessAccess() = default;

    virtual PlcResult writeProcessOutputInt32(int byteOffset, qint32 value) = 0;
    virtual PlcResult readProcessOutputInt32(int byteOffset, qint32& value) = 0;
    virtual PlcResult readProcessInputInt32(int byteOffset, qint32& value) = 0;
    virtual PlcResult readProcessInputByte(int byteOffset, quint8& value) = 0;
};

// Thin RAII wrapper around the Snap7 client used by the Python project.
//
// The vcpkg Snap7 package exposes a stable C ABI from snap7.dll, so this class
// stores the opaque S7Object handle as an integer. The rest of the application
// sees normal C++ methods and never calls Snap7 directly.
class PlcClient final : public IPlcByteAccess, public IPlcProcessAccess {
public:
    explicit PlcClient(PlcConfig config);
    ~PlcClient();

    PlcClient(const PlcClient&) = delete;
    PlcClient& operator=(const PlcClient&) = delete;

    bool isConnected() const;
    QString lastError() const;

    PlcResult connect();
    void disconnect();

    PlcResult writeDbBit(int dbNumber, int byteOffset, int bitIndex, bool value);
    PlcResult readDbBit(int dbNumber, int byteOffset, int bitIndex, bool& value);
    PlcResult writeDbByte(int dbNumber, int byteOffset, quint8 value) override;
    PlcResult readDbByte(int dbNumber, int byteOffset, quint8& value) override;
    PlcResult writeDbInt16(int dbNumber, int byteOffset, qint16 value);
    PlcResult writeDbReal32(int dbNumber, int byteOffset, float value);
    PlcResult writeMBit(int byteOffset, int bitIndex, bool value);
    PlcResult writeProcessOutputInt32(int byteOffset, qint32 value) override;
    PlcResult readProcessOutputInt32(int byteOffset, qint32& value) override;
    PlcResult readProcessInputInt32(int byteOffset, qint32& value) override;
    PlcResult readProcessInputByte(int byteOffset, quint8& value) override;

private:
    PlcResult ensureConnected();
    PlcResult snap7Failure(const QString& action, int code);

    PlcConfig config_;
    std::uintptr_t client_ = 0;
    bool connected_ = false;
    QString lastError_;
};
