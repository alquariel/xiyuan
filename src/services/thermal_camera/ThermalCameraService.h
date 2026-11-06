#pragma once

#include "thermal_camera/IThermalCameraClient.h"
#include "thermal_camera/ThermalFrameStore.h"

#include <QObject>
#include <QPointF>
#include <QImage>
#include <QRect>
#include <QSize>
#include <QTimer>
#include <QVector>

#include <memory>

class QLabel;

QT_BEGIN_NAMESPACE
namespace Ui { class ContinuosfiberInterface; }
QT_END_NAMESPACE

// The service owns the existing TIC_* workflow and keeps SDK access behind
// IThermalCameraClient while preserving the JPEG cache behavior in
// ThermalFrameStore.
class ThermalCameraService final : public QObject {
    Q_OBJECT

public:
    ThermalCameraService(Ui::ContinuosfiberInterface* ui,
                         ThermalFrameStore frameStore,
                         std::unique_ptr<IThermalCameraClient> client,
                         int displayIntervalMs = 1000,
                         QObject* parent = nullptr);
    ~ThermalCameraService() override;

    bool handleMousePress(Qt::MouseButton button, const QPointF& localPos);

public slots:
    void startCamera();
    void stopCamera();
    void clearCache();
    void showData();
    void refreshDisplayOnceForTest();

signals:
    void messageRequested(const QString& title, const QString& message);

private:
    void refreshDisplay();
    QRect calculateSquareCrop(const QSize& size) const;
    QRect calculateRoiCrop(const QSize& size) const;
    void setLabelImage(QLabel* label, const QImage& image);
    void showMessage(const QString& message);

    Ui::ContinuosfiberInterface* ui_ = nullptr;
    ThermalFrameStore frameStore_;
    std::unique_ptr<IThermalCameraClient> client_;
    QTimer displayTimer_;
    int displayIntervalMs_ = 1000;
    bool roiTracking_ = false;
    bool roiSet_ = false;
    QVector<QPointF> roiPoints_;
};
