#include <QtTest/QtTest>

#include "robot/RobotQuaternion.h"

#include <limits>
#include <stdexcept>

class TestRobotQuaternion final : public QObject {
    Q_OBJECT

private slots:
    void matchesPythonForHomeVector();
    void matchesPythonForXAxisVector();
    void makePoseKeepsPositionAndQuaternion();
    void rejectsZeroOrientationVector();
    void rejectsNonFiniteOrientationVector();
    void rejectsNanOrientationVector();
    void tinyDirectionVectorMatchesXAxis();
    void largeDirectionVectorMatchesXAxis();
    void maxComponentDirectionVectorMatchesUnitDirection();
    void denormalDirectionVectorMatchesUnitDirection();
};

void compareQuaternion(const std::array<double, 4>& actual, const std::array<double, 4>& expected) {
    for (int i = 0; i < 4; ++i) {
        QVERIFY2(std::abs(actual[i] - expected[i]) < 1e-8, qPrintable(QString("index %1").arg(i)));
    }
}

void TestRobotQuaternion::matchesPythonForHomeVector() {
    compareQuaternion(RobotQuaternion::fromOrientationVector(0.0, 0.0, -1.0),
                      {-0.0, 0.2588190451, -0.9659258263, 0.0});
}

void TestRobotQuaternion::matchesPythonForXAxisVector() {
    compareQuaternion(RobotQuaternion::fromOrientationVector(1.0, 0.0, 0.0),
                      {-0.6830127019, 0.1830127019, -0.6830127019, 0.1830127019});
}

void TestRobotQuaternion::makePoseKeepsPositionAndQuaternion() {
    const RobotPose pose = RobotQuaternion::makePose(10.0, 20.0, 30.0, 0.0, 0.0, -1.0);
    QCOMPARE(pose.position[0], 10.0);
    QCOMPARE(pose.position[1], 20.0);
    QCOMPARE(pose.position[2], 30.0);
    compareQuaternion(pose.quaternion, {-0.0, 0.2588190451, -0.9659258263, 0.0});
}

void TestRobotQuaternion::rejectsZeroOrientationVector() {
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, RobotQuaternion::fromOrientationVector(0.0, 0.0, 0.0));
}

void TestRobotQuaternion::rejectsNonFiniteOrientationVector() {
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
                             RobotQuaternion::fromOrientationVector(std::numeric_limits<double>::infinity(), 0.0, -1.0));
}

void TestRobotQuaternion::rejectsNanOrientationVector() {
    QVERIFY_THROWS_EXCEPTION(
        std::invalid_argument,
        RobotQuaternion::fromOrientationVector(std::numeric_limits<double>::quiet_NaN(), 0.0, -1.0));
}

void TestRobotQuaternion::tinyDirectionVectorMatchesXAxis() {
    compareQuaternion(RobotQuaternion::fromOrientationVector(1e-300, 0.0, 0.0),
                      {-0.6830127019, 0.1830127019, -0.6830127019, 0.1830127019});
}

void TestRobotQuaternion::largeDirectionVectorMatchesXAxis() {
    compareQuaternion(RobotQuaternion::fromOrientationVector(1e300, 0.0, 0.0),
                      {-0.6830127019, 0.1830127019, -0.6830127019, 0.1830127019});
}

void TestRobotQuaternion::maxComponentDirectionVectorMatchesUnitDirection() {
    const double max = std::numeric_limits<double>::max();
    compareQuaternion(RobotQuaternion::fromOrientationVector(max, max, max),
                      RobotQuaternion::fromOrientationVector(1.0, 1.0, 1.0));
}

void TestRobotQuaternion::denormalDirectionVectorMatchesUnitDirection() {
    const double denorm = std::numeric_limits<double>::denorm_min();
    compareQuaternion(RobotQuaternion::fromOrientationVector(denorm, denorm, 0.0),
                      RobotQuaternion::fromOrientationVector(1.0, 1.0, 0.0));
}

QTEST_MAIN(TestRobotQuaternion)
#include "test_robot_quaternion.moc"
