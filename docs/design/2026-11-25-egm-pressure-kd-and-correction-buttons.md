# EGM 压力修正：新增 Kd 微分项与修正启停双按钮（2026-11-25）

## 背景

控制律原先只有 Kp、Ki（PI），无法抑制压力快速变化时的超调；页签上「启用修正」是单个切换按钮，启停两个动作共用一个控件，状态不够直观。

## 更改内容

### 1. 新增 Kd 微分参数（默认 0）

涉及文件：`src/app/AppConfig.h`、`src/app/AppConfig.cpp`、`config/env_config.json`、`src/egm/PressurePiController.h`、`src/egm/PressurePiController.cpp`、`src/egm/EgmPressureController.h`、`src/egm/EgmPressureController.cpp`

- 配置新增 `egm_pressure.kd`，默认 `0.0`，按非负浮点解析；`env_config.json` 同步加入 `"kd": 0.0`。
- `PressurePiConfig` 新增 `kd`；`EgmPressureController::setControlParameters()` 增加 `kd` 形参，并写入控制配置。
- 控制律：
  - 微分项 `kd * Δ压力 / dt`，对测量值求导（设定值恒定时等价于误差微分，避免设定值跳变产生冲击）。
  - 首帧无历史压力，微分输出为 0；微分项限幅为 `±zMaxMm`，避免噪声或 dt 抖动产生尖峰。
  - 未使能、样本失效/超时、`reset()` 时同步清空微分历史，恢复后不会因旧压力值产生跳变。
  - `kd=0`（默认）时微分项恒为 0，行为与改动前的 PI 完全一致。

### 2. 「启用修正 / 停止修正」拆成两个按钮

涉及文件：`src/ui/EgmPressurePanel.h`、`src/ui/EgmPressurePanel.cpp`

- 删除可切换按钮，新增两个独立按钮：`启用修正`、`停止修正`。
- 点击「启用修正」调用 `setCorrectionEnabled(true)`；点击「停止修正」调用 `setCorrectionEnabled(false)`。
- 按钮使能状态由监听与修正状态决定：未监听时两个按钮均不可用；监听中且未修正时仅「启用修正」可用，修正中仅「停止修正」可用。
- 「停止监听」「一键回零」后修正状态自动回到未启用，按钮状态随之刷新。
- 参数区新增 `Kd` 输入框（0–10000，步进 0.001，4 位小数，初值取自配置）。

## 影响范围

- 默认配置 `kd=0`，现场无需改参数即可保持原行为；需要抑制超调时再逐步增大 Kd。
- 不涉及 EGM 报文、UDP 端口、RAPID 侧与配置键结构（仅新增一个可选项）。

## 验证

- `cmake --build build --target AFP_ABB_CPP test_egm_pressure_pi_controller` 构建通过。
- `test_egm_pressure_pi_controller` 全部通过：`Totals: 15 passed, 0 failed`，其中新增 `derivativeReactsToPressureRise`、`resetClearsDerivativeState` 两个微分用例。
