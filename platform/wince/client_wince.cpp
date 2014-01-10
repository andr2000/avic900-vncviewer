#include "stdafx.h"
#include "vncviewer.h"
#include "vncviewerDlg.h"

#include "client_wince.h"

Client_WinCE::Client_WinCE() : Client() {
}

Client_WinCE::~Client_WinCE() {
}

rfbBool Client_WinCE::OnMallocFrameBuffer(rfbClient *client) {
	CDialog *dlg = static_cast<CDialog *>(m_Private);
	int width, height, depth;

	width = client->width;
	height = client->height;
	depth = client->format.bitsPerPixel;
	DEBUGMSG(TRUE, (_T("OnMallocFrameBuffer: w=%d h=%d d=%d\r\n"), width, height, depth));
	client->updateRect.x = 0;
	client->updateRect.y = 0;
	client->updateRect.w = width;
	client->updateRect.h = height;

	RECT lRect;
	dlg->GetWindowRect(&lRect);
	DEBUGMSG(TRUE, (_T("OnMallocFrameBuffer: left=%d top=%d right=%d bottom=%d\r\n"),
		lRect.left, lRect.top, lRect.right, lRect.bottom));
	return TRUE;
}

void Client_WinCE::OnFrameBufferUpdate(rfbClient* cl, int x, int y, int w, int h) {
	DEBUGMSG(TRUE, (_T("OnFrameBufferUpdate: x=%d y=%d w=%d h=%d\r\n"), x, y, w, h));

	CDialog *dlg = static_cast<CDialog *>(m_Private);
}
