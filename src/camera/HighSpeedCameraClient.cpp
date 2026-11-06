#include "camera/HighSpeedCameraClient.h"

#include <QByteArray>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QVector>

#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>

#if defined(Q_OS_WIN)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#define AFP_ABB_MVS_CALL __stdcall
#else
#define AFP_ABB_MVS_CALL
#endif

namespace {
constexpr quint32 kGigEDevice = 0x00000001;
constexpr quint32 kUsbDevice = 0x00000004;
constexpr quint32 kLayerTypes = kGigEDevice | kUsbDevice;
constexpr quint32 kExclusiveAccess = 1;
constexpr int kInfoMaxBufferSize = 64;
constexpr int kMaxDeviceCount = 256;
constexpr quint32 kOk = 0;

// These declarations mirror only the MVS SDK fields used by the original
// Python ctypes wrapper. Keeping them private avoids leaking vendor SDK details
// into the service layer or tests.
struct MV_GIGE_DEVICE_INFO {
    quint32 nIpCfgOption = 0;
    quint32 nIpCfgCurrent = 0;
    quint32 nCurrentIp = 0;
    quint32 nCurrentSubNetMask = 0;
    quint32 nDefultGateWay = 0;
    quint8 chManufacturerName[32]{};
    quint8 chModelName[32]{};
    quint8 chDeviceVersion[32]{};
    quint8 chManufacturerSpecificInfo[48]{};
    quint8 chSerialNumber[16]{};
    quint8 chUserDefinedName[16]{};
    quint32 nNetExport = 0;
    quint32 nReserved[4]{};
};

struct MV_USB3_DEVICE_INFO {
    quint8 CrtlInEndPoint = 0;
    quint8 CrtlOutEndPoint = 0;
    quint8 StreamEndPoint = 0;
    quint8 EventEndPoint = 0;
    quint16 idVendor = 0;
    quint16 idProduct = 0;
    quint32 nDeviceNumber = 0;
    quint8 chDeviceGUID[kInfoMaxBufferSize]{};
    quint8 chVendorName[kInfoMaxBufferSize]{};
    quint8 chModelName[kInfoMaxBufferSize]{};
    quint8 chFamilyName[kInfoMaxBufferSize]{};
    quint8 chDeviceVersion[kInfoMaxBufferSize]{};
    quint8 chManufacturerName[kInfoMaxBufferSize]{};
    quint8 chSerialNumber[kInfoMaxBufferSize]{};
    quint8 chUserDefinedName[kInfoMaxBufferSize]{};
    quint32 nbcdUSB = 0;
    quint32 nDeviceAddress = 0;
    quint32 nReserved[2]{};
};

struct MV_CAML_DEVICE_INFO {
    quint8 chPortID[kInfoMaxBufferSize]{};
    quint8 chModelName[kInfoMaxBufferSize]{};
    quint8 chFamilyName[kInfoMaxBufferSize]{};
    quint8 chDeviceVersion[kInfoMaxBufferSize]{};
    quint8 chManufacturerName[kInfoMaxBufferSize]{};
    quint8 chSerialNumber[kInfoMaxBufferSize]{};
    quint32 nReserved[38]{};
};

union MV_DEVICE_SPECIAL_INFO {
    MV_GIGE_DEVICE_INFO stGigEInfo;
    MV_USB3_DEVICE_INFO stUsb3VInfo;
    MV_CAML_DEVICE_INFO stCamLInfo;
};

struct MV_CC_DEVICE_INFO {
    quint16 nMajorVer = 0;
    quint16 nMinorVer = 0;
    quint32 nMacAddrHigh = 0;
    quint32 nMacAddrLow = 0;
    quint32 nTLayerType = 0;
    quint32 nReserved[4]{};
    MV_DEVICE_SPECIAL_INFO SpecialInfo{};
};

struct MV_CC_DEVICE_INFO_LIST {
    quint32 nDeviceNum = 0;
    MV_CC_DEVICE_INFO* pDeviceInfo[kMaxDeviceCount]{};
};

struct MVCC_INTVALUE {
    quint32 nCurValue = 0;
    quint32 nMax = 0;
    quint32 nMin = 0;
    quint32 nInc = 0;
    quint32 nReserved[4]{};
};

struct MVCC_FLOATVALUE {
    float fCurValue = 0.0F;
    float fMax = 0.0F;
    float fMin = 0.0F;
    quint32 nReserved[4]{};
};

union MV_FRAME_CHUNK_INFO {
    void* pUnparsedChunkContent;
    qint64 nAligning;
};

struct MV_FRAME_OUT_INFO_EX {
    quint16 nWidth = 0;
    quint16 nHeight = 0;
    qint32 enPixelType = 0;
    quint32 nFrameNum = 0;
    quint32 nDevTimeStampHigh = 0;
    quint32 nDevTimeStampLow = 0;
    quint32 nReserved0 = 0;
    qint64 nHostTimeStamp = 0;
    quint32 nFrameLen = 0;
    quint32 nSecondCount = 0;
    quint32 nCycleCount = 0;
    quint32 nCycleOffset = 0;
    float fGain = 0.0F;
    float fExposureTime = 0.0F;
    quint32 nAverageBrightness = 0;
    quint32 nRed = 0;
    quint32 nGreen = 0;
    quint32 nBlue = 0;
    quint32 nFrameCounter = 0;
    quint32 nTriggerIndex = 0;
    quint32 nInput = 0;
    quint32 nOutput = 0;
    quint16 nOffsetX = 0;
    quint16 nOffsetY = 0;
    quint16 nChunkWidth = 0;
    quint16 nChunkHeight = 0;
    quint32 nLostPacket = 0;
    quint32 nUnparsedChunkNum = 0;
    MV_FRAME_CHUNK_INFO UnparsedChunkList{};
    quint32 nReserved[36]{};
};

QString zeroTerminatedText(const quint8* data, int maxLength) {
    int length = 0;
    while (length < maxLength && data[length] != 0) {
        ++length;
    }
    return QString::fromLatin1(reinterpret_cast<const char*>(data), length);
}

QString ipText(quint32 ip) {
    return QString("%1.%2.%3.%4")
        .arg((ip & 0xFF000000u) >> 24)
        .arg((ip & 0x00FF0000u) >> 16)
        .arg((ip & 0x0000FF00u) >> 8)
        .arg(ip & 0x000000FFu);
}

}

struct HighSpeedCameraClient::MvsFunctions {
    using EnumDevicesFn = quint32(AFP_ABB_MVS_CALL*)(quint32, MV_CC_DEVICE_INFO_LIST*);
    using CreateHandleFn = quint32(AFP_ABB_MVS_CALL*)(void**, MV_CC_DEVICE_INFO*);
    using DestroyHandleFn = quint32(AFP_ABB_MVS_CALL*)(void*);
    using OpenDeviceFn = quint32(AFP_ABB_MVS_CALL*)(void*, quint32, quint16);
    using CloseDeviceFn = quint32(AFP_ABB_MVS_CALL*)(void*);
    using StartGrabbingFn = quint32(AFP_ABB_MVS_CALL*)(void*);
    using StopGrabbingFn = quint32(AFP_ABB_MVS_CALL*)(void*);
    using GetOptimalPacketSizeFn = int(AFP_ABB_MVS_CALL*)(void*);
    using GetIntValueFn = quint32(AFP_ABB_MVS_CALL*)(void*, const char*, MVCC_INTVALUE*);
    using GetFloatValueFn = quint32(AFP_ABB_MVS_CALL*)(void*, const char*, MVCC_FLOATVALUE*);
    using SetIntValueFn = quint32(AFP_ABB_MVS_CALL*)(void*, const char*, quint32);
    using SetFloatValueFn = quint32(AFP_ABB_MVS_CALL*)(void*, const char*, float);
    using SetEnumValueFn = quint32(AFP_ABB_MVS_CALL*)(void*, const char*, quint32);
    using GetImageForRgbFn = quint32(AFP_ABB_MVS_CALL*)(void*, unsigned char*, quint32, MV_FRAME_OUT_INFO_EX*, quint32);

    EnumDevicesFn enumDevices = nullptr;
    CreateHandleFn createHandle = nullptr;
    DestroyHandleFn destroyHandle = nullptr;
    OpenDeviceFn openDevice = nullptr;
    CloseDeviceFn closeDevice = nullptr;
    StartGrabbingFn startGrabbing = nullptr;
    StopGrabbingFn stopGrabbing = nullptr;
    GetOptimalPacketSizeFn getOptimalPacketSize = nullptr;
    GetIntValueFn getIntValue = nullptr;
    GetFloatValueFn getFloatValue = nullptr;
    SetIntValueFn setIntValue = nullptr;
    SetFloatValueFn setFloatValue = nullptr;
    SetEnumValueFn setEnumValue = nullptr;
    GetImageForRgbFn getImageForRgb = nullptr;
};

struct HighSpeedCameraClient::MvsDeviceInfoList {
    MV_CC_DEVICE_INFO_LIST list{};
};

HighSpeedCameraClient::HighSpeedCameraClient(HighSpeedCameraConfig config, QString appDirPath)
    : config_(std::move(config)), appDirPath_(std::move(appDirPath)), functions_(new MvsFunctions) {
    buildDllSearchList();
}

HighSpeedCameraClient::~HighSpeedCameraClient() {
    close();
    delete deviceList_;
    delete functions_;
}

bool HighSpeedCameraClient::load() {
    if (library_.isLoaded()) {
        return true;
    }

    QStringList loadErrors;
    for (const QString& path : searchedDllPaths_) {
        if (!QFileInfo::exists(path)) {
            continue;
        }
        if (loadFromPath(path)) {
            return true;
        }
        loadErrors << QString("%1: %2").arg(path, lastError_);
    }

    QString message = QString("Could not load high-speed camera SDK DLL. Searched paths:\n%1")
                          .arg(searchedDllPaths_.join('\n'));
    if (!loadErrors.isEmpty()) {
        message += QString("\nAttempted existing DLLs:\n%1").arg(loadErrors.join('\n'));
    }
    setError(message);
    return false;
}

bool HighSpeedCameraClient::loaded() const {
    return library_.isLoaded();
}

QStringList HighSpeedCameraClient::searchedDllPaths() const {
    return searchedDllPaths_;
}

QVector<HighSpeedCameraDevice> HighSpeedCameraClient::enumerateDevices() {
    if (!requireLoaded("enumerate cameras")) {
        return {};
    }

    if (!deviceList_) {
        deviceList_ = new MvsDeviceInfoList;
    }
    std::memset(&deviceList_->list, 0, sizeof(deviceList_->list));

    const quint32 result = functions_->enumDevices(kLayerTypes, &deviceList_->list);
    if (result != kOk) {
        failWithSdkCode("Camera enumeration failed", result);
        return {};
    }

    QVector<HighSpeedCameraDevice> devices;
    QStringList unsupportedTransportTypes;
    const quint32 rawDeviceCount = deviceList_->list.nDeviceNum;
    const quint32 count = (std::min)(rawDeviceCount, static_cast<quint32>(kMaxDeviceCount));
    devices.reserve(static_cast<int>(count));
    for (quint32 i = 0; i < count; ++i) {
        const MV_CC_DEVICE_INFO* info = deviceList_->list.pDeviceInfo[i];
        if (!info) {
            continue;
        }

        HighSpeedCameraDevice device;
        device.index = static_cast<int>(i);
        if (info->nTLayerType == kGigEDevice) {
            device.transport = HighSpeedCameraTransport::GigE;
            device.userName = zeroTerminatedText(info->SpecialInfo.stGigEInfo.chUserDefinedName, 16);
            device.endpoint = ipText(info->SpecialInfo.stGigEInfo.nCurrentIp);
        } else if (info->nTLayerType == kUsbDevice) {
            device.transport = HighSpeedCameraTransport::Usb;
            device.userName = zeroTerminatedText(info->SpecialInfo.stUsb3VInfo.chUserDefinedName, kInfoMaxBufferSize);
            device.endpoint = zeroTerminatedText(info->SpecialInfo.stUsb3VInfo.chSerialNumber, kInfoMaxBufferSize);
        } else {
            unsupportedTransportTypes << highSpeedCameraErrorHex(info->nTLayerType);
            continue;
        }
        devices << device;
    }

    if (rawDeviceCount == 0) {
        setError("MVS enumeration succeeded, but returned 0 devices. Check camera power, cable, driver installation, GigE adapter/subnet settings, and whether MVS Viewer can see the camera.");
    } else if (devices.isEmpty()) {
        QString message = QString("MVS enumeration returned %1 raw device(s), but none were supported GigE or USB cameras for this slice.").arg(rawDeviceCount);
        if (!unsupportedTransportTypes.isEmpty()) {
            unsupportedTransportTypes.removeDuplicates();
            message += QString(" Transport type(s): %1.").arg(unsupportedTransportTypes.join(", "));
        }
        setError(message);
    } else {
        lastError_.clear();
    }
    return devices;
}

bool HighSpeedCameraClient::open(int deviceIndex) {
    if (!requireLoaded("open camera")) {
        return false;
    }
    const int availableDeviceCount = deviceList_
        ? static_cast<int>((std::min)(deviceList_->list.nDeviceNum, static_cast<quint32>(kMaxDeviceCount)))
        : 0;
    if (!deviceList_ || deviceIndex < 0 || deviceIndex >= availableDeviceCount) {
        setError("Selected camera index is not available. Enumerate cameras first.");
        return false;
    }

    close();
    MV_CC_DEVICE_INFO* info = deviceList_->list.pDeviceInfo[deviceIndex];
    if (!info) {
        setError("Selected camera information is empty.");
        return false;
    }

    quint32 result = functions_->createHandle(&handle_, info);
    if (result != kOk) {
        handle_ = nullptr;
        return failWithSdkCode("Camera handle creation failed", result);
    }

    result = functions_->openDevice(handle_, kExclusiveAccess, 0);
    if (result != kOk) {
        close();
        return failWithSdkCode("Camera open failed", result);
    }
    open_ = true;

    if (info->nTLayerType == kGigEDevice) {
        const int packetSize = functions_->getOptimalPacketSize(handle_);
        if (packetSize > 0) {
            // Python sets GevSCPSPacketSize for GigE cameras when MVS reports a
            // useful value. This reduces packet fragmentation on real hardware.
            result = functions_->setIntValue(handle_, "GevSCPSPacketSize", static_cast<quint32>(packetSize));
            if (result != kOk) {
                close();
                return failWithSdkCode("Setting GigE packet size failed", result);
            }
        }
    }

    result = functions_->setEnumValue(handle_, "TriggerMode", 0);
    if (result != kOk) {
        close();
        return failWithSdkCode("Setting continuous trigger mode failed", result);
    }
    return true;
}

bool HighSpeedCameraClient::isOpen() const {
    return open_;
}

bool HighSpeedCameraClient::startGrabbing() {
    if (!open_) {
        setError("Camera is not open.");
        return false;
    }
    if (grabbing_) {
        return true;
    }
    const quint32 result = functions_->startGrabbing(handle_);
    if (result != kOk) {
        return failWithSdkCode("Starting camera grabbing failed", result);
    }
    grabbing_ = true;
    return true;
}

bool HighSpeedCameraClient::readFrame(HighSpeedCameraFrame& frame, int timeoutMs) {
    if (!grabbing_) {
        setError("Camera is not grabbing.");
        return false;
    }

    MVCC_INTVALUE payload{};
    quint32 result = functions_->getIntValue(handle_, "PayloadSize", &payload);
    if (result != kOk) {
        return failWithSdkCode("Reading camera payload size failed", result);
    }
    if (payload.nCurValue == 0) {
        setError("Camera payload size is zero.");
        return false;
    }

    const quint64 rgbBufferBytes = static_cast<quint64>(payload.nCurValue) * 3ULL;
    if (rgbBufferBytes > static_cast<quint64>((std::numeric_limits<quint32>::max)())) {
        setError("Camera payload size is too large for the MVS RGB frame API.");
        return false;
    }
    if (rgbBufferBytes > static_cast<quint64>((std::numeric_limits<qsizetype>::max)())) {
        setError("Camera payload size is too large for the preview buffer.");
        return false;
    }

    QVector<unsigned char> buffer(static_cast<qsizetype>(rgbBufferBytes));
    MV_FRAME_OUT_INFO_EX info{};
    result = functions_->getImageForRgb(
        handle_,
        buffer.data(),
        static_cast<quint32>(buffer.size()),
        &info,
        static_cast<quint32>((std::max)(timeoutMs, 1)));
    if (result != kOk) {
        return failWithSdkCode("Reading RGB camera frame failed", result);
    }
    if (info.nWidth == 0 || info.nHeight == 0) {
        setError("Camera returned an empty frame.");
        return false;
    }

    const int width = static_cast<int>(info.nWidth);
    const int height = static_cast<int>(info.nHeight);
    const int bytesPerLine = width * 3;
    const quint64 requiredBytes = static_cast<quint64>(bytesPerLine) * static_cast<quint64>(height);
    if (requiredBytes > static_cast<quint64>(buffer.size())) {
        setError("Camera RGB frame is smaller than expected.");
        return false;
    }

    frame.image = QImage(buffer.constData(), width, height, bytesPerLine, QImage::Format_RGB888).copy();
    return true;
}

void HighSpeedCameraClient::stopGrabbing() {
    if (grabbing_ && functions_ && functions_->stopGrabbing && handle_) {
        functions_->stopGrabbing(handle_);
    }
    grabbing_ = false;
}

void HighSpeedCameraClient::close() {
    stopGrabbing();
    if (open_ && functions_ && functions_->closeDevice && handle_) {
        functions_->closeDevice(handle_);
    }
    open_ = false;
    if (functions_ && functions_->destroyHandle && handle_) {
        functions_->destroyHandle(handle_);
    }
    handle_ = nullptr;
}

bool HighSpeedCameraClient::readParameters(HighSpeedCameraParameters& parameters) {
    if (!open_) {
        setError("Camera is not open.");
        return false;
    }

    MVCC_FLOATVALUE value{};
    quint32 result = functions_->getFloatValue(handle_, "AcquisitionFrameRate", &value);
    if (result != kOk) {
        return failWithSdkCode("Reading AcquisitionFrameRate failed", result);
    }
    parameters.frameRate = value.fCurValue;

    result = functions_->getFloatValue(handle_, "ExposureTime", &value);
    if (result != kOk) {
        return failWithSdkCode("Reading ExposureTime failed", result);
    }
    parameters.exposureTime = value.fCurValue;

    result = functions_->getFloatValue(handle_, "Gain", &value);
    if (result != kOk) {
        return failWithSdkCode("Reading Gain failed", result);
    }
    parameters.gain = value.fCurValue;
    return true;
}

bool HighSpeedCameraClient::writeParameters(const HighSpeedCameraParameters& parameters) {
    if (!open_) {
        setError("Camera is not open.");
        return false;
    }

    quint32 result = functions_->setFloatValue(handle_, "AcquisitionFrameRate", static_cast<float>(parameters.frameRate));
    if (result != kOk) {
        return failWithSdkCode("Writing AcquisitionFrameRate failed", result);
    }
    result = functions_->setFloatValue(handle_, "ExposureTime", static_cast<float>(parameters.exposureTime));
    if (result != kOk) {
        return failWithSdkCode("Writing ExposureTime failed", result);
    }
    result = functions_->setFloatValue(handle_, "Gain", static_cast<float>(parameters.gain));
    if (result != kOk) {
        return failWithSdkCode("Writing Gain failed", result);
    }
    return true;
}

QString HighSpeedCameraClient::lastError() const {
    return lastError_;
}

void HighSpeedCameraClient::buildDllSearchList() {
    searchedDllPaths_.clear();
    searchedDllPaths_ << QDir(appDirPath_).filePath(config_.portableSdkDllPath);
    if (!config_.sdkDllPath.trimmed().isEmpty()) {
        searchedDllPaths_ << QDir::cleanPath(config_.sdkDllPath.trimmed());
    }
    searchedDllPaths_ << QDir::cleanPath(config_.installedSdkDllPath);
    searchedDllPaths_.removeDuplicates();
}

bool HighSpeedCameraClient::loadFromPath(const QString& path) {
    if (!configureDllSearchPath(path)) {
        return false;
    }

    library_.setFileName(path);
    const bool loadedLibrary = library_.load();
#if defined(Q_OS_WIN)
    // SetDllDirectoryW is process-wide. It is needed only while LoadLibrary is
    // resolving MVS dependent DLLs from the vendor folder, so clear it
    // immediately after the load attempt to avoid affecting unrelated Qt or
    // hardware libraries later in the process.
    SetDllDirectoryW(nullptr);
#endif
    if (!loadedLibrary) {
        setError(QString("Could not load high-speed camera SDK DLL '%1': %2").arg(path, library_.errorString()));
        return false;
    }

    if (!resolveFunctions()) {
        library_.unload();
        return false;
    }

    lastError_.clear();
    return true;
}

bool HighSpeedCameraClient::configureDllSearchPath(const QString& dllPath) {
#if defined(Q_OS_WIN)
    const QString dllDirectory = QFileInfo(dllPath).absoluteDir().absolutePath();
    const std::wstring nativeDirectory = QDir::toNativeSeparators(dllDirectory).toStdWString();
    if (!SetDllDirectoryW(nativeDirectory.c_str())) {
        setError(QString("Could not add high-speed camera DLL directory to the Windows search path: %1")
                     .arg(dllDirectory));
        return false;
    }
#else
    Q_UNUSED(dllPath);
#endif
    return true;
}

bool HighSpeedCameraClient::resolveFunctions() {
    const auto resolve = [this](auto& target, const char* name) -> bool {
        target = reinterpret_cast<std::remove_reference_t<decltype(target)>>(library_.resolve(name));
        if (!target) {
            setError(QString("MvCameraControl.dll is missing required export %1.").arg(name));
            return false;
        }
        return true;
    };

    if (!resolve(functions_->enumDevices, "MV_CC_EnumDevices") ||
        !resolve(functions_->createHandle, "MV_CC_CreateHandle") ||
        !resolve(functions_->destroyHandle, "MV_CC_DestroyHandle") ||
        !resolve(functions_->openDevice, "MV_CC_OpenDevice") ||
        !resolve(functions_->closeDevice, "MV_CC_CloseDevice") ||
        !resolve(functions_->startGrabbing, "MV_CC_StartGrabbing") ||
        !resolve(functions_->stopGrabbing, "MV_CC_StopGrabbing") ||
        !resolve(functions_->getOptimalPacketSize, "MV_CC_GetOptimalPacketSize") ||
        !resolve(functions_->getIntValue, "MV_CC_GetIntValue") ||
        !resolve(functions_->getFloatValue, "MV_CC_GetFloatValue") ||
        !resolve(functions_->setIntValue, "MV_CC_SetIntValue") ||
        !resolve(functions_->setFloatValue, "MV_CC_SetFloatValue") ||
        !resolve(functions_->setEnumValue, "MV_CC_SetEnumValue")) {
        return false;
    }

    functions_->getImageForRgb =
        reinterpret_cast<MvsFunctions::GetImageForRgbFn>(library_.resolve("MV_CC_GetImageForRGB"));
    if (!functions_->getImageForRgb) {
        setError("MvCameraControl.dll does not expose MV_CC_GetImageForRGB; live preview cannot start without RGB frame support.");
        return false;
    }
    return true;
}

bool HighSpeedCameraClient::requireLoaded(const QString& action) {
    if (load()) {
        return true;
    }
    if (lastError_.isEmpty()) {
        setError(QString("Cannot %1 because the high-speed camera SDK is not loaded.").arg(action));
    }
    return false;
}

bool HighSpeedCameraClient::failWithSdkCode(const QString& action, quint32 code) {
    setError(QString("%1. SDK return code: %2").arg(action, highSpeedCameraErrorHex(code)));
    return false;
}

void HighSpeedCameraClient::setError(const QString& message) {
    lastError_ = message;
}

#undef AFP_ABB_MVS_CALL
