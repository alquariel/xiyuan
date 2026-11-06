#include <QtTest/QtTest>

#include <array>
#include <cstdint>

#include "plc/S7Data.h"

class TestS7Data final : public QObject {
    Q_OBJECT

private slots:
    void setBitSetsOnlyRequestedBit();
    void encodeDecodeInt16UsesBigEndianOrder();
    void encodeDecodeReal32UsesBigEndianOrder();
    void int32LittleEndianRoundTrip();
    void int32LittleEndianMatchesPythonPacking();
};

void TestS7Data::setBitSetsOnlyRequestedBit() {
    // PLC bit writes must preserve neighboring command bits in the same byte.
    quint8 value = 0x00;
    S7Data::setBit(value, 5, true);
    QCOMPARE(value, static_cast<quint8>(0x20));

    S7Data::setBit(value, 1, true);
    QCOMPARE(value, static_cast<quint8>(0x22));

    S7Data::setBit(value, 5, false);
    QCOMPARE(value, static_cast<quint8>(0x02));
}

void TestS7Data::encodeDecodeInt16UsesBigEndianOrder() {
    // Siemens S7 INT values are stored most-significant byte first.
    const auto bytes = S7Data::encodeInt16(1025);
    QCOMPARE(bytes[0], static_cast<std::uint8_t>(0x04));
    QCOMPARE(bytes[1], static_cast<std::uint8_t>(0x01));
    QCOMPARE(S7Data::decodeInt16(bytes), static_cast<std::int16_t>(1025));
}

void TestS7Data::encodeDecodeReal32UsesBigEndianOrder() {
    // 12.5f has a stable IEEE-754 representation: 0x41480000.
    const auto bytes = S7Data::encodeReal32(12.5f);
    QCOMPARE(bytes[0], static_cast<std::uint8_t>(0x41));
    QCOMPARE(bytes[1], static_cast<std::uint8_t>(0x48));
    QCOMPARE(bytes[2], static_cast<std::uint8_t>(0x00));
    QCOMPARE(bytes[3], static_cast<std::uint8_t>(0x00));
    QCOMPARE(S7Data::decodeReal32(bytes), 12.5f);
}

void TestS7Data::int32LittleEndianRoundTrip() {
    const auto encoded = S7Data::encodeInt32LittleEndian(-1900000);
    QCOMPARE(S7Data::decodeInt32LittleEndian(encoded), qint32(-1900000));
}

void TestS7Data::int32LittleEndianMatchesPythonPacking() {
    const auto encoded = S7Data::encodeInt32LittleEndian(120000);
    QCOMPARE(encoded[0], static_cast<std::uint8_t>(0xC0));
    QCOMPARE(encoded[1], static_cast<std::uint8_t>(0xD4));
    QCOMPARE(encoded[2], static_cast<std::uint8_t>(0x01));
    QCOMPARE(encoded[3], static_cast<std::uint8_t>(0x00));
}

QTEST_MAIN(TestS7Data)
#include "test_s7_data.moc"
