#include "app/AppPaths.h"

AppPaths::AppPaths(QString applicationDirPath)
    : rootDir_(QDir(applicationDirPath)) {
}

QDir AppPaths::rootDir() const {
    return rootDir_;
}

QDir AppPaths::configDir() const {
    return QDir(rootDir_.absoluteFilePath("config"));
}

QDir AppPaths::assetsDir() const {
    return QDir(rootDir_.absoluteFilePath("assets"));
}

QFileInfo AppPaths::configFilePath() const {
    return QFileInfo(configDir().absoluteFilePath("env_config.json"));
}

QFileInfo AppPaths::loggingConfigFilePath() const {
    return QFileInfo(configDir().absoluteFilePath("logging_config.json"));
}

QFileInfo AppPaths::iconFilePath() const {
    return QFileInfo(assetsDir().absoluteFilePath("icons/icon.png"));
}
