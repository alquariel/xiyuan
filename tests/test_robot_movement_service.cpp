#include <QtTest/QtTest>

#include <array>
#include <stdexcept>

#include <QElapsedTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QSignalSpy>
#include <QThread>
#include <QWidget>

#include "robot/PlcRobotMotionClient.h"
#include "robot/RobotQuaternion.h"
#include "services/robot/RobotMovementService.h"
#include "ui_Interface4600_window.h"

class FakeMotionController final : public IRobotMotionController {
public:
    RobotSpeed lastSpeed;
    QStringList commands;
    RobotPose lastPose;
    std::array<double, 6> lastJoints{};
    std::array<double, 6> lastExternalAxis{};
    RobotMotionResult nextResult = RobotMotionResult::success();
    bool throwOnSetSpeed = false;
    bool throwOnJoints = false;
    bool throwOnExternalAxis = false;
    int commandDelayMs = 0;

    int commandCount() const {
        QMutexLocker lock(&mutex_);
        return commands.count();
    }

    QStringList commandSnapshot() const {
        QMutexLocker lock(&mutex_);
        return commands;
    }

    RobotSpeed speedSnapshot() const {
        QMutexLocker lock(&mutex_);
        return lastSpeed;
    }

    RobotPose poseSnapshot() const {
        QMutexLocker lock(&mutex_);
        return lastPose;
    }

    std::array<double, 6> jointsSnapshot() const {
        QMutexLocker lock(&mutex_);
        return lastJoints;
    }

    std::array<double, 6> externalAxisSnapshot() const {
        QMutexLocker lock(&mutex_);
        return lastExternalAxis;
    }

    void setSpeed(const RobotSpeed& speed) override {
        if (throwOnSetSpeed) {
            throw std::runtime_error("speed controller failed");
        }
        QMutexLocker lock(&mutex_);
        lastSpeed = speed;
    }

    RobotMotionResult moveCartesian(const RobotPose& pose) override {
        QMutexLocker lock(&mutex_);
        lastPose = pose;
        commands << "cartesian";
        lock.unlock();
        waitIfDelayed();
        return nextResult;
    }

    RobotMotionResult moveJoints(const std::array<double, 6>& joints) override {
        if (throwOnJoints) {
            throw std::runtime_error("joint controller failed");
        }
        QMutexLocker lock(&mutex_);
        lastJoints = joints;
        commands << "joints";
        lock.unlock();
        waitIfDelayed();
        return nextResult;
    }

    RobotMotionResult moveExternalAxis(const std::array<double, 6>& axisValues) override {
        if (throwOnExternalAxis) {
            throw std::runtime_error("external controller failed");
        }
        QMutexLocker lock(&mutex_);
        lastExternalAxis = axisValues;
        commands << "external";
        lock.unlock();
        waitIfDelayed();
        return nextResult;
    }

private:
    void waitIfDelayed() {
        if (commandDelayMs > 0) {
            QThread::msleep(static_cast<unsigned long>(commandDelayMs));
        }
    }

    mutable QMutex mutex_;
};

class TestRobotMovementService final : public QObject {
    Q_OBJECT

private slots:
    void missingCartesianFieldEmitsMessageAndSendsNoMotionCommand();
    void cartesianMoveUsesConfiguredSpeedsWhenSpeedFieldsAreEmpty();
    void cartesianMoveOverridesSpeedsFromUiFields();
    void invalidSpeedFieldEmitsMessageAndSendsNoMotionCommand();
    void zeroSpeedFieldEmitsMessageAndSendsNoMotionCommand();
    void negativeSpeedFieldEmitsMessageAndSendsNoMotionCommand();
    void setSpeedExceptionEmitsHardwareMessage();
    void motionFailureEmitsHardwareMessage();
    void emptyMotionFailureUsesFallbackMessage();
    void zeroOrientationVectorEmitsValidationMessageAndSendsNoMotionCommand();
    void robotHomeUsesNegativeZOrientationVector();
    void missingJointFieldEmitsMessageAndSendsNoMotionCommand();
    void jointMoveSendsSixAxisValuesInOrder();
    void missingExternalAxisFieldEmitsMessageAndSendsNoMotionCommand();
    void externalAxisMoveSendsPythonCompatibleAxisOrder();
    void externalAxisResetSendsPythonCompatibleHomePosition();
    void jointControllerExceptionEmitsHardwareMessage();
    void externalAxisControllerExceptionEmitsHardwareMessage();
    void commandAlreadyRunningEmitsRobotControlMessageAndSkipsSecondDispatch();
    void successfulAsyncCommandEventuallyReportsWithoutBlockingCaller();
};

namespace {

RobotConfig testRobotConfig() {
    RobotConfig config;
    config.defaultSpeed.tcpLinear = 11.0;
    config.defaultSpeed.tcpOrientation = 22.0;
    config.defaultSpeed.externalLinear = 33.0;
    config.defaultSpeed.externalOrientation = 44.0;
    return config;
}

void fillCartesian(Ui::ContinuosfiberInterface& ui) {
    ui.RobotXlineEdit->setText("100.5");
    ui.RobotYlineEdit->setText("200.5");
    ui.RobotZlineEdit->setText("300.5");
    ui.RobotRXlineEdit->setText("1.5");
    ui.RobotRYlineEdit->setText("2.5");
    ui.RobotRZlineEdit->setText("3.5");
}

void fillHome(Ui::ContinuosfiberInterface& ui) {
    ui.zero_position_x->setText("10");
    ui.zero_position_y->setText("20");
    ui.zero_position_z->setText("30");
}

void fillJoints(Ui::ContinuosfiberInterface& ui) {
    ui.axis1lineEdit->setText("1");
    ui.axis2lineEdit->setText("2");
    ui.axis3lineEdit->setText("3");
    ui.axis4lineEdit->setText("4");
    ui.axis5lineEdit->setText("5");
    ui.axis6lineEdit->setText("6");
}

void fillExternalAxes(Ui::ContinuosfiberInterface& ui) {
    ui.Positioner1lineEdit->setText("101");
    ui.Positioner2lineEdit->setText("202");
    ui.Positioner3lineEdit->setText("303");
}

void comparePose(const RobotPose& actual, const RobotPose& expected) {
    for (int i = 0; i < 3; ++i) {
        QCOMPARE(actual.position[i], expected.position[i]);
    }
    for (int i = 0; i < 4; ++i) {
        QCOMPARE(actual.quaternion[i], expected.quaternion[i]);
    }
}

} // namespace

void TestRobotMovementService::missingCartesianFieldEmitsMessageAndSendsNoMotionCommand() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillCartesian(ui);
    ui.RobotZlineEdit->clear();
    FakeMotionController motion;
    RobotMovementService service(testRobotConfig(), motion, &ui);
    QSignalSpy messages(&service, &RobotMovementService::messageRequested);

    service.moveRobot();

    QCOMPARE(messages.count(), 1);
    QCOMPARE(motion.commandCount(), 0);
}

void TestRobotMovementService::cartesianMoveUsesConfiguredSpeedsWhenSpeedFieldsAreEmpty() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillCartesian(ui);
    FakeMotionController motion;
    RobotMovementService service(testRobotConfig(), motion, &ui);

    service.moveRobot();

    QTRY_COMPARE(motion.commandCount(), 1);
    QCOMPARE(motion.commandSnapshot(), QStringList({"cartesian"}));
    const RobotSpeed speed = motion.speedSnapshot();
    QCOMPARE(speed.tcpLinear, 11.0);
    QCOMPARE(speed.tcpOrientation, 22.0);
    QCOMPARE(speed.externalLinear, 33.0);
    QCOMPARE(speed.externalOrientation, 44.0);
    comparePose(motion.poseSnapshot(), RobotQuaternion::makePose(100.5, 200.5, 300.5, 1.5, 2.5, 3.5));
}

void TestRobotMovementService::cartesianMoveOverridesSpeedsFromUiFields() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillCartesian(ui);
    ui.RobotlinespeedlineEdit->setText("101");
    ui.RobotangularspeedlineEdit->setText("202");
    ui.AxislinespeedlineEdit->setText("303");
    ui.AxisangularspeedlineEdit->setText("404");
    FakeMotionController motion;
    RobotMovementService service(testRobotConfig(), motion, &ui);

    service.moveRobot();

    QTRY_COMPARE(motion.commandCount(), 1);
    const RobotSpeed speed = motion.speedSnapshot();
    QCOMPARE(speed.tcpLinear, 101.0);
    QCOMPARE(speed.tcpOrientation, 202.0);
    QCOMPARE(speed.externalLinear, 303.0);
    QCOMPARE(speed.externalOrientation, 404.0);
}

void TestRobotMovementService::invalidSpeedFieldEmitsMessageAndSendsNoMotionCommand() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillCartesian(ui);
    ui.RobotlinespeedlineEdit->setText("fast");
    FakeMotionController motion;
    RobotMovementService service(testRobotConfig(), motion, &ui);
    QSignalSpy messages(&service, &RobotMovementService::messageRequested);

    service.moveRobot();

    QCOMPARE(messages.count(), 1);
    QCOMPARE(messages.at(0).at(0).toString(), QString("Robot control"));
    QCOMPARE(motion.commandCount(), 0);
}

void TestRobotMovementService::zeroSpeedFieldEmitsMessageAndSendsNoMotionCommand() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillCartesian(ui);
    ui.AxislinespeedlineEdit->setText("0");
    FakeMotionController motion;
    RobotMovementService service(testRobotConfig(), motion, &ui);
    QSignalSpy messages(&service, &RobotMovementService::messageRequested);

    service.moveRobot();

    QCOMPARE(messages.count(), 1);
    QCOMPARE(messages.at(0).at(0).toString(), QString("Robot control"));
    QVERIFY(messages.at(0).at(1).toString().contains("External axis linear speed"));
    QCOMPARE(motion.commandCount(), 0);
}

void TestRobotMovementService::negativeSpeedFieldEmitsMessageAndSendsNoMotionCommand() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillCartesian(ui);
    ui.RobotangularspeedlineEdit->setText("-1");
    FakeMotionController motion;
    RobotMovementService service(testRobotConfig(), motion, &ui);
    QSignalSpy messages(&service, &RobotMovementService::messageRequested);

    service.moveRobot();

    QCOMPARE(messages.count(), 1);
    QCOMPARE(messages.at(0).at(0).toString(), QString("Robot control"));
    QVERIFY(messages.at(0).at(1).toString().contains("Robot angular speed"));
    QCOMPARE(motion.commandCount(), 0);
}

void TestRobotMovementService::setSpeedExceptionEmitsHardwareMessage() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillCartesian(ui);
    FakeMotionController motion;
    motion.throwOnSetSpeed = true;
    RobotMovementService service(testRobotConfig(), motion, &ui);
    QSignalSpy messages(&service, &RobotMovementService::messageRequested);

    service.moveRobot();

    QTRY_COMPARE(messages.count(), 1);
    QCOMPARE(messages.at(0).at(0).toString(), QString("Robot"));
    QCOMPARE(motion.commandCount(), 0);
}

void TestRobotMovementService::motionFailureEmitsHardwareMessage() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillCartesian(ui);
    FakeMotionController motion;
    motion.nextResult = RobotMotionResult::failure("plc rejected command");
    RobotMovementService service(testRobotConfig(), motion, &ui);
    QSignalSpy messages(&service, &RobotMovementService::messageRequested);

    service.moveRobot();

    QTRY_COMPARE(messages.count(), 1);
    QCOMPARE(messages.at(0).at(0).toString(), QString("Robot"));
    QCOMPARE(motion.commandSnapshot(), QStringList({"cartesian"}));
}

void TestRobotMovementService::emptyMotionFailureUsesFallbackMessage() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillCartesian(ui);
    FakeMotionController motion;
    motion.nextResult = RobotMotionResult::failure("");
    RobotMovementService service(testRobotConfig(), motion, &ui);
    QSignalSpy messages(&service, &RobotMovementService::messageRequested);

    service.moveRobot();

    QTRY_COMPARE(messages.count(), 1);
    QCOMPARE(messages.at(0).at(0).toString(), QString("Robot"));
    QVERIFY(!messages.at(0).at(1).toString().trimmed().endsWith(":"));
}

void TestRobotMovementService::zeroOrientationVectorEmitsValidationMessageAndSendsNoMotionCommand() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillCartesian(ui);
    ui.RobotRXlineEdit->setText("0");
    ui.RobotRYlineEdit->setText("0");
    ui.RobotRZlineEdit->setText("0");
    FakeMotionController motion;
    RobotMovementService service(testRobotConfig(), motion, &ui);
    QSignalSpy messages(&service, &RobotMovementService::messageRequested);

    service.moveRobot();

    QCOMPARE(messages.count(), 1);
    QCOMPARE(messages.at(0).at(0).toString(), QString("Robot control"));
    QCOMPARE(motion.commandCount(), 0);
}

void TestRobotMovementService::robotHomeUsesNegativeZOrientationVector() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillHome(ui);
    FakeMotionController motion;
    RobotMovementService service(testRobotConfig(), motion, &ui);

    service.resetRobot();

    QTRY_COMPARE(motion.commandCount(), 1);
    QCOMPARE(motion.commandSnapshot(), QStringList({"cartesian"}));
    comparePose(motion.poseSnapshot(), RobotQuaternion::makePose(10.0, 20.0, 30.0, 0.0, 0.0, -1.0));
}

void TestRobotMovementService::missingJointFieldEmitsMessageAndSendsNoMotionCommand() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillJoints(ui);
    ui.axis4lineEdit->clear();
    FakeMotionController motion;
    RobotMovementService service(testRobotConfig(), motion, &ui);
    QSignalSpy messages(&service, &RobotMovementService::messageRequested);

    service.moveRobotAxes();

    QCOMPARE(messages.count(), 1);
    QCOMPARE(messages.at(0).at(0).toString(), QString("Robot control"));
    QCOMPARE(motion.commandCount(), 0);
}

void TestRobotMovementService::jointMoveSendsSixAxisValuesInOrder() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillJoints(ui);
    FakeMotionController motion;
    RobotMovementService service(testRobotConfig(), motion, &ui);

    service.moveRobotAxes();

    QTRY_COMPARE(motion.commandCount(), 1);
    QCOMPARE(motion.commandSnapshot(), QStringList({"joints"}));
    QCOMPARE(motion.jointsSnapshot(), (std::array<double, 6>{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}));
}

void TestRobotMovementService::missingExternalAxisFieldEmitsMessageAndSendsNoMotionCommand() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillExternalAxes(ui);
    ui.Positioner2lineEdit->clear();
    FakeMotionController motion;
    RobotMovementService service(testRobotConfig(), motion, &ui);
    QSignalSpy messages(&service, &RobotMovementService::messageRequested);

    service.moveExternalAxis();

    QCOMPARE(messages.count(), 1);
    QCOMPARE(messages.at(0).at(0).toString(), QString("Robot control"));
    QCOMPARE(motion.commandCount(), 0);
}

void TestRobotMovementService::externalAxisMoveSendsPythonCompatibleAxisOrder() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillExternalAxes(ui);
    FakeMotionController motion;
    RobotMovementService service(testRobotConfig(), motion, &ui);

    service.moveExternalAxis();

    QTRY_COMPARE(motion.commandCount(), 1);
    QCOMPARE(motion.commandSnapshot(), QStringList({"external"}));
    QCOMPARE(motion.externalAxisSnapshot(), (std::array<double, 6>{303.0, 101.0, 202.0, 0.0, 0.0, 0.0}));
}

void TestRobotMovementService::externalAxisResetSendsPythonCompatibleHomePosition() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    FakeMotionController motion;
    RobotMovementService service(testRobotConfig(), motion, &ui);

    service.resetExternalAxis();

    QTRY_COMPARE(motion.commandCount(), 1);
    QCOMPARE(motion.commandSnapshot(), QStringList({"external"}));
    QCOMPARE(motion.externalAxisSnapshot(), (std::array<double, 6>{-1900.0, 0.0, 0.0, 0.0, 0.0, 0.0}));
}

void TestRobotMovementService::jointControllerExceptionEmitsHardwareMessage() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillJoints(ui);
    FakeMotionController motion;
    motion.throwOnJoints = true;
    RobotMovementService service(testRobotConfig(), motion, &ui);
    QSignalSpy messages(&service, &RobotMovementService::messageRequested);

    service.moveRobotAxes();

    QTRY_COMPARE(messages.count(), 1);
    QCOMPARE(messages.at(0).at(0).toString(), QString("Robot"));
}

void TestRobotMovementService::externalAxisControllerExceptionEmitsHardwareMessage() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillExternalAxes(ui);
    FakeMotionController motion;
    motion.throwOnExternalAxis = true;
    RobotMovementService service(testRobotConfig(), motion, &ui);
    QSignalSpy messages(&service, &RobotMovementService::messageRequested);

    service.moveExternalAxis();

    QTRY_COMPARE(messages.count(), 1);
    QCOMPARE(messages.at(0).at(0).toString(), QString("Robot"));
}

void TestRobotMovementService::commandAlreadyRunningEmitsRobotControlMessageAndSkipsSecondDispatch() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillCartesian(ui);
    FakeMotionController motion;
    motion.commandDelayMs = 200;
    RobotMovementService service(testRobotConfig(), motion, &ui);
    QSignalSpy messages(&service, &RobotMovementService::messageRequested);

    service.moveRobot();
    QTRY_COMPARE(motion.commandCount(), 1);
    service.moveRobot();

    QTRY_COMPARE(messages.count(), 1);
    QCOMPARE(messages.at(0).at(0).toString(), QString("Robot control"));
    QCOMPARE(motion.commandCount(), 1);
}

void TestRobotMovementService::successfulAsyncCommandEventuallyReportsWithoutBlockingCaller() {
    QWidget widget;
    Ui::ContinuosfiberInterface ui;
    ui.setupUi(&widget);
    fillCartesian(ui);
    FakeMotionController motion;
    motion.nextResult = RobotMotionResult::success("cartesian move accepted");
    motion.commandDelayMs = 200;
    RobotMovementService service(testRobotConfig(), motion, &ui);
    QSignalSpy messages(&service, &RobotMovementService::messageRequested);

    QElapsedTimer elapsed;
    elapsed.start();
    service.moveRobot();

    QVERIFY(elapsed.elapsed() < 100);
    QTRY_COMPARE(motion.commandCount(), 1);
    QTRY_COMPARE(messages.count(), 1);
    QCOMPARE(messages.at(0).at(0).toString(), QString("Robot"));
    QCOMPARE(messages.at(0).at(1).toString(), QString("cartesian move accepted"));
}

QTEST_MAIN(TestRobotMovementService)
#include "test_robot_movement_service.moc"
