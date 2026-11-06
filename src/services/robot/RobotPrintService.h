#pragma once

#include <QObject>
#include <QString>

#include <memory>

#include "app/AppConfig.h"
#include "plc/PlcClient.h"
#include "plc/PlcPrintSignalManager.h"
#include "robot/RobotSocketClient.h"
#include "services/robot/RobotPrintInterfaces.h"

// Qt-facing service for the original print control buttons.
//
// This service intentionally covers only the print workflow. General robot
// movement remains in the placeholder service until a later migration slice.
class RobotPrintService final : public QObject {
    Q_OBJECT

public:
    RobotPrintService(const RobotConfig& robotConfig,
                      const PlcConfig& plcConfig,
                      IPrintUploadState& uploadState,
                      QObject* parent = nullptr);
    RobotPrintService(IPrintUploadState& uploadState,
                      IRobotCommandSocket& socket,
                      IPlcPrintController& plcPrint,
                      QObject* parent = nullptr);

    bool linkState() const;
    bool pauseFlag() const;
    bool stopFlag() const;

    // EGM layer needs to send raw CASE frames through the same TCP socket that
    // the print workflow uses (so connection state is shared and we don't hold
    // two sockets open to port 5000). This accessor exposes the socket interface
    // so callers can write "110 #" / "111 #" to flip EGM flags in SERVER.mod.
    IRobotCommandSocket* socket() const { return &socket_; }

signals:
    void messageRequested(const QString& title, const QString& message);
    void robotStatusRequested(const QString& text, bool connected);

public slots:
    void connectRobot();
    void startPrint();
    void pausePrint();
    void continuePrint();
    void stopPrint();

private:
    bool requireConnected();
    void reportPlcFailure(const QString& action, const PlcResult& result);
    void reportOptionalSocketFailure(const QString& action, const RobotSocketResult& result);

    std::unique_ptr<RobotSocketClient> ownedSocket_;
    std::unique_ptr<PlcClient> ownedPlc_;
    std::unique_ptr<PlcPrintSignalManager> ownedPrintSignals_;

    IPrintUploadState& uploadState_;
    IRobotCommandSocket& socket_;
    IPlcPrintController& plcPrint_;

    bool linkState_ = false;
    bool pauseFlag_ = false;
    bool stopFlag_ = true;
};
