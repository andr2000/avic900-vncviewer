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
	LPDIRECTDRAWSURFACE lpBackBuffer;

	void BlitFrontToBack(LPRECT rect);
};

#endif
