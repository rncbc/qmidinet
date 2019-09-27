// qmidinetUdpDevice.cpp
//
/****************************************************************************
   Copyright (C) 2010-2019, rncbc aka Rui Nuno Capela. All rights reserved.

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

#include "qmidinetUdpDevice.h"

#if defined(CONFIG_IPV6)

#include <QNetworkInterface>
#include <QByteArray>

#else

#include <stdlib.h>
#include <string.h>

#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
static WSADATA g_wsaData;
typedef int socklen_t;
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
inline void closesocket(int s) { ::close(s); }
#endif

#include <QByteArray>
#include <QThread>


//----------------------------------------------------------------------------
// qmidinetUdpDevice::RecvThread -- Network listener thread.
//

class qmidinetUdpDeviceThread : public QThread
{
public:

	// Constructor.
	qmidinetUdpDeviceThread(int *sockin, int nports = 1);

	// Run-state accessors.
	void setRunState(bool bRunState);
	bool runState() const;

protected:

	// The main thread executive.
	void run();

private:

	// The listener socket.
	int *m_sockin;
	int  m_nports;

	// Whether the thread is logically running.
	volatile bool m_bRunState;
};


// Constructor.
qmidinetUdpDeviceThread::qmidinetUdpDeviceThread ( int *sockin, int nports )
	: QThread(), m_sockin(sockin), m_nports(nports), m_bRunState(false)
{
}


// Run-state accessors.
void qmidinetUdpDeviceThread::setRunState ( bool bRunState )
{
	m_bRunState = bRunState;
}

bool qmidinetUdpDeviceThread::runState (void) const
{
	return m_bRunState;
}


// The main thread executive.
void qmidinetUdpDeviceThread::run (void)
{
	m_bRunState = true;

	while (m_bRunState) {

		// Wait for an network event...
		fd_set fds;
		FD_ZERO(&fds);

		int i, fdmax = 0;
		for(i = 0; i < m_nports; ++i) {
			FD_SET(m_sockin[i], &fds);
			if (m_sockin[i] > fdmax)
				fdmax = m_sockin[i];
		}

		// Set timeout period (1 second)...
		struct timeval tv;
		tv.tv_sec  = 1;
		tv.tv_usec = 0;

		int s = ::select(fdmax + 1, &fds, nullptr, nullptr, &tv);
		if (s < 0) {
			::perror("select");
			break;
		}
		if (s == 0)	{
			// Timeout!
			continue;
		}

		// A Network event
		for (i = 0; i < m_nports; ++i) {
			if (FD_ISSET(m_sockin[i], &fds)) {
				// Read from network...
				unsigned char buf[1024];
				struct sockaddr_in sender;
				socklen_t slen = sizeof(sender);
				int r = ::recvfrom(m_sockin[i], (char *) buf, sizeof(buf),
					0, (struct sockaddr *) &sender, &slen);
				if (r > 0)
					qmidinetUdpDevice::getInstance()->recvData(buf, r, i);
				else
				if (r < 0)
					::perror("recvfrom");
			}
		}
	}
}

#endif	// !CONFIG_IPV6


//----------------------------------------------------------------------------
// qmidinetUdpDevice -- Network interface device (UDP/IP).
//

qmidinetUdpDevice *qmidinetUdpDevice::g_pDevice = nullptr;

// Constructor.
qmidinetUdpDevice::qmidinetUdpDevice ( QObject *pParent )
	: QObject(pParent), m_nports(0),
		m_sockin(nullptr), m_sockout(nullptr)
	#if defined(CONFIG_IPV6)
		, m_udpport(nullptr)
	#else
		, m_addrout(nullptr)
		, m_pRecvThread(nullptr)
	#endif	// !CONFIG_IPV6
{
#if !defined(CONFIG_IPV6)
#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
	WSAStartup(MAKEWORD(1, 1), &g_wsaData);
#endif
#endif	// !CONFIG_IPV6

	g_pDevice = this;
}

// Destructor.
qmidinetUdpDevice::~qmidinetUdpDevice (void)
{
	close();

	g_pDevice = nullptr;

#if !defined(CONFIG_IPV6)
#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
	WSACleanup();
#endif
#endif	// !CONFIG_IPV6
}


// Kind of singleton reference.
qmidinetUdpDevice *qmidinetUdpDevice::getInstance (void)
{
	return g_pDevice;
}


// Device initialization method.
bool qmidinetUdpDevice::open ( const QString& sInterface,
	const QString& sUdpAddr, int iUdpPort, int iNumPorts )
{
	// Close if already open.
	close();

#if defined(CONFIG_IPV6)

	// Setup host address for udp multicast...
	m_udpaddr.setAddress(sUdpAddr);

	// Check whether is real for udp multicast...
	if (!m_udpaddr.isMulticast()) {
		qWarning() << "open(udpaddr):" << sUdpAddr
			<< "not an udp multicast address";
		return false;
	}

	// Check whether protocol is IPv4 or IPv6...
	const bool ipv6_protocol
		= (m_udpaddr.protocol() != QAbstractSocket::IPv4Protocol);

	// Setup network interface...
	QNetworkInterface iface;
	if (!sInterface.isEmpty())
		iface = QNetworkInterface::interfaceFromName(sInterface);

	// Set the number of ports.
	m_nports = iNumPorts;

	// Allocate sockets and addresses...
	int i;

	m_sockin  = new QUdpSocket * [m_nports];
	m_sockout = new QUdpSocket * [m_nports];

	m_udpport = new int [m_nports];

	for (i = 0; i < m_nports; ++i) {
		m_sockin[i]  = new QUdpSocket();
		m_sockout[i] = new QUdpSocket();
		m_udpport[i] = iUdpPort + i;
	}

	// Setup sockets and addreses...
	//
	for (i = 0; i < m_nports; ++i) {
		// Bind input socket...
		if (!m_sockin[i]->bind(ipv6_protocol
				? QHostAddress::AnyIPv6
				: QHostAddress::AnyIPv4,
				iUdpPort + i, QUdpSocket::ShareAddress)) {
			qWarning() << "open(sockin):" << i
				<< "udp socket error"
				<< m_sockin[i]->error()
				<< m_sockin[i]->errorString();
			return false;
		}
	#if defined(Q_OS_WIN)
		m_sockin[i]->setSocketOption(
			QAbstractSocket::MulticastLoopbackOption, 0);
	#endif
		bool joined = false;
		if (iface.isValid())
			joined = m_sockin[i]->joinMulticastGroup(m_udpaddr, iface);
		else
			joined = m_sockin[i]->joinMulticastGroup(m_udpaddr);
		if (!joined) {
			qWarning() << "open(sockin):" << i
				<< "udp socket error"
				<< m_sockin[i]->error()
				<< m_sockin[i]->errorString();
		}
		QObject::connect(m_sockin[i],
			SIGNAL(readyRead()),
			SLOT(readPendingDatagrams()));
		// Bind output socket...
		if (!m_sockout[i]->bind(ipv6_protocol
				? QHostAddress::AnyIPv6
				: QHostAddress::AnyIPv4,
				m_sockout[i]->localPort())) {
			qWarning() << "open(sockout):" << i
				<< "udp socket error"
				<< m_sockout[i]->error()
				<< m_sockout[i]->errorString();
			return false;
		}
		m_sockout[i]->setSocketOption(
			QAbstractSocket::MulticastTtlOption, 1);
	#if defined(Q_OS_UNIX)
		m_sockout[i]->setSocketOption(
			QAbstractSocket::MulticastLoopbackOption, 0);
	#endif
		if (iface.isValid())
			m_sockout[i]->setMulticastInterface(iface);
	}

#else

	// Setup network protocol...
	int i, protonum = 0;
#if 0
	struct protoent *proto = ::getprotobyname("IP");
	if (proto)
		protonum = proto->p_proto;
#endif

	// Stable interface name...
	const char *ifname = nullptr;
	const QByteArray aInterface = sInterface.toLocal8Bit();
	if (!aInterface.isEmpty())
		ifname = aInterface.constData();

	const char *udp_addr = nullptr;
	const QByteArray aUdpAddr = sUdpAddr.toLocal8Bit();
	if (!aUdpAddr.isEmpty())
		udp_addr = aUdpAddr.constData();

	// Set the number of ports.
	m_nports = iNumPorts;

	// Input socket stuff...
	//
	m_sockin = new int [m_nports];
	for (i = 0; i < m_nports; ++i)
		m_sockin[i] = -1;

	for (i = 0; i < m_nports; ++i) {

		m_sockin[i] = ::socket(PF_INET, SOCK_DGRAM, protonum);
		if (m_sockin[i] < 0) {
			::perror("socket(in)");
			return false;
		}

		struct sockaddr_in addrin;
		::memset(&addrin, 0, sizeof(addrin));
		addrin.sin_family = AF_INET;
		addrin.sin_addr.s_addr = htonl(INADDR_ANY);
		addrin.sin_port = htons(iUdpPort + i);

		if (::bind(m_sockin[i], (struct sockaddr *) (&addrin), sizeof(addrin)) < 0) {
			::perror("bind");
			return false;
		}

		// Will Hall, 2007
		// INADDR_ANY will bind to default interface,
		// specify alternate interface nameon which to bind...
		struct in_addr if_addr_in;
	#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
		if_addr_in.s_addr = htonl(INADDR_ANY);
	#else
		if (ifname) {
			if (!get_address(m_sockin[i], &if_addr_in, ifname)) {
				fprintf(stderr, "socket(in): could not find interface address for %s\n", ifname);
				return false;
			}
			if (::setsockopt(m_sockin[i], IPPROTO_IP, IP_MULTICAST_IF,
					(char *) &if_addr_in, sizeof(if_addr_in))) {
				::perror("setsockopt(IP_MULTICAST_IF)");
				return false;
			}
		} else {
			if_addr_in.s_addr = htonl(INADDR_ANY);
		}
	#endif

		struct ip_mreq mreq;
		mreq.imr_multiaddr.s_addr = ::inet_addr(udp_addr);
		mreq.imr_interface.s_addr = if_addr_in.s_addr;
		if(::setsockopt (m_sockin[i], IPPROTO_IP, IP_ADD_MEMBERSHIP,
				(char *) &mreq, sizeof(mreq)) < 0) {
			::perror("setsockopt(IP_ADD_MEMBERSHIP)");
			fprintf(stderr, "socket(in): your kernel is probably missing multicast support.\n");
			return false;
		}

	#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
		unsigned long mode = 1;
		if (::ioctlsocket(m_sockin[i], FIONBIO, &mode)) {
			::perror("ioctlsocket(O_NONBLOCK)");
			return false;
		}
	#else
		if (::fcntl(m_sockin[i], F_SETFL, O_NONBLOCK))
			::perror("fcntl(O_NONBLOCK)");
	#endif
	}

	// Output socket...
	//
	m_sockout = new int [m_nports];
	m_addrout = new struct sockaddr_in [m_nports];

	for (i = 0; i < m_nports; ++i)
		m_sockout[i] = -1;

	for (i = 0; i < m_nports; ++i) {

		m_sockout[i] = ::socket(AF_INET, SOCK_DGRAM, protonum);
		if (m_sockout[i] < 0) {
			::perror("socket(out)");
			return false;
		}

		// Will Hall, Oct 2007
	#if !defined(__WIN32__) && !defined(_WIN32) && !defined(WIN32)
		if (ifname) {
			struct in_addr if_addr_out;
			if (!get_address(m_sockout[i], &if_addr_out, ifname)) {
				::fprintf(stderr, "socket(out): could not find interface address for %s\n", ifname);
				return false;
			}
			if (::setsockopt(m_sockout[i], IPPROTO_IP, IP_MULTICAST_IF,
					(char *) &if_addr_out, sizeof(if_addr_out))) {
				::perror("setsockopt(IP_MULTICAST_IF)");
				return false;
			}
		}
	#endif

		::memset(&m_addrout[i], 0, sizeof(struct sockaddr_in));
		m_addrout[i].sin_family = AF_INET;
		m_addrout[i].sin_addr.s_addr = ::inet_addr(udp_addr);
		m_addrout[i].sin_port = htons(iUdpPort + i);

		// Turn off loopback...
		int loop = 0;
	#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
		// NOTE: The Winsock version of the IP_MULTICAST_LOOP option
		// is the semantically reverse than the UNIX version.
		const int sock = m_sockin[i];
	#else
		const int sock = m_sockout[i];
	#endif
		if (::setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP,
				(char *) &loop, sizeof (loop)) < 0) {
			::perror("setsockopt(IP_MULTICAST_LOOP)");
			return false;
		}

	#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
		unsigned long mode = 1;
		if (::ioctlsocket(m_sockout[i], FIONBIO, &mode)) {
			::perror("ioctlsocket(O_NONBLOCK)");
			return false;
		}
	#else
		if (::fcntl(m_sockout[i], F_SETFL, O_NONBLOCK)) {
			::perror("fcntl(O_NONBLOCK)");
			return false;
		}
	#endif
	}

	// Start listener thread...
	m_pRecvThread = new qmidinetUdpDeviceThread(m_sockin, m_nports);
	m_pRecvThread->start();

#endif	// !CONFIG_IPV6

	// Done.
	return true;
}


// Device termination method.
void qmidinetUdpDevice::close (void)
{
#if defined(CONFIG_IPV6)

	if (m_sockin) {
		for (int i = 0; i < m_nports; ++i) {
			if (m_sockin[i])
				delete m_sockin[i];
		}
		delete [] m_sockin;
		m_sockin = nullptr;
	}

	if (m_sockout) {
		for (int i = 0; i < m_nports; ++i) {
			if (m_sockout[i])
				delete m_sockout[i];
		}
		delete [] m_sockout;
		m_sockout = nullptr;
	}

	m_udpaddr.clear();

	if (m_udpport) {
		delete [] m_udpport;
		m_udpport = nullptr;
	}

#else

	if (m_sockin) {
		for (int i = 0; i < m_nports; ++i) {
			if (m_sockin[i] >= 0)
				::closesocket(m_sockin[i]);
		}
		delete [] m_sockin;
		m_sockin = nullptr;
	}

	if (m_sockout) {
		for (int i = 0; i < m_nports; ++i) {
			if (m_sockout[i] >= 0)
				::closesocket(m_sockout[i]);
		}
		delete [] m_sockout;
		m_sockout = nullptr;
	}

	if (m_addrout) {
		delete [] m_addrout;
		m_addrout = nullptr;
	}

	if (m_pRecvThread) {
		if (m_pRecvThread->isRunning()) {
			m_pRecvThread->setRunState(false);
		//	m_pRecvThread->terminate();
			m_pRecvThread->wait(1200); // Timeout>1sec.
		}
		delete m_pRecvThread;
		m_pRecvThread = nullptr;
	}

#endif	// !CONFIG_IPV6

	m_nports = 0;
}


// Data transmission methods.
bool qmidinetUdpDevice::sendData (
	unsigned char *data, unsigned short len, int port ) const
{
	if (port < 0 || port >= m_nports)
		return false;

#if defined(CONFIG_IPV6)

	if (m_sockout == nullptr)
		return false;
	if (m_sockout[port] == nullptr)
		return false;

	if (!m_sockout[port]->isValid()
		|| m_sockout[port]->state() != QAbstractSocket::BoundState) {
		qWarning() << "sendData(sockout):" << port
			<< "udp socket has invalid state"
			<< m_sockout[port]->state();
		return false;
	}

	QByteArray datagram((const char *) data, len);
	if (m_sockout[port]->writeDatagram(datagram, m_udpaddr, m_udpport[port]) < len) {
		qWarning() << "sendData(sockout):" << port
			<< "udp socket error"
			<< m_sockout[port]->error() << " "
			<< m_sockout[port]->errorString();
		return false;
	}

#else

	if (m_sockout == nullptr)
		return false;
	if (m_sockout[port] < 0)
		return false;

	if (::sendto(m_sockout[port], (char *) data, len, 0,
			(struct sockaddr *) &m_addrout[port],
			sizeof(struct sockaddr_in)) < 0) {
		::perror("sendto");
		return false;
	}

#endif	// !CONFIG_IPV6

	return true;
}


void qmidinetUdpDevice::recvData (
	unsigned char *data, unsigned short len, int port )
{
	emit received(QByteArray((const char *) data, len), port);
}


// Receive data slot.
void qmidinetUdpDevice::receive ( const QByteArray& data, int port )
{
	sendData((unsigned char *) data.constData(), data.length(), port);
}


#if defined(CONFIG_IPV6)

// Process incoming datagrams.
void qmidinetUdpDevice::readPendingDatagrams (void)
{
	if (m_sockin == nullptr)
		return;

	for (int i = 0; i < m_nports; ++i) {
		while (m_sockin[i] && m_sockin[i]->hasPendingDatagrams()) {
			QByteArray datagram;
			int nread = m_sockin[i]->pendingDatagramSize();
			datagram.resize(nread);
			nread = m_sockin[i]->readDatagram(datagram.data(), datagram.size());
			if (nread > 0) {
				datagram.resize(nread);
				emit received(datagram, i);
			}
		}
	}
}

#else

// Get interface address from supplied name.
bool qmidinetUdpDevice::get_address (
	int sock, struct in_addr *inaddr, const char *ifname )
{
#if !defined(__WIN32__) && !defined(_WIN32) && !defined(WIN32)

	struct ifreq ifr;
	::strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);

	if (::ioctl(sock, SIOCGIFFLAGS, (char *) &ifr)) {
		::perror("ioctl(SIOCGIFFLAGS)");
		return false;
	}

	if ((ifr.ifr_flags & IFF_UP) == 0) {
		fprintf(stderr, "interface %s is down\n", ifname);
		return false;
	}

	if (::ioctl(sock, SIOCGIFADDR, (char *) &ifr)) {
		::perror("ioctl(SIOCGIFADDR)");
		return false;
	}

	struct sockaddr_in sa;
	::memcpy(&sa, &ifr.ifr_addr, sizeof(struct sockaddr_in));
	inaddr->s_addr = sa.sin_addr.s_addr;

	return true;

#else

	return false;

#endif	// !WIN32
}

#endif	// !CONFIG_IPV6


// end of qmidinetUdpDevice.h
