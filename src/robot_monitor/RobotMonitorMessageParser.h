#pragma once

#include <QString>

#include "robot_monitor/RobotMonitorSample.h"

// Reports either a typed robot monitor sample or the reason a raw frame was rejected.
struct RobotMonitorParseResult {
    bool ok = false;
    RobotMonitorSample sample;
    QString error;
};

// Converts '#ROBOT_MONITOR key=value ...#' socket frames into RobotMonitorSample values.
class RobotMonitorMessageParser final {
public:
    static RobotMonitorParseResult parse(const QString& rawMessage);
};
