set(NAME qmidinet)

include_directories (
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

configure_file (cmake_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

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

add_executable (${NAME}
  ${UI_SOURCES}
  ${MOC_SOURCES}
  ${QRC_SOURCES}
  ${SOURCES}
)

set_target_properties (${NAME} PROPERTIES CXX_STANDARD 11)

if (WIN32)
  set_target_properties (${NAME} PROPERTIES WIN32_EXECUTABLE true)
endif ()

if (APPLE)
  set_target_properties (${NAME} PROPERTIES MACOSX_BUNDLE true)
endif ()

target_link_libraries (${NAME} PRIVATE Qt5::Widgets)

if (CONFIG_IPV6)
  target_link_libraries (${NAME} PRIVATE Qt5::Network)
endif ()

if (CONFIG_ALSA_MIDI)
  target_link_libraries (${NAME} PRIVATE ${ALSA_LIBRARIES})
endif ()

if (CONFIG_JACK_MIDI)
  target_link_libraries (${NAME} PRIVATE ${JACK_LIBRARIES})
endif ()


if (UNIX AND NOT APPLE)
  install (TARGETS ${NAME} RUNTIME
     DESTINATION ${CMAKE_INSTALL_BINDIR})
  install (FILES ${NAME}.desktop
     DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install (FILES images/${NAME}.png
     DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps)
  install (FILES images/${NAME}.svg
     DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
  install (FILES appdata/${NAME}.appdata.xml
     DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
endif ()

if (WIN32)
   install (TARGETS ${NAME} RUNTIME
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif ()
