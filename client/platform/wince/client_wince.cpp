#include "stdafx.h"

#include "client_wince.h"
#include "config_storage.h"

const wchar_t *CvncviewerDlg::WND_PROC_NAMES[] = {
	/* Launcher MUST be the first entry */
	{ TEXT("Launcher") },
	{ TEXT("MainMenu") }
};

Client_WinCE::Client_WinCE() : Client() {
	m_hWnd = NULL;
	m_FrameBuffer = NULL;
	m_hBmp = NULL;
	m_HotkeyHwnd = NULL;
	m_HotkeyWndProc = NULL;
	m_ConfigStorage = NULL;
	m_FilterAutoRepeat = false;
	m_LongPress = false;
}

Client_WinCE::~Client_WinCE() {
	Cleanup();
	SetHotkeyHandler(false);
	KillTimer(ID_TIMER_LONG_PRESS);
	if (m_ConfigStorage) {
		delete m_ConfigStorage;
	}
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

	m_hBmp = CreateDIBSection(CreateCompatibleDC(GetDC(m_hWnd)),
		&bm_info, DIB_RGB_COLORS, reinterpret_cast<void**>(&m_FrameBuffer),
		NULL, NULL);
	client->frameBuffer = m_FrameBuffer;
	return TRUE;
}

void Client_WinCE::OnFrameBufferUpdate(rfbClient* client, int x, int y, int w, int h) {
}

void Client_WinCE::OnShutdown() {
	PostMessage(WM_QUIT, 0, 0);
}

int Client_WinCE::Initialize(void *_private)
{
	m_ConfigStorage = ConfigStorage::GetInstance();

	wchar_t wfilename[MAX_PATH + 1];
	char filename[MAX_PATH + 1];

	GetModuleFileName(NULL, wfilename, MAX_PATH);
	wcstombs(filename, wfilename, sizeof(filename));
	std::string exe(filename);
	char *dot = strrchr(filename, '.');
	if (dot) {
		*dot = '\0';
	}
	std::string ini(filename);
	ini += ".ini";

	m_ConfigStorage->Initialize(exe, ini);

	SetRect(&m_ClientRect, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	server = m_ConfigStorage->GetServer();
	widestr = std::wstring(server.begin(), server.end());
	/* let's rock */
	for (i = 0; i <= CONNECT_MAX_TRY; i++) {
		if (Client::Initialize(_private) < 0) {
			return -1;
		}
		if (0 == Connect()) {
			break;
		}
		if (IDCANCEL == Message(MB_RETRYCANCEL, _T("Error"),
			_T("Failed to connect to %ls\r\nRetry?"), widestr.c_str())) {
				PostMessage(WM_CLOSE);
				return true;
		}
	}
	if (i == CONNECT_MAX_TRY) {
		Message(MB_OK, _T("Error"),
			_T("Was not able to connect to %ls\r\nGiving up now"), widestr.c_str());
		PostMessage(WM_CLOSE);
		return -1;
	}
	/* install handlers to intercept WM_HOTKEY */
	SetHotkeyHandler(true);
	return 0;
}

void Client_WinCE::SetHotkeyHandler(bool set) {
	if (set) {
		if (m_HotkeyHwnd && m_HotkeyWndProc) {
			return;
		}
		int i, num_windows = sizeof(WND_PROC_NAMES)/sizeof(WND_PROC_NAMES[0]);
		for (i = 0; i < num_windows; i++) {
			m_HotkeyHwnd = ::FindWindow(NULL, WND_PROC_NAMES[i]);
			if (NULL == m_HotkeyHwnd) {
				continue;
			}
			/* found, substitute */
			DEBUGMSG(true, (_T("Found original hotkey handler: %s\r\n"),
				WND_PROC_NAMES[i]));
			m_HotkeyWndProc = (WNDPROC)GetWindowLong(m_HotkeyHwnd, GWL_WNDPROC);
			if (m_HotkeyWndProc) {
				SetWindowLong(m_HotkeyHwnd, GWL_WNDPROC, (LONG)SubWndProc);
				break;
			} else {
				m_HotkeyHwnd = NULL;
			}
		}
		if (i == num_windows) {
			DEBUGMSG(true, (_T("DID NOT find the original hotkey handler\r\n")));
		}
	} else {
		if (m_HotkeyWndProc && m_HotkeyHwnd) {
			SetWindowLong(m_HotkeyHwnd, GWL_WNDPROC, (LONG)m_HotkeyWndProc);
		}
		m_HotkeyHwnd = NULL;
		m_HotkeyWndProc = NULL;
	}
}

LRESULT CALLBACK Client_WinCE::SubWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	CvncviewerDlg *dlg = CvncviewerDlg::GetInstance();

	if (dlg && (WM_HOTKEY == message)) {
		Client::event_t evt;
		evt.what = Client::EVT_KEY;

		switch (LOWORD(wParam)) {
			case HW_BTN_MAP:
			{
				if (LOWORD(lParam) == 0x0) {
					/* pressed */
					DEBUGMSG(true, (_T("MAP pressed\r\n")));
					if (!dlg->m_FilterAutoRepeat && !dlg->m_LongPress) {
						/* start processing long press */
						dlg->SetTimer(ID_TIMER_LONG_PRESS, ID_TIMER_LONG_PRESS_DELAY, NULL);
						dlg->m_FilterAutoRepeat = true;
					}
					return 1;
				} else if (LOWORD(lParam) == 0x1000) {
					/* released */
					DEBUGMSG(true, (_T("MAP released\r\n")));
					if (dlg->m_FilterAutoRepeat) {
						dlg->m_FilterAutoRepeat = false;
						dlg->KillTimer(ID_TIMER_LONG_PRESS);
					}
					if (dlg->m_LongPress) {
						/* already handled by the timer */
						dlg->m_LongPress = false;
						return 1;
					}
					dlg->HandleMapKey(false);
				}
				/* eat the key */
				return 1;
			}
			case HW_BTN_UP:
			{
				if (LOWORD(lParam) == 0x0) {
					/* pressed */
					DEBUGMSG(true, (_T("UP pressed\r\n")));
					evt.data.key = Client::KEY_UP;
					dlg->SendEvent(evt);
				}
				/* eat the key */
				return 1;
			}
			case HW_BTN_DOWN:
			{
				if (LOWORD(lParam) == 0x0) {
					/* pressed */
					DEBUGMSG(true, (_T("DOWN pressed\r\n")));
					evt.data.key = Client::KEY_DOWN;
					dlg->SendEvent(evt);
				}
				/* eat the key */
				return 1;
			}
			case HW_BTN_LEFT:
			{
				if (LOWORD(lParam) == 0x0) {
					/* pressed */
					DEBUGMSG(true, (_T("LEFT pressed\r\n")));
					evt.data.key = Client::KEY_LEFT;
					dlg->SendEvent(evt);
				}
				/* eat the key */
				return 1;
			}
			case HW_BTN_RIGHT:
			{
				if (LOWORD(lParam) == 0x0) {
					/* pressed */
					DEBUGMSG(true, (_T("RIGHT pressed\r\n")));
					evt.data.key = Client::KEY_RIGHT;
					dlg->SendEvent(evt);
				}
				/* eat the key */
				return 1;
			}
			default:
			{
				break;
			}
		}
	}
	/* skip this message and pass it to the adressee */
	return CallWindowProc(dlg->m_HotkeyWndProc,
		hWnd, message, wParam, lParam);
}

void Client_WinCE::HandleMapKey(bool long_press) {
	Client::event_t evt;
	evt.what = Client::EVT_KEY;
	if (long_press) {
		/* long press */
		evt.data.key = Client::KEY_HOME;
	} else {
		/* normal press */
		evt.data.key = Client::KEY_BACK;
	}
	PostEvent(evt);
}

void Client_WinCE::OnTimer(void)
{
	/* MAP long press */
	KillTimer(ID_TIMER_LONG_PRESS);
	m_FilterAutoRepeat = false;
	m_LongPress = true;
	HandleMapKey(true);
}
