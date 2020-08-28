cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(tascar_cli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS "-Wall -std=c++11 -O3")

add_subdirectory(libtascar "${CMAKE_CURRENT_BINARY_DIR}/libtascar")

add_subdirectory(plugins "${CMAKE_CURRENT_BINARY_DIR}/plugins")

add_executable(tascar_cli 
	apps/src/tascar_cli.cc
)

target_include_directories(tascar_cli PUBLIC 
	${PROJECT_SOURCE_DIR}/libtascar/src
	${PROJECT_SOURCE_DIR}/plugins/src  )

target_link_libraries(tascar_cli 
	tascar
	tascar_route
	tascar_system
	tascar_touchosc
	tascarreceiver_hrtf
	tascarreceiver_ortf
	tascarreceiver_simplefdn
	tascar_ap_delay
)

install(TARGETS tascar_cli DESTINATION bin)
