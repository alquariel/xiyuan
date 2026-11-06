#include <QFile>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "archive/InfluxAdmin.h"

class TestInfluxToken : public QObject {
    Q_OBJECT

private slots:
    void readsTokenFromJsonFile();
    void rejectsMissingTokenField();
    void buildsServeArgumentsFromConfig();
    void buildsShowDatabasesArguments();
    void buildsCreateDatabaseArgumentsFromConfig();
    void buildsCreateDatabaseEnvironment();
};

void TestInfluxToken::readsTokenFromJsonFile() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("afp-admin-token.json");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"({"token":"apiv3_test_token","name":"afp-admin"})");
    file.close();

    QString error;
    QCOMPARE(readInfluxAdminToken(path, &error), QString("apiv3_test_token"));
    QVERIFY(error.isEmpty());
}

void TestInfluxToken::rejectsMissingTokenField() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("afp-admin-token.json");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write(R"({"name":"afp-admin"})");
    file.close();

    QString error;
    QVERIFY(readInfluxAdminToken(path, &error).isEmpty());
    QVERIFY(error.contains("token"));
}

void TestInfluxToken::buildsServeArgumentsFromConfig() {
    TimeseriesArchiveConfig config;
    config.nodeId = "afp-node";
    config.objectStore = "file";
    config.dataDir = "D:/InfluxDB 3 Core/data";
    config.adminTokenFile = "D:/InfluxDB 3 Core/afp-admin-token.json";

    QCOMPARE(
        buildInfluxServeArguments(config),
        QStringList({"serve",
                     "--node-id", "afp-node",
                     "--object-store", "file",
                     "--data-dir", "D:/InfluxDB 3 Core/data",
                     "--admin-token-file", "D:/InfluxDB 3 Core/afp-admin-token.json"}));
}

void TestInfluxToken::buildsShowDatabasesArguments() {
    QCOMPARE(
        buildInfluxShowDatabasesArguments(),
        QStringList({"show", "databases", "--format", "json"}));
}

void TestInfluxToken::buildsCreateDatabaseArgumentsFromConfig() {
    TimeseriesArchiveConfig config;
    config.database = "afp_test";
    config.retentionPeriod = "7d";

    QCOMPARE(
        buildInfluxCreateDatabaseArguments(config),
        QStringList({"create", "database", "--retention-period", "7d", "afp_test"}));
}

void TestInfluxToken::buildsCreateDatabaseEnvironment() {
    TimeseriesArchiveConfig config;
    config.hostUrl = "http://localhost:8181";
    const QProcessEnvironment env = buildInfluxCliEnvironment(config, "apiv3_test_token");

    QCOMPARE(env.value("INFLUXDB3_HOST_URL"), QString("http://localhost:8181"));
    QCOMPARE(env.value("INFLUXDB3_AUTH_TOKEN"), QString("apiv3_test_token"));
}

QTEST_MAIN(TestInfluxToken)
#include "test_influx_token.moc"
