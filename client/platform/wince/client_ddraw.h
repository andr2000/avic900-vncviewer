#ifndef _CLIENT_DDRAW_H
#define _CLIENT_DDRAW_H

#include "client_wince.h"

class Client_DDraw : public Client_WinCE {
public:
	int Initialize(void *_private);
	rfbBool OnMallocFrameBuffer(rfbClient *client);
	virtual void OnFrameBufferUpdate(rfbClient *client, int x, int y, int w, int h);
};

#endif
