#pragma once

#include <cstddef>
#include <string>

#include <QtGlobal>

namespace egm {

// 控制器侧 EgmRobot 报文的关键字段副本，供控制律与界面使用。
struct EgmRobotFrame {
    quint32 seqno = 0;
    quint32 controllerTimeMs = 0;
    int mciState = 0;                 // 0 undefined / 1 error / 2 stopped / 3 running
    bool motorsOn = false;
    bool feedbackValid = false;
    double fbX = 0.0;
    double fbY = 0.0;
    double fbZ = 0.0;
    double fbQ1 = 1.0;
    double fbQ2 = 0.0;
    double fbQ3 = 0.0;
    double fbQ4 = 0.0;
};

// ABB egm.proto 的最小 protobuf wire 编解码。
//
// 只覆盖本模块实际使用的两个报文：
//   - 编码 EgmSensorPathCorr（header + pathCorr.pos + age），字段与官方
//     third_party/egm/egm.proto 完全一致，字节级兼容 Python egm_pb2；
//   - 解码 EgmRobot 的 header / feedBack.cartesian / motorState / mciState。
//
// 不引入 protobuf 运行时：现场构建机没有 Visual Studio，vcpkg 无法构建
// protoc 宿主工具；报文字段固定且很少，自带编解码更易部署与测试。
std::string encodeSensorPathCorr(quint32 seqno, quint32 tmMs, double zMm, quint32 ageMs);

// 解析失败（截断、非法 wire type）返回 false，且不保证 frame 内容。
bool decodeEgmRobot(const char* data, std::size_t size, EgmRobotFrame* frame);

// 供本地回环测试与诊断使用：解析本模块发往控制器的 EgmSensorPathCorr。
struct SensorPathCorrFrame {
    quint32 seqno = 0;
    quint32 tmMs = 0;
    quint32 mtype = 0;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    quint32 ageMs = 0;
};

bool decodeSensorPathCorr(const char* data, std::size_t size, SensorPathCorrFrame* frame);

}  // namespace egm
