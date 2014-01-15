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
	m_RenderingEnabled = FALSE;
	m_HotkeyHwnd = NULL;
	m_HotkeyWndProc = NULL;
	m_MapKeyPressStartTick = 0;
	m_Instance = this;
	m_ConfigStorage = NULL;
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
END_MESSAGE_MAP()


BOOL CvncviewerDlg::OnInitDialog()
{
	int i;
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

	vnc_client = ClientFactory::GetInstance();
	if (NULL == vnc_client) {
		MessageBox(_T("Failed to intstaniate VNC client\r\nShit happens"),
			_T("Error"), MB_OK);
		PostMessage(WM_CLOSE);
		return TRUE;
	}

	/* go full screen */
	CRect rcDesktop;
	rcDesktop.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
	rcDesktop.right = rcDesktop.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
	rcDesktop.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
	rcDesktop.bottom = rcDesktop.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);
	MoveWindow(rcDesktop, FALSE);

	/* let's rock */
	for (i = 0; i < CONNECT_MAX_TRY; i++) {
		if (0 == vnc_client->Start(static_cast<void *>(this))) {
			break;
		}
		if (IDCANCEL == MessageBox(_T("Failed to connect to the server\r\nRetry?"),
			_T("Error"), MB_RETRYCANCEL)) {
				PostMessage(WM_CLOSE);
				return FALSE;
		}
	}
	if (i == CONNECT_MAX_TRY) {
		MessageBox(_T("Was not able to connect to the VNC server\r\nTerminating now"),
			_T("Error"), MB_OK);
		PostMessage(WM_CLOSE);
		return TRUE;
	}
	/* install handlers to intercept WM_HOTKEY */
	SetHotkeyHandler(TRUE);
	return TRUE;
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
	return TRUE;
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

			DEBUGMSG(TRUE, (_T("OnPaint x=%d y=%d w=%d h=%d\r\n"), x, y, w, h));

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
			DEBUGMSG(TRUE, (_T("Found original hotkey handler: %s\r\n"),
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
			DEBUGMSG(TRUE, (_T("DID NOT find the original hotkey handler\r\n")));
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

	if ((WM_HOTKEY == message) && (HW_BTN_MAP == wParam)){
		if (lParam & 0x1000) {
			/* released */
			DEBUGMSG(TRUE, (_T("MAP released\r\n")));
		} else {
			/* pressed */
			DEBUGMSG(TRUE, (_T("MAP pressed\r\n")));
		}
	}
	/* skip this message and pass it to the adressee */
	return CallWindowProc(dlg->m_HotkeyWndProc,
		hWnd, message, wParam, lParam);;
}

void CvncviewerDlg::HandleMapKey(bool pressed) {
	if (pressed) {
		m_MapKeyPressStartTick = GetTickCount();
	} else {
		DWORD delta_ms = GetTickCount() - m_MapKeyPressStartTick;
		if (vnc_client) {
			Client::event_t evt;
			evt.what = Client::EVT_KEY;
			if (delta_ms >= MAP_LONG_PRESS_TICKS) {
				/* long press */
				evt.data.key = Client::KEY_HOME;
			} else {
				/* normal press */
				evt.data.key = Client::KEY_BACK;
			}
			vnc_client->PostEvent(evt);
		}
	}
}
