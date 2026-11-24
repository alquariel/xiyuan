#include "egm/EgmWireCodec.h"

#include <cstring>

namespace egm {
namespace {

// ---- protobuf 基础类型 ----
constexpr quint32 kWireVarint = 0;
constexpr quint32 kWireFixed64 = 1;
constexpr quint32 kWireLengthDelimited = 2;
constexpr quint32 kWireFixed32 = 5;

void appendVarint(std::string& out, quint64 value) {
    while (value >= 0x80U) {
        out.push_back(static_cast<char>((value & 0x7FU) | 0x80U));
        value >>= 7U;
    }
    out.push_back(static_cast<char>(value));
}

void appendTag(std::string& out, quint32 fieldNumber, quint32 wireType) {
    appendVarint(out, (static_cast<quint64>(fieldNumber) << 3U) | wireType);
}

void appendLengthDelimited(std::string& out, quint32 fieldNumber, const std::string& payload) {
    appendTag(out, fieldNumber, kWireLengthDelimited);
    appendVarint(out, payload.size());
    out.append(payload);
}

void appendDouble(std::string& out, quint32 fieldNumber, double value) {
    appendTag(out, fieldNumber, kWireFixed64);
    char bytes[sizeof(double)];
    std::memcpy(bytes, &value, sizeof(double));
    out.append(bytes, sizeof(double));
}

void appendVarintField(std::string& out, quint32 fieldNumber, quint64 value) {
    appendTag(out, fieldNumber, kWireVarint);
    appendVarint(out, value);
}

// ---- 读取器 ----
class Reader {
public:
    Reader(const char* data, std::size_t size)
        : cursor_(reinterpret_cast<const quint8*>(data)),
          end_(reinterpret_cast<const quint8*>(data) + size) {
    }

    bool atEnd() const { return cursor_ >= end_; }

    bool readVarint(quint64& value) {
        value = 0;
        int shift = 0;
        while (cursor_ < end_ && shift < 64) {
            const quint8 byte = *cursor_++;
            value |= static_cast<quint64>(byte & 0x7FU) << shift;
            if ((byte & 0x80U) == 0) {
                return true;
            }
            shift += 7;
        }
        return false;
    }

    bool readTag(quint32& fieldNumber, quint32& wireType) {
        quint64 tag = 0;
        if (!readVarint(tag)) {
            return false;
        }
        fieldNumber = static_cast<quint32>(tag >> 3U);
        wireType = static_cast<quint32>(tag & 0x7U);
        return fieldNumber != 0;
    }

    bool readDouble(double& value) {
        if (end_ - cursor_ < static_cast<std::ptrdiff_t>(sizeof(double))) {
            return false;
        }
        std::memcpy(&value, cursor_, sizeof(double));
        cursor_ += sizeof(double);
        return true;
    }

    bool readSubMessage(const char*& data, std::size_t& size) {
        quint64 length = 0;
        if (!readVarint(length)) {
            return false;
        }
        if (length > static_cast<quint64>(end_ - cursor_)) {
            return false;
        }
        data = reinterpret_cast<const char*>(cursor_);
        size = static_cast<std::size_t>(length);
        cursor_ += length;
        return true;
    }

    bool skip(quint32 wireType) {
        switch (wireType) {
            case kWireVarint: {
                quint64 ignored = 0;
                return readVarint(ignored);
            }
            case kWireFixed64:
                if (end_ - cursor_ < 8) {
                    return false;
                }
                cursor_ += 8;
                return true;
            case kWireLengthDelimited: {
                const char* ignoredData = nullptr;
                std::size_t ignoredSize = 0;
                return readSubMessage(ignoredData, ignoredSize);
            }
            case kWireFixed32:
                if (end_ - cursor_ < 4) {
                    return false;
                }
                cursor_ += 4;
                return true;
            default:
                return false;
        }
    }

private:
    const quint8* cursor_ = nullptr;
    const quint8* end_ = nullptr;
};

bool parseCartesian(Reader& reader, EgmRobotFrame& frame) {
    while (!reader.atEnd()) {
        quint32 field = 0;
        quint32 wire = 0;
        if (!reader.readTag(field, wire)) {
            return false;
        }
        if (wire == kWireFixed64 && (field == 1 || field == 2 || field == 3)) {
            double value = 0.0;
            if (!reader.readDouble(value)) {
                return false;
            }
            if (field == 1) {
                frame.fbX = value;
            } else if (field == 2) {
                frame.fbY = value;
            } else {
                frame.fbZ = value;
            }
        } else if (!reader.skip(wire)) {
            return false;
        }
    }
    return true;
}

bool parseQuaternion(Reader& reader, EgmRobotFrame& frame) {
    while (!reader.atEnd()) {
        quint32 field = 0;
        quint32 wire = 0;
        if (!reader.readTag(field, wire)) {
            return false;
        }
        if (wire == kWireFixed64 && field >= 1 && field <= 4U) {
            double value = 0.0;
            if (!reader.readDouble(value)) {
                return false;
            }
            switch (field) {
                case 1: frame.fbQ1 = value; break;
                case 2: frame.fbQ2 = value; break;
                case 3: frame.fbQ3 = value; break;
                default: frame.fbQ4 = value; break;
            }
        } else if (!reader.skip(wire)) {
            return false;
        }
    }
    return true;
}

bool parsePose(Reader& reader, EgmRobotFrame& frame) {
    while (!reader.atEnd()) {
        quint32 field = 0;
        quint32 wire = 0;
        if (!reader.readTag(field, wire)) {
            return false;
        }
        if (wire != kWireLengthDelimited) {
            if (!reader.skip(wire)) {
                return false;
            }
            continue;
        }
        const char* data = nullptr;
        std::size_t size = 0;
        if (!reader.readSubMessage(data, size)) {
            return false;
        }
        Reader nested(data, size);
        if (field == 1) {
            if (!parseCartesian(nested, frame)) {
                return false;
            }
        } else if (field == 2) {
            if (!parseQuaternion(nested, frame)) {
                return false;
            }
        }
    }
    return true;
}

bool parseFeedback(Reader& reader, EgmRobotFrame& frame) {
    while (!reader.atEnd()) {
        quint32 field = 0;
        quint32 wire = 0;
        if (!reader.readTag(field, wire)) {
            return false;
        }
        if (wire != kWireLengthDelimited) {
            if (!reader.skip(wire)) {
                return false;
            }
            continue;
        }
        const char* data = nullptr;
        std::size_t size = 0;
        if (!reader.readSubMessage(data, size)) {
            return false;
        }
        if (field == 2) {
            Reader nested(data, size);
            if (!parsePose(nested, frame)) {
                return false;
            }
            frame.feedbackValid = true;
        }
    }
    return true;
}

bool parseHeader(Reader& reader, EgmRobotFrame& frame) {
    while (!reader.atEnd()) {
        quint32 field = 0;
        quint32 wire = 0;
        if (!reader.readTag(field, wire)) {
            return false;
        }
        if (wire == kWireVarint && (field == 1 || field == 2)) {
            quint64 value = 0;
            if (!reader.readVarint(value)) {
                return false;
            }
            if (field == 1) {
                frame.seqno = static_cast<quint32>(value);
            } else {
                frame.controllerTimeMs = static_cast<quint32>(value);
            }
        } else if (!reader.skip(wire)) {
            return false;
        }
    }
    return true;
}

bool parseMotorState(Reader& reader, EgmRobotFrame& frame) {
    while (!reader.atEnd()) {
        quint32 field = 0;
        quint32 wire = 0;
        if (!reader.readTag(field, wire)) {
            return false;
        }
        if (wire == kWireVarint && field == 1) {
            quint64 value = 0;
            if (!reader.readVarint(value)) {
                return false;
            }
            frame.motorsOn = value == 1;  // EgmMotorState::MOTORS_ON
        } else if (!reader.skip(wire)) {
            return false;
        }
    }
    return true;
}

bool parseMciState(Reader& reader, EgmRobotFrame& frame) {
    while (!reader.atEnd()) {
        quint32 field = 0;
        quint32 wire = 0;
        if (!reader.readTag(field, wire)) {
            return false;
        }
        if (wire == kWireVarint && field == 1) {
            quint64 value = 0;
            if (!reader.readVarint(value)) {
                return false;
            }
            frame.mciState = static_cast<int>(value);
        } else if (!reader.skip(wire)) {
            return false;
        }
    }
    return true;
}

}  // namespace

std::string encodeSensorPathCorr(quint32 seqno, quint32 tmMs, double zMm, quint32 ageMs) {
    // EgmCartesian: proto2 required double x/y/z，按官方生成代码总是序列化。
    std::string pos;
    appendDouble(pos, 1, 0.0);
    appendDouble(pos, 2, 0.0);
    appendDouble(pos, 3, zMm);

    // EgmPathCorr: required pos + required age。
    std::string pathCorr;
    appendLengthDelimited(pathCorr, 1, pos);
    appendVarintField(pathCorr, 2, ageMs);

    // EgmHeader: seqno / tm / mtype。
    std::string header;
    appendVarintField(header, 1, seqno);
    appendVarintField(header, 2, tmMs);
    appendVarintField(header, 3, 4);  // EgmHeader::MSGTYPE_PATH_CORRECTION

    std::string sensor;
    appendLengthDelimited(sensor, 1, header);
    appendLengthDelimited(sensor, 2, pathCorr);
    return sensor;
}

bool decodeEgmRobot(const char* data, std::size_t size, EgmRobotFrame* frame) {
    if (data == nullptr || frame == nullptr) {
        return false;
    }

    EgmRobotFrame parsed;
    Reader reader(data, size);
    while (!reader.atEnd()) {
        quint32 field = 0;
        quint32 wire = 0;
        if (!reader.readTag(field, wire)) {
            return false;
        }
        if (wire != kWireLengthDelimited) {
            if (!reader.skip(wire)) {
                return false;
            }
            continue;
        }
        const char* subData = nullptr;
        std::size_t subSize = 0;
        if (!reader.readSubMessage(subData, subSize)) {
            return false;
        }
        Reader nested(subData, subSize);
        switch (field) {
            case 1:  // EgmRobot.header
                if (!parseHeader(nested, parsed)) {
                    return false;
                }
                break;
            case 2:  // EgmRobot.feedBack
                if (!parseFeedback(nested, parsed)) {
                    return false;
                }
                break;
            case 4:  // EgmRobot.motorState
                if (!parseMotorState(nested, parsed)) {
                    return false;
                }
                break;
            case 5:  // EgmRobot.mciState
                if (!parseMciState(nested, parsed)) {
                    return false;
                }
                break;
            default:
                break;  // 其余字段（planned / testSignals 等）本模块不使用
        }
    }

    *frame = parsed;
    return true;
}

bool decodeSensorPathCorr(const char* data, std::size_t size, SensorPathCorrFrame* frame) {
    if (data == nullptr || frame == nullptr) {
        return false;
    }

    SensorPathCorrFrame parsed;
    Reader reader(data, size);
    while (!reader.atEnd()) {
        quint32 field = 0;
        quint32 wire = 0;
        if (!reader.readTag(field, wire)) {
            return false;
        }
        if (wire != kWireLengthDelimited) {
            if (!reader.skip(wire)) {
                return false;
            }
            continue;
        }
        const char* subData = nullptr;
        std::size_t subSize = 0;
        if (!reader.readSubMessage(subData, subSize)) {
            return false;
        }

        if (field == 1) {
            Reader header(subData, subSize);
            while (!header.atEnd()) {
                quint32 headerField = 0;
                quint32 headerWire = 0;
                if (!header.readTag(headerField, headerWire)) {
                    return false;
                }
                if (headerWire == kWireVarint && headerField >= 1 && headerField <= 3) {
                    quint64 value = 0;
                    if (!header.readVarint(value)) {
                        return false;
                    }
                    if (headerField == 1) {
                        parsed.seqno = static_cast<quint32>(value);
                    } else if (headerField == 2) {
                        parsed.tmMs = static_cast<quint32>(value);
                    } else {
                        parsed.mtype = static_cast<quint32>(value);
                    }
                } else if (!header.skip(headerWire)) {
                    return false;
                }
            }
        } else if (field == 2) {
            Reader pathCorr(subData, subSize);
            while (!pathCorr.atEnd()) {
                quint32 pcField = 0;
                quint32 pcWire = 0;
                if (!pathCorr.readTag(pcField, pcWire)) {
                    return false;
                }
                if (pcField == 2 && pcWire == kWireVarint) {
                    quint64 value = 0;
                    if (!pathCorr.readVarint(value)) {
                        return false;
                    }
                    parsed.ageMs = static_cast<quint32>(value);
                } else if (pcField == 1 && pcWire == kWireLengthDelimited) {
                    const char* posData = nullptr;
                    std::size_t posSize = 0;
                    if (!pathCorr.readSubMessage(posData, posSize)) {
                        return false;
                    }
                    Reader pos(posData, posSize);
                    while (!pos.atEnd()) {
                        quint32 posField = 0;
                        quint32 posWire = 0;
                        if (!pos.readTag(posField, posWire)) {
                            return false;
                        }
                        if (posWire == kWireFixed64 && posField >= 1 && posField <= 3) {
                            double value = 0.0;
                            if (!pos.readDouble(value)) {
                                return false;
                            }
                            if (posField == 1) {
                                parsed.x = value;
                            } else if (posField == 2) {
                                parsed.y = value;
                            } else {
                                parsed.z = value;
                            }
                        } else if (!pos.skip(posWire)) {
                            return false;
                        }
                    }
                } else if (!pathCorr.skip(pcWire)) {
                    return false;
                }
            }
        }
    }

    *frame = parsed;
    return true;
}

}  // namespace egm
