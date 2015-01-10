#include <string.h>
#ifdef WINCE
#include <aygshell.h>
#endif

#include "client_wince.h"
#include "config_storage.h"

const wchar_t *Client_WinCE::WND_PROC_NAMES[] = {
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
	m_Active = false;
	memset(&m_WindowRect, 0, sizeof(m_WindowRect));
}

Client_WinCE::~Client_WinCE() {
	SetHotkeyHandler(false);
	KillTimer(m_hWnd, ID_TIMER_LONG_PRESS);
#ifdef SHOW_POINTER_TRACE
	KillTimer(m_hWnd, ID_TIMER_TRACE);
#endif
	if (m_hBmp) {
		DeleteObject(m_hBmp);
	}
#ifdef SHOW_POINTER_TRACE
	m_TraceQueue.clear();
#endif
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
	DEBUGMSG(TRUE, (TEXT("%s\r\n"), buf_w));
}

void Client_WinCE::SetLogging() {
	rfbClientLog = Logger;
	rfbClientErr = Logger;
}

int Client_WinCE::ShowMessage(DWORD type, wchar_t *caption, wchar_t *format, ...) {
	wchar_t msg_text[2 * MAX_PATH + 1];
	va_list vargs;

	va_start(vargs, format);
	_vsnwprintf(msg_text, sizeof(msg_text), format, vargs);
	va_end(vargs);
	return MessageBox(m_hWnd, msg_text, caption, type);
}

#ifdef SHOW_POINTER_TRACE
void Client_WinCE::AddTracePoint(trace_point_type_e type, LONG x, LONG y) {
	RECT rect;
	trace_point_t trace;

	trace.type = type;
	trace.x = x;
	trace.y = y;
	m_TraceQueue.push_back(trace);

	rect.left = x;
	rect.right = x + TRACE_POINT_BAR_SZ;
	rect.top = y;
	rect.bottom = y + TRACE_POINT_BAR_SZ;
	InvalidateRect(m_hWnd, &rect, FALSE);
	KillTimer(m_hWnd, ID_TIMER_TRACE);
	SetTimer(m_hWnd, ID_TIMER_TRACE, ID_TIMER_TRACE_DELAY, NULL);
}
#endif

void Client_WinCE::SetWindowHandle(HWND hWnd)
{
	m_hWnd = hWnd;
}

int Client_WinCE::Initialize()
{
	GetClientRect(m_hWnd, &m_WindowRect);
	/* let's rock */
	int i;
	for (i = 0; i <= CONNECT_MAX_TRY; i++) {
		if (Client::Initialize() < 0) {
			return -1;
		}
		if (0 == Connect()) {
			break;
		}
		std::string server = m_ConfigStorage->GetServer();
		std::wstring widestr = std::wstring(server.begin(), server.end());
		if (IDCANCEL == ShowMessage(MB_RETRYCANCEL, TEXT("Error"),
			TEXT("Failed to connect to %ls\r\nRetry?"), widestr.c_str())) {
				PostMessage(m_hWnd, WM_CLOSE, 0, 0);
				return true;
		}
	}
	if (i == CONNECT_MAX_TRY) {
		std::string server = m_ConfigStorage->GetServer();
		std::wstring widestr = std::wstring(server.begin(), server.end());
		ShowMessage(MB_OK, TEXT("Error"),
			TEXT("Was not able to connect to %ls\r\nGiving up now"), widestr.c_str());
		PostMessage(m_hWnd, WM_CLOSE, 0, 0);
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
			DEBUGMSG(true, (TEXT("Found original hotkey handler: %s\r\n"),
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
			DEBUGMSG(true, (TEXT("DID NOT find the original hotkey handler\r\n")));
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
	Client_WinCE *client = static_cast<Client_WinCE *>(Client::GetInstance());
	return client->ClientSubWndProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK Client_WinCE::ClientSubWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (WM_HOTKEY == message) {
		Client::event_t evt;
		evt.what = Client::EVT_KEY;

		switch (LOWORD(wParam)) {
			case HW_BTN_MAP:
			{
				if (LOWORD(lParam) == 0x0) {
					/* pressed */
					DEBUGMSG(true, (TEXT("MAP pressed\r\n")));
					if (!m_FilterAutoRepeat && !m_LongPress) {
						/* start processing long press */
						SetTimer(m_hWnd, ID_TIMER_LONG_PRESS, ID_TIMER_LONG_PRESS_DELAY, NULL);
						m_FilterAutoRepeat = true;
					}
					return 1;
				} else if (LOWORD(lParam) == 0x1000) {
					/* released */
					DEBUGMSG(true, (TEXT("MAP released\r\n")));
					if (m_FilterAutoRepeat) {
						m_FilterAutoRepeat = false;
						KillTimer(m_hWnd, ID_TIMER_LONG_PRESS);
					}
					if (m_LongPress) {
						/* already handled by the timer */
						m_LongPress = false;
						return 1;
					}
					HandleMapKey(false);
				}
				/* eat the key */
				return 1;
			}
			case HW_BTN_UP:
			{
				if (LOWORD(lParam) == 0x0) {
					/* pressed */
					DEBUGMSG(true, (TEXT("UP pressed\r\n")));
					evt.data.key = Client::KEY_UP;
					PostEvent(evt);
				}
				/* eat the key */
				return 1;
			}
			case HW_BTN_DOWN:
			{
				if (LOWORD(lParam) == 0x0) {
					/* pressed */
					DEBUGMSG(true, (TEXT("DOWN pressed\r\n")));
					evt.data.key = Client::KEY_DOWN;
					PostEvent(evt);
				}
				/* eat the key */
				return 1;
			}
			case HW_BTN_LEFT:
			{
				if (LOWORD(lParam) == 0x0) {
					/* pressed */
					DEBUGMSG(true, (TEXT("LEFT pressed\r\n")));
					evt.data.key = Client::KEY_LEFT;
					PostEvent(evt);
				}
				/* eat the key */
				return 1;
			}
			case HW_BTN_RIGHT:
			{
				if (LOWORD(lParam) == 0x0) {
					/* pressed */
					DEBUGMSG(true, (TEXT("RIGHT pressed\r\n")));
					evt.data.key = Client::KEY_RIGHT;
					PostEvent(evt);
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
	return CallWindowProc(m_HotkeyWndProc, hWnd, message, wParam, lParam);
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

void Client_WinCE::OnTimer(UINT_PTR nIDEvent) {
	if (ID_TIMER_LONG_PRESS == nIDEvent) {
		/* MAP long press */
		KillTimer(m_hWnd, ID_TIMER_LONG_PRESS);
		m_FilterAutoRepeat = false;
		m_LongPress = true;
		HandleMapKey(true);
	}
#ifdef SHOW_POINTER_TRACE
	else if (ID_TIMER_TRACE == nIDEvent) {
		RECT r;

		KillTimer(m_hWnd, ID_TIMER_TRACE);
		m_TraceQueue.clear();
		GetWindowRect(m_hWnd, &r);
		InvalidateRect(m_hWnd, &r, FALSE);
		DEBUGMSG(true, (TEXT("Invalidate x=%d y=%d w=%d h=%d\r\n"),
			r.left, r.top, r.right, r.bottom));
	}
#endif
}

void Client_WinCE::OnTouchUp(int x, int y) {
#ifdef SHOW_POINTER_TRACE
	AddTracePoint(TRACE_POINT_UP, x, y);
#endif
	Client::event_t evt;
	evt.what = Client::EVT_MOUSE;
	evt.data.point.is_down = 0;
	evt.data.point.x = x;
	evt.data.point.y = y;
	PostEvent(evt);
}
void Client_WinCE::OnTouchDown(int x, int y) {

#ifdef SHOW_POINTER_TRACE
	AddTracePoint(TRACE_POINT_DOWN, x, y);
#endif
	Client::event_t evt;
	evt.what = Client::EVT_MOUSE;
	evt.data.point.is_down = 1;
	evt.data.point.x = x;
	evt.data.point.y = y;
	PostEvent(evt);
}
void Client_WinCE::OnTouchMove(int x, int y) {
#ifdef SHOW_POINTER_TRACE
	AddTracePoint(TRACE_POINT_MOVE, x, y);
#endif
	Client::event_t evt;
	evt.what = Client::EVT_MOVE;
	evt.data.point.is_down = 1;
	evt.data.point.x = x;
	evt.data.point.y = y;
	PostEvent(evt);
}

void Client_WinCE::OnPaint(void) {
	PAINTSTRUCT ps;
	BeginPaint(m_hWnd, &ps);
	EndPaint(m_hWnd, &ps);
}

void Client_WinCE::OnActivate(bool isActive) {
	DEBUGMSG(TRUE, (TEXT("====================================\r\nOnActivate %d\r\n"), isActive));
	m_Active = isActive;
	SetHotkeyHandler(m_Active);
	if (m_Active)
	{
		ShowFullScreen();
	}
}

void Client_WinCE::ShowFullScreen() {
#ifdef WINCE
	SHFullScreen(m_hWnd, SHFS_HIDETASKBAR | SHFS_HIDESTARTICON | SHFS_HIDESIPBUTTON);
	::ShowWindow(SHFindMenuBar(m_hWnd), SW_HIDE);
	MoveWindow(m_hWnd, m_WindowRect.left, m_WindowRect.top,
		m_WindowRect.left + m_WindowRect.right,
		m_WindowRect.top + m_WindowRect.bottom, false);
#endif
}

void Client_WinCE::OnShutdown() {
	DeleteDC(hdcImage);
	PostMessage(m_hWnd, WM_QUIT, 0, 0);
}

rfbBool Client_WinCE::OnMallocFrameBuffer(rfbClient *client) {
	int width, height, depth;

	width = client->width;
	height = client->height;
	depth = client->format.bitsPerPixel;
	DEBUGMSG(TRUE, (TEXT("OnMallocFrameBuffer: w=%d h=%d d=%d\r\n"), width, height, depth));
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
	hdcImage = CreateCompatibleDC(NULL);
	if (!hdcImage)
	{
		return FALSE;
	}
	SelectObject(hdcImage, m_hBmp);
	SetupScaling(m_WindowRect.right - m_WindowRect.left, m_WindowRect.bottom - m_WindowRect.top);
	return TRUE;
}
