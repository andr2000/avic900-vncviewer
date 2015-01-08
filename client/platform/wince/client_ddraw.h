#ifndef _CLIENT_DDRAW_H
#define _CLIENT_DDRAW_H

#ifdef WIN32
#define INITGUID
#endif

#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <ddraw.h>

#include "client_wince.h"

class Client_DDraw : public Client_WinCE {
public:
	int Initialize();

protected:
	rfbBool OnMallocFrameBuffer(rfbClient *client);
	void OnFinishedFrameBufferUpdate(rfbClient *client);
	void OnShutdown();

private:
	LPDIRECTDRAW4 lpDD;
	LPDIRECTDRAWSURFACE4 lpFrontBuffer;
	LPDIRECTDRAWSURFACE4 lpBackBuffer;
	HDC hdcImage;

	bool RestoreSurfaces(void);
	void ReleaseResources(void);
};

#endif
