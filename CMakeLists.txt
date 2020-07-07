cmake_minimum_required(VERSION 3.10)

# set the project name and version
project(tascar_cli VERSION 0.1)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_FLAGS "-Wall -std=c++11 -O3")

add_subdirectory(libtascar "${CMAKE_CURRENT_BINARY_DIR}/libtascar")

add_executable(tascar_cli 
				apps/src/tascar_cli.cc
			)

target_include_directories(tascar_cli PUBLIC 
	${PROJECT_SOURCE_DIR}/libtascar/build
	${PROJECT_SOURCE_DIR}/libtascar/src  )

target_link_libraries(tascar_cli 
	tascar
)

install(TARGETS tascar_cli DESTINATION bin)