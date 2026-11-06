#pragma once

#include <QString>
#include <QtGlobal>

#include "plc/PlcClient.h"
#include "services/robot/RobotPrintInterfaces.h"

// DB18 command values used by the Python PLCSignalManager.
enum class PlcPrintCommand : quint8 {
    None = 0,
    Start = 10,
    Pause = 11,
    Resume = 12,
    Stop = 13,
};

// Focused manager for the host-to-PLC print command channel.
//
// DB18.DBB0 carries the command, DB18.DBB1 carries the PLC ACK, DB18.DBB2
// carries state, and DB18.DBB3 carries errors. This class only performs that
// DB18 handshake; UI button wiring, ABB robot sockets, and FTP behavior belong
// to later slices.
class PlcPrintSignalManager final : public IPlcPrintController {
public:
    explicit PlcPrintSignalManager(IPlcByteAccess& plc);
    // With enabled=false no DB18 access happens: every command returns success
    // immediately so the print workflow still runs on a cell without a PLC.
    PlcPrintSignalManager(IPlcByteAccess& plc, int timeoutMs, int pollIntervalMs, bool enabled = true);

    PlcResult startPrint(bool waitAck = true) override;
    PlcResult pausePrint(bool waitAck = true) override;
    PlcResult resumePrint(bool waitAck = true) override;
    PlcResult stopPrint(bool waitAck = true) override;
    PlcResult sendCommand(PlcPrintCommand command, bool waitAck = true);

    int dbNumber() const;
    int commandOffset() const;
    int ackOffset() const;
    int stateOffset() const;
    int errorOffset() const;

private:
    PlcResult waitForAck();
    PlcResult clearCommand();
    QString commandName(PlcPrintCommand command) const;

    IPlcByteAccess& plc_;
    bool enabled_ = true;
    int dbNumber_ = 18;
    int commandOffset_ = 0;
    int ackOffset_ = 1;
    int stateOffset_ = 2;
    int errorOffset_ = 3;
    int timeoutMs_ = 3000;
    int pollIntervalMs_ = 50;
};
