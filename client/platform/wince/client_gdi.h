#ifndef _CLIENT_GDI_H
#define _CLIENT_GDI_H

#include "client_wince.h"

class Client_GDI : public Client_WinCE {
public:
	void OnPaint(void);

protected:
	void OnFrameBufferUpdate(rfbClient *client, int x, int y, int w, int h);
};

#endif
