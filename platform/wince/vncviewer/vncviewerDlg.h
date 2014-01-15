#pragma once

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
	static const UINT ID_TIMER_LONG_PRESS = 1;
	static const UINT ID_TIMER_LONG_PRESS_DELAY = 1000;

	static const int CONNECT_MAX_TRY = 3;
	Client *vnc_client;
	bool m_RenderingEnabled;

	ConfigStorage *m_ConfigStorage;

	static LRESULT CALLBACK SubWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void SetHotkeyHandler(bool set);
	void HandleMapKey(bool long_press);
public:
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
