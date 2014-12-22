#include <atomic>
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
	void onRotation(int rotation);
	void transformCoordinates(int inX, int inY, int &outX, int &outY);

private:
	enum
	{
		/* The angle is the rotation of the drawn graphics on the screen,
		 * which is the opposite direction of the physical rotation of the device
		 */
		ROTATION_0,
		ROTATION_90,
		ROTATION_180,
		ROTATION_270
	};

	int m_Width = 0;
	int m_Height = 0;
	static const int INVALID_HANDLE = -1;
	int m_Fd = INVALID_HANDLE;
	bool m_LeftClicked = false;
	int m_TouchscreenRotation;
	std::atomic<int> m_DisplayRotation;

	void cleanup();
	void reportSync();
	void reportKey(int key, bool press);
	void reportAbs(int code, int value);
};
