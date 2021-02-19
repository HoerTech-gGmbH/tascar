cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(plugins LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS "-Wall -std=c++11 -O3")
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../cmake/modules)

# REQUIREMENTS
if( TARGET tascar )
        add_library(tascar::tascar ALIAS tascar)
        set(TASCAR_INCLUDE_DIRECTORIES ${PROJECT_SOURCE_DIR}/../libtascar/include)
else()
        find_package(tascar REQUIRED)
endif()

# PLUGINS
include(${PROJECT_SOURCE_DIR}}/../plugins.cmake)

if (RECEIVERS)
    foreach (receiver IN LISTS RECEIVERS)
        message("Building ${PLUGINPREFIX}tascarreceiver_${receiver}")
        add_library(${PLUGINPREFIX}receiver_${receiver}
                SHARED
                ${PROJECT_SOURCE_DIR}/src/receivermod_${receiver}.cc)

        add_dependencies(${PLUGINPREFIX}receiver_${receiver} tascar)
        SET_TARGET_PROPERTIES(${PLUGINPREFIX}receiver_${receiver} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}receiver_${receiver}
                PRIVATE
                ${PROJECT_SOURCE_DIR}/src
                PUBLIC
                ${TASCAR_INCLUDE_DIRECTORIES}
                )
        target_link_libraries(${PLUGINPREFIX}receiver_${receiver}
                PUBLIC
                tascar::tascar
                )
        install(TARGETS ${PLUGINPREFIX}receiver_${receiver} DESTINATION lib)
    endforeach ()
endif ()

if (TASCARMODS)
    foreach (tascarmod IN LISTS TASCARMODS)
        message("Building ${PLUGINPREFIX}tascarmod_${tascarmod}")
        add_library(${PLUGINPREFIX}tascarmod_${tascarmod}
                SHARED
                ${PROJECT_SOURCE_DIR}/src/tascarmod_${tascarmod}.cc)
        SET_TARGET_PROPERTIES(${PLUGINPREFIX}tascarmod_${tascarmod} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}tascarmod_${tascarmod}
                PRIVATE
                ${PROJECT_SOURCE_DIR}/src
                PUBLIC
                ${TASCAR_INCLUDE_DIRECTORIES}
                )
        target_link_libraries(${PLUGINPREFIX}tascarmod_${tascarmod}
                PUBLIC
                tascar::tascar
                )
        install(TARGETS ${PLUGINPREFIX}tascarmod_${tascarmod} DESTINATION lib)
    endforeach ()
endif ()

if (AUDIOPLUGINS)
    foreach (audioplugin IN LISTS AUDIOPLUGINS)
        message("Building ${PLUGINPREFIX}tascar_ap_${audioplugin}")
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