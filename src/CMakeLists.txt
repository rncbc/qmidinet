
include_directories (
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${QT_INCLUDES}
)

link_directories (
  ${CMAKE_CURRENT_BINARY_DIR}
  ${QT_LIBRARY_DIR}
)

configure_file (cmake_config.h ${CMAKE_CURRENT_BINARY_DIR}/config.h)

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

target_link_libraries (qmidinet PRIVATE Qt5::Widgets)

if (CONFIG_XUNIQUE)
  target_link_libraries (qmidinet PRIVATE Qt5::Network)
endif ()

if (CONFIG_ALSA_MIDI)
  target_link_libraries (qmidinet PRIVATE ${ALSA_LIBRARIES})
endif ()

if (CONFIG_JACK_MIDI)
  target_link_libraries (qmidinet PRIVATE ${JACK_LIBRARIES})
endif ()


if (UNIX AND NOT APPLE)
  install (TARGETS qmidinet RUNTIME
     DESTINATION ${CMAKE_INSTALL_BINDIR})
  install (FILES ${CMAKE_CURRENT_BINARY_DIR}/qmidinet.desktop
     DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install (FILES images/qmidinet.png
     DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps)
  install (FILES appdata/qmidinet.appdata.xml
     DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
endif ()
