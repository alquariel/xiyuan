#include "archive/InfluxLineProtocolWriter.h"

#include <QByteArray>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>

#include <algorithm>
#include <memory>
#include <utility>

namespace {
QUrl buildWriteUrl(const QString& hostUrl, const QString& database) {
    QUrl writeUrl(hostUrl);
    QString path = writeUrl.path();
    if (path.endsWith('/')) {
        path.chop(1);
    }
    writeUrl.setPath(path + "/api/v3/write_lp");

    QUrlQuery query;
    query.addQueryItem("db", database);
    writeUrl.setQuery(query);
    return writeUrl;
}

bool isSuccessfulHttpStatus(const QVariant& statusCode) {
    if (!statusCode.isValid()) {
        return true;
    }

    const int code = statusCode.toInt();
    return code >= 200 && code < 300;
}
}

bool QtInfluxHttpClient::postLineProtocol(const QString& url,
                                          const QString& database,
                                          const QString& token,
                                          const QString& body,
                                          int timeoutMs,
                                          QString* error) {
    QNetworkAccessManager manager;
    QNetworkRequest request(buildWriteUrl(url, database));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain; charset=utf-8");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());

    QEventLoop loop;
    bool timedOut = false;
    QNetworkReply* reply = manager.post(request, body.toUtf8());
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    // The subscriber can tolerate this synchronous call, but it must be bounded
    // so a dead InfluxDB endpoint cannot block the process indefinitely.
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    QObject::connect(&timeoutTimer, &QTimer::timeout, [&]() {
        timedOut = true;
        reply->abort();
    });
    timeoutTimer.start(std::max(1, timeoutMs));
    loop.exec();
    timeoutTimer.stop();

    const QByteArray responseBody = reply->readAll();
    const QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    const bool networkOk = reply->error() == QNetworkReply::NoError;
    const QString networkError = reply->errorString();
    reply->deleteLater();

    if (timedOut) {
        if (error) {
            *error = QString("InfluxDB write timed out after %1 ms.").arg(std::max(1, timeoutMs));
        }
        return false;
    }

    if (networkOk && isSuccessfulHttpStatus(statusCode)) {
        if (error) {
            error->clear();
        }
        return true;
    }

    if (error) {
        const QString responseText = QString::fromUtf8(responseBody).trimmed();
        if (!responseText.isEmpty()) {
            *error = responseText;
        } else if (statusCode.isValid()) {
            *error = QString("InfluxDB write failed with HTTP status %1").arg(statusCode.toInt());
        } else {
            *error = networkError;
        }
    }
    return false;
}

InfluxLineProtocolWriter::InfluxLineProtocolWriter(TimeseriesArchiveConfig config,
                                                   QString token,
                                                   std::unique_ptr<IInfluxHttpClient> httpClient)
    : config_(std::move(config)),
      token_(std::move(token)),
      httpClient_(std::move(httpClient)) {
    if (!httpClient_) {
        httpClient_ = std::make_unique<QtInfluxHttpClient>();
    }
}

bool InfluxLineProtocolWriter::enqueue(const TimeSeriesPoint& point) {
    const QString row = point.toLineProtocol();
    if (row.isEmpty()) {
        return true;
    }

    pendingRows_ << row;
    const int maxPendingPoints = std::max(1, config_.maxPendingPoints);
    if (pendingRows_.size() > maxPendingPoints) {
        pendingRows_.removeFirst();
        lastError_ = QString("InfluxDB write backlog exceeded %1 points; dropped oldest pending row.")
                         .arg(maxPendingPoints);
    }

    const int batchSize = std::max(1, config_.writeBatchSize);
    if (!flushing_ && pendingRows_.size() >= batchSize) {
        return flush();
    }

    return true;
}

bool InfluxLineProtocolWriter::flush() {
    if (pendingRows_.isEmpty() || flushing_) {
        return true;
    }

    // Move the current batch out before entering the HTTP client's nested event
    // loop. New DDS samples may still arrive and append to pendingRows_; keeping
    // them separate avoids recursive flushes and preserves them for the next
    // write cycle.
    const QStringList rowsToSend = std::move(pendingRows_);
    pendingRows_.clear();

    QString error;
    flushing_ = true;
    if (!httpClient_->postLineProtocol(
            config_.hostUrl,
            config_.database,
            token_,
            rowsToSend.join('\n'),
            config_.httpRequestTimeoutMs,
            &error)) {
        pendingRows_ = rowsToSend + pendingRows_;
        flushing_ = false;
        lastError_ = error.isEmpty() ? QString("InfluxDB write failed") : error;
        return false;
    }
    flushing_ = false;

    if (!lastError_.contains("dropped oldest pending row")) {
        lastError_.clear();
    }
    return true;
}

QString InfluxLineProtocolWriter::lastError() const {
    return lastError_;
}
