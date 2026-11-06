#include <QtTest/QtTest>

#include "pressure/PressureStatistics.h"

class TestPressureStatistics final : public QObject {
    Q_OBJECT

private slots:
    void windowIsNotReadyBeforeConfiguredSampleCount();
    void computesMeanVarianceStddevAndPeak();
    void keepsOnlyNewestSamples();
};

void TestPressureStatistics::windowIsNotReadyBeforeConfiguredSampleCount() {
    PressureStatistics stats(4);

    stats.addSample(1.0);
    stats.addSample(2.0);
    stats.addSample(3.0);

    QVERIFY(!stats.windowReady());
    QCOMPARE(stats.sampleCount(), 3);
    QCOMPARE(stats.mean(), 0.0);
    QCOMPARE(stats.variance(), 0.0);
    QCOMPARE(stats.standardDeviation(), 0.0);
    QCOMPARE(stats.peak(), 0.0);
}

void TestPressureStatistics::computesMeanVarianceStddevAndPeak() {
    PressureStatistics stats(4);

    stats.addSample(1.0);
    stats.addSample(2.0);
    stats.addSample(3.0);
    stats.addSample(4.0);

    QVERIFY(stats.windowReady());
    QCOMPARE(stats.sampleCount(), 4);
    QCOMPARE(stats.mean(), 2.5);
    QCOMPARE(stats.variance(), 1.25);
    QCOMPARE(stats.standardDeviation(), std::sqrt(1.25));
    QCOMPARE(stats.peak(), 4.0);
}

void TestPressureStatistics::keepsOnlyNewestSamples() {
    PressureStatistics stats(4);

    stats.addSample(1.0);
    stats.addSample(2.0);
    stats.addSample(3.0);
    stats.addSample(4.0);
    stats.addSample(100.0);

    QVERIFY(stats.windowReady());
    QCOMPARE(stats.sampleCount(), 4);
    QCOMPARE(stats.mean(), 27.25);
    QCOMPARE(stats.peak(), 100.0);
}

QTEST_MAIN(TestPressureStatistics)
#include "test_pressure_statistics.moc"
