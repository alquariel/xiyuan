#pragma once

#include <array>

struct RobotPose {
    std::array<double, 3> position{};
    std::array<double, 4> quaternion{};
};

// Converts the Python toQuaternion.calc_to_quaternion(vector, -30) behavior
// into a small C++ helper with no Python/SciPy runtime dependency.
class RobotQuaternion {
public:
    static std::array<double, 4> fromOrientationVector(double x, double y, double z);
    static RobotPose makePose(double x, double y, double z, double rx, double ry, double rz);

private:
    static std::array<double, 3> toEulerAngles(double i, double j, double k);
    static std::array<double, 4> toQuaternionWxyz(const std::array<double, 3>& eulerDegrees);
};
