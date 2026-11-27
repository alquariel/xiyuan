#!/usr/bin/env python3
"""EGM PI/PID 控制律可视化自测（L1 纯逻辑单测的可视化版本）

直接按 src/egm/PressurePiController.cpp 的逻辑在 Python 中复现控制律，
模拟无硬件场景并生成一份自包含 HTML 报告（内联 SVG/Canvas，无第三方依赖）：

  1. 零修正：误差非零时输出恒 0（零增益 / 未使能两条路径）
  2. 阶跃响应：带一阶被控对象的闭环，统计上升时间、超调、调节时间、稳态误差
  3. 饱和与速率限制：z_max 截断、dz_max 每周期限幅、抗积分饱和
  4. DDS 断流回零：样本失效后按 ramp_rate 平滑回零

用法：
    python src\\tools\\egm_pi_visualizer.py
    python src\\tools\\egm_pi_visualizer.py --output build\\egm_pi.html --open

报告默认写到 build\\egm_pi_visual_report.html。
"""

import argparse
import json
import math
import os
import time
import webbrowser

DT = 0.048  # EGM 周期，与 RAPID SampleRate:=48 一致

COLOR_BLUE = "#2563eb"
COLOR_RED = "#dc2626"
COLOR_GREEN = "#16a34a"
COLOR_AMBER = "#d97706"
COLOR_PURPLE = "#7c3aed"


# ---------------------------------------------------------------------------
# PressurePiController 的 Python 复刻（必须与 C++ 逐步一致）
# ---------------------------------------------------------------------------
class PiConfig:
    def __init__(self, setpoint=0.0, kp=0.0, ki=0.0, kd=0.0, z_max_mm=3.0,
                 dz_max_per_cycle_mm=0.1, stale_timeout_ms=300.0,
                 ramp_rate_mm_per_s=5.0):
        self.setpoint = setpoint
        self.kp = kp
        self.ki = ki
        self.kd = kd
        self.z_max_mm = z_max_mm
        self.dz_max_per_cycle_mm = dz_max_per_cycle_mm
        self.stale_timeout_ms = stale_timeout_ms
        self.ramp_rate_mm_per_s = ramp_rate_mm_per_s


def _clamp(value, low, high):
    return max(low, min(high, value))


class PiController:
    def __init__(self, config):
        self.config = config
        self.output_mm = 0.0
        self.integral_term = 0.0
        self.prev_pressure = 0.0
        self.has_prev_pressure = False
        self.saturated = False

    def reset(self):
        self.output_mm = 0.0
        self.integral_term = 0.0
        self.prev_pressure = 0.0
        self.has_prev_pressure = False
        self.saturated = False

    def _ramp_toward_zero(self, dt):
        step = self.config.ramp_rate_mm_per_s * dt
        if self.output_mm > 0.0:
            return max(0.0, self.output_mm - step)
        if self.output_mm < 0.0:
            return min(0.0, self.output_mm + step)
        return 0.0

    def update(self, pressure_mean, sample_valid, sample_age_ms,
               correction_enabled, dt):
        cfg = self.config
        if dt <= 0.0:
            return self.output_mm

        sample_fresh = sample_valid and sample_age_ms <= cfg.stale_timeout_ms

        # 未使能 / 样本无效 / 超时：平滑回零并清积分。
        if not correction_enabled or not sample_fresh:
            self.integral_term = 0.0
            self.has_prev_pressure = False
            self.saturated = False
            self.output_mm = self._ramp_toward_zero(dt)
            return self.output_mm

        error = pressure_mean - cfg.setpoint

        derivative = 0.0
        if cfg.kd > 0.0:
            if self.has_prev_pressure:
                derivative = _clamp(cfg.kd * (pressure_mean - self.prev_pressure) / dt,
                                    -cfg.z_max_mm, cfg.z_max_mm)
            self.prev_pressure = pressure_mean
            self.has_prev_pressure = True

        integral_next = _clamp(self.integral_term + cfg.ki * error * dt,
                               -cfg.z_max_mm, cfg.z_max_mm)
        raw = cfg.kp * error + derivative + integral_next
        saturated_positive = raw > cfg.z_max_mm
        saturated_negative = raw < -cfg.z_max_mm
        freeze_integral = ((saturated_positive and error > 0.0) or
                           (saturated_negative and error < 0.0))

        if not freeze_integral:
            self.integral_term = integral_next
        raw = cfg.kp * error + derivative + self.integral_term
        self.saturated = raw > cfg.z_max_mm or raw < -cfg.z_max_mm

        clamped = _clamp(raw, -cfg.z_max_mm, cfg.z_max_mm)
        lower = max(self.output_mm - cfg.dz_max_per_cycle_mm, -cfg.z_max_mm)
        upper = min(self.output_mm + cfg.dz_max_per_cycle_mm, cfg.z_max_mm)
        self.output_mm = _clamp(clamped, lower, upper)
        return self.output_mm


# ---------------------------------------------------------------------------
# 场景
# ---------------------------------------------------------------------------
def _round_points(points):
    return [[round(x, 4), round(y, 5)] for x, y in points]


def _chart(title, ylabel, series, hlines=None, vlines=None, note="",
           xlabel="时间 (s)"):
    return {
        "title": title,
        "xlabel": xlabel,
        "ylabel": ylabel,
        "series": series,
        "hlines": hlines or [],
        "vlines": vlines or [],
        "note": note,
    }


def _result(scenario, metric, value, criterion, ok):
    return {"scenario": scenario, "metric": metric, "value": value,
            "criterion": criterion, "ok": bool(ok)}


def scenario_zero():
    """误差非零但零增益 / 未使能时，输出必须恒为 0。"""
    zero_gain = PiController(PiConfig(kp=0.0, ki=0.0, kd=0.0))
    disabled = PiController(PiConfig(kp=0.1, ki=1.0, kd=0.0))
    series_a, series_b = [], []
    pressure = 50.0  # 与 setpoint=0 相比误差很大
    for index in range(250):
        t = (index + 1) * DT
        z_a = zero_gain.update(pressure, True, 0.0, True, DT)
        z_b = disabled.update(pressure, True, 0.0, False, DT)
        series_a.append((t, z_a))
        series_b.append((t, z_b))

    max_a = max(abs(z) for _, z in series_a)
    max_b = max(abs(z) for _, z in series_b)
    results = [
        _result("零修正", "零增益 max|z|", f"{max_a:.6f} mm", "= 0", max_a == 0.0),
        _result("零修正", "未使能 max|z|", f"{max_b:.6f} mm", "= 0", max_b == 0.0),
    ]
    chart = _chart(
        "零修正：误差 50 时输出恒 0",
        "z_corr (mm)",
        [
            {"name": "kp=ki=kd=0，已使能", "color": COLOR_BLUE,
             "points": _round_points(series_a)},
            {"name": "增益非零但未使能", "color": COLOR_AMBER,
             "points": _round_points(series_b)},
        ],
        note="两条路径均不输出修正；y 轴范围固定为 ±3 mm。")
    return [chart], results


def scenario_step():
    """一阶对象闭环阶跃：P = P0 - K*z，压力 DDS 按 100 ms 采样保持。"""
    config = PiConfig(setpoint=26.0, kp=0.05, ki=0.5, kd=0.0,
                      z_max_mm=3.0, dz_max_per_cycle_mm=0.1)
    controller = PiController(config)
    p0, plant_gain = 30.0, 2.0
    pressure = p0
    held = pressure
    cycles = 400

    z_points, p_points = [], []
    for index in range(cycles):
        if index % 2 == 0:  # 100 ms 压力更新，EGM 周期 48 ms
            held = pressure
        z = controller.update(held, True, 0.0, True, DT)
        pressure = p0 - plant_gain * z
        t = (index + 1) * DT
        z_points.append((t, z))
        p_points.append((t, pressure))

    tail = max(4, cycles // 5)
    final_z = sum(z for _, z in z_points[-tail:]) / tail
    final_p = sum(p for _, p in p_points[-tail:]) / tail
    rise_time = next((t for t, z in z_points if z >= 0.9 * final_z), None)
    max_z = max(z for _, z in z_points)
    overshoot = (max_z - final_z) / final_z * 100.0 if final_z else 0.0
    band = 0.05 * abs(final_z)
    settle_time = 0.0
    for t, z in z_points:
        if abs(z - final_z) > band:
            settle_time = t
    steady_error = abs(final_p - config.setpoint)

    results = [
        _result("阶跃响应", "上升时间 (10%→90%)",
                f"{rise_time:.2f} s" if rise_time else "--", "≤ 2.5 s",
                rise_time is not None and rise_time <= 2.5),
        _result("阶跃响应", "超调", f"{overshoot:.1f} %", "≤ 10 %", overshoot <= 10.0),
        _result("阶跃响应", "调节时间 (±5%)", f"{settle_time:.2f} s", "≤ 5.0 s",
                settle_time <= 5.0),
        _result("阶跃响应", "稳态误差", f"{steady_error:.3f} 单位", "≤ 0.5",
                steady_error <= 0.5),
    ]
    z_chart = _chart(
        "阶跃响应：z_corr",
        "z_corr (mm)",
        [{"name": "z_corr", "color": COLOR_BLUE, "points": _round_points(z_points)}],
        hlines=[{"y": config.z_max_mm, "label": f"z_max = {config.z_max_mm} mm",
                 "color": COLOR_RED}],
        note=f"setpoint={config.setpoint}，Kp={config.kp}，Ki={config.ki}，"
             f"z_max={config.z_max_mm} mm，dz_max={config.dz_max_per_cycle_mm} mm/周期")
    p_chart = _chart(
        "阶跃响应：压力",
        "压力 (原始单位)",
        [
            {"name": "压力均值", "color": COLOR_BLUE, "points": _round_points(p_points)},
            {"name": "目标压力", "color": COLOR_GREEN,
             "points": [[1 * DT, config.setpoint], [cycles * DT, config.setpoint]]},
        ],
        note=f"被控对象 P = {p0} - {plant_gain}·z；压力按 100 ms 采样保持。")
    return [z_chart, p_chart], results


def scenario_saturation():
    """大误差下的 z_max 截断、dz_max 限幅与积分冻结。"""
    config = PiConfig(setpoint=0.0, kp=1.0, ki=1.0, kd=0.0,
                      z_max_mm=3.0, dz_max_per_cycle_mm=0.1)
    controller = PiController(config)
    z_points, integral_points = [], []
    cycles = 60
    previous_z = 0.0
    max_step = 0.0
    saturate_cycle = None

    for index in range(cycles):
        z = controller.update(100.0, True, 0.0, True, DT)
        if index == 0:
            max_step = abs(z - 0.0)
        else:
            max_step = max(max_step, abs(z - previous_z))
        previous_z = z
        if saturate_cycle is None and abs(z - config.z_max_mm) < 1e-9:
            saturate_cycle = index + 1
        t = (index + 1) * DT
        z_points.append((t, z))
        integral_points.append((t, controller.integral_term))

    max_abs_z = max(abs(z) for _, z in z_points)
    max_abs_integral = max(abs(i) for _, i in integral_points)
    expected_cycles = math.ceil(config.z_max_mm / config.dz_max_per_cycle_mm)
    results = [
        _result("饱和/限幅", "max|z|", f"{max_abs_z:.6f} mm",
                f"≤ {config.z_max_mm} mm", max_abs_z <= config.z_max_mm + 1e-9),
        _result("饱和/限幅", "max|Δz|", f"{max_step:.6f} mm/周期",
                f"≤ {config.dz_max_per_cycle_mm}", max_step <= config.dz_max_per_cycle_mm + 1e-9),
        _result("饱和/限幅", "达到 z_max 周期数",
                str(saturate_cycle) if saturate_cycle else "--",
                f"= {expected_cycles}", saturate_cycle == expected_cycles),
        _result("饱和/限幅", "饱和时积分项", f"{max_abs_integral:.6f} mm",
                "冻结为 0", max_abs_integral == 0.0),
    ]
    chart = _chart(
        "饱和与速率限制：误差 100，Kp=Ki=1",
        "mm",
        [
            {"name": "z_corr", "color": COLOR_BLUE, "points": _round_points(z_points)},
            {"name": "积分项（抗饱和冻结）", "color": COLOR_PURPLE,
             "points": _round_points(integral_points)},
        ],
        hlines=[{"y": config.z_max_mm, "label": f"z_max = {config.z_max_mm} mm",
                 "color": COLOR_RED}],
        note=f"z 每周期最多增加 {config.dz_max_per_cycle_mm} mm，"
             f"{expected_cycles} 个周期后饱和；饱和期间积分项保持 0。")
    return [chart], results


def scenario_ramp_down():
    """先建立非零输出，再注入样本失效，验证按 ramp_rate 平滑回零。"""
    config = PiConfig(setpoint=26.0, kp=0.05, ki=0.5, kd=0.0,
                      z_max_mm=3.0, dz_max_per_cycle_mm=0.1,
                      ramp_rate_mm_per_s=5.0)
    controller = PiController(config)
    p0, plant_gain = 30.0, 2.0
    pressure = p0
    held = pressure
    z_points = []

    warmup = 125  # 约 6 s，进入非零稳态
    for index in range(warmup):
        if index % 2 == 0:
            held = pressure
        z = controller.update(held, True, 0.0, True, DT)
        pressure = p0 - plant_gain * z
        z_points.append(((index + 1) * DT, z))

    z_at_fail = z_points[-1][1]
    max_step = 0.0
    previous_z = z_at_fail
    for index in range(40):
        z = controller.update(held, False, 0.0, True, DT)
        max_step = max(max_step, abs(z - previous_z))
        previous_z = z
        z_points.append(((warmup + index + 1) * DT, z))

    ramp_rate = config.ramp_rate_mm_per_s
    expected_step = ramp_rate * DT
    expected_cycles = math.ceil(abs(z_at_fail) / expected_step)
    zero_cycle = next((i for i, (_, z) in enumerate(z_points[warmup:]) if z == 0.0), None)
    results = [
        _result("断流回零", "失效前 |z|", f"{abs(z_at_fail):.3f} mm", "> 0",
                abs(z_at_fail) > 0.0),
        _result("断流回零", "max|Δz|", f"{max_step:.6f} mm/周期",
                f"≤ {expected_step:.3f}", max_step <= expected_step + 1e-9),
        _result("断流回零", "回零周期数",
                str(zero_cycle + 1) if zero_cycle is not None else "--",
                f"≤ {expected_cycles}", zero_cycle is not None and zero_cycle + 1 <= expected_cycles),
        _result("断流回零", "失效后积分项", f"{controller.integral_term:.6f} mm",
                "= 0", controller.integral_term == 0.0),
        _result("断流回零", "最终输出", f"{controller.output_mm:.6f} mm", "= 0",
                controller.output_mm == 0.0),
    ]
    chart = _chart(
        "DDS 断流回零：样本失效后按 5 mm/s 平滑归零",
        "z_corr (mm)",
        [{"name": "z_corr", "color": COLOR_BLUE, "points": _round_points(z_points)}],
        vlines=[{"x": warmup * DT, "label": "样本失效", "color": COLOR_AMBER}],
        note=f"每周期下降约 {expected_step:.3f} mm，约 {expected_cycles} 个周期回到 0，"
             f"无阶跃跳变。")
    return [chart], results


# ---------------------------------------------------------------------------
# HTML 报告
# ---------------------------------------------------------------------------
HTML_TEMPLATE = """<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="utf-8">
<title>__TITLE__</title>
<style>
  :root {
    --bg: #f8fafc; --panel: #ffffff; --border: #e2e8f0;
    --text: #0f172a; --muted: #64748b; --ok: #15803d; --bad: #b91c1c;
  }
  * { box-sizing: border-box; }
  body {
    margin: 0; padding: 28px; background: var(--bg); color: var(--text);
    font-family: "Segoe UI", "Microsoft YaHei", system-ui, sans-serif;
  }
  h1 { margin: 0 0 6px; font-size: 22px; }
  .meta { color: var(--muted); font-size: 13px; margin-bottom: 20px; line-height: 1.7; }
  .panel {
    background: var(--panel); border: 1px solid var(--border); border-radius: 10px;
    padding: 16px 18px; margin-bottom: 22px; overflow: hidden;
  }
  .panel h2 { margin: 0 0 12px; font-size: 16px; }
  table { border-collapse: collapse; width: 100%; font-size: 13px; }
  th, td { text-align: left; padding: 8px 10px; border-bottom: 1px solid var(--border); }
  th { color: var(--muted); font-weight: 600; background: #f1f5f9; }
  tr:last-child td { border-bottom: none; }
  .tag { font-weight: 700; }
  .tag.ok { color: var(--ok); }
  .tag.bad { color: var(--bad); }
  .charts { display: grid; grid-template-columns: repeat(auto-fit, minmax(480px, 1fr)); gap: 18px; }
  .card {
    background: var(--panel); border: 1px solid var(--border); border-radius: 10px;
    padding: 12px 14px 8px;
  }
  .card .head { font-size: 14px; font-weight: 600; margin-bottom: 2px; }
  .card .note { font-size: 12px; color: var(--muted); margin-bottom: 6px; }
  canvas { width: 100%; height: 300px; display: block; }
  footer { color: var(--muted); font-size: 12px; margin-top: 20px; }
</style>
</head>
<body>
<h1>__TITLE__</h1>
<div class="meta">__META__</div>

<div class="panel">
  <h2>验证结论</h2>
  <table>
    <thead><tr><th>场景</th><th>指标</th><th>实测值</th><th>判据</th><th>结果</th></tr></thead>
    <tbody>__ROWS__</tbody>
  </table>
</div>

<div class="charts" id="charts"></div>
<footer>由 src\\tools\\egm_pi_visualizer.py 生成 · __GENERATED_AT__</footer>

<script>
const CHARTS = __CHARTS_JSON__;
const COLORS = __COLORS_JSON__;

function niceStep(range, target) {
  const raw = range / Math.max(target, 1);
  if (!isFinite(raw) || raw <= 0) return 1;
  const mag = Math.pow(10, Math.floor(Math.log10(raw)));
  const norm = raw / mag;
  let step;
  if (norm <= 1) step = 1;
  else if (norm <= 2) step = 2;
  else if (norm <= 5) step = 5;
  else step = 10;
  return step * mag;
}

function ticks(min, max, count) {
  if (!isFinite(min) || !isFinite(max)) { min = 0; max = 1; }
  if (max === min) { max = min + 1; }
  const step = niceStep(max - min, count);
  const start = Math.ceil(min / step) * step;
  const out = [];
  for (let v = start; v <= max + step * 1e-6; v += step) out.push(Number(v.toFixed(10)));
  return { values: out, step: step };
}

function decimalsFor(step) {
  if (step >= 1) return Math.abs(step - Math.round(step)) < 1e-9 ? 0 : 1;
  return Math.min(4, Math.max(0, Math.ceil(-Math.log10(step))));
}

function drawChart(canvas, chart) {
  const dpr = window.devicePixelRatio || 1;
  const cssW = canvas.clientWidth || 520;
  const cssH = 300;
  canvas.width = Math.round(cssW * dpr);
  canvas.height = Math.round(cssH * dpr);
  const ctx = canvas.getContext("2d");
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, cssW, cssH);

  const pad = { left: 64, right: 24, top: 20, bottom: 46 };
  const plotW = cssW - pad.left - pad.right;
  const plotH = cssH - pad.top - pad.bottom;

  let xs = [], ys = [];
  chart.series.forEach(s => s.points.forEach(p => { xs.push(p[0]); ys.push(p[1]); }));
  chart.hlines.forEach(h => ys.push(h.y));
  chart.vlines.forEach(v => xs.push(v.x));
  if (!xs.length) { xs = [0, 1]; }
  if (!ys.length) { ys = [0, 1]; }
  let xmin = Math.min.apply(null, xs), xmax = Math.max.apply(null, xs);
  let ymin = Math.min.apply(null, ys), ymax = Math.max.apply(null, ys);
  if (xmax === xmin) { xmax += 1; }
  if (ymax === ymin) { ymax += 1; ymin -= 1; }
  const xPad = (xmax - xmin) * 0.02, yPad = (ymax - ymin) * 0.08;
  xmin -= xPad; xmax += xPad; ymin -= yPad; ymax += yPad;

  const sx = x => pad.left + (x - xmin) / (xmax - xmin) * plotW;
  const sy = y => pad.top + (ymax - y) / (ymax - ymin) * plotH;

  const xt = ticks(xmin, xmax, 7);
  const yt = ticks(ymin, ymax, 6);
  const xDec = decimalsFor(xt.step), yDec = decimalsFor(yt.step);

  ctx.font = "12px Segoe UI, Microsoft YaHei, sans-serif";
  ctx.lineWidth = 1;
  ctx.strokeStyle = "#e2e8f0";
  ctx.fillStyle = "#64748b";

  xt.values.forEach(v => {
    const x = sx(v);
    ctx.beginPath(); ctx.moveTo(x, pad.top); ctx.lineTo(x, pad.top + plotH); ctx.stroke();
    ctx.textAlign = "center"; ctx.textBaseline = "top";
    ctx.fillText(v.toFixed(xDec), x, pad.top + plotH + 8);
  });
  yt.values.forEach(v => {
    const y = sy(v);
    ctx.beginPath(); ctx.moveTo(pad.left, y); ctx.lineTo(pad.left + plotW, y); ctx.stroke();
    ctx.textAlign = "right"; ctx.textBaseline = "middle";
    ctx.fillText(v.toFixed(yDec), pad.left - 8, y);
  });

  ctx.strokeStyle = "#94a3b8";
  ctx.beginPath();
  ctx.moveTo(pad.left, pad.top);
  ctx.lineTo(pad.left, pad.top + plotH);
  ctx.lineTo(pad.left + plotW, pad.top + plotH);
  ctx.stroke();

  ctx.save();
  ctx.translate(16, pad.top + plotH / 2);
  ctx.rotate(-Math.PI / 2);
  ctx.textAlign = "center"; ctx.textBaseline = "middle";
  ctx.fillStyle = "#475569";
  ctx.fillText(chart.ylabel, 0, 0);
  ctx.restore();
  ctx.textAlign = "center"; ctx.textBaseline = "bottom";
  ctx.fillStyle = "#475569";
  ctx.fillText(chart.xlabel, pad.left + plotW / 2, cssH - 8);

  chart.hlines.forEach(h => {
    if (h.y < ymin || h.y > ymax) return;
    const y = sy(h.y);
    ctx.save();
    ctx.setLineDash([6, 4]);
    ctx.strokeStyle = h.color;
    ctx.beginPath(); ctx.moveTo(pad.left, y); ctx.lineTo(pad.left + plotW, y); ctx.stroke();
    ctx.restore();
    ctx.fillStyle = h.color;
    ctx.textAlign = "right"; ctx.textBaseline = "bottom";
    ctx.fillText(h.label, pad.left + plotW - 4, y - 3);
  });

  chart.vlines.forEach(v => {
    if (v.x < xmin || v.x > xmax) return;
    const x = sx(v.x);
    ctx.save();
    ctx.setLineDash([6, 4]);
    ctx.strokeStyle = v.color;
    ctx.beginPath(); ctx.moveTo(x, pad.top); ctx.lineTo(x, pad.top + plotH); ctx.stroke();
    ctx.restore();
    ctx.fillStyle = v.color;
    ctx.textAlign = "left"; ctx.textBaseline = "top";
    ctx.fillText(v.label, x + 4, pad.top + 4);
  });

  chart.series.forEach(s => {
    if (!s.points.length) return;
    ctx.strokeStyle = s.color;
    ctx.lineWidth = 2;
    ctx.beginPath();
    s.points.forEach((p, i) => {
      const x = sx(p[0]), y = sy(p[1]);
      if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    });
    ctx.stroke();
  });

  let legendY = pad.top + 6;
  chart.series.forEach(s => {
    ctx.strokeStyle = s.color;
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.moveTo(pad.left + 10, legendY + 5);
    ctx.lineTo(pad.left + 28, legendY + 5);
    ctx.stroke();
    ctx.fillStyle = "#334155";
    ctx.textAlign = "left"; ctx.textBaseline = "top";
    ctx.font = "12px Segoe UI, Microsoft YaHei, sans-serif";
    ctx.fillText(s.name, pad.left + 34, legendY);
    legendY += 18;
  });
}

const chartHost = document.getElementById("charts");
const canvases = [];
CHARTS.forEach(chart => {
  const card = document.createElement("div");
  card.className = "card";
  const head = document.createElement("div");
  head.className = "head";
  head.textContent = chart.title;
  const note = document.createElement("div");
  note.className = "note";
  note.textContent = chart.note || "";
  const canvas = document.createElement("canvas");
  card.appendChild(head);
  card.appendChild(note);
  card.appendChild(canvas);
  chartHost.appendChild(card);
  canvases.push([canvas, chart]);
});

function redrawAll() {
  canvases.forEach(([canvas, chart]) => drawChart(canvas, chart));
}
window.addEventListener("resize", redrawAll);
redrawAll();
</script>
</body>
</html>
"""


def build_report(results, charts):
    rows = []
    for item in results:
        tag = "PASS" if item["ok"] else "FAIL"
        css = "ok" if item["ok"] else "bad"
        rows.append(
            "<tr><td>{scenario}</td><td>{metric}</td><td>{value}</td>"
            "<td>{criterion}</td><td class=\"tag {css}\">{tag}</td></tr>".format(
                scenario=item["scenario"], metric=item["metric"], value=item["value"],
                criterion=item["criterion"], css=css, tag=tag))
    meta = (
        "控制周期 48 ms（RAPID SampleRate:=48）；控制律按 src/egm/PressurePiController.cpp "
        "复刻；判定阈值为纯上位机自测基线。"
    )
    return HTML_TEMPLATE \
        .replace("__TITLE__", "EGM 压力路径修正 · L1 控制律可视化自测") \
        .replace("__META__", meta) \
        .replace("__ROWS__", "\n    ".join(rows)) \
        .replace("__CHARTS_JSON__", json.dumps(charts, ensure_ascii=False)) \
        .replace("__COLORS_JSON__", json.dumps([COLOR_BLUE, COLOR_AMBER])) \
        .replace("__GENERATED_AT__",
                 time.strftime("%Y-%m-%d %H:%M:%S"))


def main():
    parser = argparse.ArgumentParser(description="EGM PI/PID 控制律可视化自测")
    parser.add_argument("--output", default=os.path.join("build", "egm_pi_visual_report.html"),
                        help="输出 HTML 路径（默认 build\\egm_pi_visual_report.html）")
    parser.add_argument("--open", action="store_true", help="生成后用默认浏览器打开")
    args = parser.parse_args()

    results, charts = [], []
    for scenario in (scenario_zero, scenario_step, scenario_saturation, scenario_ramp_down):
        scenario_charts, scenario_results = scenario()
        charts.extend(scenario_charts)
        results.extend(scenario_results)

    output_path = os.path.abspath(args.output)
    output_dir = os.path.dirname(output_path)
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)
    with open(output_path, "w", encoding="utf-8") as handle:
        handle.write(build_report(results, charts))

    print(f"场景数 {len(set(r['scenario'] for r in results))}，"
          f"检查项 {len(results)}")
    for item in results:
        print(f"[{'PASS' if item['ok'] else 'FAIL'}] "
              f"{item['scenario']} · {item['metric']} = {item['value']} "
              f"(判据 {item['criterion']})")
    print(f"HTML 报告: {output_path}")

    if args.open:
        webbrowser.open("file:///" + output_path.replace("\\", "/"))

    return 0 if all(item["ok"] for item in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
