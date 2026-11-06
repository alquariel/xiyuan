#pragma once

#include <QString>

#include "plc/PlcClient.h"

// Read-only view of the MOD upload state needed before print can start.
//
// FtpService implements this in production. Tests use a tiny fake so the print
// workflow can be verified without opening file dialogs or contacting FTP.
class IPrintUploadState {
public:
    virtual ~IPrintUploadState() = default;

    virtual bool uploadSucceeded() const = 0;
    virtual QString uploadedModFilename() const = 0;
};

// Result returned by robot socket operations. It mirrors PlcResult so service
// code can treat socket and PLC failures in the same simple way.
struct RobotSocketResult {
    bool ok = false;
    QString message;

    static RobotSocketResult success(const QString& message = QString()) {
        return RobotSocketResult{true, message};
    }

    static RobotSocketResult failure(const QString& message) {
        return RobotSocketResult{false, message};
    }
};

// Narrow command-socket interface used by RobotPrintService.
//
// The real implementation is RobotSocketClient. Tests use a fake that records
// command order, which is important for proving LOAD_MOD is sent before PLC START.
class IRobotCommandSocket {
public:
    virtual ~IRobotCommandSocket() = default;

    virtual RobotSocketResult connectToRobot() = 0;
    virtual void disconnectFromRobot() = 0;
    virtual bool isConnected() const = 0;
    virtual RobotSocketResult sendLoadMod(const QString& modFilename) = 0;
    virtual RobotSocketResult sendPause() = 0;
    virtual RobotSocketResult sendContinue() = 0;
    virtual RobotSocketResult sendStop() = 0;
};

// Interface for the DB18 print commands needed by the robot print workflow.
//
// PlcPrintSignalManager implements this in production. Tests use a fake so the
// workflow can be verified without PLC hardware.
class IPlcPrintController {
public:
    virtual ~IPlcPrintController() = default;

    virtual PlcResult startPrint(bool waitAck = true) = 0;
    virtual PlcResult pausePrint(bool waitAck = true) = 0;
    virtual PlcResult resumePrint(bool waitAck = true) = 0;
    virtual PlcResult stopPrint(bool waitAck = true) = 0;
};
