// qmidinetMidiDevice.h
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

#ifndef __qmidinetMidiDevice_h
#define __qmidinetMidiDevice_h

#include <stdio.h>
#include <stdlib.h>

#include <alsa/asoundlib.h>

#include <QObject>
#include <QString>


//----------------------------------------------------------------------------
// qmidinetMidiDevice -- MIDI interface object.

class qmidinetMidiDevice : public QObject
{
	Q_OBJECT

public:

	// Constructor.
	qmidinetMidiDevice(QObject *pParent = NULL);

	// Destructor.
	~qmidinetMidiDevice();

	// Kind of singleton reference.
	static qmidinetMidiDevice *getInstance();

	// Device initialization method.
	bool open(const QString& sClientName, int iNumPorts = 1);

	// Device termination method.
	void close();

	// MIDI event capture method.
	void capture(snd_seq_event_t *pEv);

	// Data transmission methods.
	bool sendData(unsigned char *data, unsigned short len, int port = 0) const;
	void recvData(unsigned char *data, unsigned short len, int port = 0);

signals:

	// Received data signal.
	void received(const QByteArray& data, int port);

public slots:

	// Receive data slot.
	void receive(const QByteArray& data, int port);

private:

	// Instance variables,
	int m_nports;

	// Instance variables.
	snd_seq_t *m_pAlsaSeq;
	int  m_iAlsaClient;
	int *m_piAlsaPort;

	snd_midi_event_t **m_ppAlsaEncoder;
	snd_midi_event_t  *m_pAlsaDecoder;

	// Network receiver thread.
	class qmidinetMidiDeviceThread *m_pRecvThread;

	// Kind-of singleton reference.
	static qmidinetMidiDevice *g_pDevice;
};


#endif	// __qmidinetMidiDevice_h


// end of qmidinetMidiDevice.h
