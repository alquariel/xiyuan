#include "ui/PressureChartWindow.h"

#include <QPainter>
#include <QTimer>
#include <QVBoxLayout>
#include <QtCharts/QAbstractSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <algorithm>
#include <limits>

PressureChartWindow::PressureChartWindow(QWidget* parent)
    : QWidget(nullptr, Qt::Window)   // 不传 parent，独立顶级窗口；parent 参数忽略
{
    Q_UNUSED(parent);
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
    pressureChart_->setTitle(QStringLiteral("压力闭环对比"));
    pressureChart_->legend()->setAlignment(Qt::AlignRight);
    pressureChart_->legend()->setVisible(true);
    pressureChart_->setAnimationOptions(QChart::NoAnimation);
    pressureChart_->setDropShadowEnabled(false);

    auto* pressAxisX = new QValueAxis();
    pressAxisX->setTitleText(QStringLiteral("采样序号"));
    pressAxisX->setLabelFormat("%i");
    pressAxisX->setMinorTickCount(2);
    pressAxisX->setRange(0, 100);
    pressureAxisX_ = pressAxisX;

    auto* pressAxisY = new QValueAxis();
    pressAxisY->setTitleText(QStringLiteral("压力 (N)"));
    pressAxisY->setLabelFormat("%.0f");
    pressAxisY->setRange(0, 120);
    pressureAxisY_ = pressAxisY;
    pressureChart_->addAxis(pressAxisX, Qt::AlignBottom);
    pressureChart_->addAxis(pressAxisY, Qt::AlignLeft);
    pressureChart_->addSeries(pressureTargetSeries_);
    pressureChart_->addSeries(pressureMeasuredSeries_);
    pressureChart_->addSeries(pressureErrorSeries_);
    pressureTargetSeries_->attachAxis(pressAxisX);
    pressureTargetSeries_->attachAxis(pressAxisY);
    pressureMeasuredSeries_->attachAxis(pressAxisX);
    pressureMeasuredSeries_->attachAxis(pressAxisY);
    pressureErrorSeries_->attachAxis(pressAxisX);
    pressureErrorSeries_->attachAxis(pressAxisY);

    pressureView_ = new QChartView(pressureChart_, this);
    pressureView_->setRenderHint(QPainter::Antialiasing);
    pressureView_->setMinimumHeight(260);
    pressureView_->setRubberBand(QChartView::HorizontalRubberBand);

    mainLayout->addWidget(pressureView_, 1);

    // ============== Z 坐标图 ==============
    zTargetSeries_ = new QLineSeries();
    zActualSeries_ = new QLineSeries();

    zTargetSeries_->setName(QStringLiteral("目标 Z (mm)"));
    zActualSeries_->setName(QStringLiteral("实际反馈 Z (mm)"));

    zTargetSeries_->setColor(Qt::darkGreen);
    zActualSeries_->setColor(Qt::red);

    zChart_ = new QChart();
    zChart_->setTitle(QStringLiteral("Z 坐标对比（目标 vs 反馈）"));
    zChart_->legend()->setAlignment(Qt::AlignRight);
    zChart_->legend()->setVisible(true);
    zChart_->setAnimationOptions(QChart::NoAnimation);
    zChart_->setDropShadowEnabled(false);

    auto* zAxisX = new QValueAxis();
    zAxisX->setTitleText(QStringLiteral("采样序号"));
    zAxisX->setLabelFormat("%i");
    zAxisX->setMinorTickCount(2);
    zAxisX->setRange(0, 100);
    zAxisX_ = zAxisX;

    auto* zAxisY = new QValueAxis();
    zAxisY->setTitleText(QStringLiteral("Z (mm)"));
    zAxisY->setLabelFormat("%.2f");
    zAxisY->setRange(-450, -400);
    zAxisY_ = zAxisY;

    zChart_->addAxis(zAxisX, Qt::AlignBottom);
    zChart_->addAxis(zAxisY, Qt::AlignLeft);
    zChart_->addSeries(zTargetSeries_);
    zChart_->addSeries(zActualSeries_);
    zTargetSeries_->attachAxis(zAxisX);
    zTargetSeries_->attachAxis(zAxisY);
    zActualSeries_->attachAxis(zAxisX);
    zActualSeries_->attachAxis(zAxisY);

    zView_ = new QChartView(zChart_, this);
    zView_->setRenderHint(QPainter::Antialiasing);
    zView_->setMinimumHeight(260);
    zView_->setRubberBand(QChartView::HorizontalRubberBand);

    mainLayout->addWidget(zView_, 1);
}

void PressureChartWindow::appendPressureSample(double targetN,
                                               double measuredN,
                                               double errorN) {
    pressureTargetSeries_->append(pressureIndex_, targetN);
    pressureMeasuredSeries_->append(pressureIndex_, measuredN);
    pressureErrorSeries_->append(pressureIndex_, errorN);

    if (pressureTargetSeries_->count() > kMaxPoints) {
        pressureTargetSeries_->remove(0);
        pressureMeasuredSeries_->remove(0);
        pressureErrorSeries_->remove(0);
    }
    updateXRange(pressureAxisX_, pressureIndex_);
    // Y 轴量程每 10 个采样重算一次，避免 250Hz 下频繁扫描全部点
    if (pressureIndex_ % 10 == 0) {
        updateYRange(pressureChart_, pressureAxisY_);
    }
    ++pressureIndex_;
}

void PressureChartWindow::appendZSample(double targetZmm, double actualZmm) {
    zTargetSeries_->append(zIndex_, targetZmm);
    zActualSeries_->append(zIndex_, actualZmm);

    if (zTargetSeries_->count() > kMaxPoints) {
        zTargetSeries_->remove(0);
        zActualSeries_->remove(0);
    }
    updateXRange(zAxisX_, zIndex_);
    if (zIndex_ % 10 == 0) {
        updateYRange(zChart_, zAxisY_);
    }
    ++zIndex_;
}

void PressureChartWindow::updateXRange(QValueAxis* axis, int lastIndex) {
    // 滑动窗口只保留最近 kMaxPoints 个点，X 轴必须跟着序号一起右移。
    // 之前把 X 固定成 0..100，序号一超界新点就画在可视区外，旧点又被
    // remove(0) 逐个删掉，看起来就是曲线"先出现、再逐渐消失"。
    const int xMax = std::max(100, lastIndex + 1);
    const int xMin = std::max(0, xMax - kMaxPoints);
    axis->setRange(xMin, xMax);
}

void PressureChartWindow::updateYRange(QChart* chart, QValueAxis* axis) {
    double lo = std::numeric_limits<double>::max();
    double hi = std::numeric_limits<double>::lowest();
    for (QAbstractSeries* series : chart->series()) {
        auto* line = qobject_cast<QLineSeries*>(series);
        if (line == nullptr) continue;
        const auto points = line->points();
        for (const QPointF& point : points) {
            lo = std::min(lo, point.y());
            hi = std::max(hi, point.y());
        }
    }
    if (lo > hi) return;                 // 还没有数据
    if (hi - lo < 1.0) {                 // 数据近似常量时给一个最小可视跨度
        const double mid = (lo + hi) * 0.5;
        lo = mid - 0.5;
        hi = mid + 0.5;
    }
    const double pad = (hi - lo) * 0.05;
    axis->setRange(lo - pad, hi + pad);
}

void PressureChartWindow::clearAll() {
    pressureTargetSeries_->clear();
    pressureMeasuredSeries_->clear();
    pressureErrorSeries_->clear();
    zTargetSeries_->clear();
    zActualSeries_->clear();
    pressureIndex_ = 0;
    zIndex_ = 0;
    pressureAxisX_->setRange(0, 100);
    zAxisX_->setRange(0, 100);
    pressureAxisY_->setRange(0, 120);
    zAxisY_->setRange(-450, -400);
}
