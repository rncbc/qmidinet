// qmidinetJackMidiDevice.h
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

#ifndef __qmidinetJackMidiDevice_h
#define __qmidinetJackMidiDevice_h

#include "qmidinetAbout.h"

#ifdef CONFIG_JACK_MIDI

#include <stdio.h>
#include <stdlib.h>

#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>

#include <QObject>
#include <QString>


//----------------------------------------------------------------------------
// qmidinetJackMidiDevice -- JACK MIDI interface object.

class qmidinetJackMidiDevice : public QObject
{
	Q_OBJECT

public:

	// Constructor.
	qmidinetJackMidiDevice(QObject *pParent = NULL);

	// Destructor.
	~qmidinetJackMidiDevice();

	// Kind of singleton reference.
	static qmidinetJackMidiDevice *getInstance();

	// Device initialization method.
	bool open(const QString& sClientName, int iNumPorts = 1);

	// Device termination method.
	void close();

	// MIDI events capture method.
	void capture();

	// Data transmission methods.
	bool sendData(unsigned char *data, unsigned short len, int port = 0) const;
	void recvData(unsigned char *data, unsigned short len, int port = 0);

	// JACK specifics.
	int process (jack_nframes_t nframes);

	void shutdownNotify();

signals:

	// Received data signal.
	void received(const QByteArray& data, int port);

	// Shutdown signal.
	void shutdown();
	
public slots:

	// Receive data slot.
	void receive(const QByteArray& data, int port);

private:

	// Instance variables,
	int m_nports;

	// Instance variables.
	jack_client_t *m_pJackClient;

	jack_port_t **m_ppJackPortIn;
	jack_port_t **m_ppJackPortOut;

	jack_ringbuffer_t *m_pJackBufferIn;
	jack_ringbuffer_t *m_pJackBufferOut;

	jack_nframes_t m_last_frame_time;
	
	// Network receiver thread.
	class qmidinetJackMidiThread *m_pRecvThread;

	// Kind-of singleton reference.
	static qmidinetJackMidiDevice *g_pDevice;
};


#endif	// CONFIG_JACK_MIDI

#endif	// __qmidinetJackMidiDevice_h

// end of qmidinetJackMidiDevice.h
