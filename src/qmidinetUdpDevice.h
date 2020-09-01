// qmidinetUdpDevice.h
//
/****************************************************************************
   Copyright (C) 2010-2020, rncbc aka Rui Nuno Capela. All rights reserved.

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

#ifndef __qmidinetUdpDevice_h
#define __qmidinetUdpDevice_h

#include "qmidinetAbout.h"

#include <stdio.h>

#if !defined(CONFIG_IPV6)
#if defined(__WIN32__) || defined(_WIN32) || defined(WIN32)
#include <winsock.h>
#else
#include <arpa/inet.h>
#include <net/if.h>
#endif
#endif	// !CONFIG_IPV6

#include <QObject>
#include <QString>

#if defined(CONFIG_IPV6)
#include <QUdpSocket>
#include <QHostAddress>
#endif


//----------------------------------------------------------------------------
// qmidinetUdpDevice -- Network interface device (UDP/IP).

class qmidinetUdpDevice : public QObject
{
	Q_OBJECT

public:

	// Constructor.
	qmidinetUdpDevice(QObject *pParent = nullptr);

	// Destructor.
	~qmidinetUdpDevice();

	// Kind of singleton reference.
	static qmidinetUdpDevice *getInstance();

	// Device initialization method.
	bool open(const QString& sInterface,
		const QString& sUdpAddr, int iUdpPort, int iNumPorts = 1);

	// Device termination method.
	void close();

	// Data transmission methods.
	bool sendData(unsigned char *data, unsigned short len, int port = 0) const;
	void recvData(unsigned char *data, unsigned short len, int port = 0);

signals:

	// Received data signal.
	void received(const QByteArray& data, int port);

public slots:

	// Receive data slot.
	void receive(const QByteArray& data, int port);

#if defined(CONFIG_IPV6)
protected slots:

	// Process incoming datagrams.
	void readPendingDatagrams();

#else
protected:

	// Get interface address from supplied name.
	static bool get_address(int sock, struct in_addr *iaddr, const char *ifname);

#endif	// !CONFIG_IPV6

private:

	// Instance variables,
	int  m_nports;

#if defined(CONFIG_IPV6)

	QUdpSocket **m_sockin;
	QUdpSocket **m_sockout;

	QHostAddress m_udpaddr;

	int *m_udpport;

#else

	int *m_sockin;
	int *m_sockout;

	struct sockaddr_in *m_addrout;

	// Network receiver thread.
	class qmidinetUdpDeviceThread *m_pRecvThread;

#endif	// !CONFIG_IPV6

	// Kind-of singleton reference.
	static qmidinetUdpDevice *g_pDevice;
};


#endif	// __qmidinetUdpDevice_h


// end of qmidinetUdpDevice.h
