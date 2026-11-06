#pragma once

#include <cstdint>

#include "robot_monitor/RobotMonitorSample.h"
#include "robot_monitor/generated/RobotMonitorSample.hpp"

// Converts the parsed robot monitor socket sample into the DDS sample type
// used by publishers and archive subscribers. Keeping the field mapping here
// makes the DDS contract easy to audit against RobotMonitorSample.idl.
inline AfpRobotMonitorSample toDdsSample(const RobotMonitorSample& sample) {
    AfpRobotMonitorSample ddsSample;
    ddsSample.timestamp_ms(static_cast<std::uint64_t>(sample.receivedAtUtc.toMSecsSinceEpoch()));
    ddsSample.sample_sequence(sample.sequence < 0 ? 0ULL : static_cast<std::uint64_t>(sample.sequence));
    ddsSample.x(sample.x);
    ddsSample.y(sample.y);
    ddsSample.z(sample.z);
    ddsSample.q1(sample.q1);
    ddsSample.q2(sample.q2);
    ddsSample.q3(sample.q3);
    ddsSample.q4(sample.q4);
    ddsSample.j1(sample.joints[0]);
    ddsSample.j2(sample.joints[1]);
    ddsSample.j3(sample.joints[2]);
    ddsSample.j4(sample.joints[3]);
    ddsSample.j5(sample.joints[4]);
    ddsSample.j6(sample.joints[5]);
    ddsSample.eax1(sample.externalAxis[0]);
    ddsSample.eax2(sample.externalAxis[1]);
    ddsSample.eax3(sample.externalAxis[2]);
    ddsSample.eax4(sample.externalAxis[3]);
    ddsSample.eax5(sample.externalAxis[4]);
    ddsSample.eax6(sample.externalAxis[5]);
    ddsSample.commanded_tcp_speed(sample.commandedTcpSpeed);
    ddsSample.commanded_orientation_speed(sample.commandedOrientationSpeed);
    ddsSample.commanded_external_linear_speed(sample.commandedExternalLinearSpeed);
    ddsSample.commanded_external_orientation_speed(sample.commandedExternalOrientationSpeed);
    ddsSample.estimated_tcp_speed(sample.estimatedTcpSpeed);
    return ddsSample;
}
