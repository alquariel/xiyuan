#pragma once

#include <QString>
#include <QStringList>
#include "app/AppPaths.h"

struct RobotDefaultSpeedConfig {
    // Defaults preserve the Python main-window state used when speed fields are empty.
    double tcpLinear = 200.0;
    double tcpOrientation = 200.0;
    double externalLinear = 200.0;
    double externalOrientation = 100.0;
};

struct RobotMonitorConfig {
    // Master switch for the robot monitor socket; later Slice K tasks will use
    // this without changing the existing ConnectionRobotButton behavior.
    bool enabled = false;

    // When true, the monitor service may start from application startup config
    // instead of waiting for the user to click the robot connection button.
    bool autostart = false;

    // Dedicated RAPID monitor socket port. This replaces the older unused
    // robot.port_server setting so motion commands and monitoring stay distinct.
    int port = 5012;

    // Requested RAPID/C++ monitor sample cadence during first validation.
    int sampleIntervalMs = 100;

    // Normal robot monitor archive path. CSV/raw recording remain diagnostics.
    bool ddsPublishEnabled = true;
    int ddsDomainId = 0;
    QString ddsTopic = "afp.robot.monitor.sample";

    // Writes parsed samples to a simple CSV file for hardware diagnostics.
    bool recordCsv = false;

    // Writes exact socket text before parsing, useful when checking RAPID/C++ parser agreement.
    bool recordRawMessages = false;

    // Relative paths resolve from the application directory in later monitor tasks.
    QString recordingDir = "logs/robot_monitor";

    // Flushes every row/message to disk so test data survives crashes or forced stops.
    bool flushEverySample = true;
};

struct RobotConfig {
    QString ip = "192.168.0.104";
    int portMotion = 5010;
    QString transportType = "plc";
    RobotDefaultSpeedConfig defaultSpeed;
    RobotMonitorConfig monitor;
};

struct FtpConfig {
    // When false (RobotStudio virtual-controller stations typically have no
    // FTP server), selecting a MOD path file still validates the local file
    // but simulates upload success so the print workflow can proceed over
    // the socket channel only.
    bool enabled = true;

    QString ip = "192.168.0.104";
    int port = 21;
    QString user = "Default User";
    QString password = "robotics";
};

struct PlcConfig {
    // When false the cell has no S7-1200 (for example a RobotStudio virtual
    // controller station). The DB18 print handshake is then simulated and the
    // DB5 direct controls are disabled instead of waiting for Snap7 timeouts.
    bool enabled = true;

    QString ip = "192.168.0.201";
    int rack = 0;
    int slot = 1;
};

struct EgmConfig {
    // 总开关。false 时 EGM 完全不启动（未装 689-1 的虚拟控制器用这个降级）。
    bool enabled = false;

    // 本机监听端口。必须与控制器 EIO/SIO 配置里 UCdevice 的 RemotePortNumber 一致
    // （默认 6510）。控制器会先向 本机:6510 发 EgmRobot，本机再回发 EgmSensor。
    QString ip = "127.0.0.1";
    int port = 6510;
    int offset = 0;

    // Streaming cadence: ABB's EGM application manual recommends 250 Hz for
    // industrial use (4 ms period). Reduce to 100 Hz / 10 ms for software-
    // only playback on slower controllers.
    int periodMs = 4;

    // Deviation safety parameters — must match EGMServer.mod EGMSetup values.
    double maxDeviationTcpMm = 0.1;
    double maxSpeedDeviationTcpMmPerSec = 2000.0;
};

struct ThermalCameraConfig {
    QString ip = "192.168.0.64";
    int port = 8000;
    QString user = "admin";
    QString password = "abcd1234";

    // Optional absolute runtime folder for installed Hikvision thermal SDK files.
    QString sdkDir;

    // Relative to the application directory for portable thermal-camera releases.
    QString portableSdkDir = "lib/sensors/thermal_camera/lib/win";

    // Matches Python thermal-camera cache pruning behavior.
    int cacheMaxFiles = 60;

    // Matches Python timer2.start(1000) display refresh timing.
    int displayIntervalMs = 1000;
};

struct HighSpeedCameraConfig {
    // Empty means "try the portable path, then the installed MVS runtime path";
    // a non-empty value is tried between those two defaults.
    QString sdkDllPath;

    // Relative to the application directory. This supports portable releases.
    QString portableSdkDllPath = "lib/sensors/high_speed_camera/MvCameraControl.dll";

    // Fallback used on development PCs where the MVS runtime is installed.
    QString installedSdkDllPath =
        "C:/Program Files (x86)/Common Files/MVS/Runtime/Win64_x64/MvCameraControl.dll";

    // Matches highspeed_camera_service.py MV_CC_GetOneFrameTimeout(..., 1000).
    int frameTimeoutMs = 1000;
};

struct SerialConfig {
    int generalBaudrate = 9600;
    QString fiberLaserPort = "COM3";
    int fiberLaserBaudrate = 9600;
};

struct PressureConfig {
    // Matches pressure_reader.py time.sleep(0.1), but lives in config so users
    // can tune acquisition without recompiling the C++ application.
    int sampleIntervalMs = 100;

    // Matches pressure_monitor.py's 40-sample rolling processing window.
    int processingWindowSamples = 40;

    // Fast DDS defaults. Domain 0 is the standard local discovery domain.
    int ddsDomainId = 0;
    QString ddsTopic = "afp.pressure.sample";
};

struct TimeseriesArchiveConfig {
    // Master switch for the optional InfluxDB archive feature.
    bool enabled = false;

    // When true, the main app may start influxdb3.exe if hostUrl is not reachable.
    bool startInfluxdb = false;
    bool influxdbShowConsole = true;
    bool stopInfluxdbOnExit = true;

    // Configurable because InfluxDB is installed outside the portable AFP folder.
    QString influxdbExePath =
        "D:/InfluxDB 3 Core/influxdb3-core-3.9.2-windows_amd64/influxdb3.exe";
    QString hostUrl = "http://localhost:8181";
    QString nodeId = "afp-node";
    QString objectStore = "file";
    QString dataDir = "D:/InfluxDB 3 Core/data";
    QString adminTokenFile = "D:/InfluxDB 3 Core/afp-admin-token.json";
    QString database = "afp_process";
    QString retentionPeriod = "30d";

    // Creates the database only when it is missing. Existing data is not replaced.
    bool ensureDatabaseOnStart = true;

    // Controls whether AFP_ABB_CPP.exe starts dds_influx_subscriber.exe.
    bool subscriberAutostart = true;
    QString subscriberExecutable = "dds_influx_subscriber.exe";
    bool subscriberShowConsole = true;
    bool subscriberPrintLiveData = true;

    // Low-latency defaults for pressure data. At 100 ms/sample, 10 points is about 1 second.
    int writeBatchSize = 10;
    int writeFlushIntervalMs = 1000;

    // Bounds the synchronous HTTP write loop so an unreachable server cannot stall forever.
    int httpRequestTimeoutMs = 5000;

    // Caps buffered rows during an InfluxDB outage to prevent unbounded memory growth.
    int maxPendingPoints = 10000;

    // Startup orchestration timeouts. These are configurable because InfluxDB
    // cold starts can vary a lot between machines and storage locations.
    int reachabilityProbeTimeoutMs = 500;
    int influxdbStartupTimeoutMs = 30000;
    int cliTimeoutMs = 10000;
};

struct GrafanaConfig {
    // Optional Grafana server launcher used for local pressure-data dashboards.
    bool enabled = true;
    bool autostart = true;
    QString exePath = "D:/Grafana OSS/grafana-13.0.1+security-01/bin/grafana.exe";
    QString workingDir = "D:/Grafana OSS/grafana-13.0.1+security-01";
    QString hostUrl = "http://localhost:3000";
    bool showConsole = true;
    int startupTimeoutMs = 10000;
};

struct PressureArchiveConfig {
    // Master switch for archiving DDS pressure samples.
    bool enabled = true;

    // Fast DDS source selection for the pressure sample subscriber.
    int ddsDomainId = 0;
    QString ddsTopic = "afp.pressure.sample";

    // InfluxDB line protocol destination and source tag for pressure rows.
    QString table = "pressure_sensor";
    QString sourceTag = "pressure_sensor_1";
};

struct RobotMonitorArchiveConfig {
    // Master switch for archiving DDS robot monitor samples.
    bool enabled = true;

    // Fast DDS source selection for robot monitor samples.
    int ddsDomainId = 0;
    QString ddsTopic = "afp.robot.monitor.sample";

    // InfluxDB line protocol destination and stable source tag.
    QString table = "robot_monitor";
    QString sourceTag = "abb_irb4600";
};

class AppConfig {
public:
    RobotConfig robot;
    FtpConfig ftp;
    PlcConfig plc;
    EgmConfig egm;
    ThermalCameraConfig thermalCamera;
    HighSpeedCameraConfig highSpeedCamera;
    SerialConfig serial;
    PressureConfig pressure;
    TimeseriesArchiveConfig timeseriesArchive;
    GrafanaConfig grafana;
    PressureArchiveConfig pressureArchive;
    RobotMonitorArchiveConfig robotMonitorArchive;

    static AppConfig load(const AppPaths& paths);

    bool isLoaded() const;
    QStringList warnings() const;
    void addWarning(const QString& warning);

private:
    bool loaded_ = false;
    QStringList warnings_;
};
