cmake_minimum_required(VERSION 3.10)

project(tascar LANGUAGES CXX)

include(version.cmake)

message("Building libtascar version ${VERSION}")
message("Building components with version ${FULLVERSION}")

configure_file (
	"${PROJECT_SOURCE_DIR}/tascarver.h.in"
	"${PROJECT_SOURCE_DIR}/build/tascarver.h"
)

find_package (PkgConfig REQUIRED)
pkg_check_modules(LIBSNDFILE REQUIRED sndfile)
pkg_check_modules(LIBSAMPLERATE REQUIRED samplerate)
pkg_check_modules(LIBXML++ REQUIRED libxml++-2.6)
pkg_check_modules (JACK REQUIRED jack)
pkg_check_modules (LIBLO REQUIRED liblo)
pkg_check_modules (GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules (EIGEN3 REQUIRED eigen3)
pkg_check_modules (GSL REQUIRED gsl)
pkg_check_modules (FFTW REQUIRED fftw3f)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS "-Wall -std=c++11 -O3")

file(GLOB HEADER_FILES
    "${PROJECT_SOURCE_DIR}/src/*.h"
)
LIST(REMOVE_ITEM HEADER_FILES
  ${PROJECT_SOURCE_DIR}/src/gui_elements.h
  ${PROJECT_SOURCE_DIR}/src/dmxdriver.h
  ${PROJECT_SOURCE_DIR}/src/termsetbaud.h
  ${PROJECT_SOURCE_DIR}/src/serialport.h
  ${PROJECT_SOURCE_DIR}/src/pdfexport.h
  ${PROJECT_SOURCE_DIR}/src/alsamidicc.h)

file(GLOB SOURCE_FILES
	${HEADER_FILES}
    "${PROJECT_SOURCE_DIR}/src/*.cc"
)
LIST(REMOVE_ITEM SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/src/gui_elements.cc
  ${PROJECT_SOURCE_DIR}/src/dmxdriver.cc
  ${PROJECT_SOURCE_DIR}/src/termsetbaud.cc
  ${PROJECT_SOURCE_DIR}/src/serialport.cc
  ${PROJECT_SOURCE_DIR}/src/pdfexport.cc
  ${PROJECT_SOURCE_DIR}/src/alsamidicc.cc
  ${PROJECT_SOURCE_DIR}/src/audiochunks_unit_tests.cc
  ${PROJECT_SOURCE_DIR}/src/coordinates_unit_tests.cc
  ${PROJECT_SOURCE_DIR}/src/delayline_unit_tests.cc
  ${PROJECT_SOURCE_DIR}/src/fft_unit_tests.cc
  ${PROJECT_SOURCE_DIR}/src/filterclass_unit_tests.cc
  ${PROJECT_SOURCE_DIR}/src/hoa_unit_tests.cc
  ${PROJECT_SOURCE_DIR}/src/ola_unit_tests.cc)

add_library(tascar STATIC 
	${SOURCE_FILES}
)

target_include_directories(tascar PRIVATE
	"build"
)

target_include_directories(tascar 
PUBLIC
	${JACK_INCLUDE_DIRS}
	${LIBXML++_INCLUDE_DIRS}
	${LIBLO_INCLUDE_DIRS}
	${LIBSNDFILE_INCLUDE_DIRS}
	${LIBSAMPLERATE_INCLUDE_DIRS}
	${GSL_INCLUDE_DIRS}
	${GTKMM_INCLUDE_DIRS}
	${EIGEN3_INCLUDE_DIRS}
	${FFTW_INCLUDE_DIRS}
	${LIBCURL_INCLUDE_DIRS}
)

target_link_libraries(tascar
PUBLIC
	${JACK_LINK_LIBRARIES}
	${LIBXML++_LINK_LIBRARIES}
	${LIBLO_LINK_LIBRARIES}
	${LIBSNDFILE_LINK_LIBRARIES}
	${LIBSAMPLERATE_LINK_LIBRARIES}
	${GTEST_LINK_LIBRARIES}
	${GTKMM_LINK_LIBRARIES}
	${FFTW_LINK_LIBRARIES}
	${GSL_LINK_LIBRARIES}
	${EIGEN3_LINK_LIBRARIES}
)
install(TARGETS tascar DESTINATION lib)
install(FILES ${HEADER_FILES} DESTINATION include/libtascar)
