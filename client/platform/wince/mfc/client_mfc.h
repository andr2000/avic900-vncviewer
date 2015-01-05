#ifndef _CLIENT_MFC_H
#define _CLIENT_MFC_H

#include "client_wince.h"

class Client_MFC : public Client_WinCE {
public:
	int Initialize(void *_private);

protected:
	void OnFrameBufferUpdate(rfbClient *client, int x, int y, int w, int h);
	void OnShutdown();
};

#endif
