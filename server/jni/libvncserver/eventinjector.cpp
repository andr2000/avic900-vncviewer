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
	memset(&uinp, 0, sizeof(uinp));
	uinp.id.version = 4;
	uinp.id.bustype = BUS_USB;
	uinp.absmin[ABS_X] = 0;
	uinp.absmax[ABS_X] = m_Width - 1;
	uinp.absmin[ABS_Y] = 0;
	uinp.absmax[ABS_Y] = m_Height - 1;
	uinp.absmin[ABS_PRESSURE] = 0;
	uinp.absmax[ABS_PRESSURE] = 0xfff;

	strncpy(uinp.name, DEV_NAME, UINPUT_MAX_NAME_SIZE);

	int ret = ioctl(m_Fd, UI_SET_EVBIT, EV_ABS);
	ret = ioctl(m_Fd, UI_SET_ABSBIT, ABS_X);
	ret = ioctl(m_Fd, UI_SET_ABSBIT, ABS_Y);
	ret = ioctl(m_Fd, UI_SET_ABSBIT, ABS_PRESSURE);

	ret = ioctl(m_Fd, UI_SET_EVBIT, EV_KEY);
	for (int i=0; i < 256; i++)
	{
		ret = ioctl(m_Fd, UI_SET_KEYBIT, i);
	}
	ret = ioctl(m_Fd, UI_SET_KEYBIT, BTN_TOUCH);

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
	gettimeofday(&event.time, NULL);

	event.type  = EV_SYN;
	event.code  = SYN_REPORT;
	event.value = 0;
	write(m_Fd, &event, sizeof(event));
}

void EventInjector::reportKey(int key, bool press)
{
	struct input_event event = {0};
	gettimeofday(&event.time, NULL);

	event.type  = EV_KEY;
	event.code  = key;
	event.value = press ? 1 : 0;
	write(m_Fd, &event, sizeof(event));
}

void EventInjector::reportAbs(int code, int value)
{
	struct input_event event = {0};
	gettimeofday(&event.time, NULL);

	event.type  = EV_ABS;
	event.code  = code ;
	event.value = value;
	write(m_Fd, &event, sizeof(event));
}

void EventInjector::handlePointerEvent(int buttonMask, int x, int y, rfbClientPtr cl)
{
	LOGD("handlePointerEvent buttonMask %d x %d y %d", buttonMask, x, y);
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
		reportKey(BTN_TOUCH, true);
		reportAbs(ABS_X, x);
		reportAbs(ABS_Y, y);
		reportAbs(ABS_PRESSURE, 0xff);
		reportSync();
	}
	else if (m_LeftClicked)
	{
		/* left button released */
		m_LeftClicked = false;
		reportAbs(ABS_PRESSURE, 0);
		reportKey(BTN_TOUCH, false);
		reportSync();
	}
}

void EventInjector::handleKeyEvent(rfbBool down, rfbKeySym key, rfbClientPtr cl)
{
	int code;
	bool sh;
	bool alt;

	LOGD("handleKeyEvent down %d key %d (0x%x)", down, key, key);
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
		}
		else
		{
			/* key release */
		}
	}
}
