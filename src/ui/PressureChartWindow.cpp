#include "ui/PressureChartWindow.h"

#include <QPainter>
#include <QVBoxLayout>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

PressureChartWindow::PressureChartWindow(QWidget* parent)
    : QWidget(parent, Qt::Window)  // 独立窗口，不是子控件
{
    setWindowTitle(QStringLiteral("EGM 实时曲线 — 压力 / Z 坐标"));
    resize(900, 640);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    // ============== 压力图 ==============
    pressureTargetSeries_   = new QLineSeries();
    pressureMeasuredSeries_ = new QLineSeries();
    pressureErrorSeries_    = new QLineSeries();

    pressureTargetSeries_->setName(QStringLiteral("目标压力 (N)"));
    pressureMeasuredSeries_->setName(QStringLiteral("实测压力 (N)"));
    pressureErrorSeries_->setName(QStringLiteral("压力误差 (N)"));

    pressureTargetSeries_->setColor(Qt::darkGreen);
    pressureMeasuredSeries_->setColor(Qt::blue);
    pressureErrorSeries_->setColor(QColor(200, 60, 60));

    pressureChart_ = new QChart();
    pressureChart_->addSeries(pressureTargetSeries_);
    pressureChart_->addSeries(pressureMeasuredSeries_);
    pressureChart_->addSeries(pressureErrorSeries_);
    pressureChart_->setTitle(QStringLiteral("压力闭环对比"));
    pressureChart_->legend()->setAlignment(Qt::AlignRight);
    pressureChart_->legend()->setVisible(true);

    auto* pressAxisX = new QValueAxis();
    pressAxisX->setTitleText(QStringLiteral("采样序号"));
    pressAxisX->setLabelFormat("%i");
    pressAxisX->setMinorTickCount(2);

    auto* pressAxisY = new QValueAxis();
    pressAxisY->setTitleText(QStringLiteral("压力 (N)"));
    pressAxisY->setLabelFormat("%.0f");

    pressureChart_->addAxis(pressAxisX, Qt::AlignBottom);
    pressureChart_->addAxis(pressAxisY, Qt::AlignLeft);
    pressureTargetSeries_->attachAxis(pressAxisX);
    pressureTargetSeries_->attachAxis(pressAxisY);
    pressureMeasuredSeries_->attachAxis(pressAxisX);
    pressureMeasuredSeries_->attachAxis(pressAxisY);
    pressureErrorSeries_->attachAxis(pressAxisX);
    pressureErrorSeries_->attachAxis(pressAxisY);

    pressureView_ = new QChartView(pressureChart_, this);
    pressureView_->setRenderHint(QPainter::Antialiasing);
    pressureView_->setMinimumHeight(260);

    mainLayout->addWidget(pressureView_, 1);

    // ============== Z 坐标图 ==============
    zTargetSeries_ = new QLineSeries();
    zActualSeries_ = new QLineSeries();

    zTargetSeries_->setName(QStringLiteral("目标 Z (mm)"));
    zActualSeries_->setName(QStringLiteral("实际反馈 Z (mm)"));

    zTargetSeries_->setColor(Qt::darkGreen);
    zActualSeries_->setColor(Qt::red);

    zChart_ = new QChart();
    zChart_->addSeries(zTargetSeries_);
    zChart_->addSeries(zActualSeries_);
    zChart_->setTitle(QStringLiteral("Z 坐标对比（目标 vs 反馈）"));
    zChart_->legend()->setAlignment(Qt::AlignRight);
    zChart_->legend()->setVisible(true);

    auto* zAxisX = new QValueAxis();
    zAxisX->setTitleText(QStringLiteral("采样序号"));
    zAxisX->setLabelFormat("%i");
    zAxisX->setMinorTickCount(2);

    auto* zAxisY = new QValueAxis();
    zAxisY->setTitleText(QStringLiteral("Z (mm)"));
    zAxisY->setLabelFormat("%.2f");

    zChart_->addAxis(zAxisX, Qt::AlignBottom);
    zChart_->addAxis(zAxisY, Qt::AlignLeft);
    zTargetSeries_->attachAxis(zAxisX);
    zTargetSeries_->attachAxis(zAxisY);
    zActualSeries_->attachAxis(zAxisX);
    zActualSeries_->attachAxis(zAxisY);

    zView_ = new QChartView(zChart_, this);
    zView_->setRenderHint(QPainter::Antialiasing);
    zView_->setMinimumHeight(260);

    mainLayout->addWidget(zView_, 1);

    clearAll();
}

void PressureChartWindow::appendPressureSample(double targetN,
                                               double measuredN,
                                               double errorN) {
    pressureTargetSeries_->append(sampleIndex_, targetN);
    pressureMeasuredSeries_->append(sampleIndex_, measuredN);
    pressureErrorSeries_->append(sampleIndex_, errorN);

    if (pressureTargetSeries_->count() > kMaxPoints) {
        pressureTargetSeries_->remove(0);
        pressureMeasuredSeries_->remove(0);
        pressureErrorSeries_->remove(0);
    }
    ++sampleIndex_;
}

void PressureChartWindow::appendZSample(double targetZmm, double actualZmm) {
    zTargetSeries_->append(sampleIndex_, targetZmm);
    zActualSeries_->append(sampleIndex_, actualZmm);

    if (zTargetSeries_->count() > kMaxPoints) {
        zTargetSeries_->remove(0);
        zActualSeries_->remove(0);
    }
}

void PressureChartWindow::clearAll() {
    pressureTargetSeries_->clear();
    pressureMeasuredSeries_->clear();
    pressureErrorSeries_->clear();
    zTargetSeries_->clear();
    zActualSeries_->clear();
    sampleIndex_ = 0;
}
