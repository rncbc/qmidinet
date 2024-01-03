cmake_minimum_required (VERSION 3.15)

project (QmidiNet
  VERSION 0.9.12
  DESCRIPTION "A MIDI Network Gateway via UDP/IP Multicast"
  HOMEPAGE_URL "https://qmidinet.sourceforge.io"
  LANGUAGES C CXX)

set (PROJECT_TITLE "${PROJECT_NAME}")
string (TOLOWER "${PROJECT_TITLE}" PROJECT_NAME)

execute_process (
  COMMAND git describe --tags --dirty --abbrev=6
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_DESCRIBE_OUTPUT
  RESULT_VARIABLE GIT_DESCRIBE_RESULT
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if (GIT_DESCRIBE_RESULT EQUAL 0)
  set (GIT_VERSION "${GIT_DESCRIBE_OUTPUT}")
  string (REGEX REPLACE "^[^0-9]+" "" GIT_VERSION "${GIT_VERSION}")
  string (REGEX REPLACE "^1_"      "" GIT_VERSION "${GIT_VERSION}")
  string (REGEX REPLACE "^[_vV]+"  "" GIT_VERSION "${GIT_VERSION}")
  string (REGEX REPLACE "-g"   "git." GIT_VERSION "${GIT_VERSION}")
  string (REGEX REPLACE "[_|-]"   "." GIT_VERSION "${GIT_VERSION}")
  execute_process (
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_REVPARSE_OUTPUT
    RESULT_VARIABLE GIT_REVPARSE_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (GIT_REVPARSE_RESULT EQUAL 0 AND NOT GIT_REVPARSE_OUTPUT STREQUAL "main")
    set (GIT_VERSION "${GIT_VERSION} [${GIT_REVPARSE_OUTPUT}]")
  endif ()
  set (PROJECT_VERSION "${GIT_VERSION}")
endif ()

set (PROJECT_COPYRIGHT "Copyright (C) 2010-2024, rncbc aka Rui Nuno Capela. All rights reserved.")
set (PROJECT_DOMAIN    "rncbc.org")


if (CMAKE_BUILD_TYPE MATCHES "Debug")
  set (CONFIG_DEBUG 1)
  set (CONFIG_BUILD_TYPE "debug")
else ()
  set (CONFIG_DEBUG 0)
  set (CONFIG_BUILD_TYPE "release")
  set (CMAKE_BUILD_TYPE "Release")
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
option (CONFIG_IPV6 "Enable Network IPv6 support (default=yes)" 1)

# Enable unique/single instance.
option (CONFIG_XUNIQUE "Enable unique/single instance (default=yes)" 1)

# Enable Wayland support option.
option (CONFIG_WAYLAND "Enable Wayland support (EXPERIMENTAL) (default=no)" 0)

# Enable Qt6 build preference.
option (CONFIG_QT6 "Enable Qt6 build (default=yes)" 1)


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

find_package (Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Svg)

if (CONFIG_IPV6 OR CONFIG_XUNIQUE)
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
include (FindPkgConfig)

# Check for ALSA libraries.
if (CONFIG_ALSA_MIDI)
  pkg_check_modules (ALSA IMPORTED_TARGET alsa)
  if (ALSA_FOUND)
    find_library(ALSA_LIBRARY NAMES ${ALSA_LIBRARIES} HINTS ${ALSA_LIBDIR})
  endif ()
  if (ALSA_LIBRARY)
    set (CONFIG_ALSA_MIDI 1)
   #set (CMAKE_REQUIRED_LIBRARIES "${ALSA_LIBRARY};${CMAKE_REQUIRED_LIBRARIES}")
  else ()
    message (WARNING "*** ALSA library not found.")
    set (CONFIG_ALSA_MIDI 0)
  endif ()
endif ()

# Check for JACK libraries.
if (CONFIG_JACK_MIDI)
  pkg_check_modules (JACK IMPORTED_TARGET jack>=0.100.0)
  if (JACK_FOUND)
    find_library(JACK_LIBRARY NAMES ${JACK_LIBRARIES} HINTS ${JACK_LIBDIR})
  endif ()
  if (JACK_LIBRARY)
    set (CONFIG_JACK_MIDI 1)
   #set (CMAKE_REQUIRED_LIBRARIES "${JACK_LIBRARY};${CMAKE_REQUIRED_LIBRARIES}")
  else ()
    message (WARNING "*** JACK library not found.")
    set (CONFIG_JACK_MIDI 0)
  endif ()
endif ()


add_subdirectory (src)


# Configuration status
macro (SHOW_OPTION text value)
  if (${value})
    message ("${text}: yes")
  else ()
    message ("${text}: no")
  endif ()
endmacro ()

message   ("\n  ${PROJECT_TITLE} ${PROJECT_VERSION} (Qt ${QT_VERSION})")
message   ("\n  Build target . . . . . . . . . . . . . . . . . . .: ${CONFIG_BUILD_TYPE}\n")
show_option ("  ALSA MIDI support  . . . . . . . . . . . . . . . ." CONFIG_ALSA_MIDI)
show_option ("  JACK MIDI support  . . . . . . . . . . . . . . . ." CONFIG_JACK_MIDI)
message     ("")
show_option ("  Network IPv6 support . . . . . . . . . . . . . . ." CONFIG_IPV6)
message     ("")
show_option ("  Unique/Single instance support . . . . . . . . . ." CONFIG_XUNIQUE)
message   ("\n  Install prefix . . . . . . . . . . . . . . . . . .: ${CONFIG_PREFIX}\n")
