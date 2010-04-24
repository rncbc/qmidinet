// qmidinet.h
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

#ifndef __qmidinet_h
#define __qmidinet_h

#include "qmidinetUdpDevice.h"
#include "qmidinetMidiDevice.h"

#include <QApplication>

#include <QSystemTrayIcon>
#include <QMenu>


//-------------------------------------------------------------------------
// qmidinetApplication -- Singleton application instance.
//

class qmidinetApplication : public QApplication
{
	Q_OBJECT

public:

	// Constructor.
	qmidinetApplication(int& argc, char **argv);

	// Initializers.
	bool setup();
	void show();

	// Message bubble/dialog.
	void message(const QString& sTitle, const QString& sText);

public slots:

	// Options dialog.
	void options();
	void about();

private:

	// Instance variables.
	QMenu              m_menu;
	QSystemTrayIcon    m_icon;
	qmidinetUdpDevice  m_udpd;
	qmidinetMidiDevice m_midi;
};


#endif	// __qmidinet_h

// end of qmidinet.h
