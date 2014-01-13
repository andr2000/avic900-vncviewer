#include <rfb/rfbclient.h>
#include "client.h"
#include "logger.h"
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
	std::string filepath = get_exe_name();
	std::string host;
	size_t pos;

	pos = filepath.find_last_of(".");
	if (std::string::npos == pos) {
		return -1;
	}
	INIReader ini(filepath.substr(0, pos) + ".ini");
	pos = 0;
	host = ini.Get("General", "Host", "192.168.2.1:5901");
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
	strcpy(argv[0], get_exe_name());
}

void Client::SetDefaultParams() {
	DeleteArgv();
	AllocateArgv(sizeof(VNC_PARAMS) / sizeof(VNC_PARAMS[0]));
	for (int i = 1; i < argc; i++) {
		strcpy(argv[i], VNC_PARAMS[i - 1].c_str());
	}
}

int Client::Start(void *_private) {
	LOG("Initializing VNC Client");
	m_Private = _private;
	/* read ini file */
	if (!ReadInitData()) {
		SetDefaultParams();
	}
	/* get new RFB client */
	m_Client = rfbGetClient(8, 3, 4);
	if (NULL == m_Client) {
		return -1;
	}
	/* initialize it */
	m_Client->MallocFrameBuffer = MallocFrameBuffer;
	m_Client->canHandleNewFBSize = FALSE;
	m_Client->GotFrameBufferUpdate = GotFrameBufferUpdate;
#if 0
	m_Client->HandleKeyboardLedState=kbd_leds;
	m_Client->HandleTextChat=text_chat;
	m_Client->GotXCutText = got_selection;
#endif
	m_Client->listenPort = -1;
	m_Client->listen6Port = -1;
#if 1
	m_Client->format.redShift = 16;
	m_Client->format.greenShift = 8;
	m_Client->format.blueShift = 0;
#endif

	/* set logging options */
	SetLogging();
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
			DEBUGMSG(TRUE, (_T("Mouse event at %d:%d, is_down %d\r\n"),
				evt.data.point.x, evt.data.point.y, evt.data.point.is_down));
			break;
		}
		default:
			break;
		}
	}
	return result;
}
