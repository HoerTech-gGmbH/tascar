
####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was Config.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

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

set_and_check(TASCAR_INCLUDE_DIR "${PACKAGE_PREFIX_DIR}/include/tascar")

include("${CMAKE_CURRENT_LIST_DIR}/tascarTarget.cmake")
check_required_components("tascar")
