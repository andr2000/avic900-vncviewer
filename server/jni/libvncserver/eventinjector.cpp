#include <android/log.h>
#include <linux/input.h>

#include "eventinjector.h"
#include "uinput.h"

#include "rfbinput.cxx"

extern "C"
{
	#include "suinput.h"

	#define MODULE_NAME "eventinjector"

	#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  MODULE_NAME, __VA_ARGS__))
	#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__))
	#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__))
}

EventInjector::~EventInjector()
{
	cleanup();
}

bool EventInjector::initialize(int width, int height)
{
	static const int BUS_VIRTUAL = 0x06;
	struct input_id id =
	{
		/* Bus type */
		BUS_VIRTUAL,
		/* Vendor id */
		1,
		/* Product id */
		1,
		/* Version id */
		1
	};
	m_Fd = suinput_open("VncServer", &id);
	return m_Fd != INVALID_HANDLE;
}

void EventInjector::cleanup()
{
	if (m_Fd != INVALID_HANDLE)
	{
		suinput_close(m_Fd);
	}
}

void EventInjector::handlePointerEvent(int buttonMask, int x, int y, rfbClientPtr cl)
{
	//transformTouchCoordinates(&x,&y,cl->screen->width,cl->screen->height);

	if ((buttonMask & 1) && m_LeftClicked)
	{
		/* left btn clicked and moving */
		suinput_write(m_Fd, EV_ABS, ABS_X, x);
		suinput_write(m_Fd, EV_ABS, ABS_Y, y);
		suinput_write(m_Fd, EV_SYN, SYN_REPORT, 0);
	}
	else if (buttonMask & 1)
	{
		/* left btn clicked */
		m_LeftClicked = true;

		suinput_write(m_Fd, EV_ABS, ABS_X, x);
		suinput_write(m_Fd, EV_ABS, ABS_Y, y);
		suinput_write(m_Fd, EV_KEY,BTN_TOUCH,1);
		suinput_write(m_Fd, EV_SYN, SYN_REPORT, 0);
	}
	else if (m_LeftClicked)
	{
		/* left btn released */
		m_LeftClicked = false;
		suinput_write(m_Fd, EV_ABS, ABS_X, x);
		suinput_write(m_Fd, EV_ABS, ABS_Y, y);
		suinput_write(m_Fd, EV_KEY,BTN_TOUCH,0);
		suinput_write(m_Fd, EV_SYN, SYN_REPORT, 0);
	}

	if (buttonMask & 4)
	{
		/* right btn clicked */
		m_RightClicked = true;
		suinput_press(m_Fd, KEY_BACK);
	}
	else if (m_RightClicked)
	{
		/* right button released */
		m_RightClicked = false;
		suinput_release(m_Fd, KEY_BACK);
	}

	if (buttonMask & 2)
	{
		/* mid btn clicked */
		m_MiddleClicked = true;
		suinput_press(m_Fd, KEY_END);
	}
	else if (m_MiddleClicked)
	{
		/* mid btn released */
		m_MiddleClicked = false;
		suinput_release(m_Fd, KEY_END);
	}
}

void EventInjector::handleKeyEvent(rfbBool down, rfbKeySym key, rfbClientPtr cl)
{
	int code;
	bool sh;
	bool alt;

	if ((code = keycode(key, sh, alt, true)))
	{
		if (key && down)
		{
			if (sh)
			{
				suinput_press(m_Fd, KEY_LEFTSHIFT);
			}
			if (alt)
			{
				suinput_press(m_Fd, KEY_LEFTALT);
			}

			suinput_press(m_Fd, code);
			suinput_release(m_Fd, code);

			if (alt)
			{
				suinput_release(m_Fd, KEY_LEFTALT);
			}
			if (sh)
			{
				suinput_release(m_Fd, KEY_LEFTSHIFT);
			}
		}
		else
		{
			/* key release */
		}
	}
}
