#pragma once

#include <array>
#include <cstdint>

#include <QtGlobal>

// Helpers for Siemens S7 memory values.
//
// The PLC stores multi-byte numeric values in big-endian order. Keeping this
// logic in one small module prevents each PLC command from reimplementing byte
// packing by hand.
namespace S7Data {

// Return true when bitIndex 0..7 is set in a PLC byte.
bool bitIsSet(quint8 value, int bitIndex);

// Mutate one bit in a PLC byte while leaving the other seven bits unchanged.
void setBit(quint8& value, int bitIndex, bool enabled);

// Encode and decode S7 INT values. S7 INT is a signed 16-bit big-endian value.
std::array<std::uint8_t, 2> encodeInt16(std::int16_t value);
std::int16_t decodeInt16(const std::array<std::uint8_t, 2>& bytes);

// Encode and decode S7 REAL values. S7 REAL is an IEEE-754 float stored as
// four bytes in big-endian order.
std::array<std::uint8_t, 4> encodeReal32(float value);
float decodeReal32(const std::array<std::uint8_t, 4>& bytes);

// Python robot movement code packs process-image DINT values little-endian.
// Keep this separate from the S7 DB helpers above, which use big-endian order.
std::array<std::uint8_t, 4> encodeInt32LittleEndian(qint32 value);
qint32 decodeInt32LittleEndian(const std::array<std::uint8_t, 4>& data);

}
