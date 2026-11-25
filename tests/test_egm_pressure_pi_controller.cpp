#include <QtTest/QtTest>

#include "egm/PressurePiController.h"

namespace {

egm::PressurePiInput makeInput(double pressureMean, bool enabled = true) {
    egm::PressurePiInput input;
    input.pressureMean = pressureMean;
    input.sampleValid = true;
    input.sampleAgeMs = 0.0;
    input.correctionEnabled = enabled;
    return input;
}

bool closeTo(double actual, double expected, double tolerance = 1e-9) {
    return std::abs(actual - expected) <= tolerance;
}

}  // namespace

class TestPressurePiController final : public QObject {
    Q_OBJECT

private slots:
    void disabledKeepsZeroOutput();
    void proportionalOutputsPositiveWhenOverPressure();
    void proportionalOutputsNegativeWhenUnderPressure();
    void clampsToZMax();
    void appliesRateLimitPerCycle();
    void integratesErrorOverTime();
    void freezesIntegralWhenSaturated();
    void rampsToZeroWhenSampleInvalid();
    void rampsToZeroWhenSampleStale();
    void setConfigKeepsIntegral();
    void nonPositiveDtKeepsPreviousOutput();
    void derivativeReactsToPressureRise();
    void resetClearsDerivativeState();
};

void TestPressurePiController::disabledKeepsZeroOutput() {
    egm::PressurePiController controller;
    const double out = controller.update(makeInput(50.0, false), 0.048);
    QVERIFY(closeTo(out, 0.0));
}

void TestPressurePiController::proportionalOutputsPositiveWhenOverPressure() {
    egm::PressurePiConfig config;
    config.setpoint = 10.0;
    config.kp = 0.1;
    config.dzMaxPerCycleMm = 10.0;
    egm::PressurePiController controller(config);

    const double out = controller.update(makeInput(20.0), 0.048);
    QVERIFY(closeTo(out, 1.0));
}

void TestPressurePiController::proportionalOutputsNegativeWhenUnderPressure() {
    egm::PressurePiConfig config;
    config.setpoint = 10.0;
    config.kp = 0.1;
    config.dzMaxPerCycleMm = 10.0;
    egm::PressurePiController controller(config);

    const double out = controller.update(makeInput(5.0), 0.048);
    QVERIFY(closeTo(out, -0.5));
}

void TestPressurePiController::clampsToZMax() {
    egm::PressurePiConfig config;
    config.setpoint = 0.0;
    config.kp = 1.0;
    config.zMaxMm = 3.0;
    config.dzMaxPerCycleMm = 100.0;
    egm::PressurePiController controller(config);

    const double out = controller.update(makeInput(10.0), 0.048);
    QVERIFY(closeTo(out, 3.0));
    QVERIFY(controller.saturated());
}

void TestPressurePiController::appliesRateLimitPerCycle() {
    egm::PressurePiConfig config;
    config.setpoint = 0.0;
    config.kp = 1.0;
    config.zMaxMm = 3.0;
    config.dzMaxPerCycleMm = 0.1;
    egm::PressurePiController controller(config);

    QVERIFY(closeTo(controller.update(makeInput(10.0), 0.048), 0.1));
    QVERIFY(closeTo(controller.update(makeInput(10.0), 0.048), 0.2));
}

void TestPressurePiController::integratesErrorOverTime() {
    egm::PressurePiConfig config;
    config.setpoint = 0.0;
    config.kp = 0.0;
    config.ki = 1.0;
    config.dzMaxPerCycleMm = 10.0;
    egm::PressurePiController controller(config);

    QVERIFY(closeTo(controller.update(makeInput(10.0), 0.1), 1.0));
    QVERIFY(closeTo(controller.integralTerm(), 1.0));
}

void TestPressurePiController::freezesIntegralWhenSaturated() {
    egm::PressurePiConfig config;
    config.setpoint = 0.0;
    config.kp = 1.0;
    config.ki = 1.0;
    config.zMaxMm = 1.0;
    config.dzMaxPerCycleMm = 1.0;
    egm::PressurePiController controller(config);

    for (int i = 0; i < 5; ++i) {
        controller.update(makeInput(10.0), 0.1);
    }

    QVERIFY(closeTo(controller.outputMm(), 1.0));
    // 饱和期间误差持续为正，积分项必须被冻结，不能无限累积。
    QVERIFY(closeTo(controller.integralTerm(), 0.0));
}

void TestPressurePiController::rampsToZeroWhenSampleInvalid() {
    egm::PressurePiConfig config;
    config.setpoint = 0.0;
    config.kp = 0.1;
    config.dzMaxPerCycleMm = 10.0;
    config.rampRateMmPerS = 5.0;
    egm::PressurePiController controller(config);

    QVERIFY(closeTo(controller.update(makeInput(10.0), 0.048), 1.0));

    egm::PressurePiInput invalid = makeInput(10.0);
    invalid.sampleValid = false;
    const double out = controller.update(invalid, 0.048);
    QVERIFY(closeTo(out, 1.0 - 5.0 * 0.048));
}

void TestPressurePiController::rampsToZeroWhenSampleStale() {
    egm::PressurePiConfig config;
    config.setpoint = 0.0;
    config.kp = 0.1;
    config.dzMaxPerCycleMm = 10.0;
    config.staleTimeoutMs = 300.0;
    config.rampRateMmPerS = 5.0;
    egm::PressurePiController controller(config);

    QVERIFY(closeTo(controller.update(makeInput(10.0), 0.048), 1.0));

    egm::PressurePiInput stale = makeInput(10.0);
    stale.sampleAgeMs = 500.0;
    const double out = controller.update(stale, 0.048);
    QVERIFY(closeTo(out, 1.0 - 5.0 * 0.048));
    QVERIFY(closeTo(controller.integralTerm(), 0.0));
}

void TestPressurePiController::setConfigKeepsIntegral() {
    egm::PressurePiConfig config;
    config.setpoint = 0.0;
    config.kp = 0.0;
    config.ki = 1.0;
    config.dzMaxPerCycleMm = 10.0;
    egm::PressurePiController controller(config);

    controller.update(makeInput(10.0), 0.1);
    const double integralBefore = controller.integralTerm();

    egm::PressurePiConfig updated = config;
    updated.kp = 0.5;
    controller.setConfig(updated);

    QVERIFY(closeTo(controller.integralTerm(), integralBefore));
}

void TestPressurePiController::nonPositiveDtKeepsPreviousOutput() {
    egm::PressurePiConfig config;
    config.setpoint = 0.0;
    config.kp = 0.1;
    config.dzMaxPerCycleMm = 10.0;
    egm::PressurePiController controller(config);

    QVERIFY(closeTo(controller.update(makeInput(10.0), 0.048), 1.0));
    QVERIFY(closeTo(controller.update(makeInput(10.0), 0.0), 1.0));
    QVERIFY(closeTo(controller.update(makeInput(10.0), -0.1), 1.0));
}

void TestPressurePiController::derivativeReactsToPressureRise() {
    egm::PressurePiConfig config;
    config.setpoint = 0.0;
    config.kp = 0.0;
    config.ki = 0.0;
    config.kd = 1.0;
    config.zMaxMm = 3.0;
    config.dzMaxPerCycleMm = 10.0;
    egm::PressurePiController controller(config);

    // 首帧没有历史压力，微分项为 0。
    QVERIFY(closeTo(controller.update(makeInput(1.0), 0.1), 0.0));
    // 第二帧压力上升 1.0，kd * 1.0 / 0.1 = 10，按 zMaxMm 限幅到 3.0。
    QVERIFY(closeTo(controller.update(makeInput(2.0), 0.1), 3.0));
}

void TestPressurePiController::resetClearsDerivativeState() {
    egm::PressurePiConfig config;
    config.setpoint = 0.0;
    config.kp = 0.0;
    config.ki = 0.0;
    config.kd = 1.0;
    config.zMaxMm = 3.0;
    config.dzMaxPerCycleMm = 10.0;
    egm::PressurePiController controller(config);

    controller.update(makeInput(1.0), 0.1);
    QVERIFY(closeTo(controller.update(makeInput(2.0), 0.1), 3.0));

    controller.reset();
    QVERIFY(closeTo(controller.update(makeInput(5.0), 0.1), 0.0));
}

QTEST_MAIN(TestPressurePiController)
#include "test_egm_pressure_pi_controller.moc"
