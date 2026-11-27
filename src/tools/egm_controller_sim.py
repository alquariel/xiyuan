#!/usr/bin/env python3
"""ABB EGM Path Correction 本地模拟控制器（第一阶段：纯上位机自测）

本脚本扮演 ABB 控制器，向上位机 EGM UDP 服务端（默认 127.0.0.1:6510）
按 48 ms 周期发送 EgmRobot 报文，并解析上位机回传的 EgmSensorPathCorr，
用于无机器人、无 PLC、无压力传感器时的 L3 上位机联调：

  T1 零修正回包恒 0（250 帧）
      python src\tools\egm_controller_sim.py --frames 250 --expect-zero

  T6 UDP 超时停发与恢复（先发 50 帧，静默 1.5 s，再发 50 帧）
      python src\tools\egm_controller_sim.py --frames 50 --pause-ms 1500 --resume-frames 50

前置条件：
  1. AFP_ABB_CPP.exe 已打开「EGM 压力修正」页签并点击「开始监听」；
  2. 页签内 kp=ki=kd=0，或已点击「停止修正」，此时期望 z_corr 恒为 0；
  3. 本机回环测试无需开放防火墙；跨机测试需放行入站 UDP 6510。

未收首帧不发（T7）由 C++ 单测 test_egm_udp_server 覆盖，本脚本不重复验证。

退出码：0 表示所有检查通过，1 表示存在失败项。
"""

import argparse
import csv
import socket
import struct
import sys
import time
from dataclasses import dataclass

MSGTYPE_DATA = 2
MSGTYPE_PATH_CORRECTION = 4

# 与 tests/test_egm_udp_server.cpp 相同的黄金字节，用于 --self-test 校验编码。
GOLDEN_SENSOR_HEX = (
    "0a07080710b9601804121f0a1b090000000000000000110000000000000000"
    "19000000000000f43f1030"
)
GOLDEN_ROBOT_HEX = (
    "0a07080710d0021802124512430a1b090000000000407f40110000000000002440"
    "190000000000005940122409000000000000f03f11000000000000000019000000"
    "0000000000210000000000000000220208012a020803"
)


# ---------------------------------------------------------------------------
# protobuf wire 编解码（只覆盖本模块使用的字段）
# ---------------------------------------------------------------------------
def _varint(value: int) -> bytes:
    out = bytearray()
    while value >= 0x80:
        out.append((value & 0x7F) | 0x80)
        value >>= 7
    out.append(value)
    return bytes(out)


def _double_field(field: int, value: float) -> bytes:
    return _varint((field << 3) | 1) + struct.pack("<d", value)


def _varint_field(field: int, value: int) -> bytes:
    return _varint((field << 3) | 0) + _varint(value)


def _length_field(field: int, payload: bytes) -> bytes:
    return _varint((field << 3) | 2) + _varint(len(payload)) + payload


def encode_egm_robot(seqno: int, tm_ms: int, x: float = 0.0, y: float = 0.0,
                     z: float = 0.0) -> bytes:
    """构造一帧 EgmRobot（header + feedBack.cartesian + motorState + mciState）。"""
    cartesian = _double_field(1, x) + _double_field(2, y) + _double_field(3, z)
    orient = (_double_field(1, 1.0) + _double_field(2, 0.0) +
              _double_field(3, 0.0) + _double_field(4, 0.0))
    pose = _length_field(1, cartesian) + _length_field(2, orient)
    feedback = _length_field(2, pose)          # EgmFeedBack.cartesian = 2
    motor = _varint_field(1, 1)                # MOTORS_ON
    mci = _varint_field(1, 3)                  # MCI_RUNNING
    header = (_varint_field(1, seqno) + _varint_field(2, tm_ms) +
              _varint_field(3, MSGTYPE_DATA))
    return (_length_field(1, header) + _length_field(2, feedback) +
            _length_field(4, motor) + _length_field(5, mci))


def encode_sensor_path_corr(seqno: int, tm_ms: int, z: float, age_ms: int) -> bytes:
    """仅供 --self-test 使用，按上位机编码规则生成 EgmSensorPathCorr。"""
    pos = _double_field(1, 0.0) + _double_field(2, 0.0) + _double_field(3, z)
    path_corr = _length_field(1, pos) + _varint_field(2, age_ms)
    header = (_varint_field(1, seqno) + _varint_field(2, tm_ms) +
              _varint_field(3, MSGTYPE_PATH_CORRECTION))
    return _length_field(1, header) + _length_field(2, path_corr)


def _read_varint(data: bytes, offset: int):
    value = 0
    shift = 0
    while True:
        if offset >= len(data):
            raise ValueError("截断的 varint")
        byte = data[offset]
        offset += 1
        value |= (byte & 0x7F) << shift
        if not byte & 0x80:
            return value, offset
        shift += 7
        if shift > 63:
            raise ValueError("varint 过长")


def _iter_fields(data: bytes):
    offset = 0
    while offset < len(data):
        tag, offset = _read_varint(data, offset)
        field, wire = tag >> 3, tag & 0x07
        if field == 0:
            raise ValueError("非法字段号 0")
        if wire == 0:
            value, offset = _read_varint(data, offset)
        elif wire == 1:
            if offset + 8 > len(data):
                raise ValueError("截断的 fixed64")
            value = struct.unpack_from("<d", data, offset)[0]
            offset += 8
        elif wire == 2:
            length, offset = _read_varint(data, offset)
            if offset + length > len(data):
                raise ValueError("截断的 length-delimited")
            value = data[offset:offset + length]
            offset += length
        elif wire == 5:
            if offset + 4 > len(data):
                raise ValueError("截断的 fixed32")
            offset += 4
            value = 0
        else:
            raise ValueError(f"不支持的 wire type {wire}")
        yield field, wire, value


@dataclass
class PathCorrReply:
    seqno: int = 0
    tm_ms: int = 0
    mtype: int = 0
    x: float = 0.0
    y: float = 0.0
    z: float = 0.0
    age_ms: int = 0


def decode_sensor_path_corr(data: bytes) -> PathCorrReply:
    reply = PathCorrReply()
    for field, wire, value in _iter_fields(data):
        if field == 1 and wire == 2:
            for hf, hw, hv in _iter_fields(value):
                if hw != 0:
                    continue
                if hf == 1:
                    reply.seqno = hv
                elif hf == 2:
                    reply.tm_ms = hv
                elif hf == 3:
                    reply.mtype = hv
        elif field == 2 and wire == 2:
            for pf, pw, pv in _iter_fields(value):
                if pf == 1 and pw == 2:
                    for cf, cw, cv in _iter_fields(pv):
                        if cw != 1:
                            continue
                        if cf == 1:
                            reply.x = cv
                        elif cf == 2:
                            reply.y = cv
                        elif cf == 3:
                            reply.z = cv
                elif pf == 2 and pw == 0:
                    reply.age_ms = pv
    return reply


# ---------------------------------------------------------------------------
# 测试报告
# ---------------------------------------------------------------------------
class Report:
    def __init__(self):
        self.items = []

    def check(self, name: str, ok: bool, detail: str = ""):
        self.items.append(ok)
        suffix = f" ({detail})" if detail else ""
        print(f"[{'PASS' if ok else 'FAIL'}] {name}{suffix}")

    def failed(self) -> bool:
        return not all(self.items)


# ---------------------------------------------------------------------------
# 自检：编码结果必须与 C++ 单测黄金字节一致
# ---------------------------------------------------------------------------
def self_test() -> int:
    failures = 0

    robot_hex = encode_egm_robot(7, 336, 500.0, 10.0, 100.0).hex()
    if robot_hex == GOLDEN_ROBOT_HEX:
        print("[PASS] EgmRobot 编码与黄金字节一致")
    else:
        print("[FAIL] EgmRobot 编码不一致")
        print(f"  expected: {GOLDEN_ROBOT_HEX}")
        print(f"  actual  : {robot_hex}")
        failures += 1

    sensor_hex = encode_sensor_path_corr(7, 12345, 1.25, 48).hex()
    if sensor_hex == GOLDEN_SENSOR_HEX:
        print("[PASS] EgmSensorPathCorr 编码与黄金字节一致")
    else:
        print("[FAIL] EgmSensorPathCorr 编码不一致")
        print(f"  expected: {GOLDEN_SENSOR_HEX}")
        print(f"  actual  : {sensor_hex}")
        failures += 1

    decoded = decode_sensor_path_corr(bytes.fromhex(GOLDEN_SENSOR_HEX))
    if (decoded.seqno == 7 and decoded.mtype == MSGTYPE_PATH_CORRECTION and
            abs(decoded.z - 1.25) < 1e-12 and decoded.age_ms == 48):
        print("[PASS] EgmSensorPathCorr 解码正确")
    else:
        print(f"[FAIL] EgmSensorPathCorr 解码错误: {decoded}")
        failures += 1

    return 0 if failures == 0 else 1


# ---------------------------------------------------------------------------
# 主流程
# ---------------------------------------------------------------------------
def parse_args():
    parser = argparse.ArgumentParser(
        description="ABB EGM 路径修正本地模拟控制器（纯上位机自测）")
    parser.add_argument("--host", default="127.0.0.1", help="上位机 IP（默认 127.0.0.1）")
    parser.add_argument("--port", type=int, default=6510, help="上位机 EGM 监听端口（默认 6510）")
    parser.add_argument("--period-ms", type=int, default=48, help="EGM 周期（默认 48 ms）")
    parser.add_argument("--frames", type=int, default=250,
                        help="第一阶段发送帧数；0 表示一直发送直到 Ctrl+C（默认 250）")
    parser.add_argument("--pause-ms", type=int, default=0,
                        help="第一阶段结束后的静默时长，用于 UDP 超时停发测试（默认 0）")
    parser.add_argument("--resume-frames", type=int, default=0,
                        help="静默后继续发送的帧数，用于超时恢复测试（默认 0）")
    parser.add_argument("--drain-ms", type=int, default=200,
                        help="发送结束后继续接收回包的时长（默认 200 ms）")
    parser.add_argument("--receive-timeout-ms", type=int, default=500,
                        help="上位机链路超时阈值，需与 egm_pressure.receive_timeout_ms 一致（默认 500 ms）")
    parser.add_argument("--expect-zero", action="store_true",
                        help="断言所有回包 z 恒为 0（零修正自测）")
    parser.add_argument("--expect-age", type=int, default=-1,
                        help="断言所有回包 age 等于该值（如 48）")
    parser.add_argument("--csv", default="", help="将回包写入 CSV 文件")
    parser.add_argument("--self-test", action="store_true", help="只做编解码自检，不发包")
    return parser.parse_args()


def drain(sock: socket.socket, replies, paused_at=None):
    while True:
        try:
            data, _ = sock.recvfrom(4096)
        except (BlockingIOError, InterruptedError):
            return
        except OSError:
            return
        received_at = time.monotonic()
        try:
            replies.append((received_at, decode_sensor_path_corr(data)))
        except ValueError as exc:
            print(f"[WARN] 忽略无法解析的回包: {exc}")


def send_frames(sock, target, replies, start_seqno, count, period_s, start_time):
    seqno = start_seqno
    next_at = time.monotonic()
    for index in range(count):
        while True:
            now = time.monotonic()
            if now >= next_at:
                break
            drain(sock, replies)
            time.sleep(min(next_at - now, 0.005))
        tm_ms = int((time.monotonic() - start_time) * 1000.0)
        sock.sendto(encode_egm_robot(seqno, tm_ms), target)
        seqno += 1
        next_at += period_s
        drain(sock, replies)
        if (index + 1) % 50 == 0:
            print(f"  已发送 {index + 1} 帧，收到回包 {len(replies)} 帧")
    return seqno


def run_client(args) -> int:
    period_s = max(args.period_ms, 1) / 1000.0
    report = Report()
    replies = []
    t0 = time.monotonic()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", 0))
    sock.setblocking(False)
    target = (args.host, args.port)

    try:
        print(f"目标 {args.host}:{args.port}，周期 {args.period_ms} ms")
        if args.frames == 0:
            print("持续发送模式，按 Ctrl+C 结束。")
            seqno = 0
            try:
                while True:
                    seqno = send_frames(sock, target, replies, seqno, 1, period_s, t0)
            except KeyboardInterrupt:
                print("已手动停止。")
        else:
            send_frames(sock, target, replies, 0, args.frames, period_s, t0)

        # 静默窗口：验证超时后不再回包
        pause_start = None
        pause_silent_after_timeout = 0
        if args.pause_ms > 0:
            pause_start = time.monotonic()
            deadline = pause_start + args.pause_ms / 1000.0
            while time.monotonic() < deadline:
                drain(sock, replies)
                time.sleep(0.005)
            timeout_at = pause_start + (args.receive_timeout_ms + 100) / 1000.0
            pause_silent_after_timeout = sum(
                1 for received_at, _ in replies
                if timeout_at <= received_at <= deadline)

        # 恢复发送：验证链路可恢复
        resume_start = None
        if args.pause_ms > 0 and args.resume_frames > 0:
            resume_start = time.monotonic()
            send_frames(sock, target, replies, args.frames, args.resume_frames,
                        period_s, t0)

        # 收尾排空
        drain_deadline = time.monotonic() + max(args.drain_ms, 0) / 1000.0
        while time.monotonic() < drain_deadline:
            drain(sock, replies)
            time.sleep(0.005)
    finally:
        sock.close()

    total_sent = args.frames + (args.resume_frames if pause_start else 0)
    print(f"发送 {total_sent} 帧，收到回包 {len(replies)} 帧")

    if not replies:
        print("[FAIL] 未收到任何回包")
        print("       请确认 AFP_ABB_CPP.exe 已点击「开始监听」，且端口/防火墙正确。")
        return 1

    frames = [reply for _, reply in replies]
    report.check("回包数量与发送帧数一致", len(frames) == total_sent,
                 f"sent={total_sent}, received={len(frames)}")
    report.check("mtype 全部为 MSGTYPE_PATH_CORRECTION",
                 all(f.mtype == MSGTYPE_PATH_CORRECTION for f in frames))
    report.check("pathCorr.pos.x/y 恒为 0",
                 all(f.x == 0.0 and f.y == 0.0 for f in frames))

    max_abs_z = max(abs(f.z) for f in frames)
    if args.expect_zero:
        report.check("零修正回包 z 恒为 0", max_abs_z == 0.0, f"max|z|={max_abs_z:.6f}")
    else:
        print(f"[INFO] max|z|={max_abs_z:.6f} mm, max|Δz|="
              f"{max(abs(b.z - a.z) for a, b in zip(frames, frames[1:])) if len(frames) > 1 else 0.0:.6f} mm")

    if args.expect_age >= 0:
        report.check(f"回包 age 恒为 {args.expect_age}",
                     all(f.age_ms == args.expect_age for f in frames),
                     f"unique={sorted({f.age_ms for f in frames})}")

    seq_gaps = sum(1 for a, b in zip(frames, frames[1:])
                   if b.seqno != a.seqno + 1)
    report.check("回包 seqno 每帧严格 +1", seq_gaps == 0,
                 f"gaps={seq_gaps}, first={frames[0].seqno}, last={frames[-1].seqno}")

    if pause_start is not None:
        report.check("静默超过 receive_timeout 后不再回包",
                     pause_silent_after_timeout == 0,
                     f"count={pause_silent_after_timeout}")
        if resume_start is not None:
            resumed_replies = sum(1 for received_at, _ in replies
                                  if received_at >= resume_start)
            report.check("恢复发送后重新收到回包", resumed_replies > 0,
                         f"count={resumed_replies}")

    if args.csv:
        try:
            with open(args.csv, "w", newline="", encoding="utf-8") as handle:
                writer = csv.writer(handle)
                writer.writerow(["received_unix_ms", "seqno", "tm_ms", "mtype",
                                 "x", "y", "z", "age_ms"])
                for received_at, reply in replies:
                    writer.writerow([int(received_at * 1000), reply.seqno, reply.tm_ms,
                                     reply.mtype, reply.x, reply.y, reply.z,
                                     reply.age_ms])
            print(f"回包已写入 {args.csv}")
        except OSError as exc:
            print(f"[WARN] 写 CSV 失败: {exc}")

    ok = not report.failed()
    print("RESULT:", "PASS" if ok else "FAIL")
    return 0 if ok else 1


def main() -> int:
    args = parse_args()
    if args.self_test:
        return self_test()
    return run_client(args)


if __name__ == "__main__":
    sys.exit(main())
