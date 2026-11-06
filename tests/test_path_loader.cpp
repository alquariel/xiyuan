#include <QtTest/QtTest>

#include "robot/PathLoader.h"

#include <QTemporaryFile>

class TestPathLoader final : public QObject {
    Q_OBJECT

private slots:
    void loadTxtIgnoresCommentsAndBlankLines();
    void loadTxtRequiresSevenOrMoreColumns();
    void buildSegmentsWithCorrectLengthAndDirection();
    void positionInterpolatedLinearly();
    void positionClampedBeyondEndpoints();
};

void TestPathLoader::loadTxtIgnoresCommentsAndBlankLines() {
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write("# comment header\n\n");
    file.write("10.0 20.0 30.0 0 0 1 0 0 0 0\n");
    file.write("# another comment\n");
    file.write("15.0 25.0 35.0 0 0 1 0 0 0 0\n");
    file.close();

    PathLoader loader;
    QVERIFY(loader.loadFromTxt(file.fileName()));
    QCOMPARE(loader.rawPoints().size(), 2);
    QCOMPARE(loader.segmentCount(), 1);
}

void TestPathLoader::loadTxtRequiresSevenOrMoreColumns() {
    QTemporaryFile file;
    QVERIFY(file.open());
    // 10-col → accept
    file.write("10.0 20.0 30.0 0 0 1 5.5 8 0 0\n");
    // 6-col → skip (only x..k, no speed)
    file.write("11.0 21.0 31.0 0 0 1\n");
    // 7-col → accept (matches path_control/path_points.txt layout)
    file.write("12.0 22.0 32.0 0 0 1 7.0\n");
    // 8-col → accept (matches path_control/genPath_out_angle.txt layout)
    file.write("14.0 24.0 34.0 0 0 1 9.5 12.3\n");
    file.close();

    PathLoader loader;
    QVERIFY(loader.loadFromTxt(file.fileName()));
    QCOMPARE(loader.rawPoints().size(), 3);
    QCOMPARE(loader.segmentCount(), 2);

    // angle column should be 0 for the 7-col row, present for the 8-col row
    QCOMPARE(loader.rawPoints()[1].angle, 0.0f);
    QVERIFY(qFuzzyCompare(loader.rawPoints()[2].angle, 12.3f));
}

void TestPathLoader::buildSegmentsWithCorrectLengthAndDirection() {
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write("0 0 0 0 0 1 0 0 0 0\n");
    file.write("30 40 0 0 0 1 0 0 0 0\n");
    file.close();

    PathLoader loader;
    QVERIFY(loader.loadFromTxt(file.fileName()));
    const auto& seg = loader.segments().first();
    QVERIFY(qFuzzyCompare(seg.lengthMm, 50.0f));
    QVERIFY(qFuzzyCompare(seg.directionMmPerMm.x, 0.6f));
    QVERIFY(qFuzzyCompare(seg.directionMmPerMm.y, 0.8f));
    QVERIFY(qFuzzyCompare(seg.directionMmPerMm.z, 0.0f));
}

void TestPathLoader::positionInterpolatedLinearly() {
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write("0 0 0 0 0 1 0 0 0 0\n");
    file.write("10 0 0 0 0 1 0 0 0 0\n");
    file.close();

    PathLoader loader;
    QVERIFY(loader.loadFromTxt(file.fileName()));
    const PathVector3 mid = loader.positionAt(0, 5.0f);
    QVERIFY(qFuzzyCompare(mid.x, 5.0f));
    QVERIFY(qFuzzyCompare(mid.y, 0.0f));
    QVERIFY(qFuzzyCompare(mid.z, 0.0f));
}

void TestPathLoader::positionClampedBeyondEndpoints() {
    QTemporaryFile file;
    QVERIFY(file.open());
    file.write("0 0 0 0 0 1 0 0 0 0\n");
    file.write("10 0 0 0 0 1 0 0 0 0\n");
    file.close();

    PathLoader loader;
    QVERIFY(loader.loadFromTxt(file.fileName()));
    const PathVector3 before = loader.positionAt(0, -50.0f);
    QVERIFY(qFuzzyCompare(before.x, 0.0f));
    const PathVector3 after = loader.positionAt(0, 100.0f);
    QVERIFY(qFuzzyCompare(after.x, 10.0f));
}

QTEST_MAIN(TestPathLoader)
#include "test_path_loader.moc"
