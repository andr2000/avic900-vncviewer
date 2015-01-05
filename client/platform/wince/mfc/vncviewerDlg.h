#pragma once

#ifdef SHOW_POINTER_TRACE
#include <deque>
#endif

#include "client_factory.h"

class ConfigStorage;

class CvncviewerDlg : public CDialog {
public:
	CvncviewerDlg(CWnd* pParent = NULL);
	~CvncviewerDlg();

	enum { IDD = IDD_VNCVIEWER_DIALOG };

	static CvncviewerDlg *CvncviewerDlg::GetInstance() {
		return m_Instance;
	}
protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	virtual BOOL OnInitDialog();
#if defined(_DEVICE_RESOLUTION_AWARE) && !defined(WIN32_PLATFORM_WFSP)
	afx_msg void OnSize(UINT /*nType*/, int /*cx*/, int /*cy*/);
#endif
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
private:
	static CvncviewerDlg *m_Instance;

#ifdef SHOW_POINTER_TRACE
	enum ID_TIMER {
		ID_TIMER_TRACE = 1
	};
	static const UINT ID_TIMER_TRACE_DELAY = 5000;
#endif

	Client *m_Client;
	bool m_NeedScaling;
	bool m_SetupScaling;

	void SendEvent(Client::event_t &evt);
	void Cleanup();
	void ShowFullScreen();
#ifdef SHOW_POINTER_TRACE
	enum trace_point_type_e {
		TRACE_POINT_DOWN,
		TRACE_POINT_UP,
		TRACE_POINT_MOVE
	};
	struct trace_point_t {
		LONG x;
		LONG y;
		trace_point_type_e type;
	};
	static const int TRACE_POINT_BAR_SZ = 10;
	std::deque< trace_point_t > m_TraceQueue;
	void AddTracePoint(trace_point_type_e type, LONG x, LONG y);
#endif
public:
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	int CvncviewerDlg::Message(DWORD type, wchar_t *caption, wchar_t *format, ...);
};
