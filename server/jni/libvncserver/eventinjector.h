#include <string>

#include "rfb/rfb.h"

class EventInjector
{
public:
	EventInjector() = default;
	~EventInjector();

	bool initialize(int width, int height);
	void handlePointerEvent(int buttonMask, int x, int y, rfbClientPtr cl);
	void handleKeyEvent(rfbBool down, rfbKeySym key, rfbClientPtr cl);

private:
	int m_Width = 0;
	int m_Height = 0;
	static const int INVALID_HANDLE = -1;
	int m_Fd = INVALID_HANDLE;
	bool m_LeftClicked = false;

	void cleanup();
	void reportSync();
	void reportKey(int key, bool press);
	void reportAbs(int code, int value);
};
