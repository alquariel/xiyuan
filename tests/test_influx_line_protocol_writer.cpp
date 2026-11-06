#include <QtTest/QtTest>

#include "archive/InfluxLineProtocolWriter.h"

#include <QElapsedTimer>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>

#include <cstdint>
#include <functional>
#include <limits>
#include <memory>

class FakeInfluxHttpClient : public IInfluxHttpClient {
public:
    bool postLineProtocol(const QString& url,
                          const QString& database,
                          const QString& token,
                          const QString& body,
                          int timeoutMs,
                          QString* error) override {
        urls << url;
        databases << database;
        tokens << token;
        bodies << body;
        timeoutsMs << timeoutMs;

        if (onPost) {
            onPost();
        }

        if (!nextResult) {
            if (error) {
                *error = nextError;
            }
            return false;
        }

        if (error) {
            error->clear();
        }
        return true;
    }

    QStringList urls;
    QStringList databases;
    QStringList tokens;
    QStringList bodies;
    QList<int> timeoutsMs;
    bool nextResult = true;
    QString nextError = "write failed";
    std::function<void()> onPost;
};

class TestInfluxLineProtocolWriter : public QObject {
    Q_OBJECT

private slots:
    void flushesWhenBatchSizeReached();
    void manualFlushWritesPartialBatch();
    void skipsEmptyLineProtocolPoints();
    void preservesPendingRowsWhenWriteFails();
    void passesConfiguredTimeoutToHttpClient();
    void dropsOldestPendingRowWhenBacklogCapIsExceeded();
    void enqueueDuringFlushDoesNotReenterHttpPost();
    void qtHttpClientTimesOutWhenServerDoesNotRespond();
};

namespace {
TimeseriesArchiveConfig makeConfig(int batchSize) {
    TimeseriesArchiveConfig config;
    config.hostUrl = "http://localhost:8181";
    config.database = "afp_process";
    config.writeBatchSize = batchSize;
    return config;
}

TimeSeriesPoint makePoint(const QString& table, double value, std::uint64_t timestampNs) {
    TimeSeriesPoint point;
    point.table = table;
    point.tags.insert("source", "pressure_sensor_1");
    point.floatFields.insert("raw_value", value);
    point.timestampNs = timestampNs;
    return point;
}
}

void TestInfluxLineProtocolWriter::flushesWhenBatchSizeReached() {
    auto fake = std::make_unique<FakeInfluxHttpClient>();
    FakeInfluxHttpClient* fakePtr = fake.get();
    InfluxLineProtocolWriter writer(makeConfig(2), "apiv3_test_token", std::move(fake));

    QVERIFY(writer.enqueue(makePoint("pressure_sensor", 1.25, 10)));
    QVERIFY(fakePtr->bodies.isEmpty());

    QVERIFY(writer.enqueue(makePoint("pressure_sensor", 2.5, 20)));

    QCOMPARE(fakePtr->urls, QStringList({"http://localhost:8181"}));
    QCOMPARE(fakePtr->databases, QStringList({"afp_process"}));
    QCOMPARE(fakePtr->tokens, QStringList({"apiv3_test_token"}));
    QCOMPARE(fakePtr->timeoutsMs, QList<int>({5000}));
    QCOMPARE(
        fakePtr->bodies,
        QStringList({"pressure_sensor,source=pressure_sensor_1 raw_value=1.25 10\n"
                     "pressure_sensor,source=pressure_sensor_1 raw_value=2.5 20"}));
}

void TestInfluxLineProtocolWriter::manualFlushWritesPartialBatch() {
    auto fake = std::make_unique<FakeInfluxHttpClient>();
    FakeInfluxHttpClient* fakePtr = fake.get();
    InfluxLineProtocolWriter writer(makeConfig(10), "apiv3_test_token", std::move(fake));

    QVERIFY(writer.enqueue(makePoint("pressure_sensor", 3.75, 30)));
    QVERIFY(fakePtr->bodies.isEmpty());

    QVERIFY(writer.flush());

    QCOMPARE(fakePtr->bodies, QStringList({"pressure_sensor,source=pressure_sensor_1 raw_value=3.75 30"}));
}

void TestInfluxLineProtocolWriter::skipsEmptyLineProtocolPoints() {
    auto fake = std::make_unique<FakeInfluxHttpClient>();
    FakeInfluxHttpClient* fakePtr = fake.get();
    InfluxLineProtocolWriter writer(makeConfig(1), "apiv3_test_token", std::move(fake));

    TimeSeriesPoint noFields;
    noFields.table = "pressure_sensor";
    QVERIFY(writer.enqueue(noFields));

    TimeSeriesPoint onlyNan;
    onlyNan.table = "pressure_sensor";
    onlyNan.floatFields.insert("raw_value", std::numeric_limits<double>::quiet_NaN());
    QVERIFY(writer.enqueue(onlyNan));

    QVERIFY(writer.flush());
    QVERIFY(fakePtr->bodies.isEmpty());
}

void TestInfluxLineProtocolWriter::preservesPendingRowsWhenWriteFails() {
    auto fake = std::make_unique<FakeInfluxHttpClient>();
    FakeInfluxHttpClient* fakePtr = fake.get();
    fakePtr->nextResult = false;
    fakePtr->nextError = "temporary outage";
    InfluxLineProtocolWriter writer(makeConfig(10), "apiv3_test_token", std::move(fake));

    QVERIFY(writer.enqueue(makePoint("pressure_sensor", 4.5, 40)));

    QVERIFY(!writer.flush());
    QCOMPARE(writer.lastError(), QString("temporary outage"));
    QCOMPARE(fakePtr->bodies, QStringList({"pressure_sensor,source=pressure_sensor_1 raw_value=4.5 40"}));

    fakePtr->nextResult = true;
    QVERIFY(writer.flush());

    QCOMPARE(
        fakePtr->bodies,
        QStringList({"pressure_sensor,source=pressure_sensor_1 raw_value=4.5 40",
                     "pressure_sensor,source=pressure_sensor_1 raw_value=4.5 40"}));
}

void TestInfluxLineProtocolWriter::passesConfiguredTimeoutToHttpClient() {
    TimeseriesArchiveConfig config = makeConfig(1);
    config.httpRequestTimeoutMs = 1234;
    auto fake = std::make_unique<FakeInfluxHttpClient>();
    FakeInfluxHttpClient* fakePtr = fake.get();
    InfluxLineProtocolWriter writer(config, "apiv3_test_token", std::move(fake));

    QVERIFY(writer.enqueue(makePoint("pressure_sensor", 5.5, 50)));

    QCOMPARE(fakePtr->timeoutsMs, QList<int>({1234}));
}

void TestInfluxLineProtocolWriter::dropsOldestPendingRowWhenBacklogCapIsExceeded() {
    TimeseriesArchiveConfig config = makeConfig(10);
    config.maxPendingPoints = 2;
    auto fake = std::make_unique<FakeInfluxHttpClient>();
    FakeInfluxHttpClient* fakePtr = fake.get();
    InfluxLineProtocolWriter writer(config, "apiv3_test_token", std::move(fake));

    QVERIFY(writer.enqueue(makePoint("pressure_sensor", 1.0, 10)));
    QVERIFY(writer.enqueue(makePoint("pressure_sensor", 2.0, 20)));
    QVERIFY(writer.enqueue(makePoint("pressure_sensor", 3.0, 30)));

    QCOMPARE(
        writer.lastError(),
        QString("InfluxDB write backlog exceeded 2 points; dropped oldest pending row."));
    QVERIFY(writer.flush());
    QCOMPARE(
        writer.lastError(),
        QString("InfluxDB write backlog exceeded 2 points; dropped oldest pending row."));
    QCOMPARE(
        fakePtr->bodies,
        QStringList({"pressure_sensor,source=pressure_sensor_1 raw_value=2 20\n"
                     "pressure_sensor,source=pressure_sensor_1 raw_value=3 30"}));
}

void TestInfluxLineProtocolWriter::enqueueDuringFlushDoesNotReenterHttpPost() {
    auto fake = std::make_unique<FakeInfluxHttpClient>();
    FakeInfluxHttpClient* fakePtr = fake.get();
    InfluxLineProtocolWriter writer(makeConfig(1), "apiv3_test_token", std::move(fake));

    bool injectedDuringWrite = false;
    fakePtr->onPost = [&]() {
        if (injectedDuringWrite) {
            return;
        }

        injectedDuringWrite = true;
        QVERIFY(writer.enqueue(makePoint("pressure_sensor", 2.0, 20)));
    };

    QVERIFY(writer.enqueue(makePoint("pressure_sensor", 1.0, 10)));

    // The second point was queued while the first HTTP write was active. It
    // must not cause another post from inside the first post call, otherwise a
    // slow InfluxDB write can recursively nest until the subscriber stack
    // overflows.
    QCOMPARE(fakePtr->bodies.size(), 1);
    QCOMPARE(fakePtr->bodies.first(), QString("pressure_sensor,source=pressure_sensor_1 raw_value=1 10"));

    fakePtr->onPost = {};
    QVERIFY(writer.flush());
    QCOMPARE(fakePtr->bodies.size(), 2);
    QCOMPARE(fakePtr->bodies.last(), QString("pressure_sensor,source=pressure_sensor_1 raw_value=2 20"));
}

void TestInfluxLineProtocolWriter::qtHttpClientTimesOutWhenServerDoesNotRespond() {
    QTcpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    QList<QTcpSocket*> sockets;
    connect(&server, &QTcpServer::newConnection, [&]() {
        while (server.hasPendingConnections()) {
            QTcpSocket* socket = server.nextPendingConnection();
            socket->setParent(&server);
            sockets << socket;
        }
    });

    QtInfluxHttpClient client;
    QString error;
    QElapsedTimer elapsed;
    elapsed.start();

    const bool ok = client.postLineProtocol(
        QString("http://127.0.0.1:%1").arg(server.serverPort()),
        "afp_process",
        "apiv3_test_token",
        "pressure_sensor raw_value=1",
        250,
        &error);

    QVERIFY(!ok);
    QVERIFY2(elapsed.elapsed() < 2000, qPrintable(QString("request took %1 ms").arg(elapsed.elapsed())));
    QVERIFY(error.contains("timed out"));
    QVERIFY(error.contains("250 ms"));
}

QTEST_MAIN(TestInfluxLineProtocolWriter)
#include "test_influx_line_protocol_writer.moc"
