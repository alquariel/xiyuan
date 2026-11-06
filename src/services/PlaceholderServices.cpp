#include "services/PlaceholderServices.h"

#include <utility>

PlaceholderService::PlaceholderService(QString serviceName, QObject* parent)
    : QObject(parent), serviceName_(std::move(serviceName)) {
}

void PlaceholderService::reportNotConverted(const QString& action) {
    emit messageRequested(serviceName_, QString("%1 is not converted in Slice A.").arg(action));
}

SerialService::SerialService(QObject* parent) : PlaceholderService("Serial", parent) {}
void SerialService::portCheck() { reportNotConverted("Serial port scan"); }
void SerialService::portOpen() { reportNotConverted("Serial port open"); }
void SerialService::portClose() { reportNotConverted("Serial port close"); }

RobotService::RobotService(QObject* parent) : PlaceholderService("Robot", parent) {}
void RobotService::connectRobot() { reportNotConverted("Robot connection"); }
void RobotService::moveRobot() { reportNotConverted("Robot move"); }
void RobotService::resetRobot() { reportNotConverted("Robot reset"); }
void RobotService::moveRobotAxes() { reportNotConverted("Robot axis move"); }
void RobotService::moveExternalAxis() { reportNotConverted("External axis move"); }
void RobotService::resetExternalAxis() { reportNotConverted("External axis reset"); }
void RobotService::startPrint() { reportNotConverted("Start print"); }
void RobotService::pausePrint() { reportNotConverted("Pause print"); }
void RobotService::continuePrint() { reportNotConverted("Continue print"); }
void RobotService::stopPrint() { reportNotConverted("Stop print"); }
