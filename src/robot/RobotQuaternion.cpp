#include "robot/RobotQuaternion.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kLocalRotationDeg = -30.0;

double degrees(double radians) { return radians * 180.0 / kPi; }
double radians(double degreesValue) { return degreesValue * kPi / 180.0; }

std::array<double, 3> normalizedOrientationVector(double x, double y, double z) {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
        throw std::invalid_argument("Robot orientation vector must contain only finite values");
    }

    const double scale = std::max({std::abs(x), std::abs(y), std::abs(z)});
    if (!std::isfinite(scale)) {
        throw std::invalid_argument("Robot orientation vector scale must be finite");
    }

    if (scale == 0.0) {
        throw std::invalid_argument("Robot orientation vector must not be zero");
    }

    const double sx = x / scale;
    const double sy = y / scale;
    const double sz = z / scale;
    const double scaledMagnitude = std::hypot(sx, sy, sz);
    if (!std::isfinite(scaledMagnitude)) {
        throw std::invalid_argument("Robot orientation vector magnitude must be finite");
    }

    if (scaledMagnitude == 0.0) {
        throw std::invalid_argument("Robot orientation vector must not be zero");
    }

    return {sx / scaledMagnitude, sy / scaledMagnitude, sz / scaledMagnitude};
}

std::array<double, 4> multiplyXyzw(const std::array<double, 4>& a, const std::array<double, 4>& b) {
    const double ax = a[0], ay = a[1], az = a[2], aw = a[3];
    const double bx = b[0], by = b[1], bz = b[2], bw = b[3];
    return {
        aw * bx + ax * bw + ay * bz - az * by,
        aw * by - ax * bz + ay * bw + az * bx,
        aw * bz + ax * by - ay * bx + az * bw,
        aw * bw - ax * bx - ay * by - az * bz,
    };
}

std::array<double, 4> conjugateXyzw(const std::array<double, 4>& q) {
    return {-q[0], -q[1], -q[2], q[3]};
}

std::array<double, 3> rotateVectorByQuaternion(const std::array<double, 3>& v, const std::array<double, 4>& q) {
    const std::array<double, 4> p{v[0], v[1], v[2], 0.0};
    const auto rotated = multiplyXyzw(multiplyXyzw(q, p), conjugateXyzw(q));
    return {rotated[0], rotated[1], rotated[2]};
}

std::array<double, 4> fromRotVec(const std::array<double, 3>& axis, double angleRad) {
    const double norm = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);
    const double s = std::sin(angleRad / 2.0);
    return {axis[0] / norm * s, axis[1] / norm * s, axis[2] / norm * s, std::cos(angleRad / 2.0)};
}
}

std::array<double, 4> RobotQuaternion::fromOrientationVector(double x, double y, double z) {
    // Preserve Python toQuaternion.calc_to_quaternion(vector, -30):
    // normalize the direction first, derive Euler angles from that direction,
    // rotate around the resulting local Z axis in world space, then return
    // ABB's negated WXYZ.
    const auto orientation = normalizedOrientationVector(x, y, z);

    const auto euler = toEulerAngles(orientation[0], orientation[1], orientation[2]);
    const auto qStartWxyz = toQuaternionWxyz(euler);
    const std::array<double, 4> qStartXyzw{qStartWxyz[1], qStartWxyz[2], qStartWxyz[3], qStartWxyz[0]};
    const auto axisWorld = rotateVectorByQuaternion({0.0, 0.0, 1.0}, qStartXyzw);
    const auto delta = fromRotVec(axisWorld, radians(kLocalRotationDeg));
    const auto qEndXyzw = multiplyXyzw(delta, qStartXyzw);
    return {-qEndXyzw[3], -qEndXyzw[0], -qEndXyzw[1], -qEndXyzw[2]};
}

RobotPose RobotQuaternion::makePose(double x, double y, double z, double rx, double ry, double rz) {
    return {{x, y, z}, fromOrientationVector(rx, ry, rz)};
}

std::array<double, 3> RobotQuaternion::toEulerAngles(double i, double j, double k) {
    // This mirrors Python to_eulerangles exactly, including its axis naming:
    // angle X comes from acos(sqrt((i*i + k*k) / norm)), angle Y from atan2(i, k),
    // and angle Z is fixed at 0 for the robot movement UI's orientation vector.
    const double normSquared = i * i + j * j + k * k;
    double angleX = degrees(std::acos(std::sqrt((i * i + k * k) / normSquared)));
    if (k > 0.0) {
        angleX = 360.0 - angleX;
    }
    if (k < 0.0 && j > 0.0) {
        angleX = 360.0 - angleX;
    }

    double angleY = degrees(std::atan2(i, k));
    if (angleY < 0.0) {
        angleY += 180.0;
    }
    if (i < 0.0) {
        angleY += 180.0;
    }

    return {angleX, angleY, 0.0};
}

std::array<double, 4> RobotQuaternion::toQuaternionWxyz(const std::array<double, 3>& eulerDegrees) {
    const double angleX = eulerDegrees[0];
    const double angleY = eulerDegrees[1];
    const double angleZ = eulerDegrees[2];

    const double cosX = std::cos(radians(angleX * 0.5));
    const double sinX = std::sin(radians(angleX * 0.5));
    const double cosY = std::cos(radians(angleY * 0.5));
    const double sinY = std::sin(radians(angleY * 0.5));
    const double cosZ = std::cos(radians(angleZ * 0.5));
    const double sinZ = std::sin(radians(angleZ * 0.5));

    return {
        cosX * cosY * cosZ + sinX * sinY * sinZ,
        sinX * cosY * cosZ - cosX * sinY * sinZ,
        cosX * sinY * cosZ + sinX * cosY * sinZ,
        cosX * cosY * sinZ - sinX * sinY * cosZ,
    };
}
