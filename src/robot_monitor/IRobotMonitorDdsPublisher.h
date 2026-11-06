#pragma once

#include <QString>

#include "robot_monitor/RobotMonitorSample.h"

// Testable boundary for publishing parsed robot monitor samples. Production
// code uses RobotMonitorDdsPublisher; unit tests can inject a fake without
// starting a real Fast DDS participant.
class IRobotMonitorDdsPublisher {
public:
    virtual ~IRobotMonitorDdsPublisher() = default;

    virtual bool publish(const RobotMonitorSample& sample) = 0;
    virtual QString lastError() const = 0;
};
