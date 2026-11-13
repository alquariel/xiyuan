#include "ui/PressureChartWindow.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <QtCharts/QAbstractSeries>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <algorithm>
#include <cmath>
#include <limits>

// ==================== SplineOverlay ====================
// 透明覆盖 widget，挂在 QChartView::viewport() 上，
// 用 QPainterPath::cubicTo（贝塞尔曲线）**直接连接已有点**，
// 不插任何点、不改变 series 数据，只改变连接方式。
//
// 控制点公式（Uniform Catmull-Rom → Bezier 转换，张力 1/6）：
//   C1 = P1 + (P2 - P0) / 6
//   C2 = P2 - (P3 - P1) / 6
// 段 [P1, P2] 的贝塞尔四元组：P1, C1, C2, P2

class SplineOverlay : public QWidget {
public:
    SplineOverlay(QChartView* chartView, QLineSeries* series,
                  const QColor& color, qreal penWidth)
        : QWidget(chartView->viewport())
        , chartView_(chartView)
        , series_(series)
        , color_(color)
        , penWidth_(penWidth) {
        setAttribute(Qt::WA_TransparentForMouseEvents);  // 鼠标穿透到底层 chartView
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_OpaquePaintEvent, false);
        setStyleSheet("background: transparent;");
    }

protected:
    void paintEvent(QPaintEvent*) override {
        if (!chartView_ || !series_ || series_->count() < 2) return;

        auto* chart = chartView_->chart();
        if (!chart) return;

        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // ---- 关键 1：clip 到 chart plot area（把曲线限制在坐标轴以内，出框问题根因之一）----
        const QRectF plotAreaScene = chart->plotArea();
        const QRectF plotAreaWidget = QRectF(
            chartView_->mapFromScene(plotAreaScene.topLeft()),
            chartView_->mapFromScene(plotAreaScene.bottomRight()));
        p.setClipRect(plotAreaWidget);

        QPen pen(color_);
        pen.setWidthF(penWidth_);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        p.setPen(pen);

        const auto dataPts = series_->points();

        // ---- 关键 2：chart->mapToPosition 返回 scene 坐标，
        // SplineOverlay 是 viewport 的子 widget，paint 用 widget 坐标，必须做 scene→widget 转换 ----
        auto toWidget = [&](const QPointF& dataPt) -> QPointF {
            return chartView_->mapFromScene(chart->mapToPosition(dataPt));
        };

        QPainterPath path;
        path.moveTo(toWidget(dataPts[0]));

        const int n = dataPts.size();
        for (int i = 0; i < n - 1; ++i) {
            QPointF P0 = (i >= 1) ? dataPts[i - 1] : dataPts[0];
            QPointF P1 = dataPts[i];
            QPointF P2 = dataPts[i + 1];
            QPointF P3 = (i + 2 < n) ? dataPts[i + 2] : dataPts[n - 1];

            QPointF p1 = toWidget(P1);
            QPointF p2 = toWidget(P2);
            QPointF cp0 = toWidget(P0);
            QPointF cp3 = toWidget(P3);

            // Uniform Catmull-Rom → Bezier 控制点（张力 1/6）
            QPointF c1 = p1 + (p2 - cp0) / 6.0;
            QPointF c2 = p2 - (cp3 - p1) / 6.0;

            path.cubicTo(c1, c2, p2);
        }
        p.drawPath(path);
    }

private:
    QChartView*  chartView_;
    QLineSeries* series_;
    QColor       color_;
    qreal        penWidth_;
};

// ==================== PressureChartWindow ====================

PressureChartWindow::PressureChartWindow(QWidget* parent)
    : QWidget(nullptr, Qt::Window)
{
    Q_UNUSED(parent);
    setWindowTitle(QStringLiteral("EGM 实时曲线 — 压力 / Z 坐标"));
    resize(900, 640);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);

    // ============== 压力图：折线渲染 ==============
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
    pressAxisX->setTitleText(QStringLiteral("时间 (s)"));
    pressAxisX->setLabelFormat("%.1f");
    pressAxisX->setMinorTickCount(2);
    pressAxisX->setRange(0, 5);
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
    pressureView_->setRubberBand(QChartView::NoRubberBand);
    pressureView_->setFocusPolicy(Qt::StrongFocus);
    pressureView_->viewport()->setCursor(Qt::OpenHandCursor);
    pressureView_->viewport()->installEventFilter(this);
    pressureView_->installEventFilter(this);

    mainLayout->addWidget(pressureView_, 1);

    // ============== Z 坐标图：已有点 → 贝塞尔曲线覆盖层 ==============
    zTargetSeries_ = new QLineSeries();
    zActualSeries_ = new QLineSeries();

    zTargetSeries_->setName(QStringLiteral("目标 Z (mm)"));
    zActualSeries_->setName(QStringLiteral("实际反馈 Z (mm)"));

    zTargetSeries_->setColor(Qt::darkGreen);
    zActualSeries_->setColor(Qt::red);  // 覆盖层也用红色

    zChart_ = new QChart();
    zChart_->setTitle(QStringLiteral("Z 坐标对比（目标 vs 反馈）"));
    zChart_->legend()->setAlignment(Qt::AlignRight);
    zChart_->legend()->setVisible(true);
    zChart_->setAnimationOptions(QChart::NoAnimation);
    zChart_->setDropShadowEnabled(false);

    auto* zAxisX = new QValueAxis();
    zAxisX->setTitleText(QStringLiteral("时间 (s)"));
    zAxisX->setLabelFormat("%.1f");
    zAxisX->setMinorTickCount(2);
    zAxisX->setRange(0, 5);
    zAxisX_ = zAxisX;

    auto* zAxisY = new QValueAxis();
    zAxisY->setTitleText(QStringLiteral("Z (mm)"));
    zAxisY->setLabelFormat("%.2f");
    zAxisY->setRange(-450, -400);
    zAxisY_ = zAxisY;

    zChart_->addAxis(zAxisX, Qt::AlignBottom);
    zChart_->addAxis(zAxisY, Qt::AlignLeft);
    zChart_->addSeries(zTargetSeries_);
    zChart_->addSeries(zActualSeries_);    // 加进 chart 才能共享轴 + 出现在 legend
    zTargetSeries_->attachAxis(zAxisX);
    zTargetSeries_->attachAxis(zAxisY);
    zActualSeries_->attachAxis(zAxisX);
    zActualSeries_->attachAxis(zAxisY);

    // 关键：让 chart 不画 zActualSeries_ 的折线（用覆盖层画贝塞尔曲线），
    // 但保留它在 legend 里的显示。setVisible(false) 会同时从 legend 隐藏，
    // 所以我们需要别的方式。
    //
    // 方案：让 zActualSeries_ 保持 visible，但 setColor 为透明，
    // 折线变成"透明的线"（实际上不渲染），legend 仍显示原颜色。
    // 覆盖层在它上面画真实的红色贝塞尔曲线。
    zActualSeries_->setColor(Qt::transparent);

    zView_ = new QChartView(zChart_, this);
    zView_->setRenderHint(QPainter::Antialiasing);
    zView_->setMinimumHeight(260);
    zView_->setRubberBand(QChartView::NoRubberBand);
    zView_->setFocusPolicy(Qt::StrongFocus);
    zView_->viewport()->setCursor(Qt::OpenHandCursor);
    zView_->viewport()->installEventFilter(this);
    zView_->installEventFilter(this);

    // 透明覆盖层：挂在 viewport 上，画 zActualSeries_ 的贝塞尔曲线
    zSplineOverlay_ = new SplineOverlay(zView_, zActualSeries_, Qt::red, 2.0);
    zSplineOverlay_->setGeometry(zView_->viewport()->rect());
    zSplineOverlay_->raise();  // 确保在最上层

    mainLayout->addWidget(zView_, 1);
}

double PressureChartWindow::startTimeSec() const {
    if (!startTime_.isValid()) return 0.0;
    return startTime_.elapsed() / 1000.0;
}

void PressureChartWindow::appendPressureSample(double targetN,
                                               double measuredN,
                                               double errorN) {
    if (!startTime_.isValid()) startTime_.start();
    const double t = startTimeSec();

    pressureTargetSeries_->append(t, targetN);
    pressureMeasuredSeries_->append(t, measuredN);
    pressureErrorSeries_->append(t, errorN);

    if (pressureTargetSeries_->count() > kMaxHistory) {
        pressureTargetSeries_->remove(0);
        pressureMeasuredSeries_->remove(0);
        pressureErrorSeries_->remove(0);
    }
    if (pressureFollow_) {
        updateXRange(pressureAxisX_, t);
        if (pressureIndex_ % 10 == 0) {
            updateYRange(pressureChart_, pressureAxisY_);
        }
    } else if (focusedView_ == pressureView_ && !pressureLocked_) {
        pressureAxisX_->setRange(0.0, std::max(5.0, t));
    }
    ++pressureIndex_;
}

void PressureChartWindow::appendZSample(double targetZmm, double actualZmm) {
    if (!startTime_.isValid()) startTime_.start();
    const double t = startTimeSec();

    zTargetSeries_->append(t, targetZmm);

    // Z 降采样：每 kZDecimFactor 次才 append 到 actual 系列
    // （250Hz / 5 = 50Hz 画图，肉眼完全够，渲染负担砍 80%）
    const bool appendActual = (zIndex_ % kZDecimFactor == 0);
    if (appendActual) {
        zActualSeries_->append(t, actualZmm);
    }

    if (zTargetSeries_->count() > kMaxHistory) {
        zTargetSeries_->remove(0);
    }
    if (zActualSeries_->count() > kMaxHistory) {
        zActualSeries_->remove(0);
    }

    if (zSplineOverlay_) zSplineOverlay_->update();

    if (zFollow_) {
        updateXRange(zAxisX_, t);
        if (zIndex_ % 10 == 0) {
            updateYRange(zChart_, zAxisY_);
        }
    } else if (focusedView_ == zView_ && !zLocked_) {
        zAxisX_->setRange(0.0, std::max(5.0, t));
    }
    ++zIndex_;
}

void PressureChartWindow::updateXRange(QValueAxis* axis, double lastTime) {
    const double xMax = std::max(kWindowSeconds, lastTime + 0.05);
    const double xMin = std::max(0.0, xMax - kWindowSeconds);
    axis->setRange(xMin, xMax);
}

void PressureChartWindow::updateYRange(QChart* chart, QValueAxis* axis) {
    double lo = std::numeric_limits<double>::max();
    double hi = std::numeric_limits<double>::lowest();
    for (QAbstractSeries* series : chart->series()) {
        auto* line = qobject_cast<QLineSeries*>(series);
        if (line == nullptr) continue;
        // 跳过透明的 zActualSeries_（它的折线颜色是 transparent，
        // 但数据点还是有效的，我们仍然扫描它——因为覆盖层的曲线用的就是这些点）
        // 其实扫描没问题，Y 轴范围是对的，只是透明了而已
        const auto points = line->points();
        for (const QPointF& point : points) {
            lo = std::min(lo, point.y());
            hi = std::max(hi, point.y());
        }
    }
    if (lo > hi) return;
    if (hi - lo < 1.0) {
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
        && (watched == zView_ || watched == zView_->viewport()
            || watched == zSplineOverlay_)) {
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

double PressureChartWindow::lastTimeOf(const QChartView* view) const {
    auto* line = (view == pressureView_) ? pressureMeasuredSeries_ : zActualSeries_;
    if (line && line->count() > 0) {
        return line->at(line->count() - 1).x();
    }
    return startTimeSec();
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
    const double now = startTimeSec();
    updateXRange(pressureAxisX_, now);
    updateXRange(zAxisX_, now);
    updateYRange(pressureChart_, pressureAxisY_);
    updateYRange(zChart_, zAxisY_);
    if (zSplineOverlay_) zSplineOverlay_->update();
}

void PressureChartWindow::toggleFocus(QChartView* view) {
    if (view == nullptr) return;
    if (focusedView_ == view) {
        restoreLiveView();
        return;
    }

    focusedView_ = view;
    const bool isPressure = (view == pressureView_);
    (isPressure ? pressureView_ : zView_)->setVisible(true);
    (isPressure ? zView_ : pressureView_)->setVisible(false);

    *followOf(view) = false;
    *lockedOf(view) = false;

    const double t = lastTimeOf(view);
    axisXOf(view)->setRange(0.0, std::max(5.0, t));
    updateYRange(chartOf(view), axisYOf(view));
    if (zSplineOverlay_) zSplineOverlay_->update();
}

void PressureChartWindow::zoomXAxis(QChartView* view, double factor,
                                    double anchorPixelX) {
    if (view == nullptr || factor <= 0.0) return;

    QValueAxis* axisX = axisXOf(view);
    const QRectF plot = chartOf(view)->plotArea();
    if (plot.width() <= 0.0) return;

    const double span = axisX->max() - axisX->min();
    const double frac = std::clamp(
        (anchorPixelX - plot.left()) / plot.width(), 0.0, 1.0);
    const double anchor = axisX->min() + frac * span;

    const double last = lastTimeOf(view);
    const double maxSpan = std::max(kWindowSeconds, std::max(kHistorySeconds, last));
    const double minSpan = 0.2;
    const double newSpan = std::clamp(span / factor, minSpan, maxSpan);

    double newMin = anchor - frac * newSpan;
    newMin = std::clamp(newMin, 0.0, std::max(0.0, last - newSpan));
    axisX->setRange(newMin, newMin + newSpan);

    *followOf(view) = false;
    *lockedOf(view) = true;
    if (zSplineOverlay_) zSplineOverlay_->update();
}

bool PressureChartWindow::eventFilter(QObject* watched, QEvent* event) {
    QChartView* view = viewAt(watched);
    if (view == nullptr) return QWidget::eventFilter(watched, event);

    switch (event->type()) {
    case QEvent::Resize: {
        // zView_ / pressureView_ 或其 viewport 大小变化 → 同步 overlay 大小
        if (zSplineOverlay_ && (watched == zView_ || watched == zView_->viewport())) {
            zSplineOverlay_->setGeometry(zView_->viewport()->rect());
            zSplineOverlay_->update();
        }
        break;
    }
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
        if (view->viewport()) view->viewport()->setCursor(Qt::ClosedHandCursor);
        return true;
    }
    case QEvent::MouseMove: {
        auto* me = static_cast<QMouseEvent*>(event);
        if (dragView_ != view || !(me->buttons() & Qt::LeftButton)) break;
        const int dx = me->pos().x() - dragStartX_;
        if (!dragMoved_ && std::abs(dx) > 3) dragMoved_ = true;
        if (!dragMoved_) return true;
        *followOf(view) = false;
        *lockedOf(view) = true;
        const double last = lastTimeOf(view);
        const double maxMin = std::max(0.0, last - dragSpan_);
        const double newMin =
            std::clamp(dragStartMin_ - dx * dragUnitsPerPx_, 0.0, maxMin);
        axisXOf(view)->setRange(newMin, newMin + dragSpan_);
        // 覆盖层需要跟着重绘（view 会触发 paintEvent，但 overlay 是独立 widget，
        // Qt Charts 内部不一定会让它收到重绘事件）
        if (zSplineOverlay_) zSplineOverlay_->update();
        return true;
    }
    case QEvent::MouseButtonRelease: {
        auto* me = static_cast<QMouseEvent*>(event);
        if (dragView_ != view || me->button() != Qt::LeftButton) break;
        dragView_ = nullptr;
        if (view->viewport()) view->viewport()->setCursor(Qt::OpenHandCursor);
        if (!dragMoved_) toggleFocus(view);
        return true;
    }
    case QEvent::Wheel: {
        auto* we = static_cast<QWheelEvent*>(event);
        const int delta = we->angleDelta().y();
        if (delta == 0) break;
        const double factor = (delta > 0) ? 1.25 : (1.0 / 1.25);
        zoomXAxis(view, factor, we->position().x());
        return true;
    }
    case QEvent::NativeGesture: {
        auto* ge = static_cast<QNativeGestureEvent*>(event);
        if (ge->gestureType() != Qt::ZoomNativeGesture) break;
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
    startTime_.invalidate();
    restoreLiveView();
    pressureAxisX_->setRange(0.0, kWindowSeconds);
    zAxisX_->setRange(0.0, kWindowSeconds);
    pressureAxisY_->setRange(0, 120);
    zAxisY_->setRange(-450, -400);
    if (zSplineOverlay_) zSplineOverlay_->update();
}
