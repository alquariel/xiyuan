#pragma once

#include <QObject>

#include <memory>

class QTcpServer;
class QTcpSocket;

// 调试用压力数据源：在本机监听一个 TCP 端口，客户端（例如手机上的
// TCP 调试助手）按行发送文本压力值，解析后像真实传感器一样发出
// pressureSampleReady(double)。该类与厂商 DLL 完全解耦，
// 专门用于没有真实传感器时验证 Qt 压力曲线与 EGM 闭环链路。
//
// 协议（纯 ASCII，\n 分隔）：
//   P,80.0\n
//   P,85.0\n
//   P,75.0\n
// 只有以 "P," 开头的行才会被解析为压力值，其他内容会被忽略。
class DebugPressureTcp final : public QObject {
    Q_OBJECT

public:
    explicit DebugPressureTcp(quint16 port, QObject* parent = nullptr);
    ~DebugPressureTcp() override;

    bool start();
    void stop();

    quint16 boundPort() const;
    bool isListening() const;

signals:
    void pressureSampleReady(double measuredN);
    void listeningStateChanged(bool listening);
    void logMessage(const QString& message);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onPeerDisconnected();

private:
    void parseAndEmitLine(const QString& line);

    quint16 port_;
    std::unique_ptr<QTcpServer> server_;
    QList<QTcpSocket*> clients_;
};
