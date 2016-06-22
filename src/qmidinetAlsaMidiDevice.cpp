// qmidinetAlsaMidiDevice.cpp
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

#include "qmidinetAlsaMidiDevice.h"

#ifdef CONFIG_ALSA_MIDI

#include <QThread>

//----------------------------------------------------------------------------
// qmidinetAlsaMidiThread -- ALSA MIDI listener thread.
//

class qmidinetAlsaMidiThread : public QThread
{
public:

	// Constructor.
	qmidinetAlsaMidiThread(snd_seq_t *pAlsaSeq);

	// Run-state accessors.
	void setRunState(bool bRunState);
	bool runState() const;

protected:

	// The main thread executive.
	void run();

private:

	// The listener socket.
	snd_seq_t *m_pAlsaSeq;

	// Whether the thread is logically running.
	volatile bool m_bRunState;
};


// Constructor.
qmidinetAlsaMidiThread::qmidinetAlsaMidiThread ( snd_seq_t *pAlsaSeq )
	: QThread(), m_pAlsaSeq(pAlsaSeq), m_bRunState(false)
{
}


// Run-state accessors.
void qmidinetAlsaMidiThread::setRunState ( bool bRunState )
{
	m_bRunState = bRunState;
}

bool qmidinetAlsaMidiThread::runState (void) const
{
	return m_bRunState;
}


// The main thread executive.
void qmidinetAlsaMidiThread::run (void)
{
	int nfds;
	struct pollfd *pfds;

	nfds = snd_seq_poll_descriptors_count(m_pAlsaSeq, POLLIN);
	pfds = (struct pollfd *) alloca(nfds * sizeof(struct pollfd));
	snd_seq_poll_descriptors(m_pAlsaSeq, pfds, nfds, POLLIN);

	m_bRunState = true;
	int iPoll = 0;

	while (m_bRunState && iPoll >= 0) {
		// Wait for events...
		iPoll = poll(pfds, nfds, 1000);
		while (iPoll > 0) {
			snd_seq_event_t *pEv = NULL;
			snd_seq_event_input(m_pAlsaSeq, &pEv);
			// Process input event - ...
			// - enqueue to input track mapping;
			qmidinetAlsaMidiDevice::getInstance()->capture(pEv);
		//	snd_seq_free_event(pEv);
			iPoll = snd_seq_event_input_pending(m_pAlsaSeq, 0);
		}
	}
}


//----------------------------------------------------------------------------
// qmidinetAlsaMidiDevice -- MIDI interface device (ALSA).
//

qmidinetAlsaMidiDevice *qmidinetAlsaMidiDevice::g_pDevice = NULL;

// Constructor.
qmidinetAlsaMidiDevice::qmidinetAlsaMidiDevice ( QObject *pParent )
	: QObject(pParent), m_pAlsaSeq(NULL), m_iAlsaClient(-1), m_piAlsaPort(NULL),
		m_ppAlsaEncoder(NULL), m_pAlsaDecoder(NULL), m_pRecvThread(NULL)
{
	g_pDevice = this;
}


// Destructor.
qmidinetAlsaMidiDevice::~qmidinetAlsaMidiDevice (void)
{
	close();

	g_pDevice = NULL;
}


// Kind of singleton reference.
qmidinetAlsaMidiDevice *qmidinetAlsaMidiDevice::getInstance (void)
{
	return g_pDevice;
}


// Device initialization method.
bool qmidinetAlsaMidiDevice::open ( const QString& sClientName, int iNumPorts )
{
	// Close if already open.
	close();

	// Open new ALSA sequencer client...
	if (snd_seq_open(&m_pAlsaSeq, "hw", SND_SEQ_OPEN_DUPLEX, 0) < 0)
		return false;

	int i;

	// Set client identification...
	const QByteArray aClientName = sClientName.toLocal8Bit();
	snd_seq_set_client_name(m_pAlsaSeq, aClientName.constData());
	m_iAlsaClient = snd_seq_client_id(m_pAlsaSeq);

	m_nports = iNumPorts;

	// Create duplex ports.
	m_piAlsaPort = new int [m_nports];

	for (i = 0; i < m_nports; ++i)
		m_piAlsaPort[i] = -1;

	const QString sPortName("port %1");
	for (i = 0; i < m_nports; ++i) {
		const QByteArray aPortName = sPortName.arg(i).toLocal8Bit();
		int port = snd_seq_create_simple_port(
			m_pAlsaSeq,	aPortName.constData(),
			SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE |
			SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
			SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
		if (port < 0) {
			fprintf(stderr, "snd_seq_create_simple_port: %s\n", snd_strerror(port));
			return false;
		}
		m_piAlsaPort[i] = port;
	}

	// Create MIDI (output) encoders.
	m_ppAlsaEncoder = new snd_midi_event_t * [m_nports];

	for (i = 0; i < m_nports; ++i)
		m_ppAlsaEncoder[i] = NULL;

	for (i = 0; i < m_nports; ++i) {
		long err = snd_midi_event_new(1024, &m_ppAlsaEncoder[i]);
		if (err < 0) {
			fprintf(stderr, "snd_midi_event_new: %s\n", snd_strerror(err));
			return false;
		}
	}

	// Create MIDI (input) decoders.
	long err = snd_midi_event_new(1024, &m_pAlsaDecoder);
	if (err < 0) {
		fprintf(stderr, "snd_midi_event_new: %s\n", snd_strerror(err));
		return false;
	}

	// Start listener thread...
	m_pRecvThread = new qmidinetAlsaMidiThread(m_pAlsaSeq);
	m_pRecvThread->start();

	// Done.
	return true;
}


// Device termination method.
void qmidinetAlsaMidiDevice::close (void)
{
	if (m_pRecvThread) {
		if (m_pRecvThread->isRunning()) do {
			m_pRecvThread->setRunState(false);
		//	m_pRecvThread->terminate();
		} while	(!m_pRecvThread->wait(200));
		delete m_pRecvThread;
		m_pRecvThread = NULL;
	}

	if (m_pAlsaDecoder) {
		snd_midi_event_free(m_pAlsaDecoder);
		m_pAlsaDecoder = NULL;
	}

	if (m_ppAlsaEncoder) {
		for (int i = 0; i < m_nports; ++i) {
			if (m_ppAlsaEncoder[i])
				snd_midi_event_free(m_ppAlsaEncoder[i]);
		}
		delete [] m_ppAlsaEncoder;
		m_ppAlsaEncoder = NULL;
	}

	if (m_piAlsaPort) {
		for (int i = 0; i < m_nports; ++i) {
			if (m_piAlsaPort[i] >= 0)
				snd_seq_delete_simple_port(m_pAlsaSeq, m_piAlsaPort[i]);
		}
		delete [] m_piAlsaPort;
		m_piAlsaPort = NULL;
	}

	if (m_pAlsaSeq) {
		snd_seq_close(m_pAlsaSeq);
		m_iAlsaClient = -1;
		m_pAlsaSeq = NULL;
	}

	m_nports = 0;
}


// MIDI event capture method.
void qmidinetAlsaMidiDevice::capture ( snd_seq_event_t *pEv )
{
	if (pEv == NULL)
		return;

	// Ignore some events -- these are all ALSA internal
	// events, which don't produce any MIDI bytes...
	switch(pEv->type) {
	case SND_SEQ_EVENT_OSS:
	case SND_SEQ_EVENT_CLIENT_START:
	case SND_SEQ_EVENT_CLIENT_EXIT:
	case SND_SEQ_EVENT_CLIENT_CHANGE:
	case SND_SEQ_EVENT_PORT_START:
	case SND_SEQ_EVENT_PORT_EXIT:
	case SND_SEQ_EVENT_PORT_CHANGE:
	case SND_SEQ_EVENT_PORT_SUBSCRIBED:
	case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
	case SND_SEQ_EVENT_USR0:
	case SND_SEQ_EVENT_USR1:
	case SND_SEQ_EVENT_USR2:
	case SND_SEQ_EVENT_USR3:
	case SND_SEQ_EVENT_USR4:
	case SND_SEQ_EVENT_USR5:
	case SND_SEQ_EVENT_USR6:
	case SND_SEQ_EVENT_USR7:
	case SND_SEQ_EVENT_USR8:
	case SND_SEQ_EVENT_USR9:
	case SND_SEQ_EVENT_BOUNCE:
	case SND_SEQ_EVENT_USR_VAR0:
	case SND_SEQ_EVENT_USR_VAR1:
	case SND_SEQ_EVENT_USR_VAR2:
	case SND_SEQ_EVENT_USR_VAR3:
	case SND_SEQ_EVENT_USR_VAR4:
	case SND_SEQ_EVENT_NONE:
		return;
	}

#ifdef CONFIG_DEBUG
	// - show (input) event for debug purposes...
	fprintf(stderr, "ALSA MIDI In Port %d: 0x%02x", pEv->dest.port, pEv->type);
	if (pEv->type == SND_SEQ_EVENT_SYSEX) {
		fprintf(stderr, " SysEx {");
		unsigned char *data = (unsigned char *) pEv->data.ext.ptr;
		for (unsigned int i = 0; i < pEv->data.ext.len; ++i)
			fprintf(stderr, " %02x", data[i]);
		fprintf(stderr, " }\n");
	} else {
		for (unsigned int i = 0; i < sizeof(pEv->data.raw8.d); ++i)
			fprintf(stderr, " %3d", pEv->data.raw8.d[i]);
		fprintf(stderr, "\n");
	}
#endif

	if (pEv->type == SND_SEQ_EVENT_SYSEX) {
		unsigned char *data = (unsigned char *) pEv->data.ext.ptr;
		recvData(data, pEv->data.ext.len, pEv->dest.port);
	} else {
		// Decode ALSA event into raw bytes...
		unsigned char data[1024];
		long n = snd_midi_event_decode(m_pAlsaDecoder, data, sizeof(data), pEv);
		if (n > 0)
			recvData(data, n, pEv->dest.port);
		else
		if (n < 0)
			fprintf(stderr, "snd_midi_event_decode: %s\n", snd_strerror(n));
		snd_midi_event_reset_decode(m_pAlsaDecoder);
	}
}


// Data transmission methods.
bool qmidinetAlsaMidiDevice::sendData (
	unsigned char *data, unsigned short len, int port ) const
{
	if (port < 0 || port >= m_nports)
		return false;

	snd_seq_event_t ev;
	unsigned char *d = data;
	long l = len;
	while (l > 0) {
		snd_seq_event_t *pEv = &ev;
		snd_seq_ev_clear(pEv);
		snd_seq_ev_set_source(pEv, m_piAlsaPort[port]);
		snd_seq_ev_set_subs(pEv);
		snd_seq_ev_set_direct(pEv);
		long n = snd_midi_event_encode(m_ppAlsaEncoder[port], d, l, pEv);
		if (n < 0) {
			fprintf(stderr, "snd_midi_event_encode: %s\n", snd_strerror(n));
			return false;
		}
		else
		if (n > 0) {
		#ifdef CONFIG_DEBUG
			// - show (output) event for debug purposes...
			fprintf(stderr, "ALSA MIDI Out Port %d: 0x%02x", pEv->source.port, pEv->type);
			if (pEv->type == SND_SEQ_EVENT_SYSEX) {
				fprintf(stderr, " SysEx {");
				unsigned char *data = (unsigned char *) pEv->data.ext.ptr;
				for (unsigned int i = 0; i < pEv->data.ext.len; i++)
					fprintf(stderr, " %02x", data[i]);
				fprintf(stderr, " }\n");
			} else {
				for (unsigned int i = 0; i < sizeof(pEv->data.raw8.d); i++)
					fprintf(stderr, " %3d", pEv->data.raw8.d[i]);
				fprintf(stderr, "\n");
			}
		#endif
			snd_seq_event_output(m_pAlsaSeq, pEv);
			l -= n;
			d += n;
		}
		else break;
	}

	snd_seq_drain_output(m_pAlsaSeq);
	return true;
}


void qmidinetAlsaMidiDevice::recvData (
	unsigned char *data, unsigned short len, int port )
{
	emit received(QByteArray((const char *) data, len), port);
}


// Receive data slot.
void qmidinetAlsaMidiDevice::receive ( const QByteArray& data, int port )
{
	sendData((unsigned char *) data.constData(), data.length(), port);
}


#endif	// CONFIG_ALSA_MIDI

// end of qmidinetAlsaMidiDevice.h
