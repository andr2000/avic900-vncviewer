#include "stdafx.h"
#include "vncviewer.h"
#include "vncviewerDlg.h"

#include "client_wince.h"

Client_WinCE::Client_WinCE() : Client() {
	m_FrameBuffer = NULL;
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

	/* create frambuffer */
	BITMAPINFO bm_info;
	memset(&bm_info, 0, sizeof(BITMAPINFO));
	bm_info.bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
	bm_info.bmiHeader.biWidth           = client->width;
	bm_info.bmiHeader.biHeight          = -client->height;
	bm_info.bmiHeader.biPlanes          = 1;
	bm_info.bmiHeader.biBitCount        = client->format.bitsPerPixel;
	bm_info.bmiHeader.biCompression     = BI_RGB;
	bm_info.bmiHeader.biSizeImage       = 0;
	bm_info.bmiHeader.biXPelsPerMeter   = 0;
	bm_info.bmiHeader.biYPelsPerMeter   = 0;
	bm_info.bmiHeader.biClrUsed         = 0;
	bm_info.bmiHeader.biClrImportant    = 0;

	m_hBmp = CreateDIBSection(CreateCompatibleDC(GetDC(dlg->m_hWnd)),
		&bm_info, DIB_RGB_COLORS, reinterpret_cast<void**>(&m_FrameBuffer),
		NULL, NULL);
	client->frameBuffer = m_FrameBuffer;
	return TRUE;
}

void Client_WinCE::OnFrameBufferUpdate(rfbClient* client, int x, int y, int w, int h) {
	CDialog *dlg = static_cast<CDialog *>(m_Private);
	HDC hDcMem, hDc;

	hDc = GetDC(dlg->m_hWnd);
	hDcMem = CreateCompatibleDC(hDc);
	SelectObject(hDcMem, m_hBmp);
	BitBlt(hDc, x, y, w, h, hDcMem, x, y, SRCCOPY);
	DeleteDC(hDcMem);
	ReleaseDC(NULL, hDc);
	DEBUGMSG(TRUE, (_T("OnFrameBufferUpdate: x=%d y=%d w=%d h=%d\r\n"), x, y, w, h));
}
