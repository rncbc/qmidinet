
include_directories (
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${QT_INCLUDES}
)

link_directories (
  ${CMAKE_CURRENT_BINARY_DIR}
  ${QT_LIBRARY_DIR}
)

if (CONFIG_ALSA_MIDI)
  include_directories (${ALSA_INCLUDE_DIRS})
  link_directories (${ALSA_LIBRARY_DIRS})
endif ()

if (CONFIG_JACK_MIDI)
  include_directories (${JACK_INCLUDE_DIRS})
  link_directories (${JACK_LIBRARY_DIRS})
endif ()

configure_file (cmake_config.h ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set (ac_prefix ${CMAKE_INSTALL_PREFIX})

configure_file (qmidinet.desktop ${CMAKE_CURRENT_BINARY_DIR}/qmidinet.desktop)

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

qt5_wrap_ui (UI_SOURCES ${FORMS})
qt5_wrap_cpp (MOC_SOURCES ${HEADERS})
qt5_add_resources (QRC_SOURCES ${RESOURCES})

add_executable (qmidinet
  ${UI_SOURCES}
  ${MOC_SOURCES}
  ${QRC_SOURCES}
  ${SOURCES}
)

target_link_libraries (qmidinet
  ${QT_LIBRARIES}
)

if (CONFIG_ALSA_MIDI)
  target_link_libraries (qmidinet ${ALSA_LIBRARIES})
endif ()

if (CONFIG_JACK_MIDI)
  target_link_libraries (qmidinet ${JACK_LIBRARIES})
endif ()

qt5_use_modules (qmidinet Core Gui Widgets Network)

if (UNIX AND NOT APPLE)
  install (TARGETS qmidinet
     RUNTIME DESTINATION bin)
  install (FILES ${CMAKE_CURRENT_BINARY_DIR}/qmidinet.desktop
     DESTINATION share/applications)
  install (FILES images/qmidinet.png
     DESTINATION share/pixmaps)
  install (FILES appdata/qmidinet.appdata.xml
     DESTINATION share/metainfo)
endif ()
