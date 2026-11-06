#pragma once

#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

#include "app/AppConfig.h"

QString readInfluxAdminToken(const QString& tokenFilePath, QString* error);
QStringList buildInfluxServeArguments(const TimeseriesArchiveConfig& config);
QStringList buildInfluxShowDatabasesArguments();
QStringList buildInfluxCreateDatabaseArguments(const TimeseriesArchiveConfig& config);
QProcessEnvironment buildInfluxCliEnvironment(const TimeseriesArchiveConfig& config, const QString& token);
