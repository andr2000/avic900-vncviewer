#include <string>

#include "rfb/rfb.h"

class EventInjector
{
public:
	EventInjector() = default;
	~EventInjector();

	bool initialize(int width, int height, int rotation);
	void handlePointerEvent(int buttonMask, int x, int y, rfbClientPtr cl);
	void handleKeyEvent(rfbBool down, rfbKeySym key, rfbClientPtr cl);
	void onRotation(int rotation);

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

	struct ACTIVE_AREA
	{
		int x1;
		int y1;
		int x2;
		int y2;
	} m_ActiveArea;

	int m_Width = 0;
	int m_Height = 0;
	int m_TouchSideLength = 0;
	static const int INVALID_HANDLE = -1;
	int m_Fd = INVALID_HANDLE;
	bool m_LeftClicked = false;
	int m_TouchscreenRotation = ROTATION_0;
	int m_DisplayRotation = ROTATION_0;

	float m_TdivW;
	float m_TdivH;
	int m_Tdiv2;
	int m_IgnoreRight;
	int m_IgnoreLeft;
	float m_ScaleX;
	int m_IgnoreTop;
	int m_IgnoreBottom;
	float m_ScaleY;

	void cleanup();
	void reportSync();
	void reportKey(int key, bool press);
	void reportAbs(int code, int value);
	bool transformCoordinates(int inX, int inY, int &outX, int &outY);
};
