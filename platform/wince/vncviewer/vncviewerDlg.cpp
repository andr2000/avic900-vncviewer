#include "stdafx.h"
#include "vncviewer.h"
#include "vncviewerDlg.h"

#include "client_factory.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CvncviewerDlg::CvncviewerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CvncviewerDlg::IDD, pParent) {
	vnc_client = NULL;
}

CvncviewerDlg::~CvncviewerDlg() {
	if (vnc_client) {
		/* TODO: stop it */
		delete vnc_client;
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
END_MESSAGE_MAP()


BOOL CvncviewerDlg::OnInitDialog()
{
	int i;
	CDialog::OnInitDialog();

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

	/* enable rendering as wm_showwindow might already has arrived */
	Client::event_t evt;
	evt.what = Client::EVT_SET_RENDERING;
	evt.data.rendering_enabled = 1;
	vnc_client->PostEvent(evt);
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
	}
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

	if (vnc_client) {
		Client::event_t evt;
		evt.what = Client::EVT_SET_RENDERING;
		evt.data.rendering_enabled = nState != WA_INACTIVE;
		vnc_client->PostEvent(evt);
	}
}
