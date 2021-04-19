cmake_minimum_required(VERSION 3.10)

### GLOBAL CONFIGURATION
project(plugins LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS "-Wall -O3")
#set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake/modules)
if(APPLE)
    include(${CMAKE_CURRENT_LIST_DIR}/../cmake/FixAppleArchitecture.cmake)
endif(APPLE)


### ADDITIONAL FLAGS
add_compile_definitions("TASCARVERSION=${FULLVERSION}")

    
### DEPENDENCIES
if (NOT TARGET Tascar::Tascar)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../libtascar" "${CMAKE_CURRENT_BINARY_DIR}/libtascar" EXCLUDE_FROM_ALL)
endif ()
#find_package(LibSndFile REQUIRED)
set(EXTERNAL_INCLUDES
        #${LIBSNDFILE_INCLUDE_DIRS}
        )
set(EXTERNAL_LIBS
        Tascar::Tascar
        #${LIBSNDFILE_LIBRARIES}
        )

# PLUGINS
include(${CMAKE_CURRENT_LIST_DIR}/../version.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../plugins.cmake)

set_property(GLOBAL PROPERTY TASCAR_PLUGIN_LIBRARIES)

message(STATUS ${PROJECT_VERSION_MAJOR})
message(STATUS ${PROJECT_VERSION_MINOR})
message(STATUS ${PROJECT_VERSION_PATCH})

if (RECEIVERS)
    foreach (receiver IN LISTS RECEIVERS)
        message("Building ${PLUGINPREFIX}tascarreceiver_${receiver} for version ${FULLVERSION}")
        set_property(GLOBAL APPEND PROPERTY TASCAR_PLUGIN_LIBRARIES ${PLUGINPREFIX}tascarreceiver_${receiver})
        add_library(${PLUGINPREFIX}tascarreceiver_${receiver}
                SHARED
                ${CMAKE_CURRENT_SOURCE_DIR}/src/receivermod_${receiver}.cc)
        set_target_properties(${PLUGINPREFIX}tascarreceiver_${receiver} PROPERTIES PREFIX "")
        set_target_properties (${PLUGINPREFIX}tascarreceiver_${receiver}
                PROPERTIES BUILD_WITH_INSTALL_RPATH 1
                        INSTALL_NAME_DIR "@rpath"
        )
        #set_target_properties(${PLUGINPREFIX}tascarreceiver_${receiver} PROPERTIES
                #VERSION "${PROJECT_VERSION_MAJOR}"
                #SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
                #MACHO_CURRENT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
                #MACHO_COMPATIBILITY_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
        #)
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
        #set_target_properties(${PLUGINPREFIX}tascar_${tascarmod} PROPERTIES
	  #VERSION "${PROJECT_VERSION_MAJOR}"
          #SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
          #MACHO_CURRENT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
          #MACHO_COMPATIBILITY_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
	#)
        set_target_properties (${PLUGINPREFIX}tascar_${tascarmod}
                PROPERTIES BUILD_WITH_INSTALL_RPATH 1
                        INSTALL_NAME_DIR "@rpath"
        )
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
        set_target_properties (${PLUGINPREFIX}tascarsource_${sourcemod}
                PROPERTIES BUILD_WITH_INSTALL_RPATH 1
                        INSTALL_NAME_DIR "@rpath"
        )
        #set_target_properties(${PLUGINPREFIX}tascarsource_${sourcemod} PROPERTIES
	  #VERSION "${PROJECT_VERSION_MAJOR}"
          #SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
          #MACHO_CURRENT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
          #MACHO_COMPATIBILITY_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
	#)
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
        set_target_properties(${PLUGINPREFIX}tascar_ap_${audioplugin} PROPERTIES PREFIX "")
        set_target_properties (${PLUGINPREFIX}tascar_ap_${audioplugin}
                PROPERTIES BUILD_WITH_INSTALL_RPATH 1
                        INSTALL_NAME_DIR "@rpath"
        )
        #set_target_properties(${PLUGINPREFIX}tascar_ap_${audioplugin} PROPERTIES
	  #VERSION "${PROJECT_VERSION_MAJOR}"
          #SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
          #MACHO_CURRENT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
          #MACHO_COMPATIBILITY_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
	#)
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