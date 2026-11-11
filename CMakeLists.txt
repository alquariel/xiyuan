cmake_minimum_required(VERSION 3.20)

project(AFP_ABB_CPP VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/src/ui)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Widgets Charts Test)
find_package(snap7 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fastdds CONFIG REQUIRED)

option(AFP_ABB_REQUIRE_APP_TARGET "Fail configure if the main application target cannot be created." OFF)

set(THERMAL_CAMERA_RUNTIME_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/sensors/thermal_camera/lib/win")

function(copy_qt_test_runtime TARGET_NAME)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt6::Core>
                $<TARGET_FILE:Qt6::Test>
                $<TARGET_FILE_DIR:${TARGET_NAME}>
    )
endfunction()

function(copy_qt_widget_test_runtime TARGET_NAME)
    copy_qt_test_runtime(${TARGET_NAME})
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt6::Gui>
                $<TARGET_FILE:Qt6::Widgets>
                $<TARGET_FILE_DIR:${TARGET_NAME}>
        COMMAND ${CMAKE_COMMAND} -E make_directory
                $<TARGET_FILE_DIR:${TARGET_NAME}>/platforms
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt6::QWindowsIntegrationPlugin>
                $<TARGET_FILE_DIR:${TARGET_NAME}>/platforms
    )
endfunction()

function(copy_qt_jpeg_test_runtime TARGET_NAME)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                $<TARGET_FILE_DIR:${TARGET_NAME}>/imageformats
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt6::QJpegPlugin>
                $<TARGET_FILE_DIR:${TARGET_NAME}>/imageformats
    )
endfunction()

set(APP_SOURCES
    src/main.cpp
    src/app/AppPaths.h
    src/app/AppPaths.cpp
    src/app/AppConfig.h
    src/app/AppConfig.cpp
    src/app/StatusLogger.h
    src/app/StatusLogger.cpp
    src/archive/InfluxAdmin.h
    src/archive/InfluxAdmin.cpp
    src/archive/InfluxLineProtocolWriter.h
    src/archive/InfluxLineProtocolWriter.cpp
    src/archive/PressureSampleReader.h
    src/archive/PressureSampleReader.cpp
    src/archive/RobotMonitorSampleReader.h
    src/archive/RobotMonitorSampleReader.cpp
    src/archive/TimeSeriesPoint.h
    src/archive/TimeSeriesPoint.cpp
    src/services/archive/TimeseriesArchiveService.h
    src/services/archive/TimeseriesArchiveService.cpp
    src/services/grafana/GrafanaStartupService.h
    src/services/grafana/GrafanaStartupService.cpp
    src/ftp/FtpClient.h
    src/ftp/FtpClient.cpp
    src/robot/RobotSocketClient.h
    src/robot/RobotSocketClient.cpp
    src/robot_monitor/RobotMonitorSample.h
    src/robot_monitor/RobotMonitorMessageParser.h
    src/robot_monitor/RobotMonitorMessageParser.cpp
    src/robot_monitor/RobotMonitorRecorder.h
    src/robot_monitor/RobotMonitorRecorder.cpp
    src/robot_monitor/RobotMonitorClient.h
    src/robot_monitor/RobotMonitorClient.cpp
    src/robot_monitor/RobotMonitorDdsTypes.h
    src/robot_monitor/RobotMonitorDdsPublisher.h
    src/robot_monitor/RobotMonitorDdsPublisher.cpp
    src/robot/RobotQuaternion.h
    src/robot/RobotQuaternion.cpp
    src/robot/PlcRobotMotionClient.h
    src/robot/PlcRobotMotionClient.cpp
    src/robot/SocketRobotMotionClient.h
    src/robot/SocketRobotMotionClient.cpp
    src/robot/EgmClient.h
    src/robot/EgmClient.cpp
    src/robot/PathLoader.h
    src/robot/PathLoader.cpp
    src/plc/S7Data.h
    src/plc/S7Data.cpp
    src/plc/PlcClient.h
    src/plc/PlcClient.cpp
    src/plc/PlcPrintSignalManager.h
    src/plc/PlcPrintSignalManager.cpp
    src/pressure/PressureDdsTypes.h
    src/pressure/PressureDdsPublisher.h
    src/pressure/PressureDdsPublisher.cpp
    src/pressure/PressureAcquisitionWorker.h
    src/pressure/PressureAcquisitionWorker.cpp
    src/pressure/PressureSensorClient.h
    src/pressure/PressureSensorClient.cpp
    src/pressure/PressureStatistics.h
    src/pressure/PressureStatistics.cpp
    src/camera/HighSpeedCameraTypes.h
    src/camera/IHighSpeedCameraClient.h
    src/camera/HighSpeedCameraClient.h
    src/camera/HighSpeedCameraClient.cpp
    src/thermal_camera/ThermalCameraTypes.h
    src/thermal_camera/IThermalCameraClient.h
    src/thermal_camera/ThermalFrameStore.h
    src/thermal_camera/ThermalFrameStore.cpp
    src/thermal_camera/ThermalCameraClient.h
    src/thermal_camera/ThermalCameraClient.cpp
    src/pressure/generated/PressureSample.hpp
    src/pressure/generated/PressureSamplePubSubTypes.cxx
    src/pressure/generated/PressureSampleTypeObjectSupport.cxx
    src/robot_monitor/RobotMonitorSample.idl
    src/robot_monitor/generated/RobotMonitorSample.hpp
    src/robot_monitor/generated/RobotMonitorSamplePubSubTypes.cxx
    src/robot_monitor/generated/RobotMonitorSampleTypeObjectSupport.cxx
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/PressureChartWindow.h
    src/ui/PressureChartWindow.cpp
    src/services/PlaceholderServices.h
    src/services/PlaceholderServices.cpp
    src/services/ftp/FtpService.h
    src/services/ftp/FtpService.cpp
    src/services/plc/PlcService.h
    src/services/plc/PlcService.cpp
    src/services/pressure/PressureService.h
    src/services/pressure/PressureService.cpp
    src/pressure/PressureSimulator.h
    src/pressure/PressureSimulator.cpp
    src/services/camera/HighSpeedCameraService.h
    src/services/camera/HighSpeedCameraService.cpp
    src/services/thermal_camera/ThermalCameraService.h
    src/services/thermal_camera/ThermalCameraService.cpp
    src/services/robot/RobotPrintInterfaces.h
    src/services/robot/RobotPrintService.h
    src/services/robot/RobotPrintService.cpp
    src/services/robot/RobotMovementService.h
    src/services/robot/RobotMovementService.cpp
    src/services/robot_monitor/RobotMonitoringService.h
    src/services/robot_monitor/RobotMonitoringService.cpp
    src/ui/Interface4600_window.ui
)

set(APP_TARGET_READY TRUE)
set(MISSING_APP_SOURCES)
foreach(APP_SOURCE ${APP_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${APP_SOURCE}")
        set(APP_TARGET_READY FALSE)
        list(APPEND MISSING_APP_SOURCES ${APP_SOURCE})
    endif()
endforeach()

if(APP_TARGET_READY)
    qt_add_executable(AFP_ABB_CPP WIN32 ${APP_SOURCES})

    target_include_directories(AFP_ABB_CPP PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}/AFP_ABB_CPP_autogen/include
    )

    target_link_libraries(AFP_ABB_CPP PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::Widgets
        Qt6::Charts
        snap7::snap7
        fastdds
    )

    if(WIN32)
        target_link_libraries(AFP_ABB_CPP PRIVATE wininet)
    endif()

    add_custom_command(TARGET AFP_ABB_CPP POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/config
                $<TARGET_FILE_DIR:AFP_ABB_CPP>/config
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/assets
                $<TARGET_FILE_DIR:AFP_ABB_CPP>/assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/lib
                $<TARGET_FILE_DIR:AFP_ABB_CPP>/lib
        COMMAND ${CMAKE_COMMAND} -E echo
                "Thermal camera runtime copied from: ${THERMAL_CAMERA_RUNTIME_SOURCE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                $<TARGET_FILE_DIR:AFP_ABB_CPP>/path
    )

    # windeployqt 自动把所有 Qt 运行时 DLL + plugins + 间接依赖（含 Charts 链
    # 上的 Qt6OpenGL/Qt6OpenGLWidgets 等）全部扫到，不再需要手动逐模块 copy。
    find_program(WINDEPLOYQT_EXE windeployqt HINTS
                 $ENV{QTDIR}/bin
                 "D:/Qt/6.8.3/mingw_64/bin"
                 REQUIRED)
    add_custom_command(TARGET AFP_ABB_CPP POST_BUILD
        COMMAND ${WINDEPLOYQT_EXE}
                --no-translations --no-opengl-sw
                $<TARGET_FILE:AFP_ABB_CPP>
        COMMENT "Deploying Qt runtime DLLs via windeployqt..."
    )
else()
    string(REPLACE ";" "\n  " MISSING_APP_SOURCES_TEXT "${MISSING_APP_SOURCES}")
    set(MISSING_APP_SOURCES_TEXT "  ${MISSING_APP_SOURCES_TEXT}")
    if(AFP_ABB_REQUIRE_APP_TARGET)
        message(FATAL_ERROR "AFP_ABB_CPP app target cannot be created. Missing files:\n${MISSING_APP_SOURCES_TEXT}")
    endif()
    message(STATUS "AFP_ABB_CPP app target deferred. Missing files:\n${MISSING_APP_SOURCES_TEXT}")
endif()

set(ARCHIVE_COMMON_SOURCES
    src/app/AppPaths.h
    src/app/AppPaths.cpp
    src/app/AppConfig.h
    src/app/AppConfig.cpp
    src/app/StatusLogger.h
    src/app/StatusLogger.cpp
    src/archive/InfluxAdmin.h
    src/archive/InfluxAdmin.cpp
    src/archive/InfluxLineProtocolWriter.h
    src/archive/InfluxLineProtocolWriter.cpp
    src/archive/PressureSampleReader.h
    src/archive/PressureSampleReader.cpp
    src/archive/RobotMonitorSampleReader.h
    src/archive/RobotMonitorSampleReader.cpp
    src/archive/TimeSeriesPoint.h
    src/archive/TimeSeriesPoint.cpp
    src/pressure/generated/PressureSample.hpp
    src/pressure/generated/PressureSamplePubSubTypes.cxx
    src/pressure/generated/PressureSampleTypeObjectSupport.cxx
    src/robot_monitor/generated/RobotMonitorSample.hpp
    src/robot_monitor/generated/RobotMonitorSamplePubSubTypes.cxx
    src/robot_monitor/generated/RobotMonitorSampleTypeObjectSupport.cxx
)

add_executable(dds_influx_subscriber
    src/archive/main_dds_influx_subscriber.cpp
    src/archive/DdsInfluxSubscriberApp.h
    src/archive/DdsInfluxSubscriberApp.cpp
    ${ARCHIVE_COMMON_SOURCES}
)
target_include_directories(dds_influx_subscriber PRIVATE
    src
    src/pressure/generated
    src/robot_monitor/generated
)
target_link_libraries(dds_influx_subscriber PRIVATE
    Qt6::Core
    Qt6::Network
    fastdds
)
set(DDS_INFLUX_SUBSCRIBER_RUNTIME_FILES
    $<TARGET_FILE:fastdds>
)
foreach(DDS_RUNTIME_TARGET fastcdr foonathan_memory)
    if(TARGET ${DDS_RUNTIME_TARGET})
        list(APPEND DDS_INFLUX_SUBSCRIBER_RUNTIME_FILES
            $<TARGET_FILE:${DDS_RUNTIME_TARGET}>
        )
    endif()
endforeach()
add_custom_command(TARGET dds_influx_subscriber POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/config
            $<TARGET_FILE_DIR:dds_influx_subscriber>/config
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Core>
            $<TARGET_FILE:Qt6::Network>
            ${DDS_INFLUX_SUBSCRIBER_RUNTIME_FILES}
            $<TARGET_FILE_DIR:dds_influx_subscriber>
)

#if(TARGET AFP_ABB_CPP)
#    add_dependencies(AFP_ABB_CPP dds_influx_subscriber)
#    add_custom_command(TARGET AFP_ABB_CPP POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy_if_different
#                $<TARGET_FILE:dds_influx_subscriber>
#                $<TARGET_FILE_DIR:AFP_ABB_CPP>
#    )
#endif

enable_testing()

set(TEST_APP_PATHS_SOURCES
    tests/test_app_paths.cpp
    src/app/AppPaths.cpp
)
set(TEST_APP_PATHS_READY TRUE)
set(MISSING_TEST_APP_PATHS_SOURCES)
foreach(TEST_SOURCE ${TEST_APP_PATHS_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        set(TEST_APP_PATHS_READY FALSE)
        list(APPEND MISSING_TEST_APP_PATHS_SOURCES ${TEST_SOURCE})
    endif()
endforeach()

if(TEST_APP_PATHS_READY)
    add_executable(test_app_paths
        ${TEST_APP_PATHS_SOURCES}
    )
    target_include_directories(test_app_paths PRIVATE src)
    target_link_libraries(test_app_paths PRIVATE Qt6::Core Qt6::Test)
    copy_qt_test_runtime(test_app_paths)
    add_test(NAME test_app_paths COMMAND test_app_paths)
elseif(MISSING_TEST_APP_PATHS_SOURCES)
    string(REPLACE ";" "\n  " MISSING_TEST_APP_PATHS_TEXT "${MISSING_TEST_APP_PATHS_SOURCES}")
    message(STATUS "test_app_paths target deferred. Missing files:\n  ${MISSING_TEST_APP_PATHS_TEXT}")
endif()

set(TEST_APP_CONFIG_SOURCES
    tests/test_app_config.cpp
    src/app/AppConfig.cpp
    src/app/AppPaths.cpp
)
set(TEST_APP_CONFIG_READY TRUE)
set(MISSING_TEST_APP_CONFIG_SOURCES)
foreach(TEST_SOURCE ${TEST_APP_CONFIG_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        set(TEST_APP_CONFIG_READY FALSE)
        list(APPEND MISSING_TEST_APP_CONFIG_SOURCES ${TEST_SOURCE})
    endif()
endforeach()

if(TEST_APP_CONFIG_READY)
    add_executable(test_app_config
        ${TEST_APP_CONFIG_SOURCES}
    )
    target_include_directories(test_app_config PRIVATE src)
    target_link_libraries(test_app_config PRIVATE Qt6::Core Qt6::Test)
    copy_qt_test_runtime(test_app_config)
    add_test(NAME test_app_config COMMAND test_app_config)
elseif(MISSING_TEST_APP_CONFIG_SOURCES)
    string(REPLACE ";" "\n  " MISSING_TEST_APP_CONFIG_TEXT "${MISSING_TEST_APP_CONFIG_SOURCES}")
    message(STATUS "test_app_config target deferred. Missing files:\n  ${MISSING_TEST_APP_CONFIG_TEXT}")
endif()

add_executable(test_time_series_point
    tests/test_time_series_point.cpp
    src/archive/PressureSampleReader.cpp
    src/archive/RobotMonitorSampleReader.cpp
    src/archive/TimeSeriesPoint.cpp
    src/pressure/generated/PressureSamplePubSubTypes.cxx
    src/pressure/generated/PressureSampleTypeObjectSupport.cxx
    src/robot_monitor/generated/RobotMonitorSamplePubSubTypes.cxx
    src/robot_monitor/generated/RobotMonitorSampleTypeObjectSupport.cxx
)
target_include_directories(test_time_series_point PRIVATE
    src
    src/pressure/generated
    src/robot_monitor/generated
)
target_link_libraries(test_time_series_point PRIVATE Qt6::Core Qt6::Test fastdds)
copy_qt_test_runtime(test_time_series_point)
add_test(NAME test_time_series_point COMMAND test_time_series_point)

add_executable(test_influx_token
    tests/test_influx_token.cpp
    src/archive/InfluxAdmin.cpp
    src/app/AppConfig.cpp
    src/app/AppPaths.cpp
)
target_include_directories(test_influx_token PRIVATE src)
target_link_libraries(test_influx_token PRIVATE Qt6::Core Qt6::Network Qt6::Test)
copy_qt_test_runtime(test_influx_token)
add_custom_command(TARGET test_influx_token POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Network>
            $<TARGET_FILE_DIR:test_influx_token>
)
add_test(NAME test_influx_token COMMAND test_influx_token)

add_executable(test_influx_line_protocol_writer
    tests/test_influx_line_protocol_writer.cpp
    src/archive/InfluxLineProtocolWriter.cpp
    src/archive/TimeSeriesPoint.cpp
)
target_include_directories(test_influx_line_protocol_writer PRIVATE src)
target_link_libraries(test_influx_line_protocol_writer PRIVATE Qt6::Core Qt6::Network Qt6::Test)
copy_qt_test_runtime(test_influx_line_protocol_writer)
add_custom_command(TARGET test_influx_line_protocol_writer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Network>
            $<TARGET_FILE_DIR:test_influx_line_protocol_writer>
)
add_test(NAME test_influx_line_protocol_writer COMMAND test_influx_line_protocol_writer)

add_executable(test_helper_long_running_process
    tests/helper_long_running_process.cpp
)

add_executable(test_helper_exit_process
    tests/helper_exit_process.cpp
)

add_executable(test_timeseries_archive_service
    tests/test_timeseries_archive_service.cpp
    src/services/archive/TimeseriesArchiveService.h
    src/services/archive/TimeseriesArchiveService.cpp
    src/archive/InfluxAdmin.cpp
    src/app/AppConfig.cpp
    src/app/AppPaths.cpp
)
target_include_directories(test_timeseries_archive_service PRIVATE src)
target_link_libraries(test_timeseries_archive_service PRIVATE Qt6::Core Qt6::Network Qt6::Test)
target_compile_definitions(test_timeseries_archive_service PRIVATE
    TEST_HELPER_LONG_RUNNING_PROCESS="$<TARGET_FILE:test_helper_long_running_process>"
    TEST_HELPER_EXIT_PROCESS="$<TARGET_FILE:test_helper_exit_process>"
)
add_dependencies(test_timeseries_archive_service test_helper_long_running_process test_helper_exit_process)
copy_qt_test_runtime(test_timeseries_archive_service)
add_custom_command(TARGET test_timeseries_archive_service POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Network>
            $<TARGET_FILE_DIR:test_timeseries_archive_service>
)
add_test(NAME test_timeseries_archive_service COMMAND test_timeseries_archive_service)

add_executable(test_grafana_startup_service
    tests/test_grafana_startup_service.cpp
    src/services/grafana/GrafanaStartupService.h
    src/services/grafana/GrafanaStartupService.cpp
    src/app/AppPaths.cpp
)
target_include_directories(test_grafana_startup_service PRIVATE src)
target_link_libraries(test_grafana_startup_service PRIVATE Qt6::Core Qt6::Network Qt6::Test)
copy_qt_test_runtime(test_grafana_startup_service)
add_custom_command(TARGET test_grafana_startup_service POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Network>
            $<TARGET_FILE_DIR:test_grafana_startup_service>
)
add_test(NAME test_grafana_startup_service COMMAND test_grafana_startup_service)

set(TEST_S7_DATA_SOURCES
    tests/test_s7_data.cpp
    src/plc/S7Data.cpp
)
set(TEST_S7_DATA_READY TRUE)
set(MISSING_TEST_S7_DATA_SOURCES)
foreach(TEST_SOURCE ${TEST_S7_DATA_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        set(TEST_S7_DATA_READY FALSE)
        list(APPEND MISSING_TEST_S7_DATA_SOURCES ${TEST_SOURCE})
    endif()
endforeach()

if(TEST_S7_DATA_READY)
    add_executable(test_s7_data
        ${TEST_S7_DATA_SOURCES}
    )
    target_include_directories(test_s7_data PRIVATE src)
    target_link_libraries(test_s7_data PRIVATE Qt6::Core Qt6::Test)
    copy_qt_test_runtime(test_s7_data)
    add_test(NAME test_s7_data COMMAND test_s7_data)
elseif(MISSING_TEST_S7_DATA_SOURCES)
    string(REPLACE ";" "\n  " MISSING_TEST_S7_DATA_TEXT "${MISSING_TEST_S7_DATA_SOURCES}")
    message(STATUS "test_s7_data target deferred. Missing files:\n  ${MISSING_TEST_S7_DATA_TEXT}")
endif()

set(TEST_PRESSURE_STATISTICS_SOURCES
    tests/test_pressure_statistics.cpp
    src/pressure/PressureStatistics.cpp
)
set(TEST_PRESSURE_STATISTICS_READY TRUE)
set(MISSING_TEST_PRESSURE_STATISTICS_SOURCES)
foreach(TEST_SOURCE ${TEST_PRESSURE_STATISTICS_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        set(TEST_PRESSURE_STATISTICS_READY FALSE)
        list(APPEND MISSING_TEST_PRESSURE_STATISTICS_SOURCES ${TEST_SOURCE})
    endif()
endforeach()

if(TEST_PRESSURE_STATISTICS_READY)
    add_executable(test_pressure_statistics ${TEST_PRESSURE_STATISTICS_SOURCES})
    target_include_directories(test_pressure_statistics PRIVATE src)
    target_link_libraries(test_pressure_statistics PRIVATE Qt6::Core Qt6::Test)
    copy_qt_test_runtime(test_pressure_statistics)
    add_test(NAME test_pressure_statistics COMMAND test_pressure_statistics)
elseif(MISSING_TEST_PRESSURE_STATISTICS_SOURCES)
    string(REPLACE ";" "\n  " MISSING_TEST_PRESSURE_STATISTICS_TEXT "${MISSING_TEST_PRESSURE_STATISTICS_SOURCES}")
    message(STATUS "test_pressure_statistics target deferred. Missing files:\n  ${MISSING_TEST_PRESSURE_STATISTICS_TEXT}")
endif()

set(TEST_PRESSURE_DDS_TYPES_SOURCES
    tests/test_pressure_dds_types.cpp
)
set(TEST_PRESSURE_DDS_TYPES_READY TRUE)
set(MISSING_TEST_PRESSURE_DDS_TYPES_SOURCES)
foreach(TEST_SOURCE ${TEST_PRESSURE_DDS_TYPES_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        set(TEST_PRESSURE_DDS_TYPES_READY FALSE)
        list(APPEND MISSING_TEST_PRESSURE_DDS_TYPES_SOURCES ${TEST_SOURCE})
    endif()
endforeach()

if(TEST_PRESSURE_DDS_TYPES_READY)
    add_executable(test_pressure_dds_types ${TEST_PRESSURE_DDS_TYPES_SOURCES})
    target_include_directories(test_pressure_dds_types PRIVATE
        src
        src/pressure/generated
    )
    target_link_libraries(test_pressure_dds_types PRIVATE Qt6::Core Qt6::Test)
    copy_qt_test_runtime(test_pressure_dds_types)
    add_test(NAME test_pressure_dds_types COMMAND test_pressure_dds_types)
elseif(MISSING_TEST_PRESSURE_DDS_TYPES_SOURCES)
    string(REPLACE ";" "\n  " MISSING_TEST_PRESSURE_DDS_TYPES_TEXT "${MISSING_TEST_PRESSURE_DDS_TYPES_SOURCES}")
    message(STATUS "test_pressure_dds_types target deferred. Missing files:\n  ${MISSING_TEST_PRESSURE_DDS_TYPES_TEXT}")
endif()

add_executable(test_robot_monitor_dds_types
    tests/test_robot_monitor_dds_types.cpp
)
target_include_directories(test_robot_monitor_dds_types PRIVATE
    src
    src/robot_monitor/generated
)
target_link_libraries(test_robot_monitor_dds_types PRIVATE Qt6::Core Qt6::Test)
copy_qt_test_runtime(test_robot_monitor_dds_types)
add_test(NAME test_robot_monitor_dds_types COMMAND test_robot_monitor_dds_types)

set(TEST_PRESSURE_SENSOR_CLIENT_SOURCES
    tests/test_pressure_sensor_client.cpp
    src/pressure/PressureSensorClient.cpp
)
set(TEST_PRESSURE_SENSOR_CLIENT_READY TRUE)
set(MISSING_TEST_PRESSURE_SENSOR_CLIENT_SOURCES)
foreach(TEST_SOURCE ${TEST_PRESSURE_SENSOR_CLIENT_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        set(TEST_PRESSURE_SENSOR_CLIENT_READY FALSE)
        list(APPEND MISSING_TEST_PRESSURE_SENSOR_CLIENT_SOURCES ${TEST_SOURCE})
    endif()
endforeach()

if(TEST_PRESSURE_SENSOR_CLIENT_READY)
    add_executable(test_pressure_sensor_client ${TEST_PRESSURE_SENSOR_CLIENT_SOURCES})
    target_include_directories(test_pressure_sensor_client PRIVATE src)
    target_link_libraries(test_pressure_sensor_client PRIVATE Qt6::Core Qt6::Test)
    copy_qt_test_runtime(test_pressure_sensor_client)
    add_test(NAME test_pressure_sensor_client COMMAND test_pressure_sensor_client)
elseif(MISSING_TEST_PRESSURE_SENSOR_CLIENT_SOURCES)
    string(REPLACE ";" "\n  " MISSING_TEST_PRESSURE_SENSOR_CLIENT_TEXT "${MISSING_TEST_PRESSURE_SENSOR_CLIENT_SOURCES}")
    message(STATUS "test_pressure_sensor_client target deferred. Missing files:\n  ${MISSING_TEST_PRESSURE_SENSOR_CLIENT_TEXT}")
endif()

add_executable(pressure_probe
    src/tools/pressure_probe.cpp
    src/pressure/PressureSensorClient.cpp
)
target_include_directories(pressure_probe PRIVATE src)
target_link_libraries(pressure_probe PRIVATE Qt6::Core)
add_custom_command(TARGET pressure_probe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Core>
            $<TARGET_FILE_DIR:pressure_probe>
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/lib
            $<TARGET_FILE_DIR:pressure_probe>/lib
)

add_executable(test_high_speed_camera_client
    tests/test_high_speed_camera_client.cpp
    src/camera/HighSpeedCameraClient.cpp
)
target_include_directories(test_high_speed_camera_client PRIVATE src)
target_link_libraries(test_high_speed_camera_client PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
copy_qt_test_runtime(test_high_speed_camera_client)
add_custom_command(TARGET test_high_speed_camera_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Gui>
            $<TARGET_FILE_DIR:test_high_speed_camera_client>
)
add_test(NAME test_high_speed_camera_client COMMAND test_high_speed_camera_client)

add_executable(test_thermal_frame_store
    tests/test_thermal_frame_store.cpp
    src/thermal_camera/ThermalFrameStore.cpp
)
target_include_directories(test_thermal_frame_store PRIVATE src)
target_link_libraries(test_thermal_frame_store PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
copy_qt_test_runtime(test_thermal_frame_store)
add_custom_command(TARGET test_thermal_frame_store POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Gui>
            $<TARGET_FILE_DIR:test_thermal_frame_store>
)
copy_qt_jpeg_test_runtime(test_thermal_frame_store)
add_test(NAME test_thermal_frame_store COMMAND test_thermal_frame_store)

add_executable(test_thermal_camera_client
    tests/test_thermal_camera_client.cpp
    src/app/AppPaths.cpp
    src/thermal_camera/ThermalCameraClient.cpp
    src/thermal_camera/ThermalFrameStore.cpp
)
target_include_directories(test_thermal_camera_client PRIVATE src)
target_link_libraries(test_thermal_camera_client PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
copy_qt_test_runtime(test_thermal_camera_client)
add_custom_command(TARGET test_thermal_camera_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Gui>
            $<TARGET_FILE_DIR:test_thermal_camera_client>
)
add_test(NAME test_thermal_camera_client COMMAND test_thermal_camera_client)

add_executable(test_thermal_camera_service
    tests/test_thermal_camera_service.cpp
    src/services/thermal_camera/ThermalCameraService.cpp
    src/thermal_camera/ThermalFrameStore.cpp
    src/ui/Interface4600_window.ui
)
target_include_directories(test_thermal_camera_service PRIVATE
    src
    ${CMAKE_CURRENT_BINARY_DIR}/test_thermal_camera_service_autogen/include
)
target_link_libraries(test_thermal_camera_service PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
copy_qt_widget_test_runtime(test_thermal_camera_service)
copy_qt_jpeg_test_runtime(test_thermal_camera_service)
add_test(NAME test_thermal_camera_service COMMAND test_thermal_camera_service)

add_executable(test_high_speed_camera_service
    tests/test_high_speed_camera_service.cpp
    src/services/camera/HighSpeedCameraService.cpp
    src/services/camera/HighSpeedCameraService.h
    src/camera/HighSpeedCameraClient.cpp
    src/ui/Interface4600_window.ui
)
target_include_directories(test_high_speed_camera_service PRIVATE
    src
    ${CMAKE_CURRENT_BINARY_DIR}/test_high_speed_camera_service_autogen/include
)
target_link_libraries(test_high_speed_camera_service PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
copy_qt_widget_test_runtime(test_high_speed_camera_service)
add_test(NAME test_high_speed_camera_service COMMAND test_high_speed_camera_service)

set(TEST_FTP_SERVICE_SOURCES
    tests/test_ftp_service.cpp
    src/services/ftp/FtpService.cpp
    src/ftp/FtpClient.cpp
)
set(TEST_FTP_SERVICE_READY TRUE)
set(MISSING_TEST_FTP_SERVICE_SOURCES)
foreach(TEST_SOURCE ${TEST_FTP_SERVICE_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        set(TEST_FTP_SERVICE_READY FALSE)
        list(APPEND MISSING_TEST_FTP_SERVICE_SOURCES ${TEST_SOURCE})
    endif()
endforeach()

if(TEST_FTP_SERVICE_READY)
    add_executable(test_ftp_service ${TEST_FTP_SERVICE_SOURCES})
    target_include_directories(test_ftp_service PRIVATE src)
    target_link_libraries(test_ftp_service PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
    if(WIN32)
        target_link_libraries(test_ftp_service PRIVATE wininet)
    endif()
    copy_qt_widget_test_runtime(test_ftp_service)
    add_test(NAME test_ftp_service COMMAND test_ftp_service)
elseif(MISSING_TEST_FTP_SERVICE_SOURCES)
    string(REPLACE ";" "\n  " MISSING_TEST_FTP_SERVICE_TEXT "${MISSING_TEST_FTP_SERVICE_SOURCES}")
    message(STATUS "test_ftp_service target deferred. Missing files:\n  ${MISSING_TEST_FTP_SERVICE_TEXT}")
endif()

set(TEST_ROBOT_SOCKET_CLIENT_SOURCES
    tests/test_robot_socket_client.cpp
    src/robot/RobotSocketClient.cpp
)
set(TEST_ROBOT_SOCKET_CLIENT_READY TRUE)
set(MISSING_TEST_ROBOT_SOCKET_CLIENT_SOURCES)
foreach(TEST_SOURCE ${TEST_ROBOT_SOCKET_CLIENT_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        set(TEST_ROBOT_SOCKET_CLIENT_READY FALSE)
        list(APPEND MISSING_TEST_ROBOT_SOCKET_CLIENT_SOURCES ${TEST_SOURCE})
    endif()
endforeach()

if(TEST_ROBOT_SOCKET_CLIENT_READY)
    add_executable(test_robot_socket_client ${TEST_ROBOT_SOCKET_CLIENT_SOURCES})
    target_include_directories(test_robot_socket_client PRIVATE src)
    target_link_libraries(test_robot_socket_client PRIVATE Qt6::Core Qt6::Network Qt6::Test)
    copy_qt_test_runtime(test_robot_socket_client)
    add_custom_command(TARGET test_robot_socket_client POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt6::Network>
                $<TARGET_FILE_DIR:test_robot_socket_client>
    )
    add_test(NAME test_robot_socket_client COMMAND test_robot_socket_client)
elseif(MISSING_TEST_ROBOT_SOCKET_CLIENT_SOURCES)
    string(REPLACE ";" "\n  " MISSING_TEST_ROBOT_SOCKET_CLIENT_TEXT "${MISSING_TEST_ROBOT_SOCKET_CLIENT_SOURCES}")
    message(STATUS "test_robot_socket_client target deferred. Missing files:\n  ${MISSING_TEST_ROBOT_SOCKET_CLIENT_TEXT}")
endif()

set(TEST_PATH_LOADER_SOURCES
    tests/test_path_loader.cpp
    src/robot/PathLoader.cpp
)
add_executable(test_path_loader ${TEST_PATH_LOADER_SOURCES})
target_include_directories(test_path_loader PRIVATE src)
target_link_libraries(test_path_loader PRIVATE Qt6::Core Qt6::Test)
copy_qt_test_runtime(test_path_loader)
add_test(NAME test_path_loader COMMAND test_path_loader)

set(TEST_EGM_CLIENT_SOURCES
    tests/test_egm_client.cpp
    src/robot/EgmClient.cpp
)
add_executable(test_egm_client ${TEST_EGM_CLIENT_SOURCES})
target_include_directories(test_egm_client PRIVATE src)
target_link_libraries(test_egm_client PRIVATE Qt6::Core Qt6::Network Qt6::Test)
copy_qt_test_runtime(test_egm_client)
add_test(NAME test_egm_client COMMAND test_egm_client)

set(TEST_ROBOT_QUATERNION_SOURCES
    tests/test_robot_quaternion.cpp
    src/robot/RobotQuaternion.cpp
)
add_executable(test_robot_quaternion ${TEST_ROBOT_QUATERNION_SOURCES})
target_include_directories(test_robot_quaternion PRIVATE src)
target_link_libraries(test_robot_quaternion PRIVATE Qt6::Core Qt6::Test)
copy_qt_test_runtime(test_robot_quaternion)
add_test(NAME test_robot_quaternion COMMAND test_robot_quaternion)

add_executable(test_robot_monitor_message_parser
    tests/test_robot_monitor_message_parser.cpp
    src/robot_monitor/RobotMonitorSample.h
    src/robot_monitor/RobotMonitorMessageParser.h
    src/robot_monitor/RobotMonitorMessageParser.cpp
)
target_include_directories(test_robot_monitor_message_parser PRIVATE src)
target_link_libraries(test_robot_monitor_message_parser PRIVATE Qt6::Core Qt6::Test)
copy_qt_test_runtime(test_robot_monitor_message_parser)
add_test(NAME test_robot_monitor_message_parser COMMAND test_robot_monitor_message_parser)

add_executable(test_robot_monitor_recorder
    tests/test_robot_monitor_recorder.cpp
    src/robot_monitor/RobotMonitorSample.h
    src/robot_monitor/RobotMonitorRecorder.h
    src/robot_monitor/RobotMonitorRecorder.cpp
)
target_include_directories(test_robot_monitor_recorder PRIVATE src)
target_link_libraries(test_robot_monitor_recorder PRIVATE Qt6::Core Qt6::Test)
copy_qt_test_runtime(test_robot_monitor_recorder)
add_test(NAME test_robot_monitor_recorder COMMAND test_robot_monitor_recorder)

add_executable(test_robot_monitor_client
    tests/test_robot_monitor_client.cpp
    src/robot_monitor/RobotMonitorSample.h
    src/robot_monitor/RobotMonitorMessageParser.h
    src/robot_monitor/RobotMonitorMessageParser.cpp
    src/robot_monitor/RobotMonitorClient.h
    src/robot_monitor/RobotMonitorClient.cpp
)
target_include_directories(test_robot_monitor_client PRIVATE src)
target_link_libraries(test_robot_monitor_client PRIVATE Qt6::Core Qt6::Network Qt6::Test)
copy_qt_test_runtime(test_robot_monitor_client)
add_custom_command(TARGET test_robot_monitor_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Network>
            $<TARGET_FILE_DIR:test_robot_monitor_client>
)
add_test(NAME test_robot_monitor_client COMMAND test_robot_monitor_client)

add_executable(test_robot_monitoring_service
    tests/test_robot_monitoring_service.cpp
    src/app/AppPaths.h
    src/app/AppPaths.cpp
    src/app/AppConfig.h
    src/app/AppConfig.cpp
    src/robot_monitor/RobotMonitorSample.h
    src/robot_monitor/RobotMonitorMessageParser.h
    src/robot_monitor/RobotMonitorMessageParser.cpp
    src/robot_monitor/RobotMonitorRecorder.h
    src/robot_monitor/RobotMonitorRecorder.cpp
    src/robot_monitor/RobotMonitorClient.h
    src/robot_monitor/RobotMonitorClient.cpp
    src/robot_monitor/IRobotMonitorDdsPublisher.h
    src/robot_monitor/RobotMonitorDdsTypes.h
    src/robot_monitor/RobotMonitorDdsPublisher.h
    src/robot_monitor/RobotMonitorDdsPublisher.cpp
    src/robot_monitor/generated/RobotMonitorSamplePubSubTypes.cxx
    src/robot_monitor/generated/RobotMonitorSampleTypeObjectSupport.cxx
    src/services/robot_monitor/RobotMonitoringService.h
    src/services/robot_monitor/RobotMonitoringService.cpp
)
target_include_directories(test_robot_monitoring_service PRIVATE
    src
    src/robot_monitor/generated
)
target_link_libraries(test_robot_monitoring_service PRIVATE Qt6::Core Qt6::Network Qt6::Test fastdds)
copy_qt_test_runtime(test_robot_monitoring_service)
add_custom_command(TARGET test_robot_monitoring_service POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt6::Network>
            $<TARGET_FILE_DIR:test_robot_monitoring_service>
)
add_test(NAME test_robot_monitoring_service COMMAND test_robot_monitoring_service)

set(TEST_ROBOT_PRINT_SERVICE_SOURCES
    tests/test_robot_print_service.cpp
    src/services/robot/RobotPrintService.cpp
    src/robot/RobotSocketClient.cpp
    src/plc/PlcClient.cpp
    src/plc/PlcPrintSignalManager.cpp
    src/plc/S7Data.cpp
)
set(TEST_ROBOT_PRINT_SERVICE_READY TRUE)
set(MISSING_TEST_ROBOT_PRINT_SERVICE_SOURCES)
foreach(TEST_SOURCE ${TEST_ROBOT_PRINT_SERVICE_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        set(TEST_ROBOT_PRINT_SERVICE_READY FALSE)
        list(APPEND MISSING_TEST_ROBOT_PRINT_SERVICE_SOURCES ${TEST_SOURCE})
    endif()
endforeach()

if(TEST_ROBOT_PRINT_SERVICE_READY)
    add_executable(test_robot_print_service ${TEST_ROBOT_PRINT_SERVICE_SOURCES})
    target_include_directories(test_robot_print_service PRIVATE src)
    target_link_libraries(test_robot_print_service PRIVATE Qt6::Core Qt6::Network Qt6::Test snap7::snap7)
    copy_qt_test_runtime(test_robot_print_service)
    add_custom_command(TARGET test_robot_print_service POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt6::Network>
                $<TARGET_FILE:snap7::snap7>
                $<TARGET_FILE_DIR:test_robot_print_service>
    )
    add_test(NAME test_robot_print_service COMMAND test_robot_print_service)
elseif(MISSING_TEST_ROBOT_PRINT_SERVICE_SOURCES)
    string(REPLACE ";" "\n  " MISSING_TEST_ROBOT_PRINT_SERVICE_TEXT "${MISSING_TEST_ROBOT_PRINT_SERVICE_SOURCES}")
    message(STATUS "test_robot_print_service target deferred. Missing files:\n  ${MISSING_TEST_ROBOT_PRINT_SERVICE_TEXT}")
endif()

add_executable(test_robot_movement_service
    tests/test_robot_movement_service.cpp
    src/services/robot/RobotMovementService.cpp
    src/services/robot/RobotMovementService.h
    src/robot/PlcRobotMotionClient.cpp
    src/robot/SocketRobotMotionClient.cpp
    src/robot/RobotQuaternion.cpp
    src/plc/PlcClient.cpp
    src/plc/S7Data.cpp
    src/ui/Interface4600_window.ui
)
target_include_directories(test_robot_movement_service PRIVATE
    src
    ${CMAKE_CURRENT_BINARY_DIR}/test_robot_movement_service_autogen/include
)
target_link_libraries(test_robot_movement_service PRIVATE Qt6::Core Qt6::Gui Qt6::Network Qt6::Widgets Qt6::Test snap7::snap7)
copy_qt_widget_test_runtime(test_robot_movement_service)
add_custom_command(TARGET test_robot_movement_service POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:snap7::snap7>
            $<TARGET_FILE_DIR:test_robot_movement_service>
)
add_test(NAME test_robot_movement_service COMMAND test_robot_movement_service)

set(TEST_PLC_CLIENT_STATE_SOURCES
    tests/test_plc_client_state.cpp
    src/plc/PlcClient.cpp
    src/plc/S7Data.cpp
)
set(TEST_PLC_CLIENT_STATE_READY TRUE)
set(MISSING_TEST_PLC_CLIENT_STATE_SOURCES)
foreach(TEST_SOURCE ${TEST_PLC_CLIENT_STATE_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        set(TEST_PLC_CLIENT_STATE_READY FALSE)
        list(APPEND MISSING_TEST_PLC_CLIENT_STATE_SOURCES ${TEST_SOURCE})
    endif()
endforeach()

if(TEST_PLC_CLIENT_STATE_READY)
    add_executable(test_plc_client_state
        ${TEST_PLC_CLIENT_STATE_SOURCES}
    )
    target_include_directories(test_plc_client_state PRIVATE src)
    target_link_libraries(test_plc_client_state PRIVATE Qt6::Core Qt6::Test snap7::snap7)
    copy_qt_test_runtime(test_plc_client_state)
    add_custom_command(TARGET test_plc_client_state POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:snap7::snap7>
                $<TARGET_FILE_DIR:test_plc_client_state>
    )
    add_test(NAME test_plc_client_state COMMAND test_plc_client_state)
elseif(MISSING_TEST_PLC_CLIENT_STATE_SOURCES)
    string(REPLACE ";" "\n  " MISSING_TEST_PLC_CLIENT_STATE_TEXT "${MISSING_TEST_PLC_CLIENT_STATE_SOURCES}")
    message(STATUS "test_plc_client_state target deferred. Missing files:\n  ${MISSING_TEST_PLC_CLIENT_STATE_TEXT}")
endif()

set(TEST_PLC_PRINT_SIGNAL_MANAGER_SOURCES
    tests/test_plc_print_signal_manager.cpp
    src/plc/PlcPrintSignalManager.cpp
    src/plc/PlcClient.cpp
    src/plc/S7Data.cpp
)
set(TEST_PLC_PRINT_SIGNAL_MANAGER_READY TRUE)
set(MISSING_TEST_PLC_PRINT_SIGNAL_MANAGER_SOURCES)
foreach(TEST_SOURCE ${TEST_PLC_PRINT_SIGNAL_MANAGER_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        set(TEST_PLC_PRINT_SIGNAL_MANAGER_READY FALSE)
        list(APPEND MISSING_TEST_PLC_PRINT_SIGNAL_MANAGER_SOURCES ${TEST_SOURCE})
    endif()
endforeach()

if(TEST_PLC_PRINT_SIGNAL_MANAGER_READY)
    add_executable(test_plc_print_signal_manager
        ${TEST_PLC_PRINT_SIGNAL_MANAGER_SOURCES}
    )
    target_include_directories(test_plc_print_signal_manager PRIVATE src)
    target_link_libraries(test_plc_print_signal_manager PRIVATE Qt6::Core Qt6::Test snap7::snap7)
    copy_qt_test_runtime(test_plc_print_signal_manager)
    add_custom_command(TARGET test_plc_print_signal_manager POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:snap7::snap7>
                $<TARGET_FILE_DIR:test_plc_print_signal_manager>
    )
    add_test(NAME test_plc_print_signal_manager COMMAND test_plc_print_signal_manager)
elseif(MISSING_TEST_PLC_PRINT_SIGNAL_MANAGER_SOURCES)
    string(REPLACE ";" "\n  " MISSING_TEST_PLC_PRINT_SIGNAL_MANAGER_TEXT "${MISSING_TEST_PLC_PRINT_SIGNAL_MANAGER_SOURCES}")
    message(STATUS "test_plc_print_signal_manager target deferred. Missing files:\n  ${MISSING_TEST_PLC_PRINT_SIGNAL_MANAGER_TEXT}")
endif()

set(TEST_PLC_ROBOT_MOTION_CLIENT_SOURCES
    tests/test_plc_robot_motion_client.cpp
    src/robot/PlcRobotMotionClient.cpp
    src/robot/RobotQuaternion.cpp
    src/plc/PlcClient.cpp
    src/plc/S7Data.cpp
)
set(TEST_PLC_ROBOT_MOTION_CLIENT_READY TRUE)
set(MISSING_TEST_PLC_ROBOT_MOTION_CLIENT_SOURCES)
foreach(TEST_SOURCE ${TEST_PLC_ROBOT_MOTION_CLIENT_SOURCES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE}")
        set(TEST_PLC_ROBOT_MOTION_CLIENT_READY FALSE)
        list(APPEND MISSING_TEST_PLC_ROBOT_MOTION_CLIENT_SOURCES ${TEST_SOURCE})
    endif()
endforeach()

if(TEST_PLC_ROBOT_MOTION_CLIENT_READY)
    add_executable(test_plc_robot_motion_client
        ${TEST_PLC_ROBOT_MOTION_CLIENT_SOURCES}
    )
    target_include_directories(test_plc_robot_motion_client PRIVATE src)
    target_link_libraries(test_plc_robot_motion_client PRIVATE Qt6::Core Qt6::Test snap7::snap7)
    copy_qt_test_runtime(test_plc_robot_motion_client)
    add_custom_command(TARGET test_plc_robot_motion_client POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:snap7::snap7>
                $<TARGET_FILE_DIR:test_plc_robot_motion_client>
    )
    add_test(NAME test_plc_robot_motion_client COMMAND test_plc_robot_motion_client)
elseif(MISSING_TEST_PLC_ROBOT_MOTION_CLIENT_SOURCES)
    string(REPLACE ";" "\n  " MISSING_TEST_PLC_ROBOT_MOTION_CLIENT_TEXT "${MISSING_TEST_PLC_ROBOT_MOTION_CLIENT_SOURCES}")
    message(STATUS "test_plc_robot_motion_client target deferred. Missing files:\n  ${MISSING_TEST_PLC_ROBOT_MOTION_CLIENT_TEXT}")
endif()

if(APP_TARGET_READY AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DeployQt.cmake")
    add_custom_target(deploy_portable
        COMMAND ${CMAKE_COMMAND}
                -DQT_DEPLOY_EXECUTABLE=$<TARGET_FILE:AFP_ABB_CPP>
                -DQT_DEPLOY_OUTPUT=$<TARGET_FILE_DIR:AFP_ABB_CPP>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/DeployQt.cmake
        DEPENDS AFP_ABB_CPP
        COMMENT "Deploying a portable Windows application folder"
    )
endif()
