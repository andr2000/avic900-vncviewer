#include "stdafx.h"
#include "vncviewer.h"
#include "vncviewerDlg.h"

#include "client_wince.h"
#include "config_storage.h"

rfbBool Client_MFC::OnMallocFrameBuffer(rfbClient *client) {
	CDialog *dlg = static_cast<CDialog *>(m_Private);
	m_hWnd = dlg->m_hWnd;
	return Cleint_WinCE::OnMallocFrameBuffer(client);
}

void Client_MFC::OnFrameBufferUpdate(rfbClient* client, int x, int y, int w, int h) {
	CDialog *dlg = static_cast<CDialog *>(m_Private);
	RECT ps;

	if (m_NeedScaling) {
		/* TODO: we invalidate the whole screen, otherwise we have partial
		 updates. This needs to be fixed */
		ps.left = 0;
		ps.top = 0;
		ps.right = client->width;
		ps.bottom = client->height;
	} else {
		ps.left = x;
		ps.top = y;
		ps.right = x + w;
		ps.bottom = y + h;
	}
	dlg->InvalidateRect(&ps, FALSE);
	DEBUGMSG(TRUE, (_T("OnFrameBufferUpdate: x=%d y=%d w=%d h=%d\r\n"), x, y, w, h));
}
