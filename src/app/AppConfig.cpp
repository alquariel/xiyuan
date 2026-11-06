#include "app/AppConfig.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
QString readString(const QJsonObject& object, const QString& key, const QString& fallback) {
    const QJsonValue value = object.value(key);
    return value.isString() ? value.toString() : fallback;
}

int readInt(const QJsonObject& object, const QString& key, int fallback) {
    const QJsonValue value = object.value(key);
    return value.isDouble() ? value.toInt() : fallback;
}

bool readBool(const QJsonObject& object, const QString& key, bool fallback) {
    const QJsonValue value = object.value(key);
    return value.isBool() ? value.toBool() : fallback;
}

int readPositiveInt(AppConfig& config,
                    const QJsonObject& object,
                    const QString& key,
                    int fallback,
                    const QString& warningName) {
    const QJsonValue value = object.value(key);
    if (!value.isDouble()) {
        return fallback;
    }
    const int parsed = value.toInt();
    if (parsed <= 0) {
        config.addWarning(QString("%1 must be greater than 0; using %2").arg(warningName).arg(fallback));
        return fallback;
    }
    return parsed;
}

double readPositiveDouble(AppConfig& config,
                          const QJsonObject& object,
                          const QString& key,
                          double fallback,
                          const QString& warningName) {
    const QJsonValue value = object.value(key);
    if (!value.isDouble()) {
        return fallback;
    }
    const double parsed = value.toDouble();
    if (parsed <= 0.0) {
        config.addWarning(QString("%1 must be greater than 0; using %2").arg(warningName).arg(fallback));
        return fallback;
    }
    return parsed;
}

int readNonNegativeInt(AppConfig& config,
                       const QJsonObject& object,
                       const QString& key,
                       int fallback,
                       const QString& warningName) {
    const QJsonValue value = object.value(key);
    if (!value.isDouble()) {
        return fallback;
    }
    const int parsed = value.toInt();
    if (parsed < 0) {
        config.addWarning(QString("%1 must be non-negative; using %2").arg(warningName).arg(fallback));
        return fallback;
    }
    return parsed;
}

QString readNonEmptyString(AppConfig& config,
                           const QJsonObject& object,
                           const QString& key,
                           const QString& fallback,
                           const QString& warningName) {
    const QJsonValue value = object.value(key);
    if (!value.isString()) {
        return fallback;
    }
    const QString parsed = value.toString().trimmed();
    if (parsed.isEmpty()) {
        config.addWarning(QString("%1 must be non-empty; using %2").arg(warningName, fallback));
        return fallback;
    }
    return parsed;
}
}

AppConfig AppConfig::load(const AppPaths& paths) {
    AppConfig config;

    QFile file(paths.configFilePath().absoluteFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        config.warnings_ << QString("Could not open %1").arg(paths.configFilePath().absoluteFilePath());
        return config;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        config.warnings_ << QString("Could not parse env_config.json: %1").arg(error.errorString());
        return config;
    }
    if (!document.isObject()) {
        config.warnings_ << "env_config.json root must be a JSON object";
        return config;
    }

    const QJsonObject root = document.object();

    const QJsonObject robot = root.value("robot").toObject();
    config.robot.ip = readString(robot, "ip", config.robot.ip);
    config.robot.portMotion = readInt(robot, "port_motion", config.robot.portMotion);
    const QJsonValue transportValue = robot.value("transport");
    if (transportValue.isObject()) {
        config.robot.transportType = readString(transportValue.toObject(), "type", config.robot.transportType);
    } else if (transportValue.isString()) {
        config.robot.transportType = transportValue.toString();
    }
    const QJsonObject defaultSpeed = robot.value("default_speed").toObject();
    config.robot.defaultSpeed.tcpLinear = readPositiveDouble(
        config, defaultSpeed, "tcp_linear", config.robot.defaultSpeed.tcpLinear, "robot.default_speed.tcp_linear");
    config.robot.defaultSpeed.tcpOrientation = readPositiveDouble(
        config, defaultSpeed, "tcp_orientation", config.robot.defaultSpeed.tcpOrientation, "robot.default_speed.tcp_orientation");
    config.robot.defaultSpeed.externalLinear = readPositiveDouble(
        config, defaultSpeed, "external_linear", config.robot.defaultSpeed.externalLinear, "robot.default_speed.external_linear");
    config.robot.defaultSpeed.externalOrientation = readPositiveDouble(
        config, defaultSpeed, "external_orientation", config.robot.defaultSpeed.externalOrientation, "robot.default_speed.external_orientation");
    const QJsonObject robotMonitor = robot.value("monitor").toObject();
    config.robot.monitor.enabled = readBool(robotMonitor, "enabled", config.robot.monitor.enabled);
    config.robot.monitor.autostart = readBool(robotMonitor, "autostart", config.robot.monitor.autostart);
    config.robot.monitor.port = readPositiveInt(
        config, robotMonitor, "port", config.robot.monitor.port, "robot.monitor.port");
    config.robot.monitor.sampleIntervalMs = readPositiveInt(
        config,
        robotMonitor,
        "sample_interval_ms",
        config.robot.monitor.sampleIntervalMs,
        "robot.monitor.sample_interval_ms");
    config.robot.monitor.ddsPublishEnabled = readBool(
        robotMonitor, "dds_publish_enabled", config.robot.monitor.ddsPublishEnabled);
    config.robot.monitor.ddsDomainId = readNonNegativeInt(
        config,
        robotMonitor,
        "dds_domain_id",
        config.robot.monitor.ddsDomainId,
        "robot.monitor.dds_domain_id");
    config.robot.monitor.ddsTopic = readNonEmptyString(
        config, robotMonitor, "dds_topic", config.robot.monitor.ddsTopic, "robot.monitor.dds_topic");
    config.robot.monitor.recordCsv = readBool(robotMonitor, "record_csv", config.robot.monitor.recordCsv);
    config.robot.monitor.recordRawMessages = readBool(
        robotMonitor, "record_raw_messages", config.robot.monitor.recordRawMessages);
    config.robot.monitor.recordingDir = readNonEmptyString(
        config,
        robotMonitor,
        "recording_dir",
        config.robot.monitor.recordingDir,
        "robot.monitor.recording_dir");
    config.robot.monitor.flushEverySample = readBool(
        robotMonitor, "flush_every_sample", config.robot.monitor.flushEverySample);

    const QJsonObject ftp = root.value("ftp").toObject();
    config.ftp.ip = readString(ftp, "ip", config.ftp.ip);
    config.ftp.port = readInt(ftp, "port", config.ftp.port);
    config.ftp.user = readString(ftp, "user", config.ftp.user);
    config.ftp.password = readString(ftp, "password", config.ftp.password);

    const QJsonObject plc = root.value("plc").toObject();
    config.plc.ip = readString(plc, "ip", config.plc.ip);
    config.plc.rack = readInt(plc, "rack", config.plc.rack);
    config.plc.slot = readInt(plc, "slot", config.plc.slot);

    const QJsonObject thermal = root.value("thermal_camera").toObject();
    config.thermalCamera.ip = readString(thermal, "ip", config.thermalCamera.ip);
    config.thermalCamera.port = readInt(thermal, "port", config.thermalCamera.port);
    config.thermalCamera.user = readString(thermal, "user", config.thermalCamera.user);
    config.thermalCamera.password = readString(thermal, "password", config.thermalCamera.password);
    config.thermalCamera.sdkDir = readString(thermal, "sdk_dir", config.thermalCamera.sdkDir).trimmed();
    config.thermalCamera.portableSdkDir = readNonEmptyString(
        config,
        thermal,
        "portable_sdk_dir",
        config.thermalCamera.portableSdkDir,
        "thermal_camera.portable_sdk_dir");
    config.thermalCamera.cacheMaxFiles = readPositiveInt(
        config,
        thermal,
        "cache_max_files",
        config.thermalCamera.cacheMaxFiles,
        "thermal_camera.cache_max_files");
    config.thermalCamera.displayIntervalMs = readPositiveInt(
        config,
        thermal,
        "display_interval_ms",
        config.thermalCamera.displayIntervalMs,
        "thermal_camera.display_interval_ms");

    const QJsonObject highSpeedCamera = root.value("high_speed_camera").toObject();
    config.highSpeedCamera.sdkDllPath = readString(
        highSpeedCamera, "sdk_dll_path", config.highSpeedCamera.sdkDllPath).trimmed();
    config.highSpeedCamera.portableSdkDllPath = readNonEmptyString(
        config,
        highSpeedCamera,
        "portable_sdk_dll_path",
        config.highSpeedCamera.portableSdkDllPath,
        "high_speed_camera.portable_sdk_dll_path");
    config.highSpeedCamera.installedSdkDllPath = readNonEmptyString(
        config,
        highSpeedCamera,
        "installed_sdk_dll_path",
        config.highSpeedCamera.installedSdkDllPath,
        "high_speed_camera.installed_sdk_dll_path");
    config.highSpeedCamera.frameTimeoutMs = readPositiveInt(
        config,
        highSpeedCamera,
        "frame_timeout_ms",
        config.highSpeedCamera.frameTimeoutMs,
        "high_speed_camera.frame_timeout_ms");

    const QJsonObject serial = root.value("serial").toObject();
    config.serial.generalBaudrate = readInt(serial, "general_baudrate", config.serial.generalBaudrate);
    const QJsonObject fiberLaser = serial.value("fiber_laser").toObject();
    config.serial.fiberLaserPort = readString(fiberLaser, "port", config.serial.fiberLaserPort);
    config.serial.fiberLaserBaudrate = readInt(fiberLaser, "baudrate", config.serial.fiberLaserBaudrate);

    const QJsonObject pressure = root.value("pressure").toObject();
    config.pressure.sampleIntervalMs = readPositiveInt(
        config, pressure, "sample_interval_ms", config.pressure.sampleIntervalMs, "pressure.sample_interval_ms");
    config.pressure.processingWindowSamples = readPositiveInt(
        config, pressure, "processing_window_samples", config.pressure.processingWindowSamples, "pressure.processing_window_samples");
    config.pressure.ddsDomainId = readNonNegativeInt(
        config, pressure, "dds_domain_id", config.pressure.ddsDomainId, "pressure.dds_domain_id");
    config.pressure.ddsTopic = readNonEmptyString(
        config, pressure, "dds_topic", config.pressure.ddsTopic, "pressure.dds_topic");

    const QJsonObject timeseriesArchive = root.value("timeseries_archive").toObject();
    config.timeseriesArchive.enabled = readBool(
        timeseriesArchive, "enabled", config.timeseriesArchive.enabled);
    config.timeseriesArchive.startInfluxdb = readBool(
        timeseriesArchive, "start_influxdb", config.timeseriesArchive.startInfluxdb);
    config.timeseriesArchive.influxdbShowConsole = readBool(
        timeseriesArchive, "influxdb_show_console", config.timeseriesArchive.influxdbShowConsole);
    config.timeseriesArchive.stopInfluxdbOnExit = readBool(
        timeseriesArchive, "stop_influxdb_on_exit", config.timeseriesArchive.stopInfluxdbOnExit);
    config.timeseriesArchive.influxdbExePath = readNonEmptyString(
        config,
        timeseriesArchive,
        "influxdb_exe_path",
        config.timeseriesArchive.influxdbExePath,
        "timeseries_archive.influxdb_exe_path");
    config.timeseriesArchive.hostUrl = readNonEmptyString(
        config, timeseriesArchive, "host_url", config.timeseriesArchive.hostUrl, "timeseries_archive.host_url");
    config.timeseriesArchive.nodeId = readNonEmptyString(
        config, timeseriesArchive, "node_id", config.timeseriesArchive.nodeId, "timeseries_archive.node_id");
    config.timeseriesArchive.objectStore = readNonEmptyString(
        config, timeseriesArchive, "object_store", config.timeseriesArchive.objectStore, "timeseries_archive.object_store");
    config.timeseriesArchive.dataDir = readNonEmptyString(
        config, timeseriesArchive, "data_dir", config.timeseriesArchive.dataDir, "timeseries_archive.data_dir");
    config.timeseriesArchive.adminTokenFile = readNonEmptyString(
        config,
        timeseriesArchive,
        "admin_token_file",
        config.timeseriesArchive.adminTokenFile,
        "timeseries_archive.admin_token_file");
    config.timeseriesArchive.database = readNonEmptyString(
        config, timeseriesArchive, "database", config.timeseriesArchive.database, "timeseries_archive.database");
    config.timeseriesArchive.retentionPeriod = readNonEmptyString(
        config,
        timeseriesArchive,
        "retention_period",
        config.timeseriesArchive.retentionPeriod,
        "timeseries_archive.retention_period");
    config.timeseriesArchive.ensureDatabaseOnStart = readBool(
        timeseriesArchive, "ensure_database_on_start", config.timeseriesArchive.ensureDatabaseOnStart);
    config.timeseriesArchive.subscriberAutostart = readBool(
        timeseriesArchive, "subscriber_autostart", config.timeseriesArchive.subscriberAutostart);
    config.timeseriesArchive.subscriberExecutable = readNonEmptyString(
        config,
        timeseriesArchive,
        "subscriber_executable",
        config.timeseriesArchive.subscriberExecutable,
        "timeseries_archive.subscriber_executable");
    config.timeseriesArchive.subscriberShowConsole = readBool(
        timeseriesArchive, "subscriber_show_console", config.timeseriesArchive.subscriberShowConsole);
    config.timeseriesArchive.subscriberPrintLiveData = readBool(
        timeseriesArchive, "subscriber_print_live_data", config.timeseriesArchive.subscriberPrintLiveData);
    config.timeseriesArchive.writeBatchSize = readPositiveInt(
        config,
        timeseriesArchive,
        "write_batch_size",
        config.timeseriesArchive.writeBatchSize,
        "timeseries_archive.write_batch_size");
    config.timeseriesArchive.writeFlushIntervalMs = readPositiveInt(
        config,
        timeseriesArchive,
        "write_flush_interval_ms",
        config.timeseriesArchive.writeFlushIntervalMs,
        "timeseries_archive.write_flush_interval_ms");
    config.timeseriesArchive.httpRequestTimeoutMs = readPositiveInt(
        config,
        timeseriesArchive,
        "http_request_timeout_ms",
        config.timeseriesArchive.httpRequestTimeoutMs,
        "timeseries_archive.http_request_timeout_ms");
    config.timeseriesArchive.maxPendingPoints = readPositiveInt(
        config,
        timeseriesArchive,
        "max_pending_points",
        config.timeseriesArchive.maxPendingPoints,
        "timeseries_archive.max_pending_points");
    config.timeseriesArchive.reachabilityProbeTimeoutMs = readPositiveInt(
        config,
        timeseriesArchive,
        "reachability_probe_timeout_ms",
        config.timeseriesArchive.reachabilityProbeTimeoutMs,
        "timeseries_archive.reachability_probe_timeout_ms");
    config.timeseriesArchive.influxdbStartupTimeoutMs = readPositiveInt(
        config,
        timeseriesArchive,
        "influxdb_startup_timeout_ms",
        config.timeseriesArchive.influxdbStartupTimeoutMs,
        "timeseries_archive.influxdb_startup_timeout_ms");
    config.timeseriesArchive.cliTimeoutMs = readPositiveInt(
        config,
        timeseriesArchive,
        "cli_timeout_ms",
        config.timeseriesArchive.cliTimeoutMs,
        "timeseries_archive.cli_timeout_ms");

    const QJsonObject grafana = root.value("grafana").toObject();
    config.grafana.enabled = readBool(grafana, "enabled", config.grafana.enabled);
    config.grafana.autostart = readBool(grafana, "autostart", config.grafana.autostart);
    config.grafana.exePath = readNonEmptyString(
        config, grafana, "exe_path", config.grafana.exePath, "grafana.exe_path");
    config.grafana.workingDir = readNonEmptyString(
        config, grafana, "working_dir", config.grafana.workingDir, "grafana.working_dir");
    config.grafana.hostUrl = readNonEmptyString(
        config, grafana, "host_url", config.grafana.hostUrl, "grafana.host_url");
    config.grafana.showConsole = readBool(grafana, "show_console", config.grafana.showConsole);
    config.grafana.startupTimeoutMs = readPositiveInt(
        config, grafana, "startup_timeout_ms", config.grafana.startupTimeoutMs, "grafana.startup_timeout_ms");

    const QJsonObject pressureArchive = root.value("pressure_archive").toObject();
    config.pressureArchive.enabled = readBool(pressureArchive, "enabled", config.pressureArchive.enabled);
    config.pressureArchive.ddsDomainId = readNonNegativeInt(
        config,
        pressureArchive,
        "dds_domain_id",
        config.pressureArchive.ddsDomainId,
        "pressure_archive.dds_domain_id");
    config.pressureArchive.ddsTopic = readNonEmptyString(
        config, pressureArchive, "dds_topic", config.pressureArchive.ddsTopic, "pressure_archive.dds_topic");
    config.pressureArchive.table = readNonEmptyString(
        config, pressureArchive, "table", config.pressureArchive.table, "pressure_archive.table");
    config.pressureArchive.sourceTag = readNonEmptyString(
        config, pressureArchive, "source_tag", config.pressureArchive.sourceTag, "pressure_archive.source_tag");

    const QJsonObject robotMonitorArchive = root.value("robot_monitor_archive").toObject();
    config.robotMonitorArchive.enabled = readBool(
        robotMonitorArchive, "enabled", config.robotMonitorArchive.enabled);
    config.robotMonitorArchive.ddsDomainId = readNonNegativeInt(
        config,
        robotMonitorArchive,
        "dds_domain_id",
        config.robotMonitorArchive.ddsDomainId,
        "robot_monitor_archive.dds_domain_id");
    config.robotMonitorArchive.ddsTopic = readNonEmptyString(
        config,
        robotMonitorArchive,
        "dds_topic",
        config.robotMonitorArchive.ddsTopic,
        "robot_monitor_archive.dds_topic");
    config.robotMonitorArchive.table = readNonEmptyString(
        config,
        robotMonitorArchive,
        "table",
        config.robotMonitorArchive.table,
        "robot_monitor_archive.table");
    config.robotMonitorArchive.sourceTag = readNonEmptyString(
        config,
        robotMonitorArchive,
        "source_tag",
        config.robotMonitorArchive.sourceTag,
        "robot_monitor_archive.source_tag");

    config.loaded_ = true;
    return config;
}

bool AppConfig::isLoaded() const {
    return loaded_;
}

QStringList AppConfig::warnings() const {
    return warnings_;
}

void AppConfig::addWarning(const QString& warning) {
    warnings_ << warning;
}
