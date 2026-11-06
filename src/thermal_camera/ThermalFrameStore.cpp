#include "thermal_camera/ThermalFrameStore.h"

#include <QFile>
#include <QFileInfoList>

#include <algorithm>
#include <utility>

namespace {
constexpr int kDefaultCacheMaxFiles = 60;
constexpr int kNewestReadableCandidates = 5;
}

ThermalFrameStore::ThermalFrameStore(QDir appRootDir, int cacheMaxFiles)
    : appRootDir_(std::move(appRootDir)),
      cacheMaxFiles_(cacheMaxFiles > 0 ? cacheMaxFiles : kDefaultCacheMaxFiles) {}

QDir ThermalFrameStore::cacheDir() const {
    return QDir(appRootDir_.filePath("logs/pic/cache"));
}

QDir ThermalFrameStore::savedDir() const {
    return QDir(appRootDir_.filePath("logs/pic/saved"));
}

QDir ThermalFrameStore::roiDir() const {
    return QDir(appRootDir_.filePath("logs/pic/roi"));
}

QString ThermalFrameStore::roiFilePath() const {
    // Preserve the Python filename exactly: downstream scripts and operators
    // already expect the ROI crop at logs/pic/roi/uarter.png.
    return roiDir().filePath("uarter.png");
}

bool ThermalFrameStore::ensureDirectories() const {
    QDir root(appRootDir_);
    return root.mkpath("logs/pic/cache")
        && root.mkpath("logs/pic/saved")
        && root.mkpath("logs/pic/roi");
}

bool ThermalFrameStore::clearCache() const {
    if (!ensureDirectories()) {
        return false;
    }

    bool removedAll = true;
    const QFileInfoList files = cacheDir().entryInfoList(
        {"*.jpg", "*.jpeg", "*.JPG", "*.JPEG"}, QDir::Files | QDir::NoSymLinks);
    for (const QFileInfo& file : files) {
        removedAll = QFile::remove(file.absoluteFilePath()) && removedAll;
    }
    return removedAll && cacheDir().exists();
}

void ThermalFrameStore::pruneCache() const {
    const QList<QFileInfo> files = cacheJpegsNewestFirst();
    for (int index = cacheMaxFiles_; index < files.size(); ++index) {
        QFile::remove(files.at(index).absoluteFilePath());
    }
}

QFileInfo ThermalFrameStore::newestReadableJpeg(QImage* image) const {
    const QList<QFileInfo> files = cacheJpegsNewestFirst();
    const int candidateCount = std::min(kNewestReadableCandidates, static_cast<int>(files.size()));

    for (int index = 0; index < candidateCount; ++index) {
        QImage candidate(files.at(index).absoluteFilePath());
        if (!candidate.isNull()) {
            if (image) {
                *image = candidate;
            }
            return files.at(index);
        }
    }

    if (image) {
        *image = QImage();
    }
    return QFileInfo();
}

bool ThermalFrameStore::saveRoiImage(const QImage& image) const {
    if (image.isNull() || !ensureDirectories()) {
        return false;
    }
    return image.save(roiFilePath(), "PNG");
}

QList<QFileInfo> ThermalFrameStore::cacheJpegsNewestFirst() const {
    // The thermal camera bridge writes a rolling JPEG cache into the same
    // logs/pic/cache layout used by the Python UI. Sorting here centralizes the
    // newest-first interpretation for preview recovery and retention pruning.
    QFileInfoList files = cacheDir().entryInfoList(
        {"*.jpg", "*.jpeg", "*.JPG", "*.JPEG"}, QDir::Files | QDir::NoSymLinks);

    std::sort(files.begin(), files.end(), [](const QFileInfo& left, const QFileInfo& right) {
        if (left.lastModified() == right.lastModified()) {
            return left.fileName() > right.fileName();
        }
        return left.lastModified() > right.lastModified();
    });

    return files;
}
