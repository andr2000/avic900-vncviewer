#include <stdlib.h>

#include "stdafx.h"
#include "vncviewer.h"
#include "vncviewerDlg.h"

#include "client_wince.h"

Client_WinCE::Client_WinCE() : Client() {
	m_FrameBuffer = NULL;
	GetScreenRotation(m_OrigScreenRotation);
	m_RotationNeeded = 0;
}

Client_WinCE::~Client_WinCE() {
	RotationRestore();
}

void Client_WinCE::Logger(const char *format, ...) {
    va_list args;
    char buf[LOG_BUF_SZ];
	wchar_t buf_w [LOG_BUF_SZ];

    if(!rfbEnableClientLogging)
      return;

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

	/* get original screen size */
	RECT wnd_rect;
	dlg->GetWindowRect(&wnd_rect);
	DEBUGMSG(TRUE, (_T("OnMallocFrameBuffer: left=%d top=%d right=%d bottom=%d\r\n"),
		wnd_rect.left, wnd_rect.top, wnd_rect.right, wnd_rect.bottom));

	int wnd_width, wnd_height;

	wnd_width = wnd_rect.right - wnd_rect.left;
	wnd_height = wnd_rect.bottom - wnd_rect.top;
	/* check if screen rotation is needed */
	m_RotationNeeded = RotationNeeded(wnd_width, wnd_height, client->width, client->height);
	RotationSet();
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

int Client_WinCE::RotateScreen(int angle) {
	DEVMODE dev_mode;
	int rotation_angles;

	/* Check for rotation support by getting the rotation angles supported. */
	memset(&dev_mode, 0, sizeof(dev_mode));
	dev_mode.dmSize = sizeof(dev_mode);
	dev_mode.dmFields = DM_DISPLAYQUERYORIENTATION;
	if (DISP_CHANGE_SUCCESSFUL == ChangeDisplaySettingsEx(NULL, &dev_mode, NULL, CDS_TEST, NULL)) {
		rotation_angles = dev_mode.dmDisplayOrientation;
		DEBUGMSG(TRUE, (_T("Display supports these rotation angles %d\r\n"), rotation_angles));
	} else {
		DEBUGMSG(TRUE, (_T("Failed to get the supported rotation angles.\r\n")));
		rotation_angles = -1;
		return -1;
	}
	/* check if requested rotation supported, DMDO_0 is always supported  */
	if ((DMDO_0 != angle) && 0 == (angle & rotation_angles)) {
		DEBUGMSG(TRUE, (_T("Rotation angle is not supported.\r\n")));
		return -1;
	}
	/* Rotate it now */
	memset(&dev_mode, 0, sizeof(dev_mode));
	dev_mode.dmSize = sizeof(dev_mode);
	dev_mode.dmFields = DM_DISPLAYORIENTATION;
	dev_mode.dmDisplayOrientation = angle;

	if (DISP_CHANGE_SUCCESSFUL == ChangeDisplaySettingsEx(NULL, &dev_mode, NULL, CDS_RESET, NULL)) {
		DEBUGMSG(TRUE, (_T("Changed rotation angle to %d\r\n"), angle));
	} else {
		DEBUGMSG(TRUE, (_T("Failed to change the rotation angle to %d\r\n"), angle));
	}
	return 0;
}

int Client_WinCE::GetScreenRotation(int &angle) {
	DEVMODE dev_mode;

	memset(&dev_mode, 0, sizeof(dev_mode));
	dev_mode.dmSize = sizeof(dev_mode);
	dev_mode.dmFields = DM_DISPLAYORIENTATION;
	if (DISP_CHANGE_SUCCESSFUL == ChangeDisplaySettingsEx(NULL, &dev_mode, NULL, CDS_TEST, NULL)) {
		angle = dev_mode.dmDisplayOrientation;
		DEBUGMSG(TRUE, (_T("Current rotation angle is %d\r\n"), angle));
	} else {
		DEBUGMSG(TRUE, (_T("Failed to get current rotation angle.\r\n")));
		angle = -1;
		return -1;
	}
	return 0;
}

int Client_WinCE::RotationNeeded(int wnd_width, int wnd_height, int cl_width, int cl_height) {
	int wnd_is_vertical, cl_is_vertical;

	wnd_is_vertical = wnd_width < wnd_height;
	cl_is_vertical = cl_width < cl_height;
	if (wnd_is_vertical && cl_is_vertical) {
		/* both are vertical orientation */
		return 0;
	}
	if (wnd_is_vertical) {
		if (cl_is_vertical) {
			return 0;
		} else {
			return cl_width > wnd_width;
		}
	} else {
		if (cl_is_vertical) {
			return cl_height > wnd_height;
		} else {
			return 0;
		}
	}
	return 0;
}

void Client_WinCE::RotationSet() {
	if (m_RotationNeeded) {
		RotateScreen(DMDO_0);
	}
}

void Client_WinCE::RotationRestore() {
	if (-1 != m_OrigScreenRotation) {
		RotateScreen(m_OrigScreenRotation);
	}
}
