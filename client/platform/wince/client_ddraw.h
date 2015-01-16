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

class Client_DDraw : public Client_WinCE
{
public:
	Client_DDraw();

	int Initialize();
	void OnActivate(bool isActive);
	virtual void OnPaint(void);

protected:
#ifdef WINMOB
	LPDIRECTDRAW lpDD;
	LPDIRECTDRAWSURFACE lpBlitSurface;
	LPDIRECTDRAWSURFACE lpFrontBuffer;
#else
	LPDIRECTDRAW4 lpDD;
	LPDIRECTDRAWSURFACE4 lpBlitSurface;
	LPDIRECTDRAWSURFACE4 lpFrontBuffer;
#endif
	LPDIRECTDRAWCLIPPER lpClipper;

	DWORD m_CooperativeLevel;
	bool m_Initialized;

	Mutex_WinCE m_DrawLock;

	virtual void OnFinishedFrameBufferUpdate(rfbClient *client);
	void OnShutdown();

	bool RestoreSurfaces(void);
	virtual void ReleaseResources(void);
	virtual BOOL Blit(int x, int y, int w, int h);
	virtual int CreateSurface();
	virtual int SetupClipper();
	void ShowFullScreen(bool fullScreen);
};

#endif
