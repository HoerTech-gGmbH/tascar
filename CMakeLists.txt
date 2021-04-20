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
if(APPLE)
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/FixAppleArchitecture.cmake)
endif(APPLE)
message("Building libtascar version ${VERSION}")
message("Building components with version ${FULLVERSION}")
configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/libtascar/tascarver.h.in"
        "${CMAKE_CURRENT_LIST_DIR}/libtascar/include/tascarver.h"
)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
if (APPLE)
    set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
endif (APPLE)


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

# Plugins
add_subdirectory(plugins)
get_property(TASCAR_PLUGIN_LIBRARIES GLOBAL PROPERTY TASCAR_PLUGIN_LIBRARIES)

# Tascar gui
add_executable(tascar_cli
        apps/src/tascar_cli.cc
        )
set_property(TARGET tascar_cli PROPERTY CXX_STANDARD 14)
set_property(TARGET tascar_cli PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(tascar_cli
        PRIVATE
        Tascar::Tascar
        ${TASCAR_PLUGIN_LIBRARIES}
        )
install(TARGETS tascar_cli DESTINATION bin)
