# EGM 压力修正页签界面精简（2026-11-25）

## 背景

「EGM 压力修正」页签原先包含两个 60 s 滚动趋势图（压力均值、Z 修正），现场使用中不需要，且占用大量纵向空间。同时「启用修正」为勾选框，启停语义不够直观。

## 更改内容

### 1. 删除两个趋势曲线窗口

涉及文件：`src/ui/EgmPressurePanel.h`、`src/ui/EgmPressurePanel.cpp`

- 删除「压力均值」「Z 修正 (mm)」两个曲线控件。
- 一并移除：`TrendPlot` 类、`pressurePlot_` / `correctionPlot_` 成员、`clock_`（`QElapsedTimer`）及 `updateLabels()` 中的采样推送、`applyParameters()` 中的参考线设置。
- 清理不再使用的 include：`QPainter`、`QVector`、`QElapsedTimer`、`QHBoxLayout`、`<algorithm>`。
- 页面保留：连接状态区（DDS / EGM 链路、压力均值、修正输出、收发帧数）、控制参数区、操作按钮、告警文本。

### 2. 「启用修正」勾选框改为启停按钮

- `QCheckBox* correctionCheck_` 改为 `QPushButton* correctionButton_`，`setCheckable(true)`。
- 未启用时显示「启用修正」；点击后变为「停止修正」并调用 `EgmPressureController::setCorrectionEnabled(true)`；再次点击恢复并调用 `setCorrectionEnabled(false)`。
- 点击「停止监听」或「一键回零」时自动 `setChecked(false)`，按钮文本随 `toggled` 信号恢复为「启用修正」。

## 影响范围

- 不涉及 EGM 报文、控制律、配置文件与 RAPID 侧逻辑，仅 UI 层与页签刷新逻辑。
- 主设计文档 `2026-11-24-afp-abb-cpp-slice-m-egm-pressure-path-correction-design.md` 中线程职责、组件表、UI 区域说明已同步为「无曲线、按钮式启停」。

## 验证

`cmake --build build --target AFP_ABB_CPP` 构建通过（退出码 0）。
