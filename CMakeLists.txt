cmake_minimum_required(VERSION 3.16)

# set the project name and version
project(tascar LANGUAGES CXX)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS "-Wall -std=c++11 -O3")
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

set(PLUGINPREFIX "" CACHE STRING "Prefix for plugins")

# DEPENDENCIES
#find_package(Jack REQUIRED)
#find_package(LibSndFile REQUIRED)
#find_package(LibSamplerate REQUIRED)
#find_package(FFTW REQUIRED)
#find_package(Eigen3 REQUIRED)
#find_package(liblo REQUIRED)
#find_package(GSL REQUIRED)
#find_package(GTK3 COMPONENTS gtkmm)
# TODO: Following find routine does not work on macOS
#find_package(LibXml++ REQUIRED)
# TODO: Remove following workaround
#find_package(PkgConfig REQUIRED)
#pkg_check_modules(LIBXML++ REQUIRED libxml++-2.6)


# LIBTASCAR
add_subdirectory(libtascar)
add_library(tascar::tascar ALIAS tascar)

# PLUGINS
include(plugins.cmake)

if (RECEIVERS)
    foreach (receiver IN LISTS RECEIVERS)
        message("Building ${PLUGINPREFIX}tascarreceiver_${receiver}")
        add_library(${PLUGINPREFIX}receiver_${receiver}
                SHARED
                ${PROJECT_SOURCE_DIR}/plugins/src/receivermod_${receiver}.cc)
        SET_TARGET_PROPERTIES(${PLUGINPREFIX}receiver_${receiver} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}receiver_${receiver}
                PRIVATE
                ${PROJECT_SOURCE_DIR}/plugins/src
                ${PROJECT_SOURCE_DIR}/libtascar/include
                PUBLIC
                ${LIBXML++_INCLUDE_DIRS}
                ${LIBLO_INCLUDE_DIRS}
                ${LIBSNDFILE_INCLUDE_DIRS}
                ${LIBSAMPLERATE_INCLUDE_DIRS}
                ${FFTW_INCLUDE_DIRS}
                ${JACK_INCLUDE_DIRS}
                ${EIGEN3_INCLUDE_DIR}
                ${GSL_INCLUDE_DIR}
                )
        target_link_libraries(${PLUGINPREFIX}receiver_${receiver}
                PUBLIC
                tascar
                ${LIBXML++_LIBRARIES}
                ${LIBLO_LIBRARIES}
                ${LIBSNDFILE_LIBRARIES}
                ${LIBSAMPLERATE_LIBRARIES}
                ${FFTW_LIBRARIES}
                ${JACK_LIBRARIES}
                ${GSL_LIBRARIES}
                )
        install(TARGETS ${PLUGINPREFIX}receiver_${receiver} DESTINATION lib)
    endforeach ()
endif ()

if (TASCARMODS)
    foreach (tascarmod IN LISTS TASCARMODS)
        message("Building ${PLUGINPREFIX}tascarmod_${tascarmod}")
        add_library(${PLUGINPREFIX}tascarmod_${tascarmod}
                SHARED
                ${PROJECT_SOURCE_DIR}/plugins/src/tascarmod_${tascarmod}.cc)
        SET_TARGET_PROPERTIES(${PLUGINPREFIX}tascarmod_${tascarmod} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}tascarmod_${tascarmod}
                PRIVATE
                ${PROJECT_SOURCE_DIR}/plugins/src
                ${PROJECT_SOURCE_DIR}/libtascar/include
                PUBLIC
                ${LIBXML++_INCLUDE_DIRS}
                ${LIBLO_INCLUDE_DIRS}
                ${LIBSNDFILE_INCLUDE_DIRS}
                ${LIBSAMPLERATE_INCLUDE_DIRS}
                ${FFTW_INCLUDE_DIRS}
                ${JACK_INCLUDE_DIRS}
                ${EIGEN3_INCLUDE_DIR}
                ${GSL_INCLUDE_DIR}
                ${GTK3_INCLUDE_DIRS}
                )
        target_link_libraries(${PLUGINPREFIX}tascarmod_${tascarmod}
                PUBLIC
                tascar::tascar
                ${LIBXML++_LIBRARIES}
                ${LIBLO_LIBRARIES}
                ${LIBSNDFILE_LIBRARIES}
                ${LIBSAMPLERATE_LIBRARIES}
                ${FFTW_LIBRARIES}
                ${JACK_LIBRARIES}
                ${GSL_LIBRARIES}
                ${GTK3_LIBRARIES}
                )
        install(TARGETS ${PLUGINPREFIX}tascarmod_${tascarmod} DESTINATION lib)
    endforeach ()
endif ()

if (AUDIOPLUGINS)
    foreach (audioplugin IN LISTS AUDIOPLUGINS)
        message("Building ${PLUGINPREFIX}tascar_ap_${audioplugin}")
        add_library(${PLUGINPREFIX}tascar_ap_${audioplugin}
                SHARED
                ${PROJECT_SOURCE_DIR}/plugins/src/tascar_ap_${audioplugin}.cc)
        SET_TARGET_PROPERTIES(${PLUGINPREFIX}tascar_ap_${audioplugin} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}tascar_ap_${audioplugin}
                PRIVATE
                ${PROJECT_SOURCE_DIR}/plugins/src
                ${PROJECT_SOURCE_DIR}/libtascar/include
                )
        target_link_libraries(${PLUGINPREFIX}tascar_ap_${audioplugin}
                PUBLIC
                tascar::tascar
                ${LIBXML++_LIBRARIES}
                ${LIBLO_LIBRARIES}
                ${LIBSNDFILE_LIBRARIES}
                ${LIBSAMPLERATE_LIBRARIES}
                ${FFTW_LIBRARIES}
                ${JACK_LIBRARIES}
                ${GSL_LIBRARIES}
                ${GTK3_LIBRARIES}
                )
        install(TARGETS ${PLUGINPREFIX}tascar_ap_${audioplugin} DESTINATION lib)
    endforeach ()
endif ()


# CLI
add_executable(tascar_cli
        apps/src/tascar_cli.cc
        )
target_include_directories(tascar_cli
        PRIVATE
        ${PROJECT_SOURCE_DIR}/libtascar/include
        )

target_link_libraries(tascar_cli
        PRIVATE
        tascar::tascar
        )
install(TARGETS tascar_cli DESTINATION bin)
