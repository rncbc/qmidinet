// qmidinetOptions.cpp
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

#include "qmidinetAbout.h"
#include "qmidinetOptions.h"

#include <QTextStream>


//-------------------------------------------------------------------------
// qmidinetOptions - Prototype settings structure (pseudo-singleton).
//

// Singleton instance pointer.
qmidinetOptions *qmidinetOptions::g_pOptions = NULL;

// Singleton instance accessor (static).
qmidinetOptions *qmidinetOptions::getInstance (void)
{
	return g_pOptions;
}


// Constructor.
qmidinetOptions::qmidinetOptions (void)
	: m_settings(QMIDINET_DOMAIN, QMIDINET_TITLE)
{
	// Pseudo-singleton reference setup.
	g_pOptions = this;

	loadOptions();
}


// Default Destructor.
qmidinetOptions::~qmidinetOptions (void)
{
	saveOptions();

	// Pseudo-singleton reference shut-down.
	g_pOptions = NULL;
}


// Explicit load method.
void qmidinetOptions::loadOptions (void)
{
	// And go into general options group.
	m_settings.beginGroup("/Options");

	// General options...
	m_settings.beginGroup("/General");
	iNumPorts = m_settings.value("/NumPorts", 1).toInt();
	m_settings.endGroup();

	// Network specific options...
	m_settings.beginGroup("/Network");
	sInterface = m_settings.value("/Interface").toString();
	iUdpPort = m_settings.value("/UdpPort", 21928).toInt();
	m_settings.endGroup();

	m_settings.endGroup();
}


// Explicit save method.
void qmidinetOptions::saveOptions (void)
{
	// And go into general options group.
	m_settings.beginGroup("/Options");

	// General options...
	m_settings.beginGroup("/General");
	m_settings.setValue("/NumPorts", iNumPorts);
	m_settings.endGroup();

	// Network specific options...
	m_settings.beginGroup("/Network");
	m_settings.setValue("/Interface", sInterface);
	m_settings.setValue("/UdpPort", iUdpPort);
	m_settings.endGroup();

	m_settings.endGroup();

	// Save/commit to disk.
	m_settings.sync();
}


// Settings accessor.
QSettings& qmidinetOptions::settings (void)
{
	return m_settings;
}


// Help about command line options.
void qmidinetOptions::print_usage ( const QString& arg0 )
{
	QTextStream out(stderr);
	const QString sEot = "\n\t";
	const QString sEol = "\n\n";

	out << QMIDINET_TITLE " - " << QObject::tr(QMIDINET_SUBTITLE) + sEol;
	out << QObject::tr("Usage: %1 [options]").arg(arg0) + sEol;
	out << QObject::tr("Options:") + sEol;
	out << "  -n, --num-ports=[num-ports]" + sEot +
			QObject::tr("Use this number of ports (default=1)") + sEol;
	out << "  -i, --interface=[interface]" + sEot +
		QObject::tr("Use specific network interface (default=all)") + sEol;
	out << "  -p, --port=[port]" + sEot +
			QObject::tr("Use specific network port (default=21928)") + sEol;
	out << "  -h, --help" + sEot +
		QObject::tr("Show help about command line options") + sEol;
	out << "  -v, --version" + sEot +
		QObject::tr("Show version information") + sEol;
}


// Parse command line arguments into m_settings.
bool qmidinetOptions::parse_args ( const QStringList& args )
{
	QTextStream out(stderr);
	const QString sEot = "\n\t";
	const QString sEol = "\n\n";
	int argc = args.count();

	for (int i = 1; i < argc; ++i) {

		QString sVal;
		QString sArg = args.at(i);
		int iEqual = sArg.indexOf('=');
		if (iEqual >= 0) {
			sVal = sArg.right(sArg.length() - iEqual - 1);
			sArg = sArg.left(iEqual);
		}
		else if (i < argc - 1)
			sVal = args.at(i + 1);

		if (sArg == "-n" || sArg == "--num-ports") {
			if (sVal.isEmpty()) {
				out << QObject::tr("Option -n requires an argument (num-ports).") + sEol;
				return false;
			}
			iNumPorts = sVal.toInt();
			if (iEqual < 0)
				i++;
		}
		else
		if (sArg == "-i" || sArg == "--interface") {
			sInterface = sVal; // Maybe empty!
			if (iEqual < 0)
				i++;
		}
		else
		if (sArg == "-p" || sArg == "--port") {
			if (sVal.isEmpty()) {
				out << QObject::tr("Option -p requires an argument (port).") + sEol;
				return false;
			}
			iUdpPort = sVal.toInt();
			if (iEqual < 0)
				i++;
		}
		else
		if (sArg == "-h" || sArg == "--help") {
			print_usage(args.at(0));
			return false;
		}
		else if (sArg == "-v" || sArg == "--version") {
			out << QObject::tr("Qt: %1\n").arg(qVersion());
			out << QObject::tr(QMIDINET_TITLE ": %1\n").arg(QMIDINET_VERSION);
			return false;
		}
	}

	// Alright with argument parsing.
	return true;
}


// end of qmidinetOptions.cpp
