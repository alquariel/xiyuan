#pragma once

// PressureChartWindow — EGM 运行时的实时曲线窗口
//
// 上半：压力对比图
//   两条折线：目标压力（常量）vs 实测压力，附误差曲线
//
// 下半：Z 坐标对比图
//   两条折线：目标 z（PathLoader 轨迹 + offset）vs 实际反馈 z（EgmRobot）
//
// 用 QtCharts 的 QChartView + QLineSeries 实现；保留最近 N 个采样点做滑动窗口。

#include <QPoint>
#include <QWidget>

#include <QtCharts>   // 总头：自动带 QtCharts 命名空间

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

    // 一个采样周期（约 4 ms）调用一次，窗口内部保留最近 kMaxPoints 个点。
    void appendPressureSample(double targetN, double measuredN, double errorN);
    void appendZSample(double targetZmm, double actualZmm);

    void clearAll();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    static constexpr int kMaxPoints  = 1000;   // 实时滑动窗口深度：约 4 s 数据（250Hz）
    static constexpr int kMaxHistory = 20000;  // 历史保留上限（约 80 s），供完整曲线查看

    // 让 X 轴跟随滑动窗口，避免采样序号超过固定范围后曲线滑出可视区
    void updateXRange(QValueAxis* axis, int lastIndex);
    // 按当前可见数据自适应 Y 轴范围（留出边距），避免硬编码量程裁掉曲线
    void updateYRange(QChart* chart, QValueAxis* axis);

    // ---- 鼠标交互 ----
    QChartView* viewAt(QObject* watched) const;  // 事件源对应的图表视图
    QValueAxis* axisXOf(const QChartView* view) const;
    QValueAxis* axisYOf(const QChartView* view) const;
    QChart*     chartOf(const QChartView* view) const;
    int         lastIndexOf(const QChartView* view) const;
    bool*       followOf(const QChartView* view);
    bool*       lockedOf(const QChartView* view);
    void        toggleFocus(QChartView* view);   // 单击：放大该图并显示完整曲线 / 恢复
    void        restoreLiveView();               // 恢复双图 + 实时滑动窗口
    void        zoomXAxis(QChartView* view, double factor, double anchorPixelX);

    // ---- 压力图 ----
    QLineSeries* pressureTargetSeries_   = nullptr;  // 目标
    QLineSeries* pressureMeasuredSeries_ = nullptr;  // 实测
    QLineSeries* pressureErrorSeries_    = nullptr;  // 误差（目标-实测）
    QChart*      pressureChart_          = nullptr;
    QChartView*  pressureView_           = nullptr;
    QValueAxis*  pressureAxisX_          = nullptr;
    QValueAxis*  pressureAxisY_          = nullptr;

    // ---- Z 坐标图 ----
    QLineSeries* zTargetSeries_ = nullptr;  // 目标 z
    QLineSeries* zActualSeries_ = nullptr;  // 实际反馈 z
    QChart*      zChart_        = nullptr;
    QChartView*  zView_         = nullptr;
    QValueAxis*  zAxisX_        = nullptr;
    QValueAxis*  zAxisY_        = nullptr;

    int pressureIndex_ = 0;  // 压力图独立的 x 轴采样计数
    int zIndex_        = 0;  // Z 图独立的 x 轴采样计数

    // 每个图独立维护视图状态：是否自动跟随、是否被手动平移锁定
    bool pressureFollow_ = true;
    bool zFollow_        = true;
    bool pressureLocked_ = false;
    bool zLocked_        = false;

    QChartView* focusedView_ = nullptr;  // 被单击放大的图（nullptr = 双图平铺）

    // 拖动平移的临时状态
    QChartView* dragView_    = nullptr;
    bool        dragMoved_   = false;
    int         dragStartX_  = 0;
    double      dragStartMin_ = 0.0;
    double      dragSpan_     = 0.0;
    double      dragUnitsPerPx_ = 1.0;
};
