project(QMIDINET)

cmake_minimum_required(VERSION 2.6)

set (VERSION "0.5.6")

execute_process (
  COMMAND git describe --tags --dirty --abbrev=6
  OUTPUT_VARIABLE GIT_DESCRIBE_OUTPUT
  RESULT_VARIABLE GIT_DESCRIBE_RESULT
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if (GIT_DESCRIBE_RESULT EQUAL 0)
  set (VERSION "${GIT_DESCRIBE_OUTPUT}")
  string (REGEX REPLACE "^[^_vV]+" "" VERSION "${VERSION}")
  string (REGEX REPLACE "^[_vV]+"  "" VERSION "${VERSION}")
  string (REGEX REPLACE "-g"   "git." VERSION "${VERSION}")
  string (REGEX REPLACE "[_|-]+"  "." VERSION "${VERSION}")
  execute_process (
    COMMAND git rev-parse --abbrev-ref HEAD
    OUTPUT_VARIABLE GIT_REVPARSE_OUTPUT
    RESULT_VARIABLE GIT_REVPARSE_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (GIT_REVPARSE_RESULT EQUAL 0 AND NOT GIT_REVPARSE_OUTPUT STREQUAL "master")
    set (VERSION "${VERSION} [${GIT_REVPARSE_OUTPUT}]")
  endif ()  
endif ()

set (PACKAGE_NAME "QmidiNet")
set (PACKAGE_VERSION "${VERSION}")
set (PACKAGE_BUGREPORT "rncbc@rncbc.org")
set (PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set (PACKAGE_TARNAME "qmidinet")

set (CONFIG_BUILD_VERSION "${PACKAGE_VERSION}")

if (CMAKE_BUILD_TYPE)
  set (CONFIG_BUILD_TYPE CMAKE_BUILD_TYPE)
else ()
  set (CONFIG_BUILD_TYPE "release")
endif ()

set (CONFIG_DEBUG)
if (CONFIG_BUILD_TYPE MATCHES "debug")
  set(CONFIG_DEBUG 1)
endif ()
set (CONFIG_PREFIX "${CMAKE_INSTALL_PREFIX}")


# Enable ALSA MIDI support option.
option (CONFIG_ALSA_MIDI "Enable ALSA MIDI support (default=yes)" 1)

# Enable JACK MIDI support option.
option (CONFIG_JACK_MIDI "Enable JACK MIDI support (default=yes)" 1)

# Fix for new CMAKE_REQUIRED_LIBRARIES policy.
if (POLICY CMP0075)
  cmake_policy (SET CMP0075 NEW)
endif ()

# Check for Qt
find_package (Qt5 REQUIRED NO_MODULE COMPONENTS Core Gui Widgets Network)
find_package (Qt5LinguistTools)

include (GNUInstallDirs)
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
pkg_check_modules (ALSA alsa)
set (CONFIG_ALSA_MIDI ${ALSA_FOUND})
if (CONFIG_ALSA_MIDI)
  include_directories (${ALSA_INCLUDE_DIRS})
  link_directories (${ALSA_LIBRARY_DIRS})
  set (CMAKE_REQUIRED_LIBRARIES "${ALSA_LIBRARIES};${CMAKE_REQUIRED_LIBRARIES}")
else ()
  message (WARNING "*** ALSA library not found.")
endif ()

# Check for JACK libraries.
pkg_check_modules (JACK jack>=0.100.0)
set (CONFIG_JACK_MIDI ${JACK_FOUND})
if (CONFIG_JACK_MIDI)
  include_directories (${JACK_INCLUDE_DIRS})
  link_directories (${JACK_LIBRARY_DIRS})
  set (CMAKE_REQUIRED_LIBRARIES "${JACK_LIBRARIES};${CMAKE_REQUIRED_LIBRARIES}")
else ()
  message (WARNING "*** JACK library not found.")
endif ()


add_subdirectory (src)

configure_file (qmidinet.spec.in qmidinet.spec IMMEDIATE @ONLY)

install (FILES qmidinet.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
install (FILES qmidinet.fr.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/fr/man1 RENAME qmidinet.1)

# Configuration status
macro (SHOW_OPTION text value)
  if (${value})
    message("${text}: yes")
  else ()
    message("${text}: no")
  endif ()
endmacro ()


message   ("\n  ${PACKAGE_NAME} ${PACKAGE_VERSION}")
message   ("\n  Build target . . . . . . . . . . . . . . . . . . .: ${CONFIG_BUILD_TYPE}\n")
show_option ("  ALSA MIDI support  . . . . . . . . . . . . . . . ." CONFIG_ALSA_MIDI)
show_option ("  JACK MIDI support  . . . . . . . . . . . . . . . ." CONFIG_JACK_MIDI)
message   ("\n  Install prefix . . . . . . . . . . . . . . . . . .: ${CMAKE_INSTALL_PREFIX}")
message   ("\nNow type 'make', followed by 'make install' as root.\n")
