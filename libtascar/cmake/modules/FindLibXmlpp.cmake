# Distributed under the OSI-approved BSD 3-Clause License.
# Copyright Stefano Sinigardi

#.rst:
# FindLibXmlpp
# ------------
#
# Find the LibXML++ includes and library.
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ``LibXmlpp_FOUND``
#   True if RealTime library found
#
# ``LibXmlpp_INCLUDE_DIRS``
#   Location of LibXmlpp headers
#
# ``LibXmlpp_LIBRARIES``
#   List of libraries to link with when using LibXmlpp
#

find_path(GLIBMM_INCLUDE_DIR NAMES glibmm.h PATH_SUFFIXES glibmm-2.4)
find_library(GLIBMM_LIBRARY NAMES glibmm glibmm-2.4)
find_library(GLIB_LIBRARY NAMES glib glib-2.0)
find_package(LibXml2)

find_path(LibXmlpp_INCLUDE_DIR NAMES libxmlpp/libxmlpp.h libxml++/libxml++.h PATH_SUFFIXES libxml++-2.6)
find_library(LibXmlpp_LIBRARY NAMES xmlpp xmlpp-2.6 xml++ xml++-2.6)

set(LibXmlpp_INCLUDE_DIRS ${LibXmlpp_INCLUDE_DIR} ${GLIBMM_INCLUDE_DIR} ${LIBXML2_INCLUDE_DIR})
set(LibXmlpp_LIBRARIES ${LibXmlpp_LIBRARY} ${GLIBMM_LIBRARY} ${LIBXML2_LIBRARIES})

message(STATUS "${LibXmlpp_INCLUDE_DIRS}")
message(STATUS "${LibXmlpp_LIBRARIES}")

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LibXmlpp DEFAULT_MSG LibXmlpp_LIBRARY LibXmlpp_INCLUDE_DIR)

mark_as_advanced(LibXmlpp_INCLUDE_DIR LibXmlpp_LIBRARY)