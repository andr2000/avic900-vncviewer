// vncviewer.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "vncviewer.h"
#include "vncviewerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const wchar_t *CvncviewerApp::APP_TITLE = TEXT("Avic VNC viewer");

// CvncviewerApp

BEGIN_MESSAGE_MAP(CvncviewerApp, CWinApp)
END_MESSAGE_MAP()


// CvncviewerApp construction
CvncviewerApp::CvncviewerApp()
	: CWinApp()
{
	hMutex = NULL;
}

CvncviewerApp::~CvncviewerApp()
{
	if (hMutex) {
		CloseHandle(hMutex);
	}
}

// The one and only CvncviewerApp object
CvncviewerApp theApp;

// CvncviewerApp initialization

BOOL CvncviewerApp::InitInstance()
{
	static const wchar_t *APP_NAME = TEXT("avic_vncviewer");

	// SHInitExtraControls should be called once during your application's initialization to initialize any
    // of the Windows Mobile specific controls such as CAPEDIT and SIPPREF.
    SHInitExtraControls();

	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	CvncviewerDlg dlg;

	/* allow only single instance */
	hMutex = CreateMutex(NULL, FALSE, APP_NAME);
	if (!hMutex) {
		MessageBox(NULL, TEXT("Failed to create named mutex\r\n") \
			TEXT("Unable to check if single instance is running... Will exit now"),
			TEXT("Error"), MB_OK);
		return FALSE;
	}
	if (ERROR_ALREADY_EXISTS == GetLastError()) {
		HWND hWnd = FindWindow(0, APP_TITLE);
		DEBUGMSG(TRUE, (_T("Found running instance, will exit now\r\n")));
		SetForegroundWindow(hWnd);
		return FALSE;
	}

	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
	}
	return TRUE;
}
