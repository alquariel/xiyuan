#pragma once

#include <QObject>
#include <QThread>

#include <memory>

#include "app/AppConfig.h"
#include "app/AppPaths.h"

namespace Ui { class ContinuosfiberInterface; }
class PressureAcquisitionWorker;
class DebugPressureTcp;

// Qt-facing service for the pressure controls already present in the original
// UI. It keeps acquisition off the UI thread and reports status through the
// same message signal used by the converted PLC service.
//
// 压力输入源可通过 PressureConfig::source 切换：
//   - RealDll  ：厂商 pressure.dll（真实传感器）
//   - DebugTcp ：独立 QTcpServer 调试端口，方便没有硬件时验证
// 两种源对外统一发出 pressureSampleReady(double)，下游无需改动。
class PressureService final : public QObject {
    Q_OBJECT

public:
    PressureService(const AppConfig& config,
                    const AppPaths& paths,
                    Ui::ContinuosfiberInterface* ui,
                    QObject* parent = nullptr);
    ~PressureService() override;

signals:
    void messageRequested(const QString& title, const QString& message);
    void pressureSampleReady(double measuredN);

public slots:
    void initializeMachine();
    void startCollection();
    void stopCollection();
    void showData();

private slots:
    void handleStarted();
    void handleStopped();
    void handleError(const QString& message);

private:
    QString pressureDllPath() const;
    void cleanupWorkerThread();
    void cleanupDebugTcp();
    void handleDebugTcpMessage(const QString& message);

    PressureConfig config_;
    AppPaths paths_;
    Ui::ContinuosfiberInterface* ui_ = nullptr;

    // RealDll 源：采集线程 + 工作对象
    std::unique_ptr<QThread> workerThread_;
    PressureAcquisitionWorker* worker_ = nullptr;

    // DebugTcp 源：独立 QTcpServer
    std::unique_ptr<DebugPressureTcp> debugTcp_;

    bool running_ = false;
};
