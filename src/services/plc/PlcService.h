#pragma once

#include <QObject>
#include <QString>

#include "app/AppConfig.h"
#include "plc/PlcClient.h"

class QLineEdit;
class QTimer;

QT_BEGIN_NAMESPACE
namespace Ui { class ContinuosfiberInterface; }
QT_END_NAMESPACE

// Qt-facing PLC service for the direct controls on the original UI.
//
// This class translates button clicks and line-edit values into the PLC memory
// writes used by the Python application. It owns process/UI state such as
// "motor enabled" and "target temperature set", while PlcClient owns only the
// Snap7 network handle.
class PlcService final : public QObject {
    Q_OBJECT

public:
    PlcService(const PlcConfig& config, Ui::ContinuosfiberInterface* ui, QObject* parent = nullptr);

    // Called by MainWindow once setupUi() has run: wires the connection-status
    // indicator and starts the link health timer.
    void initialize();

signals:
    // MainWindow displays these messages using the existing message-box helper.
    void messageRequested(const QString& title, const QString& message);

public slots:
    // Two-state connect/disconnect button: also the master switch that enables
    // or blocks the lazy connect performed by ensureConnected().
    void toggleConnection();
    void connectPlc();
    void disconnectPlc();
    void cylinder1();
    void cylinder2();
    void cylinder3();
    void motorEnable();
    void motorReset();
    void motorActuation();
    void parameterInput();
    void setMotorJogMode();
    void setMotorContinuousMode();
    void laserConnect();
    void setTargetTemperature();
    void setLaserPower();
    void toggleTemperatureControl();
    void togglePowerControl();
    void openRedLight();
    void closeRedLight();
    void laserStart();
    void laserStop();
    void setHysteresisRatio();

private:
    bool ensureConnected();
    bool runCommand(const QString& action, const PlcResult& result);
    bool writeDbBitConfirmed(const QString& action, int byteOffset, int bitIndex, bool value);
    double readDoubleField(const QLineEdit* lineEdit, double fallback) const;
    int readIntField(const QLineEdit* lineEdit, int fallback) const;
    void showInfo(const QString& message);

    // Refreshes the two-state button text and the connection-status label.
    void updateConnectionUi();
    // Health timer: detects a PLC power cycle and reconnects automatically.
    void onHealthTick();
    void attemptReconnect();

    Ui::ContinuosfiberInterface* ui_ = nullptr;
    PlcClient client_;

    // Mirrors PlcConfig::enabled. When false no Snap7 connection is attempted.
    bool enabled_ = true;

    // Master switch. true keeps the lazy connect behavior of ensureConnected();
    // an explicit disconnect turns it off until the operator presses 连接PLC.
    bool masterOn_ = true;
    // true once a connection has been observed, so a sudden loss can trigger a
    // reconnect without connecting eagerly at startup.
    bool linkWasUp_ = false;
    // true while the health timer is trying to restore a lost link.
    bool reconnectPending_ = false;
    QTimer* healthTimer_ = nullptr;

    bool motorEnabled_ = false;
    bool redLight_ = false;
    bool laserState_ = false;
    bool targetSet_ = false;
    bool temperatureControl_ = false;
    bool powerSet_ = false;
    bool powerControl_ = false;
};
