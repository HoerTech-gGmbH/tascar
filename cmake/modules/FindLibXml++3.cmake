# - Try to find libxml++
# Once done, this will define
#
#  LIBXML++3_FOUND - system has libxml++
#  LIBXML++3_INCLUDE_DIRS - the libxml++ include directories
#  LIBXML++3_LIBRARIES - link these to use libxml++

# Use pkg-config to get hints about paths

if (LIBXML++3_LIBRARIES AND LIBXML++3_INCLUDE_DIRS)
    set(LIBXML++3_FOUND TRUE)
else ()
    find_package(Glibmm REQUIRED)
    #find_package(LibXml2 REQUIRED)
    find_package(PkgConfig QUIET)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(LIBXML++3_PKGCONF libxml++-3.0)
    endif (PKG_CONFIG_FOUND)

    # Include dir
    find_path(LIBXML++3_INCLUDE_DIR
            NAMES libxml++/libxml++.h
            PATHS ${LIBXML++3_PKGCONF_INCLUDE_DIRS}
            )

    # Include dir
    find_path(LIBXML++3_CONFIG_INCLUDE_DIR
            NAMES libxml++config.h
            PATHS ${LIBXML++3_PKGCONF_INCLUDE_DIRS}
            )

    # Library
    find_library(LIBXML++3_LIBRARY
            NAMES xml++-3.0
            PATHS ${LIBXML++3_PKGCONF_LIBRARY_DIRS}
            )

    find_package(PackageHandleStandardArgs)
    find_package_handle_standard_args(LibXml++3 DEFAULT_MSG LIBXML++3_LIBRARY LIBXML++3_INCLUDE_DIR LIBXML++3_CONFIG_INCLUDE_DIR)

    if (LIBXML++3_FOUND)
        set(LIBXML++3_LIBRARIES ${LIBXML++3_LIBRARY} ${GLIBMM_LIBRARIES})
        set(LIBXML++3_INCLUDE_DIRS ${LIBXML++3_INCLUDE_DIR} ${LIBXML++3_CONFIG_INCLUDE_DIR} ${GLIBMM_INCLUDE_DIRS})
        if (NOT LibXml++3_FIND_QUIETLY)
            message(STATUS "Found LibXml++3: ${LIBXML++3_LIBRARIES}")
        endif (NOT LibXml++3_FIND_QUIETLY)
    else (LIBXML++3_FOUND)
        if (LibXml++3_FIND_REQUIRED)
            message(FATAL_ERROR "Could not find LibXml++3")
        endif (LibXml++3_FIND_REQUIRED)
    endif (LIBXML++3_FOUND)

    # Mark the singular variables as advanced, to hide them of the GUI by default
    mark_as_advanced(LIBXML++3_LIBRARY LIBXML++3_INCLUDE_DIR)
endif ()


