#ifndef _CLIENT_WINCE_H
#define _CLIENT_WINCE_H

#ifdef SHOW_POINTER_TRACE
#include <deque>
#endif

#include <rfb/rfbclient.h>
#include "client.h"
#include "compat.h"

class Client_WinCE : public Client {
public:
	Client_WinCE();
	~Client_WinCE();

	virtual int Initialize(void *_private);
	void SetWindowHandle(HWND hWnd);
	void OnTouchUp(int x, int y);
	void OnTouchDown(int x, int y);
	void OnTouchMove(int x, int y);
	virtual void OnPaint(void);
	void OnActivate(bool isActive);

protected:
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

	HWND m_hWnd;

	RECT m_ServerRect;
	RECT m_ClientRect;

	virtual void OnFrameBufferUpdate(rfbClient *client, int x, int y, int w, int h);
	virtual void OnShutdown();

	long GetTimeMs() {
		return GetTickCount();
	}

	uint8_t *m_FrameBuffer;
	HBITMAP m_hBmp;

	static void Logger(const char *format, ...);
	int ShowMessage(DWORD type, wchar_t *caption, wchar_t *format, ...);

private:
	static const int CONNECT_MAX_TRY = 3;

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

	enum ID_TIMER {
		ID_TIMER_LONG_PRESS = 1,
#ifdef SHOW_POINTER_TRACE
		ID_TIMER_TRACE
#endif
	};

	static const UINT ID_TIMER_LONG_PRESS_DELAY = 1000;
#ifdef SHOW_POINTER_TRACE
	static const UINT ID_TIMER_TRACE_DELAY = 5000;
#endif

	static const int LOG_BUF_SZ = 256;

	static const wchar_t *WND_PROC_NAMES[];

	HWND m_HotkeyHwnd;
	WNDPROC m_HotkeyWndProc;
	bool m_FilterAutoRepeat;
	bool m_LongPress;

	void OnTimer(UINT_PTR nIDEvent);
	static LRESULT CALLBACK SubWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK ClientSubWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void SetHotkeyHandler(bool set);
	void HandleMapKey(bool long_press);

	rfbBool OnMallocFrameBuffer(rfbClient *client);
	void ShowFullScreen();

	void SetLogging();
};

#endif
