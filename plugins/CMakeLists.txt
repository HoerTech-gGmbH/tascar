cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(plugins LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS "-Wall -O3")
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake/modules)

add_compile_definitions("TASCARVERSION=${FULLVERSION}")

# REQUIREMENTS
if( TARGET tascar )
        add_library(tascar::tascar ALIAS tascar)
        set(TASCAR_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/../libtascar/include)
else()
        find_package(tascar REQUIRED)
endif()

# PLUGINS
include(${PROJECT_SOURCE_DIR}/../version.cmake)
include(${PROJECT_SOURCE_DIR}/../plugins.cmake)

set_property(GLOBAL PROPERTY TASCAR_PLUGIN_LIBRARIES)

if (RECEIVERS)
    foreach (receiver IN LISTS RECEIVERS)
        message("Building ${PLUGINPREFIX}tascarreceiver_${receiver} for version ${FULLVERSION}")
        set_property(GLOBAL APPEND PROPERTY TASCAR_PLUGIN_LIBRARIES ${PLUGINPREFIX}tascarreceiver_${receiver}) 
        add_library(${PLUGINPREFIX}tascarreceiver_${receiver}
                SHARED
                ${PROJECT_SOURCE_DIR}/src/receivermod_${receiver}.cc)

        add_dependencies(${PLUGINPREFIX}tascarreceiver_${receiver} tascar)
        SET_TARGET_PROPERTIES(${PLUGINPREFIX}tascarreceiver_${receiver} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}tascarreceiver_${receiver}
                PRIVATE
                ${PROJECT_SOURCE_DIR}/src
                PUBLIC
                ${TASCAR_INCLUDE_DIRECTORIES}
                )
        target_link_libraries(${PLUGINPREFIX}tascarreceiver_${receiver}
                PUBLIC
                tascar::tascar
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
                ${PROJECT_SOURCE_DIR}/src/tascarmod_${tascarmod}.cc)
        set_target_properties(${PLUGINPREFIX}tascar_${tascarmod} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}tascar_${tascarmod}
                PRIVATE
                ${PROJECT_SOURCE_DIR}/src
                PUBLIC
                ${TASCAR_INCLUDE_DIRECTORIES}
                )
        target_link_libraries(${PLUGINPREFIX}tascar_${tascarmod}
                PUBLIC
                tascar::tascar
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
                ${PROJECT_SOURCE_DIR}/src/tascarsource_${sourcemod}.cc)
        set_target_properties(${PLUGINPREFIX}tascarsource_${sourcemod} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}tascarsource_${sourcemod}
                PRIVATE
                ${PROJECT_SOURCE_DIR}/src
                PUBLIC
                ${TASCAR_INCLUDE_DIRECTORIES}
                )
        target_link_libraries(${PLUGINPREFIX}tascarsource_${sourcemod}
                PUBLIC
                tascar::tascar
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
                ${PROJECT_SOURCE_DIR}/src/tascar_ap_${audioplugin}.cc)
        SET_TARGET_PROPERTIES(${PLUGINPREFIX}tascar_ap_${audioplugin} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}tascar_ap_${audioplugin}
                PRIVATE
                ${PROJECT_SOURCE_DIR}/src
                PUBLIC
                ${TASCAR_INCLUDE_DIRECTORIES}
                )
        target_link_libraries(${PLUGINPREFIX}tascar_ap_${audioplugin}
                PUBLIC
                tascar::tascar
                )
        install(TARGETS ${PLUGINPREFIX}tascar_ap_${audioplugin} DESTINATION lib)
    endforeach ()
endif ()