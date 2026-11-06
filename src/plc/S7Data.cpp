#include "plc/S7Data.h"

#include <cstring>

namespace {

// S7 bit indexes are addressed from the least significant bit of the byte.
quint8 bitMask(int bitIndex) {
    return static_cast<quint8>(1u << bitIndex);
}

}

namespace S7Data {

bool bitIsSet(quint8 value, int bitIndex) {
    return (value & bitMask(bitIndex)) != 0;
}

void setBit(quint8& value, int bitIndex, bool enabled) {
    if (enabled) {
        value = static_cast<quint8>(value | bitMask(bitIndex));
    } else {
        value = static_cast<quint8>(value & ~bitMask(bitIndex));
    }
}

std::array<std::uint8_t, 2> encodeInt16(std::int16_t value) {
    // Cast through uint16_t so negative INT values keep their two's-complement
    // representation while we split the bytes.
    const auto unsignedValue = static_cast<std::uint16_t>(value);
    return {
        static_cast<std::uint8_t>((unsignedValue >> 8) & 0xFF),
        static_cast<std::uint8_t>(unsignedValue & 0xFF)
    };
}

std::int16_t decodeInt16(const std::array<std::uint8_t, 2>& bytes) {
    const std::uint16_t combined = static_cast<std::uint16_t>(
        (static_cast<std::uint16_t>(bytes[0]) << 8) |
        static_cast<std::uint16_t>(bytes[1]));
    return static_cast<std::int16_t>(combined);
}

std::array<std::uint8_t, 4> encodeReal32(float value) {
    // memcpy avoids strict-aliasing problems when reading the float bit pattern.
    std::uint32_t raw = 0;
    std::memcpy(&raw, &value, sizeof(value));
    return {
        static_cast<std::uint8_t>((raw >> 24) & 0xFF),
        static_cast<std::uint8_t>((raw >> 16) & 0xFF),
        static_cast<std::uint8_t>((raw >> 8) & 0xFF),
        static_cast<std::uint8_t>(raw & 0xFF)
    };
}

float decodeReal32(const std::array<std::uint8_t, 4>& bytes) {
    const std::uint32_t raw =
        (static_cast<std::uint32_t>(bytes[0]) << 24) |
        (static_cast<std::uint32_t>(bytes[1]) << 16) |
        (static_cast<std::uint32_t>(bytes[2]) << 8) |
        static_cast<std::uint32_t>(bytes[3]);

    float value = 0.0f;
    std::memcpy(&value, &raw, sizeof(value));
    return value;
}

std::array<std::uint8_t, 4> encodeInt32LittleEndian(qint32 value) {
    const auto raw = static_cast<quint32>(value);
    return {
        static_cast<std::uint8_t>(raw & 0xFF),
        static_cast<std::uint8_t>((raw >> 8) & 0xFF),
        static_cast<std::uint8_t>((raw >> 16) & 0xFF),
        static_cast<std::uint8_t>((raw >> 24) & 0xFF),
    };
}

qint32 decodeInt32LittleEndian(const std::array<std::uint8_t, 4>& data) {
    const quint32 raw = static_cast<quint32>(data[0])
        | (static_cast<quint32>(data[1]) << 8)
        | (static_cast<quint32>(data[2]) << 16)
        | (static_cast<quint32>(data[3]) << 24);
    return static_cast<qint32>(raw);
}

}
