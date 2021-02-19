cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(plugins LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS "-Wall -std=c++11 -O3")
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake/modules)

# REQUIREMENTS
#find_package(tascar REQUIRED)
add_library(tascar STATIC IMPORTED GLOBAL ${PROJECT_SOURCE_DIR}/../libtascar)
add_library(tascar::tascar ALIAS tascar)

find_package(Jack REQUIRED)
find_package(LibSndFile REQUIRED)
find_package(LibSamplerate REQUIRED)
find_package(FFTW REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(liblo REQUIRED)
find_package(GSL REQUIRED)
find_package(GTK3 COMPONENTS gtkmm)
# TODO: Following find routine does not work on macOS
#find_package(LibXml++ REQUIRED)
# TODO: Remove following workaround
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBXML++ REQUIRED libxml++-2.6)

if (NOT RECEIVERS)
    list(APPEND RECEIVERS
            amb1h0v
            amb1h1v
            amb3h0v
            amb3h3v
            cardioid
            cardioidmod
            chmap
            debugpos
            fakebf
            foaconv
            hann
            hoa2d
            hoa2d_fuma
            hoa2d_fuma_hos
            hoa3d
            hoa3d_enc
            hrtf
            nsp
            omni
            ortf
            simplefdn
            vmic
            wfs
            )
endif ()

if (NOT TASCARMODS)
    list(APPEND TASCARMODS
            #artnetdmx
            #circularfence
            #datalogging
            dirgain
            dummy
            epicycles
            fail
            #fence
            foa2hoadiff
            geopresets
            #glabsensors
            granularsynth
            hoafdnrot
            hossustain
            hrirconv
            jackrec
            #joystick
            #levels2osc
            #lightctl
            locationmodulator
            locationvelocity
            #lslactor
            #lsljacktime
            #ltcgen
            matrix
            #midicc2osc
            #midictl
            motionpath
            mpu6050track
            nearsensor
            #opendmxusb
            orientationmodulator
            oscevents
            oscjacktime
            #oscrelay
            #oscserver
            pendulum
            #pos2lsl
            pos2osc
            #qualisystracker
            rotator
            route
            sampler
            savegains
            #simplecontroller
            sleep
            system
            #timedisplay
            touchosc
            tracegui
            waitforjackport
            )
endif ()

if (NOT AUDIOPLUGINS)
    list(APPEND AUDIOPLUGINS
            delay
            dummy
            gate
            hannenv
            identity lipsync
            lipsync_paper
            lookatme
            metronome
            onsetdetector
            sine
            sndfile
            spksim
            gainramp
            pulse pink
            dumplevels
            feedbackdelay
            bandpass
            filter
            level2osc
            const
            noise
            loopmachine
            pos2osc
            gain
            sndfileasync
            addchannel
            )
endif ()

if (GTK3_FOUND AND NOT TASCARMODSGUI)
    list(APPEND TASCARMODSGUI
            tracegui
            simplecontroller
            timedisplay
            geopresets
            transportgui
            lightcolorpicker
            )
    list(APPEND TASCARMOD
            ${TASCARMODSGUI}
            )
endif ()

if (RECEIVERS)
    foreach (receiver IN LISTS RECEIVERS)
        message("Building ${PLUGINPREFIX}tascarreceiver_${receiver}")
        add_library(${PLUGINPREFIX}receiver_${receiver}
                SHARED
                src/receivermod_${receiver}.cc)
        SET_TARGET_PROPERTIES(${PLUGINPREFIX}receiver_${receiver} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}receiver_${receiver}
                PRIVATE
                ${PROJECT_SOURCE_DIR}/src
                ${PROJECT_SOURCE_DIR}/../libtascar/include
                PUBLIC
                ${LIBXML++_INCLUDE_DIRS}
                ${LIBLO_INCLUDE_DIRS}
                ${LIBSNDFILE_INCLUDE_DIRS}
                ${LIBSAMPLERATE_INCLUDE_DIRS}
                ${FFTW_INCLUDE_DIRS}
                ${JACK_INCLUDE_DIRS}
                ${EIGEN3_INCLUDE_DIR}
                ${GSL_INCLUDE_DIR}
                )
        target_link_libraries(${PLUGINPREFIX}receiver_${receiver}
                PRIVATE
                tascar::tascar
                PUBLIC
                ${LIBXML++_LIBRARIES}
                ${LIBLO_LIBRARIES}
                ${LIBSNDFILE_LIBRARIES}
                ${LIBSAMPLERATE_LIBRARIES}
                ${FFTW_LIBRARIES}
                ${JACK_LIBRARIES}
                ${GSL_LIBRARIES}
                )
        install(TARGETS ${PLUGINPREFIX}receiver_${receiver} DESTINATION lib)
    endforeach ()
endif ()

if (TASCARMODS)
    foreach (tascarmod IN LISTS TASCARMODS)
        message("Building ${PLUGINPREFIX}tascarmod_${tascarmod}")
        add_library(${PLUGINPREFIX}tascarmod_${tascarmod}
                SHARED
                src/tascarmod_${tascarmod}.cc)
        SET_TARGET_PROPERTIES(${PLUGINPREFIX}tascarmod_${tascarmod} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}tascarmod_${tascarmod}
                PRIVATE
                ${PROJECT_SOURCE_DIR}/src
                ${PROJECT_SOURCE_DIR}/../libtascar/include
                PUBLIC
                ${LIBXML++_INCLUDE_DIRS}
                ${LIBLO_INCLUDE_DIRS}
                ${LIBSNDFILE_INCLUDE_DIRS}
                ${LIBSAMPLERATE_INCLUDE_DIRS}
                ${FFTW_INCLUDE_DIRS}
                ${JACK_INCLUDE_DIRS}
                ${EIGEN3_INCLUDE_DIR}
                ${GSL_INCLUDE_DIR}
                ${GTK3_INCLUDE_DIRS}
                )
        target_link_libraries(${PLUGINPREFIX}tascarmod_${tascarmod}
                PRIVATE
                tascar
                PUBLIC
                ${LIBXML++_LIBRARIES}
                ${LIBLO_LIBRARIES}
                ${LIBSNDFILE_LIBRARIES}
                ${LIBSAMPLERATE_LIBRARIES}
                ${FFTW_LIBRARIES}
                ${JACK_LIBRARIES}
                ${GSL_LIBRARIES}
                ${GTK3_LIBRARIES}
                )
        install(TARGETS ${PLUGINPREFIX}tascarmod_${tascarmod} DESTINATION lib)
    endforeach ()
endif ()

if (AUDIOPLUGINS)
    foreach (audioplugin IN LISTS AUDIOPLUGINS)
        message("Building ${PLUGINPREFIX}tascar_ap_${audioplugin}")
        add_library(${PLUGINPREFIX}tascar_ap_${audioplugin}
                SHARED
                src/tascar_ap_${audioplugin}.cc)
        SET_TARGET_PROPERTIES(${PLUGINPREFIX}tascar_ap_${audioplugin} PROPERTIES PREFIX "")
        target_include_directories(${PLUGINPREFIX}tascar_ap_${audioplugin}
                PRIVATE
                ${PROJECT_SOURCE_DIR}/src
                ${PROJECT_SOURCE_DIR}/../libtascar/include
                PUBLIC
                ${LIBXML++_INCLUDE_DIRS}
                ${LIBLO_INCLUDE_DIRS}
                ${LIBSNDFILE_INCLUDE_DIRS}
                ${LIBSAMPLERATE_INCLUDE_DIRS}
                ${FFTW_INCLUDE_DIRS}
                ${JACK_INCLUDE_DIRS}
                ${EIGEN3_INCLUDE_DIR}
                ${GSL_INCLUDE_DIR}
                )
        target_link_libraries(${PLUGINPREFIX}tascar_ap_${audioplugin}
                PRIVATE
                PUBLIC
                tascar::tascar
                ${LIBXML++_LIBRARIES}
                ${LIBLO_LIBRARIES}
                ${LIBSNDFILE_LIBRARIES}
                ${LIBSAMPLERATE_LIBRARIES}
                ${FFTW_LIBRARIES}
                ${JACK_LIBRARIES}
                ${GSL_LIBRARIES}
                ${GTK3_LIBRARIES}
                )
        install(TARGETS ${PLUGINPREFIX}tascar_ap_${audioplugin} DESTINATION lib)
    endforeach ()
endif ()