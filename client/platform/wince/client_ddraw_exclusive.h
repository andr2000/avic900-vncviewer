#ifndef _CLIENT_DDRAW_EXCLUSIVE_H
#define _CLIENT_DDRAW_EXCLUSIVE_H

#include "client_ddraw.h"

class Client_DDraw_Exclusive : public Client_DDraw {
public:
	Client_DDraw_Exclusive();

	void OnPaint(void);

protected:
	void ReleaseResources(void);
	BOOL Blit(int x, int y, int w, int h);
	int CreateSurface();
	int SetupClipper();

private:
#ifdef WINMOB
	LPDIRECTDRAWSURFACE lpBackBuffer;
	bool m_SingleBuffer;
#else
	LPDIRECTDRAWSURFACE4 lpBackBuffer;
#endif

	void BlitFrontToBack(LPRECT rect);
#ifdef WINMOB
	static HRESULT PASCAL EnumFunction(LPDIRECTDRAWSURFACE pSurface,
		LPDDSURFACEDESC lpSurfaceDesc, LPVOID  lpContext);
#endif
};

#endif
