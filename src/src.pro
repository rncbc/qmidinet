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

	# variables
	OBJECTS_DIR = .obj
	MOC_DIR     = .moc
	UI_DIR      = .ui

	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}

	isEmpty(BINDIR) {
		BINDIR = $${PREFIX}/bin
	}

	isEmpty(DATADIR) {
		DATADIR = $${PREFIX}/share
	}

	#DEFINES += DATADIR=\"$${DATADIR}\"

	# make install
	INSTALLS += target desktop icon appdata

	target.path = $${BINDIR}

	desktop.path = $${DATADIR}/applications
	desktop.files += $${TARGET}.desktop

	icon.path = $${DATADIR}/icons/hicolor/32x32/apps
	icon.files += images/$${TARGET}.png 

	appdata.path = $${DATADIR}/appdata
	appdata.files += appdata/$${TARGET}.appdata.xml
}


# QT5 support
!lessThan(QT_MAJOR_VERSION, 5) {
	QT += widgets
}
