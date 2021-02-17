cmake_minimum_required(VERSION 3.10)

# GLOBAL
project(tascar LANGUAGES CXX)
include(../version.cmake)
set(PROJECT_VERSION VERSION)
message("Building libtascar version ${VERSION}")
message("Building components with version ${FULLVERSION}")
configure_file(
        "${PROJECT_SOURCE_DIR}/tascarver.h.in"
        "${PROJECT_BINARY_DIR}/tascarver.h"
)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Source file management
set(LIB_HEADER
        ${PROJECT_SOURCE_DIR}/src/licensehandler.h
        ${PROJECT_SOURCE_DIR}/src/audiostates.h
        ${PROJECT_SOURCE_DIR}/src/coordinates.h
        ${PROJECT_SOURCE_DIR}/src/audiochunks.h
        ${PROJECT_SOURCE_DIR}/src/xmlconfig.h
        ${PROJECT_SOURCE_DIR}/src/dynamicobjects.h
        ${PROJECT_SOURCE_DIR}/src/sourcemod.h
        ${PROJECT_SOURCE_DIR}/src/receivermod.h
        ${PROJECT_SOURCE_DIR}/src/filterclass.h
        ${PROJECT_SOURCE_DIR}/src/osc_helper.h
        ${PROJECT_SOURCE_DIR}/src/pluginprocessor.h
        ${PROJECT_SOURCE_DIR}/src/acousticmodel.h
        ${PROJECT_SOURCE_DIR}/src/scene.h
        ${PROJECT_SOURCE_DIR}/src/render.h
        ${PROJECT_SOURCE_DIR}/src/session_reader.h
        ${PROJECT_SOURCE_DIR}/src/session.h
        ${PROJECT_SOURCE_DIR}/src/jackclient.h
        ${PROJECT_SOURCE_DIR}/src/delayline.h
        ${PROJECT_SOURCE_DIR}/src/async_file.h
        ${PROJECT_SOURCE_DIR}/src/errorhandling.h
        ${PROJECT_SOURCE_DIR}/src/osc_scene.h
        ${PROJECT_SOURCE_DIR}/src/ringbuffer.h
        ${PROJECT_SOURCE_DIR}/src/viewport.h
        ${PROJECT_SOURCE_DIR}/src/sampler.h
        ${PROJECT_SOURCE_DIR}/src/jackiowav.h
        ${PROJECT_SOURCE_DIR}/src/cli.h
        ${PROJECT_SOURCE_DIR}/src/irrender.h
        ${PROJECT_SOURCE_DIR}/src/jackrender.h
        ${PROJECT_SOURCE_DIR}/src/audioplugin.h
        ${PROJECT_SOURCE_DIR}/src/levelmeter.h
        ${PROJECT_SOURCE_DIR}/src/serviceclass.h
        ${PROJECT_SOURCE_DIR}/src/speakerarray.h
        ${PROJECT_SOURCE_DIR}/src/spectrum.h
        ${PROJECT_SOURCE_DIR}/src/fft.h
        ${PROJECT_SOURCE_DIR}/src/stft.h
        ${PROJECT_SOURCE_DIR}/src/ola.h
        ${PROJECT_SOURCE_DIR}/src/vbap3d.h
        ${PROJECT_SOURCE_DIR}/src/hoa.h
        ${PROJECT_BINARY_DIR}/tascarver.h
        )
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
set(DMX_HEADER
        ${PROJECT_SOURCE_DIR}/src/termsetbaud.h
        ${PROJECT_SOURCE_DIR}/src/serialport.h
        ${PROJECT_SOURCE_DIR}/src/dmxdriver.h
        )
set(DMX_SOURCES
        ${PROJECT_SOURCE_DIR}/src/termsetbaud.cc
        ${PROJECT_SOURCE_DIR}/src/serialport.cc
        ${PROJECT_SOURCE_DIR}/src/dmxdriver.cc
        )
set(UI_HEADER
        ${PROJECT_SOURCE_DIR}/src/gui_elements.h
        ${PROJECT_SOURCE_DIR}/src/pdfexport.h
        )
set(UI_SOURCES
        ${PROJECT_SOURCE_DIR}/src/gui_elements.cc
        ${PROJECT_SOURCE_DIR}/src/pdfexport.cc
        )


# REQUIREMENTS
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBSNDFILE REQUIRED sndfile)
pkg_check_modules(LIBSAMPLERATE REQUIRED samplerate)
pkg_check_modules(LIBXML++ REQUIRED libxml++-2.6)
pkg_check_modules(JACK REQUIRED jack)
pkg_check_modules(LIBLO REQUIRED liblo)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules(EIGEN3 REQUIRED eigen3)
pkg_check_modules(GSL REQUIRED gsl)
pkg_check_modules(FFTW REQUIRED fftw3f)


# ADDITIONAL FLAGS
set(PLUGINPREFIX "" CACHE STRING "Prefix for plugins")
if (NOT PLUGINPREFIX MATCHES "")
    message("Using plugin prefix ${PLUGINPREFIX}")
    add_compile_definitions(PLUGINPREFIX=${PLUGINPREFIX})
endif ()


# TARGET
# libtascar
add_library(tascar SHARED ${LIB_SOURCES} ${LIB_HEADER})
set_target_properties(tascar PROPERTIES PUBLIC_HEADER ${LIB_HEADER})
target_include_directories(tascar
        PRIVATE
        ${PROJECT_BINARY_DIR}
        PUBLIC
        ${LIBXML++_INCLUDE_DIRS}
        ${LIBLO_INCLUDE_DIRS}
        ${LIBSNDFILE_INCLUDE_DIRS}
        ${LIBSAMPLERATE_INCLUDE_DIRS}
        ${FFTW_INCLUDE_DIRS}
        ${JACK_INCLUDE_DIRS}
        ${EIGEN3_INCLUDE_DIRS}
        )
target_link_libraries(tascar
        PUBLIC
        ${LIBXML++_LINK_LIBRARIES}
        ${LIBLO_LINK_LIBRARIES}
        ${LIBSNDFILE_LINK_LIBRARIES}
        ${LIBSAMPLERATE_LINK_LIBRARIES}
        ${FFTW_LINK_LIBRARIES}
        ${JACK_LINK_LIBRARIES}
        ${EIGEN3_LINK_LIBRARIES}
        ${GSL_LINK_LIBRARIES}
        )
#[[if(APPLE)
	set_target_properties(tascar PROPERTIES
		FRAMEWORK TRUE
		FRAMEWORK_VERSION C
		MACOSX_FRAMEWORK_IDENTIFIER org.tascar.libTascar
		#MACOSX_FRAMEWORK_INFO_PLIST Info.plist
		# "current version" in semantic format in Mach-O binary file
		VERSION 16.4.0
		# "compatibility version" in semantic format in Mach-O binary file
		SOVERSION 1.0.0
		#PUBLIC_HEADER ${LIB_HEADER}
		#XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
	)
endif()]]

# libtascargui
add_library(tascargui SHARED ${UI_SOURCES} ${UI_HEADER})
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
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Target")
write_basic_package_version_file(
        "${generated_dir}/tascarConfigVersion.cmake" COMPATIBILITY SameMajorVersion
)
#write_basic_package_version_file(
#        "${generated_dir}/tascarGuiConfigVersion.cmake" COMPATIBILITY SameMajorVersion
#)
set(TARGETS_EXPORT_NAME "tascarTarget")
configure_package_config_file(
        "cmake/Config.cmake.in"
        "${generated_dir}/tascarConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tascar"
)
#configure_package_config_file(
#        "cmake/Config.cmake.in"
#        "${generated_dir}/tascarGuiConfig.cmake"
#        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tascar"
#)
install(
        TARGETS tascar
        EXPORT "${TARGETS_EXPORT_NAME}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(
        TARGETS tascargui
        EXPORT "${TARGETS_EXPORT_NAME}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
install(
        FILES "${generated_dir}/tascarConfig.cmake" "${generated_dir}/tascarConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tascar"
)
#install(
#        FILES "${generated_dir}/tascarGuiConfig.cmake" "${generated_dir}/tascarGuiConfigVersion.cmake"
#        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tascar"
#)
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