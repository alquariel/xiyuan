#pragma once

#include <QObject>
#include <QTcpSocket>

#include "robot_monitor/RobotMonitorSample.h"

// Non-blocking Qt socket client for the RAPID robot monitor stream.
// It reads '#ROBOT_MONITOR ...#' frames and never sends robot motion, print,
// or control commands back to the controller. The only outbound message is a
// lightweight heartbeat so RAPID can discard a stale PC socket after the main
// application is closed and restarted.
class RobotMonitorClient final : public QObject {
    Q_OBJECT

public:
    explicit RobotMonitorClient(QObject* parent = nullptr);

    void connectToRobot(const QString& ip, int port);
    void disconnectFromRobot();
    bool isConnected() const;
    bool sendHeartbeat();

signals:
    void connected();
    void disconnected();
    void rawMessageReceived(const QString& rawMessage);
    void sampleReceived(const RobotMonitorSample& sample);
    void parseErrorReported(const QString& rawMessage, const QString& error);
    void errorReported(const QString& error);

private slots:
    void onReadyRead();

private:
    void processFrame(const QByteArray& frame);

    QTcpSocket socket_;
    QByteArray pendingBytes_;
};
