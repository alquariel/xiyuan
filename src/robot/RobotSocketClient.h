#pragma once

#include <QString>
#include <QTcpSocket>

#include "app/AppConfig.h"
#include "services/robot/RobotPrintInterfaces.h"

// Lightweight ABB robot socket wrapper for the print workflow.
//
// It only sends the exact string commands used by the Python print path:
// LOAD_MOD:<filename>#, PAUSE#, CONTINUE#, and STOP#. It does not implement
// robot motion, pose conversion, or RAPID handling.
class RobotSocketClient final : public IRobotCommandSocket {
public:
    explicit RobotSocketClient(RobotConfig config);

    RobotSocketResult connectToRobot() override;
    void disconnectFromRobot() override;
    bool isConnected() const override;
    RobotSocketResult sendLoadMod(const QString& modFilename) override;
    RobotSocketResult sendPause() override;
    RobotSocketResult sendContinue() override;
    RobotSocketResult sendStop() override;
    RobotSocketResult sendRawMessage(const QString& frame) override;
    RobotSocketResult sendRawMessageWithReply(const QString& frame, QString& reply) override;
    RobotSocketResult sendRawMessageWithReply(const QString& frame, QString& reply,
                                              int timeoutMs) override;

    // CASE 0 Ping — sends "0 #" and waits for "0 1" to verify the peer is
    // really the SERVER.mod RAPID process, not just any program bound to the
    // port (we saw an orphan RobVC.exe accept a TCP connect but never reply).
    RobotSocketResult sendPing();

    static QString loadModMessage(const QString& modFilename);

private:
    QString endpointText() const;

    RobotConfig config_;
    QTcpSocket socket_;
    int timeoutMs_ = 3000;
};
