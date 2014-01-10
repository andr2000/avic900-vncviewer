#ifndef _CLIENT_WINCE_H
#define _CLIENT_WINCE_H

#include <rfb/rfbclient.h>
#include "client.h"

class Client_WinCE : public Client {
public:
	Client_WinCE();
	virtual ~Client_WinCE();

	static Client *GetInstance_WinCE() {
		if (NULL != Client::GetInstance()) {
			return Client::GetInstance();
		}
		m_Instance = new Client_WinCE();
		return m_Instance;
	};
protected:
	rfbBool OnMallocFrameBuffer(rfbClient *client);
	void OnFrameBufferUpdate(rfbClient *cl, int x, int y, int w, int h);
};

#endif
