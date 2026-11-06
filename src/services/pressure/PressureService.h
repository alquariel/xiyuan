#pragma once

#include <QObject>
#include <QThread>

#include <memory>

#include "app/AppConfig.h"
#include "app/AppPaths.h"

namespace Ui { class ContinuosfiberInterface; }
class PressureAcquisitionWorker;

// Qt-facing service for the pressure controls already present in the original
// UI. It keeps acquisition off the UI thread and reports status through the
// same message signal used by the converted PLC service.
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

    PressureConfig config_;
    AppPaths paths_;
    Ui::ContinuosfiberInterface* ui_ = nullptr;
    std::unique_ptr<QThread> workerThread_;
    PressureAcquisitionWorker* worker_ = nullptr;
    bool running_ = false;
};
