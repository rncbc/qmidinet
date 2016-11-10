// qmidinet.cpp
//
/****************************************************************************
   Copyright (C) 2010-2016, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#include "qmidinet.h"

#include "qmidinetOptions.h"
#include "qmidinetOptionsForm.h"

#include <QMessageBox>
#include <QBitmap>
#include <QPainter>
#include <QTimer>


//-------------------------------------------------------------------------
// qmidinetApplication -- Singleton application instance.
//

// Constructor.
qmidinetApplication::qmidinetApplication ( int& argc, char **argv, bool bGUI )
	: QObject(NULL), m_pApp(NULL), m_pIcon(NULL)
	#ifdef CONFIG_ALSA_MIDI	
		, m_alsa(this)
	#endif
	#ifdef CONFIG_JACK_MIDI	
		, m_jack(this)
	#endif
		, m_udpd(this)
{
	if (bGUI) {
		QApplication *pApp = new QApplication(argc, argv);
		pApp->setQuitOnLastWindowClosed(false);
		m_pApp  = pApp;
		m_pIcon = new qmidinetSystemTrayIcon(this);
	} else {
		m_pApp  = new QCoreApplication(argc, argv);
		m_pIcon = NULL;
	}


#ifdef CONFIG_ALSA_MIDI
	QObject::connect(
		&m_udpd, SIGNAL(received(const QByteArray&, int)),
		&m_alsa, SLOT(receive(const QByteArray&, int)));
	QObject::connect(
		&m_alsa, SIGNAL(received(const QByteArray&, int)),
		&m_udpd, SLOT(receive(const QByteArray&, int)));
#endif

#ifdef CONFIG_JACK_MIDI
	QObject::connect(
		&m_udpd, SIGNAL(received(const QByteArray&, int)),
		&m_jack, SLOT(receive(const QByteArray&, int)));
	QObject::connect(
		&m_jack, SIGNAL(received(const QByteArray&, int)),
		&m_udpd, SLOT(receive(const QByteArray&, int)));
	QObject::connect(&m_jack,
		SIGNAL(shutdown()),
		SLOT(shutdown()));
#endif

	if (m_pIcon) {
		QObject::connect(
			&m_udpd, SIGNAL(received(const QByteArray&, int)),
			m_pIcon, SLOT(receiving()));
	#ifdef CONFIG_ALSA_MIDI
		QObject::connect(
			&m_alsa, SIGNAL(received(const QByteArray&, int)),
			m_pIcon, SLOT(sending()));
	#endif
	#ifdef CONFIG_JACK_MIDI
		QObject::connect(
			&m_jack, SIGNAL(received(const QByteArray&, int)),
			m_pIcon, SLOT(sending()));
	#endif
	}
}


// Destructor.
qmidinetApplication::~qmidinetApplication (void)
{
	if (m_pIcon) delete m_pIcon;
	if (m_pApp)  delete m_pApp;
}


// Initializer.
bool qmidinetApplication::setup (void)
{
	qmidinetOptions *pOptions = qmidinetOptions::getInstance();
	if (pOptions == NULL)
		return false;

	m_udpd.close();
#ifdef CONFIG_JACK_MIDI	
	m_jack.close();
#endif
#ifdef CONFIG_ALSA_MIDI
	m_alsa.close();
#endif

#ifdef CONFIG_ALSA_MIDI
	if (pOptions->bAlsaMidi
		&& !m_alsa.open(QMIDINET_TITLE, pOptions->iNumPorts)) {
		message(tr("ALSA MIDI Inferface Error - %1").arg(QMIDINET_TITLE),
			tr("The ALSA MIDI interface could not be established.\n\n"
			"Please, make sure you have a ALSA MIDI sub-system working "
			"correctly and try again."));
		return false;
	}
#endif

#ifdef CONFIG_JACK_MIDI
	if (pOptions->bJackMidi
		&& !m_jack.open(QMIDINET_TITLE, pOptions->iNumPorts)) {
	#ifdef CONFIG_ALSA_MIDI
		m_alsa.close();
	#endif
		message(tr("JACK MIDI Inferface Error - %1").arg(QMIDINET_TITLE),
			tr("The JACK MIDI interface could not be established.\n\n"
			"Please, make sure you have a JACK MIDI sub-system working "
			"correctly and try again."));
		return false;
	}
#endif

	if (!m_udpd.open(
			pOptions->sInterface,
			pOptions->sUdpAddr,
			pOptions->iUdpPort,
			pOptions->iNumPorts)) {
	#ifdef CONFIG_ALSA_MIDI
		m_alsa.close();
	#endif
	#ifdef CONFIG_JACK_MIDI
		m_jack.close();
	#endif
		message(tr("Network Inferface Error - %1").arg(QMIDINET_TITLE),
			tr("The network interface could not be established.\n\n"
			"Please, make sure you have an on-line network connection "
			"and try again."));
		return false;
	}

	return true;
}


void qmidinetApplication::reset (void)
{
	if (m_pIcon)
		m_pIcon->reset();
	else if (!setup())
		QTimer::singleShot(180000, this, SLOT(reset()));
}


// Message bubble/dialog.
void qmidinetApplication::message (
	const QString& sTitle, const QString& sText )
{
	if (m_pIcon && m_pIcon->isVisible()) {
		m_pIcon->message(sTitle, sText);
	} else {
		const QString sMessage = sTitle + ": " + sText.simplified();
		qCritical(sMessage.toUtf8().constData());
	}
}


#ifdef CONFIG_JACK_MIDI
void qmidinetApplication::shutdown (void)
{
	m_jack.close();

	message(tr("JACK MIDI Inferface Error - %1").arg(QMIDINET_TITLE),
		tr("The JACK MIDI interface has been shutdown.\n\n"
		"Please, make sure you reactivate the JACK MIDI sub-system "
		"and try again."));

	if (m_pIcon)
		m_pIcon->show(false);
}
#endif


//-------------------------------------------------------------------------
// qmidinetSystemTrayIcon -- Singleton application instance.
//

// Constructor.
qmidinetSystemTrayIcon::qmidinetSystemTrayIcon ( qmidinetApplication *pApp )
	: QSystemTrayIcon(pApp), m_pApp(pApp), m_iSending(0), m_iReceiving(0)
{
	m_menu.addAction(
		QIcon(":/images/qmidinet.png"),
		tr("Options..."), this, SLOT(options()));
	m_menu.addAction(tr("Reset"), this, SLOT(reset()));
	m_menu.addSeparator();
	m_menu.addAction(tr("About..."), this, SLOT(about()));
	m_menu.addAction(tr("About Qt..."), m_pApp->app(), SLOT(aboutQt()));
	m_menu.addSeparator();
	m_menu.addAction(
		QIcon(":/images/formReject.png"),
		tr("Quit"), m_pApp->app(), SLOT(quit()));

	QSystemTrayIcon::setContextMenu(&m_menu);
	QSystemTrayIcon::setToolTip(QMIDINET_TITLE " - " + tr(QMIDINET_SUBTITLE));

	QObject::connect(this,
		SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
		SLOT(activated(QSystemTrayIcon::ActivationReason)));
}


// Initializer.
void qmidinetSystemTrayIcon::show ( bool bSetup )
{
	QPixmap pm(":/images/qmidinet.png");

	if (!bSetup) {
		// Merge with the error overlay pixmap...
		const QPixmap pmError(":/images/iconError.png");
		if (!pmError.mask().isNull()) {
			QBitmap mask = pm.mask();
			QPainter(&mask).drawPixmap(0, 0, pmError.mask());
			pm.setMask(mask);
			QPainter(&pm).drawPixmap(0, 0, pmError);
		}
		// Restart timeout (3 minutes)...
		QTimer::singleShot(180000, this, SLOT(reset()));
	} else {
		// Merge with the status overlay pixmaps...
		if (m_iSending > 0) {
			const QPixmap pmSend(":/images/iconSend.png");
			if (!pmSend.mask().isNull()) {
				QBitmap mask = pm.mask();
				QPainter(&mask).drawPixmap(0, 0, pmSend.mask());
				pm.setMask(mask);
				QPainter(&pm).drawPixmap(0, 0, pmSend);
			}
		}
		if (m_iReceiving > 0) {
			const QPixmap pmReceive(":/images/iconReceive.png");
			if (!pmReceive.mask().isNull()) {
				QBitmap mask = pm.mask();
				QPainter(&mask).drawPixmap(0, 0, pmReceive.mask());
				pm.setMask(mask);
				QPainter(&pm).drawPixmap(0, 0, pmReceive);
			}
		}
	}

	QSystemTrayIcon::setIcon(QIcon(pm));
	QSystemTrayIcon::show();
}


// Options dialog.
void qmidinetSystemTrayIcon::options (void)
{
	if (qmidinetOptionsForm(NULL).exec())
		reset();
}


// Restart/reset action
void qmidinetSystemTrayIcon::reset (void)
{
	show(m_pApp->setup());
}


// About dialog.
void qmidinetSystemTrayIcon::about (void)
{
	// Stuff the about box text...
	QString sText = "<p>\n";
	sText += "<b>" QMIDINET_TITLE " - " + tr(QMIDINET_SUBTITLE) + "</b><br />\n";
	sText += "<br />\n";
	sText += tr("Version") + ": <b>" CONFIG_BUILD_VERSION "</b><br />\n";
	sText += "<small>" + tr("Build") + ": " CONFIG_BUILD_DATE "</small><br />\n";
#ifndef CONFIG_ALSA_MIDI
	sText += "<small><font color=\"red\">";
	sText += tr("ALSA MIDI support disabled.");
	sText += "</font></small><br />";
#endif
#ifndef CONFIG_JACK_MIDI
	sText += "<small><font color=\"red\">";
	sText += tr("JACK MIDI support disabled.");
	sText += "</font></small><br />";
#endif
	sText += "<br />\n";
	sText += tr("Website") + ": <a href=\"" QMIDINET_WEBSITE "\">" QMIDINET_WEBSITE "</a><br />\n";
	sText += "<br />\n";
	sText += "<small>";
	sText += QMIDINET_COPYRIGHT "<br />\n";
	sText += "<br />\n";
	sText += tr("This program is free software; you can redistribute it and/or modify it") + "<br />\n";
	sText += tr("under the terms of the GNU General Public License version 2 or later.");
	sText += "</small>";
	sText += "</p>\n";

	QMessageBox abox;
	abox.setWindowIcon(QSystemTrayIcon::icon());
	abox.setWindowTitle(tr("About %1").arg(QMIDINET_TITLE));
	abox.setIconPixmap(QPixmap(":/images/qmidinet.png"));
	abox.setText(sText);
	abox.exec();
}


// Message bubble/dialog.
void qmidinetSystemTrayIcon::message (
	const QString& sTitle, const QString& sText )
{
	if (QSystemTrayIcon::supportsMessages()) {
		QSystemTrayIcon::showMessage(sTitle, sText, QSystemTrayIcon::Critical);
	} else {
		QMessageBox::critical(NULL, sTitle, sText);
	}
}


// Handle systeam tray activity.
void qmidinetSystemTrayIcon::activated ( QSystemTrayIcon::ActivationReason reason )
{
	if (reason == QSystemTrayIcon::Trigger)
		QSystemTrayIcon::contextMenu()->exec(QCursor::pos());
}


// Status changes.
void qmidinetSystemTrayIcon::sending (void)
{
	if (++m_iSending < 2) {
		show(true);
		QTimer::singleShot(200, this, SLOT(timerOff()));
	}
}


void qmidinetSystemTrayIcon::receiving (void)
{
	if (++m_iReceiving < 2) {
		show(true);
		QTimer::singleShot(200, this, SLOT(timerOff()));
	}
}


void qmidinetSystemTrayIcon::timerOff (void)
{
	m_iSending = m_iReceiving = 0;

	show(true);
}


//-------------------------------------------------------------------------
// main - The main program trunk.
//

int main ( int argc, char* argv[] )
{
	Q_INIT_RESOURCE(qmidinet);

#ifdef Q_WS_X11
	bool bGUI = (::getenv("DISPLAY") != 0);
#else
	bool bGUI = true;
#endif
	if (bGUI) for (int i = 1; i < argc; ++i) {
		const QString& sArg = QString::fromLocal8Bit(argv[i]);
		if (sArg == "-g" || sArg == "--no-gui") {
			bGUI = false;
			break;
		}
	}

	qmidinetApplication app(argc, argv, bGUI);

	qmidinetOptions opts;
	if (!opts.parse_args(app.app()->arguments())) {
		app.app()->quit();
		return 1;
	}

	app.reset();

	return app.app()->exec();
}


// end of qmidinet.cpp
