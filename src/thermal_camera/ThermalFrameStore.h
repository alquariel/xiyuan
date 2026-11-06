#pragma once

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QList>
#include <QString>

// Owns the thermal camera image folders under the application root.
//
// The Python application wrote transient thermal JPEGs under logs/pic/cache and
// the ROI crop to logs/pic/roi/uarter.png. Keeping those paths stable lets the
// C++ slice interoperate with existing deployment scripts and operator habits.
class ThermalFrameStore final {
public:
    ThermalFrameStore(QDir appRootDir, int cacheMaxFiles);

    QDir cacheDir() const;
    QDir savedDir() const;
    QDir roiDir() const;
    QString roiFilePath() const;

    bool ensureDirectories() const;
    bool clearCache() const;
    void pruneCache() const;
    QFileInfo newestReadableJpeg(QImage* image = nullptr) const;
    bool saveRoiImage(const QImage& image) const;

private:
    QList<QFileInfo> cacheJpegsNewestFirst() const;

    QDir appRootDir_;
    int cacheMaxFiles_ = 60;
};
