# project (qmidinet)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

set (CMAKE_AUTOUIC ON)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
  file (REMOVE ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
endif ()
configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set (HEADERS
  qmidinet.h
  qmidinetAbout.h
  qmidinetUdpDevice.h
  qmidinetAlsaMidiDevice.h
  qmidinetJackMidiDevice.h
  qmidinetOptions.h
  qmidinetOptionsForm.h
)

set (SOURCES
  qmidinet.cpp
  qmidinetUdpDevice.cpp
  qmidinetAlsaMidiDevice.cpp
  qmidinetJackMidiDevice.cpp
  qmidinetOptions.cpp
  qmidinetOptionsForm.cpp
)

set (FORMS
  qmidinetOptionsForm.ui
)

set (RESOURCES
  qmidinet.qrc
)


add_executable (${PROJECT_NAME}
  ${HEADERS}
  ${SOURCES}
  ${FORMS}
  ${RESOURCES}
)

# Add some debugger flags.
if (CONFIG_DEBUG AND UNIX AND NOT APPLE)
  set (CONFIG_DEBUG_OPTIONS -g -fsanitize=address -fno-omit-frame-pointer)
  target_compile_options (${PROJECT_NAME} PRIVATE ${CONFIG_DEBUG_OPTIONS})
  target_link_options (${PROJECT_NAME} PRIVATE ${CONFIG_DEBUG_OPTIONS})
endif ()

set_target_properties (${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

if (WIN32)
  set_target_properties (${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE true)
endif ()

if (APPLE)
  set_target_properties (${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE true)
endif ()

target_link_libraries (${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Svg)

if (CONFIG_IPV6 OR CONFIG_XUNIQUE)
  target_link_libraries (${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Network)
endif ()

if (CONFIG_ALSA_MIDI)
  target_link_libraries (${PROJECT_NAME} PRIVATE PkgConfig::ALSA)
endif ()

if (CONFIG_JACK_MIDI)
  target_link_libraries (${PROJECT_NAME} PRIVATE PkgConfig::JACK)
endif ()


if (UNIX AND NOT APPLE)
  install (TARGETS ${PROJECT_NAME} RUNTIME
    DESTINATION ${CMAKE_INSTALL_BINDIR})
  install (FILES images/${PROJECT_NAME}.png
    RENAME org.rncbc.${PROJECT_NAME}.png
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps)
  install (FILES images/${PROJECT_NAME}.svg
    RENAME org.rncbc.${PROJECT_NAME}.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
  install (FILES appdata/org.rncbc.${PROJECT_NAME}.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install (FILES appdata/org.rncbc.${PROJECT_NAME}.metainfo.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
  install (FILES man1/${PROJECT_NAME}.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)
  install (FILES man1/${PROJECT_NAME}.fr.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/fr/man1 RENAME ${PROJECT_NAME}.1)
endif ()

if (WIN32)
  install (TARGETS ${PROJECT_NAME} RUNTIME
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif ()
