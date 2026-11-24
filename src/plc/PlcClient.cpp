#include "plc/PlcClient.h"

#include <array>
#include <utility>

#include <QByteArray>
#include <snap7/snap7_libmain.h>

#include "plc/S7Data.h"

namespace {

// Snap7 area constants are kept behind descriptive names so call sites read as
// PLC memory operations rather than raw library invocations.
constexpr int s7AreaDb = S7AreaDB;
constexpr int s7AreaMk = S7AreaMK;
constexpr int s7AreaPa = S7AreaPA;
constexpr int s7AreaPe = S7AreaPE;
constexpr int s7WordLenByte = S7WLByte;

}

PlcResult PlcResult::success() {
    return {true, QString()};
}

PlcResult PlcResult::failure(const QString& message) {
    return {false, message};
}

PlcClient::PlcClient(PlcConfig config)
    : config_(std::move(config)),
      client_(Cli_Create()) {
}

PlcClient::~PlcClient() {
    disconnect();
    if (client_ != 0) {
        // Cli_Destroy takes the handle by reference and clears it internally.
        Cli_Destroy(client_);
    }
}

bool PlcClient::isConnected() const {
    return connected_;
}

QString PlcClient::lastError() const {
    return lastError_;
}

PlcResult PlcClient::connect() {
    if (isConnected()) {
        return PlcResult::success();
    }

    // Python used snap7.client.Client().connect(ip, rack, slot). This mirrors
    // that call through Snap7's C ABI and the same rack/slot config.
    const QByteArray ip = config_.ip.toUtf8();
    const int result = Cli_ConnectTo(client_, ip.constData(), config_.rack, config_.slot);
    if (result != 0) {
        connected_ = false;
        return snap7Failure("connect", result);
    }

    connected_ = true;
    lastError_.clear();
    return PlcResult::success();
}

void PlcClient::disconnect() {
    if (client_ != 0 && connected_) {
        Cli_Disconnect(client_);
        connected_ = false;
    }
}

PlcResult PlcClient::writeDbBit(int dbNumber, int byteOffset, int bitIndex, bool value) {
    bool currentBit = false;
    PlcResult readResult = readDbBit(dbNumber, byteOffset, bitIndex, currentBit);
    if (!readResult.ok) {
        return readResult;
    }

    // Snap7 writes a whole byte, not an isolated bit. Read-modify-write keeps
    // neighboring PLC command bits untouched.
    std::array<std::uint8_t, 1> data{};
    int readCode = Cli_ReadArea(client_, s7AreaDb, dbNumber, byteOffset, 1, s7WordLenByte, data.data());
    if (readCode != 0) {
        connected_ = false;
        return snap7Failure("read DB byte", readCode);
    }

    quint8 byteValue = static_cast<quint8>(data[0]);
    S7Data::setBit(byteValue, bitIndex, value);
    data[0] = static_cast<std::uint8_t>(byteValue);

    int writeCode = Cli_WriteArea(client_, s7AreaDb, dbNumber, byteOffset, 1, s7WordLenByte, data.data());
    if (writeCode != 0) {
        connected_ = false;
        return snap7Failure("write DB bit", writeCode);
    }

    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::readDbBit(int dbNumber, int byteOffset, int bitIndex, bool& value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    std::array<std::uint8_t, 1> data{};
    int code = Cli_ReadArea(client_, s7AreaDb, dbNumber, byteOffset, 1, s7WordLenByte, data.data());
    if (code != 0) {
        connected_ = false;
        return snap7Failure("read DB bit", code);
    }

    value = S7Data::bitIsSet(static_cast<quint8>(data[0]), bitIndex);
    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::writeDbByte(int dbNumber, int byteOffset, quint8 value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    std::array<std::uint8_t, 1> data{static_cast<std::uint8_t>(value)};
    int code = Cli_WriteArea(client_, s7AreaDb, dbNumber, byteOffset, 1, s7WordLenByte, data.data());
    if (code != 0) {
        connected_ = false;
        return snap7Failure("write DB byte", code);
    }

    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::readDbByte(int dbNumber, int byteOffset, quint8& value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    std::array<std::uint8_t, 1> data{};
    int code = Cli_ReadArea(client_, s7AreaDb, dbNumber, byteOffset, 1, s7WordLenByte, data.data());
    if (code != 0) {
        connected_ = false;
        return snap7Failure("read DB byte", code);
    }

    value = static_cast<quint8>(data[0]);
    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::writeDbInt16(int dbNumber, int byteOffset, qint16 value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    // S7 INT is two big-endian bytes. The helper centralizes that encoding.
    auto data = S7Data::encodeInt16(value);
    int code = Cli_WriteArea(client_, s7AreaDb, dbNumber, byteOffset, static_cast<int>(data.size()), s7WordLenByte, data.data());
    if (code != 0) {
        connected_ = false;
        return snap7Failure("write DB int", code);
    }

    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::writeDbReal32(int dbNumber, int byteOffset, float value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    // S7 REAL is the 32-bit IEEE float format used by the Python snap7 helper.
    auto data = S7Data::encodeReal32(value);
    int code = Cli_WriteArea(client_, s7AreaDb, dbNumber, byteOffset, static_cast<int>(data.size()), s7WordLenByte, data.data());
    if (code != 0) {
        connected_ = false;
        return snap7Failure("write DB real", code);
    }

    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::writeMBit(int byteOffset, int bitIndex, bool value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    // M bits live in the marker memory area. Motor reset uses M12.7 in the
    // Python application.
    std::array<std::uint8_t, 1> data{};
    int readCode = Cli_ReadArea(client_, s7AreaMk, 0, byteOffset, 1, s7WordLenByte, data.data());
    if (readCode != 0) {
        connected_ = false;
        return snap7Failure("read M byte", readCode);
    }

    quint8 byteValue = static_cast<quint8>(data[0]);
    S7Data::setBit(byteValue, bitIndex, value);
    data[0] = static_cast<std::uint8_t>(byteValue);

    int writeCode = Cli_WriteArea(client_, s7AreaMk, 0, byteOffset, 1, s7WordLenByte, data.data());
    if (writeCode != 0) {
        connected_ = false;
        return snap7Failure("write M bit", writeCode);
    }

    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::writeProcessOutputInt32(int byteOffset, qint32 value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    // Process output DINTs mirror the Python robot movement writes, which pack
    // with struct.pack("<i", value) instead of S7 DB big-endian helpers.
    auto data = S7Data::encodeInt32LittleEndian(value);
    int code = Cli_WriteArea(client_, s7AreaPa, 0, byteOffset, static_cast<int>(data.size()), s7WordLenByte, data.data());
    if (code != 0) {
        connected_ = false;
        return snap7Failure("write process output int32", code);
    }

    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::readProcessOutputInt32(int byteOffset, qint32& value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    std::array<std::uint8_t, 4> data{};
    int code = Cli_ReadArea(client_, s7AreaPa, 0, byteOffset, static_cast<int>(data.size()), s7WordLenByte, data.data());
    if (code != 0) {
        connected_ = false;
        return snap7Failure("read process output int32", code);
    }

    value = S7Data::decodeInt32LittleEndian(data);
    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::readProcessInputInt32(int byteOffset, qint32& value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    std::array<std::uint8_t, 4> data{};
    int code = Cli_ReadArea(client_, s7AreaPe, 0, byteOffset, static_cast<int>(data.size()), s7WordLenByte, data.data());
    if (code != 0) {
        connected_ = false;
        return snap7Failure("read process input int32", code);
    }

    value = S7Data::decodeInt32LittleEndian(data);
    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::readProcessInputByte(int byteOffset, quint8& value) {
    PlcResult connection = ensureConnected();
    if (!connection.ok) {
        return connection;
    }

    std::array<std::uint8_t, 1> data{};
    int code = Cli_ReadArea(client_, s7AreaPe, 0, byteOffset, 1, s7WordLenByte, data.data());
    if (code != 0) {
        connected_ = false;
        return snap7Failure("read process input byte", code);
    }

    value = static_cast<quint8>(data[0]);
    lastError_.clear();
    return PlcResult::success();
}

PlcResult PlcClient::ensureConnected() {
    if (isConnected()) {
        return PlcResult::success();
    }
    return connect();
}

PlcResult PlcClient::snap7Failure(const QString& action, int code) {
    lastError_ = QString("PLC %1 failed with Snap7 error %2").arg(action).arg(code);
    return PlcResult::failure(lastError_);
}
