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
		EVT_MOVE
	};
	struct event_t {
		event_type_t what;
		union event_data_t {
			struct pointer_action_t {
				int is_down;
				int x;
				int y;
			} point;
		} data;
	};
	int PostEvent(event_t &evt);
	virtual void *GetDrawingContext() = 0;
protected:
	static Client *m_Instance;
	void *m_Private;
	rfbClient* m_Client;
	Thread *m_Thread;
	Mutex *m_Mutex;
	/* message queue */
	std::deque< event_t > m_MessageQueue;
	static const int MAX_EVT_PROCESS_AT_ONCE = 20;

	virtual void SetLogging() = 0;
	int Poll();
	int GetEvent(event_t &evt);
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
