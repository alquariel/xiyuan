if(NOT WIN32)
    message(FATAL_ERROR "Portable deployment is currently defined only for Windows.")
endif()

if(NOT DEFINED QT_DEPLOY_EXECUTABLE)
    message(FATAL_ERROR "QT_DEPLOY_EXECUTABLE is required.")
endif()

if(NOT EXISTS "${QT_DEPLOY_EXECUTABLE}")
    message(FATAL_ERROR "Executable does not exist: ${QT_DEPLOY_EXECUTABLE}")
endif()

find_program(WINDEPLOYQT_EXECUTABLE
    NAMES windeployqt windeployqt.exe
    HINTS
        "$ENV{QTDIR}/bin"
        "C:/Qt/6.9.1/msvc2022_64/bin"
)

if(NOT WINDEPLOYQT_EXECUTABLE)
    message(FATAL_ERROR "windeployqt was not found. Add Qt bin to PATH or set QTDIR.")
endif()

execute_process(
    COMMAND "${WINDEPLOYQT_EXECUTABLE}" --release --compiler-runtime "${QT_DEPLOY_EXECUTABLE}"
    RESULT_VARIABLE deployResult
)

if(NOT deployResult EQUAL 0)
    message(FATAL_ERROR "windeployqt failed with exit code ${deployResult}")
endif()

find_file(SNAP7_RUNTIME_DLL
    NAMES snap7.dll
    HINTS
        "C:/vcpkg/installed/x64-windows/bin"
        "$ENV{VCPKG_ROOT}/installed/x64-windows/bin"
)

if(NOT SNAP7_RUNTIME_DLL)
    message(FATAL_ERROR "snap7.dll was not found. Install snap7 with vcpkg or set VCPKG_ROOT.")
endif()

get_filename_component(QT_DEPLOY_OUTPUT_DIR "${QT_DEPLOY_EXECUTABLE}" DIRECTORY)

function(copy_required_runtime DLL_LABEL)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs NAMES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    find_file(${DLL_LABEL}_RUNTIME_DLL
        NAMES ${ARG_NAMES}
        HINTS
            "C:/vcpkg/installed/x64-windows/bin"
            "$ENV{VCPKG_ROOT}/installed/x64-windows/bin"
    )

    if(NOT ${DLL_LABEL}_RUNTIME_DLL)
        string(REPLACE ";" ", " DLL_NAMES_TEXT "${ARG_NAMES}")
        message(FATAL_ERROR "${DLL_LABEL} runtime DLL was not found. Checked: ${DLL_NAMES_TEXT}")
    endif()

    file(COPY "${${DLL_LABEL}_RUNTIME_DLL}" DESTINATION "${QT_DEPLOY_OUTPUT_DIR}")
    message(STATUS "Copied ${DLL_LABEL} runtime: ${${DLL_LABEL}_RUNTIME_DLL}")
endfunction()

# Snap7 is loaded by the PLC wrapper at runtime, so the portable folder must
# ship the DLL beside AFP_ABB_CPP.exe just like the Qt runtime DLLs.
file(COPY "${SNAP7_RUNTIME_DLL}" DESTINATION "${QT_DEPLOY_OUTPUT_DIR}")
message(STATUS "Copied Snap7 runtime: ${SNAP7_RUNTIME_DLL}")

# Fast DDS is linked by the pressure publisher. The exact DLL names include
# ABI versions in the current vcpkg package, so accept both generic and
# versioned names to make the deployment script less brittle.
copy_required_runtime(FASTDDS NAMES fastdds.dll fastdds-3.4.dll)
copy_required_runtime(FASTCDR NAMES fastcdr.dll fastcdr-2.3.dll)
copy_required_runtime(FOONATHAN_MEMORY NAMES foonathan_memory.dll foonathan_memory-0.7.4.dll)
copy_required_runtime(TINYXML2 NAMES tinyxml2.dll)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/../lib" DESTINATION "${QT_DEPLOY_OUTPUT_DIR}")
message(STATUS "Copied pressure sensor runtime folder.")
