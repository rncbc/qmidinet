# qmidinet.pro
#
TARGET = qmidinet

TEMPLATE = app
DEPENDPATH += .
INCLUDEPATH += .

include(src.pri)

#DEFINES += DEBUG

HEADERS += config.h \
	qmidinet.h \
	qmidinetAbout.h \
	qmidinetUdpDevice.h \
	qmidinetAlsaMidiDevice.h \
	qmidinetJackMidiDevice.h \
	qmidinetOptions.h \
	qmidinetOptionsForm.h

SOURCES += \
	qmidinet.cpp \
	qmidinetUdpDevice.cpp \
	qmidinetAlsaMidiDevice.cpp \
	qmidinetJackMidiDevice.cpp \
	qmidinetOptions.cpp \
	qmidinetOptionsForm.cpp

FORMS += qmidinetOptionsForm.ui

RESOURCES += qmidinet.qrc

unix {

	#VARIABLES
	OBJECTS_DIR = .obj
	MOC_DIR     = .moc
	UI_DIR      = .ui

	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}

	BINDIR = $(bindir)
	DATADIR = $(datadir)

	DEFINES += BINDIR=\"$$BINDIR\"
	DEFINES += DATADIR=\"$$DATADIR\"

	#MAKE INSTALL
	INSTALLS += target desktop icon

	target.path = $$BINDIR

	desktop.path = $$DATADIR/applications
	desktop.files += $${TARGET}.desktop

	icon.path = $$DATADIR/icons/hicolor/32x32/apps
	icon.files += images/$${TARGET}.png 
}
