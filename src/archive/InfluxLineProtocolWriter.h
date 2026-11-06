#pragma once

#include "app/AppConfig.h"
#include "archive/TimeSeriesPoint.h"

#include <QString>
#include <QStringList>

#include <memory>

// Injectable transport boundary for writing InfluxDB line protocol. Tests use
// a fake client so writer buffering can be verified without opening sockets.
class IInfluxHttpClient {
public:
    virtual ~IInfluxHttpClient() = default;

    virtual bool postLineProtocol(const QString& url,
                                  const QString& database,
                                  const QString& token,
                                  const QString& body,
                                  int timeoutMs,
                                  QString* error) = 0;
};

// Qt-backed HTTP client for InfluxDB 3's line-protocol write endpoint.
class QtInfluxHttpClient : public IInfluxHttpClient {
public:
    bool postLineProtocol(const QString& url,
                          const QString& database,
                          const QString& token,
                          const QString& body,
                          int timeoutMs,
                          QString* error) override;
};

// Buffers writable time-series points and posts them as newline-separated
// InfluxDB line-protocol rows once a batch is full or flush() is called.
class InfluxLineProtocolWriter {
public:
    InfluxLineProtocolWriter(TimeseriesArchiveConfig config,
                             QString token,
                             std::unique_ptr<IInfluxHttpClient> httpClient);

    bool enqueue(const TimeSeriesPoint& point);
    bool flush();

    QString lastError() const;

private:
    TimeseriesArchiveConfig config_;
    QString token_;
    std::unique_ptr<IInfluxHttpClient> httpClient_;
    QStringList pendingRows_;
    QString lastError_;

    // Prevents recursive flush() calls while the synchronous Qt HTTP client is
    // running its nested event loop. DDS samples can still be queued during the
    // write; they will be sent by the next flush instead of growing the stack.
    bool flushing_ = false;
};
