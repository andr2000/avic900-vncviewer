#include "stdafx.h"
#include "vncviewer.h"
#include "vncviewerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CvncviewerDlg *CvncviewerDlg::m_Instance = NULL;

CvncviewerDlg::CvncviewerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CvncviewerDlg::IDD, pParent) {
	m_Client = NULL;
	m_Instance = this;
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

	m_Client = ClientFactory::GetInstance();
	if (NULL == m_Client) {
		Message(MB_OK, _T("Error"),
			_T("Failed to intstaniate VNC client\r\nShit happens"));
		PostMessage(WM_CLOSE);
		return true;
	}

	if (m_Client->Initialize(static_cast<void *>(this) >=0) {
		/* go full screen */
		ShowFullScreen();
	}
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
	PAINTSTRUCT ps;
	CDC *pDC = BeginPaint(&ps);

	if (m_Client) {
		CBitmap *bitmap = CBitmap::FromHandle(
			static_cast<HBITMAP>(m_Client->GetDrawingContext()));
		if (bitmap) {
			CDC dcMem;
			LONG x, y, w, h;

			if (m_SetupScaling) {
				int width, height;

				m_SetupScaling = false;
				m_Client->GetScreenSize(width, height);
				m_ServerRect.left = 0;
				m_ServerRect.top = 0;
				m_ServerRect.right = width;
				m_ServerRect.bottom = height;
				DEBUGMSG(true, (_T("Server screen is %dx%d\r\n"), m_ServerRect.right, m_ServerRect.bottom));
				/* decide if we need to fit */
				if ((m_ServerRect.right > m_ClientRect.right) || (m_ServerRect.bottom > m_ClientRect.bottom)) {
					m_Client->SetClientSize(m_ClientRect.right, m_ClientRect.bottom);
					DEBUGMSG(true, (_T("Will fit the screen\r\n")));
					m_NeedScaling = true;
				}
				if ((m_ServerRect.right < m_ClientRect.right) && (m_ServerRect.bottom < m_ClientRect.bottom)) {
					m_Client->SetClientSize(m_ClientRect.right, m_ClientRect.bottom);
					DEBUGMSG(true, (_T("Will expand\r\n")));
					m_NeedScaling = true;
				}
			}
			x = ps.rcPaint.left;
			y = ps.rcPaint.top;
			w = ps.rcPaint.right - ps.rcPaint.left;
			h = ps.rcPaint.bottom - ps.rcPaint.top;

			DEBUGMSG(true, (_T("OnPaint x=%d y=%d w=%d h=%d\r\n"), x, y, w, h));

			dcMem.CreateCompatibleDC(pDC);
			CBitmap *old_bitmap = dcMem.SelectObject(bitmap);
			if (m_NeedScaling) {
				pDC->StretchBlt(m_ClientRect.left, m_ClientRect.top, m_ClientRect.right, m_ClientRect.bottom, &dcMem,
					m_ServerRect.left, m_ServerRect.top, m_ServerRect.right, m_ServerRect.bottom, SRCCOPY);
			} else {
				pDC->BitBlt(x, y, w, h, &dcMem, x, y, SRCCOPY);
			}
			dcMem.SelectObject(old_bitmap);
			dcMem.DeleteDC();
		}
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
	EndPaint(&ps);
}

void CvncviewerDlg::SendEvent(Client::event_t &evt) {
	if (m_Client) {
		m_Client->PostEvent(evt);
	}
}

void CvncviewerDlg::OnTimer(UINT_PTR nIDEvent)
{
#ifdef SHOW_POINTER_TRACE
	if (ID_TIMER_TRACE == nIDEvent) {
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
#ifdef SHOW_POINTER_TRACE
	KillTimer(ID_TIMER_TRACE);
#endif
	if (m_Client) {
		delete m_Client;
		m_Client = NULL;
	}
}

void CvncviewerDlg::OnDestroy()
{
	Cleanup();
	CDialog::OnDestroy();
}
