# Slice M 设计：基于 EGM Path Correction 的压力法向闭环修正

日期：2026-11-24

## 1. 目标

在主程序 `AFP_ABB_CPP.exe` 内新增 **EGM 压力修正模块**（主界面新增页签），进程内新建 DDS Participant 订阅压力与机器人监控实时数据，按 PI 控制律计算沿**铺放法向**的 Z 偏移量，并通过 ABB **EGM Path Correction（路径校正）**通道按 48 ms 周期回传给控制器，实时叠加到现有 RAPID 运动指令的标称路径上。

数据链路：

```text
PressureAcquisitionWorker ──DDS afp.pressure.sample────────┐
                                                          ├──> EGM 压力修正模块（主进程内 QThread）
RobotMonitoringService ────DDS afp.robot.monitor.sample────┘      │  (DDS subscriber node)
                                                                  │  PI: P_mean → z_corr
                                                                  │  48ms 回包
                                                                  v
                                              UDP 6510 + protobuf (EgmSensorPathCorr / EgmRobot)
                                                                  │
                                                                  v
                                     ABB 控制器 EGM Path Correction（RobotWare 6.15）
                                                                  │
                                     RAPID: EGMSetupUC\PathCorr\APTR + EGMActMove
                                            EGMMoveL / EGMMoveC / EGMReset
                                     标称路径 + 外部法向偏移
```

关键设计约束：

1. EGM 控制回路与打印/PLC 指令**物理隔离**，不共享线程、不共享 Snap7 连接、不共享进程。
2. DDS 只做数据传输，控制律在 EGM 节点内部实现（DDS 本身不含任何控制算法）。
3. 控制器侧保留标称路径与安全限幅，外部只能施加有界的偏移量。

## 2. 前提与现状

### 2.1 已确认的现场条件

| 项 | 结论 |
|---|---|
| 控制器型号 | **IRC5**（FlexPendant 菜单路径与 OmniCore 不同，见 §10.1） |
| 控制器 IP | **192.168.0.104**（取自 `config/env_config.json` 的 `robot.ip`，同时用于运动 socket 5010、监控 socket 5012、FTP 21） |
| 控制器 RobotWare 版本 | **6.15**（项目备份 `rapid/BACKINFO/version.xml`） |
| EGM 授权 | 已具备（选项 689-1） |
| `currentTool` Z 轴 | 铺放法向（工具垂直铺放面） |
| 控制律首版 | **PI** |
| 部署形态 | **合并进主程序 `AFP_ABB_CPP.exe`**：主界面新增「EGM 压力修正」页签，EGM 收发与 PI 跑专用 `QThread`，不新增独立 exe（决策见 §16） |

### 2.2 手册版本差异（重要）

以 **RobotWare 6.5/6.16 版《应用手册 - Externally Guided Motion》**（`d:\EGM_ROBOTWARE6.5.pdf`，文档编号 3HAC073319-010）为准，控制器 **6.15** 的 Path Correction 指令集如下（已在手册 §5.1.2、§5.1.6、§5.1.14、§5.4.4 确认）：

| 项 | RAPID 指令 | 说明 |
|---|---|---|
| 取标识 | `EGMGetId egmId1;` | 每个运动任务最多 4 个 egmident |
| 建立/激活 | `EGMSetupUC ROB_1,egmId1,ExtConfigName,UCDevice\PathCorr\APTR;` + `EGMActMove egmId1,currentTool.tframe\SampleRate:=48;` | `\PathCorr` 必须配 `\APTR`（at-point）或 `\LATR`（前瞻）；SampleRate 为 24 ms 倍数 |
| 执行带校正的路径 | `EGMMoveL egmId,ToPoint,Speed,Zone,Tool\WObj:=...;` | **起点必须是普通 `MoveL ... fine`**；最后一个 `EGMMoveL` 必须 `fine` |
| 结束 | `EGMReset egmId1;` | 释放标识；异常停止时可用 `EGMStop egmId1,EGM_STOP_RAMP_DOWN\RampOutTime:=5.0;` |

**注意**：6.5 手册中不存在 `EGMActPathCorr` / `EGMDeactPathCorr`（那是早期设计稿的猜测），实际落地一律使用 `EGMActMove` + `EGMMoveL`。

报文层不再需要 `protoc`：`third_party/egm/egm.proto` 仅作为字段契约，实际的编解码由 `src/egm/EgmWireCodec` 手写完成，并已用参考工程 Python `egm_pb2` 的黄金字节做单测校验（见 §9.3）。现场无需再取 6.15 模板或生成 `egm.pb.*`。

### 2.3 可复用的现有资产

| 资产 | 用途 |
|---|---|
| `src/pressure/PressureSample.idl` | `AfpPressureSample`：`raw_value`、`mean_value`、`variance_value`、`standard_deviation_value`、`peak_value`、`window_ready`、`sensor_running` |
| `src/robot_monitor/RobotMonitorSample.idl` | `AfpRobotMonitorSample`：TCP `x/y/z`、四元数 `q1..q4`、关节 `j1..j6`、外轴、速度 |
| `src/archive/PressureSampleReader.*` | Fast DDS DataReader 订阅范式，可直接仿写 |
| `src/archive/DdsInfluxSubscriberApp.*` | DDS 多 reader 编排与配置加载范式（结构可参考，本方案不再独立成进程） |
| `src/pressure/PressureStatistics.*` | 40 点滑窗统计，`mean_value` 已是滤波结果 |
| `src/app/AppConfig.*` | 现有 JSON 配置加载与"warnings 收集 + 默认值兜底"范式，`egm_pressure` 段直接复用 |
| `docs/design/slice-c / slice-j / slice-k / slice-l` | DDS 话题、进程、配置的既有约定 |

### 2.4 现有话题与域

| 数据 | domain | topic |
|---|---|---|
| 压力 | 0 | `afp.pressure.sample` |
| 机器人监控 | 0 | `afp.robot.monitor.sample` |

EGM 模块在主进程内新建**独立 DDS Participant（node）**接入同一 domain，与现有 publisher 的 participant 互不影响，属正常多订阅者用法，不需要改动任何现有 publisher。

## 3. 总体架构

### 3.1 进程与线程模型

```text
AFP_ABB_CPP.exe（主程序：UI + PLC + 打印 + EGM 压力修正）
   ├── Qt 主线程        : 主界面（含 EGM 页签）、参数编辑、状态刷新、日志
   ├── EGM 线程(QThread): 阻塞 recvfrom(EgmRobot) → 立即 sendto(EgmSensorPathCorr) ← 48ms 硬节拍
   │                     （PI 计算在同一线程内按 48ms dt 执行）
   └── DDS 订阅线程     : Fast DDS listener 回调，仅写入"最新样本"快照
```

线程分工的关键原因：

- **EGM 必须由收包驱动**，不能用 UI 定时器驱动。控制器每 48 ms 发一个 `EgmRobot`，模块必须在同一节拍内回 `EgmSensorPathCorr`；用 `recvfrom` 阻塞等待 + 立即回包，天然对齐，不受本地事件循环抖动影响。
- **DDS 回调只做"写快照"**，不做控制计算，避免 Fast DDS listener 线程被长时间占用。
- **UI 与通信解耦**：UI 卡顿（例如滑动窗口重绘）不得影响回包。UI 通过原子快照读数据，参数通过原子/互斥写入。

### 3.2 时间尺度

| 环节 | 周期 | 说明 |
|---|---|---|
| 压力 DDS | 100 ms | `pressure.sample_interval_ms` |
| 机器人监控 DDS | 100 ms | `robot.monitor.sample_interval_ms` |
| EGM 收发 | 48 ms（`SampleRate:=48`，合法值 24 / 48 / 72） | 控制回路的硬节拍 |

DDS 慢于 EGM：一个压力样本会被约 2 个 EGM 周期复用，属**采样保持**，由 PI 的积分项与输出速率限制保证连续性，不会出现阶跃跳变。

### 3.3 控制界面（主界面新增页签）

首版界面只保留必要项：

| 区域 | 内容 |
|---|---|
| 状态栏 | DDS 压力订阅状态、DDS 监控订阅状态、EGM UDP 连接状态、压力样本新鲜度（ms）、当前 `z_corr` |
| 参数区 | `pressure_setpoint`、`kp`、`ki`、`z_max_mm`、`dz_max_per_cycle_mm` |
| 操作区 | 开始监听 / 停止监听、启用修正 / 停止修正按钮、**一键回零**（`z_corr` 平滑回 0） |
| 实时区 | 压力均值、修正输出、收/发帧数等文本状态 |
| 告警区 | 数据超时、方向契约未确认、UDP 断连、输出饱和 |

安全要求：**关闭窗口或点击"停止"等价于 `z_corr` 按 `ramp_down_ms` 平滑回 0**，绝不直接断掉 UDP（断掉会让控制器进入 EGM 超时停止，虽安全但会中断打印）。

## 4. RAPID 侧改造

### 4.1 接入点

打印路径在 `rapid/RAPID/TASK1/PROGMOD/Motion.mod` 的 **CASE 33**（缓冲路径 `MoveL` 循环）。EGM 只包裹这一循环，不影响示教点、回零等其它分支。

### 4.2 已落地写法（`Motion.mod` CASE 33）

模块数据区新增（`PERS` 便于在示教器直接改，不用重装模块）：

```rapid
PERS bool useEgmPathCorr:=FALSE;          ! 默认关闭，现场联调通过后再改 TRUE
PERS string egmExtConfigName:="pathCorr"; ! External Motion Interface Data 名称
PERS string egmUcDevice:="UCdevice";      ! UdpUc 装置名称
PERS num egmSampleRate:=48;               ! 24 ms 的倍数
VAR egmident egmId1;
```

CASE 33 在保留 PLC 参数逐点下发逻辑的前提下，按下面的方式执行缓冲路径：

```rapid
IF useEgmPathCorr AND BUFFER_POS>1 THEN
    EGMGetId egmId1;
    EGMSetupUC ROB_1,egmId1,egmExtConfigName,egmUcDevice\PathCorr\APTR;
    EGMActMove egmId1,currentTool.tframe\SampleRate:=egmSampleRate;
ENDIF

FOR i FROM 1 TO (BUFFER_POS) DO
    ... (原有 PLC 变化检测与 SendPLCParamsToPC 逻辑，保持不变) ...
    IF useEgmPathCorr AND BUFFER_POS>1 THEN
        IF i=1 THEN
            MoveL bufferTargets{i},bufferSpeeds{i},fine,currentTool\WObj:=currentWobj;
        ELSEIF i=BUFFER_POS THEN
            EGMMoveL egmId1,bufferTargets{i},bufferSpeeds{i},fine,currentTool\WObj:=currentWobj;
        ELSE
            EGMMoveL egmId1,bufferTargets{i},bufferSpeeds{i},currentZone,currentTool\WObj:=currentWobj;
        ENDIF
    ELSE
        MoveL bufferTargets{i},bufferSpeeds{i},currentZone,currentTool\WObj:=currentWobj;
    ENDIF
ENDFOR

IF useEgmPathCorr AND BUFFER_POS>1 THEN
    EGMReset egmId1;
ENDIF
```

要点：

1. **第一点用普通 `MoveL ... fine` 到位**，从第二点起才用 `EGMMoveL`；**最后一个 `EGMMoveL` 必须 `fine`**，否则下一个普通运动可能被拒绝。
2. `\APTR`（at-point）表示传感器在 TCP 处，符合"压力传感器装在铺放头末端"的实际；`\LATR`（前瞻）不适用。
3. `EGMSetupUC` 会与 UdpUc 装置建立通信，因此**启动顺序必须是：PC 端先开始监听，再把 `useEgmPathCorr` 改为 TRUE 并触发 CASE 33**。
4. `BUFFER_POS<=1`（空路径或单点）时完全走原有 `MoveL` 分支，不建立 EGM，避免无意义的通信。
5. 现场调试若 `useEgmPathCorr:=TRUE` 但 PC 未就绪，会抛可恢复错误 `ERR_UDPUC_COMM`；此时把开关改回 FALSE 即可回退，不需要改代码。
6. 原 `use_plc_forwarding` 单点转发路径（`PLC_SERVER.mod` 的 `exec_composite_120`）**未**接入 EGM，仍为普通 `MoveL`。

### 4.3 控制器侧参数

在系统参数中建立：

- **UdpUc 装置**：Remote Address = 上位机 IP，Remote Port = 6510；Local Port = 控制器侧端口。
- **External Motion Interface Data** 配置 `"pathCorr"`：Default Ramp Time、Default Proportional Position Gain、Default Low Pass Filter Bandwidth。

建议：Ramp Time 保持默认 0.5 s 级别，位置增益先用默认，让 PI 负责外环，避免内外两个增益互相干扰。

## 5. 坐标语义与符号契约（最高优先级风险项）

手册明确：**Correction frame is always the path frame**（校正量始终在路径坐标系中表达）；`\APTR` 时 SensorFrame 为工具坐标系。

本应用中工具始终垂直铺放面、路径切向与工具 XY 平面对齐，因此**路径系 Z ≈ 工具 Z ≈ 铺放法向**。但这是"应用假设"，不是手册保证，必须用实验确认：

1. **零修正干跑**：`z_corr` 恒为 0，跑完一条路径，确认机器人轨迹与标称路径重合、无系统性偏移。
2. **开环阶跃**：手动给 `z_corr = +1 mm`，确认末端沿**离开工件表面**的方向移动 1 mm。
3. 若方向相反，只调整符号契约，**不允许**改 PI 增益来"凑"方向。

符号契约（写入代码常量并加注释）：

```text
工具 +Z = 离开工件表面（抬刀）
P_mean > setpoint（压力过大） -> 抬刀 -> z_corr > 0
P_mean < setpoint（压力不足） -> 下压 -> z_corr < 0
```

单位约定：压力为传感器原始单位（未标定前不做物理量换算）；`z_corr` 单位为 **mm**，与 EGM 平移量一致。

## 6. EGM 报文契约

### 6.1 方向与端口

- **控制器是 UDP 客户端，PC 是 UDP 服务端**，PC 绑定 6510。
- 控制器先发 `EgmRobot`；PC 必须回 `EgmSensorPathCorr`，否则 EGM 超时停止（安全侧失效）。
- 报文为 protobuf 编码；本模块使用自带的 `src/egm/EgmWireCodec` 完成最小 wire 编解码，不引入 protobuf 运行时（原因见 §9.3）。

### 6.2 EgmSensorPathCorr（PC → 控制器）

> 已按参考实现 `PyQt5 main_202509_WDY_path_corr.py` / `ABBRobotEGM/egm.py` 的 `send_to_robot_path_corr` 校正报文结构。

```text
EgmSensorPathCorr
 ├── header
 │     ├── seqno   : 每帧只 +1 一次
 │     └── mtype   : MSGTYPE_PATH_CORRECTION (= 4)
 └── pathCorr
       ├── pos { x = 0, y = 0, z = z_corr (mm) }
       └── age : uint32，单位**毫秒**（proto 注释：Sensor measurement age in ms），建议填 48 或实测时延
```

实现要点：

- 路径校正专用报文是 **`EgmSensorPathCorr`**，**不是** `EgmSensor.planned.cartesian`（后者属于位置流 / `EGMRunPose`，本方案不用）。
- `pos` 是**路径坐标系**下的位移（对应 RAPID `CorrConn` 语义），x/y 恒 0，只给 z。
- `age` 是 uint32 **毫秒**（传感器测量时延），不能传 0 或小数。
- **未收到过机器人任何报文前不能发送**（参考实现以 `egm_addr` 为空直接返回 false）。
- 每帧 `seqno` 只自增一次；参考实现自增两次，属缺陷，不复刻。
- 字段编号以 `third_party/egm/egm.proto` 为准；编码结果已与参考工程 Python `egm_pb2` 做字节级比对（`tests/test_egm_udp_server.cpp`）。

### 6.3 EgmRobot（控制器 → PC）

用途：

- 作为回包节拍源（收到即回）。
- 取 `feedback` 中的实际位姿/关节，用于界面显示、限位判断与录波。
- 报文序号/时间戳用于检测丢包与延迟。

## 7. C++ 模块设计

新增目录 `src/egm/`，模块编入主程序目标 `AFP_ABB_CPP`（不新增独立 exe）。

| 模块 | 职责 | 依赖 |
|---|---|---|
| `PressureSampleSubscriber` | 订阅 `afp.pressure.sample`，写入最新样本快照（含接收时间戳） | Fast DDS |
| `RobotMonitorSubscriber` | 订阅 `afp.robot.monitor.sample`，写入最新位姿快照 | Fast DDS |
| `EgmSnapshots.h` | 压力/机器人最新样本的原子快照结构（含接收时间戳），供 UDP 线程与 UI 无锁读取 | 无 |
| `PressurePiController` | 纯 PI 控制律：输入压力均值 + setpoint + dt，输出 `z_corr`。**无 Qt / 无网络依赖，可单测** | 无 |
| `EgmUdpServer` | 绑定 6510，阻塞 `recvfrom(EgmRobot)` → 调用控制律 → 组 `EgmSensorPathCorr` → `sendto` | Qt Network、`EgmWireCodec` |
| `EgmPressureController` | 编排：`QThread` 生命周期、启停、健康状态、优雅退出 | 上述模块 |
| `EgmPressurePanel` | 主界面新增页签（Qt Widgets），参数读写、状态、告警 | Qt |
| `EgmWireCodec` | ABB `egm.proto` 的最小 protobuf wire 编解码（仅 EgmRobot / EgmSensorPathCorr） | 无 |
| `third_party/egm/egm.proto` | ABB 官方 proto，作为字段契约参考，不参与编译 | 无 |

### 7.1 PI 控制律

每个 EGM 周期（dt = 48 ms）执行：

```text
输入:
  p_meas   = 最新 pressure.mean_value（需 window_ready == true 且未超时）
  p_set    = pressure_setpoint
  kp, ki   = 增益
  z_max    = 输出限幅（mm）
  dz_max   = 每周期最大变化量（mm/cycle）

计算:
  e        = p_meas - p_set                     // 见 §5 符号契约
  i_term  += ki * e * dt                        // 条件积分，抗饱和
  u_raw    = kp * e + i_term
  u_clamp  = clamp(u_raw, -z_max, +z_max)
  z_corr   = rateLimit(u_clamp, z_prev, dz_max)

抗饱和:
  若 u_raw 已饱和且 e 使饱和加剧 -> 冻结 i_term（条件积分）

输出:
  z_corr (mm)
```

附加规则：

- **数据失效降级**：压力样本超过 `stale_timeout_ms` 未更新，或 `sensor_running == false`，则 `z_corr` 按 `ramp_rate_mm_per_s` 平滑回 0，并置告警；不回 0 则等于用旧数据继续压，危险。
- **首周期**：`i_term = 0`，`z_corr = 0`，不做突跳。
- **参数热更新**：`kp/ki/setpoint/z_max` 修改后立即生效，但 `i_term` 不重置（避免扰动），仅在下一次饱和判断时使用新限幅。
- **积分限幅**：`i_term` 单独限幅到 `±z_max`，避免长期偏差导致积分巨大。

### 7.2 安全限位（应用层）

`z_corr` 输出前再做一次应用级约束：

- 绝对值不超过 `z_max_mm`（默认 3 mm）。
- 若监控 DDS 显示 TCP 实际位置偏离标称路径超过软阈值（如 5 mm），立即回零并告警。
- 若 UDP 连续 `n` 个周期未收到 `EgmRobot`，视为链路异常，界面告警（此时控制器侧也会自行停止）。

## 8. 配置

在 `config/env_config.json` 新增段落（与现有 `pressure` / `robot.monitor` 风格一致）：

```json
"egm_pressure": {
  "_note": "EGM path-correction module inside AFP_ABB_CPP. DDS subscriber + UDP 6510 server.",
  "enabled": true,
  "robot_ip": "192.168.0.104",
  "egm_udp_port": 6510,
  "control_period_ms": 48,
  "correction_age_ms": 48,
  "receive_timeout_ms": 500,
  "pressure_topic": "afp.pressure.sample",
  "robot_monitor_topic": "afp.robot.monitor.sample",
  "dds_domain_id": 0,
  "pressure_setpoint": 0.0,
  "kp": 0.0,
  "ki": 0.0,
  "z_max_mm": 3.0,
  "dz_max_per_cycle_mm": 0.1,
  "stale_timeout_ms": 300,
  "ramp_rate_mm_per_s": 5.0
}
```

对应扩展 `src/app/AppConfig.h` / `AppConfig.cpp`，新增 `EgmPressureConfig` 结构；解析失败时沿用现有"warnings 收集 + 默认值兜底"的约定。

`enabled=false` 时不创建页签与 EGM 线程；页签内点「开始监听」才绑定 6510 并启动 DDS 订阅。

## 9. 构建与集成

### 9.1 CMake

```cmake
# 追加到主程序 APP_SOURCES 列表
    src/egm/EgmSnapshots.h
    src/egm/EgmPressureController.h        src/egm/EgmPressureController.cpp
    src/egm/EgmUdpServer.h                 src/egm/EgmUdpServer.cpp
    src/egm/EgmWireCodec.h                 src/egm/EgmWireCodec.cpp
    src/egm/PressurePiController.h         src/egm/PressurePiController.cpp
    src/egm/PressureSampleSubscriber.h     src/egm/PressureSampleSubscriber.cpp
    src/egm/RobotMonitorSubscriber.h       src/egm/RobotMonitorSubscriber.cpp
    src/ui/EgmPressurePanel.h              src/ui/EgmPressurePanel.cpp
```

对应测试目标：`test_egm_pressure_pi_controller`、`test_egm_udp_server`。

- 复用现有 `src/pressure/generated`、`src/robot_monitor/generated` 类型。
- 不新增 protobuf 依赖，因此无需复制 protobuf 运行时 DLL。
- MinGW 构建需定义 `MINGW_COMPILER`（Fast DDS 头文件要求），已在 `CMakeLists.txt` 顶部按 `MINGW` 条件加 `add_compile_definitions`。
- 不再需要 `QProcess` 管理；模块随主程序启动，页签内点「开始监听」时创建 `QThread`。

### 9.2 启动时序

```text
1. 主程序启动 -> EGM 模块默认不监听，页签显示"未启动"
2. 操作员在页签点「开始监听」-> 绑定 UDP 6510 -> DDS 订阅就绪 -> 显示"等待 EGM 连接"
3. 触发打印 -> RAPID 执行 CASE 33 -> EGMSetupUC + EGMActMove 建立连接
4. 模块首次收到 EgmRobot -> 回 EgmSensorPathCorr(z=0) -> 进入常驻闭环
5. 打印结束 -> EGMReset -> 模块保持监听（进程不退出）
```

**禁止**在页签未开始监听时把 `useEgmPathCorr` 置 TRUE：`EGMSetupUC`/`EGMActMove` 会等待 UdpUc 通信，控制器侧报 `ERR_UDPUC_COMM`。

### 9.3 实施状态与 protobuf 取舍

已实现（`build/AFP_ABB_CPP.exe` 可编译）：

- `EgmWireCodec`：protobuf wire 编解码。`EgmSensorPathCorr` 编码与参考工程 Python `egm_pb2` 逐字节一致；`EgmRobot` 解码覆盖 `header` / `feedBack.cartesian` / `motorState` / `mciState`。
- `EgmUdpServer`：6510 绑定、先收后发、seqno 每帧单增、未收首帧不发、链路超时停发、输出硬限幅。
- `PressureSampleSubscriber` / `RobotMonitorSubscriber`：DDS 回调内仅写最新快照。
- `PressurePiController` + `EgmPressureController`：PI 在专用线程执行，参数热更新，失效平滑回零，链路恢复清积分。
- `EgmPressurePanel`：主界面新增「EGM 压力修正」页签（状态、参数、启停、回零、启用/停止修正按钮）。
- 单元测试：`tests/test_egm_pressure_pi_controller.cpp`（11 例）、`tests/test_egm_udp_server.cpp`（7 例，含 Python 黄金字节比对）。

不使用 protobuf 运行时的原因：

1. 构建机没有 Visual Studio，vcpkg 无法构建 protobuf 的 x64-windows 宿主 `protoc`；
2. 指定 `--host-triplet=x64-mingw-dynamic` 时安装卡在 GitHub 源码下载；
3. 本模块只需两个报文、字段固定，自带 wire 编解码可离线构建、无额外 DLL，且能用参考 `egm_pb2` 做字节级校验。

RAPID 侧已完成：按 6.5/6.16 手册把 `Motion.mod` CASE 33 改为 `EGMSetupUC\PathCorr\APTR` + `EGMActMove` + `EGMMoveL` + `EGMReset`，以 `PERS bool useEgmPathCorr`（默认 FALSE）作为总开关，见 §4.2。仍需现场编译确认语法与 UdpUc/ExtConfig 命名。

### 9.4 运行时依赖部署（已实测）

主程序 `POST_BUILD` 依次执行：

1. 复制 `config/`、`assets/`、`lib/`，并创建 `mod_files/`；
2. 调用 `windeployqt` 部署 Qt 6.8.3 运行时（解决 `Qt6Widgets.dll`、`platforms/qwindows.dll` 缺失）；
3. 复制 `$<TARGET_RUNTIME_DLLS:AFP_ABB_CPP>`（Snap7、Fast DDS 等 vcpkg 动态库）；
4. 显式补齐 `libcrypto-3-x64.dll`、`libssl-3-x64.dll`、`libtinyxml2.dll`、`libfoonathan_memory-0.7.4.dll`（Fast DDS 的间接依赖，`TARGET_RUNTIME_DLLS` 不一定覆盖）。

实测 `build/AFP_ABB_CPP.exe` 可直接双击启动（窗口标题「机器人面向铺带3D打印软件平台」）；`egm_pressure.enabled=true` 时主界面出现「EGM 压力修正」页签。

## 10. 手动操作清单（现场实施）

> 约定：标注【需重启】的系统参数改动，必须在控制器上执行"重启控制器"。所有改动前**先做系统备份**（示教器 → 备份与恢复 → 备份系统）。菜单名以现场 FlexPendant 实际显示为准，不同 IRC5/OmniCore 版本措辞略有差异。

### 10.1 控制器侧（示教器）

**10.1.1 前置确认**

1. 确认已安装选项 **689-1 EGM**：示教器 → 系统信息 / 系统属性 → Options，或核对 `rapid/BACKINFO/backinfo.txt`。
2. 记下控制器 IP，并与上位机放到同一网段（如 `192.168.0.x/24`），双方互相 `ping` 通。
3. `currentTool` 已标定，且其 Z 轴为铺放法向（本方案的前提）。

**10.1.2 新建 UdpUc（UDP 单播）通信装置【需重启】**

**IRC5 菜单路径**：FlexPendant → **ABB 菜单 → 控制面板（Control Panel）→ 配置（Configuration）→ 主题 Communication → UdpUc**。

已知信息：控制器 IP 为 **`192.168.0.104`**（来自现有 `config/env_config.json` 的 `robot.ip`，运动 socket 5010、监控 socket 5012、FTP 都用它）；上位机需与该地址同网段。

| 参数 | 填写值 | 说明 |
|---|---|---|
| Name | `UCdevice` | 必须与 RAPID `egmUcDevice`（`EGMSetupUC` 第 4 个参数）一致 |
| Remote Address | 上位机 IP（如 `192.168.0.50`） | 控制器是 UDP 客户端，"Remote"指的是运行 EGM 服务端的上位机 |
| Remote Port Number | `6510` | 上位机 EGM 服务端的监听端口 |
| Local Port Number | `6511` | 控制器本地端口，避免与其它服务冲突 |

注意：端口方向不要填反——**上位机在 6510 上收 `EgmRobot`、回 `EgmSensorPathCorr`**。参考实现固定使用 6510。

**10.1.3 新建 External Motion Interface Data 配置【需重启】**

示教器 → 配置（Configuration）→ 主题 **Motion** → 类型 **External Motion Interface Data** → 添加

| 参数 | 填写值 | 说明 |
|---|---|---|
| Name | `pathCorr` | **必须与 RAPID `EGMSetupUC ... "pathCorr"` 的字符串完全一致，大小写敏感** |
| Default Ramp Time | 默认（约 0.5 s），可设 0.2–0.5 | EGM 停止时速度归零所用时间 |
| Default Proportional Position Gain | 保持默认 | 外环 PI 在上位机，控制器内环别再调高 |
| Default Low Pass Filter Bandwidth | 保持默认 | 同上，避免双环耦合 |

**10.1.4 重启并核对配置**

1. 重启控制器。
2. 回到配置界面，确认 `UdpUc/UCdevice` 与 `Motion/pathCorr` 两个实例存在、无红色报错。
3. 若提示端口冲突或配置无效，检查 Local/Remote Port 是否与已有服务重复。

**10.1.5 载入改造后的 RAPID**

1. 用现有 FTP 上传流程上传修改后的 `Motion.mod`（或在 RobotStudio 同步后下载到控制器）。
2. 确认模块内已声明 `VAR egmident egmId1;` 以及 `useEgmPathCorr` / `egmExtConfigName` / `egmUcDevice` / `egmSampleRate`，且 CASE 33 中包含 `EGMGetId` / `EGMSetupUC` / `EGMActMove` / `EGMMoveL` / `EGMReset`。
3. **在示教器指令列表中逐一确认指令存在**：`EGMGetId`、`EGMSetupUC`（含 `\PathCorr`、`\APTR`）、`EGMActMove`、`EGMMoveL`、`EGMReset`。
   - 若 `EGMSetupUC` 无法选择 `\PathCorr` 开关，需检查 EGM 选项是否完整安装。
4. 编译（Ctrl+F5 或"编译"），确认无语法错误；确认后 `useEgmPathCorr` 仍保持 FALSE，进入 §10.2 联调时再置 TRUE。

**10.1.6 运行前置条件**

- 控制器切 **Auto** 模式、电机上电（Motors On）、RAPID 程序就绪。
- 上位机 EGM 页签先点「开始监听」，再把控制器上的 `useEgmPathCorr` 置 TRUE。
- CASE 33 首点已由代码固定用 `MoveL ... fine` 到位，不再依赖 `currentZone` 的取值。
- 不要用手动/单步模式验证 EGM 闭环。

### 10.2 上位机（Windows）侧

1. 设置静态 IP，与控制器同网段；关闭该网卡的节能/休眠。
2. **放行入站 UDP 6510**（专用网络）。PowerShell（管理员）：
   ```powershell
   New-NetFirewallRule -DisplayName "AFP EGM UDP 6510" -Direction Inbound -Protocol UDP -LocalPort 6510 -Action Allow
   ```
3. 检查端口是否被占用：`netstat -ano | findstr 6510`，有输出则先释放。
4. `egm.proto` 已放在 `third_party/egm/`，只作字段契约参考；**不需要**从控制器取 EGM 模板，**不需要** protoc 生成代码（编解码在 `src/egm/EgmWireCodec`）。
5. 填写 `config/env_config.json` 的 `egm_pressure` 段；首轮联调把 `kp`、`ki` 置 0（等价零修正）。
6. 构建并启动主程序，在「EGM 压力修正」页签点「开始监听」，确认界面显示"DDS 订阅正常 / 等待 EGM 连接"，且 `netstat` 能看到 UDP 6510 处于监听。

### 10.3 联调顺序（严格按序，禁止跳步）

1. **零修正干跑**：`kp=ki=0`，跑完整条打印路径；确认轨迹与标称重合、`EgmRobot` 持续到达、界面无告警。
2. **开环阶跃**：手动置 `z_corr=±1 mm`，确认末端沿工具 Z 正方向抬刀（见 §5）。
3. **小增益闭环**：`kp` 从小往大调，`ki` 最后加；观察压力收敛与 `z_corr` 平滑度。
4. **异常注入**：断压力 DDS、断 UDP、`sensor_running=false`、超软限位，确认 `z_corr` 平滑回零并告警。

### 10.4 回滚预案

1. 上位机：先点界面"停止/回零"，待 `z_corr` 平滑归零后再点页签「停止监听」（直接关闭主程序会让控制器 EGM 超时停止）。
2. 控制器：把 `useEgmPathCorr` 改回 FALSE 即可立即回退；必要时恢复改造前的 `Motion.mod` 备份，或删除 `UdpUc/UCdevice` 与 `Motion/pathCorr` 实例并重启。
3. 情况严重时直接用改动前的系统备份恢复控制器。

### 10.5 需要你提供的现场信息

| # | 信息 | 状态 | 用途 |
|---|---|---|---|
| 1 | 控制器是 IRC5 还是 OmniCore | **已确认：IRC5** | 决定菜单路径与模板目录 |
| 2 | 控制器 IP / 上位机 IP | 控制器 **192.168.0.104**（已从配置确认）；**上位机 IP 待定** | 填写 UdpUc 与配置文件 |
| 3 | 6510 / 6511 是否被占用 | 待检查 | 避免端口冲突 |
| 4 | 是否已标定 `currentTool` 的 Z 为法向 | 待现场确认 | 决定符号契约能否直接用 |
| 5 | 6.15 版 `egm.proto` | **不再阻塞**：字段已按参考实现固定并做字节级校验 | 报文层已实现，无需现场提供 |

## 11. 测试与验证

| 阶段 | 内容 | 通过标准 |
|---|---|---|
| 1. 单元测试（**已实现，11 例**） | `PressurePiController`：阶跃响应、限幅、速率限制、抗饱和、失效回零、dt 变化 | 纯逻辑断言全通过，不接硬件 |
| 2. 报文单测（**已实现，7 例**） | `EgmUdpServer` 用本地回环模拟控制器：发 `EgmRobot`，校验回包 `mtype`/`seqno`/`pos.z`，含 Python `egm_pb2` 黄金字节 | 字段与序号正确；无回包时超时逻辑正确 |
| 2.1 构建/启动冒烟（**已通过**） | 构建主程序并部署运行时依赖 | `AFP_ABB_CPP.exe` 可启动，EGM 页签可见 |
| 3. 零修正联调 | `z_corr ≡ 0`，跑完整条打印路径 | 轨迹与标称重合；`EgmRobot` 持续到达；UI 状态正常 |
| 4. 开环阶跃 | 手动 `z_corr = ±1 mm` | 末端沿法向正确移动，方向与 §5 契约一致 |
| 5. 闭环整定 | 小 `kp` 起步，观察压力收敛与 `z_corr` 平滑度 | 无持续振荡；超调可接受；不触发限幅饱和 |
| 6. 异常注入 | 断开压力 DDS、断开 UDP、超软限位、`sensor_running=false` | `z_corr` 平滑回零、告警、打印不中断（或按预期停止） |

## 12. 风险与待确认项

| # | 风险 | 处理 |
|---|---|---|
| 1 | ~~RW 6.15 与 7.8 指令集不一致~~ **已消除** | 以 6.5/6.16 手册确认 `EGMSetupUC\PathCorr\APTR` + `EGMActMove` + `EGMMoveL` + `EGMReset`，现场只需编译确认 |
| 2 | ~~拿不到 6.15 版 `egm.proto`~~ **已消除** | 采用 `third_party/egm/egm.proto` + 自带 wire 编解码，已与 Python `egm_pb2` 字节比对 |
| 3 | 路径系 Z 与工具 Z 是否严格同向 | §5 的零修正 + 开环阶跃两步强制验证；符号写死为契约常量 |
| 4 | 压力单位未标定，`kp/ki` 无物理意义 | 首版用无量纲整定；后续若标定为 N，可换算并重标增益 |
| 5 | 压力采样 100 ms 慢于 EGM 48 ms | 采样保持 + PI 积分 + 速率限制；必要时提高压力采样率 |
| 6 | ~~主程序与 EGM 节点版本漂移~~ 已消除 | 合并为同一构建目标，无独立部署物 |
| 7 | 操作员误操作导致突跳 | 输出限幅 + 速率限制 + 一键回零 + 参数改动不重置积分 |

## 13. 实施顺序与当前状态

**已完成 1–5、7**：主程序构建通过，18 项单测通过，exe 启动冒烟通过；**只剩 6 的现场联调**。

1. 【已完成】`PressurePiController` + 单元测试（11 例，纯逻辑、零硬件风险）。
2. 【已完成】报文契约 `EgmWireCodec` + `EgmUdpServer` 报文单测（7 例，本地回环 + Python `egm_pb2` 黄金字节）。
3. 【已完成】DDS 订阅模块（`PressureSampleSubscriber` / `RobotMonitorSubscriber`）。
4. 【已完成】主界面「EGM 压力修正」页签 + `egm_pressure` 配置接入。
5. 【已完成】RAPID `Motion.mod` CASE 33 改造；默认 `useEgmPathCorr=FALSE`，待现场编译与联调时再置 TRUE。
6. 【待现场】零修正联调 → 开环阶跃 → 闭环整定 → 异常注入。
7. 【已完成】生命周期收尾：页签「停止」先回零再停监听，随主程序退出，无 `QProcess`。

## 14. 本切片范围外

- 不改动现有 `PressureDdsPublisher` / `RobotMonitorDdsPublisher` / `dds_influx_subscriber`。
- 不改动 PLC 打印/运动指令通道与其 Snap7 连接。
- 不做压力传感器的物理量标定。
- 不做多机器人/多 EGM 实例支持。
- 不引入 ROS；沿用 Fast DDS。

## 15. 参考实现对照（PyQt5 `main_202509_WDY_path_corr.py`）

参考实现位于 `d:/pyqt5_2025_1012_WDY_pos_control/`，其中 `path_corr()`（第 479 行起）与 `ABBRobotEGM/egm.py::send_to_robot_path_corr()`（第 494 行起）是本方案的直接依据。**不参考其 `speed_corr` 与 `pose_control` 部分。**

### 15.1 直接采纳

| # | 参考做法 | 本方案对应 |
|---|---|---|
| 1 | 报文用 `EgmSensorPathCorr`，字段 `pathCorr.pos{x,y,z}` + `pathCorr.age` | 已修正 §6.2（此前误写为 `EgmSensor.planned.cartesian`） |
| 2 | EGM 循环在**专用线程**内阻塞 `recvfrom`，先收后发 | §3.1 EGM 通信线程 |
| 3 | 未收到机器人首帧前不发送（`egm_addr` 为空即返回 false） | `EgmUdpServer` 首次收包后才允许发送 |
| 4 | 修正量逐轴限幅（参考 ±8 mm） | `z_max_mm`（默认 3 mm）单轴限幅 |
| 5 | 闭环开关关闭时**继续按周期发送 0 修正**，不停发 | 保持链路存活，避免 EGM 超时停止 |
| 6 | 起始点不施加修正，进入执行段后才闭环 | CASE 33 首点 `fine` 到位后再使能修正 |
| 7 | 压力超阈值时置故障标志并抬刀/停止 | §7.1 数据失效与 §7.2 安全限位 |

### 15.2 明确不复刻

| # | 参考实现的问题 | 本方案处理 |
|---|---|---|
| 1 | 接收超时只置 `break_flag`、不退出循环，仍向旧地址继续发修正 | 超时即停止发送，进入回零/告警 |
| 2 | `seqno` 每帧自增两次（`send_to_robot_path_corr` 内重复自增） | 每帧只 +1 |
| 3 | 跨线程共享裸全局变量（`high` / `start_corr` / `break_flag`），无同步 | 原子快照 + 互斥保护 |
| 4 | 压力采样不新鲜时仍用旧值继续闭环 | `stale_timeout_ms` 超时即回零 |
| 5 | 校正量与 UI 控件状态强耦合（`action` / `pressure_activate_flag`） | 控制律与界面解耦，界面只改参数 |

## 16. 部署形态决策（参考实现后更新）

参考实现把 `path_corr`、`speed_corr`、压力采集全部放在**同一个 PyQt5 进程**里，用 `threading.Thread(daemon=True)` 并发（`start_robot_thread`，第 798 行）。这证明"同进程 + 专用线程"在工程上可行，**独立 exe 不是硬性要求**。

两种部署方式的取舍：

| | A. 独立 exe（早期方案，已放弃） | B. 合并进 `AFP_ABB_CPP.exe`（现方案） |
|---|---|---|
| UI | 独立控制窗口 | 主界面新增页签/对话框 |
| EGM 线程 | 进程内专用线程 | 进程内专用 `QThread`（同样必须独立线程） |
| 调试期改增益 | 可单独重启 EGM 节点，不影响打印 | 需重启整个主程序（除非参数全部热更新） |
| 故障隔离 | EGM 崩溃不影响打印主程序 | EGM 异常可能拖垮主程序 |
| 依赖 | protobuf 只进 EGM target | protobuf 进入主 target |
| 配置/日志 | 需独立加载配置、日志分两处 | 直接复用 `AppConfig` 与日志系统 |
| 部署 | 多一个 exe 与 DLL | 单 exe |

**两种方案都必须满足的硬约束**（与是否拆进程无关）：

1. EGM 收发与 PI 计算**绝不能在 Qt 主线程**演算——现有 `PlcPrintSignalManager::waitForAck` 会在主线程忙等最多 3 s（`RobotPrintService::startPrint` 由按钮直连），主线程一旦被占，EGM 必然超时。
2. 必须"先收后发"，且未收到首帧不发送。
3. 必须独立 socket，不共享 PLC 的 Snap7 连接。

**决策（2026-11-24）：采用方案 B —— 合并进主进程。** 主界面新增「EGM 压力修正」页签，EGM 收发与 PI 计算运行在专用 `QThread`，复用 `AppConfig`、日志与 DDS；不新增独立 exe。§1、§3、§7、§8、§9 已按此更新。
