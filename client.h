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

	enum event_type_t {
		EVT_MOUSE,
		EVT_MOVE,
		EVT_ACTION
	};
	enum event_args_t {
		ARG_KEY_UP,
		ARG_KEY_DOWN,
		ARG_HOLD_ON,
		ARG_HOLD_OFF
	};
	struct input_event_t {
		event_type_t type;
		event_args_t opts;
		int x;
		int y;
	};
	int PostEvent(input_event_t &evt);
protected:
	static Client *m_Instance;
	void *m_Private;
	rfbClient* m_Client;
	Thread *m_Thread;
	Mutex *m_Mutex;
	/* message queue */
	std::deque< input_event_t > m_MessageQueue;

	virtual void SetLogging() = 0;
	int Poll();
	int GetEvent(input_event_t &evt);
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
