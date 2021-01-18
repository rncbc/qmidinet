# project(qmidinet)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

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

if (QT_VERSION VERSION_LESS 5.15.0)
  qt5_wrap_ui (UI_SOURCES ${FORMS})
  qt5_wrap_cpp (MOC_SOURCES ${HEADERS})
  qt5_add_resources (QRC_SOURCES ${RESOURCES})
else ()
  qt_wrap_ui (UI_SOURCES ${FORMS})
  qt_wrap_cpp (MOC_SOURCES ${HEADERS})
  qt_add_resources (QRC_SOURCES ${RESOURCES})
endif ()


add_executable (${PROJECT_NAME}
  ${UI_SOURCES}
  ${MOC_SOURCES}
  ${QRC_SOURCES}
  ${SOURCES}
)

set_target_properties (${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

if (WIN32)
  set_target_properties (${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE true)
endif ()

if (APPLE)
  set_target_properties (${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE true)
endif ()

target_link_libraries (${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if (CONFIG_IPV6)
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
  install (FILES ${PROJECT_NAME}.desktop
     DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
  install (FILES images/${PROJECT_NAME}.png
     DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps)
  install (FILES images/${PROJECT_NAME}.svg
     DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
  install (FILES appdata/${PROJECT_NAME}.appdata.xml
     DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
endif ()

if (WIN32)
   install (TARGETS ${PROJECT_NAME} RUNTIME
     DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif ()
