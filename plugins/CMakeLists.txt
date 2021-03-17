cmake_minimum_required(VERSION 3.10)

### GLOBAL CONFIGURATION
project(plugins LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS "-Wall -O3")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake/modules)


### ADDITIONAL FLAGS
add_compile_definitions("TASCARVERSION=${FULLVERSION}")


### DEPENDENCIES
if (TARGET tascar)
    add_library(tascar::tascar ALIAS tascar)
    set(TASCAR_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/../libtascar/include)
else ()
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../libtascar" "${CMAKE_CURRENT_BINARY_DIR}/libtascar" EXCLUDE_FROM_ALL)
    add_library(tascar::tascar ALIAS tascar)
    set(TASCAR_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_LIST_DIR}/../libtascar/include)
endif ()
find_package(Jack REQUIRED)
find_package(liblo REQUIRED)
find_package(LibSndFile REQUIRED)
find_package(FFTW REQUIRED)
find_package(LibSamplerate REQUIRED)
set(EXTERNAL_INCLUDES
        ${TASCAR_INCLUDE_DIRECTORIES}
        #${JACK_INCLUDE_DIRS}
        #${LIBLO_INCLUDE_DIRS}
        ${LIBSNDFILE_INCLUDE_DIRS}
        #${FFTW_INCLUDE_DIRS}
        #${LIBSAMPLERATE_INCLUDE_DIRS}e
        )
set(EXTERNAL_LIBS
        tascar::tascar
        #${JACK_LIBRARIES}
        #${LIBLO_LIBRARIES}
        ${LIBSNDFILE_LIBRARIES}
        #${FFTW_LIBRARIES}
        #${LIBSAMPLERATE_LIBRARIES}
        )


# PLUGINS
include(${CMAKE_CURRENT_LIST_DIR}/../version.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../plugins.cmake)

set_property(GLOBAL PROPERTY TASCAR_PLUGIN_LIBRARIES)

if (RECEIVERS)
    foreach (receiver IN LISTS RECEIVERS)
        message("Building ${PLUGINPREFIX}tascarreceiver_${receiver} for version ${FULLVERSION}")
        set_property(GLOBAL APPEND PROPERTY TASCAR_PLUGIN_LIBRARIES ${PLUGINPREFIX}tascarreceiver_${receiver})
        add_library(${PLUGINPREFIX}tascarreceiver_${receiver}
                SHARED
                ${CMAKE_CURRENT_SOURCE_DIR}/src/receivermod_${receiver}.cc)

        add_dependencies(${PLUGINPREFIX}tascarreceiver_${receiver} tascar)
        SET_TARGET_PROPERTIES(${PLUGINPREFIX}tascarreceiver_${receiver} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}tascarreceiver_${receiver}
                PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src
                ${EXTERNAL_INCLUDES}
                )
        target_link_libraries(${PLUGINPREFIX}tascarreceiver_${receiver}
                PRIVATE
                ${EXTERNAL_LIBS}
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
                ${CMAKE_CURRENT_SOURCE_DIR}/src/tascarmod_${tascarmod}.cc)
        set_target_properties(${PLUGINPREFIX}tascar_${tascarmod} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}tascar_${tascarmod}
                PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src
                ${EXTERNAL_INCLUDES}
                )
        target_link_libraries(${PLUGINPREFIX}tascar_${tascarmod}
                PRIVATE
                ${EXTERNAL_LIBS}
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
                ${CMAKE_CURRENT_SOURCE_DIR}/src/tascarsource_${sourcemod}.cc)
        set_target_properties(${PLUGINPREFIX}tascarsource_${sourcemod} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}tascarsource_${sourcemod}
                PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src
                ${EXTERNAL_INCLUDES}
                )
        target_link_libraries(${PLUGINPREFIX}tascarsource_${sourcemod}
                PRIVATE
                ${EXTERNAL_LIBS}
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
                ${CMAKE_CURRENT_SOURCE_DIR}/src/tascar_ap_${audioplugin}.cc)
        SET_TARGET_PROPERTIES(${PLUGINPREFIX}tascar_ap_${audioplugin} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}tascar_ap_${audioplugin}
                PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src
                ${EXTERNAL_INCLUDES}
                )
        target_link_libraries(${PLUGINPREFIX}tascar_ap_${audioplugin}
                PRIVATE
                ${EXTERNAL_LIBS}
                )
        install(TARGETS ${PLUGINPREFIX}tascar_ap_${audioplugin} DESTINATION lib)
    endforeach ()
endif ()