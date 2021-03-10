cmake_minimum_required(VERSION 3.10)

### GLOBAL CONFIGURATION
project(tascar LANGUAGES CXX)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(${PROJECT_SOURCE_DIR}/version.cmake)
set(PROJECT_VERSION ${VERSION})
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake/modules)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
message("Building libtascar version ${VERSION}")
message("Building components with version ${FULLVERSION}")
configure_file(
        "${PROJECT_SOURCE_DIR}/libtascar/tascarver.h.in"
        "${PROJECT_SOURCE_DIR}/libtascar/include/tascarver.h"
)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")


### DEPENDENCIES
find_package(Threads REQUIRED)
find_package(Jack REQUIRED)
find_package(LibSndFile REQUIRED)
find_package(LibSamplerate REQUIRED)
find_package(FFTW REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(liblo REQUIRED)
find_package(GSL REQUIRED)
find_package(LibXml++ REQUIRED)
find_package(GTK3 COMPONENTS gtkmm)


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
set(TASCAR_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/libtascar/include)

# Plugins
include(${PROJECT_SOURCE_DIR}/plugins.cmake)
set_property(GLOBAL PROPERTY TASCAR_PLUGIN_LIBRARIES)

if (RECEIVERS)
    foreach (receiver IN LISTS RECEIVERS)
        message("Building ${PLUGINPREFIX}tascarreceiver_${receiver} for version ${FULLVERSION}")
        set_property(GLOBAL APPEND PROPERTY TASCAR_PLUGIN_LIBRARIES ${PLUGINPREFIX}tascarreceiver_${receiver})
        add_library(${PLUGINPREFIX}tascarreceiver_${receiver}
                SHARED
                ${PROJECT_SOURCE_DIR}/plugins/src/receivermod_${receiver}.cc)

        add_dependencies(${PLUGINPREFIX}tascarreceiver_${receiver} tascar)
        SET_TARGET_PROPERTIES(${PLUGINPREFIX}tascarreceiver_${receiver} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}tascarreceiver_${receiver}
                PRIVATE
               # ${PROJECT_SOURCE_DIR}/plugins/src
                PUBLIC
                ${TASCAR_INCLUDE_DIRECTORIES}
                ${LIBSNDFILE_INCLUDE_DIRS}
                ${LIBSAMPLERATE_INCLUDE_DIRS}
                ${FFTW_INCLUDE_DIRS}
                ${EIGEN3_INCLUDE_DIR}
                ${GSL_INCLUDE_DIR}
                ${JACK_INCLUDE_DIRS}
                ${LIBXML++_INCLUDE_DIRS}
                ${LIBLO_INCLUDE_DIRS}
                )
        target_link_libraries(${PLUGINPREFIX}tascarreceiver_${receiver}
                PUBLIC
                tascar
                )
        install(TARGETS ${PLUGINPREFIX}tascarreceiver_${receiver} DESTINATION lib)
    endforeach ()
endif ()

if (TASCARMODS)
    foreach (tascarmod IN LISTS TASCARMODS)
        message("Building ${PLUGINPREFIX}tascar_${tascarmod} for version ${FULLVERSION}")
        set_property(GLOBAL APPEND PROPERTY TASCAR_PLUGIN_LIBRARIES ${PLUGINPREFIX}tascar_${tascarmod})
        add_library(${PLUGINPREFIX}tascar_${tascarmod}
                SHARED
                ${PROJECT_SOURCE_DIR}/plugins/src/tascarmod_${tascarmod}.cc)
        set_target_properties(${PLUGINPREFIX}tascar_${tascarmod} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}tascar_${tascarmod}
                PRIVATE
                #${PROJECT_SOURCE_DIR}/plugins/src
                PUBLIC
                ${TASCAR_INCLUDE_DIRECTORIES}
                ${LIBSNDFILE_INCLUDE_DIRS}
                ${LIBSAMPLERATE_INCLUDE_DIRS}
                ${FFTW_INCLUDE_DIRS}
                ${EIGEN3_INCLUDE_DIR}
                ${GSL_INCLUDE_DIR}
                ${JACK_INCLUDE_DIRS}
                ${LIBXML++_INCLUDE_DIRS}
                ${LIBLO_INCLUDE_DIRS}
                )
        target_link_libraries(${PLUGINPREFIX}tascar_${tascarmod}
                PUBLIC
                tascar
                )
        install(TARGETS ${PLUGINPREFIX}tascar_${tascarmod} DESTINATION lib)
    endforeach ()
endif ()


if (SOURCEMODS)
    foreach (sourcemod IN LISTS SOURCEMODS)
        message("Building ${PLUGINPREFIX}tascarsource_${sourcemod} for version ${FULLVERSION}")
        set_property(GLOBAL APPEND PROPERTY TASCAR_PLUGIN_LIBRARIES ${PLUGINPREFIX}tascarsource_${sourcemod})
        add_library(${PLUGINPREFIX}tascarsource_${sourcemod}
                SHARED
                ${PROJECT_SOURCE_DIR}/plugins/src/tascarsource_${sourcemod}.cc)
        set_target_properties(${PLUGINPREFIX}tascarsource_${sourcemod} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}tascarsource_${sourcemod}
                PRIVATE
                #${PROJECT_SOURCE_DIR}/plugins/src
                PUBLICx
                ${TASCAR_INCLUDE_DIRECTORIES}
                ${LIBSNDFILE_INCLUDE_DIRS}
                ${LIBSAMPLERATE_INCLUDE_DIRS}
                ${FFTW_INCLUDE_DIRS}
                ${EIGEN3_INCLUDE_DIR}
                ${GSL_INCLUDE_DIR}
                ${JACK_INCLUDE_DIRS}
                ${LIBXML++_INCLUDE_DIRS}
                ${LIBLO_INCLUDE_DIRS}
                )
        target_link_libraries(${PLUGINPREFIX}tascarsource_${sourcemod}
                PUBLIC
                tascar
                #tascar::tascar
                )
        install(TARGETS ${PLUGINPREFIX}tascarsource_${sourcemod} DESTINATION lib)
    endforeach ()
endif ()

if (AUDIOPLUGINS)
    foreach (audioplugin IN LISTS AUDIOPLUGINS)
        message("Building ${PLUGINPREFIX}tascar_ap_${audioplugin} for version ${FULLVERSION}")
        set_property(GLOBAL APPEND PROPERTY TASCAR_PLUGIN_LIBRARIES ${PLUGINPREFIX}tascar_ap_${audioplugin})
        add_library(${PLUGINPREFIX}tascar_ap_${audioplugin}
                SHARED
                ${PROJECT_SOURCE_DIR}/plugins/src/tascar_ap_${audioplugin}.cc)
        SET_TARGET_PROPERTIES(${PLUGINPREFIX}tascar_ap_${audioplugin} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}tascar_ap_${audioplugin}
                PRIVATE
                #${PROJECT_SOURCE_DIR}/plugins/src
                PUBLIC
                ${TASCAR_INCLUDE_DIRECTORIES}
                ${LIBSNDFILE_INCLUDE_DIRS}
                ${LIBSAMPLERATE_INCLUDE_DIRS}
                ${FFTW_INCLUDE_DIRS}
                ${EIGEN3_INCLUDE_DIR}
                ${GSL_INCLUDE_DIR}
                ${JACK_INCLUDE_DIRS}
                ${LIBXML++_INCLUDE_DIRS}
                ${LIBLO_INCLUDE_DIRS}
                )
        target_link_libraries(${PLUGINPREFIX}tascar_ap_${audioplugin}
                PUBLIC
                tascar
                #tascar::tascar
                )
        install(TARGETS ${PLUGINPREFIX}tascar_ap_${audioplugin} DESTINATION lib)
    endforeach ()
endif ()

### EXPORTS (for parent cmake configurations)
if (LINUX)
    if (TARGET tascargui)
        export(TARGETS tascar tascargui tascardmx NAMESPACE tascar:: FILE tascarTarget.cmake)
    elseif ()
        export(TARGETS tascar tascardmx NAMESPACE tascar:: FILE tascarTarget.cmake)
    endif ()
else ()
    if (TARGET tascargui)
        export(TARGETS tascar tascargui NAMESPACE tascar:: FILE tascarTarget.cmake)
    elseif ()
        export(TARGETS tascar NAMESPACE tascar:: FILE tascarTarget.cmake)
    endif ()
endif ()
