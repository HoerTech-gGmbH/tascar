cmake_minimum_required(VERSION 3.10)

### GLOBAL CONFIGURATION
project(tascar LANGUAGES CXX)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(${CMAKE_CURRENT_SOURCE_DIR}/version.cmake)
set(PROJECT_VERSION ${VERSION})
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake/modules)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
#set(THREADS_PREFER_PTHREAD_FLAG ON)
message("Building libtascar version ${VERSION}")
message("Building components with version ${FULLVERSION}")
configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/libtascar/tascarver.h.in"
        "${CMAKE_CURRENT_LIST_DIR}/libtascar/include/tascarver.h"
)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
if (APPLE)
    set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
endif (APPLE)



## DEPENDENCIES
find_package(Jack REQUIRED)
find_package(XercesC REQUIRED)
find_package(liblo REQUIRED)
find_package(LibSndFile REQUIRED)
find_package(FFTW REQUIRED)
find_package(LibSamplerate REQUIRED)
set(EXTERNAL_INCLUDES
        ${JACK_INCLUDE_DIRS}
        ${LIBLO_INCLUDE_DIRS}
        ${LIBSNDFILE_INCLUDE_DIRS}
        ${FFTW_INCLUDE_DIRS}
        ${LIBSAMPLERATE_INCLUDE_DIRS})
set(EXTERNAL_LIBS
        ${JACK_LIBRARIES}
        XercesC::XercesC
        ${LIBLO_LIBRARIES}
        ${LIBSNDFILE_LIBRARIES}
        ${FFTW_LIBRARIES}
        ${LIBSAMPLERATE_LIBRARIES})


### ADDITIONAL FLAGS
set(PLUGINPREFIX "" CACHE STRING "Prefix for plugins")
if (NOT PLUGINPREFIX MATCHES "")
    message("Using plugin prefix ${PLUGINPREFIX}")
    add_compile_definitions(PLUGINPREFIX=${PLUGINPREFIX})
endif ()
add_compile_definitions(CMAKE)


### TARGETS
# libov
add_subdirectory(libtascar)
set(TASCAR_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/libtascar/include)

# Plugins
add_subdirectory(plugins)

# Tascar gui
add_executable(tascar_cli
        apps/src/tascar_cli.cc
        )
set_property(TARGET tascar_cli PROPERTY CXX_STANDARD 14)
set_property(TARGET tascar_cli PROPERTY CXX_STANDARD_REQUIRED ON)
target_include_directories(tascar_cli
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/libtascar/include
        ${EXTERNAL_INCLUDES}
        )

target_link_libraries(tascar_cli
        PRIVATE
        tascar
        ${EXTERNAL_LIBS}
        )
install(TARGETS tascar_cli DESTINATION bin)
