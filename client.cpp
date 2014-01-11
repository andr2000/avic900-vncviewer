#include "client.h"
#include "logger.h"
#include "compat.h"

Client *Client::m_Instance = NULL;

Client::Client() {
	m_Private = NULL;
	m_Client = NULL;
	m_Thread = NULL;
	argc = 0;
	argv = NULL;
	SetDefaultParams();
}

Client::~Client() {
	DeleteArgv();
}

int Client::PollRFB(void *data) {
	Client *context = reinterpret_cast<Client *>(data);
	return context->Poll();
}

void Client::SetDefaultParams() {
	m_Params.host_name = "192.168.2.1:5901";
	m_Params.exe_name = get_exe_name();
}

int Client::ReadInitData() {
	return -1;
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

void Client::PrepareArgv() {
	DeleteArgv();
	argc = NUM_VNC_PARAMS;
	argv = new char *[NUM_VNC_PARAMS];
	for (int i = 0; i < argc; i++) {
		argv[i] = new char[MAX_PATH + 1];
	}
	strcpy(argv[0], m_Params.exe_name.c_str());
	strcpy(argv[1], m_Params.host_name.c_str());
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
	PrepareArgv();

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

	/* and start */
	if (!rfbInitClient(m_Client, &argc, argv)) {
		/* rfbInitClient has already freed the client struct */
		m_Client = NULL;
		return -1;
	}
	/* start worker thread */
	m_Thread = ThreadFactory::GetNewThread();
	m_Thread->SetWorker(PollRFB, static_cast<void *>(this));
	return m_Thread->Start();
}

int Client::Poll() {
	int result;

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
	return result;
}

rfbBool Client::MallocFrameBuffer(rfbClient *client) {
	return Client::GetInstance()->OnMallocFrameBuffer(client);
}

void Client::GotFrameBufferUpdate(rfbClient *client, int x, int y, int w, int h) {
	Client::GetInstance()->OnFrameBufferUpdate(client, x, y, w, h);
}
