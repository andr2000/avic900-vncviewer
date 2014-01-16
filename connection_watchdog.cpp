#include "config_storage.h"
#include "connection_watchdog.h"
#include "thread_factory.h"
#include "client.h"

ConnectionWatchdog::ConnectionWatchdog(Client *client) {
	m_Client = client;
	m_Thread = ThreadFactory::GetNewThread();
	m_ConfigStorage = ConfigStorage::GetInstance();
	m_MaxTryCounter = 0;
}

ConnectionWatchdog::~ConnectionWatchdog() {
	ShutDown();
}

int ConnectionWatchdog::Start() {
	m_MaxTryCounter = 0;
	m_ServerIP = m_Client->GetServerIP();
	/* start worker thread */
	if (NULL == m_Thread) {
		/* m_Thread will be non-null if we try to reconnect  */
		m_Thread = ThreadFactory::GetNewThread();
		m_Thread->SetWorker(PollConnectionCb, static_cast<void *>(this));
	}
	return m_Thread->Start();
}

void ConnectionWatchdog::ShutDown() {
	if (m_Thread) {
		m_Thread->Terminate();
		delete m_Thread;
		m_Thread = NULL;
	}
}

int ConnectionWatchdog::PollConnectionCb(void *data) {
	ConnectionWatchdog *context = reinterpret_cast<ConnectionWatchdog *>(data);
	return context->PollConnection();
}

int ConnectionWatchdog::PollConnection() {
	if (!m_Client->IsServerAlive(m_ServerIP)) {
		m_MaxTryCounter++;
		if (m_MaxTryCounter > MAX_TRY) {
			Client::event_t evt;

			evt.what = Client::EVT_CONNECTION_LOST;
			m_Client->PostEvent(evt);
			return -1;
		}
	} else {
		m_MaxTryCounter = 0;
	}
	m_Thread->SleepMs(1000);
	return 0;
}
