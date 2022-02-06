cmake_minimum_required(VERSION 3.10)

### GLOBAL CONFIGURATION
project(tascar LANGUAGES CXX)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(${CMAKE_CURRENT_LIST_DIR}/../version.cmake)
set(PROJECT_VERSION ${VERSION})
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake/modules)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
#set(THREADS_PREFER_PTHREAD_FLAG ON)
message("Building libtascar version ${VERSION}")
message("Building components with version ${FULLVERSION}")
configure_file(
        "${CMAKE_CURRENT_LIST_DIR}/tascarver.h.in"
        "${CMAKE_CURRENT_LIST_DIR}/include/tascarver.h"
)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_WARN_DEPRECATED FALSE)
if(APPLE)
    include(${CMAKE_CURRENT_LIST_DIR}/../cmake/FixAppleArchitecture.cmake)
endif(APPLE)

### SOURCE FILE MANAGEMENT
file(GLOB LIB_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
set(LIB_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/licensehandler.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/audiostates.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/coordinates.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/audiochunks.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dynamicobjects.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/sourcemod.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/receivermod.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/filterclass.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/osc_helper.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pluginprocessor.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/acousticmodel.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/scene.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/render.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/session_reader.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/session.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/jackclient.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/delayline.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/async_file.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/errorhandling.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/osc_scene.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ringbuffer.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/viewport.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/sampler.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/jackiowav.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/jackclient.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/cli.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/irrender.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/jackrender.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/audioplugin.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/maskplugin.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/levelmeter.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/serviceclass.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/speakerarray.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/spectrum.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/fft.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/stft.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ola.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/vbap3d.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/hoa.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tscconfig.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/tascar_os.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/spawn_process.cc
        )
if (Linux)
    list(APPEND LIB_HEADER
            ${CMAKE_CURRENT_SOURCE_DIR}/include/alsamidicc.h
            )
    list(APPEND LIB_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/alsamidicc.cc
            )
endif ()
set(DMX_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/termsetbaud.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/serialport.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/dmxdriver.cc
        )
set(UI_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gui_elements.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/src/pdfexport.cc
        )


### DEPENDENCIES
find_package(Threads REQUIRED)
find_package(Jack REQUIRED)
find_package(LibSndFile REQUIRED)
find_package(LibSamplerate REQUIRED)
find_package(FFTW REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(liblo REQUIRED)
find_package(GSL REQUIRED)
find_package(XercesC REQUIRED)
find_package(GTK3 COMPONENTS gtkmm)


### ADDITIONAL FLAGS
set(PLUGINPREFIX "" CACHE STRING "Prefix for plugins")
if (NOT PLUGINPREFIX MATCHES "")
    message("Using plugin prefix ${PLUGINPREFIX}")
    add_compile_definitions(PLUGINPREFIX=${PLUGINPREFIX})
endif ()
add_compile_definitions(CMAKE)


### TARGETS
# libtascar
add_library(Tascar STATIC ${LIB_SOURCES})
add_library(Tascar::Tascar ALIAS Tascar)
target_include_directories(Tascar
        PUBLIC
        $<INSTALL_INTERFACE:include> 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        ${LIBSNDFILE_INCLUDE_DIRS}
        ${LIBSAMPLERATE_INCLUDE_DIRS}
        ${FFTW_INCLUDE_DIRS}
        ${EIGEN3_INCLUDE_DIR}
        ${GSL_INCLUDE_DIR}
        ${JACK_INCLUDE_DIRS}
        ${LIBLO_INCLUDE_DIRS}
        )
target_link_libraries(Tascar
        PRIVATE
        Threads::Threads
        ${CMAKE_DL_LIBS}
        PUBLIC
        XercesC::XercesC
        ${LIBLO_LIBRARIES}
        ${LIBSNDFILE_LIBRARIES}
        ${LIBSAMPLERATE_LIBRARIES}
        ${FFTW_LIBRARIES}
        ${GSL_LIBRARIES}
        ${JACK_LIBRARIES}
        )

set(TARGETS_EXPORT_NAME TascarTarget)
set_target_properties(Tascar PROPERTIES PUBLIC_HEADER "${LIB_HEADER}")
configure_package_config_file(
        "${CMAKE_CURRENT_LIST_DIR}/cmake/Config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/tascarConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tascar"
)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/tascarConfigVersion.cmake"
        VERSION ${VERSION}
        COMPATIBILITY SameMajorVersion
)
install(
        TARGETS Tascar
        EXPORT "${TARGETS_EXPORT_NAME}"
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tascar"
)
install(
        FILES "${CMAKE_CURRENT_BINARY_DIR}/tascarConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/tascarConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tascar"
)
install(
        EXPORT "${TARGETS_EXPORT_NAME}"
        NAMESPACE "Tascar::"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Tascar"
)

# libtascargui
if (GTK3_FOUND)
    add_library(Gui SHARED ${UI_SOURCES})
    add_library(Tascar::Gui ALIAS Gui)
    target_compile_definitions(Gui PRIVATE -DGTKMM30)
    target_include_directories(Gui
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            PUBLIC
            ${GTK3_INCLUDE_DIRS}
            )
    target_link_libraries(Gui
            PRIVATE
            Threads::Threads
            PUBLIC
            Tascar::Tascar
            XercesC::XercesC
            ${GTK3_LIBRARIES}
            )
    install(
            TARGETS Gui
            EXPORT "${TARGETS_EXPORT_NAME}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tascar"
    )
endif ()

if (Linux)
    # libtascardmx
    add_library(DMX SHARED ${DMX_SOURCES})
    add_library(Tascar::DMX ALIAS DMX)
    target_include_directories(DMX
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            )
    target_link_libraries(DMX
            PUBLIC
            Tascar::Tascar
            )

    install(
            TARGETS DMX
            EXPORT "${TARGETS_EXPORT_NAME}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
            PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tascar"
    )
    install(
            FILES "${generated_dir}/tascarDmxConfig.cmake" "${generated_dir}/tascarDmxConfigVersion.cmake"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tascarDmx"
    )
    install(
            EXPORT "${TARGETS_EXPORT_NAME}"
            NAMESPACE "Tascar::"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/tascar"
    )
endif ()

if (LINUX)
    if (TARGET Gui)
        export(TARGETS Tascar Gui DMX NAMESPACE Tascar:: FILE TascarTarget.cmake)
    elseif ()
        export(TARGETS Tascar DMX NAMESPACE Tascar:: FILE TascarTarget.cmake)
    endif ()
else ()
    if (TARGET Gui)
        export(TARGETS Tascar Gui NAMESPACE Tascar:: FILE TascarTarget.cmake)
    elseif ()
        export(TARGETS Tascar NAMESPACE Tascar:: FILE TascarTarget.cmake)
    endif ()
endif ()
