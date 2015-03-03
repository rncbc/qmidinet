// qmidinetUdpDevice.cpp
//
/****************************************************************************
   Copyright (C) 2010-2015, rncbc aka Rui Nuno Capela. All rights reserved.

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
#include "qmidinetUdpDevice.h"

#include <stdlib.h>
#include <string.h>

#if defined(WIN32)
static WSADATA g_wsaData;
typedef int socklen_t;
#else
#include <unistd.h>
#include <sys/ioctl.h>
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

		int s = ::select(fdmax + 1, &fds, NULL, NULL, &tv);
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


//----------------------------------------------------------------------------
// qmidinetUdpDevice -- Network interface device (UDP/IP).
//

qmidinetUdpDevice *qmidinetUdpDevice::g_pDevice = NULL;

// Constructor.
qmidinetUdpDevice::qmidinetUdpDevice ( QObject *pParent )
	: QObject(pParent), m_nports(0),
		m_sockin(NULL), m_sockout(NULL), m_addrout(NULL), m_pRecvThread(NULL)
{
#if defined(WIN32)
	WSAStartup(MAKEWORD(1, 1), &g_wsaData);
#endif

	g_pDevice = this;
}

// Destructor.
qmidinetUdpDevice::~qmidinetUdpDevice (void)
{
	close();

	g_pDevice = NULL;

#if defined(WIN32)
	WSACleanup();
#endif
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

	// Setup network protocol...
	int i, protonum = 0;
#if 0
	struct protoent *proto = ::getprotobyname("IP");
	if (proto)
		protonum = proto->p_proto;
#endif

	// Stable interface name...
	const char *ifname = NULL;
	const QByteArray aInterface = sInterface.toLocal8Bit();
	if (!aInterface.isEmpty())
		ifname = aInterface.constData();

	const char *udp_addr = NULL;
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

		struct ip_mreq mreq;
		mreq.imr_multiaddr.s_addr = ::inet_addr(udp_addr);
		mreq.imr_interface.s_addr = if_addr_in.s_addr;
		if(::setsockopt (m_sockin[i], IPPROTO_IP, IP_ADD_MEMBERSHIP,
				(char *) &mreq, sizeof(mreq)) < 0) {
			::perror("setsockopt(IP_ADD_MEMBERSHIP)");
			fprintf(stderr, "socket(in): your kernel is probably missing multicast support.\n");
			return false;
		}
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

		::memset(&m_addrout[i], 0, sizeof(struct sockaddr_in));
		m_addrout[i].sin_family = AF_INET;
		m_addrout[i].sin_addr.s_addr = ::inet_addr(udp_addr);
		m_addrout[i].sin_port = htons(iUdpPort + i);

		// Turn off loopback...
		int loop = 0;
		if (::setsockopt(m_sockout[i], IPPROTO_IP, IP_MULTICAST_LOOP,
				(char *) &loop, sizeof (loop)) < 0) {
			::perror("setsockopt(IP_MULTICAST_LOOP)");
			return false;
		}
	}

	// Start listener thread...
	m_pRecvThread = new qmidinetUdpDeviceThread(m_sockin, m_nports);
	m_pRecvThread->start();

	// Done.
	return true;
}


// Device termination method.
void qmidinetUdpDevice::close (void)
{
	if (m_sockin) {
		for (int i = 0; i < m_nports; ++i) {
			if (m_sockin[i] >= 0)
				::closesocket(m_sockin[i]);
		}
		delete [] m_sockin;
		m_sockin = NULL;
	}

	if (m_sockout) {
		for (int i = 0; i < m_nports; ++i) {
			if (m_sockout[i] >= 0)
				::closesocket(m_sockout[i]);
		}
		delete [] m_sockout;
		m_sockout = NULL;
	}

	if (m_addrout) {
		delete [] m_addrout;
		m_addrout = NULL;
	}

	if (m_pRecvThread) {
		if (m_pRecvThread->isRunning()) {
			m_pRecvThread->setRunState(false);
		//	m_pRecvThread->terminate();
			m_pRecvThread->wait(1200); // Timeout>1sec.
		}
		delete m_pRecvThread;
		m_pRecvThread = NULL;
	}

	m_nports = 0;
}


// Data transmission methods.
bool qmidinetUdpDevice::sendData (
	unsigned char *data, unsigned short len, int port ) const
{
	if (port < 0 || port >= m_nports)
		return false;

	if (m_sockout == NULL)
		return false;
	if (m_sockout[port] < 0)
		return false;

	if (::sendto(m_sockout[port], (char *) data, len, 0,
			(struct sockaddr *) &m_addrout[port],
			sizeof(struct sockaddr_in)) < 0) {
		::perror("sendto");
		return false;
	}

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


// Get interface address from supplied name.
bool qmidinetUdpDevice::get_address (
	int sock, struct in_addr *inaddr, const char *ifname )
{
#if !defined(WIN32)

	struct ifreq ifr;
	::strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

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


// end of qmidinetUdpDevice.h
