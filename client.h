#ifndef _CLIENT_H
#define _CLIENT_H

#include <string>
#include <deque>
#include <rfb/rfbclient.h>
#include "mutex_factory.h"
#include "thread_factory.h"

class Client {
public:
	Client();
	virtual ~Client();

	static Client *GetInstance() {
		return m_Instance;
	};

	int Start(void *_private);
protected:
	enum input_event_type_t {
		INPUT_EVT_MOUSE,
		INPUT_EVT_MOVE,
		INPUT_EVENT_KEYPAD
	};
	enum input_event_state_t {
		INPUT_EVT_UP,
		INPUT_EVT_DOWN
	};
	struct input_event_t {
		input_event_type_t type;
		input_event_state_t state;
		int x;
		int y;
	};
	static Client *m_Instance;
	void *m_Private;
	rfbClient* m_Client;
	Thread *m_Thread;
	Mutex *m_Mutex;
	/* message queue */
	std::deque< input_event_t > m_InputQueue;

	virtual void SetLogging() = 0;
	int Poll();
	int PostInputEvent(input_event_t &evt);
	int GetInputEvent(input_event_t &evt);
	virtual rfbBool OnMallocFrameBuffer(rfbClient *client) = 0;
	virtual void OnFrameBufferUpdate(rfbClient *cl, int x, int y, int w, int h) = 0;
private:
	static std::string VNC_PARAMS[];
	int argc;
	char **argv;

	void AllocateArgv(int count);
	void DeleteArgv();
	int ReadInitData();
	void SetDefaultParams();

	static int PollRFB(void *data);
	static rfbBool MallocFrameBuffer(rfbClient *client);
	static void GotFrameBufferUpdate(rfbClient *client, int x, int y, int w, int h);
};

#endif
