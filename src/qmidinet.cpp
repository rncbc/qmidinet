// qmidinet.cpp
//
/****************************************************************************
   Copyright (C) 2010, rncbc aka Rui Nuno Capela. All rights reserved.

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


//-------------------------------------------------------------------------
// qmidinetApplication -- Singleton application instance.
//

// Constructor.
qmidinetApplication::qmidinetApplication ( int& argc, char **argv )
	: QApplication(argc, argv), m_icon(this), m_udpd(this)
	#ifdef CONFIG_ALSA_MIDI	
		, m_alsa(this)
	#endif
	#ifdef CONFIG_JACK_MIDI	
		, m_jack(this)
	#endif
{
	m_menu.addAction(
		QIcon(":/images/qmidinet.png"),
		tr("Options..."), this, SLOT(options()));
	m_menu.addAction(tr("Reset"), this, SLOT(reset()));
	m_menu.addSeparator();
	m_menu.addAction(tr("About..."), this, SLOT(about()));
	m_menu.addAction(tr("About Qt..."), this, SLOT(aboutQt()));
	m_menu.addSeparator();
	m_menu.addAction(
		QIcon(":/images/formReject.png"),
		tr("Quit"), this, SLOT(quit()));

	m_icon.setContextMenu(&m_menu);
	m_icon.setToolTip(QMIDINET_TITLE " - " + tr(QMIDINET_SUBTITLE));

	QObject::connect(&m_icon,
		SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
		SLOT(activated(QSystemTrayIcon::ActivationReason)));

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
	QObject::connect(
		&m_jack, SIGNAL(shutdown()), SLOT(shutdown()));
#endif

	QApplication::setQuitOnLastWindowClosed(false);
}


// Initializer.
bool qmidinetApplication::setup (void)
{
	qmidinetOptions *pOptions = qmidinetOptions::getInstance();
	if (pOptions == NULL)
		return false;

#ifdef CONFIG_ALSA_MIDI
	m_alsa.close();
#endif
#ifdef CONFIG_JACK_MIDI	
	m_jack.close();
#endif

	if (!m_udpd.open(
			pOptions->sInterface,
			pOptions->iUdpPort,
			pOptions->iNumPorts)) {
		message(tr("Network Inferface Error - %1").arg(QMIDINET_TITLE),
			tr("The network interface could not be established.\n\n"
			"Please, make sure you have an on-line network connection "
			"and try again."));
		return false;
	}

#ifdef CONFIG_ALSA_MIDI
	if (pOptions->bAlsaMidi
		&& !m_alsa.open(QMIDINET_TITLE, pOptions->iNumPorts)) {
		m_udpd.close();
		message(tr("ALSA MIDI Inferface Error - %1").arg(QMIDINET_TITLE),
			tr("The ALSA MIDI interface could not be established.\n\n"
			"Please, make sure you have a ALSA MIDI sub-system working"
			"correctly and try again."));
		return false;
	}
#endif

#ifdef CONFIG_JACK_MIDI
	if (pOptions->bJackMidi
		&& !m_jack.open(QMIDINET_TITLE, pOptions->iNumPorts)) {
		m_udpd.close();
	#ifdef CONFIG_ALSA_MIDI
		m_alsa.close();
	#endif
		message(tr("JACK MIDI Inferface Error - %1").arg(QMIDINET_TITLE),
			tr("The JACK MIDI interface could not be established.\n\n"
			"Please, make sure you have a JACK MIDI sub-system working"
			"correctly and try again."));
		return false;
	}
#endif

	return true;
}


void qmidinetApplication::show ( bool bSetup )
{
	const QIcon icon(":/images/qmidinet.png");
	QPixmap pm(icon.pixmap(22, 22));

	if (!bSetup) {
		// Merge with the overlay pixmap...
		const QPixmap pmOverlay(":/images/iconError.png");
		if (!pmOverlay.mask().isNull()) {
			QBitmap mask = pm.mask();
			QPainter(&mask).drawPixmap(0, 0, pmOverlay.mask());
			pm.setMask(mask);
			QPainter(&pm).drawPixmap(0, 0, pmOverlay);
		}
	}

	m_icon.setIcon(QIcon(pm));
	m_icon.show();
}


// Options dialog.
void qmidinetApplication::options (void)
{
	if (qmidinetOptionsForm(NULL).exec())
		reset();
}


// Restart/reset action
void qmidinetApplication::reset (void)
{
	show(setup());
}


// About dialog.
void qmidinetApplication::about (void)
{
	// Stuff the about box text...
	QString sText = "<p>\n";
	sText += "<b>" QMIDINET_TITLE " - " + tr(QMIDINET_SUBTITLE) + "</b><br />\n";
	sText += "<br />\n";
	sText += tr("Version") + ": <b>" QMIDINET_VERSION "</b><br />\n";
	sText += "<small>" + tr("Build") + ": " __DATE__ " " __TIME__ "</small><br />\n";
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
	abox.setWindowIcon(m_icon.icon());
	abox.setWindowTitle(tr("About %1").arg(QMIDINET_TITLE));
	abox.setIconPixmap(QPixmap(":/images/qmidinet.png"));
	abox.setText(sText);
	abox.exec();
}


// Message bubble/dialog.
void qmidinetApplication::message (
	const QString& sTitle, const QString& sText )
{
	if (m_icon.supportsMessages()) {
		m_icon.showMessage(sTitle, sText, QSystemTrayIcon::Critical);
	} else {
		QMessageBox::critical(NULL, sTitle, sText);
	}
}


// Handle systeam tray activity.
void qmidinetApplication::activated ( QSystemTrayIcon::ActivationReason reason )
{
	if (reason == QSystemTrayIcon::Trigger)
		m_icon.contextMenu()->exec(QCursor::pos());
}


#ifdef CONFIG_JACK_MIDI
void qmidinetApplication::shutdown (void)
{
	m_jack.close();

	message(tr("JACK MIDI Inferface Error - %1").arg(QMIDINET_TITLE),
		tr("The JACK MIDI interface has been shutdown.\n\n"
		"Please, make sure you reactivate the JACK MIDI sub-system"
		"and try again."));

	show(false);
}
#endif


//-------------------------------------------------------------------------
// main - The main program trunk.
//

int main ( int argc, char* argv[] )
{
	Q_INIT_RESOURCE(qmidinet);

	qmidinetApplication app(argc, argv);

	qmidinetOptions opts;
	if (!opts.parse_args(app.arguments())) {
		app.quit();
		return 1;
	}

	app.reset();

	return app.exec();
}


// end of qmidinet.cpp
