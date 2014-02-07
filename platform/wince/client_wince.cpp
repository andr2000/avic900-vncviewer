#include "stdafx.h"
#include "vncviewer.h"
#include "vncviewerDlg.h"

#include "client_wince.h"

Client_WinCE::Client_WinCE() : Client() {
	m_FrameBuffer = NULL;
	m_hBmp = NULL;
}

Client_WinCE::~Client_WinCE() {
	Cleanup();
	if (m_hBmp) {
		DeleteObject(m_hBmp);
	}
}

void Client_WinCE::Logger(const char *format, ...) {
	va_list args;
	char buf[LOG_BUF_SZ];
	wchar_t buf_w [LOG_BUF_SZ];

	if (!rfbEnableClientLogging) {
		return;
	}
	va_start(args, format);
	_vsnprintf(buf, LOG_BUF_SZ, format, args);
	va_end(args);
	mbstowcs(buf_w, buf, LOG_BUF_SZ);
	DEBUGMSG(TRUE, (_T("%s\r\n"), buf_w));
}

void Client_WinCE::SetLogging() {
	rfbClientLog = Logger;
	rfbClientErr = Logger;
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

	/* create frambuffer */
	/* For Windows bitmap is BGR565/BGR888, upside down - see -height below */
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
	m_LastRefreshTimeMs = GetTimeMs();
}

void Client_WinCE::OnShutdown() {
	CDialog *dlg = static_cast<CDialog *>(m_Private);

	dlg->PostMessage(WM_QUIT, 0, 0);
}
