#include "app/StatusLogger.h"

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QTextStream>

namespace {
QFile logFile;

void messageHandler(QtMsgType type, const QMessageLogContext&, const QString& message) {
    const QString level = [type]() {
        switch (type) {
        case QtDebugMsg: return QString("DEBUG");
        case QtInfoMsg: return QString("INFO");
        case QtWarningMsg: return QString("WARNING");
        case QtCriticalMsg: return QString("ERROR");
        case QtFatalMsg: return QString("FATAL");
        }
        return QString("INFO");
    }();

    const QString line = QString("%1 %2 %3\n")
        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
        .arg(level)
        .arg(message);

    if (logFile.isOpen()) {
        QTextStream stream(&logFile);
        stream << line;
        stream.flush();
    }

    QTextStream(stderr) << line;
}
}

void StatusLogger::install(const char* fileName) {
    if (logFile.isOpen()) {
        logFile.close();
    }
    logFile.setFileName(QString::fromUtf8(fileName != nullptr ? fileName : "afp_abb_cpp.log"));
    logFile.open(QIODevice::Append | QIODevice::Text);
    qInstallMessageHandler(messageHandler);
}
