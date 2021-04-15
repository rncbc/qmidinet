cmake_minimum_required(VERSION 3.10)

project(qmidinet
  VERSION 0.9.3
  DESCRIPTION "A MIDI Network Gateway via UDP/IP Multicast"
  LANGUAGES C CXX)

set (CONFIG_VERSION ${PROJECT_VERSION})
execute_process (
  COMMAND git describe --tags --dirty --abbrev=6
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_DESCRIBE_OUTPUT
  RESULT_VARIABLE GIT_DESCRIBE_RESULT
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if (GIT_DESCRIBE_RESULT EQUAL 0)
  set (VERSION "${GIT_DESCRIBE_OUTPUT}")
  string (REGEX REPLACE "^[^0-9]+" "" VERSION "${VERSION}")
  string (REGEX REPLACE "^1_"      "" VERSION "${VERSION}")
  string (REGEX REPLACE "^[_vV]+"  "" VERSION "${VERSION}")
  string (REGEX REPLACE "-g"   "git." VERSION "${VERSION}")
  string (REGEX REPLACE "[_|-]"   "." VERSION "${VERSION}")
  execute_process (
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_REVPARSE_OUTPUT
    RESULT_VARIABLE GIT_REVPARSE_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (GIT_REVPARSE_RESULT EQUAL 0 AND NOT GIT_REVPARSE_OUTPUT STREQUAL "master")
    set (VERSION "${VERSION} [${GIT_REVPARSE_OUTPUT}]")
  endif ()
else ()
  set (VERSION "${PROJECT_VERSION}")
endif ()

set (PACKAGE_NAME "QmidiNet")
set (PACKAGE_VERSION "${VERSION}")
set (PACKAGE_BUGREPORT "rncbc@rncbc.org")
set (PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set (PACKAGE_TARNAME "qmidinet")

set (ac_version "${PROJECT_VERSION}")
set (ac_prefix "${CMAKE_INSTALL_PREFIX}")

set (CONFIG_BUILD_VERSION "${PACKAGE_VERSION}")

if (CMAKE_BUILD_TYPE)
  set (CONFIG_BUILD_TYPE ${CMAKE_BUILD_TYPE})
else ()
  set (CONFIG_BUILD_TYPE "release")
endif ()

set (CONFIG_DEBUG 0)
if (CONFIG_BUILD_TYPE MATCHES "debug")
  set (CONFIG_DEBUG 1)
endif ()

set (CONFIG_PREFIX "${CMAKE_INSTALL_PREFIX}")

include (GNUInstallDirs)
set (CONFIG_BINDIR  "${CONFIG_PREFIX}/${CMAKE_INSTALL_BINDIR}")
set (CONFIG_LIBDIR  "${CONFIG_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set (CONFIG_DATADIR "${CONFIG_PREFIX}/${CMAKE_INSTALL_DATADIR}")
set (CONFIG_MANDIR  "${CONFIG_PREFIX}/${CMAKE_INSTALL_MANDIR}")


# Enable ALSA MIDI support option.
option (CONFIG_ALSA_MIDI "Enable ALSA MIDI support (default=yes)" 1)

# Enable JACK MIDI support option.
option (CONFIG_JACK_MIDI "Enable JACK MIDI support (default=yes)" 1)

# Enable Network IPv6 support option.
option (CONFIG_IPV6 "Enable Network IPv6 support (EXPERIMENTAL) (default=yes)" 1)


# Enable Qt6 build preference.
option (CONFIG_QT6 "Enable Qt6 build (default=no)" 0)


# Fix for new CMAKE_REQUIRED_LIBRARIES policy.
if (POLICY CMP0075)
  cmake_policy (SET CMP0075 NEW)
endif ()

# Check for Qt...
if (CONFIG_QT6)
  find_package (Qt6 QUIET)
  if (NOT Qt6_FOUND)
    set (CONFIG_QT6 0)
  endif ()
endif ()

if (CONFIG_QT6)
  find_package (QT QUIET NAMES Qt6)
else ()
  find_package (QT QUIET NAMES Qt5)
endif ()

find_package (Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets)

if (CONFIG_IPV6)
  find_package (Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network)
endif ()

#find_package (Qt${QT_VERSION_MAJOR}LinguistTools)

include (CheckIncludeFile)
include (CheckIncludeFiles)
include (CheckIncludeFileCXX)
include (CheckFunctionExists)
include (CheckLibraryExists)

# Checks for header files.
if (UNIX AND NOT APPLE)
  check_include_files ("fcntl.h;unistd.h;signal.h" HAVE_SIGNAL_H)
endif ()


# Find package modules
find_package (PkgConfig REQUIRED)

# Check for ALSA libraries.
if (CONFIG_ALSA_MIDI)
  pkg_check_modules (ALSA IMPORTED_TARGET alsa)
  if (ALSA_FOUND)
    include_directories (${ALSA_INCLUDE_DIRS})
    link_directories (${ALSA_LIBRARY_DIRS})
    link_libraries (${ALSA_LIBRARIES})
  # set (CMAKE_REQUIRED_LIBRARIES "${ALSA_LIBRARIES};${CMAKE_REQUIRED_LIBRARIES}")
  else ()
    message (WARNING "*** ALSA library not found.")
    set (CONFIG_ALSA_MIDI 0)
  endif ()
endif ()

# Check for JACK libraries.
if (CONFIG_JACK_MIDI)
  pkg_check_modules (JACK IMPORTED_TARGET jack>=0.100.0)
  if (JACK_FOUND)
    include_directories (${JACK_INCLUDE_DIRS})
    link_directories (${JACK_LIBRARY_DIRS})
    link_libraries (${JACK_LIBRARIES})
  # set (CMAKE_REQUIRED_LIBRARIES "${JACK_LIBRARIES};${CMAKE_REQUIRED_LIBRARIES}")
  else ()
    message (WARNING "*** JACK library not found.")
    set (CONFIG_JACK_MIDI 0)
  endif ()
endif ()


add_subdirectory (src)

configure_file (qmidinet.spec.in qmidinet.spec IMMEDIATE @ONLY)

install (FILES qmidinet.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install (FILES qmidinet.fr.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/fr/man1 RENAME qmidinet.1)

# Configuration status
macro (SHOW_OPTION text value)
  if (${value})
    message ("${text}: yes")
  else ()
    message ("${text}: no")
  endif ()
endmacro ()


message   ("\n  ${PACKAGE_NAME} ${PACKAGE_VERSION} (Qt ${QT_VERSION})")
message   ("\n  Build target . . . . . . . . . . . . . . . . . . .: ${CONFIG_BUILD_TYPE}\n")
show_option ("  ALSA MIDI support  . . . . . . . . . . . . . . . ." CONFIG_ALSA_MIDI)
show_option ("  JACK MIDI support  . . . . . . . . . . . . . . . ." CONFIG_JACK_MIDI)
message     ("")
show_option ("  Network IPv6 support (EXPERIMENTAL)  . . . . . . ." CONFIG_IPV6)
message   ("\n  Install prefix . . . . . . . . . . . . . . . . . .: ${CMAKE_INSTALL_PREFIX}")
message   ("\nNow type 'make', followed by 'make install' as root.\n")
