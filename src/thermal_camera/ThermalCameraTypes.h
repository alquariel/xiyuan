#pragma once

#include <QString>

inline QString thermalCameraErrorText(unsigned long code) {
    // Hikvision docs often list SDK errors as decimal values, while Windows
    // debugging commonly surfaces the same code in hexadecimal.
    const QString hexDigits = QString("%1").arg(code, 8, 16, QLatin1Char('0')).toUpper();
    return QString("%1 (0x%2)").arg(code).arg(hexDigits);
}
