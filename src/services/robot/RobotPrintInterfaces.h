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

    // Send a raw "<code> <params>#" or "STRING_COMMAND#" frame — used by EGM
    // and any other extension that the narrow print-only interface above does
    // not cover (e.g. CASE 110 / 111 / 112 to flip EGM flags in SERVER.mod).
    virtual RobotSocketResult sendRawMessage(const QString& frame) = 0;

    // Same as sendRawMessage but also returns the single-line reply. Used by
    // the EGM path to read back the current TCP pose (CASE 113) so the PC can
    // convert absolute path points into EGM offsets. The default reports
    // "unsupported" so existing socket fakes keep compiling.
    virtual RobotSocketResult sendRawMessageWithReply(const QString& frame, QString& reply) {
        Q_UNUSED(frame);
        Q_UNUSED(reply);
        return RobotSocketResult::failure("Reply capture is not supported by this socket.");
    }

    // 带超时的版本：EGM 启动前的握手查询必须快（不能像默认 3 s 那样把界面
    // 卡住十几秒）。默认实现忽略超时，退回上面的固定超时版本。
    virtual RobotSocketResult sendRawMessageWithReply(const QString& frame, QString& reply,
                                                      int timeoutMs) {
        Q_UNUSED(timeoutMs);
        return sendRawMessageWithReply(frame, reply);
    }
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
