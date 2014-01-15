#include "stdafx.h"
#include "vncviewer.h"
#include "vncviewerDlg.h"

#include "client_factory.h"
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
	vnc_client = NULL;
	m_RenderingEnabled = false;
	m_HotkeyHwnd = NULL;
	m_HotkeyWndProc = NULL;
	m_Instance = this;
	m_ConfigStorage = NULL;
	m_FilterAutoRepeat = false;
	m_LongPress = false;
}

CvncviewerDlg::~CvncviewerDlg() {
	if (vnc_client) {
		/* TODO: stop it */
		delete vnc_client;
	}
	if (m_ConfigStorage) {
		delete m_ConfigStorage;
	}
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
END_MESSAGE_MAP()


BOOL CvncviewerDlg::OnInitDialog()
{
	int i;
	CDialog::OnInitDialog();

	SetWindowText(CvncviewerApp::APP_TITLE);

	m_ConfigStorage = new ConfigStorage();

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

	vnc_client = ClientFactory::GetInstance();
	if (NULL == vnc_client) {
		MessageBox(_T("Failed to intstaniate VNC client\r\nShit happens"),
			_T("Error"), MB_OK);
		PostMessage(WM_CLOSE);
		return true;
	}

	/* go full screen */
	CRect rcDesktop;
	rcDesktop.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
	rcDesktop.right = rcDesktop.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
	rcDesktop.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
	rcDesktop.bottom = rcDesktop.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
	MoveWindow(rcDesktop, false);

	/* let's rock */
	for (i = 0; i < CONNECT_MAX_TRY; i++) {
		if (0 == vnc_client->Start(static_cast<void *>(this))) {
			break;
		}
		if (IDCANCEL == MessageBox(_T("Failed to connect to the server\r\nRetry?"),
			_T("Error"), MB_RETRYCANCEL)) {
				PostMessage(WM_CLOSE);
				return false;
		}
	}
	if (i == CONNECT_MAX_TRY) {
		MessageBox(_T("Was not able to connect to the VNC server\r\nTerminating now"),
			_T("Error"), MB_OK);
		PostMessage(WM_CLOSE);
		return true;
	}
	/* install handlers to intercept WM_HOTKEY */
	SetHotkeyHandler(true);
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


void CvncviewerDlg::OnLButtonUp(UINT nFlags, CPoint point) {
	CDialog::OnLButtonUp(nFlags, point);

	if (vnc_client) {
		Client::event_t evt;
		evt.what = Client::EVT_MOUSE;
		evt.data.point.is_down = 0;
		evt.data.point.x = point.x;
		evt.data.point.y = point.y;
		vnc_client->PostEvent(evt);
	}
}

void CvncviewerDlg::OnLButtonDown(UINT nFlags, CPoint point) {
	CDialog::OnLButtonDown(nFlags, point);

	if (vnc_client) {
		Client::event_t evt;
		evt.what = Client::EVT_MOUSE;
		evt.data.point.is_down = 1;
		evt.data.point.x = point.x;
		evt.data.point.y = point.y;
		vnc_client->PostEvent(evt);
	}
}

void CvncviewerDlg::OnMouseMove(UINT nFlags, CPoint point) {
	CDialog::OnMouseMove(nFlags, point);

	if (vnc_client && (nFlags & MK_LBUTTON)) {
		Client::event_t evt;
		evt.what = Client::EVT_MOVE;
		evt.data.point.is_down = 1;
		evt.data.point.x = point.x;
		evt.data.point.y = point.y;
		vnc_client->PostEvent(evt);
	}
}

void CvncviewerDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialog::OnActivate(nState, pWndOther, bMinimized);

	m_RenderingEnabled = nState != WA_INACTIVE;
	SetHotkeyHandler(m_RenderingEnabled);
}

BOOL CvncviewerDlg::OnEraseBkgnd(CDC* pDC)
{
	/* do nothing, we are full screen and painting by ourselves */
	return true;
}

void CvncviewerDlg::OnPaint()
{
	if (m_RenderingEnabled && vnc_client) {
		CBitmap *bitmap = CBitmap::FromHandle(
			static_cast<HBITMAP>(vnc_client->GetDrawingContext()));
		if (bitmap) {
			PAINTSTRUCT ps;
			CDC dcMem;
			LONG x, y, w, h;
			CDC *pDC = BeginPaint(&ps);

			x = ps.rcPaint.left;
			y = ps.rcPaint.top;
			w = ps.rcPaint.right - ps.rcPaint.left;
			h = ps.rcPaint.bottom - ps.rcPaint.top;

			DEBUGMSG(true, (_T("OnPaint x=%d y=%d w=%d h=%d\r\n"), x, y, w, h));

			dcMem.CreateCompatibleDC(pDC);
			CBitmap *old_bitmap = dcMem.SelectObject(bitmap);
			pDC->BitBlt(x, y, w, h, &dcMem, x, y, SRCCOPY);
			dcMem.SelectObject(old_bitmap);
			dcMem.DeleteDC();
			EndPaint(&ps);
		}
	}
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

	if (dlg && (WM_HOTKEY == message) && (HW_BTN_MAP == LOWORD(wParam))){
		if (LOWORD(lParam) == 0x0) {
			/* pressed */
			DEBUGMSG(true, (_T("MAP pressed\r\n")));
			if (!dlg->m_FilterAutoRepeat && !dlg->m_LongPress) {				/* start processing long press */				dlg->SetTimer(ID_TIMER_LONG_PRESS, ID_TIMER_LONG_PRESS_DELAY, NULL);				dlg->m_FilterAutoRepeat = true;			}			return 1;
		} else if (LOWORD(lParam) == 0x1000) {
			/* released */
			DEBUGMSG(true, (_T("MAP released\r\n")));
			if (dlg->m_FilterAutoRepeat) {				dlg->m_FilterAutoRepeat = false;				dlg->KillTimer(ID_TIMER_LONG_PRESS);			}			if (dlg->m_LongPress) {				/* already handled by the timer */				dlg->m_LongPress = false;				return 1;			}
			dlg->HandleMapKey(false);
		}
		/* eat the key */
		return 1;
	}
	/* skip this message and pass it to the adressee */
	return CallWindowProc(dlg->m_HotkeyWndProc,
		hWnd, message, wParam, lParam);;
}

void CvncviewerDlg::HandleMapKey(bool long_press) {
	if (vnc_client) {
		Client::event_t evt;
		evt.what = Client::EVT_KEY;
		if (long_press) {
			/* long press */
			evt.data.key = Client::KEY_HOME;
		} else {
			/* normal press */
			evt.data.key = Client::KEY_BACK;
		}
		vnc_client->PostEvent(evt);
	}
}

void CvncviewerDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (ID_TIMER_LONG_PRESS == nIDEvent) {		/* MAP long press */		KillTimer(ID_TIMER_LONG_PRESS);		m_FilterAutoRepeat = false;
		m_LongPress = true;
		HandleMapKey(true);
	}	CDialog::OnTimer(nIDEvent);
}
