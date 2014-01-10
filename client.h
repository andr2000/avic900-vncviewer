#ifndef _CLIENT_H
#define _CLIENT_H

#include <string>
#include <rfb/rfbclient.h>
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
	static Client *m_Instance;
	void *m_Private;
	rfbClient* m_Client;
	Thread *m_Thread;

	int Poll();
	virtual rfbBool OnMallocFrameBuffer(rfbClient *client) = 0;
	virtual void OnFrameBufferUpdate(rfbClient *cl, int x, int y, int w, int h) = 0;
private:
	static const int NUM_VNC_PARAMS = 2;
	struct vnc_params_t {
		std::string host_name;
		std::string exe_name;
	};
	int argc;
	char **argv;
	vnc_params_t m_Params;

	void PrepareArgv();
	void DeleteArgv();
	int ReadInitData();
	void SetDefaultParams();

	static int PollRFB(void *data);
	static rfbBool MallocFrameBuffer(rfbClient *client);
	static void GotFrameBufferUpdate(rfbClient *client, int x, int y, int w, int h);
};

#endif
