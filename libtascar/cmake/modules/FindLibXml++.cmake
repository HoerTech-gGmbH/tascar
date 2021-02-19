# - Try to find JACK
# Once done, this will define
#
#  JACK_FOUND - system has JACK
#  JACK_INCLUDE_DIRS - the JACK include directories
#  JACK_LIBRARIES - link these to use JACK

# Use pkg-config to get hints about paths
if (LIBXML++_LIBRARIES AND LIBXML++_INCLUDE_DIRS)
    # in cache already
    set(LIBLO_FOUND TRUE)
else ()
    find_package(PkgConfig QUITE)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(LIBXML++_PKG_CONFIG libxml++-2.6)
    endif (PKG_CONFIG_FOUND)

    # Include dir
    find_path(LIBXML++_INCLUDE_DIR
            NAMES libxml++/libxml++.h
            PATHS ${LIBXML++_PKG_CONFIG_INCLUDE_DIRS}
            )

    # Library
    find_library(LIBXML++_LIBRARY
            NAMES xml++ xml++-2.6
            PATHS ${LIBXML++_PKG_CONFIG_LIBRARY_DIRS}
            )

    find_package(PackageHandleStandardArgs)
    find_package_handle_standard_args(LibXml++ DEFAULT_MSG LIBXML++_LIBRARY LIBXML++_INCLUDE_DIR)

    if (LIBXML++_PKG_CONFIG_FOUND)
        set(LIBXML++_LIBRARIES ${LIBXML++_LIBRARY})
        set(LIBXML++_INCLUDE_DIRS ${LIBXML++_INCLUDE_DIR})
    endif (LIBXML++_PKG_CONFIG_FOUND)

    # Mark the singular variables as advanced, to hide them of the GUI by default
    mark_as_advanced(LIBXML++_LIBRARY LIBXML++_INCLUDE_DIR)
endif ()