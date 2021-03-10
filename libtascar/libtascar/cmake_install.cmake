# Install script for directory: /Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/libtascar/libtascar.dylib")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libtascar.dylib" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libtascar.dylib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" -x "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libtascar.dylib")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/tascar" TYPE FILE FILES
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/acousticmodel.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/alsamidicc.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/amb33defs.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/async_file.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/audiochunks.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/audioplugin.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/audiostates.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/cli.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/coordinates.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/defs.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/delayline.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/dmxdriver.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/dynamicobjects.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/errorhandling.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/fft.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/filterclass.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/gui_elements.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/hoa.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/irrender.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/jackclient.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/jackiowav.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/jackrender.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/levelmeter.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/licensehandler.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/ola.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/osc_helper.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/osc_scene.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/pdfexport.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/pluginprocessor.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/receivermod.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/render.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/ringbuffer.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/sampler.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/scene.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/serialport.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/serviceclass.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/session.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/session_reader.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/sourcemod.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/speakerarray.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/spectrum.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/stft.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/tascar.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/tascarplugin.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/tascarver.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/termsetbaud.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/vbap3d.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/viewport.h"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/include/tscconfig.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/tascar" TYPE FILE FILES
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/libtascar/tascarConfig.cmake"
    "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/libtascar/tascarConfigVersion.cmake"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/tascar/tascarTarget.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/tascar/tascarTarget.cmake"
         "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/libtascar/CMakeFiles/Export/lib/cmake/tascar/tascarTarget.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/tascar/tascarTarget-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/tascar/tascarTarget.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/tascar" TYPE FILE FILES "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/libtascar/CMakeFiles/Export/lib/cmake/tascar/tascarTarget.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^()$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/tascar" TYPE FILE FILES "/Users/tobias/Developer/digitalstage/ds-client/libov/tascar/libtascar/libtascar/CMakeFiles/Export/lib/cmake/tascar/tascarTarget-noconfig.cmake")
  endif()
endif()

