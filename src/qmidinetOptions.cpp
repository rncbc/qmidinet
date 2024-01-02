// qmidinetOptions.cpp
//
/****************************************************************************
   Copyright (C) 2010-2024, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include <QApplication>

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
#include <QCommandLineParser>
#include <QCommandLineOption>
#if defined(Q_OS_WINDOWS)
#include <QMessageBox>
#endif
#endif


//-------------------------------------------------------------------------
// qmidinetOptions - Prototype settings structure (pseudo-singleton).
//

// Singleton instance pointer.
qmidinetOptions *qmidinetOptions::g_pOptions = nullptr;

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
	g_pOptions = nullptr;
}


// Explicit load method.
void qmidinetOptions::loadOptions (void)
{
	// And go into general options group.
	m_settings.beginGroup("/Options");

	// General options...
	m_settings.beginGroup("/General");
	iNumPorts = m_settings.value("/NumPorts", 1).toInt();
	bAlsaMidi = m_settings.value("/AlsaMidi", true).toBool();
	bJackMidi = m_settings.value("/JackMidi", false).toBool();
	m_settings.endGroup();

	// Network specific options...
	m_settings.beginGroup("/Network");
	sInterface = m_settings.value("/Interface").toString();
	sUdpAddr = m_settings.value("/UdpAddr", QMIDINET_UDP_IPV4_ADDR).toString();
	iUdpPort = m_settings.value("/UdpPort", QMIDINET_UDP_PORT).toInt();
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
	m_settings.setValue("/AlsaMidi", bAlsaMidi);
	m_settings.setValue("/JackMidi", bJackMidi);
	m_settings.endGroup();

	// Network specific options...
	m_settings.beginGroup("/Network");
	m_settings.setValue("/Interface", sInterface);
	m_settings.setValue("/UdpAddr", sUdpAddr);
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


#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)

void qmidinetOptions::show_error( const QString& msg )
{
#if defined(Q_OS_WINDOWS)
	QMessageBox::information(nullptr, QApplication::applicationName(), msg);
#else
	const QByteArray tmp = msg.toUtf8() + '\n';
	::fputs(tmp.constData(), stderr);
#endif
}

#else

// Help about command line options.
void qmidinetOptions::print_usage ( const QString& arg0 )
{
	QTextStream out(stderr);
	const QString sEot = "\n\t";
	const QString sEol = "\n\n";

	out << QObject::tr("Usage: %1 [options]").arg(arg0) + sEol;
	out << QMIDINET_TITLE " - " << QObject::tr(QMIDINET_SUBTITLE) + sEol;
	out << QObject::tr("Options:") + sEol;
	out << "  -n, --num-ports=[num-ports]" + sEot +
		QObject::tr("Use this number of ports (default = %1)")
			.arg(iNumPorts) + sEol;
	out << "  -i, --interface=[interface]" + sEot +
		QObject::tr("Use specific network interface (default = %1)")
			.arg(sInterface.isEmpty() ? "all" : sInterface) + sEol;
	out << "  -u, --udp-addr=[addr]" + sEot +
		QObject::tr("Use specific network address (default = %1)")
			.arg(sUdpAddr) + sEol;
	out << "  -p, --udp-port=[port]" + sEot +
		QObject::tr("Use specific network port (default = %1)")
			.arg(iUdpPort) + sEol;
	out << "  -a, --alsa-midi[=flag]" + sEot +
		QObject::tr("Enable ALSA MIDI (0|1|yes|no|on|off, default = %1)")
			.arg(int(bAlsaMidi)) + sEol;
	out << "  -j, --jack-midi[=flag]" + sEot +
		QObject::tr("Enable JACK MIDI (0|1|yes|no|on|off, default = %1)")
			.arg(int(bJackMidi)) + sEol;
	out << "  -g, --no-gui" + sEot +
		QObject::tr("Disable the graphical user interface (GUI)") + sEol;
	out << "  -h, --help" + sEot +
		QObject::tr("Show help about command line options.") + sEol;
	out << "  -v, --version" + sEot +
		QObject::tr("Show version information.") + sEol;
}

#endif


// Parse command line arguments into m_settings.
bool qmidinetOptions::parse_args ( const QStringList& args )
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)

	QCommandLineParser parser;
	parser.setApplicationDescription(
		QMIDINET_TITLE " - " + QObject::tr(QMIDINET_SUBTITLE));

	parser.addOption({{"n", "num-ports"},
		QObject::tr("Use this number of ports (default = %1)")
			.arg(iNumPorts), "num"});
	parser.addOption({{"i", "interface"},
		QObject::tr("Use specific network interface (default = %1)")
			.arg(sInterface.isEmpty() ? "all" : sInterface), "name"});
	parser.addOption({{"u", "udp-addr"},
		QObject::tr("Use specific network address (default = %1)")
			.arg(sUdpAddr), "addr"});
	parser.addOption({{"p", "udp-port"},
		QObject::tr("Use specific network port (default = %1)")
			.arg(iUdpPort), "port"});
	parser.addOption({{"a", "alsa-midi"},
		QObject::tr("Enable ALSA MIDI (0|1|yes|no|on|off, default = %1)")
			.arg(int(bAlsaMidi)), "flag"});
	parser.addOption({{"j", "jack-midi"},
		QObject::tr("Enable JACK MIDI (0|1|yes|no|on|off, default = %1)")
			.arg(int(bJackMidi)), "flag"});
	parser.addOption({{"g", "no-gui"},
		QObject::tr("Disable the graphical user interface (GUI)")});
	parser.addHelpOption();
	parser.addVersionOption();
	parser.process(args);

	if (parser.isSet("num-ports")) {
		bool bOK = false;
		const int iVal = parser.value("num-ports").toInt(&bOK);
		if (!bOK) {
			show_error(QObject::tr("Option -n requires an argument (num)."));
			return false;
		}
		iNumPorts = iVal;
	}

	if (parser.isSet("interface")) {
		sInterface = parser.value("interface"); // Maybe empty!
	}

	if (parser.isSet("udp-addr")) {
		const QString& sVal = parser.value("udp-addr");
		if (sVal.isEmpty()) {
			show_error(QObject::tr("Option -u requires an argument (addr)."));
			return false;
		}
		sUdpAddr = sVal;
	}

	if (parser.isSet("udp-port")) {
		bool bOK = false;
		const int iVal = parser.value("udp-port").toInt(&bOK);
		if (!bOK) {
			show_error(QObject::tr("Option -p requires an argument (port)."));
			return false;
		}
		iUdpPort = iVal;
	}

	if (parser.isSet("alsa-midi")) {
		const QString& sVal = parser.value("alsa-midi");
		if (sVal.isEmpty()) {
			bAlsaMidi = true;
		} else {
			bAlsaMidi = !(sVal == "0" || sVal == "no" || sVal == "off");
		}
	}

	if (parser.isSet("jack-midi")) {
		const QString& sVal = parser.value("jack-midi");
		if (sVal.isEmpty()) {
			bJackMidi = true;
		} else {
			bJackMidi = !(sVal == "0" || sVal == "no" || sVal == "off");
		}
	}

	if (parser.isSet("no-gui")) {
		// Ignored: parsed on startup...
	}

#else

	QTextStream out(stderr);
	const QString sEot = "\n\t";
	const QString sEol = "\n\n";
	const int argc = args.count();

	for (int i = 1; i < argc; ++i) {

		QString sVal;
		QString sArg = args.at(i);
		const int iEqual = sArg.indexOf('=');
		if (iEqual >= 0) {
			sVal = sArg.right(sArg.length() - iEqual - 1);
			sArg = sArg.left(iEqual);
		} else if (i < argc - 1) {
			sVal = args.at(i + 1);
			if (sVal[0] == '-')
				sVal.clear();
		}

		if (sArg == "-n" || sArg == "--num-ports") {
			if (sVal.isEmpty()) {
				out << QObject::tr("Option -n requires an argument (num-ports).") + sEol;
				return false;
			}
			iNumPorts = sVal.toInt();
			if (iEqual < 0) ++i;
		}
		else
		if (sArg == "-i" || sArg == "--interface") {
			sInterface = sVal; // Maybe empty!
			if (iEqual < 0) ++i;
		}
		else
		if (sArg == "-u" || sArg == "--udp-addr") {
			if (sVal.isEmpty()) {
				out << QObject::tr("Option -d requires an argument (address).") + sEol;
				return false;
			}
			sUdpAddr = sVal;
			if (iEqual < 0) ++i;
		}
		else
		if (sArg == "-p" ||  sArg == "--udp-port") {
			if (sVal.isEmpty()) {
				out << QObject::tr("Option -p requires an argument (port).") + sEol;
				return false;
			}
			iUdpPort = sVal.toInt();
			if (iEqual < 0) ++i;
		}
		else
		if (sArg == "-a" || sArg == "--alsa-midi") {
			if (sVal.isEmpty()) {
				bAlsaMidi = true;
			} else {
				bAlsaMidi = !(sVal == "0" || sVal == "no" || sVal == "off");
				if (iEqual < 0) ++i;
			}
		}
		else
		if (sArg == "-j" || sArg == "--jack-midi") {
			if (sVal.isEmpty()) {
				bJackMidi = true;
			} else {
				bJackMidi = !(sVal == "0" || sVal == "no" || sVal == "off");
				if (iEqual < 0) ++i;
			}
		}
		else
		if (sArg == "-h" || sArg == "--help") {
			print_usage(args.at(0));
			return false;
		}
		else if (sArg == "-v" || sArg == "--version") {
			out << QString("Qt: %1").arg(qVersion());
		#if defined(QT_STATIC)
			out << "-static";
		#endif
			out << '\n';
			out << QString("%1: %2\n")
				.arg(QMIDINET_TITLE)
				.arg(PROJECT_VERSION);
			return false;
		}
	}

#endif

	// Alright with argument parsing.
	return true;
}


// end of qmidinetOptions.cpp
