#pragma once

#include <QDir>
#include <QFileInfo>
#include <QString>

class AppPaths {
public:
    explicit AppPaths(QString applicationDirPath);

    QDir rootDir() const;
    QDir configDir() const;
    QDir assetsDir() const;
    QFileInfo configFilePath() const;
    QFileInfo loggingConfigFilePath() const;
    QFileInfo iconFilePath() const;

private:
    QDir rootDir_;
};
