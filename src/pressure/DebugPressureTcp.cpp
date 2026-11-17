#include "pressure/DebugPressureTcp.h"

#include <QTcpServer>
#include <QTcpSocket>

#include <QDebug>

DebugPressureTcp::DebugPressureTcp(quint16 port, QObject* parent)
    : QObject(parent), port_(port) {
    qInfo() << "[DebugPressureTcp] ctor port=" << port;
}

DebugPressureTcp::~DebugPressureTcp() {
    qInfo() << "[DebugPressureTcp] dtor";
    stop();
}

bool DebugPressureTcp::start() {
    if (server_ && server_->isListening()) {
        return true;
    }

    server_ = std::make_unique<QTcpServer>(this);

    connect(server_.get(), &QTcpServer::newConnection,
            this, &DebugPressureTcp::onNewConnection);

    if (!server_->listen(QHostAddress::AnyIPv4, port_)) {
        qWarning() << "[DebugPressureTcp] listen FAILED:" << server_->errorString();
        server_.reset();
        emit listeningStateChanged(false);
        return false;
    }

    qInfo() << "[DebugPressureTcp] listen SUCCESS on port" << server_->serverPort();
    emit listeningStateChanged(true);
    return true;
}

void DebugPressureTcp::stop() {
    if (!server_) return;

    for (QTcpSocket* client : clients_) {
        client->disconnectFromHost();
    }
    clients_.clear();
    server_->close();
    server_.reset();
    emit listeningStateChanged(false);
}

quint16 DebugPressureTcp::boundPort() const {
    return server_ ? server_->serverPort() : 0;
}

bool DebugPressureTcp::isListening() const {
    return server_ && server_->isListening();
}

void DebugPressureTcp::onNewConnection() {
    while (server_->hasPendingConnections()) {
        QTcpSocket* client = server_->nextPendingConnection();
        clients_.append(client);
        connect(client, &QTcpSocket::readyRead,
                this, &DebugPressureTcp::onReadyRead);
        connect(client, &QTcpSocket::disconnected,
                this, &DebugPressureTcp::onPeerDisconnected);
        qInfo() << "[DebugPressureTcp] client connected from"
                << client->peerAddress().toString() << ":" << client->peerPort();
    }
}

void DebugPressureTcp::onReadyRead() {
    auto* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    // 手机 TCP 调试助手有两种发"换行"的方式：
    //   1) 真正的 ASCII 0x0A —— Qt 的 canReadLine() / readLine() 能直接切
    //   2) 字面字符串 "\n"（反斜杠 + n 两个可见字符）—— Qt 不认为是换行
    // 这里两层都处理，确保两种格式都能正确按行分割。

    auto dispatchChunk = [this](const QString& chunk) {
        if (chunk.isEmpty()) return;
        if (chunk.contains(QStringLiteral("\\n"))) {
            // 这一块里包含字面 "\n"，说明手机一次发了多行，按字面切
            const QStringList parts = chunk.split(QStringLiteral("\\n"), Qt::SkipEmptyParts);
            for (const QString& p : parts) {
                parseAndEmitLine(p);
            }
        } else {
            parseAndEmitLine(chunk);
        }
    };

    // 第一层：Qt readLine 按真换行切
    while (client->canReadLine()) {
        const QString line = QString::fromUtf8(client->readLine()).trimmed();
        dispatchChunk(line);
    }

    // 兜底：如果没有真换行但还有数据（手机全程只发字面 "\n"），
    // 把所有剩余数据读出来再按字面切
    if (client->bytesAvailable() > 0) {
        const QString leftover = QString::fromUtf8(client->readAll()).trimmed();
        dispatchChunk(leftover);
    }
}

void DebugPressureTcp::onPeerDisconnected() {
    auto* client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;
    clients_.removeOne(client);
    client->deleteLater();
}

void DebugPressureTcp::parseAndEmitLine(const QString& rawLine) {
    if (rawLine.isEmpty()) return;

    // 尾部可能残留字面 "\n"、"\r"、真换行等，统一 strip
    QString line = rawLine;
    while (line.endsWith(QStringLiteral("\\n"))) line.chop(2);
    while (line.endsWith(QStringLiteral("\\r"))) line.chop(2);
    line = line.trimmed();
    if (line.isEmpty()) return;

    // 支持格式：
    //   P,80.0     带前缀
    //   80.0       纯数值
    double value = 0.0;
    bool ok = false;

    if (line.startsWith(QStringLiteral("P,"), Qt::CaseInsensitive)) {
        value = line.mid(2).trimmed().toDouble(&ok);
    } else {
        value = line.toDouble(&ok);
    }

    if (!ok) {
        qInfo() << "[DebugPressureTcp] ignored line:" << rawLine;
        return;
    }

    qInfo() << "[DebugPressureTcp] -> pressureSampleReady(" << value << ")";
    emit pressureSampleReady(value);
}
