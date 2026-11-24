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

    static QString loadModMessage(const QString& modFilename);

private:
    RobotSocketResult sendRawMessage(const QString& message);
    QString endpointText() const;

    RobotConfig config_;
    QTcpSocket socket_;
    int timeoutMs_ = 3000;
};
