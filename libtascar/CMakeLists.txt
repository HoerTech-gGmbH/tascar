cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(tascar VERSION 0.1)

find_package (PkgConfig REQUIRED)
pkg_check_modules(LIBSNDFILE REQUIRED sndfile)
pkg_check_modules(LIBXML++ REQUIRED libxml++-2.6)
pkg_check_modules (JACK REQUIRED jack)
pkg_check_modules (LIBLO REQUIRED liblo)
pkg_check_modules (GTEST REQUIRED gtest)
pkg_check_modules (GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules (EIGEN3 REQUIRED eigen3)
pkg_check_modules (GSL REQUIRED gsl)
pkg_check_modules (FFTW REQUIRED fftw3f)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS "-Wall -std=c++11 -O3")

set(HEADER_FILES
	include/tascar.h
)
set(
	SOURCE_FILES
	src/session.cc
)


file(GLOB tascar_SRC
    "src/*.h"
    "src/*.cc"
)
LIST(REMOVE_ITEM tascar_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gui_elements.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/gui_elements.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dmxdriver.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/dmxdriver.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/termsetbaud.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/termsetbaud.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/serialport.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/serialport.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pdfexport.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/pdfexport.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/alsamidicc.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/src/alsamidicc.h)

add_library(tascar STATIC 
	${tascar_SRC}
)

target_include_directories(tascar 
PUBLIC
	"build"
	${JACK_INCLUDE_DIRS}
	${LIBXML++_INCLUDE_DIRS}
	${LIBLO_INCLUDE_DIRS}
	${LIBSNDFILE_INCLUDE_DIRS}
	${GTEST_INCLUDE_DIRS}
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
	${GTEST_LINK_LIBRARIES}
	${GTKMM_LINK_LIBRARIES}
	${FFTW_LINK_LIBRARIES}
	${GSL_LINK_LIBRARIES}
	${EIGEN3_LINK_LIBRARIES}
)
install(TARGETS tascar DESTINATION lib)
install(FILES ${HEADER_FILES} DESTINATION include/libtascar)