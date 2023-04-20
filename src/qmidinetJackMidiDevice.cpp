// qmidinetJackMidiDevice.cpp
//
/****************************************************************************
   Copyright (C) 2010-2023, rncbc aka Rui Nuno Capela. All rights reserved.

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


//---------------------------------------------------------------------
// qmidinetJackMidiQueue - Home-brew sorter queue.
//

class qmidinetJackMidiQueue
{
public:

	// Constructor.
	qmidinetJackMidiQueue ( unsigned int size, unsigned int slack )
	{
		m_pool  = pool_create(size * (slack + sizeof(Slot)));
		m_items = new char * [size];
		m_size  = size;		
		m_count = 0;
		m_dirty = 0;
	}

	// Destructor.
	~qmidinetJackMidiQueue ()
	{
		pool_delete(m_pool);
		delete [] m_items;
	}

	// Queue cleanup.
	void clear ()
	{
		pool_clear(m_pool);
		m_count = 0;
		m_dirty = 0;
	}

	// Queue size accessor.
	int size () const
		{ return m_size; }

	// Queue count accessor.
	int count () const
		{ return m_count; }

	// Queue dirty accessor.
	bool isDirty () const
		{ return (m_dirty > 0); }

	// Queue item push insert.
	char *push ( int port, jack_nframes_t time, size_t size )
	{
		char *item = push_item(time, size + sizeof(unsigned short));
		if (item) {
			*(unsigned short *) item = port;
			item += sizeof(unsigned short);
		}
		return item;
	}

	// Queue item pop/remove.
	char *pop ( int *port, jack_nframes_t *time, size_t *size )
	{
		char *item = pop_item(time, size);
		if (item) {
			if (port) *port = *(unsigned short *) item;
			if (size) *size -= sizeof(unsigned short);
			item += sizeof(unsigned short);
		}
		return item;
	}

protected:

	struct Slot {
		unsigned int size;
		union {
			unsigned long key;
			Slot *next;
		} u;
	};

	static
	Slot *pool_slot ( char *data )
		{ return (Slot *) ((char *) data - sizeof(Slot)); }

	static
	unsigned int pool_slot_size ( char *data )
		{ return pool_slot(data)->size - sizeof(Slot); }

	static
	unsigned long pool_slot_key ( char *data )
		{ return pool_slot(data)->u.key; }

	void pool_clear ( Slot *pool )
	{
		Slot *p = (Slot *) ((char *) pool + sizeof(Slot));
		pool->u.next = p;

		p->size = pool->size - sizeof(Slot);
		p->u.next = nullptr;
	}

	Slot *pool_create ( unsigned int size )
	{
		Slot *pool = (Slot *) ::malloc(sizeof(Slot) + size);
		pool->size = size;

		pool_clear(pool);

		return pool;
	}

	void pool_delete ( Slot *pool )
		{ ::free(pool);	}

	char *pool_alloc ( Slot *pool, unsigned long key, unsigned int size )
	{
		Slot *q = pool;
		Slot *p = pool->u.next;

		size += sizeof(Slot);

		while (p && p->size < size) {
			q = p;
			p = p->u.next;
		}

		if (p == nullptr)
			return nullptr;

		Slot *pnext = p->u.next;
		unsigned int psize = p->size - size;

		if (psize < sizeof(Slot)) {
			q->u.next = pnext;
		} else {
			q->u.next = (Slot *) ((char *) p + size);
			q->u.next->size = psize;
			q->u.next->u.next = pnext;
		}

		p->size = size;
		p->u.key = key;

		return (char *) p + sizeof(Slot); 
	}

	void pool_free ( Slot *pool, char *data )
	{
		if (data == nullptr)
			return;

		Slot *p = pool_slot(data);
	//	p->size = size;
		p->u.next = pool->u.next;

		pool->u.next = p;
	}

	static
	int sort_item ( const void *elem1, const void *elem2 )
	{
		return long(pool_slot_key(*(char **) elem2))
			 - long(pool_slot_key(*(char **) elem1));
	}

	char *push_item ( jack_nframes_t time, size_t size )
	{
		char *item = nullptr;

		if (m_count < m_size) {
			item = pool_alloc(m_pool, time, size);
			if (item) {
				m_items[m_count++] = item;
				m_dirty++;
			}
		}

		return item;
	}

	char *pop_item ( jack_nframes_t *time, size_t *size )
	{
		char *item = nullptr;

		if (m_count > 0) {
			if (m_dirty > 0) {
				::qsort(m_items, m_count, sizeof(char *), sort_item);
				m_dirty = 0;
			}
			item = m_items[--m_count];
			if (time) *time = pool_slot_key(item);
			if (size) *size = pool_slot_size(item);
		}
		else pool_clear(m_pool);

		return item;
	}

private:

	// Queue instance variables.
	Slot         *m_pool;
	char        **m_items;
	unsigned int  m_size;
	unsigned int  m_count;
	int           m_dirty;
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

	pJackMidiDevice->shutdownNotify();
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

	// Sleep for some microseconds.
	void usleep(unsigned long usecs);

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


// Sleep for some microseconds.
void qmidinetJackMidiThread::usleep ( unsigned long usecs )
{
	QThread::usleep(usecs);
}


//----------------------------------------------------------------------------
// qmidinetJackMidiDevice -- MIDI interface device (JACK).
//

qmidinetJackMidiDevice *qmidinetJackMidiDevice::g_pDevice = nullptr;

// Constructor.
qmidinetJackMidiDevice::qmidinetJackMidiDevice ( QObject *pParent )
	: QObject(pParent), m_pJackClient(nullptr),
		m_ppJackPortIn(nullptr), m_ppJackPortOut(nullptr),
		m_pJackBufferIn(nullptr), m_pJackBufferOut(nullptr),
		m_pQueueIn(nullptr), m_pRecvThread(nullptr)
{
	g_pDevice = this;
}


// Destructor.
qmidinetJackMidiDevice::~qmidinetJackMidiDevice (void)
{
	close();

	g_pDevice = nullptr;
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
		aClientName.constData(), JackNullOption, nullptr);
	if (m_pJackClient == nullptr)
		return false;

	m_nports = iNumPorts;

	int i;

	// Create duplex ports.
	m_ppJackPortIn  = new jack_port_t * [m_nports];
	m_ppJackPortOut = new jack_port_t * [m_nports];

	for (i = 0; i < m_nports; ++i) {
		m_ppJackPortIn[i] = nullptr;
		m_ppJackPortOut[i] = nullptr;
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
	m_pJackBufferIn  = jack_ringbuffer_create(1024 * m_nports);
	m_pJackBufferOut = jack_ringbuffer_create(1024 * m_nports);

	// Prepare the queue sorter stuff...
	m_pQueueIn = new qmidinetJackMidiQueue(1024 * m_nports, 8);
	
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
		m_pRecvThread = nullptr;
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
		m_ppJackPortIn = nullptr;
		m_ppJackPortOut = nullptr;
	}

	if (m_pJackClient) {
		jack_client_close(m_pJackClient);
		m_pJackClient = nullptr;
	}

	if (m_pJackBufferIn) {
		jack_ringbuffer_free(m_pJackBufferIn);
		m_pJackBufferIn = nullptr;
	}

	if (m_pJackBufferOut) {
		jack_ringbuffer_free(m_pJackBufferOut);
		m_pJackBufferOut = nullptr;
	}

	if (m_pQueueIn) {
		delete m_pQueueIn;
		m_pQueueIn = nullptr;
	}

	m_nports = 0;
}


// MIDI events capture method.
void qmidinetJackMidiDevice::capture (void)
{
	if (m_pJackBufferIn == nullptr)
		return;

	char *pchBuffer;
	qmidinetJackMidiEvent ev;

	while (jack_ringbuffer_peek(m_pJackBufferIn,
			(char *) &ev, sizeof(ev)) == sizeof(ev)) {
		jack_ringbuffer_read_advance(m_pJackBufferIn, sizeof(ev));
		pchBuffer = m_pQueueIn->push(ev.port, ev.event.time, ev.event.size);
		if (pchBuffer)
			jack_ringbuffer_read(m_pJackBufferIn, pchBuffer, ev.event.size);
		else
			jack_ringbuffer_read_advance(m_pJackBufferIn, ev.event.size);
	}

	float sample_rate = jack_get_sample_rate(m_pJackClient);
	jack_nframes_t frame_time = jack_frame_time(m_pJackClient);

	while ((pchBuffer = m_pQueueIn->pop(
			&ev.port, &ev.event.time, &ev.event.size)) != nullptr) {	
		ev.event.time += m_last_frame_time;
		if (ev.event.time > frame_time) {
			unsigned long sleep_time = ev.event.time - frame_time;
			float secs = float(sleep_time) / sample_rate;
			if (secs > 0.0001f) {
			#if 0 // defined(__GNUC__) && defined(Q_OS_LINUX)
				struct timespec ts;
				ts.tv_sec  = time_t(secs);
				ts.tv_nsec = long(1E+9f * (secs - ts.tv_sec));
				::nanosleep(&ts, nullptr);
			#else
				m_pRecvThread->usleep(long(1E+6f * secs));
			#endif
			}
			frame_time = ev.event.time;
		}	
	#ifdef CONFIG_DEBUG
		// - show (input) event for debug purposes...
		fprintf(stderr, "JACK MIDI In Port %d: (%d)", ev.port, int(ev.event.size));
		for (unsigned int i = 0; i < ev.event.size; ++i)
			fprintf(stderr, " 0x%02x", (unsigned char) pchBuffer[i]);
		fprintf(stderr, "\n");
	#endif
		recvData((unsigned char *) pchBuffer, ev.event.size, ev.port);
	}
}


// JACK specifics.
int qmidinetJackMidiDevice::process ( jack_nframes_t nframes )
{
	jack_nframes_t buffer_size = jack_get_buffer_size(m_pJackClient);

	m_last_frame_time = jack_last_frame_time(m_pJackClient);

	// Enqueue/dequeue events
	// to/from ring-buffers...
	for (int i = 0; i < m_nports; ++i) {

		if (m_ppJackPortIn && m_ppJackPortIn[i] && m_pJackBufferIn) {
			void *pvBufferIn
				= jack_port_get_buffer(m_ppJackPortIn[i], nframes);
			const int nevents = jack_midi_get_event_count(pvBufferIn);
			const unsigned int nlimit
				= jack_ringbuffer_write_space(m_pJackBufferIn);
			unsigned char  achBuffer[nlimit];
			unsigned char *pchBuffer = &achBuffer[0];
			unsigned int nwrite = 0;
			for (int n = 0; n < nevents; ++n) {
				if (nwrite + sizeof(qmidinetJackMidiEvent) >= nlimit)
					break;
				qmidinetJackMidiEvent *pJackEventIn
					= (struct qmidinetJackMidiEvent *) pchBuffer;
				jack_midi_event_get(&pJackEventIn->event, pvBufferIn, n);
				if (nwrite + sizeof(qmidinetJackMidiEvent)
					+ pJackEventIn->event.size >= nlimit)
					break;
				pJackEventIn->port = i;
				pchBuffer += sizeof(qmidinetJackMidiEvent);
				nwrite += sizeof(qmidinetJackMidiEvent);
				::memcpy(pchBuffer,
					pJackEventIn->event.buffer, pJackEventIn->event.size);
				pchBuffer += pJackEventIn->event.size;
				nwrite += pJackEventIn->event.size;
			}
			if (nwrite > 0) {
				jack_ringbuffer_write(m_pJackBufferIn,
					(const char *) achBuffer, nwrite);
			}
		}
	
		if (m_ppJackPortOut && m_ppJackPortOut[i] && m_pJackBufferOut) {
			void *pvBufferOut
				= jack_port_get_buffer(m_ppJackPortOut[i], nframes);
			jack_midi_clear_buffer(pvBufferOut);
			const unsigned int nlimit
				= jack_midi_max_event_size(pvBufferOut);
			unsigned int nread = 0;
			qmidinetJackMidiEvent ev;
			while (jack_ringbuffer_peek(m_pJackBufferOut,
					(char *) &ev, sizeof(ev)) == sizeof(ev)
					&& nread < nlimit) {
				if (ev.port != i)
					break;
				if (ev.event.time >= m_last_frame_time)
					break;
				jack_nframes_t offset = m_last_frame_time - ev.event.time;
				if (offset > buffer_size)
					offset = 0;
				else
					offset = buffer_size - offset;
				jack_ringbuffer_read_advance(m_pJackBufferOut, sizeof(ev));
				jack_midi_data_t *pMidiData
					= jack_midi_event_reserve(pvBufferOut, offset, ev.event.size);
				if (pMidiData)
					jack_ringbuffer_read(m_pJackBufferOut,
						(char *) pMidiData, ev.event.size);
				else
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


void qmidinetJackMidiDevice::shutdownNotify (void)
{
	emit shutdown();
}


// Data transmission methods.
bool qmidinetJackMidiDevice::sendData (
	unsigned char *data, unsigned short len, int port ) const
{
	if (port < 0 || port >= m_nports)
		return false;

	if (m_pJackBufferOut == nullptr)
		return false;

	const unsigned int nlimit
		= jack_ringbuffer_write_space(m_pJackBufferOut);
	if (sizeof(qmidinetJackMidiEvent) + len < nlimit) {
		unsigned char  achBuffer[nlimit];
		unsigned char *pchBuffer = &achBuffer[0];
		qmidinetJackMidiEvent *pJackEventOut
			= (struct qmidinetJackMidiEvent *) pchBuffer;
		pchBuffer += sizeof(qmidinetJackMidiEvent);
		memcpy(pchBuffer, data, len);
		pJackEventOut->event.time = jack_frame_time(m_pJackClient);
		pJackEventOut->event.buffer = (jack_midi_data_t *) pchBuffer;
		pJackEventOut->event.size = len;
		pJackEventOut->port = port;
	#ifdef CONFIG_DEBUG
		// - show (output) event for debug purposes...
		fprintf(stderr, "JACK MIDI Out Port %d:", port);
		for (unsigned int i = 0; i < len; ++i)
			fprintf(stderr, " 0x%02x", (unsigned char) pchBuffer[i]);
		fprintf(stderr, "\n");
	#endif
		jack_ringbuffer_write(m_pJackBufferOut,
			(const char *) achBuffer, sizeof(qmidinetJackMidiEvent) + len);
	}

	return true;
}


void qmidinetJackMidiDevice::recvData (
	unsigned char *data, unsigned short len, int port )
{
	emit received(QByteArray((const char *) data, len), port);
}


// Receive data slot.
void qmidinetJackMidiDevice::receive ( QByteArray data, int port )
{
	sendData((unsigned char *) data.constData(), data.length(), port);
}


#endif	// CONFIG_JACK_MIDI

// end of qmidinetJackMidiDevice.h
