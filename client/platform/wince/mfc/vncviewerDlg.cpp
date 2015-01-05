#include "stdafx.h"
#include "vncviewer.h"
#include "vncviewerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CvncviewerDlg *CvncviewerDlg::m_Instance = NULL;

CvncviewerDlg::CvncviewerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CvncviewerDlg::IDD, pParent) {
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

BOOL CvncviewerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText(CvncviewerApp::APP_TITLE);

	ClientFactory::GetInstance()->SetWindow(m_hWnd);
	if (ClientFactory::GetInstance()->Initialize(static_cast<void *>(this) >=0) {
		/* go full screen */
		ClientFactory::GetInstance()->ShowFullScreen();
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

void CvncviewerDlg::OnLButtonUp(UINT nFlags, CPoint point) {
	CDialog::OnLButtonUp(nFlags, point);
	ClientFactory::GetInstance()->OnTouchUp(point.x, point.y);
}

void CvncviewerDlg::OnLButtonDown(UINT nFlags, CPoint point) {
	CDialog::OnLButtonDown(nFlags, point);
	ClientFactory::GetInstance()->OnTouchDown(point.x, point.y);
}

void CvncviewerDlg::OnMouseMove(UINT nFlags, CPoint point) {
	CDialog::OnMouseMove(nFlags, point);
	if (nFlags & MK_LBUTTON) {
		ClientFactory::GetInstance()->OnTouchMove(point.x, point.y);
	}
}

void CvncviewerDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialog::OnActivate(nState, pWndOther, bMinimized);
	ClientFactory::GetInstance()->OnActivate(nState);
}

BOOL CvncviewerDlg::OnEraseBkgnd(CDC* pDC)
{
	/* do nothing, we are full screen and painting by ourselves */
	return true;
}

void CvncviewerDlg::OnPaint()
{
}
