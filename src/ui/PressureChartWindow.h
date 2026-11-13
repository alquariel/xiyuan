#pragma once

// PressureChartWindow — EGM 运行时的实时曲线窗口
//
// 上半：压力对比图
//   三条折线：目标压力 vs 实测压力，附误差曲线
//
// 下半：Z 坐标对比图
//   两条曲线：
//     - 目标 z：折线渲染
//     - 实际反馈 z：**不插点**，直接用贝塞尔曲线连接已有点（通过透明覆盖层画 QPainterPath）
//
// 用 QtCharts 的 QChartView + QLineSeries 实现；Z 坐标图额外用一个
// 透明覆盖 Widget 画 QPainterPath 贝塞尔曲线，实现"已有点直接生成样条"。

#include <QElapsedTimer>
#include <QPoint>
#include <QWidget>

#include <QtCharts>

QT_BEGIN_NAMESPACE
class QLineSeries;
class QChart;
class QChartView;
class QValueAxis;
QT_END_NAMESPACE

class PressureChartWindow : public QWidget {
    Q_OBJECT

public:
    explicit PressureChartWindow(QWidget* parent = nullptr);

    // 一个采样周期（约 4 ms）调用一次
    void appendPressureSample(double targetN, double measuredN, double errorN);
    void appendZSample(double targetZmm, double actualZmm);

    void clearAll();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    static constexpr int    kMaxPoints    = 1000;
    static constexpr int    kMaxHistory   = 20000;
    static constexpr double kWindowSeconds = 4.0;
    static constexpr double kHistorySeconds = 80.0;
    static constexpr int    kZDecimFactor = 5;  // Z 绘制降采样因子：每 N 次采样 append 一次（250Hz / 5 = 50Hz 画图）

    void updateXRange(QValueAxis* axis, double lastTime);
    void updateYRange(QChart* chart, QValueAxis* axis);
    double startTimeSec() const;

    // ---- 鼠标交互 ----
    QChartView* viewAt(QObject* watched) const;
    QValueAxis* axisXOf(const QChartView* view) const;
    QValueAxis* axisYOf(const QChartView* view) const;
    QChart*     chartOf(const QChartView* view) const;
    double      lastTimeOf(const QChartView* view) const;
    bool*       followOf(const QChartView* view);
    bool*       lockedOf(const QChartView* view);
    void        toggleFocus(QChartView* view);
    void        restoreLiveView();
    void        zoomXAxis(QChartView* view, double factor, double anchorPixelX);

    // ---- 压力图（全部折线） ----
    QLineSeries* pressureTargetSeries_   = nullptr;
    QLineSeries* pressureMeasuredSeries_ = nullptr;
    QLineSeries* pressureErrorSeries_    = nullptr;
    QChart*      pressureChart_          = nullptr;
    QChartView*  pressureView_           = nullptr;
    QValueAxis*  pressureAxisX_          = nullptr;
    QValueAxis*  pressureAxisY_          = nullptr;

    // ---- Z 坐标图 ----
    QLineSeries* zTargetSeries_ = nullptr;  // 目标 z（折线渲染）
    QLineSeries* zActualSeries_ = nullptr;  // 实际反馈 z（存已有点，不直接渲染；贝塞尔覆盖层画）
    QChart*      zChart_        = nullptr;
    QChartView*  zView_         = nullptr;
    QValueAxis*  zAxisX_        = nullptr;
    QValueAxis*  zAxisY_        = nullptr;

    // 实际反馈 z 的贝塞尔覆盖层（透明 widget，画已有点的贝塞尔曲线）
    QWidget*     zSplineOverlay_ = nullptr;

    int pressureIndex_ = 0;
    int zIndex_        = 0;

    QElapsedTimer startTime_;

    bool pressureFollow_ = true;
    bool zFollow_        = true;
    bool pressureLocked_ = false;
    bool zLocked_        = false;

    QChartView* focusedView_ = nullptr;

    QChartView* dragView_    = nullptr;
    bool        dragMoved_   = false;
    int         dragStartX_  = 0;
    double      dragStartMin_ = 0.0;
    double      dragSpan_     = 0.0;
    double      dragUnitsPerPx_ = 1.0;
};
