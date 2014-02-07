#include <rfb/rfbclient.h>
#include "client.h"
#include "config_storage.h"

Client *Client::m_Instance = NULL;

const char Client::VERSION[] = "AVIC-F900BT VNC client 0.1alpha " __DATE__;
const char Client::COPYRIGHT[] = "(c) 2014 Andrushchenko, Oleksandr andr2000@gmail.com";

Client::Client() {
	m_Private = NULL;
	m_Client = NULL;
	m_Thread = NULL;
	m_Mutex = MutexFactory::GetNewMutex();
	m_NeedsVirtInpHack = false;
	m_ConfigStorage = NULL;
	m_ScalingFactorX = 1.0f;
	m_ScalingFactorY = 1.0f;
	m_NeedScaling = false;
	m_LastRefreshTimeMs = -1L;
	m_ForceRefreshToMs = 0;
}

Client::~Client() {
	Cleanup();
}

void Client::Cleanup() {
	if (m_Thread) {
		m_Thread->Terminate();
		delete m_Thread;
		m_Thread = NULL;
	}
	if (m_Client) {
		rfbClientCleanup(m_Client);
		m_Client = NULL;
	}
	m_MessageQueue.clear();
	if (m_Mutex) {
		delete m_Mutex;
		m_Mutex = NULL;
	}
}

int Client::PollRFB(void *data) {
	Client *context = reinterpret_cast<Client *>(data);
	return context->Poll();
}

int Client::Initialize(void *_private) {
	m_ConfigStorage = ConfigStorage::GetInstance();
	/* set logging options */
	rfbEnableClientLogging = m_ConfigStorage->LoggingEnabled();
	SetLogging();
	/* do we need to hack android's virtual input event? */
	m_NeedsVirtInpHack = m_ConfigStorage->NeedsVirtualInputHack();
	/* force screen refresh? */
	m_ForceRefreshToMs = m_ConfigStorage->ForceRefreshToMs();
	rfbClientLog("Initializing VNC Client\n");
	m_Private = _private;
	/* get new RFB client */
	m_Client = rfbGetClient(5, 3, 2);
	if (NULL == m_Client) {
		return -1;
	}
	/* initialize it */
	m_Client->MallocFrameBuffer = MallocFrameBuffer;
	m_Client->canHandleNewFBSize = FALSE;
	m_Client->GotFrameBufferUpdate = GotFrameBufferUpdate;
	m_Client->listenPort = -1;
	m_Client->listen6Port = -1;

	/* TODO: for some reason even if I set RGB565 (converted
	 * with reverse shift order to BGR565) it doesn't work.
	 * So, it seems to be 15-bit, but how does it
	 * work on win bgr565 then? */
	m_Client->format.redShift = 10;
	m_Client->format.greenShift = 5;
	m_Client->format.blueShift = 0;
	return 0;
}

std::string Client::GetServerIP() {
	std::string server = m_ConfigStorage->GetServer();
	return server.erase(server.find_first_of(':'));
}

int Client::Connect() {
	int argc;
	char **argv;
	argc = m_ConfigStorage->GetArgC();
	argv = m_ConfigStorage->GetArgV();
	if (!rfbInitClient(m_Client, &argc, argv)) {
		/* rfbInitClient has already freed the client struct */
		m_Client = NULL;
		return -1;
	}
	/* start worker thread */
	if (NULL == m_Thread) {
		/* m_Thread will be non-null if we try to reconnect  */
		m_Thread = ThreadFactory::GetNewThread();
		m_Thread->SetWorker(PollRFB, static_cast<void *>(this));
	}
	return m_Thread->Start();
}

int Client::GetScreenSize(int &width, int &height) {
	width = 0;
	height = 0;
	if (NULL == m_Client) {
		return -1;
	}
	width = m_Client->width;
	height = m_Client->height;
	return 0;
}

void Client::SetClientSize(int width, int height) {
	m_ScalingFactorX = static_cast<float>(m_Client->width) / width;
	m_ScalingFactorY = static_cast<float>(m_Client->height) / height;
	m_NeedScaling = m_Client->width != width || m_Client->height != height;
}

rfbBool Client::MallocFrameBuffer(rfbClient *client) {
	return Client::GetInstance()->OnMallocFrameBuffer(client);
}

void Client::GotFrameBufferUpdate(rfbClient *client, int x, int y, int w, int h) {
	Client::GetInstance()->OnFrameBufferUpdate(client, x, y, w, h);
}

int Client::PostEvent(event_t &evt) {
	m_Mutex->lock();
	m_MessageQueue.push_back(evt);
	m_Mutex->unlock();
	return 0;
}

int Client::GetEvent(event_t &evt) {
	int result = 0;

	m_Mutex->lock();
	if (m_MessageQueue.size()) {
		evt = m_MessageQueue.front();
		m_MessageQueue.pop_front();
		result = 1;
	}
	m_Mutex->unlock();
	return result;
}

void Client::HandleKey(key_t key) {
	uint32_t rfb_key;
	switch (key) {
	case KEY_BACK:
		rfb_key = XK_Escape;
		rfbClientLog("Key event: KEY_BACK\n");
		break;
	case KEY_HOME:
		rfb_key = XK_Home;
		rfbClientLog("Key event: KEY_HOME\n");
		break;
	default:
		return;
	}
	if (m_NeedsVirtInpHack) {
		SendPointerEvent(m_Client, -1, -1, rfbButton1Mask);
	}
	SendKeyEvent(m_Client, rfb_key, TRUE);
	SendKeyEvent(m_Client, rfb_key, FALSE);
	if (m_NeedsVirtInpHack) {
		SendPointerEvent(m_Client, -1, -1, 0);
	}
}

int Client::Poll() {
	int result, evt_count;
	event_t evt;

	result = WaitForMessage(m_Client, 500);
	if (result < 0) {
		/* terminating due to error */
		OnShutdown();
		return result;
	}
	if (result) {
		if (!HandleRFBServerMessage(m_Client)) {
			/* terminating due to error */
			OnShutdown();
			return -1;
		}
	}
	if (m_ForceRefreshToMs && (GetTimeMs() - m_LastRefreshTimeMs > m_ForceRefreshToMs)) {
		SendFramebufferUpdateRequest(m_Client, 0, 0, m_Client->width, m_Client->height, false);
		m_LastRefreshTimeMs = GetTimeMs();
		DEBUGMSG(TRUE, (_T("\r\nSendFramebufferUpdateRequest\r\n\r\n")));
	}
	/* checki if there are input events */
	evt_count = 0;
	while ((evt_count < MAX_EVT_PROCESS_AT_ONCE) && GetEvent(evt)) {
		/* send to the server */
		switch (evt.what) {
		case EVT_MOUSE:
		/* fall through */
		case EVT_MOVE:
		{
			SendPointerEvent(m_Client,
				static_cast<int>(evt.data.point.x * m_ScalingFactorX),
				static_cast<int>(evt.data.point.y * m_ScalingFactorY),
				evt.data.point.is_down ? rfbButton1Mask : 0);
			rfbClientLog("Mouse event at %d:%d, is_down %d\n",
				evt.data.point.x, evt.data.point.y, evt.data.point.is_down);
			break;
		}
		case EVT_KEY:
		{
			HandleKey(evt.data.key);
			break;
		}
		default:
			break;
		}
	}
	return result;
}
