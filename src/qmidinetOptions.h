// qmidinetOptions.h
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

#ifndef __qmidinetOptions_h
#define __qmidinetOptions_h

#include <QSettings>
#include <QStringList>


// Some hard-coded default options....
#define QMIDINET_UDP_ADDR "225.0.0.37"
#define QMIDINET_UDP_PORT  21928


//-------------------------------------------------------------------------
// qmidinetOptions - Prototype settings class (singleton).
//

class qmidinetOptions
{
public:

	// Constructor.
	qmidinetOptions();
	// Default destructor.
	~qmidinetOptions();

	// The settings object accessor.
	QSettings& settings();

	// Explicit I/O methods.
	void loadOptions();
	void saveOptions();

	// Command line arguments parser.
	bool parse_args(const QStringList& args);
	// Command line usage helper.
	void print_usage(const QString& arg0);

	// General options...
	int     iNumPorts;
	bool    bAlsaMidi;
	bool    bJackMidi;

	// Network options...
	QString sInterface;
	QString sUdpAddr;
	int     iUdpPort;

	// Singleton instance accessor.
	static qmidinetOptions *getInstance();

private:

	// Settings member variables.
	QSettings m_settings;

	// The singleton instance.
	static qmidinetOptions *g_pOptions;
};


#endif  // __qmidinetOptions_h


// end of qmidinetOptions.h
