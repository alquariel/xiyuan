#pragma once

#include <QImage>
#include <QString>
#include <QtGlobal>

enum class HighSpeedCameraTransport {
    GigE,
    Usb,
    Unknown,
};

// Lightweight description of a discovered camera, matching the Python UI label format.
struct HighSpeedCameraDevice {
    int index = -1;
    HighSpeedCameraTransport transport = HighSpeedCameraTransport::Unknown;
    QString userName;
    QString endpoint;

    QString displayText() const {
        const QString transportName = transport == HighSpeedCameraTransport::GigE
            ? "GigE"
            : (transport == HighSpeedCameraTransport::Usb ? "USB" : "Unknown");
        return QString("[%1]%2: %3(%4)").arg(index).arg(transportName, userName, endpoint);
    }
};

// Camera settings shared by UI/services and concrete SDK-backed clients.
struct HighSpeedCameraParameters {
    double frameRate = 0.0;
    double exposureTime = 0.0;
    double gain = 0.0;
};

// Frame container kept separate so later SDK clients can add metadata without changing callers.
struct HighSpeedCameraFrame {
    QImage image;
};

inline QString highSpeedCameraErrorHex(quint32 code) {
    const QString digits = QString("%1").arg(code, 8, 16, QLatin1Char('0')).toUpper();
    return QString("0x%1").arg(digits);
}

inline QString highSpeedCameraErrorHex(int code) {
    return highSpeedCameraErrorHex(static_cast<quint32>(code));
}
