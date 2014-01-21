#pragma once

#ifdef SHOW_POINTER_TRACE
#include <deque>
#endif

class Client;
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

	enum AVIC_HW_BUTTONS {
		HW_BTN_FIRST,
		HW_BTN_MAP,
		HW_BTN_MENU,
		HW_BTN_PUSH,
		HW_BTN_UP,
		HW_BTN_DOWN,
		HW_BTN_LEFT,
		HW_BTN_RIGHT,
		HW_BTN_ROTATE_LEFT,
		HW_BTN_ROTATE_RIGHT,
		HW_BTN_LAST
	};
	static const wchar_t *WND_PROC_NAMES[];
	HWND m_HotkeyHwnd;
	WNDPROC m_HotkeyWndProc;
	bool m_FilterAutoRepeat;
	bool m_LongPress;
	bool m_SwipeActive;
	LONG m_SwipeUpPointX;
	LONG m_SwipeUpPointY;
	enum ID_TIMER {
		ID_TIMER_LONG_PRESS = 1,
		ID_TIMER_SWIPE
#ifdef SHOW_POINTER_TRACE
		, ID_TIMER_TRACE
#endif
	};
	static const UINT ID_TIMER_LONG_PRESS_DELAY = 1000;
	static const UINT ID_TIMER_SWIPE_DELAY = 100;
#ifdef SHOW_POINTER_TRACE
	static const UINT ID_TIMER_TRACE_DELAY = 5000;
#endif

	static const int CONNECT_MAX_TRY = 3;
	Client *vnc_client;
	bool m_RenderingEnabled;

	ConfigStorage *m_ConfigStorage;

	static LRESULT CALLBACK SubWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void SetHotkeyHandler(bool set);
	void HandleMapKey(bool long_press);
	void Cleanup();
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
	afx_msg void OnOK();
	int CvncviewerDlg::Message(DWORD type, wchar_t *caption, wchar_t *format, ...);
};
