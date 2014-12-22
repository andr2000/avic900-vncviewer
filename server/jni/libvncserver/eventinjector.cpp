#include <android/log.h>
#include <linux/input.h>
#include <fcntl.h>

#include "eventinjector.h"
#include "uinput.h"

#include "rfbinput.cxx"

extern "C"
{
	#define MODULE_NAME "eventinjector"

	#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO,  MODULE_NAME, __VA_ARGS__))
	#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, MODULE_NAME, __VA_ARGS__))
	#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, MODULE_NAME, __VA_ARGS__))
}

EventInjector::~EventInjector()
{
	cleanup();
}

#ifndef UI_SET_PROPBIT
#define UI_SET_PROPBIT _IOW(UINPUT_IOCTL_BASE, 110, int)
#endif
#ifndef INPUT_PROP_DIRECT
#define INPUT_PROP_DIRECT 0x01
#endif
#ifndef BUS_VIRTUAL
#define BUS_VIRTUAL 0x06
#endif

bool EventInjector::initialize(int width, int height)
{
	static const char *DEV_UINPUT = "/dev/uinput";
	static const char *DEV_NAME = "VncServer";
	struct uinput_user_dev uinp;

	m_Fd = open(DEV_UINPUT, O_WRONLY | O_NDELAY);
	if (m_Fd < 0)
	{
		LOGE("Failed to open %s", DEV_UINPUT);
		return -1;
	}

	m_Width = width;
	m_Height = height;
	/* we only support 0 and 90 for the touch screen, coordinates updated
	 * according to display's rotation */
	if (m_Width > m_Height)
	{
		m_TouchscreenRotation = ROTATION_90;
	}
	else
	{
		/* for square resolutions as well */
		m_TouchscreenRotation = ROTATION_0;
	}
	memset(&uinp, 0, sizeof(uinp));
	uinp.id.bustype = BUS_VIRTUAL;
	uinp.absmin[ABS_X] = 0;
	uinp.absmax[ABS_X] = m_Width - 1;
	uinp.absmin[ABS_Y] = 0;
	uinp.absmax[ABS_Y] = m_Height - 1;

	strncpy(uinp.name, DEV_NAME, UINPUT_MAX_NAME_SIZE);

	int ret;
	ret = ioctl(m_Fd, UI_SET_EVBIT, EV_ABS);
	ret = ioctl(m_Fd, UI_SET_EVBIT, EV_SYN);
	ret = ioctl(m_Fd, UI_SET_EVBIT, EV_KEY);

	ret = ioctl(m_Fd, UI_SET_ABSBIT, ABS_X);
	ret = ioctl(m_Fd, UI_SET_ABSBIT, ABS_Y);
	ret = ioctl(m_Fd, UI_SET_KEYBIT, BTN_TOUCH);
	/* this is to make us touchscreen */
	ret = ioctl(m_Fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);
	/* FIXME: the supported range should be KEY_MAX */
	for (int i = 0; i < 0xff; i++)
	{
		ret = ioctl(m_Fd, UI_SET_KEYBIT, i);
	}

	ret = write(m_Fd, &uinp, sizeof(uinp));
	if (ioctl(m_Fd, UI_DEV_CREATE))
	{
		LOGE("Failed to create new uinput device");
		close(m_Fd);
		return -1;
	}
	return m_Fd != INVALID_HANDLE;
}

void EventInjector::cleanup()
{
	if (m_Fd != INVALID_HANDLE)
	{
		close(m_Fd);
	}
}

void EventInjector::reportSync()
{
	struct input_event event = {0};

	event.type  = EV_SYN;
	event.code  = SYN_REPORT;
	event.value = 0;
	write(m_Fd, &event, sizeof(event));
}

void EventInjector::reportKey(int key, bool press)
{
	struct input_event event = {0};

	event.type  = EV_KEY;
	event.code  = key;
	event.value = press ? 1 : 0;
	write(m_Fd, &event, sizeof(event));
}

void EventInjector::reportAbs(int code, int value)
{
	struct input_event event = {0};

	event.type  = EV_ABS;
	event.code  = code ;
	event.value = value;
	write(m_Fd, &event, sizeof(event));
}

void EventInjector::handlePointerEvent(int buttonMask, int inX, int inY, rfbClientPtr cl)
{
	int x, y;

	transformCoordinates(inX, inY, x, y);

	if ((buttonMask & 1) && m_LeftClicked)
	{
		/* left button clicked and moving */
		reportAbs(ABS_X, x);
		reportAbs(ABS_Y, y);
		reportSync();
	}
	else if (buttonMask & 1)
	{
		/* left button clicked */
		m_LeftClicked = true;
		reportAbs(ABS_X, x);
		reportAbs(ABS_Y, y);
		reportKey(BTN_TOUCH, true);
		reportSync();
	}
	else if (m_LeftClicked)
	{
		/* left button released */
		m_LeftClicked = false;
		reportKey(BTN_TOUCH, false);
		reportSync();
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
				reportKey(KEY_LEFTSHIFT, true);
			}
			if (alt)
			{
				reportKey(KEY_LEFTALT, true);
			}

			reportKey(code, true);
			reportKey(code, false);

			if (alt)
			{
				reportKey(KEY_LEFTALT, false);
			}
			if (sh)
			{
				reportKey(KEY_LEFTSHIFT, false);
			}
			reportSync();
		}
		else
		{
			/* key release */
		}
	}
}

void EventInjector::onRotation(int rotation)
{
	m_DisplayRotation = rotation;
}

void EventInjector::transformCoordinates(int inX, int inY, int &outX, int &outY)
{
	if (m_DisplayRotation == ROTATION_0)
	{
		if (m_TouchscreenRotation == ROTATION_0)
		{
		}
		else
		{
		}
	}
	else if (m_DisplayRotation == ROTATION_90)
	{
		if (m_TouchscreenRotation == ROTATION_0)
		{
		}
		else if (m_TouchscreenRotation == ROTATION_90)
		{
			outY = inX * m_Height / m_Width;
			outX = m_Width - inY * m_Width / m_Height;
		}
	}
	else if (m_DisplayRotation == ROTATION_180)
	{
		if (m_TouchscreenRotation == ROTATION_0)
		{
		}
		else
		{
		}
	}
	else if (m_DisplayRotation == ROTATION_270)
	{
		if (m_TouchscreenRotation == ROTATION_0)
		{
		}
		else if (m_TouchscreenRotation == ROTATION_90)
		{
			outY = m_Height - inX * m_Height / m_Width;
			outX = inY * m_Width / m_Height;
			LOGD("m_DisplayRotation == ROTATION_270 m_TouchscreenRotation == ROTATION_90");
		}
	}
}
