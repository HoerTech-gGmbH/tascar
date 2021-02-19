cmake_minimum_required(VERSION 3.10)

# GLOBAL
project(tascar LANGUAGES CXX)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
include(../version.cmake)
set(PROJECT_VERSION ${VERSION})
message("Building libtascar version ${VERSION}")
message("Building components with version ${FULLVERSION}")
configure_file(
        "${PROJECT_SOURCE_DIR}/tascarver.h.in"
        "${PROJECT_BINARY_DIR}/tascarver.h"
)
configure_file(
        "${PROJECT_SOURCE_DIR}/tascarver.h.in"
        "${PROJECT_SOURCE_DIR}/include/tascarver.h"
)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    if (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64")
        # Check whether we are running under Rosetta on arm64 hardware.
        execute_process(COMMAND sysctl -q hw.optional.arm64
                OUTPUT_VARIABLE _sysctl_stdout
                ERROR_VARIABLE _sysctl_stderr
                RESULT_VARIABLE _sysctl_result
                )
        if (_sysctl_result EQUAL 0 AND _sysctl_stdout MATCHES "hw.optional.arm64: 1")
            #message("Compiling multi build for x84_64 and arm64")
            set(CMAKE_HOST_SYSTEM_PROCESSOR "arm64")
            set(QMAKE_APPLE_DEVICE_ARCHS arm64)
            set(CMAKE_OSX_ARCHITECTURES arm64)
        else ()
            set(CMAKE_OSX_ARCHITECTURES x86_64)
        endif ()
    endif ()
endif ()

# Source file management
file(GLOB LIB_HEADER ${PROJECT_SOURCE_DIR}/include/*.h)
set(LIB_SOURCES
        ${PROJECT_SOURCE_DIR}/src/licensehandler.cc
        ${PROJECT_SOURCE_DIR}/src/audiostates.cc
        ${PROJECT_SOURCE_DIR}/src/coordinates.cc
        ${PROJECT_SOURCE_DIR}/src/audiochunks.cc
        ${PROJECT_SOURCE_DIR}/src/xmlconfig.cc
        ${PROJECT_SOURCE_DIR}/src/dynamicobjects.cc
        ${PROJECT_SOURCE_DIR}/src/sourcemod.cc
        ${PROJECT_SOURCE_DIR}/src/receivermod.cc
        ${PROJECT_SOURCE_DIR}/src/filterclass.cc
        ${PROJECT_SOURCE_DIR}/src/osc_helper.cc
        ${PROJECT_SOURCE_DIR}/src/pluginprocessor.cc
        ${PROJECT_SOURCE_DIR}/src/acousticmodel.cc
        ${PROJECT_SOURCE_DIR}/src/scene.cc
        ${PROJECT_SOURCE_DIR}/src/render.cc
        ${PROJECT_SOURCE_DIR}/src/session_reader.cc
        ${PROJECT_SOURCE_DIR}/src/session.cc
        ${PROJECT_SOURCE_DIR}/src/jackclient.cc
        ${PROJECT_SOURCE_DIR}/src/delayline.cc
        ${PROJECT_SOURCE_DIR}/src/async_file.cc
        ${PROJECT_SOURCE_DIR}/src/errorhandling.cc
        ${PROJECT_SOURCE_DIR}/src/osc_scene.cc
        ${PROJECT_SOURCE_DIR}/src/ringbuffer.cc
        ${PROJECT_SOURCE_DIR}/src/viewport.cc
        ${PROJECT_SOURCE_DIR}/src/sampler.cc
        ${PROJECT_SOURCE_DIR}/src/jackiowav.cc
        ${PROJECT_SOURCE_DIR}/src/jackclient.cc
        ${PROJECT_SOURCE_DIR}/src/cli.cc
        ${PROJECT_SOURCE_DIR}/src/irrender.cc
        ${PROJECT_SOURCE_DIR}/src/jackrender.cc
        ${PROJECT_SOURCE_DIR}/src/audioplugin.cc
        ${PROJECT_SOURCE_DIR}/src/levelmeter.cc
        ${PROJECT_SOURCE_DIR}/src/serviceclass.cc
        ${PROJECT_SOURCE_DIR}/src/speakerarray.cc
        ${PROJECT_SOURCE_DIR}/src/spectrum.cc
        ${PROJECT_SOURCE_DIR}/src/fft.cc
        ${PROJECT_SOURCE_DIR}/src/stft.cc
        ${PROJECT_SOURCE_DIR}/src/ola.cc
        ${PROJECT_SOURCE_DIR}/src/vbap3d.cc
        ${PROJECT_SOURCE_DIR}/src/hoa.cc
        )
if (Linux)
    list(APPEND LIB_HEADER
            ${PROJECT_SOURCE_DIR}/src/alsamidicc.h
            )
    list(APPEND LIB_SOURCES
            ${PROJECT_SOURCE_DIR}/src/alsamidicc.cc
            )
endif ()
set(DMX_SOURCES
        ${PROJECT_SOURCE_DIR}/src/termsetbaud.cc
        ${PROJECT_SOURCE_DIR}/src/serialport.cc
        ${PROJECT_SOURCE_DIR}/src/dmxdriver.cc
        )
set(UI_SOURCES
        ${PROJECT_SOURCE_DIR}/src/gui_elements.cc
        ${PROJECT_SOURCE_DIR}/src/pdfexport.cc
        )


# REQUIREMENTS
find_package(LibSndFile REQUIRED)
find_package(LibSamplerate REQUIRED)
find_package(LibXml++-2.6 REQUIRED)
find_package(FFTW REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(liblo REQUIRED)
find_package(GSL REQUIRED)


# ADDITIONAL FLAGS
set(PLUGINPREFIX "" CACHE STRING "Prefix for plugins")
if (NOT PLUGINPREFIX MATCHES "")
    message("Using plugin prefix ${PLUGINPREFIX}")
    add_compile_definitions(PLUGINPREFIX=${PLUGINPREFIX})
endif ()
add_compile_definitions(CMAKE)


# TARGET
# libtascar
add_library(tascar SHARED ${LIB_SOURCES})
target_include_directories(tascar
        PRIVATE
        ${PROJECT_BINARY_DIR}
        "${PROJECT_SOURCE_DIR}/src"
        PUBLIC
        ${LIBXML++_INCLUDE_DIRS}
        ${LIBLO_INCLUDE_DIRS}
        ${LIBSNDFILE_INCLUDE_DIRS}
        ${LIBSAMPLERATE_INCLUDE_DIRS}
        ${FFTW_INCLUDE_DIRS}
        ${JACK_INCLUDE_DIRS}
        ${EIGEN3_INCLUDE_DIRS}
        ${GSL_INCLUDE_DIR}
        ${GTKMM_INCLUDE_DIRS}
        )
target_link_libraries(tascar
        PUBLIC
        ${LIBXML++_LIBRARIES}
        ${LIBLO_LIBRARIES}
        ${LIBSNDFILE_LIBRARIES}
        ${LIBSAMPLERATE_LIBRARIES}
        ${FFTW_LIBRARIES}
        ${JACK_LIBRARIES}
        ${GSL_LIBRARIES}
        ${GTKMM_LIBRARIES}
        )

# libtascargui
add_library(tascargui SHARED ${UI_SOURCES})
target_compile_definitions(tascargui PRIVATE -DGTKMM30)
target_include_directories(tascargui
        PRIVATE
        ${PROJECT_BINARY_DIR}
        PUBLIC
        ${LIBXML++_INCLUDE_DIRS}
        ${JACK_INCLUDE_DIRS}
        ${GTKMM_INCLUDE_DIRS}
        )
target_link_libraries(tascargui
        PUBLIC
        ${LIBXML++_LINK_LIBRARIES}
        ${JACK_LINK_LIBRARIES}
        ${GTKMM_LINK_LIBRARIES}
        tascar
        dl
        )

if (Linux)
    # libtascardmx
    add_library(tascardmx SHARED ${DMX_SOURCES} ${DMX_HEADER})
    target_include_directories(tascardmx
            PUBLIC
            )
endif ()


# CMake package config
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
#set_target_properties(tascar PROPERTIES INCLUDES "${LIB_HEADER}")
set_target_properties(tascar PROPERTIES PUBLIC_HEADER "${LIB_HEADER}")
set(TARGETS_EXPORT_NAME tascarTarget)
set(INCLUDE_INSTALL_DIR include/tascar CACHE PATH "Install path for include files")
configure_package_config_file(
        "cmake/Config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/tascarConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tascar"
        PATH_VARS INCLUDE_INSTALL_DIR
)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/tascarConfigVersion.cmake"
        VERSION ${VERSION}
        COMPATIBILITY SameMajorVersion
)
install(
        TARGETS tascar
        EXPORT "${TARGETS_EXPORT_NAME}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tascar"
)
install(
        TARGETS tascargui
        EXPORT "${TARGETS_EXPORT_NAME}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tascar"
)
install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/tascarConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/tascarConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tascar"
)
install(
        EXPORT "${TARGETS_EXPORT_NAME}"
        NAMESPACE "tascar::"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tascar"
)
if (Linux)
    install(
            TARGETS tascardmx
            EXPORT "${TARGETS_EXPORT_NAME}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tascar"
    )
    install(
            FILES "${generated_dir}/tascarDmxConfig.cmake" "${generated_dir}/tascarDmxConfigVersion.cmake"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tascarDmx"
    )
    install(
            EXPORT "${TARGETS_EXPORT_NAME}"
            NAMESPACE "tascar::"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tascar"
    )
endif ()

if (LINUX)
    export(TARGETS tascar tascargui tascardmx NAMESPACE tascar:: FILE tascarTarget.cmake)
else ()
    export(TARGETS tascar tascargui NAMESPACE tascar:: FILE tascarTarget.cmake)
endif ()
