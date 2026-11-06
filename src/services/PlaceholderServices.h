#pragma once

#include <QObject>
#include <QString>

class PlaceholderService : public QObject {
    Q_OBJECT
public:
    explicit PlaceholderService(QString serviceName, QObject* parent = nullptr);

signals:
    void messageRequested(const QString& title, const QString& message);

protected:
    void reportNotConverted(const QString& action);

private:
    QString serviceName_;
};

class SerialService final : public PlaceholderService {
    Q_OBJECT
public:
    explicit SerialService(QObject* parent = nullptr);
public slots:
    void portCheck();
    void portOpen();
    void portClose();
};

class RobotService final : public PlaceholderService {
    Q_OBJECT
public:
    explicit RobotService(QObject* parent = nullptr);
public slots:
    void connectRobot();
    void moveRobot();
    void resetRobot();
    void moveRobotAxes();
    void moveExternalAxis();
    void resetExternalAxis();
    void startPrint();
    void pausePrint();
    void continuePrint();
    void stopPrint();
};
