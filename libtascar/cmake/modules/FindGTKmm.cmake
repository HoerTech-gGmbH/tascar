################################################################################
#                                                                              #
#                              Louis-Kenzo Cahier                              #
#                                                                              #
################################################################################

# CMake finder for the GTKmm library (www.gtkmm.org)
#
# Calling this finder will appropriately set the following CMake variables:
# - GTKMM_FOUND, to a boolean value indicating if GTKmm headers and libraries were found;
# - GTKMM_INCLUDE_DIRS, to the path of the directory found to contain the header files;
# - GTKMM_LIBRARIES, to the path of the directory found to contain the binary library files;
# - GTKMM_DEFINITIONS, to the compiler flags necessary when using GTKmm.

# Default library names
set(GTKMM_LIBRARY_NAME gtkmm-${GTKmm_FIND_VERSION_MAJOR}.${GTKmm_FIND_VERSION_MINOR})
set(GDKMM_LIBRARY_NAME gdkmm-${GTKmm_FIND_VERSION_MAJOR}.${GTKmm_FIND_VERSION_MINOR})

# Determine package name and dependent versions based on version
if (${GTKmm_FIND_VERSION_MAJOR}.${GTKmm_FIND_VERSION_MINOR} EQUAL 3.0)
    set(   GTK+_VERSION 3)
    set( GLibmm_VERSION 2.4)
    set(Pangomm_VERSION 1.4)
    set(Cairomm_VERSION 1.0)
    set(  ATKmm_VERSION 1.6)
elseif (${GTKmm_FIND_VERSION_MAJOR}.${GTKmm_FIND_VERSION_MINOR} EQUAL 2.4)
    set(   GTK+_VERSION 2.0)
    set( GLibmm_VERSION 2.4)
    set(Pangomm_VERSION 1.4)
    set(Cairomm_VERSION 1.0)
    set(  ATKmm_VERSION 1.6)
endif ()

# Override parameters specific to platforms
if     (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # TODO
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # TODO
endif ()

# Forward search for dependencies
set(FIND_PACKAGE_ARGUMENTS)
if (GTKMM_FIND_QUIETLY)
    set(FIND_PACKAGE_ARGUMENTS FIND_PACKAGE_ARGUMENTS QUIET)
endif ()
if (GTKMM_FIND_REQUIRED)
    set(FIND_PACKAGE_ARGUMENTS FIND_PACKAGE_ARGUMENTS REQUIRED)
endif ()
find_package(GTK+       ${GTK+_VERSION} ${FIND_PACKAGE_ARGUMENTS})
find_package(GLibmm   ${GLibmm_VERSION} ${FIND_PACKAGE_ARGUMENTS})
find_package(Pangomm ${Pangomm_VERSION} ${FIND_PACKAGE_ARGUMENTS})
find_package(Cairomm ${Cairomm_VERSION} ${FIND_PACKAGE_ARGUMENTS})
find_package(ATKmm     ${ATKmm_VERSION} ${FIND_PACKAGE_ARGUMENTS})

# Under Unix systems -including Linux, OSX, and Cygwin- try using pkg-config to find GTKmm
find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
    # Run package config for GTKmm
    pkg_check_modules(PKGCONFIG_GTKMM QUIET ${GTKMM_LIBRARY_NAME})
    set(GTKMM_DEFINITIONS ${PKGCONFIG_GTKMM_CFLAGS_OTHER})

    # Run package config for GDKmm
    pkg_check_modules(PKGCONFIG_GDKMM QUIET ${GDKMM_LIBRARY_NAME})
endif ()

# Extract the needed variables
set(GTKMM_INCLUDE_HINTS ${PKGCONFIG_GTKMM_INCLUDEDIR} ${PKGCONFIG_GTKMM_INCLUDE_DIRS})
set(GTKMM_LIBRARY_HINTS ${PKGCONFIG_GTKMM_LIBDIR}     ${PKGCONFIG_GTKMM_LIBRARY_DIRS})
set(GDKMM_INCLUDE_HINTS ${PKGCONFIG_GTKMM_LIBDIR}     ${PKGCONFIG_GTKMM_LIBRARY_DIRS} \ # Include GTKmm's lib dir for some distributions like MacPorts
        ${PKGCONFIG_GDKMM_INCLUDEDIR} ${PKGCONFIG_GDKMM_INCLUDE_DIRS})

# Search for the path to the root directory of the GTKmm header files
find_path(GTKMM_INCLUDE_DIR
        NAMES gtkmm.h
        HINTS ${GTKMM_INCLUDE_HINTS}
        /usr/include
        /usr/local/include
        /opt/local/include
        PATH_SUFFIXES ${GTKMM_LIBRARY_NAME})

# Search for the path to the config header files directory for GTKmm, which is situated under the library directory
find_path(GTKMM_CONFIG_INCLUDE_DIR
        NAMES gtkmmconfig.h
        HINTS ${GTKMM_LIBRARY_HINTS}
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        PATH_SUFFIXES ${GTKMM_LIBRARY_NAME}/include)

# Search for the path to the GTKmm library files
find_library(GTKMM_LIBRARY
        NAMES ${GTKMM_LIBRARY_NAME}
        HINTS ${GTKMM_LIBRARY_HINTS}
        /usr/lib
        /usr/local/lib
        /opt/local/lib)

# Search for the path to the root directory of the GDKmm header files
find_path(GDKMM_INCLUDE_DIR
        NAMES gdkmm.h
        HINTS /usr/include
        /usr/local/include
        /opt/local/include
        PATH_SUFFIXES ${GDKMM_LIBRARY_NAME})

# Search for the path to the config header files directory for GDKmm, which is situated under the library directory
find_path(GDKMM_CONFIG_INCLUDE_DIR
        NAMES gdkmmconfig.h
        HINTS ${GDKMM_INCLUDE_HINTS}
        /usr/lib
        /usr/local/lib
        /opt/local/lib
        PATH_SUFFIXES ${GDKMM_LIBRARY_NAME}/include)

# Set the plural forms of the variables as is the convention
set(GTKMM_LIBRARIES    ${GTKMM_LIBRARY};${GTK+_LIBRARIES};${GLIBMM_LIBRARIES};${PANGOMM_LIBRARIES};${CAIROMM_LIBRARIES};${ATKMM_LIBRARIES})
set(GTKMM_INCLUDE_DIRS ${GTKMM_INCLUDE_DIR};${GTKMM_CONFIG_INCLUDE_DIR};${GDKMM_INCLUDE_DIR};${GDKMM_CONFIG_INCLUDE_DIR};${GTK+_INCLUDE_DIRS};${GLIBMM_INCLUDE_DIRS};${PANGOMM_INCLUDE_DIRS};${CAIROMM_INCLUDE_DIRS};${ATKMM_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
# Handle the QUIETLY and REQUIRED arguments and set GTKMM_FOUND to TRUE if all listed variables are TRUE
find_package_handle_standard_args(GTKmm
        DEFAULT_MSG
        GTKMM_LIBRARY GTKMM_INCLUDE_DIR)

# Mark the singular variables as advanced, to hide them of the GUI by default
mark_as_advanced(GTKMM_INCLUDE_DIR GTKMM_LIBRARY)

################################################################################