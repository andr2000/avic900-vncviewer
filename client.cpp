#include <rfb/rfbclient.h>
#include "client.h"
#include "compat.h"
#include "INIReader.h"

Client *Client::m_Instance = NULL;

std::string Client::VNC_PARAMS[] = {
	/* this one MUST be the last */
	"192.168.2.1:5901"
};

Client::Client() {
	m_Private = NULL;
	m_Client = NULL;
	m_Thread = NULL;
	m_Mutex = MutexFactory::GetNewMutex();
	argc = 0;
	argv = NULL;
	SetDefaultParams();
}

Client::~Client() {
	m_MessageQueue.clear();
	DeleteArgv();
	if (m_Mutex) {
		delete m_Mutex;
	}
}

int Client::PollRFB(void *data) {
	Client *context = reinterpret_cast<Client *>(data);
	return context->Poll();
}

int Client::ReadInitData() {
	std::string host;
	size_t pos;

	if (m_IniFName.empty()) {
		return -1;
	}
	INIReader ini(m_IniFName);
	pos = 0;
	host = ini.Get("General", "Host", "192.168.2.1:5901");
	rfbClientLog("Host is %s\n", host.c_str());
	pos++;
	AllocateArgv(pos);
	/* this MUST be the last */
	strcpy(argv[pos], host.c_str());
	return 0;
}

void Client::DeleteArgv() {
	if (argv) {
		for (int i = 0; i < argc; i++) {
			delete[] argv[i];
		}
		delete[] argv;
	}
	argc = 0;
}

void Client::AllocateArgv(int count) {
	DeleteArgv();
	/* +1 for exe name */
	count++;
	argv = new char *[count];
	for (int i = 0; i < count; i++) {
		argv[i] = new char[MAX_PATH + 1];
	}
	argc = count;
	strcpy(argv[0], m_ExecFName.c_str());
}

void Client::SetDefaultParams() {
	DeleteArgv();
	AllocateArgv(sizeof(VNC_PARAMS) / sizeof(VNC_PARAMS[0]));
	for (int i = 1; i < argc; i++) {
		strcpy(argv[i], VNC_PARAMS[i - 1].c_str());
	}
}

int Client::Start(void *_private, std::string &exe, std::string &ini) {
	m_ExecFName = exe;
	m_IniFName = ini;
	/* set logging options */
#ifdef DEBUG
	rfbEnableClientLogging = TRUE;
#else
	rfbEnableClientLogging = FALSE;
#endif
	SetLogging();
	rfbClientLog("Initializing VNC Client\n");
	m_Private = _private;
	/* read ini file */
	if (ReadInitData() < 0) {
		SetDefaultParams();
	}
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

	/* and start */
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
		rfb_key = XK_Delete;
		break;
	case KEY_HOME:
		rfb_key = XK_Home;
		break;
	default:
		return;
	}
	SendKeyEvent(m_Client, rfb_key, TRUE);
	SendKeyEvent(m_Client, rfb_key, FALSE);
}

int Client::Poll() {
	int result, evt_count;
	event_t evt;

	result = WaitForMessage(m_Client, 500);
	if (result < 0) {
		/* terminating due to error */
		return result;
	}
	if (result) {
		if (!HandleRFBServerMessage(m_Client)) {
			/* terminating due to error */
			return -1;
		}
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
			SendPointerEvent(m_Client, evt.data.point.x, evt.data.point.y,
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
