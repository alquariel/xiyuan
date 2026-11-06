#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include "app/AppConfig.h"

class TestAppConfig : public QObject {
    Q_OBJECT

private slots:
    void loadsExpectedSections();
    void supportsStringTransport();
    void loadsRobotMonitorConfig();
    void loadsRobotMonitorDdsConfig();
    void invalidRobotMonitorConfigFallsBackToDefaults();
    void invalidRobotMonitorDdsConfigFallsBackToDefaults();
    void loadsRobotDefaultSpeedConfig();
    void missingRobotDefaultSpeedUsesPythonDefaults();
    void invalidRobotDefaultSpeedFallsBackToDefaults();
    void loadsPressureConfig();
    void invalidPressureConfigFallsBackToDefaults();
    void loadsTimeseriesArchiveConfig();
    void invalidTimeseriesArchiveConfigFallsBackToDefaults();
    void loadsGrafanaConfig();
    void invalidGrafanaConfigFallsBackToDefaults();
    void loadsPressureArchiveConfig();
    void loadsRobotMonitorArchiveConfig();
    void invalidPressureArchiveConfigFallsBackToDefaults();
    void invalidRobotMonitorArchiveConfigFallsBackToDefaults();
    void loadsThermalCameraRuntimeConfig();
    void invalidThermalCameraRuntimeConfigFallsBackToDefaults();
    void loadsHighSpeedCameraConfig();
    void invalidHighSpeedCameraConfigFallsBackToDefaults();
    void missingConfigUsesDefaults();
    void malformedConfigUsesDefaults();
    void nonObjectConfigUsesDefaults();
};

void TestAppConfig::loadsExpectedSections() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "robot": {"ip": "10.0.0.4", "port_motion": 6001, "transport": {"type": "socket"}},
        "ftp": {"ip": "10.0.0.5", "port": 2121, "user": "Operator", "password": "secret"},
        "plc": {"ip": "10.0.0.6", "rack": 2, "slot": 3},
        "thermal_camera": {"ip": "10.0.0.7", "port": 9000, "user": "thermal", "password": "thermal-pass"},
        "serial": {"general_baudrate": 115200, "fiber_laser": {"port": "COM9", "baudrate": 19200}}
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.robot.ip, QString("10.0.0.4"));
    QCOMPARE(config.robot.portMotion, 6001);
    QCOMPARE(config.robot.transportType, QString("socket"));
    QCOMPARE(config.ftp.ip, QString("10.0.0.5"));
    QCOMPARE(config.ftp.port, 2121);
    QCOMPARE(config.ftp.user, QString("Operator"));
    QCOMPARE(config.ftp.password, QString("secret"));
    QCOMPARE(config.plc.ip, QString("10.0.0.6"));
    QCOMPARE(config.plc.rack, 2);
    QCOMPARE(config.plc.slot, 3);
    QCOMPARE(config.thermalCamera.ip, QString("10.0.0.7"));
    QCOMPARE(config.thermalCamera.port, 9000);
    QCOMPARE(config.thermalCamera.user, QString("thermal"));
    QCOMPARE(config.thermalCamera.password, QString("thermal-pass"));
    QCOMPARE(config.serial.generalBaudrate, 115200);
    QCOMPARE(config.serial.fiberLaserPort, QString("COM9"));
    QCOMPARE(config.serial.fiberLaserBaudrate, 19200);
}

void TestAppConfig::supportsStringTransport() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({"robot": {"transport": "socket"}})";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.robot.transportType, QString("socket"));
}

void TestAppConfig::loadsRobotMonitorConfig() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "robot": {
            "monitor": {
                "enabled": false,
                "autostart": false,
                "port": 6102,
                "sample_interval_ms": 250,
                "record_csv": false,
                "record_raw_messages": false,
                "recording_dir": "logs/test_robot_monitor",
                "flush_every_sample": false
            }
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QVERIFY(!config.robot.monitor.enabled);
    QVERIFY(!config.robot.monitor.autostart);
    QCOMPARE(config.robot.monitor.port, 6102);
    QCOMPARE(config.robot.monitor.sampleIntervalMs, 250);
    QVERIFY(!config.robot.monitor.recordCsv);
    QVERIFY(!config.robot.monitor.recordRawMessages);
    QCOMPARE(config.robot.monitor.recordingDir, QString("logs/test_robot_monitor"));
    QVERIFY(!config.robot.monitor.flushEverySample);
}

void TestAppConfig::loadsRobotMonitorDdsConfig() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "robot": {
            "monitor": {
                "dds_publish_enabled": true,
                "dds_domain_id": 5,
                "dds_topic": "afp.test.robot.monitor"
            }
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QVERIFY(config.robot.monitor.ddsPublishEnabled);
    QCOMPARE(config.robot.monitor.ddsDomainId, 5);
    QCOMPARE(config.robot.monitor.ddsTopic, QString("afp.test.robot.monitor"));
}

void TestAppConfig::invalidRobotMonitorConfigFallsBackToDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "robot": {
            "monitor": {
                "port": 0,
                "sample_interval_ms": -1,
                "recording_dir": ""
            }
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QVERIFY(!config.robot.monitor.enabled);
    QVERIFY(!config.robot.monitor.autostart);
    QCOMPARE(config.robot.monitor.port, 5012);
    QCOMPARE(config.robot.monitor.sampleIntervalMs, 100);
    QVERIFY(!config.robot.monitor.recordCsv);
    QVERIFY(!config.robot.monitor.recordRawMessages);
    QCOMPARE(config.robot.monitor.recordingDir, QString("logs/robot_monitor"));
    QVERIFY(config.robot.monitor.flushEverySample);
    const QString warnings = config.warnings().join('\n');
    QVERIFY(warnings.contains("robot.monitor.port"));
    QVERIFY(warnings.contains("robot.monitor.sample_interval_ms"));
    QVERIFY(warnings.contains("robot.monitor.recording_dir"));
}

void TestAppConfig::invalidRobotMonitorDdsConfigFallsBackToDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "robot": {
            "monitor": {
                "dds_domain_id": -1,
                "dds_topic": ""
            }
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.robot.monitor.ddsPublishEnabled);
    QCOMPARE(config.robot.monitor.ddsDomainId, 0);
    QCOMPARE(config.robot.monitor.ddsTopic, QString("afp.robot.monitor.sample"));
    const QString warnings = config.warnings().join('\n');
    QVERIFY(warnings.contains("robot.monitor.dds_domain_id"));
    QVERIFY(warnings.contains("robot.monitor.dds_topic"));
}

void TestAppConfig::loadsRobotDefaultSpeedConfig() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "robot": {
            "default_speed": {
                "tcp_linear": 111.5,
                "tcp_orientation": 222.5,
                "external_linear": 333.5,
                "external_orientation": 44.5
            }
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.robot.defaultSpeed.tcpLinear, 111.5);
    QCOMPARE(config.robot.defaultSpeed.tcpOrientation, 222.5);
    QCOMPARE(config.robot.defaultSpeed.externalLinear, 333.5);
    QCOMPARE(config.robot.defaultSpeed.externalOrientation, 44.5);
}

void TestAppConfig::missingRobotDefaultSpeedUsesPythonDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({"robot": {"ip": "10.0.0.4"}})";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.robot.defaultSpeed.tcpLinear, 200.0);
    QCOMPARE(config.robot.defaultSpeed.tcpOrientation, 200.0);
    QCOMPARE(config.robot.defaultSpeed.externalLinear, 200.0);
    QCOMPARE(config.robot.defaultSpeed.externalOrientation, 100.0);
}

void TestAppConfig::invalidRobotDefaultSpeedFallsBackToDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "robot": {
            "default_speed": {
                "tcp_linear": 0,
                "tcp_orientation": -1,
                "external_linear": 0,
                "external_orientation": -2
            }
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.robot.defaultSpeed.tcpLinear, 200.0);
    QCOMPARE(config.robot.defaultSpeed.tcpOrientation, 200.0);
    QCOMPARE(config.robot.defaultSpeed.externalLinear, 200.0);
    QCOMPARE(config.robot.defaultSpeed.externalOrientation, 100.0);
    const QString warnings = config.warnings().join('\n');
    QVERIFY(warnings.contains("robot.default_speed.tcp_linear"));
    QVERIFY(warnings.contains("robot.default_speed.tcp_orientation"));
    QVERIFY(warnings.contains("robot.default_speed.external_linear"));
    QVERIFY(warnings.contains("robot.default_speed.external_orientation"));
}

void TestAppConfig::loadsPressureConfig() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "pressure": {
            "sample_interval_ms": 250,
            "processing_window_samples": 12,
            "dds_domain_id": 3,
            "dds_topic": "afp.test.pressure"
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.pressure.sampleIntervalMs, 250);
    QCOMPARE(config.pressure.processingWindowSamples, 12);
    QCOMPARE(config.pressure.ddsDomainId, 3);
    QCOMPARE(config.pressure.ddsTopic, QString("afp.test.pressure"));
}

void TestAppConfig::invalidPressureConfigFallsBackToDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "pressure": {
            "sample_interval_ms": 0,
            "processing_window_samples": -2,
            "dds_domain_id": -1,
            "dds_topic": ""
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.pressure.sampleIntervalMs, 100);
    QCOMPARE(config.pressure.processingWindowSamples, 40);
    QCOMPARE(config.pressure.ddsDomainId, 0);
    QCOMPARE(config.pressure.ddsTopic, QString("afp.pressure.sample"));
    QVERIFY(config.warnings().join('\n').contains("pressure.sample_interval_ms"));
    QVERIFY(config.warnings().join('\n').contains("pressure.processing_window_samples"));
    QVERIFY(config.warnings().join('\n').contains("pressure.dds_domain_id"));
    QVERIFY(config.warnings().join('\n').contains("pressure.dds_topic"));
}

void TestAppConfig::loadsTimeseriesArchiveConfig() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "timeseries_archive": {
            "enabled": true,
            "start_influxdb": true,
            "influxdb_show_console": true,
            "stop_influxdb_on_exit": false,
            "influxdb_exe_path": "D:/InfluxDB 3 Core/influxdb3.exe",
            "host_url": "http://127.0.0.1:8181",
            "node_id": "afp-node-test",
            "object_store": "file",
            "data_dir": "D:/InfluxDB 3 Core/test-data",
            "admin_token_file": "D:/InfluxDB 3 Core/afp-admin-token.json",
            "database": "afp_process_test",
            "retention_period": "7d",
            "ensure_database_on_start": true,
            "subscriber_autostart": true,
            "subscriber_executable": "archive/dds_influx_subscriber.exe",
            "subscriber_show_console": false,
            "subscriber_print_live_data": false,
            "write_batch_size": 10,
            "write_flush_interval_ms": 250,
            "http_request_timeout_ms": 1234,
            "max_pending_points": 4321,
            "reachability_probe_timeout_ms": 234,
            "influxdb_startup_timeout_ms": 3456,
            "cli_timeout_ms": 4567
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QVERIFY(config.timeseriesArchive.enabled);
    QVERIFY(config.timeseriesArchive.startInfluxdb);
    QVERIFY(config.timeseriesArchive.influxdbShowConsole);
    QVERIFY(!config.timeseriesArchive.stopInfluxdbOnExit);
    QCOMPARE(config.timeseriesArchive.influxdbExePath, QString("D:/InfluxDB 3 Core/influxdb3.exe"));
    QCOMPARE(config.timeseriesArchive.hostUrl, QString("http://127.0.0.1:8181"));
    QCOMPARE(config.timeseriesArchive.nodeId, QString("afp-node-test"));
    QCOMPARE(config.timeseriesArchive.objectStore, QString("file"));
    QCOMPARE(config.timeseriesArchive.dataDir, QString("D:/InfluxDB 3 Core/test-data"));
    QCOMPARE(config.timeseriesArchive.adminTokenFile, QString("D:/InfluxDB 3 Core/afp-admin-token.json"));
    QCOMPARE(config.timeseriesArchive.database, QString("afp_process_test"));
    QCOMPARE(config.timeseriesArchive.retentionPeriod, QString("7d"));
    QVERIFY(config.timeseriesArchive.ensureDatabaseOnStart);
    QVERIFY(config.timeseriesArchive.subscriberAutostart);
    QCOMPARE(config.timeseriesArchive.subscriberExecutable, QString("archive/dds_influx_subscriber.exe"));
    QVERIFY(!config.timeseriesArchive.subscriberShowConsole);
    QVERIFY(!config.timeseriesArchive.subscriberPrintLiveData);
    QCOMPARE(config.timeseriesArchive.writeBatchSize, 10);
    QCOMPARE(config.timeseriesArchive.writeFlushIntervalMs, 250);
    QCOMPARE(config.timeseriesArchive.httpRequestTimeoutMs, 1234);
    QCOMPARE(config.timeseriesArchive.maxPendingPoints, 4321);
    QCOMPARE(config.timeseriesArchive.reachabilityProbeTimeoutMs, 234);
    QCOMPARE(config.timeseriesArchive.influxdbStartupTimeoutMs, 3456);
    QCOMPARE(config.timeseriesArchive.cliTimeoutMs, 4567);
}

void TestAppConfig::invalidTimeseriesArchiveConfigFallsBackToDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "timeseries_archive": {
            "host_url": "",
            "node_id": "",
            "object_store": "",
            "database": "",
            "admin_token_file": "",
            "subscriber_executable": "",
            "write_batch_size": 0,
            "write_flush_interval_ms": -1,
            "http_request_timeout_ms": 0,
            "max_pending_points": -5,
            "reachability_probe_timeout_ms": 0,
            "influxdb_startup_timeout_ms": -5,
            "cli_timeout_ms": 0
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QCOMPARE(config.timeseriesArchive.hostUrl, QString("http://localhost:8181"));
    QVERIFY(config.timeseriesArchive.stopInfluxdbOnExit);
    QCOMPARE(config.timeseriesArchive.nodeId, QString("afp-node"));
    QCOMPARE(config.timeseriesArchive.objectStore, QString("file"));
    QCOMPARE(config.timeseriesArchive.database, QString("afp_process"));
    QCOMPARE(config.timeseriesArchive.adminTokenFile, QString("D:/InfluxDB 3 Core/afp-admin-token.json"));
    QCOMPARE(config.timeseriesArchive.subscriberExecutable, QString("dds_influx_subscriber.exe"));
    QCOMPARE(config.timeseriesArchive.writeBatchSize, 10);
    QCOMPARE(config.timeseriesArchive.writeFlushIntervalMs, 1000);
    QCOMPARE(config.timeseriesArchive.httpRequestTimeoutMs, 5000);
    QCOMPARE(config.timeseriesArchive.maxPendingPoints, 10000);
    QCOMPARE(config.timeseriesArchive.reachabilityProbeTimeoutMs, 500);
    QCOMPARE(config.timeseriesArchive.influxdbStartupTimeoutMs, 30000);
    QCOMPARE(config.timeseriesArchive.cliTimeoutMs, 10000);
    const QString warnings = config.warnings().join('\n');
    QVERIFY(warnings.contains("timeseries_archive.host_url"));
    QVERIFY(warnings.contains("timeseries_archive.subscriber_executable"));
    QVERIFY(warnings.contains("timeseries_archive.write_batch_size"));
    QVERIFY(warnings.contains("timeseries_archive.http_request_timeout_ms"));
    QVERIFY(warnings.contains("timeseries_archive.max_pending_points"));
    QVERIFY(warnings.contains("timeseries_archive.reachability_probe_timeout_ms"));
    QVERIFY(warnings.contains("timeseries_archive.influxdb_startup_timeout_ms"));
    QVERIFY(warnings.contains("timeseries_archive.cli_timeout_ms"));
}

void TestAppConfig::loadsGrafanaConfig() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "grafana": {
            "enabled": true,
            "autostart": true,
            "exe_path": "D:/Grafana OSS/grafana-13.0.1+security-01/bin/grafana.exe",
            "working_dir": "D:/Grafana OSS/grafana-13.0.1+security-01",
            "host_url": "http://127.0.0.1:3000",
            "show_console": true,
            "startup_timeout_ms": 12000
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.grafana.enabled);
    QVERIFY(config.grafana.autostart);
    QCOMPARE(config.grafana.exePath, QString("D:/Grafana OSS/grafana-13.0.1+security-01/bin/grafana.exe"));
    QCOMPARE(config.grafana.workingDir, QString("D:/Grafana OSS/grafana-13.0.1+security-01"));
    QCOMPARE(config.grafana.hostUrl, QString("http://127.0.0.1:3000"));
    QVERIFY(config.grafana.showConsole);
    QCOMPARE(config.grafana.startupTimeoutMs, 12000);
}

void TestAppConfig::invalidGrafanaConfigFallsBackToDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "grafana": {
            "exe_path": "",
            "working_dir": "",
            "host_url": "",
            "startup_timeout_ms": 0
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QCOMPARE(config.grafana.exePath, QString("D:/Grafana OSS/grafana-13.0.1+security-01/bin/grafana.exe"));
    QCOMPARE(config.grafana.workingDir, QString("D:/Grafana OSS/grafana-13.0.1+security-01"));
    QCOMPARE(config.grafana.hostUrl, QString("http://localhost:3000"));
    QCOMPARE(config.grafana.startupTimeoutMs, 10000);
    const QString warnings = config.warnings().join('\n');
    QVERIFY(warnings.contains("grafana.exe_path"));
    QVERIFY(warnings.contains("grafana.working_dir"));
    QVERIFY(warnings.contains("grafana.host_url"));
    QVERIFY(warnings.contains("grafana.startup_timeout_ms"));
}

void TestAppConfig::loadsPressureArchiveConfig() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "pressure_archive": {
            "enabled": true,
            "dds_domain_id": 2,
            "dds_topic": "afp.test.pressure",
            "table": "pressure_sensor_test",
            "source_tag": "pressure_sensor_lab"
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QVERIFY(config.pressureArchive.enabled);
    QCOMPARE(config.pressureArchive.ddsDomainId, 2);
    QCOMPARE(config.pressureArchive.ddsTopic, QString("afp.test.pressure"));
    QCOMPARE(config.pressureArchive.table, QString("pressure_sensor_test"));
    QCOMPARE(config.pressureArchive.sourceTag, QString("pressure_sensor_lab"));
}

void TestAppConfig::loadsRobotMonitorArchiveConfig() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "robot_monitor_archive": {
            "enabled": true,
            "dds_domain_id": 6,
            "dds_topic": "afp.test.robot.monitor",
            "table": "robot_monitor_test",
            "source_tag": "abb_test_cell"
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.robotMonitorArchive.enabled);
    QCOMPARE(config.robotMonitorArchive.ddsDomainId, 6);
    QCOMPARE(config.robotMonitorArchive.ddsTopic, QString("afp.test.robot.monitor"));
    QCOMPARE(config.robotMonitorArchive.table, QString("robot_monitor_test"));
    QCOMPARE(config.robotMonitorArchive.sourceTag, QString("abb_test_cell"));
}

void TestAppConfig::invalidPressureArchiveConfigFallsBackToDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "pressure_archive": {
            "dds_domain_id": -2,
            "dds_topic": "",
            "table": "",
            "source_tag": ""
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QCOMPARE(config.pressureArchive.ddsDomainId, 0);
    QCOMPARE(config.pressureArchive.ddsTopic, QString("afp.pressure.sample"));
    QCOMPARE(config.pressureArchive.table, QString("pressure_sensor"));
    QCOMPARE(config.pressureArchive.sourceTag, QString("pressure_sensor_1"));
    const QString warnings = config.warnings().join('\n');
    QVERIFY(warnings.contains("pressure_archive.dds_domain_id"));
    QVERIFY(warnings.contains("pressure_archive.dds_topic"));
}

void TestAppConfig::invalidRobotMonitorArchiveConfigFallsBackToDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "robot_monitor_archive": {
            "dds_domain_id": -2,
            "dds_topic": "",
            "table": "",
            "source_tag": ""
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.robotMonitorArchive.enabled);
    QCOMPARE(config.robotMonitorArchive.ddsDomainId, 0);
    QCOMPARE(config.robotMonitorArchive.ddsTopic, QString("afp.robot.monitor.sample"));
    QCOMPARE(config.robotMonitorArchive.table, QString("robot_monitor"));
    QCOMPARE(config.robotMonitorArchive.sourceTag, QString("abb_irb4600"));
    const QString warnings = config.warnings().join('\n');
    QVERIFY(warnings.contains("robot_monitor_archive.dds_domain_id"));
    QVERIFY(warnings.contains("robot_monitor_archive.dds_topic"));
    QVERIFY(warnings.contains("robot_monitor_archive.table"));
    QVERIFY(warnings.contains("robot_monitor_archive.source_tag"));
}

void TestAppConfig::loadsThermalCameraRuntimeConfig() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "thermal_camera": {
            "ip": "10.0.0.77",
            "port": 8100,
            "user": "operator",
            "password": "pass",
            "sdk_dir": "D:/Hikvision/Thermal",
            "portable_sdk_dir": "lib/sensors/thermal_camera/custom",
            "cache_max_files": 12,
            "display_interval_ms": 500
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.thermalCamera.ip, QString("10.0.0.77"));
    QCOMPARE(config.thermalCamera.port, 8100);
    QCOMPARE(config.thermalCamera.user, QString("operator"));
    QCOMPARE(config.thermalCamera.password, QString("pass"));
    QCOMPARE(config.thermalCamera.sdkDir, QString("D:/Hikvision/Thermal"));
    QCOMPARE(config.thermalCamera.portableSdkDir, QString("lib/sensors/thermal_camera/custom"));
    QCOMPARE(config.thermalCamera.cacheMaxFiles, 12);
    QCOMPARE(config.thermalCamera.displayIntervalMs, 500);
}

void TestAppConfig::invalidThermalCameraRuntimeConfigFallsBackToDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "thermal_camera": {
            "sdk_dir": "",
            "portable_sdk_dir": "",
            "cache_max_files": 0,
            "display_interval_ms": -1
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.thermalCamera.sdkDir, QString(""));
    QCOMPARE(config.thermalCamera.portableSdkDir, QString("lib/sensors/thermal_camera/lib/win"));
    QCOMPARE(config.thermalCamera.cacheMaxFiles, 60);
    QCOMPARE(config.thermalCamera.displayIntervalMs, 1000);
    const QString warnings = config.warnings().join('\n');
    QVERIFY(warnings.contains("thermal_camera.portable_sdk_dir"));
    QVERIFY(warnings.contains("thermal_camera.cache_max_files"));
    QVERIFY(warnings.contains("thermal_camera.display_interval_ms"));
}

void TestAppConfig::loadsHighSpeedCameraConfig() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "high_speed_camera": {
            "sdk_dll_path": "D:/Camera/MvCameraControl.dll",
            "portable_sdk_dll_path": "lib/sensors/high_speed_camera/custom.dll",
            "installed_sdk_dll_path": "C:/MVS/MvCameraControl.dll",
            "frame_timeout_ms": 250
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.highSpeedCamera.sdkDllPath, QString("D:/Camera/MvCameraControl.dll"));
    QCOMPARE(config.highSpeedCamera.portableSdkDllPath, QString("lib/sensors/high_speed_camera/custom.dll"));
    QCOMPARE(config.highSpeedCamera.installedSdkDllPath, QString("C:/MVS/MvCameraControl.dll"));
    QCOMPARE(config.highSpeedCamera.frameTimeoutMs, 250);
}

void TestAppConfig::invalidHighSpeedCameraConfigFallsBackToDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << R"({
        "high_speed_camera": {
            "sdk_dll_path": "",
            "portable_sdk_dll_path": "",
            "installed_sdk_dll_path": "",
            "frame_timeout_ms": 0
        }
    })";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(config.isLoaded());
    QCOMPARE(config.highSpeedCamera.sdkDllPath, QString(""));
    QCOMPARE(config.highSpeedCamera.portableSdkDllPath, QString("lib/sensors/high_speed_camera/MvCameraControl.dll"));
    QCOMPARE(config.highSpeedCamera.installedSdkDllPath, QString("C:/Program Files (x86)/Common Files/MVS/Runtime/Win64_x64/MvCameraControl.dll"));
    QCOMPARE(config.highSpeedCamera.frameTimeoutMs, 1000);
    const QString warnings = config.warnings().join('\n');
    QVERIFY(warnings.contains("high_speed_camera.portable_sdk_dll_path"));
    QVERIFY(warnings.contains("high_speed_camera.installed_sdk_dll_path"));
    QVERIFY(warnings.contains("high_speed_camera.frame_timeout_ms"));
}

void TestAppConfig::missingConfigUsesDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    const AppConfig config = AppConfig::load(AppPaths(tempDir.path()));

    QVERIFY(!config.isLoaded());
    QVERIFY(!config.warnings().isEmpty());
    QCOMPARE(config.robot.ip, QString("192.168.0.104"));
    QCOMPARE(config.plc.ip, QString("192.168.0.201"));
}

void TestAppConfig::malformedConfigUsesDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << "{ invalid json";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(!config.isLoaded());
    QVERIFY(!config.warnings().isEmpty());
    QCOMPARE(config.robot.ip, QString("192.168.0.104"));
    QCOMPARE(config.plc.ip, QString("192.168.0.201"));
}

void TestAppConfig::nonObjectConfigUsesDefaults() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    QDir root(tempDir.path());
    QVERIFY(root.mkpath("config"));

    QFile file(root.filePath("config/env_config.json"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QTextStream stream(&file);
    stream << "[]";
    file.close();

    const AppConfig config = AppConfig::load(AppPaths(root.absolutePath()));

    QVERIFY(!config.isLoaded());
    QVERIFY(config.warnings().join('\n').contains("root must be a JSON object"));
    QCOMPARE(config.robot.ip, QString("192.168.0.104"));
    QCOMPARE(config.plc.ip, QString("192.168.0.201"));
}

QTEST_MAIN(TestAppConfig)
#include "test_app_config.moc"
