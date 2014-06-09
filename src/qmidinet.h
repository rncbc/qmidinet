// qmidinet.h
//
/****************************************************************************
   Copyright (C) 2010-2014, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qmidinet_h
#define __qmidinet_h

#include "qmidinetUdpDevice.h"

#include "qmidinetAlsaMidiDevice.h"
#include "qmidinetJackMidiDevice.h"

#include <QCoreApplication>

#include <QSystemTrayIcon>
#include <QMenu>


// Forward decls.
class qmidinetSystemTrayIcon;


//-------------------------------------------------------------------------
// qmidinetApplication -- Singleton application instance.
//

class qmidinetApplication : public QObject
{
	Q_OBJECT

public:

	// Constructor.
	qmidinetApplication(int& argc, char **argv, bool bGUI);

	// Destructor.
	~qmidinetApplication();

	// Initializers.
	bool setup();

	// Messager.
	void message(const QString& sTitle, const QString& sText);

	// Simple accessor.
	QCoreApplication *app() const { return m_pApp; }

public slots:

	// Action slots...
	void reset();

#ifdef CONFIG_JACK_MIDI
	void shutdown();
#endif

private:

	// Instance variables.
	QCoreApplication *m_pApp;

	qmidinetSystemTrayIcon *m_pIcon;

#ifdef CONFIG_ALSA_MIDI
	qmidinetAlsaMidiDevice m_alsa;
#endif
#ifdef CONFIG_JACK_MIDI
	qmidinetJackMidiDevice m_jack;
#endif
	qmidinetUdpDevice m_udpd;
};


//-------------------------------------------------------------------------
// qmidinetSystemTrayIcon -- Singleton widget instance.
//

class qmidinetSystemTrayIcon : public QSystemTrayIcon
{
	Q_OBJECT

public:

	// Constructor.
	qmidinetSystemTrayIcon(qmidinetApplication *pApp);

	// Initializers.
	void show(bool bSetup);

	// Message bubble/dialog.
	void message(const QString& sTitle, const QString& sText);

public slots:

	// Action slots...
	void options();
	void reset();
	void about();

	// Handle system tray activity.
	void activated(QSystemTrayIcon::ActivationReason);

private:

	// Instance variables.
	qmidinetApplication *m_pApp;

	QMenu m_menu;
};


#endif	// __qmidinet_h

// end of qmidinet.h
