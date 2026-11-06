#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QTemporaryDir>
#include <QThread>

#include "thermal_camera/ThermalFrameStore.h"

class TestThermalFrameStore final : public QObject {
    Q_OBJECT

private slots:
    void createsExpectedDirectories();
    void clearCacheKeepsCacheDirectory();
    void newestReadableJpegSkipsBrokenNewestFile();
    void pruneCacheKeepsNewestFiles();
    void saveRoiImageWritesExpectedFile();
};

namespace {

void waitForDistinctFileTime() {
    // Windows file timestamps can collapse when files are created too quickly.
    // Give each cache image a distinct mtime so newest-first pruning is stable.
    QThread::msleep(1200);
}

void writeTextFile(const QString& path, const QByteArray& contents) {
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write(contents), contents.size());
}

void writeJpeg(const QString& path, const QSize& size, const QColor& color = Qt::red) {
    QImage image(size, QImage::Format_RGB32);
    image.fill(color);
    QVERIFY2(image.save(path, "JPG"), qPrintable(QString("Could not save test JPEG %1").arg(path)));
}

}

void TestThermalFrameStore::createsExpectedDirectories() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    ThermalFrameStore store(QDir(tempDir.path()), 60);

    QVERIFY(store.ensureDirectories());
    QVERIFY(QDir(tempDir.path()).exists("logs/pic/cache"));
    QVERIFY(QDir(tempDir.path()).exists("logs/pic/saved"));
    QVERIFY(QDir(tempDir.path()).exists("logs/pic/roi"));
}

void TestThermalFrameStore::clearCacheKeepsCacheDirectory() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalFrameStore store(QDir(tempDir.path()), 60);
    QVERIFY(store.ensureDirectories());

    writeJpeg(store.cacheDir().filePath("first.jpg"), QSize(8, 8));
    writeJpeg(store.cacheDir().filePath("second.jpeg"), QSize(8, 8));
    writeTextFile(store.cacheDir().filePath("keep.txt"), "not a jpeg");

    QVERIFY(store.clearCache());
    QVERIFY(store.cacheDir().exists());
    QVERIFY(!QFileInfo::exists(store.cacheDir().filePath("first.jpg")));
    QVERIFY(!QFileInfo::exists(store.cacheDir().filePath("second.jpeg")));
    QVERIFY(QFileInfo::exists(store.cacheDir().filePath("keep.txt")));
}

void TestThermalFrameStore::newestReadableJpegSkipsBrokenNewestFile() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalFrameStore store(QDir(tempDir.path()), 60);
    QVERIFY(store.ensureDirectories());

    writeJpeg(store.cacheDir().filePath("older.jpg"), QSize(13, 17), Qt::green);
    waitForDistinctFileTime();
    writeTextFile(store.cacheDir().filePath("newer.jpg"), "broken jpg bytes");

    QImage image;
    const QFileInfo newestReadable = store.newestReadableJpeg(&image);

    QVERIFY(newestReadable.exists());
    QCOMPARE(newestReadable.fileName(), QString("older.jpg"));
    QCOMPARE(image.size(), QSize(13, 17));
}

void TestThermalFrameStore::pruneCacheKeepsNewestFiles() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalFrameStore store(QDir(tempDir.path()), 2);
    QVERIFY(store.ensureDirectories());

    writeJpeg(store.cacheDir().filePath("one.jpg"), QSize(8, 8));
    waitForDistinctFileTime();
    writeJpeg(store.cacheDir().filePath("two.jpg"), QSize(8, 8));
    waitForDistinctFileTime();
    writeJpeg(store.cacheDir().filePath("three.jpg"), QSize(8, 8));
    waitForDistinctFileTime();
    writeJpeg(store.cacheDir().filePath("four.jpg"), QSize(8, 8));

    store.pruneCache();

    QVERIFY(!QFileInfo::exists(store.cacheDir().filePath("one.jpg")));
    QVERIFY(!QFileInfo::exists(store.cacheDir().filePath("two.jpg")));
    QVERIFY(QFileInfo::exists(store.cacheDir().filePath("three.jpg")));
    QVERIFY(QFileInfo::exists(store.cacheDir().filePath("four.jpg")));
}

void TestThermalFrameStore::saveRoiImageWritesExpectedFile() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    ThermalFrameStore store(QDir(tempDir.path()), 60);
    QImage roi(QSize(21, 9), QImage::Format_RGB32);
    roi.fill(Qt::blue);

    QVERIFY(store.saveRoiImage(roi));

    const QString expectedPath = QDir(tempDir.path()).filePath("logs/pic/roi/uarter.png");
    QCOMPARE(store.roiFilePath(), expectedPath);
    QImage loaded(expectedPath);
    QVERIFY(!loaded.isNull());
    QCOMPARE(loaded.size(), QSize(21, 9));
}

QTEST_GUILESS_MAIN(TestThermalFrameStore)
#include "test_thermal_frame_store.moc"
