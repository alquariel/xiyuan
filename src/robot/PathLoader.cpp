#include "robot/PathLoader.h"

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

#include <cmath>

PathLoader::PathLoader(QObject* parent)
    : QObject(parent) {}

float PathVector3::length() const {
    return std::sqrt(x * x + y * y + z * z);
}

// Pure-math quaternion slerp, matches ABB EGM quaternion ordering (q1=scalar).
static PathQuaternion Slerp(const PathQuaternion& qa,
                            const PathQuaternion& qb,
                            float t) {
    // Normalize inputs
    auto normQ = [](const PathQuaternion& q) {
        const float len = std::sqrt(q.q1 * q.q1 + q.q2 * q.q2 + q.q3 * q.q3 + q.q4 * q.q4);
        if (len < 1e-6f) return PathQuaternion{1, 0, 0, 0};
        return PathQuaternion{q.q1 / len, q.q2 / len, q.q3 / len, q.q4 / len};
    };
    PathQuaternion a = normQ(qa);
    PathQuaternion b = normQ(qb);

    // Compute dot product (scalar components)
    float dot = a.q1 * b.q1 + a.q2 * b.q2 + a.q3 * b.q3 + a.q4 * b.q4;
    if (dot < 0.0f) {
        // Flip b to ensure the shortest-path interpolation
        b = {-b.q1, -b.q2, -b.q3, -b.q4};
        dot = -dot;
    }

    if (dot > 0.9995f) {
        // Angles too small — linear interpolation is numerically safer.
        PathQuaternion result{
            a.q1 + t * (b.q1 - a.q1),
            a.q2 + t * (b.q2 - a.q2),
            a.q3 + t * (b.q3 - a.q3),
            a.q4 + t * (b.q4 - a.q4)
        };
        return normQ(result);
    }

    const float theta0 = std::acos(std::clamp(dot, -1.0f, 1.0f));
    const float theta = theta0 * t;
    const float sinTheta0 = std::sin(theta0);
    const float s1 = std::cos(theta) - dot * std::sin(theta) / sinTheta0;
    const float s2 = std::sin(theta) / sinTheta0;

    return normQ({
        s1 * a.q1 + s2 * b.q1,
        s1 * a.q2 + s2 * b.q2,
        s1 * a.q3 + s2 * b.q3,
        s1 * a.q4 + s2 * b.q4
    });
}

bool PathLoader::loadFromTxt(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream in(&file);
    QStringList allLines;
    while (!in.atEnd()) {
        allLines.append(in.readLine());
    }

    rawPoints_.clear();
    segments_.clear();
    jointPoints_.clear();
    jointSegments_.clear();
    jointMode_ = false;
    explicitQuatMode_ = false;

    // A comment line containing MODE=JOINT switches the file to joint space.
    for (const QString& candidate : allLines) {
        const QString trimmed = candidate.trimmed();
        if (trimmed.startsWith('#') && trimmed.contains("MODE=JOINT", Qt::CaseInsensitive)) {
            jointMode_ = true;
            break;
        }
    }

    // MODE=POSE means the orientation columns are an explicit quaternion
    // (x y z q1 q2 q3 q4 v) instead of the legacy (i,j,k) direction vector.
    if (!jointMode_) {
        for (const QString& candidate : allLines) {
            const QString trimmed = candidate.trimmed();
            if (trimmed.startsWith('#') && trimmed.contains("MODE=POSE", Qt::CaseInsensitive)) {
                explicitQuatMode_ = true;
                break;
            }
        }
    }

    const QRegularExpression separator("\\s+");

    if (jointMode_) {
        for (const QString& candidate : allLines) {
            const QString line = candidate.trimmed();
            if (line.isEmpty() || line.startsWith('#')) {
                continue;
            }
            const QStringList tokens = line.split(separator, Qt::SkipEmptyParts);
            if (tokens.size() < 7) {
                continue;
            }
            PathJointTarget jt;
            bool numeric = true;
            for (int axis = 0; axis < 6; ++axis) {
                bool ok = false;
                jt.joints[axis] = tokens[axis].toFloat(&ok);
                numeric = numeric && ok;
            }
            bool okSpeed = false;
            jt.v = tokens[6].toFloat(&okSpeed);
            if (!numeric || !okSpeed) {
                continue;
            }
            jointPoints_.append(jt);
        }

        // Segments are measured in joint space (degrees) so the caller can
        // advance with a deg/s speed just like the cartesian mm/s case.
        for (int i = 0; i + 1 < jointPoints_.size(); ++i) {
            float squared = 0;
            for (int axis = 0; axis < 6; ++axis) {
                const float delta = jointPoints_[i + 1].joints[axis]
                                  - jointPoints_[i].joints[axis];
                squared += delta * delta;
            }
            PathJointSegment seg;
            seg.fromIndex = i;
            seg.toIndex = i + 1;
            seg.lengthDeg = std::sqrt(squared);
            jointSegments_.append(seg);
        }
        return jointPoints_.size() >= 2;
    }

    for (const QString& candidate : allLines) {
        const QString line = candidate.trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        const QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        // path_control files come in 7–8 columns (x,y,z,i,j,k,v[, angle]) —
        // only the first 7 are mandatory. Python's legacy "10-column" layout
        // had speed, turning angle and two legacy scalars appended after;
        // we fill missing trailing columns with 0 rather than reject the row.
        // MODE=POSE files instead carry "x y z q1 q2 q3 q4 v" (8 mandatory).
        const int requiredColumns = explicitQuatMode_ ? 8 : 7;
        if (tokens.size() < requiredColumns) {
            continue;
        }

        PathControlPoint pt;
        pt.x = tokens[0].toFloat();
        pt.y = tokens[1].toFloat();
        pt.z = tokens[2].toFloat();
        if (explicitQuatMode_) {
            // q1..q4 是 ABB rot 的 [q1,q2,q3,q4]（q1 为标量），直接使用，
            // 不能再用 (i,j,k) 方向向量反推姿态
            pt.q1 = tokens[3].toFloat();
            pt.q2 = tokens[4].toFloat();
            pt.q3 = tokens[5].toFloat();
            pt.q4 = tokens[6].toFloat();
            pt.v  = tokens[7].toFloat();
        } else {
            pt.i = tokens[3].toFloat();
            pt.j = tokens[4].toFloat();
            pt.k = tokens[5].toFloat();
            pt.v = tokens[6].toFloat();
            pt.angle = tokens.size() > 7 ? tokens[7].toFloat() : 0.0f;
            pt.a     = tokens.size() > 8 ? tokens[8].toFloat() : 0.0f;
            pt.b     = tokens.size() > 9 ? tokens[9].toFloat() : 0.0f;
        }
        rawPoints_.append(pt);
    }

    // Build segments
    segments_.clear();
    for (int i = 0; i + 1 < rawPoints_.size(); ++i) {
        const PathControlPoint& a = rawPoints_[i];
        const PathControlPoint& b = rawPoints_[i + 1];
        const PathVector3 pa{a.x, a.y, a.z};
        const PathVector3 pb{b.x, b.y, b.z};
        const PathVector3 delta = pb - pa;
        const float length = delta.length();

        PathSegment seg;
        seg.fromIndex = i;
        seg.toIndex = i + 1;
        seg.lengthMm = length;
        seg.directionMmPerMm = (length > 1e-6f) ? (delta * (1.0f / length))
                                                 : PathVector3{1, 0, 0};
        if (explicitQuatMode_) {
            seg.startOrientation = PathQuaternion{a.q1, a.q2, a.q3, a.q4};
            seg.endOrientation = PathQuaternion{b.q1, b.q2, b.q3, b.q4};
        } else {
            seg.startOrientation = directionToQuaternion(a.i, a.j, a.k);
            seg.endOrientation = directionToQuaternion(b.i, b.j, b.k);
        }
        segments_.append(seg);
    }

    return rawPoints_.size() >= 2;
}

PathVector3 PathLoader::positionAt(int segmentIndex, float sMm) const {
    if (segmentIndex < 0 || segmentIndex >= segments_.size()) {
        return {};
    }
    const PathSegment& seg = segments_[segmentIndex];
    if (seg.lengthMm <= 1e-6f) {
        const auto& p = rawPoints_[seg.fromIndex];
        return {p.x, p.y, p.z};
    }
    const float t = std::clamp(sMm / seg.lengthMm, 0.0f, 1.0f);
    const auto& p0 = rawPoints_[seg.fromIndex];
    return PathVector3{p0.x, p0.y, p0.z} + seg.directionMmPerMm * (seg.lengthMm * t);
}

PathQuaternion PathLoader::orientationAt(int segmentIndex, float sMm) const {
    if (segmentIndex < 0 || segmentIndex >= segments_.size()) {
        return {};
    }
    const PathSegment& seg = segments_[segmentIndex];
    const float t = std::clamp(sMm / seg.lengthMm, 0.0f, 1.0f);
    return Slerp(seg.startOrientation, seg.endOrientation, t);
}

void PathLoader::jointPositionAt(int segmentIndex, float sDeg, float outJoints[6]) const {
    for (int axis = 0; axis < 6; ++axis) {
        outJoints[axis] = 0.0f;
    }
    if (jointPoints_.isEmpty() || jointSegments_.isEmpty()) {
        return;
    }
    if (segmentIndex < 0 || segmentIndex >= jointSegments_.size()) {
        const PathJointTarget& last = jointPoints_.last();
        for (int axis = 0; axis < 6; ++axis) {
            outJoints[axis] = last.joints[axis];
        }
        return;
    }
    const PathJointSegment& seg = jointSegments_[segmentIndex];
    const PathJointTarget& a = jointPoints_[seg.fromIndex];
    const PathJointTarget& b = jointPoints_[seg.toIndex];
    const float t = (seg.lengthDeg <= 1e-6f)
                        ? 0.0f
                        : std::clamp(sDeg / seg.lengthDeg, 0.0f, 1.0f);
    for (int axis = 0; axis < 6; ++axis) {
        outJoints[axis] = a.joints[axis] + (b.joints[axis] - a.joints[axis]) * t;
    }
}

PathQuaternion PathLoader::directionToQuaternion(float i, float j, float k) {
    // Treat (i,j,k) as the primary axis direction of the tool frame and
    // construct a right-handed frame with world-up = (0,0,1). Convert that
    // rotation matrix to a quaternion.
    const float len = std::sqrt(i * i + j * j + k * k);
    if (len < 1e-6f) {
        return {1, 0, 0, 0};
    }
    const float ax = i / len, ay = j / len, az = k / len;
    // cross with world up
    float cx = 0 * az - 1 * ay;
    float cy = 1 * ax - 0 * az;
    float cz = 0 * ay - 0 * ax;
    const float clen = std::sqrt(cx * cx + cy * cy + cz * cz);
    if (clen < 1e-6f) {
        // Degenerate direction (parallel to world up) — apply a fixed roll.
        return {std::cos(0.25f), std::sin(0.25f), 0, 0};
    }
    cx /= clen; cy /= clen; cz /= clen;
    const float bx = ay * cz - az * cy;
    const float by = az * cx - ax * cz;
    const float bz = ax * cy - ay * cx;

    // Build rotation matrix (columns: primary axis, cross, cross2).
    // R = | ax bx cx |
    //     | ay by cy |
    //     | az bz cz |
    // Then convert to quaternion (q1=scalar, q2..q4=i/j/k).
    const float tr = ax + by + cz;
    float q1, q2, q3, q4;
    if (tr > 0) {
        const float s = std::sqrt(tr + 1.0f) * 2.0f;
        q1 = 0.25f * s;
        q2 = (bz - cy) / s;
        q3 = (cx - az) / s;
        q4 = (ay - bx) / s;
    } else if (ax > by && ax > cz) {
        const float s = std::sqrt(1.0f + ax - by - cz) * 2.0f;
        q1 = (bz - cy) / s;
        q2 = 0.25f * s;
        q3 = (bx + ay) / s;
        q4 = (cx + az) / s;
    } else if (by > cz) {
        const float s = std::sqrt(1.0f + by - ax - cz) * 2.0f;
        q1 = (cx - az) / s;
        q2 = (bx + ay) / s;
        q3 = 0.25f * s;
        q4 = (cy + bz) / s;
    } else {
        const float s = std::sqrt(1.0f + cz - ax - by) * 2.0f;
        q1 = (ay - bx) / s;
        q2 = (cx + az) / s;
        q3 = (cy + bz) / s;
        q4 = 0.25f * s;
    }
    return {q1, q2, q3, q4};
}
