#include <QtTest/QtTest>

#include "archive/PressureSampleReader.h"
#include "archive/RobotMonitorSampleReader.h"
#include "archive/TimeSeriesPoint.h"

class TestTimeSeriesPoint : public QObject {
    Q_OBJECT

private slots:
    void formatsPressurePoint();
    void omitsZeroTimestamp();
    void escapesNamesTagsAndStrings();
    void omitsNonFiniteFloatFields();
    void returnsEmptyStringWhenNoFieldsAreWritable();
    void normalizesNewlinesBeforeEscaping();
    void convertsPressureSampleToPoint();
    void clampsPressureSequenceToSignedInfluxRange();
    void omitsOverflowingPressureTimestamp();
    void convertsRobotMonitorSampleToPoint();
};

void TestTimeSeriesPoint::formatsPressurePoint() {
    TimeSeriesPoint point;
    point.table = "pressure_sensor";
    point.tags.insert("source", "pressure_sensor_1");
    point.floatFields.insert("raw_value", 12.3);
    point.integerFields.insert("sequence", 42);
    point.boolFields.insert("window_ready", true);
    point.timestampNs = 1710000000000000000ULL;

    QCOMPARE(
        point.toLineProtocol(),
        QString("pressure_sensor,source=pressure_sensor_1 raw_value=12.3,sequence=42i,window_ready=true 1710000000000000000"));
}

void TestTimeSeriesPoint::omitsZeroTimestamp() {
    TimeSeriesPoint point;
    point.table = "pressure_sensor";
    point.floatFields.insert("raw_value", 1);
    point.timestampNs = 0;

    QCOMPARE(
        point.toLineProtocol(),
        QString("pressure_sensor raw_value=1"));
}

void TestTimeSeriesPoint::escapesNamesTagsAndStrings() {
    TimeSeriesPoint point;
    point.table = "pressure sensor";
    point.tags.insert("source name", "sensor,1");
    point.stringFields.insert("status text", "ready \"now\"");
    point.timestampNs = 10;

    QCOMPARE(
        point.toLineProtocol(),
        QString("pressure\\ sensor,source\\ name=sensor\\,1 status\\ text=\"ready \\\"now\\\"\" 10"));
}

void TestTimeSeriesPoint::omitsNonFiniteFloatFields() {
    TimeSeriesPoint point;
    point.table = "pressure_sensor";
    point.floatFields.insert("raw_value", 1.25);
    point.floatFields.insert("bad_nan", std::numeric_limits<double>::quiet_NaN());
    point.floatFields.insert("bad_inf", std::numeric_limits<double>::infinity());
    point.floatFields.insert("bad_neg_inf", -std::numeric_limits<double>::infinity());

    QCOMPARE(
        point.toLineProtocol(),
        QString("pressure_sensor raw_value=1.25"));
}

void TestTimeSeriesPoint::returnsEmptyStringWhenNoFieldsAreWritable() {
    TimeSeriesPoint point;
    point.table = "pressure_sensor";
    point.tags.insert("source", "pressure_sensor_1");
    point.floatFields.insert("bad_nan", std::numeric_limits<double>::quiet_NaN());

    QVERIFY(point.toLineProtocol().isEmpty());
}

void TestTimeSeriesPoint::normalizesNewlinesBeforeEscaping() {
    TimeSeriesPoint point;
    point.table = "pressure\nsensor";
    point.tags.insert("source\rname", "sensor\n1");
    point.stringFields.insert("status\ntext", "ready\rnow");

    const QString line = point.toLineProtocol();

    QVERIFY(!line.contains('\n'));
    QVERIFY(!line.contains('\r'));
    QCOMPARE(
        line,
        QString("pressure\\ sensor,source\\ name=sensor\\ 1 status\\ text=\"ready now\""));
}

void TestTimeSeriesPoint::convertsPressureSampleToPoint() {
    PressureArchiveConfig config;
    config.table = "pressure_sensor";
    config.sourceTag = "pressure_sensor_1";

    AfpPressureSample sample;
    sample.timestamp_ms(1234);
    sample.sample_sequence(7);
    sample.raw_value(1.5);
    sample.mean_value(1.2);
    sample.variance_value(0.4);
    sample.standard_deviation_value(0.632);
    sample.peak_value(2.0);
    sample.window_ready(true);
    sample.sensor_running(true);

    const TimeSeriesPoint point = pressureSampleToPoint(sample, config);

    QCOMPARE(point.table, QString("pressure_sensor"));
    QCOMPARE(point.tags.value("source"), QString("pressure_sensor_1"));
    QCOMPARE(point.floatFields.value("raw_value"), 1.5);
    QCOMPARE(point.floatFields.value("mean_value"), 1.2);
    QCOMPARE(point.floatFields.value("variance_value"), 0.4);
    QCOMPARE(point.floatFields.value("standard_deviation_value"), 0.632);
    QCOMPARE(point.floatFields.value("peak_value"), 2.0);
    QCOMPARE(point.integerFields.value("sequence"), 7);
    QVERIFY(point.boolFields.value("window_ready"));
    QVERIFY(point.boolFields.value("sensor_running"));
    QCOMPARE(point.timestampNs, 1234000000ULL);
}

void TestTimeSeriesPoint::clampsPressureSequenceToSignedInfluxRange() {
    PressureArchiveConfig config;
    AfpPressureSample sample;
    sample.sample_sequence(std::numeric_limits<std::uint64_t>::max());

    const TimeSeriesPoint point = pressureSampleToPoint(sample, config);

    QCOMPARE(point.integerFields.value("sequence"), std::numeric_limits<qint64>::max());
}

void TestTimeSeriesPoint::omitsOverflowingPressureTimestamp() {
    PressureArchiveConfig config;
    AfpPressureSample sample;
    sample.timestamp_ms(std::numeric_limits<std::uint64_t>::max());

    const TimeSeriesPoint point = pressureSampleToPoint(sample, config);

    QCOMPARE(point.timestampNs, 0ULL);
}

void TestTimeSeriesPoint::convertsRobotMonitorSampleToPoint() {
    RobotMonitorArchiveConfig config;
    config.table = "robot_monitor";
    config.sourceTag = "abb_irb4600";

    AfpRobotMonitorSample sample;
    sample.timestamp_ms(1234);
    sample.sample_sequence(7);
    sample.x(10.5);
    sample.y(20.5);
    sample.z(30.5);
    sample.q1(1.0);
    sample.q2(0.1);
    sample.q3(0.2);
    sample.q4(0.3);
    sample.j1(1);
    sample.j6(6);
    sample.eax1(11);
    sample.eax6(16);
    sample.commanded_tcp_speed(100.0);
    sample.estimated_tcp_speed(12.25);

    const TimeSeriesPoint point = robotMonitorSampleToPoint(sample, config);

    QCOMPARE(point.table, QString("robot_monitor"));
    QCOMPARE(point.tags.value("source"), QString("abb_irb4600"));
    QCOMPARE(point.integerFields.value("sequence"), 7);
    QCOMPARE(point.floatFields.value("x"), 10.5);
    QCOMPARE(point.floatFields.value("z"), 30.5);
    QCOMPARE(point.floatFields.value("q2"), 0.1);
    QCOMPARE(point.floatFields.value("j1"), 1.0);
    QCOMPARE(point.floatFields.value("j6"), 6.0);
    QCOMPARE(point.floatFields.value("eax1"), 11.0);
    QCOMPARE(point.floatFields.value("eax6"), 16.0);
    QCOMPARE(point.floatFields.value("commanded_tcp_speed"), 100.0);
    QCOMPARE(point.floatFields.value("estimated_tcp_speed"), 12.25);
    QCOMPARE(point.timestampNs, 1234000000ULL);
}

QTEST_MAIN(TestTimeSeriesPoint)
#include "test_time_series_point.moc"
