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


list(APPEND receivers hrtf ortf simplefdn)

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

list(APPEND plugins tascarmod_route tascarmod_system tascarmod_touchosc tascar_ap_delay)

foreach(plugin IN LISTS plugins)
	message("Building ${plugin}")
	add_library(${plugin}
				SHARED
				src/${plugin}.cc)
	target_include_directories(${plugin} PUBLIC 
		${PROJECT_SOURCE_DIR}/../libtascar/build
		${PROJECT_SOURCE_DIR}/../libtascar/src
		${PROJECT_SOURCE_DIR}/plugins/src 

		${JACK_INCLUDE_DIRS}
		${LIBXML++_INCLUDE_DIRS}
		${LIBLO_INCLUDE_DIRS}
		${LIBSNDFILE_INCLUDE_DIRS}
		${FFTW_INCLUDE_DIRS}
	)

	target_link_libraries(${plugin} 
		tascar
		${JACK_LINK_LIBRARIES}
		${LIBXML++_LINK_LIBRARIES}
		${LIBLO_LINK_LIBRARIES}
		${LIBSNDFILE_LINK_LIBRARIES}
		${FFTW_LINK_LIBRARIES}
	)

	install(TARGETS ${plugin} DESTINATION lib)

endforeach()
