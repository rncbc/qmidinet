// qmidinetJackMidiDevice.cpp
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

#include "qmidinetJackMidiDevice.h"

#ifdef CONFIG_JACK_MIDI

#include <QThread>
#include <QMutex>
#include <QWaitCondition>


// JACK MIDI event, plus the port its destined for...
struct qmidinetJackMidiEvent
{
	jack_midi_event_t event;
	int port;
};


//----------------------------------------------------------------------
// qmidinetJackMidiDevice_process -- JACK client process callback.
//

static int qmidinetJackMidiDevice_process ( jack_nframes_t nframes, void *pvArg )
{
	qmidinetJackMidiDevice *pJackMidiDevice
		= static_cast<qmidinetJackMidiDevice *> (pvArg);

	return pJackMidiDevice->process(nframes);
}


//----------------------------------------------------------------------
// qmidinetJackMidiDevice_shutdown -- JACK client shutdown callback.
//

static void qmidinetJackMidiDevice_shutdown ( void *pvArg )
{
	qmidinetJackMidiDevice *pJackMidiDevice
		= static_cast<qmidinetJackMidiDevice *> (pvArg);

	pJackMidiDevice->shutdown();
}


//----------------------------------------------------------------------------
// qmidinetJackMidiThread -- JACK MIDI transfer thread.
//

class qmidinetJackMidiThread : public QThread
{
public:

	// Constructor.
	qmidinetJackMidiThread();

	// Run-state accessors.
	void setRunState(bool bRunState);
	bool runState() const;

	// Wake from executive wait condition (RT-safe).
	void sync();

protected:

	// The main thread executive.
	void run();

private:

	// Whether the thread is logically running.
	volatile bool m_bRunState;

	// Thread synchronization objects.
	QMutex m_mutex;
	QWaitCondition m_cond;
};


// Constructor.
qmidinetJackMidiThread::qmidinetJackMidiThread (void)
	: QThread(), m_bRunState(false)
{
}


// Run-state accessors.
void qmidinetJackMidiThread::setRunState ( bool bRunState )
{
	QMutexLocker locker(&m_mutex);

	m_bRunState = bRunState;
}

bool qmidinetJackMidiThread::runState (void) const
{
	return m_bRunState;
}


// The main thread executive.
void qmidinetJackMidiThread::run (void)
{
	m_mutex.lock();
	m_bRunState = true;
	while (m_bRunState) {
		// Wait for events...
		m_cond.wait(&m_mutex);
		// Process input events...
		qmidinetJackMidiDevice::getInstance()->capture();
	}
	m_mutex.unlock();
}


// Wake from executive wait condition (RT-safe).
void qmidinetJackMidiThread::sync (void)
{
	if (m_mutex.tryLock()) {
		m_cond.wakeAll();
		m_mutex.unlock();
	}
#ifdef CONFIG_DEBUG
	else qDebug("qmidinetJackMidiThread[%p]::sync(): tryLock() failed.", this);
#endif
}


//----------------------------------------------------------------------------
// qmidinetJackMidiDevice -- MIDI interface device (JACK).
//

qmidinetJackMidiDevice *qmidinetJackMidiDevice::g_pDevice = NULL;

// Constructor.
qmidinetJackMidiDevice::qmidinetJackMidiDevice ( QObject *pParent )
	: QObject(pParent), m_pJackClient(NULL),
		m_ppJackPortIn(NULL), m_ppJackPortOut(NULL),
		m_pJackBufferIn(NULL), m_pJackBufferOut(NULL),
		m_pRecvThread(NULL)
{
	g_pDevice = this;
}


// Destructor.
qmidinetJackMidiDevice::~qmidinetJackMidiDevice (void)
{
	close();

	g_pDevice = NULL;
}


// Kind of singleton reference.
qmidinetJackMidiDevice *qmidinetJackMidiDevice::getInstance (void)
{
	return g_pDevice;
}


// Device initialization method.
bool qmidinetJackMidiDevice::open ( const QString& sClientName, int iNumPorts )
{
	// Close if already open.
	close();

	// Open new JACK client...
	const QByteArray aClientName = sClientName.toLocal8Bit();
	m_pJackClient = jack_client_open(
		aClientName.constData(), JackNullOption, NULL);
	if (m_pJackClient == NULL)
		return false;

	m_nports = iNumPorts;

	int i;

	// Create duplex ports.
	m_ppJackPortIn  = new jack_port_t * [m_nports];
	m_ppJackPortOut = new jack_port_t * [m_nports];

	for (i = 0; i < m_nports; ++i) {
		m_ppJackPortIn[i] = NULL;
		m_ppJackPortOut[i] = NULL;
	}

	const QString sPortNameIn("in_%1");
	const QString sPortNameOut("out_%1");
	for (i = 0; i < m_nports; ++i) {
		m_ppJackPortIn[i] = jack_port_register(m_pJackClient,
			sPortNameIn.arg(i + 1).toLocal8Bit().constData(),
			JACK_DEFAULT_MIDI_TYPE,
			JackPortIsInput, 0);
		m_ppJackPortOut[i] = jack_port_register(m_pJackClient,
			sPortNameOut.arg(i + 1).toLocal8Bit().constData(),
			JACK_DEFAULT_MIDI_TYPE,
			JackPortIsOutput, 0);
	}

	// Create transient buffers.
	m_pJackBufferIn = jack_ringbuffer_create(
		1024 * m_nports * sizeof(qmidinetJackMidiEvent));
	m_pJackBufferOut = jack_ringbuffer_create(
		1024 * m_nports * sizeof(qmidinetJackMidiEvent));

	// Set and go usual callbacks...
	jack_set_process_callback(m_pJackClient,
		qmidinetJackMidiDevice_process, this);
	jack_on_shutdown(m_pJackClient,
		qmidinetJackMidiDevice_shutdown, this);

	jack_activate(m_pJackClient);

	// Start listener thread...
	m_pRecvThread = new qmidinetJackMidiThread();
	m_pRecvThread->start();

	// Done.
	return true;
}


// Device termination method.
void qmidinetJackMidiDevice::close (void)
{
	if (m_pRecvThread) {
		if (m_pRecvThread->isRunning()) do {
			m_pRecvThread->setRunState(false);
		//	m_pRecvThread->terminate();
			m_pRecvThread->sync();
		} while (!m_pRecvThread->wait(100));
		delete m_pRecvThread;
		m_pRecvThread = NULL;
	}

	if (m_pJackClient)
		jack_deactivate(m_pJackClient);

	if (m_ppJackPortIn || m_ppJackPortOut) {
		for (int i = 0; i < m_nports; ++i) {
			if (m_ppJackPortIn && m_ppJackPortIn[i])
				jack_port_unregister(m_pJackClient, m_ppJackPortIn[i]);
			if (m_ppJackPortOut && m_ppJackPortOut[i])
				jack_port_unregister(m_pJackClient, m_ppJackPortOut[i]);
		}
		if (m_ppJackPortIn)
			delete [] m_ppJackPortIn;
		if (m_ppJackPortOut)
			delete [] m_ppJackPortOut;
		m_ppJackPortIn = NULL;
		m_ppJackPortOut = NULL;
	}

	if (m_pJackClient) {
		jack_client_close(m_pJackClient);
		m_pJackClient = NULL;
	}

	if (m_pJackBufferIn) {
		jack_ringbuffer_free(m_pJackBufferIn);
		m_pJackBufferIn = NULL;
	}

	if (m_pJackBufferOut) {
		jack_ringbuffer_free(m_pJackBufferOut);
		m_pJackBufferOut = NULL;
	}

	m_nports = 0;
}


// MIDI events capture method.
void qmidinetJackMidiDevice::capture (void)
{
	if (m_pJackBufferIn == NULL)
		return;

	jack_nframes_t frame_time = jack_frame_time(m_pJackClient);

	qmidinetJackMidiEvent ev;
	while (jack_ringbuffer_peek(m_pJackBufferIn,
			(char *) &ev, sizeof(ev)) == sizeof(ev)) {
		ev.event.time += m_last_frame_time;
		if (ev.event.time < frame_time)
			break;
		jack_ringbuffer_read_advance(m_pJackBufferIn, sizeof(ev));
		char *pMidiData = new char [ev.event.size];
		jack_ringbuffer_read(m_pJackBufferIn, pMidiData, ev.event.size);
	#ifdef CONFIG_DEBUG
		// - show (input) event for debug purposes...
		fprintf(stderr, "JACK MIDI In Port %d: ", ev.port);
		for (unsigned int i = 0; i < ev.event.size; ++i)
			fprintf(stderr, " 0x%02x", pMidiData[i]);
		fprintf(stderr, "\n");
	#endif	
		recvData((unsigned char *) pMidiData, ev.event.size, ev.port);			
		delete [] pMidiData;
	}
}


// JACK specifics.
int qmidinetJackMidiDevice::process ( jack_nframes_t nframes )
{
	jack_nframes_t buffer_size = jack_get_buffer_size(m_pJackClient);

	m_last_frame_time  = jack_last_frame_time(m_pJackClient);

	// Enqueue/dequeue events
	// to/from ring-buffers...
	for (int i = 0; i < m_nports; ++i) {

		if (m_ppJackPortIn && m_ppJackPortIn[i] && m_pJackBufferIn) {
			void *pvBufferIn
				= jack_port_get_buffer(m_ppJackPortIn[i], nframes);
			jack_ringbuffer_data_t vector[2];
			jack_ringbuffer_get_write_vector(m_pJackBufferIn, vector);
			char *pchBuffer = vector[0].buf;
			qmidinetJackMidiEvent *pJackEventIn
				= (struct qmidinetJackMidiEvent *) pchBuffer;
			int nlimit = vector[0].len;
			int nevents = jack_midi_get_event_count(pvBufferIn);
			int nwrite = 0;
			for (int n = 0; n < nevents && nwrite < nlimit; ++n) {
				jack_midi_event_get(&pJackEventIn->event, pvBufferIn, n);
				pJackEventIn->port = i;
				pchBuffer += sizeof(qmidinetJackMidiEvent);
				nwrite += sizeof(qmidinetJackMidiEvent);
				memcpy(pchBuffer, pJackEventIn->event.buffer, pJackEventIn->event.size);
				pchBuffer += pJackEventIn->event.size;
				nwrite += pJackEventIn->event.size;
				pJackEventIn = (struct qmidinetJackMidiEvent *) pchBuffer;
			}
			if (nwrite > 0)
				jack_ringbuffer_write_advance(m_pJackBufferIn, nwrite);
		}
	
		if (m_ppJackPortOut && m_ppJackPortOut[i] && m_pJackBufferOut) {
			void *pvBufferOut
				= jack_port_get_buffer(m_ppJackPortOut[i], nframes);
			jack_midi_clear_buffer(pvBufferOut);
			int nlimit = jack_midi_max_event_size(pvBufferOut); 
			int nread = 0;
			qmidinetJackMidiEvent ev;
			while (jack_ringbuffer_peek(m_pJackBufferOut,
					(char *) &ev, sizeof(ev)) == sizeof(ev)
					&& nread < nlimit) {
				if (ev.event.time > m_last_frame_time)
					break;
				jack_nframes_t offset = m_last_frame_time - ev.event.time;
				if (offset > buffer_size)
					offset = 0;
				else
					offset = buffer_size - offset;
				jack_ringbuffer_read_advance(m_pJackBufferOut, sizeof(ev));
				jack_midi_data_t *pMidiData
					= jack_midi_event_reserve(pvBufferOut, offset, ev.event.size);
				if (pMidiData == NULL)
					break;
				jack_ringbuffer_read(m_pJackBufferOut,
					(char *) pMidiData, ev.event.size);
				jack_ringbuffer_read_advance(m_pJackBufferOut, ev.event.size);
				nread += ev.event.size;
			}
		}
	}

	if (m_pJackBufferIn
		&& jack_ringbuffer_read_space(m_pJackBufferIn) > 0)
		m_pRecvThread->sync();

	return 0;
}


void qmidinetJackMidiDevice::shutdown (void)
{
	m_pJackClient = NULL;

	close();
}


// Data transmission methods.
bool qmidinetJackMidiDevice::sendData (
	unsigned char *data, unsigned short len, int port ) const
{
	if (port < 0 || port >= m_nports)
		return false;

	if (m_pJackBufferOut == NULL)
		return false;

	jack_ringbuffer_data_t vector[2];
	jack_ringbuffer_get_write_vector(m_pJackBufferOut, vector);
	int nlimit = vector[0].len;
	if (nlimit < len)
		return false;

	int nwrite = 0;
	char *pchBuffer = vector[0].buf;
	qmidinetJackMidiEvent *pJackEventOut
		= (struct qmidinetJackMidiEvent *) pchBuffer;
	pchBuffer += sizeof(qmidinetJackMidiEvent);
	nwrite += sizeof(qmidinetJackMidiEvent);
	memcpy(pchBuffer, data, len);
	pJackEventOut->event.time = jack_frame_time(m_pJackClient);
	pJackEventOut->event.buffer = (jack_midi_data_t *) pchBuffer;
	pJackEventOut->event.size = len;
	pJackEventOut->port = port;
#ifdef CONFIG_DEBUG
	// - show (output) event for debug purposes...
	fprintf(stderr, "JACK MIDI Out Port %d: ", port);
	for (unsigned int i = 0; i < len; ++i)
		fprintf(stderr, " 0x%02x", pchBuffer[i]);
	fprintf(stderr, "\n");
#endif	
	pchBuffer += len;
	nwrite += len;
	jack_ringbuffer_write_advance(m_pJackBufferOut, nwrite);

	return true;
}


void qmidinetJackMidiDevice::recvData (
	unsigned char *data, unsigned short len, int port )
{
	emit received(QByteArray((const char *) data, len), port);
}


// Receive data slot.
void qmidinetJackMidiDevice::receive ( const QByteArray& data, int port )
{
	sendData((unsigned char *) data.constData(), data.length(), port);
}


#endif	// CONFIG_JACK_MIDI

// end of qmidinetJackMidiDevice.h
