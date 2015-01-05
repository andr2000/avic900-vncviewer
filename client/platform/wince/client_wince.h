#ifndef _CLIENT_WINCE_H
#define _CLIENT_WINCE_H

#include <rfb/rfbclient.h>
#include "client.h"
#include "compat.h"

class Client_WinCE : public Client {
public:
	Client_WinCE();
	~Client_WinCE();

	int Initialize(void *_private);
	void *GetDrawingContext() {
		return static_cast<void *>(m_hBmp);
	};
protected:
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
	};
	static const UINT ID_TIMER_LONG_PRESS_DELAY = 1000;
	static const wchar_t *WND_PROC_NAMES[];

	HWND m_hWnd;

	HWND m_HotkeyHwnd;
	WNDPROC m_HotkeyWndProc;
	bool m_FilterAutoRepeat;
	bool m_LongPress;
	ConfigStorage *m_ConfigStorage;
	RECT m_ServerRect;
	RECT m_ClientRect;
	static const int CONNECT_MAX_TRY = 3;

	void SetLogging();
	rfbBool OnMallocFrameBuffer(rfbClient *client);
	virtual void OnFrameBufferUpdate(rfbClient *client, int x, int y, int w, int h);
	virtual void OnShutdown();
	long GetTimeMs() {
		return GetTickCount();
	}
	static LRESULT CALLBACK SubWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void SetHotkeyHandler(bool set);
	void HandleMapKey(bool long_press);

	void OnTimer(void);

	uint8_t *m_FrameBuffer;
	HBITMAP m_hBmp;

	static const int LOG_BUF_SZ = 256;
	static void Logger(const char *format, ...);
};

#endif
