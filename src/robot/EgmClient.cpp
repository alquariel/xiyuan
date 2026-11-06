#include "robot/EgmClient.h"

#include <QHostAddress>
#include <QUdpSocket>

#include <chrono>
#include <cstring>
#include <vector>

namespace {

// ===== Protobuf 编码（只覆盖 EGM 需要的字节）=====
// protobuf wire format: tag = (field << 3) | wire_type
//   wire 0 = varint, wire 1 = fixed64(double), wire 2 = length-delimited
//   wire 5 = fixed32

void AppendVarint(QByteArray& out, std::uint64_t value) {
    while (value >= 0x80u) {
        const std::uint8_t byte = static_cast<std::uint8_t>(value & 0x7Fu) | 0x80u;
        out.append(static_cast<char>(byte));
        value >>= 7;
    }
    out.append(static_cast<char>(static_cast<std::uint8_t>(value)));
}

void AppendTag(QByteArray& out, int field, int wireType) {
    AppendVarint(out, (static_cast<std::uint64_t>(field) << 3) |
                          static_cast<std::uint64_t>(wireType));
}

void AppendVarintField(QByteArray& out, int field, std::uint64_t value) {
    AppendTag(out, field, 0);
    AppendVarint(out, value);
}

void AppendDoubleField(QByteArray& out, int field, double value) {
    AppendTag(out, field, 1);
    // protobuf fixed64 是小端
    std::uint64_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    for (int i = 0; i < 8; ++i) {
        out.append(static_cast<char>(static_cast<std::uint8_t>(bits >> (8 * i))));
    }
}

void AppendMessageField(QByteArray& out, int field, const QByteArray& message) {
    AppendTag(out, field, 2);
    AppendVarint(out, static_cast<std::uint64_t>(message.size()));
    out.append(message);
}

// ===== Protobuf 解码（只取 EgmRobot.feedBack.cartesian）=====

bool ReadVarint(const QByteArray& data, int* offset, std::uint64_t* value) {
    std::uint64_t result = 0;
    int shift = 0;
    while (*offset < data.size() && shift < 64) {
        const std::uint8_t byte = static_cast<std::uint8_t>(data.at(*offset));
        ++(*offset);
        result |= static_cast<std::uint64_t>(byte & 0x7Fu) << shift;
        if ((byte & 0x80u) == 0) {
            *value = result;
            return true;
        }
        shift += 7;
    }
    return false;
}

struct PbField {
    int field = 0;
    int wire = 0;
    std::uint64_t varint = 0;
    double number = 0.0;
    QByteArray payload;
};

// 把一帧 protobuf 拆成字段列表；遇到无法识别的 wire type 直接停止。
std::vector<PbField> ParseFields(const QByteArray& data) {
    std::vector<PbField> fields;
    int offset = 0;
    while (offset < data.size()) {
        std::uint64_t tag = 0;
        if (!ReadVarint(data, &offset, &tag) || tag == 0) break;

        PbField item;
        item.field = static_cast<int>(tag >> 3);
        item.wire = static_cast<int>(tag & 0x07u);

        if (item.wire == 0) {
            if (!ReadVarint(data, &offset, &item.varint)) break;
        } else if (item.wire == 1) {
            if (offset + 8 > data.size()) break;
            std::uint64_t bits = 0;
            for (int i = 0; i < 8; ++i) {
                bits |= static_cast<std::uint64_t>(
                            static_cast<std::uint8_t>(data.at(offset + i)))
                        << (8 * i);
            }
            std::memcpy(&item.number, &bits, sizeof(item.number));
            offset += 8;
        } else if (item.wire == 2) {
            std::uint64_t length = 0;
            if (!ReadVarint(data, &offset, &length)) break;
            const int len = static_cast<int>(length);
            if (len < 0 || offset + len > data.size()) break;
            item.payload = data.mid(offset, len);
            offset += len;
        } else if (item.wire == 5) {
            if (offset + 4 > data.size()) break;
            offset += 4;
        } else {
            break;  // 未知 wire type，放弃解析
        }
        fields.push_back(item);
    }
    return fields;
}

const PbField* FindField(const std::vector<PbField>& fields, int field, int wire) {
    for (const PbField& f : fields) {
        if (f.field == field && f.wire == wire) return &f;
    }
    return nullptr;
}

// 解析 EgmPose（pos=1, orient=2）到 EgmTcpPose；缺字段时保留原值。
bool ParseEgmPose(const QByteArray& data, EgmTcpPose* pose) {
    if (pose == nullptr) return false;
    bool any = false;

    for (const PbField& f : ParseFields(data)) {
        if (f.field == 1 && f.wire == 2) {  // pos
            for (const PbField& c : ParseFields(f.payload)) {
                if (c.wire != 1) continue;
                if (c.field == 1) { pose->x = c.number; any = true; }
                if (c.field == 2) { pose->y = c.number; any = true; }
                if (c.field == 3) { pose->z = c.number; any = true; }
            }
        } else if (f.field == 2 && f.wire == 2) {  // orient
            for (const PbField& q : ParseFields(f.payload)) {
                if (q.wire != 1) continue;
                if (q.field == 1) { pose->q1 = q.number; any = true; }
                if (q.field == 2) { pose->q2 = q.number; any = true; }
                if (q.field == 3) { pose->q3 = q.number; any = true; }
                if (q.field == 4) { pose->q4 = q.number; any = true; }
            }
        }
    }
    return any;
}

// EgmRobot.feedBack(2).cartesian(2)
bool ParseRobotFeedbackCartesian(const QByteArray& data, EgmTcpPose* pose) {
    for (const PbField& f : ParseFields(data)) {
        if (f.field != 2 || f.wire != 2) continue;  // feedBack
        for (const PbField& inner : ParseFields(f.payload)) {
            if (inner.field == 2 && inner.wire == 2) {  // cartesian
                return ParseEgmPose(inner.payload, pose);
            }
        }
    }
    return false;
}

// ===== EgmSensor 组包 =====
// EgmSensor { header{seqno, mtype=CORRECTION} planned{ cartesian{ pos{xyz}, orient{u0..u3} } } }
QByteArray BuildSensorMessage(const EgmTcpPose& pose, std::uint32_t sequenceNumber) {
    QByteArray position;
    AppendDoubleField(position, 1, pose.x);
    AppendDoubleField(position, 2, pose.y);
    AppendDoubleField(position, 3, pose.z);

    QByteArray orientation;
    AppendDoubleField(orientation, 1, pose.q1);
    AppendDoubleField(orientation, 2, pose.q2);
    AppendDoubleField(orientation, 3, pose.q3);
    AppendDoubleField(orientation, 4, pose.q4);

    QByteArray cartesian;
    AppendMessageField(cartesian, 1, position);
    AppendMessageField(cartesian, 2, orientation);

    QByteArray planned;
    AppendMessageField(planned, 2, cartesian);

    QByteArray header;
    AppendVarintField(header, 1, sequenceNumber);  // seqno
    AppendVarintField(header, 3, 3u);              // mtype = MSGTYPE_CORRECTION

    QByteArray sensor;
    AppendMessageField(sensor, 1, header);
    AppendMessageField(sensor, 2, planned);
    return sensor;
}

// ===== EgmSensor 组包（关节模式）=====
// EgmPlanned { joints = 1, cartesian = 2 }，EgmJoints { repeated double joints = 1 }。
// 单位：度（DEG）。ABB EGM 关节通道收发的就是关节角（度），不是弧度。
// repeated double 在 proto2 里默认不打包，这里逐条写入 field 1。
QByteArray BuildJointSensorMessage(const double jointsDeg[6], std::uint32_t sequenceNumber) {
    QByteArray jointList;
    for (int axis = 0; axis < 6; ++axis) {
        AppendDoubleField(jointList, 1, jointsDeg[axis]);
    }

    QByteArray planned;
    AppendMessageField(planned, 1, jointList);

    QByteArray header;
    AppendVarintField(header, 1, sequenceNumber);
    AppendVarintField(header, 3, 3u);   // mtype = MSGTYPE_CORRECTION

    QByteArray sensor;
    AppendMessageField(sensor, 1, header);
    AppendMessageField(sensor, 2, planned);
    return sensor;
}

}  // namespace

EgmClient::EgmClient(EgmConfig config, QObject* parent)
    : QObject(parent),
      config_(std::move(config)) {}

EgmClient::~EgmClient() {
    stop();
}

bool EgmClient::isStreaming() const {
    return running_.load();
}

bool EgmClient::isPeerConnected() const {
    return peerConnected_.load();
}

quint16 EgmClient::boundPort() const {
    return boundPort_.load();
}

bool EgmClient::start() {
    if (!config_.enabled) {
        return false;
    }
    if (config_.periodMs <= 0) {
        return false;
    }
    if (running_.load()) {
        return true;  // 已经在跑
    }

    stopRequested_.store(false);
    peerConnected_.store(false);
    packetsSent_.store(0);
    boundPort_.store(0);
    running_.store(true);

    worker_ = std::thread([this] { runStreamingLoop(); });
    emit streamingStateChanged(true);
    return true;
}

void EgmClient::stop() {
    if (!running_.load()) return;

    stopRequested_.store(true);
    if (worker_.joinable()) {
        worker_.join();
    }
    running_.store(false);
    if (peerConnected_.exchange(false)) {
        emit peerStateChanged(false);
    }
    emit streamingStateChanged(false);
}

void EgmClient::setTargetPose(const EgmTcpPose& pose) {
    std::lock_guard<std::mutex> guard(poseMutex_);
    currentPose_ = pose;
}

void EgmClient::setMode(Mode mode) {
    std::lock_guard<std::mutex> guard(poseMutex_);
    mode_ = mode;
}

EgmClient::Mode EgmClient::mode() const {
    std::lock_guard<std::mutex> guard(poseMutex_);
    return mode_;
}

void EgmClient::setTargetJoints(const double jointsDeg[6]) {
    std::lock_guard<std::mutex> guard(poseMutex_);
    for (int axis = 0; axis < 6; ++axis) {
        currentJoints_[axis] = jointsDeg[axis];
    }
}

void EgmClient::runStreamingLoop() {
    QUdpSocket socket;
    // UdpUc 的 RemotePortNumber 就是本机监听端口（SIO.cfg 里配的 6510）。
    if (!socket.bind(QHostAddress::AnyIPv4, static_cast<quint16>(config_.port))) {
        running_.store(false);
        emit streamingStateChanged(false);
        return;
    }
    boundPort_.store(socket.localPort());

    const std::chrono::milliseconds period{config_.periodMs};

    auto nextTick = std::chrono::steady_clock::now();
    auto lastHeartbeat = nextTick;
    std::uint32_t sequence = 0;

    QHostAddress peerAddress;
    quint16 peerPort = 0;

    while (!stopRequested_.load()) {
        // 1) 收取控制器周期发来的 EgmRobot，记住源地址/端口
        //    用非阻塞 readDatagram 直接读空缓冲（不依赖 Qt 事件循环）
        for (;;) {
            QByteArray incoming(1024, '\0');
            QHostAddress sender;
            quint16 senderPort = 0;
            const qint64 size = socket.readDatagram(incoming.data(), incoming.size(),
                                                    &sender, &senderPort);
            if (size <= 0) break;
            incoming.truncate(static_cast<int>(size));
            if (senderPort == 0) continue;

            peerAddress = sender;
            peerPort = senderPort;
            if (!peerConnected_.exchange(true)) {
                emit peerStateChanged(true);
            }
            ParseRobotFeedbackCartesian(incoming, nullptr);  // 仅作连通性判断
        }

        // 2) 已建立对端后，把当前目标位姿回发（绝对位姿，mm + 四元数）
        if (peerPort != 0) {
            EgmTcpPose pose;
            double joints[6] = {0, 0, 0, 0, 0, 0};
            Mode mode = Mode::Pose;
            {
                std::lock_guard<std::mutex> guard(poseMutex_);
                pose = currentPose_;
                mode = mode_;
                for (int axis = 0; axis < 6; ++axis) {
                    joints[axis] = currentJoints_[axis];
                }
            }
            const QByteArray payload = (mode == Mode::Joint)
                                           ? BuildJointSensorMessage(joints, sequence)
                                           : BuildSensorMessage(pose, sequence);
            socket.writeDatagram(payload, peerAddress, peerPort);
            ++sequence;
            ++packetsSent_;
        }

        // 3) 250 Hz 节拍（steady_clock 累加，避免长期漂移）
        nextTick += period;
        std::this_thread::sleep_until(nextTick);

        // 4) 每秒一次 UI 心跳
        const auto now = std::chrono::steady_clock::now();
        if (now - lastHeartbeat >= std::chrono::seconds(1)) {
            emit packetCounterTick(packetsSent_.load());
            lastHeartbeat = now;
        }
    }

    socket.close();
}
