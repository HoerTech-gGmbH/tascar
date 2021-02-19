# - Try to find LibXml++
# Once done, this will define
#
#  LIBXML++_FOUND - system has LibXml++
#  LIBXML++_INCLUDE_DIRS - the LibXml++ include directories
#  LIBXML++_LIBRARIES - link these to use LibXml++

# Use pkg-config to get hints about paths
if (LIBXML++_LIBRARIES AND LIBXML++_INCLUDE_DIRS)
   # in cache already
   set(LIBXML++_FOUND TRUE)
else ()

   IF (LIBXML++_FIND_REQUIRED)
       find_package(Glib REQUIRED)
       find_package(Glibmm REQUIRED)
       find_package(LibXml2 REQUIRED)
   ELSE (LIBXML++_FIND_REQUIRED)
       find_package(Glib)
       find_package(Glibmm)
       find_package(LibXml2)
   ENDIF (LIBXML++_FIND_REQUIRED)

   find_package(PkgConfig QUIET)
   if (PKG_CONFIG_FOUND)
       pkg_check_modules(LIBXML++_PKG_CONFIG libxml++-2.6)
   endif (PKG_CONFIG_FOUND)

   # Include dir
   find_path(LIBXML++_INCLUDE_DIR
           NAMES libxml++/libxml++.h
           HINTS ${LIBXML++_PKG_CONFIG_INCLUDE_DIRS} ${LIBXML++_PKG_CONFIG_INCLUDE_DIR}
           )

   # Config Include dir
   find_path(LIBXML++_CONFIG_INCLUDE_DIR
           NAMES libxml++config.h
           HINTS ${LIBXML++_PKG_CONFIG_INCLUDE_DIRS} ${LIBXML++_PKG_CONFIG_INCLUDE_DIR}
           )

   # Library
   find_library(LIBXML++_LIBRARY
           NAMES xml++ xml++-2.6
           PATHS ${LIBXML++_PKG_CONFIG_LIBRARY_DIRS}
           )

   find_package(PackageHandleStandardArgs)
   find_package_handle_standard_args(LibXml++ DEFAULT_MSG LIBXML++_LIBRARY LIBXML++_INCLUDE_DIR LIBXML++_CONFIG_INCLUDE_DIR)

   if (LIBXML++_FOUND)
       set(LIBXML++_LIBRARIES ${LIBXML++_LIBRARY} ${GLIBMM_LIBRARIES} ${LIBXML2_LIBRARIES} ${GLIB_LIBRARIES})
       set(LIBXML++_INCLUDE_DIRS ${LIBXML++_INCLUDE_DIR} ${LIBXML++_CONFIG_INCLUDE_DIR} ${GLIBMM_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIRS})
   endif (LIBXML++_FOUND)

   # Mark the singular variables as advanced, to hide them of the GUI by default
   mark_as_advanced(LIBXML++_LIBRARY LIBXML++_INCLUDE_DIR LIBXML++_CONFIG_INCLUDE_DIR)
endif ()