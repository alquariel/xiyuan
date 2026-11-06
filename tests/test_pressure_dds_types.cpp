#include <QtTest/QtTest>

#include <cstdint>

#include "pressure/PressureDdsTypes.h"

class TestPressureDdsTypes final : public QObject {
    Q_OBJECT

private slots:
    void mapsInternalSampleToDdsSample();
};

void TestPressureDdsTypes::mapsInternalSampleToDdsSample() {
    PressureSampleData sample;
    sample.timestampMs = 123456789ULL;
    sample.sequence = 7ULL;
    sample.rawValue = 10.5;
    sample.meanValue = 9.5;
    sample.varianceValue = 1.25;
    sample.standardDeviationValue = 1.11803398875;
    sample.peakValue = 12.0;
    sample.windowReady = true;
    sample.sensorRunning = true;

    const AfpPressureSample ddsSample = toDdsSample(sample);

    QCOMPARE(ddsSample.timestamp_ms(), static_cast<std::uint64_t>(123456789ULL));
    QCOMPARE(ddsSample.sample_sequence(), static_cast<std::uint64_t>(7ULL));
    QCOMPARE(ddsSample.raw_value(), 10.5);
    QCOMPARE(ddsSample.mean_value(), 9.5);
    QCOMPARE(ddsSample.variance_value(), 1.25);
    QCOMPARE(ddsSample.standard_deviation_value(), 1.11803398875);
    QCOMPARE(ddsSample.peak_value(), 12.0);
    QCOMPARE(ddsSample.window_ready(), true);
    QCOMPARE(ddsSample.sensor_running(), true);
}

QTEST_MAIN(TestPressureDdsTypes)
#include "test_pressure_dds_types.moc"
