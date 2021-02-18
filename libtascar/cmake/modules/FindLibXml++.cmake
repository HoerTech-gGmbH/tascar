# find libxml++
#
# exports:
#
#   LibXML++_FOUND
#   LibXML++_INCLUDE_DIRS
#   LibXML++_LIBRARIES
#

# Use pkg-config to get hints about paths
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBXML++_PKGCONF REQUIRED libxml++-2.6)
endif(PKG_CONFIG_FOUND)

# Use pkg-config to get hints about paths

# Include dir
find_path(LibXML++_INCLUDE_DIR
        NAMES libxml++/libxml++.h
        PATHS ${LibXML++_PKGCONF_INCLUDE_DIRS}
        )

# Finally the library itself
find_library(LibXML++_LIBRARY
        NAMES xml++ xml++-2.6
        PATHS ${LibXML++_PKGCONF_LIBRARY_DIRS}
        )

find_package(PackageHandleStandardArgs)
find_package_handle_standard_args(LibXml++ DEFAULT_MSG LIBXML++_LIBRARY LIBXML++_INCLUDE_DIR)

if(LIBXML++_PKGCONF_FOUND)
    set(LIBXML++_LIBRARIES ${LIBXML++_LIBRARY} ${LIBXML++_PKGCONF_LIBRARIES})
    set(LIBXML++_INCLUDE_DIRS ${LIBXML++_INCLUDE_DIR} ${LIBXML++_PKGCONF_INCLUDE_DIRS})
    set(LIBXML++_FOUND yes)
else()
    set(LibXML++_LIBRARIES)
    set(LIBXML++_INCLUDE_DIRS)
    set(LIBXML++_FOUND no)
endif()

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
#set(LibXML++_PROCESS_INCLUDES LibXML++_INCLUDE_DIR)
#set(LibXML++_PROCESS_LIBS LibXML++_LIBRARY)
mark_as_advanced(LIBXML++_LIBRARIES LIBXML++_INCLUDE_DIRS)
