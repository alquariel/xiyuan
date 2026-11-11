#include "ui/PressureChartWindow.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QTimer>
#include <QVBoxLayout>
#include <QtCharts/QAbstractSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <algorithm>
#include <cmath>
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
    pressureView_->setRubberBand(QChartView::NoRubberBand);   // 左键拖动改为平移，不再用橡皮筋缩放
    pressureView_->setFocusPolicy(Qt::StrongFocus);
    pressureView_->viewport()->setCursor(Qt::OpenHandCursor);
    pressureView_->viewport()->installEventFilter(this);
    pressureView_->installEventFilter(this);

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
    zView_->setRubberBand(QChartView::NoRubberBand);
    zView_->setFocusPolicy(Qt::StrongFocus);
    zView_->viewport()->setCursor(Qt::OpenHandCursor);
    zView_->viewport()->installEventFilter(this);
    zView_->installEventFilter(this);

    mainLayout->addWidget(zView_, 1);
}

void PressureChartWindow::appendPressureSample(double targetN,
                                               double measuredN,
                                               double errorN) {
    pressureTargetSeries_->append(pressureIndex_, targetN);
    pressureMeasuredSeries_->append(pressureIndex_, measuredN);
    pressureErrorSeries_->append(pressureIndex_, errorN);

    if (pressureTargetSeries_->count() > kMaxHistory) {
        pressureTargetSeries_->remove(0);
        pressureMeasuredSeries_->remove(0);
        pressureErrorSeries_->remove(0);
    }
    if (pressureFollow_) {
        updateXRange(pressureAxisX_, pressureIndex_);
        // Y 轴量程每 10 个采样重算一次，避免 250Hz 下频繁扫描全部点
        if (pressureIndex_ % 10 == 0) {
            updateYRange(pressureChart_, pressureAxisY_);
        }
    } else if (focusedView_ == pressureView_ && !pressureLocked_) {
        // 放大查看完整曲线时，X 轴保持从 0 到最新点，曲线随数据继续延伸
        pressureAxisX_->setRange(0, std::max(100, pressureIndex_));
    }
    ++pressureIndex_;
}

void PressureChartWindow::appendZSample(double targetZmm, double actualZmm) {
    zTargetSeries_->append(zIndex_, targetZmm);
    zActualSeries_->append(zIndex_, actualZmm);

    if (zTargetSeries_->count() > kMaxHistory) {
        zTargetSeries_->remove(0);
        zActualSeries_->remove(0);
    }
    if (zFollow_) {
        updateXRange(zAxisX_, zIndex_);
        if (zIndex_ % 10 == 0) {
            updateYRange(zChart_, zAxisY_);
        }
    } else if (focusedView_ == zView_ && !zLocked_) {
        zAxisX_->setRange(0, std::max(100, zIndex_));
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

// ======================= 鼠标交互 =======================

QChartView* PressureChartWindow::viewAt(QObject* watched) const {
    if (pressureView_ != nullptr
        && (watched == pressureView_ || watched == pressureView_->viewport())) {
        return pressureView_;
    }
    if (zView_ != nullptr
        && (watched == zView_ || watched == zView_->viewport())) {
        return zView_;
    }
    return nullptr;
}

QValueAxis* PressureChartWindow::axisXOf(const QChartView* view) const {
    return view == pressureView_ ? pressureAxisX_ : zAxisX_;
}

QValueAxis* PressureChartWindow::axisYOf(const QChartView* view) const {
    return view == pressureView_ ? pressureAxisY_ : zAxisY_;
}

QChart* PressureChartWindow::chartOf(const QChartView* view) const {
    return view == pressureView_ ? pressureChart_ : zChart_;
}

int PressureChartWindow::lastIndexOf(const QChartView* view) const {
    const int index = (view == pressureView_) ? pressureIndex_ : zIndex_;
    return std::max(0, index - 1);
}

bool* PressureChartWindow::followOf(const QChartView* view) {
    return view == pressureView_ ? &pressureFollow_ : &zFollow_;
}

bool* PressureChartWindow::lockedOf(const QChartView* view) {
    return view == pressureView_ ? &pressureLocked_ : &zLocked_;
}

void PressureChartWindow::restoreLiveView() {
    focusedView_ = nullptr;
    pressureView_->setVisible(true);
    zView_->setVisible(true);
    pressureFollow_ = true;
    zFollow_ = true;
    pressureLocked_ = false;
    zLocked_ = false;
    if (pressureIndex_ > 0) updateXRange(pressureAxisX_, pressureIndex_ - 1);
    if (zIndex_ > 0) updateXRange(zAxisX_, zIndex_ - 1);
    updateYRange(pressureChart_, pressureAxisY_);
    updateYRange(zChart_, zAxisY_);
}

void PressureChartWindow::toggleFocus(QChartView* view) {
    if (view == nullptr) return;
    if (focusedView_ == view) {   // 再次单击：恢复双图 + 实时滑动
        restoreLiveView();
        return;
    }

    focusedView_ = view;
    const bool isPressure = (view == pressureView_);
    (isPressure ? pressureView_ : zView_)->setVisible(true);
    (isPressure ? zView_ : pressureView_)->setVisible(false);

    *followOf(view) = false;
    *lockedOf(view) = false;

    // 显示完整曲线：X 轴覆盖 0 到最新采样序号，后续新点继续向右延伸
    axisXOf(view)->setRange(0, std::max(100, lastIndexOf(view)));
    updateYRange(chartOf(view), axisYOf(view));
}

void PressureChartWindow::zoomXAxis(QChartView* view, double factor,
                                    double anchorPixelX) {
    if (view == nullptr || factor <= 0.0) return;

    QValueAxis* axisX = axisXOf(view);
    const QRectF plot = chartOf(view)->plotArea();
    if (plot.width() <= 0.0) return;

    // 锚点：鼠标（或捏合中心）所在的采样序号，缩放前后保持不动
    const double span = axisX->max() - axisX->min();
    const double frac = std::clamp(
        (anchorPixelX - plot.left()) / plot.width(), 0.0, 1.0);
    const double anchor = axisX->min() + frac * span;

    const double last = static_cast<double>(lastIndexOf(view));
    const double maxSpan = std::max(100.0, last + 1.0);
    const double newSpan = std::clamp(span / factor, 20.0, maxSpan);

    double newMin = anchor - frac * newSpan;
    newMin = std::clamp(newMin, 0.0, std::max(0.0, last + 1.0 - newSpan));
    axisX->setRange(newMin, newMin + newSpan);

    // 手动缩放后暂停自动跟随，避免新数据把窗口拉回实时位置
    *followOf(view) = false;
    *lockedOf(view) = true;
}

bool PressureChartWindow::eventFilter(QObject* watched, QEvent* event) {
    QChartView* view = viewAt(watched);
    if (view == nullptr) return QWidget::eventFilter(watched, event);

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() != Qt::LeftButton) break;
        view->setFocus(Qt::MouseFocusReason);
        QValueAxis* axisX = axisXOf(view);
        dragView_ = view;
        dragMoved_ = false;
        dragStartX_ = me->pos().x();
        dragStartMin_ = axisX->min();
        dragSpan_ = axisX->max() - axisX->min();
        const double plotWidth = std::max(1.0, chartOf(view)->plotArea().width());
        dragUnitsPerPx_ = dragSpan_ / plotWidth;
        view->viewport()->setCursor(Qt::ClosedHandCursor);
        return true;
    }
    case QEvent::MouseMove: {
        auto* me = static_cast<QMouseEvent*>(event);
        if (dragView_ != view || !(me->buttons() & Qt::LeftButton)) break;
        const int dx = me->pos().x() - dragStartX_;
        if (!dragMoved_ && std::abs(dx) > 3) dragMoved_ = true;
        if (!dragMoved_) return true;
        // 手动拖动后暂停自动跟随并锁定视图，避免新数据把窗口拉回实时位置
        *followOf(view) = false;
        *lockedOf(view) = true;
        const double maxMin =
            std::max(0.0, static_cast<double>(lastIndexOf(view)) - dragSpan_);
        const double newMin =
            std::clamp(dragStartMin_ - dx * dragUnitsPerPx_, 0.0, maxMin);
        axisXOf(view)->setRange(newMin, newMin + dragSpan_);
        return true;
    }
    case QEvent::MouseButtonRelease: {
        auto* me = static_cast<QMouseEvent*>(event);
        if (dragView_ != view || me->button() != Qt::LeftButton) break;
        dragView_ = nullptr;
        view->viewport()->setCursor(Qt::OpenHandCursor);
        if (!dragMoved_) toggleFocus(view);   // 未拖动 = 单击：放大 / 还原
        return true;
    }
    case QEvent::Wheel: {
        auto* we = static_cast<QWheelEvent*>(event);
        const int delta = we->angleDelta().y();
        if (delta == 0) break;
        // 滚轮向上放大、向下缩小，以鼠标位置为锚点
        const double factor = (delta > 0) ? 1.25 : (1.0 / 1.25);
        zoomXAxis(view, factor, we->position().x());
        return true;
    }
    case QEvent::NativeGesture: {
        auto* ge = static_cast<QNativeGestureEvent*>(event);
        if (ge->gestureType() != Qt::ZoomNativeGesture) break;
        // 触摸板双指捏合：value() 为正表示放大、为负表示缩小
        const double factor = std::clamp(1.0 + ge->value(), 0.2, 5.0);
        zoomXAxis(view, factor, ge->position().x());
        return true;
    }
    case QEvent::KeyPress: {
        auto* ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Escape && focusedView_ != nullptr) {
            restoreLiveView();
            return true;
        }
        break;
    }
    default:
        break;
    }
    return QWidget::eventFilter(watched, event);
}

void PressureChartWindow::clearAll() {
    pressureTargetSeries_->clear();
    pressureMeasuredSeries_->clear();
    pressureErrorSeries_->clear();
    zTargetSeries_->clear();
    zActualSeries_->clear();
    pressureIndex_ = 0;
    zIndex_ = 0;
    restoreLiveView();
    pressureAxisX_->setRange(0, 100);
    zAxisX_->setRange(0, 100);
    pressureAxisY_->setRange(0, 120);
    zAxisY_->setRange(-450, -400);
}
