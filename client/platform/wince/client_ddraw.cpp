#include "client_ddraw.h"

rfbBool Client_DDraw::OnMallocFrameBuffer(rfbClient *client)
{
	return TRUE;
}

void Client_DDraw::OnFrameBufferUpdate(rfbClient* client, int x, int y, int w, int h) {
	DEBUGMSG(TRUE, (_T("OnFrameBufferUpdate: x=%d y=%d w=%d h=%d\r\n"), x, y, w, h));
}

int Client_DDraw::Initialize(void *_private)
{
	return 0;
}
