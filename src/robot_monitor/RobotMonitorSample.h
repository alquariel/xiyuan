#pragma once

#include <array>

#include <QDateTime>

// Holds one parsed robot monitor sample received from the RAPID monitoring socket.
struct RobotMonitorSample {
    QDateTime receivedAtUtc;
    int sequence = 0;

    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double q1 = 0.0;
    double q2 = 0.0;
    double q3 = 0.0;
    double q4 = 0.0;

    std::array<double, 6> joints = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::array<double, 6> externalAxis = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    double commandedTcpSpeed = 0.0;
    double commandedOrientationSpeed = 0.0;
    double commandedExternalLinearSpeed = 0.0;
    double commandedExternalOrientationSpeed = 0.0;

    double estimatedTcpSpeed = 0.0;
};
