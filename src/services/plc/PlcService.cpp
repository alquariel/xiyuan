#include "services/plc/PlcService.h"

#include <QLineEdit>
#include <QTextBrowser>

#include "ui_Interface4600_window.h"

namespace {

constexpr int kDb5 = 5;
constexpr int kConfirmRetries = 3;

}

PlcService::PlcService(const PlcConfig& config, Ui::ContinuosfiberInterface* ui, QObject* parent)
    : QObject(parent),
      ui_(ui),
      client_(config),
      enabled_(config.enabled) {
}

void PlcService::disconnectPlc() {
    client_.disconnect();
    showInfo("PLC disconnected.");
}

void PlcService::cylinder1() {
    if (!ensureConnected()) {
        return;
    }

    // Python cylinder1(): DB5.DBX14.5 is a one-shot cylinder command.
    writeDbBitConfirmed("Cylinder 1", 14, 5, true);
}

void PlcService::cylinder2() {
    if (!ensureConnected()) {
        return;
    }

    // Match the Python behavior: read the real PLC bit once, then write the
    // inverse once. These cylinder bits act like edge-sensitive toggle commands
    // on the machine, so retrying a confirmed write can toggle more than once.
    bool currentState = false;
    if (!runCommand("Cylinder 2 read", client_.readDbBit(kDb5, 14, 6, currentState))) {
        return;
    }

    const bool nextState = !currentState;
    if (runCommand("Cylinder 2", client_.writeDbBit(kDb5, 14, 6, nextState))) {
        if (ui_ != nullptr) {
            ui_->cylinder2_state->setChecked(nextState);
        }
    }
}

void PlcService::cylinder3() {
    if (!ensureConnected()) {
        return;
    }

    // Match the Python behavior: read DB5.DBX14.7 once and write its inverse
    // once. The checkbox display remains inverted, as in the Python UI.
    bool currentState = false;
    if (!runCommand("Cylinder 3 read", client_.readDbBit(kDb5, 14, 7, currentState))) {
        return;
    }

    const bool nextState = !currentState;
    if (runCommand("Cylinder 3", client_.writeDbBit(kDb5, 14, 7, nextState))) {
        if (ui_ != nullptr) {
            ui_->cylinder3_state->setChecked(!nextState);
        }
    }
}

void PlcService::motorEnable() {
    if (!ensureConnected()) {
        return;
    }

    // DB5.DBX15.1 enables/disables the feed motor in the Python service.
    const bool nextState = !motorEnabled_;
    if (writeDbBitConfirmed("Motor enable", 15, 1, nextState)) {
        motorEnabled_ = nextState;
        if (ui_ != nullptr) {
            ui_->state_show->setText(motorEnabled_ ? "Motor enabled" : "Motor disabled");
        }
    }
}

void PlcService::motorReset() {
    if (!motorEnabled_) {
        showInfo("Motor is not enabled.");
        return;
    }
    if (!ensureConnected()) {
        return;
    }

    // Python Motor_reset() writes marker bit M12.7.
    runCommand("Motor reset", client_.writeMBit(12, 7, true));
}

void PlcService::motorActuation() {
    if (!motorEnabled_) {
        showInfo("Motor is not enabled.");
        return;
    }
    if (!ensureConnected()) {
        return;
    }

    const bool jogMode = ui_ != nullptr && ui_->interruption->isChecked();
    const bool continuousMode = ui_ != nullptr && ui_->continuous->isChecked();

    if (jogMode) {
        // Jog mode: DB5.DBX15.0 true, then DB5.DBX40.0 triggers actuation.
        if (!runCommand("Motor jog command", client_.writeDbBit(kDb5, 15, 0, true))) {
            return;
        }
        runCommand("Motor actuation", client_.writeDbBit(kDb5, 40, 0, true));
        return;
    }

    if (continuousMode) {
        // Continuous mode clears DB5.DBX15.0, keeps DB5.DBX15.1 true, then
        // raises the shared actuation flag DB5.DBX40.0.
        if (!runCommand("Motor continuous command", client_.writeDbBit(kDb5, 15, 0, false))) {
            return;
        }
        if (!runCommand("Motor continuous enable", client_.writeDbBit(kDb5, 15, 1, true))) {
            return;
        }
        runCommand("Motor actuation", client_.writeDbBit(kDb5, 40, 0, true));
        return;
    }

    showInfo("Select jog or continuous motor mode first.");
}

void PlcService::parameterInput() {
    if (!motorEnabled_) {
        showInfo("Motor is not enabled.");
        return;
    }
    if (!ensureConnected()) {
        return;
    }

    const double wireFeedRate = readDoubleField(ui_ != nullptr ? ui_->Wire_feed_rate : nullptr, 10.0);
    const double pavingDistance = readDoubleField(ui_ != nullptr ? ui_->Paving_distance : nullptr, 1.0);

    // Python writes scaled REAL values: 1000*paving_distance and 60*wire_feed.
    if (!runCommand("Paving distance", client_.writeDbReal32(kDb5, 16, static_cast<float>(1000.0 * pavingDistance)))) {
        return;
    }
    runCommand("Wire feed rate", client_.writeDbReal32(kDb5, 20, static_cast<float>(60.0 * wireFeedRate)));
}

void PlcService::setMotorJogMode() {
    if (!ensureConnected()) {
        return;
    }

    // Python Motorstate("jog") writes INT 1 at DB5.DBW38.
    runCommand("Motor jog mode", client_.writeDbInt16(kDb5, 38, 1));
}

void PlcService::setMotorContinuousMode() {
    if (!ensureConnected()) {
        return;
    }

    // Python Motorstate("continuous") writes INT 3 at DB5.DBW38.
    runCommand("Motor continuous mode", client_.writeDbInt16(kDb5, 38, 3));
}
void PlcService::laserConnect() {
    if (!ensureConnected()) {
        return;
    }

    // Python Laser_connect(): DB5.DBX14.4 requests the laser connection.
    if (writeDbBitConfirmed("Laser connect", 14, 4, true)) {
        laserState_ = true;
        if (ui_ != nullptr) {
            ui_->laser_state_display->append("Laser connected.");
        }
    }
}

void PlcService::setTargetTemperature() {
    if (!laserState_) {
        showInfo("Laser is not connected.");
        return;
    }
    if (!ensureConnected()) {
        return;
    }

    // Target temperature is written to DB5.DBD0 as an S7 REAL.
    const double targetTemperature = readDoubleField(ui_ != nullptr ? ui_->target_temperature_input : nullptr, 0.0);
    if (runCommand("Target temperature", client_.writeDbReal32(kDb5, 0, static_cast<float>(targetTemperature)))) {
        targetSet_ = true;
        if (ui_ != nullptr) {
            ui_->laser_state_display->append("Target temperature written.");
        }
    }
}

void PlcService::setLaserPower() {
    if (!laserState_) {
        showInfo("Laser is not connected.");
        return;
    }
    if (temperatureControl_) {
        showInfo("Disable temperature control before setting laser power.");
        return;
    }
    if (!ensureConnected()) {
        return;
    }

    // Laser power is written to DB5.DBD6 as an S7 REAL when temperature closed
    // loop control is not active.
    const double laserPower = readDoubleField(ui_ != nullptr ? ui_->laser_power_input : nullptr, 0.0);
    if (runCommand("Laser power", client_.writeDbReal32(kDb5, 6, static_cast<float>(laserPower)))) {
        powerSet_ = true;
        if (ui_ != nullptr) {
            ui_->laser_state_display->append("Laser power written.");
        }
    }
}

void PlcService::toggleTemperatureControl() {
    if (!laserState_) {
        showInfo("Laser is not connected.");
        return;
    }
    if (!targetSet_) {
        showInfo("Set target temperature before enabling temperature control.");
        return;
    }
    if (!ensureConnected()) {
        return;
    }

    // DB5.DBX4.0 toggles PLC temperature control.
    const bool nextState = !temperatureControl_;
    if (writeDbBitConfirmed("Temperature control", 4, 0, nextState)) {
        temperatureControl_ = nextState;
        if (ui_ != nullptr) {
            ui_->laser_state_display->append(temperatureControl_
                ? "Temperature control enabled."
                : "Temperature control disabled.");
        }
    }
}

void PlcService::togglePowerControl() {
    if (!laserState_) {
        showInfo("Laser is not connected.");
        return;
    }
    if (!powerSet_) {
        showInfo("Set laser power before enabling power control.");
        return;
    }
    if (!ensureConnected()) {
        return;
    }

    // DB5.DBX4.1 toggles PLC laser power control.
    const bool nextState = !powerControl_;
    if (writeDbBitConfirmed("Power control", 4, 1, nextState)) {
        powerControl_ = nextState;
        if (ui_ != nullptr) {
            ui_->laser_state_display->append(powerControl_
                ? "Power control enabled."
                : "Power control disabled.");
        }
    }
}

void PlcService::openRedLight() {
    if (!laserState_) {
        showInfo("Laser is not connected.");
        return;
    }
    if (!ensureConnected()) {
        return;
    }

    // Python opens the red light by confirming DB5.DBX14.3, then DB5.DBX14.0.
    if (!writeDbBitConfirmed("Red light open step 1", 14, 3, true)) {
        return;
    }
    if (writeDbBitConfirmed("Red light open step 2", 14, 0, true)) {
        redLight_ = true;
        if (ui_ != nullptr) {
            ui_->laser_state_display->append("Red light open.");
        }
    }
}

void PlcService::closeRedLight() {
    if (!laserState_) {
        showInfo("Laser is not connected.");
        return;
    }
    if (!ensureConnected()) {
        return;
    }

    // DB5.DBX14.1 is the red-light close command used by Python.
    if (writeDbBitConfirmed("Red light close", 14, 1, true)) {
        redLight_ = false;
        if (ui_ != nullptr) {
            ui_->laser_state_display->append("Red light closed.");
        }
    }
}

void PlcService::laserStart() {
    if (!laserState_) {
        showInfo("Laser is not connected.");
        return;
    }
    if (!targetSet_) {
        showInfo("Set target temperature before starting the laser.");
        return;
    }
    if (!ensureConnected()) {
        return;
    }

    // Python starts the laser by closing the red light first, then setting
    // DB5.DBX14.2.
    if (!writeDbBitConfirmed("Laser start red light close", 14, 1, true)) {
        return;
    }
    if (writeDbBitConfirmed("Laser start", 14, 2, true)) {
        redLight_ = false;
        if (ui_ != nullptr) {
            ui_->laser_state_display->append("Laser started.");
        }
    }
}

void PlcService::laserStop() {
    if (!laserState_) {
        showInfo("Laser is not connected.");
        return;
    }
    if (!ensureConnected()) {
        return;
    }

    // Python stops the laser by raising DB5.DBX14.3, which leaves the red
    // guide light active.
    if (writeDbBitConfirmed("Laser stop", 14, 3, true)) {
        redLight_ = true;
        if (ui_ != nullptr) {
            ui_->laser_state_display->append("Laser stopped.");
        }
    }
}
void PlcService::setHysteresisRatio() {
    if (!ensureConnected()) {
        return;
    }

    const int hysteresisRatio = readIntField(ui_ != nullptr ? ui_->Hysteresis_ratio_text : nullptr, 200);

    // Python writes the ratio to DB5.DBD34 and then raises DB5.DBX40.1 to
    // notify the PLC that the value should be applied.
    if (!runCommand("Hysteresis ratio", client_.writeDbReal32(kDb5, 34, static_cast<float>(hysteresisRatio)))) {
        return;
    }
    runCommand("Hysteresis apply flag", client_.writeDbBit(kDb5, 40, 1, true));
}

bool PlcService::ensureConnected() {
    if (!enabled_) {
        emit messageRequested("PLC", "PLC is disabled in env_config.json (simulation mode). "
                                      "Laser, motor and cylinder controls are unavailable.");
        return false;
    }

    if (client_.isConnected()) {
        return true;
    }
    return runCommand("PLC connect", client_.connect());
}

bool PlcService::runCommand(const QString& action, const PlcResult& result) {
    if (result.ok) {
        return true;
    }

    emit messageRequested("PLC", QString("%1 failed: %2").arg(action, result.message));
    return false;
}

bool PlcService::writeDbBitConfirmed(const QString& action, int byteOffset, int bitIndex, bool value) {
    // The Python code repeatedly wrote until the PLC bit read back true. This
    // bounded version preserves confirmation behavior without risking an
    // infinite UI-thread loop if the PLC or network is unavailable.
    for (int attempt = 0; attempt < kConfirmRetries; ++attempt) {
        if (!runCommand(action, client_.writeDbBit(kDb5, byteOffset, bitIndex, value))) {
            return false;
        }

        bool confirmed = false;
        if (!runCommand(action, client_.readDbBit(kDb5, byteOffset, bitIndex, confirmed))) {
            return false;
        }
        if (confirmed == value) {
            return true;
        }
    }

    emit messageRequested("PLC", QString("%1 was written but did not confirm.").arg(action));
    return false;
}

double PlcService::readDoubleField(const QLineEdit* lineEdit, double fallback) const {
    if (lineEdit == nullptr) {
        return fallback;
    }

    bool ok = false;
    const double value = lineEdit->text().toDouble(&ok);
    return ok ? value : fallback;
}

int PlcService::readIntField(const QLineEdit* lineEdit, int fallback) const {
    if (lineEdit == nullptr) {
        return fallback;
    }

    bool ok = false;
    const int value = lineEdit->text().toInt(&ok);
    return ok ? value : fallback;
}

void PlcService::showInfo(const QString& message) {
    emit messageRequested("PLC", message);
}
