cmake_minimum_required(VERSION 3.16)

# set the project name and version
project(tascar_cli LANGUAGES CXX)

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

add_subdirectory(libtascar)
add_library(tascar::tascar ALIAS tascar)
#add_subdirectory(plugins "${CMAKE_CURRENT_BINARY_DIR}/plugins")

add_executable(tascar_cli
        apps/src/tascar_cli.cc
        )
message("INCLUDE: ${TASCAR_INCLUDE_DIR}")
target_include_directories(tascar_cli
        PRIVATE
        ${PROJECT_SOURCE_DIR}/libtascar/include

        PUBLIC

        #${TASCAR_INCLUDE_DIR}
        #${PROJECT_SOURCE_DIR}/libtascar/src
        #${PROJECT_BINARY_DIR}}/libtascar
        )

target_link_libraries(tascar_cli
        PRIVATE
        tascar::tascar
        #tascar_route
        #tascar_system
        #tascar_touchosc
        #tascarreceiver_hrtf
        #tascarreceiver_ortf
        #tascarreceiver_simplefdn
        #tascar_ap_delay
        )

install(TARGETS tascar_cli DESTINATION bin)
