#pragma once

#include <QObject>
#include <QString>
#include <QList>

// POD rotation (quaternion scalar-first, matching ABB EGM convention:
// q1=scalar, q2..q4=i/j/k). Kept free of Qt6::Gui so PathLoader can be used
// by the unit tests without pulling QtGui.
struct PathQuaternion {
    float q1 = 1;
    float q2 = 0;
    float q3 = 0;
    float q4 = 0;
};

// POD position (mm). Again kept independent of QtGui.
struct PathVector3 {
    float x = 0;
    float y = 0;
    float z = 0;

    PathVector3 operator+(const PathVector3& other) const {
        return {x + other.x, y + other.y, z + other.z};
    }
    PathVector3 operator-(const PathVector3& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }
    PathVector3 operator*(float scalar) const {
        return {x * scalar, y * scalar, z * scalar};
    }
    float length() const;
};

// One row from the legacy Python path-control .txt files. 10 columns,
// whitespace-separated, comments prefixed with '#' are ignored.
struct PathControlPoint {
    // Cartesian position [mm]
    float x = 0;
    float y = 0;
    float z = 0;

    // Orientation direction vector — Python converts (i,j,k) to target quaternion
    // around a reference tool axis; here we just stash the raw triplet.
    float i = 0;
    float j = 0;
    float k = 0;

    // Explicit TCP orientation (quaternion, ABB order q1=scalar).
    // Only filled when the file declares "# MODE=POSE"; columns are then
    // "x y z q1 q2 q3 q4 v" and the i/j/k triplet above is left unused.
    float q1 = 1;
    float q2 = 0;
    float q3 = 0;
    float q4 = 0;

    // Legacy columns 7-10 exist but are not used by the C++ EGM interpolator
    // (speed is controlled by targetSpeed, turning angle is unused, tangent
    // basis is unused in position-only mode). We keep them so we can still
    // round-trip a path file without information loss.
    float v = 0;
    float angle = 0;
    float a = 0;
    float b = 0;
};

// One row of a joint-space path file. Written in DEGREES; the EGM encoder
// converts to radians because the controller expects radians on the wire.
struct PathJointTarget {
    float joints[6] = {0, 0, 0, 0, 0, 0};
    float v = 0;   // deg/s
};

struct PathJointSegment {
    int fromIndex = -1;
    int toIndex = -1;
    float lengthDeg = 0;
};

struct PathSegment {
    // Start / end row indices in the original PathControlPoint list
    int fromIndex = -1;
    int toIndex = -1;

    // Precomputed unit direction vector of the segment (P1-P0)/|P1-P0|
    PathVector3 directionMmPerMm;

    // Segment arc length in mm
    float lengthMm = 0;

    // Average orientation over the segment — derived from (i,j,k) start/end.
    PathQuaternion startOrientation;
    PathQuaternion endOrientation;
};

class PathLoader : public QObject {
public:
    explicit PathLoader(QObject* parent = nullptr);

    // Load a path file from disk. Returns true on success (>= 2 valid rows).
    bool loadFromTxt(const QString& filePath);

    const QList<PathControlPoint>& rawPoints() const { return rawPoints_; }
    const QList<PathSegment>& segments() const { return segments_; }

    // Return the position (mm) at parametric coordinate s ∈ [0, segment.length]
    // along the requested segment. s is clamped to [0, segment.length].
    PathVector3 positionAt(int segmentIndex, float sMm) const;

    // Return the interpolated orientation at parametric coordinate s ∈ [0, segment.length].
    PathQuaternion orientationAt(int segmentIndex, float sMm) const;

    // Number of segments (equals rawPoints.size() - 1 if loadFromTxt succeeded).
    int segmentCount() const { return segments_.size(); }

    // ---- joint-space paths ----------------------------------------------
    // A file is treated as a joint path when one of its comment lines
    // contains MODE=JOINT. Format: 7 columns "j1 j2 j3 j4 j5 j6 v"
    // with joints in DEGREES and v in DEG/S.
    bool isJointPath() const { return jointMode_; }
    const QList<PathJointTarget>& jointPoints() const { return jointPoints_; }
    const QList<PathJointSegment>& jointSegments() const { return jointSegments_; }
    int jointSegmentCount() const { return jointSegments_.size(); }

    // Interpolate the 6 joint angles (DEGREES) at sDeg along the given segment.
    void jointPositionAt(int segmentIndex, float sDeg, float outJoints[6]) const;

private:
    // Convert raw (i,j,k) direction-vector start/end to quaternion in the
    // tool's rotation frame. This keeps the behavior compatible with the
    // Python main-window's quaternion construction.
    static PathQuaternion directionToQuaternion(float i, float j, float k);

    QList<PathControlPoint> rawPoints_;
    QList<PathSegment> segments_;

    bool jointMode_ = false;
    QList<PathJointTarget> jointPoints_;
    QList<PathJointSegment> jointSegments_;

    // true when the file declares "# MODE=POSE" (explicit quaternion column).
    bool explicitQuatMode_ = false;
};
