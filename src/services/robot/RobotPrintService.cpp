#include "services/robot/RobotPrintService.h"

RobotPrintService::RobotPrintService(const RobotConfig& robotConfig,
                                     const PlcConfig& plcConfig,
                                     IPrintUploadState& uploadState,
                                     QObject* parent)
    : QObject(parent),
      ownedSocket_(std::make_unique<RobotSocketClient>(robotConfig)),
      ownedPlc_(std::make_unique<PlcClient>(plcConfig)),
      ownedPrintSignals_(std::make_unique<PlcPrintSignalManager>(*ownedPlc_, 3000, 50, plcConfig.enabled)),
      uploadState_(uploadState),
      socket_(*ownedSocket_),
      plcPrint_(*ownedPrintSignals_) {
}

RobotPrintService::RobotPrintService(IPrintUploadState& uploadState,
                                     IRobotCommandSocket& socket,
                                     IPlcPrintController& plcPrint,
                                     QObject* parent)
    : QObject(parent),
      uploadState_(uploadState),
      socket_(socket),
      plcPrint_(plcPrint) {
}

bool RobotPrintService::linkState() const {
    return linkState_;
}

bool RobotPrintService::pauseFlag() const {
    return pauseFlag_;
}

bool RobotPrintService::stopFlag() const {
    return stopFlag_;
}

void RobotPrintService::connectRobot() {
    const RobotSocketResult result = socket_.connectToRobot();
    linkState_ = result.ok && socket_.isConnected();

    emit robotStatusRequested(result.message, linkState_);
    emit messageRequested(linkState_ ? "Robot" : "Robot connection", result.message);
}

void RobotPrintService::startPrint() {
    if (!uploadState_.uploadSucceeded()) {
        emit messageRequested("Warning", "Please upload a MOD file before starting print.");
        return;
    }

    const QString modFilename = uploadState_.uploadedModFilename().trimmed();
    if (modFilename.isEmpty()) {
        emit messageRequested("Warning", "Uploaded MOD filename is empty. Please upload again.");
        return;
    }

    if (!requireConnected()) {
        return;
    }

    const RobotSocketResult loadResult = socket_.sendLoadMod(modFilename);
    if (!loadResult.ok) {
        emit messageRequested("Robot", QString("Failed to send MOD filename to robot: %1").arg(loadResult.message));
        return;
    }

    const PlcResult plcResult = plcPrint_.startPrint(true);
    if (!plcResult.ok) {
        reportPlcFailure("start print", plcResult);
        return;
    }

    stopFlag_ = false;
    pauseFlag_ = false;
    emit messageRequested("Success", "MOD filename sent. Print task started.");
}

void RobotPrintService::pausePrint() {
    const PlcResult plcResult = plcPrint_.pausePrint(true);
    if (!plcResult.ok) {
        reportPlcFailure("pause print", plcResult);
        return;
    }

    pauseFlag_ = true;
    if (socket_.isConnected()) {
        reportOptionalSocketFailure("pause side message", socket_.sendPause());
    }
    emit messageRequested("Success", "Print paused.");
}

void RobotPrintService::continuePrint() {
    const PlcResult plcResult = plcPrint_.resumePrint(true);
    if (!plcResult.ok) {
        reportPlcFailure("continue print", plcResult);
        return;
    }

    pauseFlag_ = false;
    if (socket_.isConnected()) {
        reportOptionalSocketFailure("continue side message", socket_.sendContinue());
    }
    emit messageRequested("Success", "Print continued.");
}

void RobotPrintService::stopPrint() {
    const PlcResult plcResult = plcPrint_.stopPrint(true);
    if (!plcResult.ok) {
        reportPlcFailure("stop print", plcResult);
        return;
    }

    stopFlag_ = true;
    pauseFlag_ = false;
    if (socket_.isConnected()) {
        reportOptionalSocketFailure("stop side message", socket_.sendStop());
    }
    emit messageRequested("Success", "Print stopped.");
}

bool RobotPrintService::requireConnected() {
    linkState_ = socket_.isConnected();
    if (!linkState_) {
        emit messageRequested("Robot control", "Robot is not connected. Please connect the robot first.");
        emit robotStatusRequested("Robot socket not connected.", false);
        return false;
    }
    return true;
}

void RobotPrintService::reportPlcFailure(const QString& action, const PlcResult& result) {
    emit messageRequested("PLC", QString("PLC handshake failed during %1: %2").arg(action, result.message));
}

void RobotPrintService::reportOptionalSocketFailure(const QString& action, const RobotSocketResult& result) {
    if (!result.ok) {
        emit messageRequested("Robot", QString("Non-fatal %1 failed: %2").arg(action, result.message));
    }
}
