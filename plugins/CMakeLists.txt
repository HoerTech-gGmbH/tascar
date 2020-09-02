cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(plugins LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS "-Wall -std=c++11 -O3")

find_package (PkgConfig REQUIRED)
pkg_check_modules(LIBSNDFILE REQUIRED sndfile)
pkg_check_modules(LIBXML++ REQUIRED libxml++-2.6)
pkg_check_modules (JACK REQUIRED jack)
pkg_check_modules (LIBLO REQUIRED liblo)
pkg_check_modules (FFTW REQUIRED fftw3f)
#pkg_check_modules (LSL REQUIRED lsl)

list(APPEND receivers 
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

foreach(receiver IN LISTS receivers)
	message("Building tascarreceiver_${receiver}")
	add_library(tascarreceiver_${receiver}
				SHARED
				src/receivermod_${receiver}.cc)
	SET_TARGET_PROPERTIES(tascarreceiver_${receiver} PROPERTIES PREFIX "")
	target_include_directories(tascarreceiver_${receiver} PUBLIC 
		${PROJECT_SOURCE_DIR}/../libtascar/build
		${PROJECT_SOURCE_DIR}/../libtascar/src
		${PROJECT_SOURCE_DIR}/plugins/src 

		${JACK_INCLUDE_DIRS}
		${LIBXML++_INCLUDE_DIRS}
		${LIBLO_INCLUDE_DIRS}
		${LIBSNDFILE_INCLUDE_DIRS}
		${FFTW_INCLUDE_DIRS}
	)

	target_link_libraries(tascarreceiver_${receiver} 
		tascar
		${JACK_LINK_LIBRARIES}
		${LIBXML++_LINK_LIBRARIES}
		${LIBLO_LINK_LIBRARIES}
		${LIBSNDFILE_LINK_LIBRARIES}
		${FFTW_LINK_LIBRARIES}
	)

	install(TARGETS tascarreceiver_${receiver} DESTINATION lib)

endforeach()

list(APPEND plugins 
tascar_ap_addchannel
tascar_ap_bandpass
tascar_ap_const
tascar_ap_delay
tascar_ap_dummy
tascar_ap_dumplevels
tascar_ap_feedbackdelay
tascar_ap_filter
tascar_ap_gain
tascar_ap_gainramp
tascar_ap_gate
tascar_ap_hannenv
tascar_ap_identity
tascar_ap_level2osc
tascar_ap_lipsync
tascar_ap_lipsync_paper
tascar_ap_lookatme
tascar_ap_loopmachine
tascar_ap_metronome
tascar_ap_noise
tascar_ap_onsetdetector
#tascar_ap_openmha
#tascar_ap_periodogram
tascar_ap_pink
tascar_ap_pos2osc
tascar_ap_pulse
tascar_ap_sine
tascar_ap_sndfile
tascar_ap_sndfileasync
#tascar_ap_speechactivity
tascar_ap_spksim
#tascarmod_artnetdmx
#tascarmod_circularfence
#tascarmod_datalogging
tascarmod_dirgain
tascarmod_dummy
tascarmod_epicycles
tascarmod_fail
#tascarmod_fence
tascarmod_foa2hoadiff
tascarmod_geopresets
#tascarmod_glabsensors
tascarmod_granularsynth
tascarmod_hoafdnrot
tascarmod_hossustain
tascarmod_hrirconv
#tascarmod_joystick
#tascarmod_levels2osc
#tascarmod_lightctl
tascarmod_locationmodulator
tascarmod_locationvelocity
#tascarmod_lslactor
#tascarmod_lsljacktime
#tascarmod_ltcgen
tascarmod_matrix
#tascarmod_midicc2osc
#tascarmod_midictl
tascarmod_motionpath
tascarmod_mpu6050track
tascarmod_nearsensor
#tascarmod_opendmxusb
tascarmod_orientationmodulator
tascarmod_oscevents
tascarmod_oscjacktime
#tascarmod_oscrelay
#tascarmod_oscserver
tascarmod_pendulum
#tascarmod_pos2lsl
tascarmod_pos2osc
#tascarmod_qualisystracker
tascarmod_rotator
tascarmod_route
tascarmod_sampler
tascarmod_savegains
#tascarmod_simplecontroller
tascarmod_sleep
tascarmod_system
#tascarmod_timedisplay
tascarmod_touchosc
tascarmod_tracegui
tascarmod_waitforjackport
tascarsource_cardioidmod
tascarsource_door
tascarsource_generic1storder
tascarsource_omni)

foreach(plugin IN LISTS plugins)
	STRING(REPLACE "tascarmod_" "tascar_" plugin_name ${plugin})
	message("Building ${plugin_name}")
	
	add_library(${plugin_name}
				SHARED
				src/${plugin}.cc)
	SET_TARGET_PROPERTIES(${plugin_name} PROPERTIES PREFIX "")
	target_include_directories(${plugin_name} PUBLIC 
		${PROJECT_SOURCE_DIR}/../libtascar/build
		${PROJECT_SOURCE_DIR}/../libtascar/src
		${PROJECT_SOURCE_DIR}/plugins/src 

		${JACK_INCLUDE_DIRS}
		${LIBXML++_INCLUDE_DIRS}
		${LIBLO_INCLUDE_DIRS}
		${LIBSNDFILE_INCLUDE_DIRS}
		${FFTW_INCLUDE_DIRS}
		${LSL_INCLUDE_DIRS}
	)

	target_link_libraries(${plugin_name} 
		tascar
		${JACK_LINK_LIBRARIES}
		${LIBXML++_LINK_LIBRARIES}
		${LIBLO_LINK_LIBRARIES}
		${LIBSNDFILE_LINK_LIBRARIES}
		${FFTW_LINK_LIBRARIES}
		${LSL_LINK_LIBRARIES}
	)

	install(TARGETS ${plugin_name} DESTINATION lib)

endforeach()
