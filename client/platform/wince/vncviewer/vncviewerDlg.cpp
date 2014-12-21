#include "stdafx.h"
#include "vncviewer.h"
#include "vncviewerDlg.h"

#include "config_storage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const wchar_t *CvncviewerDlg::WND_PROC_NAMES[] = {
	/* Launcher MUST be the first entry */
	{ TEXT("Launcher") },
	{ TEXT("MainMenu") }
};

CvncviewerDlg *CvncviewerDlg::m_Instance = NULL;

CvncviewerDlg::CvncviewerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CvncviewerDlg::IDD, pParent) {
	m_Client = NULL;
	m_HotkeyHwnd = NULL;
	m_HotkeyWndProc = NULL;
	m_Instance = this;
	m_ConfigStorage = NULL;
	m_FilterAutoRepeat = false;
	m_LongPress = false;
	memset(&m_ServerRect, 0, sizeof(m_ServerRect));
	memset(&m_ClientRect, 0, sizeof(m_ClientRect));
	m_NeedScaling = false;
	m_SetupScaling = true;
}

CvncviewerDlg::~CvncviewerDlg() {
#ifdef SHOW_POINTER_TRACE
	m_TraceQueue.clear();
#endif
}

void CvncviewerDlg::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CvncviewerDlg, CDialog)
#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
	ON_WM_SIZE()
#endif
	//}}AFX_MSG_MAP
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
//	ON_WM_SHOWWINDOW()
	ON_WM_ACTIVATE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

int CvncviewerDlg::Message(DWORD type, wchar_t *caption, wchar_t *format, ...) {
	wchar_t msg_text[2 * MAX_PATH + 1];
	va_list vargs;

	va_start(vargs, format);
	StringCchVPrintf(msg_text, sizeof(msg_text), format, vargs);
	va_end(vargs);
	return MessageBox(msg_text, caption, type);
}

void CvncviewerDlg::ShowFullScreen() {
	SHFullScreen(m_hWnd, SHFS_HIDETASKBAR | SHFS_HIDESTARTICON | SHFS_HIDESIPBUTTON);
	::ShowWindow(SHFindMenuBar(m_hWnd), SW_HIDE);
	MoveWindow(&m_ClientRect, false);
}

BOOL CvncviewerDlg::OnInitDialog()
{
	int i;
	std::wstring widestr;
	std::string server;

	CDialog::OnInitDialog();

	SetWindowText(CvncviewerApp::APP_TITLE);

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

	m_Client = ClientFactory::GetInstance();
	if (NULL == m_Client) {
		Message(MB_OK, _T("Error"),
			_T("Failed to intstaniate VNC client\r\nShit happens"));
		PostMessage(WM_CLOSE);
		return true;
	}

	/* go full screen */
	//SetRect(&m_ClientRect, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	//ShowFullScreen();

	SetupGx();

#if 1
	server = m_ConfigStorage->GetServer();
	widestr = std::wstring(server.begin(), server.end());
	/* let's rock */
	for (i = 0; i <= CONNECT_MAX_TRY; i++) {
		if (m_Client->Initialize(static_cast<void *>(this)) < 0) {
			return true;
		}
		if (0 == m_Client->Connect()) {
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
		return true;
	}
	/* install handlers to intercept WM_HOTKEY */
	SetHotkeyHandler(true);
#endif
	return true;
}

#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
void CvncviewerDlg::OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/) {
	if (AfxIsDRAEnabled())
	{
		DRA::RelayoutDialog(
			AfxGetResourceHandle(), 
			this->m_hWnd, 
			DRA::GetDisplayMode() != DRA::Portrait ? 
			MAKEINTRESOURCE(IDD_VNCVIEWER_DIALOG_WIDE) : 
			MAKEINTRESOURCE(IDD_VNCVIEWER_DIALOG));
	}
}
#endif

#ifdef SHOW_POINTER_TRACE
void CvncviewerDlg::AddTracePoint(trace_point_type_e type, LONG x, LONG y) {
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
	InvalidateRect(&rect, FALSE);
	KillTimer(ID_TIMER_TRACE);
	SetTimer(ID_TIMER_TRACE, ID_TIMER_TRACE_DELAY, NULL);
}
#endif

void CvncviewerDlg::OnLButtonUp(UINT nFlags, CPoint point) {
	CDialog::OnLButtonUp(nFlags, point);
#ifdef SHOW_POINTER_TRACE
	AddTracePoint(TRACE_POINT_UP, point.x, point.y);
#endif
	if (m_Client) {
		Client::event_t evt;
		evt.what = Client::EVT_MOUSE;
		evt.data.point.is_down = 0;
		evt.data.point.x = point.x;
		evt.data.point.y = point.y;
		m_Client->PostEvent(evt);
	}
}

void CvncviewerDlg::OnLButtonDown(UINT nFlags, CPoint point) {
	CDialog::OnLButtonDown(nFlags, point);

#ifdef SHOW_POINTER_TRACE
	AddTracePoint(TRACE_POINT_DOWN, point.x, point.y);
#endif
	if (m_Client) {
		Client::event_t evt;
		evt.what = Client::EVT_MOUSE;
		evt.data.point.is_down = 1;
		evt.data.point.x = point.x;
		evt.data.point.y = point.y;
		m_Client->PostEvent(evt);
	}
}

void CvncviewerDlg::OnMouseMove(UINT nFlags, CPoint point) {
	CDialog::OnMouseMove(nFlags, point);

#ifdef SHOW_POINTER_TRACE
	AddTracePoint(TRACE_POINT_MOVE, point.x, point.y);
#endif
	if (m_Client && (nFlags & MK_LBUTTON)) {
		Client::event_t evt;
		evt.what = Client::EVT_MOVE;
		evt.data.point.is_down = 1;
		evt.data.point.x = point.x;
		evt.data.point.y = point.y;
		m_Client->PostEvent(evt);
	}
}

void CvncviewerDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialog::OnActivate(nState, pWndOther, bMinimized);

	SetHotkeyHandler(nState != WA_INACTIVE);
	if (nState != WA_INACTIVE) {
		ShowFullScreen();
	}
}

BOOL CvncviewerDlg::OnEraseBkgnd(CDC* pDC)
{
	/* do nothing, we are full screen and painting by ourselves */
	return true;
}

void CvncviewerDlg::OnPaint()
{
			unsigned char *videoMemory = (unsigned char *)GXBeginDraw(); 
			if (videoMemory == NULL)
			{
				DEBUGMSG(true, (_T("videoMemory is null\r\n")));
			}
			else
			{
				DEBUGMSG(true, (_T("onPaint\r\n")));
				unsigned char *bmp = (unsigned char *)m_Client->GetDrawingContext();
				memcpy(videoMemory, bmp, 800 * 480 * 2);
				GXEndDraw();
			}
#ifdef SHOW_POINTER_TRACE
	{
		CBrush *brush, *old_brush;
		CPen *pen, *old_pen;
		RECT r;
		CPen pen_red, pen_green, pen_blue;

		CBrush brush_red(RGB(255, 0, 0));
		CBrush brush_green(RGB(0, 255, 0));
		CBrush brush_blue(RGB(0, 0, 255));

		pen_red.CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
		pen_green.CreatePen(PS_SOLID, 3, RGB(0, 255, 0));
		pen_blue.CreatePen(PS_SOLID, 3, RGB(0, 0, 255));

		old_brush = pDC->SelectObject(&brush_red);
		old_pen = pDC->SelectObject(&pen_red);
		for (size_t i = 0; i < m_TraceQueue.size(); i++) {
			switch (m_TraceQueue[i].type) {
				case TRACE_POINT_DOWN:
				{
					brush = &brush_red;
					pen = &pen_red;
					break;
				}
				case TRACE_POINT_MOVE:
				{
					brush = &brush_green;
					pen = &pen_green;
					break;
				}
				case TRACE_POINT_UP:
				{
					brush = &brush_blue;
					pen = &pen_blue;
					break;
				}
				default:
				{
					break;
				}
			}

			r.left = m_TraceQueue[i].x;
			r.right = r.left + TRACE_POINT_BAR_SZ;
			r.top = m_TraceQueue[i].y;
			r.bottom = r.top + TRACE_POINT_BAR_SZ;

			pDC->SelectObject(&brush_red);
			pDC->SelectObject(&pen_red);
			pDC->FillRect(&r, brush);

			DEBUGMSG(true, (_T("touch at x=%d y=%d\r\n"),
				m_TraceQueue[i].x, m_TraceQueue[i].y));
		}
		pDC->SelectObject(old_brush);
		pDC->SelectObject(old_pen);
	}
#endif
}

void CvncviewerDlg::SetHotkeyHandler(bool set) {
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

LRESULT CALLBACK CvncviewerDlg::SubWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
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

void CvncviewerDlg::HandleMapKey(bool long_press) {
	Client::event_t evt;
	evt.what = Client::EVT_KEY;
	if (long_press) {
		/* long press */
		evt.data.key = Client::KEY_HOME;
	} else {
		/* normal press */
		evt.data.key = Client::KEY_BACK;
	}
	SendEvent(evt);
}

void CvncviewerDlg::SendEvent(Client::event_t &evt) {
	if (m_Client) {
		m_Client->PostEvent(evt);
	}
}

void CvncviewerDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (ID_TIMER_LONG_PRESS == nIDEvent) {
		/* MAP long press */
		KillTimer(ID_TIMER_LONG_PRESS);
		m_FilterAutoRepeat = false;
		m_LongPress = true;
		HandleMapKey(true);
	}
#ifdef SHOW_POINTER_TRACE
	else  if (ID_TIMER_TRACE == nIDEvent) {
		RECT r;

		KillTimer(ID_TIMER_TRACE);
		m_TraceQueue.clear();
		GetWindowRect(&r);
		InvalidateRect(&r, FALSE);
		DEBUGMSG(true, (_T("Invalidate x=%d y=%d w=%d h=%d\r\n"),
			r.left, r.top, r.right, r.bottom));
	}
#endif
	CDialog::OnTimer(nIDEvent);
}

void CvncviewerDlg::Cleanup() {
	SetHotkeyHandler(false);
	KillTimer(ID_TIMER_LONG_PRESS);
#ifdef SHOW_POINTER_TRACE
	KillTimer(ID_TIMER_TRACE);
#endif
	if (m_Client) {
		delete m_Client;
		m_Client = NULL;
	}
	if (m_ConfigStorage) {
		delete m_ConfigStorage;
	}
}

void CvncviewerDlg::OnDestroy()
{
	Cleanup();
	CDialog::OnDestroy();
}

bool CvncviewerDlg::SetupGx()
{
	/* get current window's handle */
	CWnd *cWnd = GetDesktopWindow();
	HWND hWnd = cWnd->GetSafeHwnd();
	/* Initialize GAPI */
	if(GXOpenDisplay(hWnd, 0) == 0 )
	{
		DEBUGMSG(true, (_T("Cannot initialize GAME API with fullscreen\r\n")));
	}
			unsigned char *videoMemory = (unsigned char *)GXBeginDraw(); 
			if (videoMemory == NULL)
			{
				DEBUGMSG(true, (_T("videoMemory is null\r\n")));
			}
			else
			{
				DEBUGMSG(true, (_T("onPaint\r\n")));
				//memset(videoMemory, 0x55, 40000);
				GXEndDraw();
			}
	/* Captures the buttons */
	//GXOpenInput();
	/* Obtain information about the video frame buffer */
	m_DisplayProperties = GXGetDisplayProperties();
	if( !(m_DisplayProperties.cBPP == 16) || !(m_DisplayProperties.ffFormat | kfDirect565))
	{
		DEBUGMSG(true, (_T("Full 16-bit color display is required!\r\n")));
		GXCloseDisplay();
		return false;
	}
	/* Obtain information about the e hardware button assignment */
	//gxKeys = GXGetDefaultKeys(GX_NORMALKEYS);
	return true;
}
